#ifndef DEXTRA_H
#define DEXTRA_H

#include <stdint.h>
#include "DStar.h"
#include "RP2C.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

#define DEXTRA_DEFAULT_PORT   30001
#define DEXTRA_BUFFER_SIZE    64

#define DEXTRA_STATUS_LENGTH  3
#define DEXTRA_STATUS_ACK     "ACK"
#define DEXTRA_STATUS_NACK    "NAK"

#define DEXTRA_TYPE_GATEWAY   0
#define DEXTRA_TYPE_DONGLE    'D'

struct DExtraPoll
{
  char call[LONG_CALLSIGN_LENGTH];
  char type;
};

struct DExtraPoll2
{
  char call[LONG_CALLSIGN_LENGTH];
  char module;
  char type;
};

struct DExtraConnect
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  char reflectorModule;
  char zero;
};

struct DExtraConnectReply
{
  char repeaterCall[LONG_CALLSIGN_LENGTH];
  char repeaterModule;
  char reflectorModule;
  char status[DEXTRA_STATUS_LENGTH];
  char zero;
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif
