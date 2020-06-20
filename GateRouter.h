#ifndef GATEROUTER_H
#define GATEROUTER_H

#include <stdint.h>
#include <stdarg.h>
#include "DStar.h"
#include "RP2C.h"
#include "GateLink.h"
#include "GateModule.h"
#include "GateSession.h"

#define SESSION_ACTIVITY_TIMEOUT  20

class GateRouter : public GateLink::Observer
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    GateRouter(const char* address, const char* call, GateModule** modules);
    ~GateRouter();

    void doBackgroundActivity();

    ReportHandler onReport;

  private:

    uint16_t enumeration;
    char call[LONG_CALLSIGN_LENGTH + 1];

    GateModule** modules;
    GateSession* sessions;

    void report(int priority, const char* format, ...);

    GateSession* createSession(
      GateModule* module, const struct DStarRoute& heard, const struct DStarRoute& target,
      uint16_t number, const struct sockaddr_in& source, const struct sockaddr_in& destination);
    GateSession* findSession(const struct sockaddr_in& address, uint16_t number);
    void removeExpiredSessions(time_t time, time_t interval = SESSION_ACTIVITY_TIMEOUT);
    void removeSession(GateSession* session);
    void removeAllSessions();

    GateModule* getModule(char name);
    GateModule* getModuleByRepeater(const char* call);
    GateModule* getDestinationModule(struct DStarRoute* route);
    bool verifyAddress(const struct in_addr& address);

    void processData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, struct DStarRoute* route, struct DStarDVFrame* frame);
};

#endif
