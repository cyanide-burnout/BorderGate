#ifndef GATELINK_H
#define GATELINK_H

#include <stdint.h>
#include <netinet/in.h>

#include "DStar.h"
#include "RP2C.h"

#define LINK_INITIAL_SEQUENCE  RP2C_NUMBER_RADIO_HEADER

class GateLink
{
  public:

    class Observer
    {
      public:
        virtual void report(int priority, const char* format, ...) = 0;
        virtual void processData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, struct DStarRoute* route, struct DStarDVFrame* frame) = 0;
    };

    virtual int getHandle() = 0;
    virtual int getDefaultPort() = 0;

    virtual const char* getCall() = 0;

    virtual bool receiveData() = 0;
    virtual bool sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame) = 0;

    virtual void doBackgroundActivity() = 0;
    virtual void disconnectAll() = 0;
};

#endif
