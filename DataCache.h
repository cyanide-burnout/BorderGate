#ifndef DATACACHE_H
#define DATACACHE_H

#include <stddef.h>
#include <stdarg.h>
#include <libmemcached/memcached.h>

class DataCache
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    DataCache(const char* options);
    ~DataCache();

    const char* error();

    bool add(const char* key, const char* value, size_t length);

    ReportHandler onReport;

  private:

    memcached_st* context;

    void report(int priority, const char* format, ...);

};

#endif