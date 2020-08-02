#ifndef MASTERMODULE_H
#define MASTERMODULE_H

#include <time.h>
#include <semaphore.h>
#include <libconfig.h>
#include <mosquitto.h>
#include <netinet/in.h>
#include "GateModule.h"

class MasterModule : public GateModule
{
  public:

    MasterModule(config_setting_t* setting, GateLink* link, Observer* observer, ReportHandler handler, GateModule* next);
    ~MasterModule();

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
    Observer* observer;
    ReportHandler handler;
    GateModule* next;

    struct in_addr remote;
    struct mosquitto* connection;

    int handle1;
    time_t* date;

    int handle2;
    sem_t semaphore;

    void handleMessage(const char* data);

    void report(int priority, const char* format, ...);

    void checkMosquittoError(int result);
    static void handleConnect(struct mosquitto* connection, void* user, int result);
    static void handleMessage(struct mosquitto* connection, void* user, const struct mosquitto_message* message);
};

#endif