#include "DataCache.h"
#include <string.h>
#include <syslog.h>

DataCache::DataCache(const char* options) :
  onReport(NULL)
{
  context = memcached(options, strlen(options));
}

DataCache::~DataCache()
{
  memcached_free(context);
}

void DataCache::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

const char* DataCache::error()
{
  const char* message = memcached_last_error_message(context);
  if ((message != NULL) && (strcmp(message, "SUCCESS") != 0))
  {
    report(LOG_ERR, "Cache error: %s\n", message);
    return message;
  }
  return NULL;
}

bool DataCache::add(const char* key, const char* value, size_t length)
{
  memcached_return_t result = memcached_add(context, key, strlen(key), value, length, 0, 0);
  if ((result != MEMCACHED_SUCCESS) &&
      (result != MEMCACHED_NOTSTORED) &&
      (result != MEMCACHED_DATA_EXISTS))
  {
    const char* message = memcached_last_error_message(context);
    report(LOG_ERR, "Cache error: %d %s\n", result, message);
  }
  return result == MEMCACHED_SUCCESS;
}
