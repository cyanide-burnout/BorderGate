#ifndef GATESESSION_H
#define GATESESSION_H

#include <time.h>
#include <stdint.h>
#include <netinet/in.h>
#include "GateModule.h"
#include "DStar.h"
#include "RP2C.h"

class GateSession
{
  public:

    GateSession(GateModule* module,
      const struct DStarRoute& heardRoute, const struct DStarRoute& targetRoute,
      const struct sockaddr_in& sourceAddress, uint16_t sourceNumber,
      const struct sockaddr_in& targetAddress, uint16_t targetNumber,
      GateSession* next);

    bool matchesTo(const struct sockaddr_in& address, uint16_t number);

    bool forward(uint8_t number, struct DStarDVFrame* frame);

    uint16_t getNumber();
    time_t getLastAccessTime();
    GateSession** getNext();

  private:

    GateModule* module;
    struct DStarRoute route1;
    struct DStarRoute route2;
    struct sockaddr_in address1;
    struct sockaddr_in address2;
    uint16_t number1;
    uint16_t number2;
    uint16_t count;
    time_t time;

    uint8_t header;
    char text[SLOW_DATA_TEXT_LENGTH];

    GateSession* next;

    void processSlowData(uint16_t number, uint8_t* data);
};

#endif