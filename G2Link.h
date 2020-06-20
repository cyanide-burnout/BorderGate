#ifndef G2LINK_H
#define G2LINK_H

#include "DStar.h"
#include "RP2C.h"
#include "GateLink.h"
#include "GateModule.h"

#define SESSION_ACTIVITY_TIMEOUT  20

class G2Link : public GateLink
{
  public:

    G2Link(const char* address, Observer* observer);
    ~G2Link();

    int getHandle();
    int getDefaultPort();
    const char* getCall();

    bool receiveData();
    bool sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame);

    void doBackgroundActivity();
    void disconnectAll();

  protected:

    int handle;
    Observer* observer;

    G2Link(Observer* observer);

    void handleData(const struct sockaddr_in& address, struct DSVT* data);
};

#endif
