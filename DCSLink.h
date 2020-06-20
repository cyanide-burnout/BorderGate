#ifndef DCSLINK_H
#define DCSLINK_H

#include <time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include "DCS.h"
#include "GateLink.h"
#include "GateModule.h"
#include "GateContext.h"

class DCSLink : public GateLink
{
  public:

    DCSLink(const char* call, const char* address, const char* version, GateModule** modules, Observer* observer);
    ~DCSLink();

    int getHandle();
    int getDefaultPort();
    const char* getCall();

    bool receiveData();
    bool sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame);
    
    void doBackgroundActivity();
    void disconnectAll();

  private:

    int handle;
    char call[LONG_CALLSIGN_LENGTH + 1];
    char version[DCS_HTML_LENGTH];
    GateModule** modules;
    Observer* observer;

    void report(int priority, const char* format, ...);

    void connect(GateContext* context);
    void disconnect(GateContext* context);

    void handlePoll(const struct sockaddr_in& address, const struct DCSPoll2* data);
    void handleConnectReply(const struct sockaddr_in& address, const struct DCSConnectReply* data);

    uint32_t getConnectionFrameNumber(const struct sockaddr_in& address);
};

#endif
