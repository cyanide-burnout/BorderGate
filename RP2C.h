#ifndef RP2C_H
#define RP2C_H

#include "DStar.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

#define RP2C_CONTROLLER_PORT      20319
#define RP2C_GATEWAY_PORT         20000
#define RP2C_ROUTING_PORT         40000

#define RP2C_BUFFER_SIZE          2048

#pragma mark Common RP2C

#define RP2C_SIGN_LENGTH          4

#define RP2C_TYPE_DIGITAL_VOICE   0x20
#define RP2C_TYPE_DIGITAL_DATA    0x40

// Sequence number flags
#define RP2C_NUMBER_LAST_FRAME    0x40
#define RP2C_NUMBER_RADIO_HEADER  0x80
#define RP2C_NUMBER_DIGITAL_DATA  0xC0
#define RP2C_NUMBER_MASK          0x3f

#define RP2C_DATA_TAIL_BYTE       0x80

typedef char RP2CSign[RP2C_SIGN_LENGTH];

/*
struct TP2CBandData
{
  uint8_t targetRepeater;
  uint8_t sourceRepeater;
  uint8_t sourceTerminal;
};
*/

struct TrunkHeader
{
  uint8_t type;                   // RP2C_TYPE_*
  uint8_t repeater2;
  uint8_t repeater1;
  uint8_t terminal;
  uint16_t session;               // BE16
  uint8_t sequence;               // [0 - 20]
};

union TrunkData
{
  struct DStarRadioHeader header;
  struct DStarDVFrame frame;
};

struct Trunk
{
  struct TrunkHeader header;
  union TrunkData data;
};

#pragma mark G2 / DSVT

#define DSVT_DATA_SIGN            "DSVT"

#define DSVT_TYPE_RADIO_HEADER    htole16(0x10)
#define DSVT_TYPE_DIGITAL_VOICE   htole16(RP2C_TYPE_DIGITAL_VOICE)
#define DSVT_TYPE_DIGITAL_DATA    htole16(RP2C_TYPE_DIGITAL_DATA)

#define DSVT_RADIO_HEADER_SIZE    (sizeof(struct DSVTHeader) + sizeof(struct TrunkHeader) + sizeof(struct DStarRadioHeader))
#define DSVT_AUDIO_FRAME_SIZE     (sizeof(struct DSVTHeader) + sizeof(struct TrunkHeader) + sizeof(struct DStarDVFrame))

struct DSVTHeader
{
  RP2CSign sign;      // DSVT_DATA_SIGN
  uint16_t type;      // DSVT_TYPE_*
  uint16_t reserved;
};

struct DSVT
{
  struct DSVTHeader header;
  struct Trunk trunk;
};

#pragma mark RP2C / DSTR

#define DSTR_INIT_SIGN            "INIT"
#define DSTR_DATA_SIGN            "DSTR"

#define DSTR_CLASS_SENT           htole16('s')
#define DSTR_CLASS_REPLIED        htole16('r')
#define DSTR_CLASS_DIGITAL_DATA   htole16(1 << 8)
#define DSTR_CLASS_DIGITAL_VOICE  htole16(1 << 9)
#define DSTR_CLASS_TRANSMISSION   htole16(1 << 12)
#define DSTR_CLASS_LAST_HEARD     htole16(1 << 13)

#define DSTR_TYPE_ACK             (DSTR_CLASS_REPLIED)
#define DSTR_TYPE_POLL            (DSTR_CLASS_SENT)
#define DSTR_TYPE_DIGITAL_DATA    (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_DATA | DSTR_CLASS_TRANSMISSION)
#define DSTR_TYPE_DIGITAL_VOICE   (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_VOICE | DSTR_CLASS_TRANSMISSION)
#define DSTR_TYPE_LAST_HEARD      (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_VOICE | DSTR_CLASS_LAST_HEARD)

#define DSTR_FLAG_SENT            (DSTR_CLASS_SENT ^ DSTR_CLASS_REPLIED)

struct DSTRHeader
{
  RP2CSign sign;    // DSTR_INIT_SIGN, DSTR_DATA_SIGN
  uint16_t number;  // packet number, BE16
  uint16_t type;    // DSTR_TYPE_*
  uint16_t length;  // total length of header2 + data, BE16
};

struct HeardData
{
  char ownCall[LONG_CALLSIGN_LENGTH];
  char repeater1[LONG_CALLSIGN_LENGTH];
};

union DSTRData
{
  struct HeardData heard;
  struct Trunk trunk;
};

struct DSTR
{
  struct DSTRHeader header;
  union DSTRData data;
};

#pragma pack(pop)

#ifdef __cplusplus
};
#endif

#endif
