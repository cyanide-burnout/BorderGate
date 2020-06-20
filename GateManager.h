#ifndef GATEMANAGER_H
#define GATEMANAGER_H

#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include "DStar.h"
#include "GateModule.h"

#define SESSION_ACTIVITY_TIMEOUT  20

class GateManager : public GateModule::Observer
{
  public:

    enum Capability
    {
      Basic,
      Extended,
      Full
    };

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    GateManager(const char* call, GateModule** modules, Capability capability, char** announces, char** outcasts = NULL);

    GateModule** getModuleList();
    void addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except);
    void processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except);
    void doBackgroundActivity(int type);
    void removeAllModules();

    bool hasLimitExceeded();
    int getOperationCount();

    void setCallVisibility(const char* station, bool state);

    void invokeModuleCommand(char name, const char* command);

    ReportHandler onReport;

  private:

    enum Scope
    {
      All,
      Group,
      LocalAndGroup
    };

    void report(int priority, const char* format, ...);
    void considerOperation(int number = 1);

    bool isBanned(const char* call);
    void publishHeard(char origin, const char* area, const char* call1, const char* call2, Scope scope);
    void handleRoutingUpdate(GateModule* module, const char* repeater, const char* gateway);
    void handleLocationUpdate(GateModule* module, const char* station, const char* repeater);
    void handleVisibilityUpdate(GateModule* module, const char* station, bool state);

    time_t time;
    size_t count;

    Capability capability;
    char call[LONG_CALLSIGN_LENGTH];

    GateModule** modules;
    char** announces;
    char** outcasts;
};

#endif
