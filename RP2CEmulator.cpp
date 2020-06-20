#include "RP2CEmulator.h"
#include "NetworkTools.h"
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>

// #include "Dump.h"

#define RP2C_POLL_INTERVAL  15
#define RP2C_DATA_TIMEOUT   60

RP2CEmulator::RP2CEmulator(const char* address, int port) :
  onReport(NULL),
  number(0),
  time1(0),
  time2(0)
{
  handle = OpenUDPSocket(address, port);
}

RP2CEmulator::~RP2CEmulator()
{
  close(handle);
}

void RP2CEmulator::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

void RP2CEmulator::poll()
{
  struct DSTR data;
  memcpy(data.header.sign, DSTR_DATA_SIGN, RP2C_SIGN_LENGTH);
  data.header.number = htobe16(number);
  data.header.type = DSTR_TYPE_POLL;
  data.header.length = 0;
  sendto(handle, &data, sizeof(struct DSTRHeader),
    0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
}

void RP2CEmulator::initiateConnection(const struct sockaddr_in& address)
{
  memcpy(&this->address, &address, sizeof(struct sockaddr_in));
  poll();
}

void RP2CEmulator::publishHeard(const struct DStarRoute& route)
{
  struct DSTR data;
  memcpy(data.header.sign, DSTR_DATA_SIGN, RP2C_SIGN_LENGTH);
  data.header.number = htobe16(number);
  data.header.type = DSTR_TYPE_LAST_HEARD;
  data.header.length = sizeof(struct HeardData);
  memcpy(data.data.heard.ownCall, route.ownCall1, LONG_CALLSIGN_LENGTH);
  memcpy(data.data.heard.repeater1, route.repeater1, LONG_CALLSIGN_LENGTH);
  sendto(handle, &data, sizeof(struct DSTRHeader) + sizeof(struct HeardData),
    0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
}

void RP2CEmulator::handleInitialMessage(const struct sockaddr_in& address, struct DSTR* data)
{
  time_t now = time(NULL);
  if ((data->header.type == DSTR_TYPE_POLL) && (now > time1))
  {
    memcpy(&this->address, &address, sizeof(struct sockaddr_in));
    number = be16toh(data->header.number) + 1;
    data->header.type = DSTR_TYPE_ACK;
    sendto(handle, data, sizeof(struct DSTRHeader),
      0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
  }
  report(LOG_INFO, "RP2C: connected to %s\n", inet_ntoa(address.sin_addr));
  time1 = now + RP2C_POLL_INTERVAL;
  time2 = now + RP2C_DATA_TIMEOUT;
}

void RP2CEmulator::handleControllerMessage(const struct sockaddr_in& address, struct DSTR* data)
{
  if (data->header.type == DSTR_TYPE_ACK)
  {
    number = be16toh(data->header.number) + 1;
    time2 = time(NULL) + RP2C_DATA_TIMEOUT;
  }
  if (data->header.type & DSTR_FLAG_SENT)
  {
    number = be16toh(data->header.number) + 1;
    data->header.type = DSTR_TYPE_ACK;
    data->header.length = 0;
    sendto(handle, data, sizeof(struct DSTRHeader),
      0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
  }
}

void RP2CEmulator::receiveData()
{
  char buffer[RP2C_BUFFER_SIZE];
  struct sockaddr_in address;
  socklen_t size = sizeof(address);

  ssize_t length = recvfrom(handle, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, &size);

  // dump("RP2C", buffer, length);

  if ((length > 0) && (strncmp(buffer, DSTR_INIT_SIGN, RP2C_SIGN_LENGTH) == 0))
    handleInitialMessage(address, (struct DSTR*)buffer);

  if ((length > 0) && (strncmp(buffer, DSTR_DATA_SIGN, RP2C_SIGN_LENGTH) == 0))
    handleControllerMessage(address, (struct DSTR*)buffer);
}

void RP2CEmulator::doBackgroundActivity()
{
  time_t now = time(NULL);
  if ((now > time1) && (now < time2))
  {
    poll();
    time1 = now + RP2C_POLL_INTERVAL;
  }
  if ((now >= time2) && (time1 != 0))
  {
    report(LOG_ERR, "RP2C: connection timed out\n");
    time1 = 0;
  }
}

int RP2CEmulator::getHandle()
{
  return handle;
}

bool RP2CEmulator::isConnected()
{
  return (time1 != 0);
}

bool RP2CEmulator::assertConnection()
{
  if (time1 == 0)
  {
    report(LOG_ERR, "RP2C: gateway not connected\n");
    return false;
  }
  return true;
}