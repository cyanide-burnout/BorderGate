#ifndef CCSLINK_H
#define CCSLINK_H

#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <netinet/in.h>
#include "CCS.h"
#include "GateLink.h"
#include "GateModule.h"
#include "GateContext.h"

class CCSLink : public GateLink
{
  public:

    CCSLink(const char* call, const char* address, const char* version, GateModule** modules, Observer* observer);
    ~CCSLink();

    int getHandle();
    int getDefaultPort();
    const char* getCall();

    bool receiveData();
    bool sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame);

    void doBackgroundActivity();
    void disconnectAll();

    bool publishHeard(const struct sockaddr_in& address, const struct DStarRoute& route);

  private:

    int handle;
    char call[LONG_CALLSIGN_LENGTH + 1];
    char version[CCS_VERSION_LENGTH];
    char location[CCS_LOCATION_LENGTH];
    GateModule** modules;
    Observer* observer;

    void connect(GateContext* context);
    void disconnect(GateContext* context);

    void handlePoll(const struct sockaddr_in& address);
    void handleConnectReply(const struct sockaddr_in& address, struct CCSConnectReply* data);
    void handleStatusQuery(const struct sockaddr_in& address, struct CCSStatusQuery* query);

    uint32_t getConnectionFrameNumber(const struct sockaddr_in& address);
};

#endif
