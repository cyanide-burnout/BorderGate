#ifndef RP2CMODULE_H
#define RP2CMODULE_H

#include <libconfig.h>
#include "GateModule.h"
#include "RP2CEmulator.h"
#include "RP2CDirectory.h"
#include "MonitorClient.h"
#include "DataCache.h"

class RP2CModule : public GateModule
{
  public:

    RP2CModule(config_setting_t* setting, GateLink* link, Observer* observer, ReportHandler handler, GateModule* next);
    ~RP2CModule();

    char getName();
    bool isActive();
    GateModule* getNext();

    GateLink* getLink();
    GateContext* getContext();

    void addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except);
    void processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except);

    bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address);

    bool verifyRepeater(const char* call);
    bool verifyAddress(const struct in_addr& address);

    void publishHeard(const struct DStarRoute& route, const char* addressee, const char* text);
    void publishHeard(const struct DStarRoute& route, uint16_t number);

    void doBackgroundActivity(int type);

    void handleCommand(const char* command);

  private:

    char name;
    GateLink* link;
    RP2CEmulator* emulator;
    RP2CDirectory* directory;
    MonitorClient* client;
    Observer* observer;
    DataCache* cache;
    GateModule* next;

    void handleUpdateData(const char* date, const char* station, const char* repeater, const char* gateway);
    static void handleUpdateData(RP2CDirectory* directory, const char* date, const char* call1, const char* call2, const char* call3);
};

#endif