#include "LiveLogClient.h"
#include "StringTools.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define LIVELOG_STATE_SIZE        sizeof(struct LiveLogState)
#define LIVELOG_UPDATE_INTERVAL   15
#define LIVELOG_UPDATE_LIMIT      10
#define LIVELOG_UPDATE_THRESHOLD  100000

LiveLogClient::LiveLogClient(const char* location, const char* file) :
  onUpdateData(NULL),
  onReport(NULL),
  state(NULL)
{
  time = ::time(NULL) + LIVELOG_UPDATE_INTERVAL;

  client = curl_easy_init();
  curl_easy_setopt(client, CURLOPT_WRITEFUNCTION, handleResponse);
  curl_easy_setopt(client, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(client, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(client, CURLOPT_URL, location);

  handle = open(file, O_CREAT | O_RDWR, 0644);

  if (handle != EOF)
  {
    struct stat status;
    fstat(handle, &status);
    if (status.st_size < LIVELOG_STATE_SIZE)
    {
      char buffer[LIVELOG_STATE_SIZE];
      memset(buffer, 0, LIVELOG_STATE_SIZE);
      write(handle, buffer, LIVELOG_STATE_SIZE);
    }
    state = (struct LiveLogState*)mmap(NULL, LIVELOG_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
  }
}

LiveLogClient::~LiveLogClient()
{
  if ((state != NULL) && (state != MAP_FAILED))
    munmap(state, LIVELOG_STATE_SIZE);
  if (client != NULL)
    curl_easy_cleanup(client);
  if (handle != EOF)
    close(handle);
}

void LiveLogClient::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

void LiveLogClient::doBackgroundActivity()
{
  time_t now = ::time(NULL);
  if (now > time)
  {
    CURLcode code = curl_easy_perform(client);
    if (code != CURLE_OK)
    {
      const char* location = NULL;
      const char* error = curl_easy_strerror(code);
      curl_easy_getinfo(client, CURLINFO_EFFECTIVE_URL, &location);
      report(LOG_ERR, "Error requesting URL %s: %s\n", location, error);
    }
    time = now + LIVELOG_UPDATE_INTERVAL;
  }
}

void LiveLogClient::parseLine(char* line, size_t* count)
{
  const char* location = NULL;
  curl_easy_getinfo(client, CURLINFO_EFFECTIVE_URL, &location);

  unsigned long number;
  char date[LIVELOG_DATE_LENGTH];
  char call[LONG_CALLSIGN_LENGTH];
  char target[LONG_CALLSIGN_LENGTH];
  char repeater[LONG_CALLSIGN_LENGTH];

  //    338743:20121114060319DB0SAB_ADB0SAB_*2DB0SAB_ACQCQCQ__000000RPTRD1________CCS020_______ Logout_____CCS020__  ....
  //    298174:20140503083428SM7IKJ__DCS010_B2SK7RMQ_CCQCQCQ__0000003366D1________CCS010___________________________  3366
  //  13779673:20150824102511ON6FV___DCS007_B2ON6FV__BCQCQCQ__000000P___D1________CCS011___________________________2060079 

  if ((sscanf(line, "%ld:%14s%8s%8s2%8s", &number, date, call, target, repeater) == 5) &&
      ((number > state->number) ||
      ((state->number - number) > LIVELOG_UPDATE_THRESHOLD)))
  {
    report(LOG_DEBUG, "%s: %s\n", location, line);

    if ((target[MODULE_NAME_POSITION] != '*') &&
        (strncmp(call, "STN", 3) != 0) &&
        (strncmp(call, "REF", 3) != 0) &&
        (strncmp(call, "XRF", 3) != 0) &&
        (strncmp(call, "DCS", 3) != 0))
    {
      replace(call, '_', ' ', LONG_CALLSIGN_LENGTH);
      replace(repeater, '_', ' ', LONG_CALLSIGN_LENGTH);
      if (onUpdateData != NULL)
      {
        onUpdateData(this, call, repeater);
        (*count) ++;
      }
    }

    state->number = number;
    memcpy(state->date, date, LIVELOG_DATE_LENGTH);
    msync(date, LIVELOG_STATE_SIZE, MS_ASYNC);
  }
}

void LiveLogClient::parseResponse(char* data, size_t length)
{
  size_t count = 0;
  while ((length > 0) && (count < LIVELOG_UPDATE_LIMIT))
  {
    char* pointer = find(data, length, '\n');

    if (pointer == NULL)
      break;

    *pointer = '\0';
    parseLine(data, &count);

    pointer ++;
    length -= pointer - data;
    data = pointer;
  }
}

size_t LiveLogClient::handleResponse(char* data, size_t size, size_t count, void* context)
{
  LiveLogClient* self = (LiveLogClient*)context;
  size_t length = size * count;
  self->parseResponse(data, length);
  return length;
}
