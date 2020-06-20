#include "DCSLink.h"
#include "DStarTools.h"
#include "NetworkTools.h"
#include <unistd.h>
#include <syslog.h>
#include <string.h>

DCSLink::DCSLink(const char* call, const char* address, const char* version, GateModule** modules, GateLink::Observer* observer) :
  modules(modules),
  observer(observer)
{
  CopyDStarCall(call, this->call, NULL, COPY_CLEAR_MODULE);
  CopyDStarString(version, this->version, DCS_HTML_LENGTH);
  handle = OpenUDPSocket(address, DCS_DEFAULT_PORT);
}

DCSLink::~DCSLink()
{
  close(handle);
}

int DCSLink::getHandle()
{
  return handle;
}

int DCSLink::getDefaultPort()
{
  return DCS_DEFAULT_PORT;
}
  
const char* DCSLink::getCall()
{
  return call;
}

bool DCSLink::sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame)
{
  if ((route != NULL) && (frame != NULL))
  {
    DCSData data;
    memset(&data, 0, sizeof(struct DCSData));
    
    memcpy(data.sign, DCS_DATA_SIGN, DCS_SIGN_LENGTH);
    memset(data.route.repeater2, ' ', LONG_CALLSIGN_LENGTH);
    memcpy(data.route.repeater1, call, LONG_CALLSIGN_LENGTH);
    data.route.repeater1[MODULE_NAME_POSITION] = route->repeater1[MODULE_NAME_POSITION];
    memcpy(data.route.ownCall1, route->ownCall1, LONG_CALLSIGN_LENGTH);
    memcpy(data.route.ownCall2, route->ownCall2, LONG_CALLSIGN_LENGTH);
    memcpy(data.route.yourCall, CQCQCQ, LONG_CALLSIGN_LENGTH);
    memcpy(&data.frame, frame, sizeof(struct DStarDVFrame));
    memset(data.text, ' ', SLOW_DATA_TEXT_LENGTH);
    memcpy(data.number, &number, DCS_NUMBER_SIZE);
    data.session = session;
    data.sequence = sequence;
    data.language = DCS_LANGUAGE_DEFAULT;
    data.version = DCS_DATA_VERSION;

    return (sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) > 0);

  }
  return false;
}

void DCSLink::connect(GateContext* context)
{
  struct DCSConnect2 data;
  const char* reflector = context->getCall();
  memcpy(data.repeaterCall, call, LONG_CALLSIGN_LENGTH);
  memcpy(data.reflectorCall, reflector, LONG_CALLSIGN_LENGTH - 1);
  memset(data.description, 0, DCS_HTML_LENGTH);
  strncpy(data.description, version, DCS_HTML_LENGTH);
  data.reflectorCall[MODULE_NAME_POSITION] = ' ';
  data.repeaterModule = context->getModule();
  data.reflectorModule = reflector[MODULE_NAME_POSITION];
  data.zero = 0;
  data.separator = '@';
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_CONNECTING);
  
  observer->report(LOG_INFO, "%c: connecting to %s\n", context->getModule(), context->getServer());
}

void DCSLink::disconnect(GateContext* context)
{
  struct DCSConnect data;
  const char* reflector = context->getCall();
  memcpy(data.repeaterCall, call, LONG_CALLSIGN_LENGTH);
  memcpy(data.reflectorCall, reflector, LONG_CALLSIGN_LENGTH);
  data.reflectorCall[MODULE_NAME_POSITION] = ' ';
  data.repeaterModule = context->getModule();
  data.reflectorModule = ' ';
  data.zero = 0;
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_DISCONNECTED);
}

void DCSLink::handlePoll(const struct sockaddr_in& address, const struct DCSPoll2* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
  if (module->getLink() == this)
  {
    GateContext* context = module->getContext();
    if ((context != NULL) &&
        (CompareSocketAddress(&address, context->getAddress()) == 0))
    {
      struct DCSPollReply reply;
      const char* reflector = context->getCall();
      memcpy(reply.repeaterCall, call, LONG_CALLSIGN_LENGTH);
      memcpy(reply.reflectorCall, call, LONG_CALLSIGN_LENGTH);
      reply.reflectorCall[MODULE_NAME_POSITION] = ' ';
      reply.zero = 0;
      sendto(handle, &reply, sizeof(reply), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
      context->touch();
    }
  }
}

void DCSLink::handleConnectReply(const struct sockaddr_in& address, const struct DCSConnectReply* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((data->repeaterCall[MODULE_NAME_POSITION] == context->getModule()) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0) &&
          (memcmp(data->status, DCS_STATUS_ACK, DCS_STATUS_LENGTH) == 0))
      {
        context->setState(handle);
        observer->report(LOG_INFO, "%c: connected to %s\n", context->getModule(), context->getServer());
      }
    }
}

bool DCSLink::receiveData()
{
  char buffer[DCS_BUFFER_SIZE];
  struct sockaddr_in address;
  socklen_t size = sizeof(address);

  int length = recvfrom(handle, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, &size);

  switch (length)
  {
    case sizeof(struct DCSPoll):
    case sizeof(struct DCSPoll2):
      handlePoll(address, (struct DCSPoll2*)buffer);
      break;

    case sizeof(struct DCSConnectReply):
      handleConnectReply(address, (struct DCSConnectReply*)buffer);
      break;

    /*
    case DCS_SHORT_DATA_SIZE:
    case DCS_LONG_DATA_SIZE:
      if (strncmp(buffer, DCS_DATA_SIGN, DCS_SIGN_LENGTH) == 0)
      {
        handleDigitalVoice(address, (struct DCSData*)buffer);
        return;
      }
      break;
    */

    default:
      break;
  }

  return (length > 0);
}

void DCSLink::disconnectAll()
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
  if (module->getLink() == this)
  {
    GateContext* context = module->getContext();
    if ((context != NULL) &&
        (context->getState() >= LINK_STATE_CONNECTED))
      disconnect(context);
  }
}

void DCSLink::doBackgroundActivity()
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
  if (module->getLink() == this)
  {
    GateContext* context = module->getContext();
    if ((context != NULL) &&
        (context->getState() == LINK_STATE_DISCONNECTED))
      connect(context);
  }
}
