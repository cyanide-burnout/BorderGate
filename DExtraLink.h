#ifndef DEXTRALINK_H
#define DEXTRALINK_H

#include <time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include "DExtra.h"
#include "G2Link.h"
#include "GateModule.h"
#include "GateContext.h"

class DExtraLink : public G2Link
{
  public:

    DExtraLink(const char* call, const char* address, GateModule** modules, Observer* observer);

    int getDefaultPort();
    const char* getCall();

    bool receiveData();
    bool sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame);

    void disconnectAll();
    void doBackgroundActivity();

  private:

    char call[LONG_CALLSIGN_LENGTH + 1];
    GateModule** modules;

    void connect(GateContext* context);
    void disconnect(GateContext* context);

    void handlePoll(const struct sockaddr_in& address, const struct DExtraPoll* data);
    void handlePoll(const struct sockaddr_in& address, const struct DExtraPoll2* data);
    void handleConnectReply(const struct sockaddr_in& address, const struct DExtraConnectReply* data);
};

#endif
