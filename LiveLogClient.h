#ifndef LIVELOGCLIENT_H
#define LIVELOGCLIENT_H

#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <curl/curl.h>
#include "DStar.h"

#define LIVELOG_DATE_LENGTH  14

#pragma pack(push, 1)
extern "C"
{
  struct LiveLogState
  {
    unsigned long number;
    char date[LIVELOG_DATE_LENGTH];
  };
};
#pragma pack(pop)

class LiveLogClient
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);
    typedef void (*UpdateDataHandler)(LiveLogClient* client, const char* call, const char* repeater);

    LiveLogClient(const char* location, const char* file);
    ~LiveLogClient();

    void doBackgroundActivity();

    ReportHandler onReport;
    UpdateDataHandler onUpdateData;

    void* userData;

  private:

    int handle;
    struct LiveLogState* state;

    CURL* client;

    time_t time;
    long number;

    void report(int priority, const char* format, ...);

    void parseLine(char* line, size_t* count);
    void parseResponse(char* data, size_t length);
    static size_t handleResponse(char* data, size_t size, size_t count, void* context);
};

#endif
