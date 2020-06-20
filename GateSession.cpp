#include "GateSession.h"
#include "NetworkTools.h"
#include "DStarTools.h"
#include <string.h>

GateSession::GateSession(GateModule* module,
    const struct DStarRoute& heardRoute, const struct DStarRoute& targetRoute,
    const struct sockaddr_in& sourceAddress, uint16_t sourceNumber,
    const struct sockaddr_in& targetAddress, uint16_t targetNumber,
    GateSession* next) :
  module(module),
  number1(sourceNumber),
  number2(targetNumber),
  next(next),
  count(0)
{
  ::time(&time);
  header = SLOW_DATA_SYNC_VECTOR;
  memcpy(&route1, &heardRoute, sizeof(struct DStarRoute));
  memcpy(&route2, &targetRoute, sizeof(struct DStarRoute));
  memcpy(&address1, &sourceAddress, sizeof(struct sockaddr_in));
  memcpy(&address2, &targetAddress, sizeof(struct sockaddr_in));
  memset(text, ' ', SLOW_DATA_TEXT_LENGTH);
}

bool GateSession::matchesTo(const struct sockaddr_in& address, uint16_t number)
{
  return (number1 == number) && (CompareSocketAddress(&address1, &address) == 0);
}

void GateSession::processSlowData(uint16_t number, uint8_t* data)
{
  const uint8_t choke[] =
  {
    SLOW_DATA_FILLER_BYTE ^ SCRAMBLER_BYTE1,
    SLOW_DATA_FILLER_BYTE ^ SCRAMBLER_BYTE2,
    SLOW_DATA_FILLER_BYTE ^ SCRAMBLER_BYTE3
  };

  const uint8_t scramble[] =
  {
    SCRAMBLER_BYTE1,
    SCRAMBLER_BYTE2,
    SCRAMBLER_BYTE3
  };

  if (number & RP2C_NUMBER_MASK)
  {
    if (number & 1)
      header = *data ^ SCRAMBLER_BYTE1;

    switch (header & SLOW_DATA_TYPE_MASK)
    {
      case SLOW_DATA_TYPE_HEADER:
        memcpy(data, choke, SLOW_DATA_FRAME_SIZE);
        break;

      case SLOW_DATA_TYPE_TEXT:
        size_t part = header & SLOW_DATA_TEXT_PART_MASK;
        size_t position = part * SLOW_DATA_DATA_SIZE;
        position += (~number & 1) * (SLOW_DATA_FRAME_SIZE - 1);
        for (size_t index = number & 1; index < SLOW_DATA_FRAME_SIZE; index ++)
        {
          text[position] = data[index] ^ scramble[index];
          position ++;
        }
        break;
    }
  }
}

bool GateSession::forward(uint8_t number, struct DStarDVFrame* frame)
{
  GateLink* link = module->getLink();

  if (count == 0)
    link->sendData(address2, number2, LINK_INITIAL_SEQUENCE, 0, &route2, NULL);

  processSlowData(number, frame->slowData);
  link->sendData(address2, number2, number, count, &route2, frame);

  count ++;
  ::time(&time);

  if (count == SLOW_DATA_FRAME_COUNT)
    module->publishHeard(route1, route2.repeater2, text);

  if (number & RP2C_NUMBER_LAST_FRAME)
    module->publishHeard(route1, count);

  return !(number & RP2C_NUMBER_LAST_FRAME);
}

uint16_t GateSession::getNumber()
{
  return number2;
}

time_t GateSession::getLastAccessTime()
{
  return time;
}

GateSession** GateSession::getNext()
{
  return &next;
}
