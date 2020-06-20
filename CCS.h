#ifndef CCS_H
#define CCS_H

#include <stdint.h>
#include <endian.h>
#include "DStar.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

// Version 03-20.08.2015

#define CCS_DEFAULT_PORT        30062
#define CCS_BUFFER_SIZE         1024

#define CCS_SIGN_LENGTH         4
#define CCS_CONTACT_LENGTH      17
#define CCS_VERSION_LENGTH      20
#define CCS_LOCATION_LENGTH     6
#define CCS_NUMBER_SIZE         3

#define CCS_DATA_SIGN           "0001"
#define CCS_DATA_VERSION        htobe16(0x0100)

#define CCS_STATUS_SIGN         "DDDD"
#define CCS_LAST_HEARD_SIGN     "GGGG"

#define CCS_STATUS_ACK          "ACK"
#define CCS_STATUS_NACK         "NAK"
#define CCS_STATUS_LENGTH       3

#define CCS_POLL_SIZE           25
#define CCS_DATA_SIZE           100

#define CCS_DATA_TYPE           '!'

#define CCS_SOURCE_TYPE_RF      '0'
#define CCS_SOURCE_TYPE_XRF     '1'
#define CCS_SOURCE_TYPE_DCS     '2'
#define CCS_SOURCE_TYPE_REF     '4'
#define CCS_SOURCE_TYPE_CCS     '5'

#define CCS_QUERY_REQ_SIGN      "0002"
#define CCS_QUERY_RES_SIGN      "1002"

#define CCS_QUERY_V1_SIZE       38
#define CCS_QUERY_V2_SIZE       60

#define CCS_QUERY_STATUS_OK     0
#define CCS_QUERY_STATUS_BUSY   1

#define CCS_TIMEOUT_LENGTH      9

struct CCSData
{
  char sign[CCS_SIGN_LENGTH];
  struct DStarRoute route;
  uint16_t session;
  uint8_t sequence;
  struct DStarDVFrame frame;
  uint8_t number[CCS_NUMBER_SIZE];
  uint16_t version;
  uint8_t dataType;     // CCS_DATA_TYPE
  char text[SLOW_DATA_TEXT_LENGTH];
  uint8_t reserved0[9]; // 0x00
  uint8_t sourceType;   // CCS_SOURCE_TYPE_xxx
  uint8_t reserved1[6]; // 0x00
};

struct CCSPollReply
{
  char call[LONG_CALLSIGN_LENGTH];
  char contact[CCS_CONTACT_LENGTH];
};

struct CCSConnect
{
  char call[LONG_CALLSIGN_LENGTH];
  char module;
  char spacer1; // 'A'
  char spacer2; // '@'
  char location[CCS_LOCATION_LENGTH];
  char spacer3; // ' '
  char spacer4; // '@'
  char version[CCS_VERSION_LENGTH];
};

struct CCSDisonnect
{
  char call[LONG_CALLSIGN_LENGTH];
  char module;
  char spacer[10]; // ' '
};

struct CCSConnectReply
{
  char call[LONG_CALLSIGN_LENGTH];
  char module;
  char reserved;
  char status[CCS_STATUS_LENGTH];
  char zero;
};

// This type of message not presented in specification
struct CCSLastHeard
{
  char sign[CCS_SIGN_LENGTH];
  char ownCall[LONG_CALLSIGN_LENGTH];
  char ownLocation[CCS_LOCATION_LENGTH];
  char yourCall[LONG_CALLSIGN_LENGTH];
  char yourLocation[CCS_LOCATION_LENGTH];
  char spacer;
};

struct CCSStatusQuery
{
  char remoteCall[LONG_CALLSIGN_LENGTH];
  char sign[CCS_SIGN_LENGTH];
  char reflectorCall[LONG_CALLSIGN_LENGTH];
  char status;
  char targetCall[LONG_CALLSIGN_LENGTH];
  char timeout[CCS_TIMEOUT_LENGTH];
  char clientCall[LONG_CALLSIGN_LENGTH];
  char zero[14];
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif
