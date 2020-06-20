#ifndef CCSMODULE_H
#define CCSMODULE_H

#include <libconfig.h>
#include "CCSLink.h"
#include "GateModule.h"
#include "GateContext.h"
#include "LiveLogClient.h"

class CCSModule : public GateModule
{
  public:

    CCSModule(config_setting_t* setting, CCSLink* link, Observer* observer, ReportHandler report, GateModule* next);
    ~CCSModule();

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
    CCSLink* link;
    GateContext* context;
    LiveLogClient* client;

    Observer* observer;
    GateModule* next;

    static void handleUpdateData(LiveLogClient* client, const char* call, const char* repeater);
};

#endif