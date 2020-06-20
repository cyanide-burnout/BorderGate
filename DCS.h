#ifndef DCS_H
#define DCS_H

#include <stdint.h>
#include <endian.h>
#include "DStar.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

#define DCS_DEFAULT_PORT        30051
#define DCS_BUFFER_SIZE         1024

#define DCS_FLAG_CLOSE          0x40

#define DCS_SIGN_LENGTH         4
#define DCS_NUMBER_SIZE         3

#define DCS_DATA_SIGN           "0001"
#define DCS_STATUS_SIGN         "EEEE"

#define DCS_LANGUAGE_DEFAULT    0x21

#define DCS_HTML_LENGTH         500
#define DCS_SHORT_DATA_SIZE     sizeof(DCSData)
#define DCS_LONG_DATA_SIZE      (DCS_SHORT_DATA_SIZE + DCS_HTML_LENGTH)

#define DCS_STATUS_ACK          "ACK"
#define DCS_STATUS_NACK         "NAK"
#define DCS_STATUS_LENGTH       3

#define DCS_DATA_VERSION        htobe16(0x0100)

struct DCSData
{
  char sign[DCS_SIGN_LENGTH];
  struct DStarRoute route;
  uint16_t session;
  uint8_t sequence;
  struct DStarDVFrame frame;
  uint8_t number[DCS_NUMBER_SIZE];
  uint16_t version;
  uint8_t language;
  char text[SLOW_DATA_TEXT_LENGTH];
  uint8_t reserved[16];
};

struct DCSPoll
{
  char reflectorCall[LONG_CALLSIGN_LENGTH];
  uint8_t zero;
};

struct DCSPoll2
{
  char reflectorCall[LONG_CALLSIGN_LENGTH];
  uint8_t type;
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  uint8_t unknown[SHORT_CALLSIGN_LENGTH];
};

struct DCSPollReply
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char zero;
  char reflectorCall[LONG_CALLSIGN_LENGTH];
};

struct DCSConnect
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  char reflectorModule;
  char zero;
  char reflectorCall[LONG_CALLSIGN_LENGTH];
};

struct DCSConnect2
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  char reflectorModule;
  char zero;
  char reflectorCall[LONG_CALLSIGN_LENGTH - 1];
  char separator;
  char description[DCS_HTML_LENGTH];
};

struct DCSConnectReply
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  char reflectorModule;
  char status[DCS_STATUS_LENGTH];
  char zero;
};

struct DCSStatus
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char status[26];
  char zero;
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif
