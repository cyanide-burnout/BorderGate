#ifndef RP2CCONTROLLER_H
#define RP2CCONTROLLER_H

#include <time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include "DStar.h"
#include "RP2C.h"

class RP2CEmulator
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    RP2CEmulator(const char* address, int port);
	~RP2CEmulator();

    void publishHeard(const struct DStarRoute& route);

    void receiveData();
    void doBackgroundActivity();

    int getHandle();
    bool isConnected();
    bool assertConnection();

    void initiateConnection(const struct sockaddr_in& address);

    ReportHandler onReport;

  private:

    int handle;
    time_t time1;
    time_t time2;
    uint16_t number;
    struct sockaddr_in address;

    void report(int priority, const char* format, ...);

    void poll();

    void handleInitialMessage(const struct sockaddr_in& address, struct DSTR* data);
    void handleControllerMessage(const struct sockaddr_in& address, struct DSTR* data);

};

#endif