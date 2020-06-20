#ifndef GATECONTEXT_H
#define GATECONTEXT_H

#include <time.h>
#include <stdint.h>
#include <netinet/in.h>
#include "DStar.h"

#define LINK_STATE_DISCONNECTED  -2
#define LINK_STATE_CONNECTING    -1
#define LINK_STATE_CONNECTED     0

#define LINK_CONNECTION_TIMEOUT  30

class GateContext
{
  public:

    GateContext(char module, const char* call, const char* server, int port);
    ~GateContext();

    bool matchesTo(const char* call);
    bool matchesTo(const struct sockaddr_in& address);

    void touch();
    void setState(int value);
    void setOption(int value, int mask);

    char getModule();
    const char* getCall();
    const char* getServer();
    const struct sockaddr_in* getAddress();

    unsigned int getFrameNumber();
    int getOption(int mask);
    int getState();

  private:

    char module;
    char* server;
    char call[LONG_CALLSIGN_LENGTH + 1];
    struct sockaddr_in address;
    unsigned int option;
    unsigned int number;
    time_t time;
    int state;
};


#endif