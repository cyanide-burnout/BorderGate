#include <unistd.h>
#include <string.h>
#include "G2Link.h"
#include "DStarTools.h"
#include "NetworkTools.h"

#define DSVT_RADIO_HEADER_REPETITIONS  5

G2Link::G2Link(const char* address, GateLink::Observer* observer) :
  observer(observer)
{
  handle = OpenUDPSocket(address, RP2C_ROUTING_PORT);
}

G2Link::G2Link(GateLink::Observer* observer) :
  observer(observer)
{

}

G2Link::~G2Link()
{
  close(handle);
}

int G2Link::getHandle()
{
  return handle;
}

int G2Link::getDefaultPort()
{
  return RP2C_ROUTING_PORT;
}

const char* G2Link::getCall()
{
  return NULL;
}

bool G2Link::receiveData()
{
  char buffer[RP2C_BUFFER_SIZE];
  struct sockaddr_in address;
  socklen_t size = sizeof(address);

  ssize_t length = recvfrom(handle, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, &size);

  if ((length > 0) && (strncmp(buffer, DSVT_DATA_SIGN, RP2C_SIGN_LENGTH) == 0))
    handleData(address, (struct DSVT*)buffer);

  return (length > 0);
}

bool G2Link::sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame)
{
  size_t size;
  size_t count;
  struct DSVT data;
  
  memcpy(data.header.sign, DSVT_DATA_SIGN, RP2C_SIGN_LENGTH);
  memset(&data.trunk.header, 0, sizeof(struct TrunkHeader));
  data.header.reserved = 0;
  
  data.trunk.header.type = DSVT_TYPE_DIGITAL_VOICE;
  data.trunk.header.session = session;
  
  if (sequence == LINK_INITIAL_SEQUENCE)
  {
    data.header.type = DSVT_TYPE_RADIO_HEADER;
    data.trunk.header.sequence = RP2C_NUMBER_RADIO_HEADER;
    data.trunk.data.header.checkSum = CalculateCCITTCheckSum(route, sizeof(struct DStarRoute));
    memcpy(&data.trunk.data.header.route, route, sizeof(struct DStarRoute));
    size = DSVT_RADIO_HEADER_SIZE;
    count = DSVT_RADIO_HEADER_REPETITIONS;
  }
  else
  {
    data.header.type = DSVT_TYPE_DIGITAL_VOICE;
    data.trunk.header.sequence = sequence;
    memcpy(&data.trunk.data.frame, frame, sizeof(struct DStarDVFrame));
    size = DSVT_AUDIO_FRAME_SIZE;
    count = 1;
  }
  
  bool outcome = false;
  
  for (size_t index = 0; index < count; index ++)
    outcome |= (sendto(handle, &data, size, 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) > 0);
  
  return outcome;
}

void G2Link::handleData(const struct sockaddr_in& address, struct DSVT* data)
{
  switch (data->header.type)
  {
    case DSVT_TYPE_RADIO_HEADER:
      observer->processData(address, data->trunk.header.session, LINK_INITIAL_SEQUENCE, &data->trunk.data.header.route, NULL);
      break;
    case DSVT_TYPE_DIGITAL_VOICE:
      observer->processData(address, data->trunk.header.session, data->trunk.header.sequence, NULL, &data->trunk.data.frame);
      break;
  }
}

void G2Link::doBackgroundActivity()
{

}

void G2Link::disconnectAll()
{

}
