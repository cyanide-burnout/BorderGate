#include "MonitorClient.h"
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <time.h>

#define DSM_DATE_LENGTH  19

MonitorClient::MonitorClient(const char* file) :
  onReport(NULL),
  start(0),
  end(0)
{
  connection = mysql_init(NULL);

  mysql_options(connection, MYSQL_READ_DEFAULT_FILE, realpath(file, NULL));
  mysql_real_connect(connection, NULL, NULL, NULL, NULL, 0, NULL, CLIENT_REMEMBER_OPTIONS);

  my_bool active = true;
  mysql_options(connection, MYSQL_OPT_RECONNECT, &active);

  pthread_mutex_init(&lock, NULL);
}

MonitorClient::~MonitorClient()
{
  pthread_mutex_destroy(&lock);
  while (start != end)
  {
    free(queue[start]);
    start ++;
    start %= MONITOR_QUEUE_SIZE;
  }
  mysql_close(connection);
}

void MonitorClient::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

bool MonitorClient::checkConnectionError()
{
  const char* error = mysql_error(connection);
  if ((error != NULL) && (*error != '\0'))
  {
    report(LOG_CRIT, "Error in creating DSM connection: %s\n", error);
    return true;
  }
  return false;
}

bool MonitorClient::pushQueue(char* value)
{
  bool outcome = false;
  pthread_mutex_lock(&lock);
  size_t coming = end + 1;
  coming %= MONITOR_QUEUE_SIZE;
  if (coming != start)
  {
    queue[end] = value;
    outcome = true;
    end = coming;
  }
  pthread_mutex_unlock(&lock);
  return outcome;
}

char* MonitorClient::popQueue()
{
  char* outcome = NULL;
  pthread_mutex_lock(&lock);
  if (start != end)
  {
    outcome = queue[start];
    start ++;
    start %= MONITOR_QUEUE_SIZE;
  }
  pthread_mutex_unlock(&lock);
  return outcome;
}

/*
  DSM/DStarJDBC.java:

  lastHeard:
    UPDATE LastHeard SET ReportTime=?, RepeaterCall=? WHERE StationCall=?

  lastHeardRF:
    UPDATE LastHeard SET ReportTime=?, RepeaterCall=?, XmtType=?, iXmtType=?, Flag1=?, Flag2=?, Flag3=?, DestRptr=?, SrcRptr=?, DestStn=?, SrcStn=?, SrcStnExt=?, Length=?
      WHERE StationCall=?

  lastHeardInsert:
    INSERT INTO LastHeard (ReportTime, StationCall, RepeaterCall, XmtType, iXmtType, Flag1, Flag2, Flag3, DestRptr, SrcRptr, DestStn, SrcStn, SrcStnExt, Length)
      VALUES (?, ?, ?, ' ', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)

  lastHeardInsertRF:
    INSERT INTO LastHeard (ReportTime, StationCall, RepeaterCall, XmtType, iXmtType, Flag1, Flag2, Flag3, DestRptr, SrcRptr, DestStn, SrcStn, SrcStnExt, Length)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
*/

void MonitorClient::publishHeard(const struct DStarRoute& route)
{
  char date[DSM_DATE_LENGTH + 1];
  time_t now = ::time(NULL);
   struct tm* time = gmtime(&now);
   strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", time);

  char* query = NULL;
  asprintf(&query,
    "INSERT INTO LastHeard (ReportTime, StationCall, RepeaterCall, XmtType) "
    "VALUES ('%s', '%.8s', '%.8s', ' ') "
    "ON DUPLICATE KEY UPDATE "
    "ReportTime = '%s', RepeaterCall='%.8s'",
    date, route.ownCall1, route.repeater1,
    date, route.repeater1);

  if (!pushQueue(query))
  {
    free(query);
    report(LOG_ERR, "Maximum queue size exceeded\n");
  }
}

void MonitorClient::doBackgroundActivity()
{
  char* query;
  while (query = popQueue())
  {
    report(LOG_DEBUG, "%s: UPDATE\n", mysql_get_host_info(connection));
    if (mysql_query(connection, query) != 0)
    {
      const char* error = mysql_error(connection);
      report(LOG_ERR, "Error querying DSM: %s\n", error);
    }
    free(query);
  }
}
