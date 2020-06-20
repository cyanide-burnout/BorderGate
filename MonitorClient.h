#ifndef MONITORCLIENT_H
#define MONITORCLIENT_H

#include <stddef.h>
#include <stdarg.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include "DStar.h"

#define MONITOR_QUEUE_SIZE  80

class MonitorClient
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    MonitorClient(const char* file);
    ~MonitorClient();

    bool checkConnectionError();

    void publishHeard(const struct DStarRoute& route);
    void doBackgroundActivity();

    ReportHandler onReport;

  private:

    MYSQL* connection;

	pthread_mutex_t lock;
    char* queue[MONITOR_QUEUE_SIZE];
    volatile size_t start;
    volatile size_t end;

	void report(int priority, const char* format, ...);

    bool pushQueue(char* query);
    char* popQueue();

};

#endif