#ifndef DDBMODULE_H
#define DDBMODULE_H

#include <time.h>
#include <libconfig.h>
#include "GateModule.h"
#include "DDBClient.h"

class DDBModule : public GateModule
{
  public:

    DDBModule(config_setting_t* setting, const char* version, GateLink* link, Observer* observer, ReportHandler report, GateModule* next);
    ~DDBModule();

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
    DDBClient* client;
    DDBClient::Store* store;
    Observer* observer;
    GateModule* next;

    time_t time1;
    time_t time2;

    static void handleUpdateCommand(DDBClient* client, int table, const char* date, const char* call1, const char* call2);
};

#endif