#include "DStarTools.h"
#include "RP2CDirectory.h"
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

// http://db0fhn.efi.fh-nuernberg.de/doku.php?id=projects:dstar:ircddb:ircddb-windows-g4klx

#define RP2C_UPDATE_INTERVAL  300
#define RP2C_DATE_LENGTH      32
#define RP2C_STATE_POSITION   (RP2C_DATE_LENGTH - 1)

RP2CDirectory::RP2CDirectory(const char* parameters, const char* file) :
  onUpdateData(NULL),
  onReport(NULL),
  connection(NULL),
  result(NULL),
  date(NULL),
  time(0)
{
  pthread_mutex_init(&lock, NULL);
  this->parameters = strdup(parameters);

  handle = open(file, O_CREAT | O_RDWR, 0644);

  if (handle != EOF)
  {
    struct stat status;
    fstat(handle, &status);
    if (status.st_size < RP2C_DATE_LENGTH)
    {
      char buffer[RP2C_DATE_LENGTH];
      memset(buffer, ' ', RP2C_DATE_LENGTH);
      strcpy(buffer, "2001-01-01 00:00:00");
      write(handle, buffer, RP2C_DATE_LENGTH);
    }
    date = (char*)mmap(NULL, RP2C_DATE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
  }

  updateStateIndicator();
}

RP2CDirectory::~RP2CDirectory()
{
  if (result != NULL)
    PQclear(result);
  if (connection != NULL)
    PQfinish(connection);

  if ((date != NULL) && (date != MAP_FAILED))
    munmap(date, RP2C_DATE_LENGTH);
  if (handle != EOF)
    close(handle);

  free(parameters);
  pthread_mutex_destroy(&lock);
}

void RP2CDirectory::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

void RP2CDirectory::updateStateIndicator()
{
  date[RP2C_STATE_POSITION] = (connection != NULL) ? '*' : ' ';
  msync(date, RP2C_DATE_LENGTH, MS_ASYNC);
}

void RP2CDirectory::connect()
{
  pthread_mutex_lock(&lock);
  connection = PQconnectdb(parameters);
  if (PQstatus(connection) != CONNECTION_OK)
  {
    char* error = PQerrorMessage(connection);
    report(LOG_ERR, "PostgreSQL error: %s\n", error);
    PQfinish(connection);
    connection = NULL;
  }
  pthread_mutex_unlock(&lock);
  updateStateIndicator();
}

void RP2CDirectory::disconnect()
{
  pthread_mutex_lock(&lock);
  if (result != NULL)
  {
    PQclear(result);
    result = NULL;
  }
  if (connection != NULL)
  {
    PQfinish(connection);
    connection = NULL;
  }
  pthread_mutex_unlock(&lock);
  updateStateIndicator();
}

PGresult* RP2CDirectory::execute(const char* query, const char* parameter)
{
  pthread_mutex_lock(&lock);

  const char* values[] =
  {
    parameter
  };

  PGresult* result = PQexecParams(connection, query, 1, NULL, values, NULL, NULL, 0);

  pthread_mutex_unlock(&lock);
  return result;
}

bool RP2CDirectory::isConnected()
{
  return (connection != NULL);
}

bool RP2CDirectory::checkConnectionError()
{
  if (handle < 0)
  {
    report(LOG_CRIT, "Error opening the file\n");
    return true;
  }
  if ((date == NULL) || (date == MAP_FAILED))
  {
    report(LOG_CRIT, "Error mapping the file\n");
    return true;
  }
  if ((connection != NULL) && (PQstatus(connection) != CONNECTION_OK))
  {
    char* error = PQerrorMessage(connection);
    report(LOG_ERR, "PostgreSQL error: %s\n", error);
    return true;
  }
  return false;
}

bool RP2CDirectory::checkConnectionError(PGresult* result)
{
  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    char* error = PQerrorMessage(connection);
    report(LOG_ERR, "PostgreSQL error: %s\n", error);
    return true;
  }
  return false;
}

bool RP2CDirectory::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  bool outcome = false;
  if (connection != NULL)
  {
    char filter[LONG_CALLSIGN_LENGTH + 1];
    CopyDStarCall(call, filter, NULL, COPY_MAKE_STRING);

    PGresult* result = execute(
      "SELECT 'CQCQCQ  ', arearp_cs, sync_gip.zonerp_cs, zonerp_ipaddr FROM sync_mng "
      "JOIN sync_gip ON sync_gip.zonerp_cs = sync_mng.zonerp_cs WHERE arearp_cs = $1::character(8) "
      "UNION "
      "SELECT target_cs, arearp_cs, sync_gip.zonerp_cs, zonerp_ipaddr FROM sync_mng "
      "JOIN sync_gip ON sync_gip.zonerp_cs = sync_mng.zonerp_cs WHERE target_cs = $1::character(8) "
      "LIMIT 1",
      filter);

    if (PQntuples(result) > 0)
    {
      CopyDStarCall(PQgetvalue(result, 0, 0), route->yourCall, NULL, 0);
      CopyDStarCall(PQgetvalue(result, 0, 1), route->repeater2, NULL, 0);
      CopyDStarCall(PQgetvalue(result, 0, 2), route->repeater1, NULL, 0);
      route->repeater1[LONG_CALLSIGN_LENGTH - 1] = 'G';
      inet_aton(PQgetvalue(result, 0, 3), address);
      outcome = true;
    }

    checkConnectionError(result);
    PQclear(result);
  }
  return outcome;
}

bool RP2CDirectory::verifyRepeater(const char* call)
{
  bool outcome = false;
  if (connection != NULL)
  {
    char filter[LONG_CALLSIGN_LENGTH + 1];
    CopyDStarCall(call, filter, NULL, COPY_MAKE_STRING);

    PGresult* result = execute("SELECT zonerp_cs FROM sync_mng WHERE arearp_cs = $1::character(8) LIMIT 1", filter);

    outcome = (PQntuples(result) > 0);
    checkConnectionError(result);
    PQclear(result);
  }
  return outcome;
}

bool RP2CDirectory::verifyAddress(const struct in_addr& address)
{
  bool outcome = false;
  if (connection != NULL)
  {
    char* filter = inet_ntoa(address);

    PGresult* result = execute("SELECT zonerp_cs FROM sync_gip WHERE zonerp_ipaddr = $1::inet LIMIT 1", filter);

    outcome = (PQntuples(result) > 0);
    checkConnectionError(result);
    PQclear(result);
  }
  return outcome;
}

void RP2CDirectory::doBackgroundActivity()
{
  time_t now = ::time(NULL);

  if ((result == NULL) && (now > time))
  {
    time = now + RP2C_UPDATE_INTERVAL;
    number = 0;

    result = execute("SELECT mod_date, target_cs, arearp_cs, zonerp_cs FROM sync_mng "
      "WHERE mod_date > $1::char(26) ORDER BY mod_date", date);

    if (checkConnectionError(result))
    {
      PQclear(result);
      result = NULL;
    }
  }

  if (result != NULL)
  {
    int count = PQntuples(result);
    if ((number < count) && (onUpdateData != NULL))
    {
      strcpy(date, PQgetvalue(result, number, 0));
      char* call1 = PQgetvalue(result, number, 1);
      char* call2 = PQgetvalue(result, number, 2);
      char* call3 = PQgetvalue(result, number, 3);
      onUpdateData(this, date, call1, call2, call3);
      report(LOG_DEBUG, "%s: %s %s %s %s\n", PQdb(connection), date, call1, call2, call3);
      number ++;
    }
    else
    {
      msync(date, RP2C_DATE_LENGTH, MS_ASYNC);
      PQclear(result);
      result = NULL;
    }
  }
}