#ifndef ZONEMODULE_H
#define ZONEMODULE_H

#include <libconfig.h>
#include "GateModule.h"
#include "GateContext.h"

class ZoneModule : public GateModule
{
  public:

    ZoneModule(config_setting_t* setting, GateLink* link, GateModule* next);
    ~ZoneModule();

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
    GateModule* next;
    GateContext* context;
};

#endif