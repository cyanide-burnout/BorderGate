#ifndef RP2CDIRECTORY_H
#define RP2CDIRECTORY_H

#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <netinet/in.h>
#include <libpq-fe.h>
#include "DStar.h"

class RP2CDirectory
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);
    typedef void (*UpdateDataHandler)(RP2CDirectory* directory, const char* date, const char* call1, const char* call2, const char* call3);


    RP2CDirectory(const char* parameters, const char* file);
    ~RP2CDirectory();

    void connect();
    void disconnect();

    bool isConnected();
    bool checkConnectionError();
    bool checkConnectionError(PGresult* result);
 
    bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address);

    bool verifyRepeater(const char* call);
    bool verifyAddress(const struct in_addr& address);

    void doBackgroundActivity();

    ReportHandler onReport;
    UpdateDataHandler onUpdateData;

    void* userData;

  private:

    char* parameters;
    pthread_mutex_t lock;
    PGconn* volatile connection;

    int handle;
    char* date;

    PGresult* result;
    time_t time;
    int number;

    void report(int priority, const char* format, ...);

    void updateStateIndicator();
    PGresult* execute(const char* query, const char* parameter);
};

#endif