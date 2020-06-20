#ifndef GATEMODULE_H
#define GATEMODULE_H

#include <stdarg.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "DStar.h"
#include "GateLink.h"
#include "GateContext.h"

#define REAL_TIME_ACTIVITY  0
#define ROUTINE_ACTIVITY    1

class GateModule
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    class Observer
    {
      public:
        virtual void handleRoutingUpdate(GateModule* module, const char* repeater, const char* gateway) = 0;
        virtual void handleLocationUpdate(GateModule* module, const char* station, const char* repeater) = 0;
        virtual void handleVisibilityUpdate(GateModule* module, const char* station, bool visible) = 0;
    };

    virtual char getName() = 0;
    virtual bool isActive() = 0;
    virtual GateModule* getNext() = 0;

    virtual GateLink* getLink() = 0;
    virtual GateContext* getContext() = 0;

    virtual void addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except) = 0;
    virtual void processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except) = 0;

    virtual bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address) = 0;

    virtual bool verifyRepeater(const char* call) = 0;
    virtual bool verifyAddress(const struct in_addr& address) = 0;

    virtual void publishHeard(const struct DStarRoute& route, const char* addressee, const char* text) = 0;
    virtual void publishHeard(const struct DStarRoute& route, uint16_t number) = 0;

    virtual void doBackgroundActivity(int type) = 0;

    virtual void handleCommand(const char* command) = 0;

};

#endif