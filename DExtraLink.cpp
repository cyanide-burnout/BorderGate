#include "DExtraLink.h"
#include "DStarTools.h"
#include "NetworkTools.h"
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#define OPTION_DEXTRA_VERSION_1  (1 << 0)
#define OPTION_DEXTRA_VERSION_2  (1 << 1)

DExtraLink::DExtraLink(const char* call, const char* address, GateModule** modules, Observer* observer) :
  G2Link(observer),
  modules(modules)
{
  CopyDStarCall(call, this->call, NULL, COPY_CLEAR_MODULE);
  handle = OpenUDPSocket(address, DEXTRA_DEFAULT_PORT);
}

int DExtraLink::getDefaultPort()
{
  return DEXTRA_DEFAULT_PORT;
}

const char* DExtraLink::getCall()
{
  return call;
}

void DExtraLink::connect(GateContext* context)
{
  struct DExtraConnect data;
  const char* reflector = context->getCall();
  memcpy(data.repeaterCall, call, LONG_CALLSIGN_LENGTH);
  data.repeaterModule = context->getModule();
  data.reflectorModule = reflector[MODULE_NAME_POSITION];
  data.zero = 0;
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_CONNECTING);
  observer->report(LOG_INFO, "%c: connecting to %.8s (%s)\n", context->getModule(), context->getCall(), context->getServer());
}

void DExtraLink::disconnect(GateContext* context)
{
  struct DExtraConnect data;
  memcpy(data.repeaterCall, call, LONG_CALLSIGN_LENGTH);
  data.repeaterModule = context->getModule();
  data.reflectorModule = ' ';
  data.zero = 0;
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_DISCONNECTED);
}

void DExtraLink::handlePoll(const struct sockaddr_in& address, const struct DExtraPoll* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((context != NULL) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0))
      {
        struct DExtraPoll reply;
        memcpy(reply.call, call, LONG_CALLSIGN_LENGTH);
        reply.type = DEXTRA_TYPE_GATEWAY;
        sendto(handle, &reply, sizeof(reply), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
        context->setOption(OPTION_DEXTRA_VERSION_1, 0);
        context->touch();
      }
    }
}

void DExtraLink::handlePoll(const struct sockaddr_in& address, const struct DExtraPoll2* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((context != NULL) &&
          (data->module == context->getModule()) &&
          (memcmp(data->call, context->getCall(), LONG_CALLSIGN_LENGTH) == 0) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0))
      {
        struct DExtraPoll2 reply;
        memcpy(reply.call, call, LONG_CALLSIGN_LENGTH);
        reply.call[MODULE_NAME_POSITION] = data->module;
        reply.module = data->call[MODULE_NAME_POSITION];
        reply.type = DEXTRA_TYPE_GATEWAY;
        sendto(handle, &reply, sizeof(reply), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
        context->setOption(OPTION_DEXTRA_VERSION_2, 0);
        context->touch();
      }
    }
}

void DExtraLink::handleConnectReply(const struct sockaddr_in& address, const struct DExtraConnectReply* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((context != NULL) &&
          (data->repeaterModule == context->getModule()) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0) &&
          (memcmp(data->status, DEXTRA_STATUS_ACK, DEXTRA_STATUS_LENGTH) == 0))
      {
        struct DExtraPoll reply;
        memcpy(reply.call, call, LONG_CALLSIGN_LENGTH);
        reply.type = DEXTRA_TYPE_GATEWAY;
        sendto(handle, &reply, sizeof(reply), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
        context->setState(handle);
        observer->report(LOG_INFO, "%c: connected to %.8s (%s)\n", context->getModule(), context->getCall(), context->getServer());
      }
    }
}

bool DExtraLink::receiveData()
{
  char buffer[DEXTRA_BUFFER_SIZE];
  struct sockaddr_in address;
  socklen_t size = sizeof(address);

  int length = recvfrom(handle, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, &size);

  /*
  if ((length > 0) && (strncmp(buffer, DSVT_DATA_SIGN, RP2C_SIGN_LENGTH) == 0))
  {
    handleData(address, (struct DSVT*)buffer);
    return (length > 0);
  }
  */

  switch (length)
  {
    case sizeof(struct DExtraPoll):
      handlePoll(address, (struct DExtraPoll*)buffer);
      break;

    case sizeof(struct DExtraPoll2):
      handlePoll(address, (struct DExtraPoll2*)buffer);
      break;

    case sizeof(struct DExtraConnectReply):
      handleConnectReply(address, (struct DExtraConnectReply*)buffer);
      break;
  }

  return (length > 0);
}

bool DExtraLink::sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame)
{
  if (sequence == LINK_INITIAL_SEQUENCE)
    for (GateModule* module = *modules; module != NULL; module = module->getNext())
      if (module->getLink() == this)
      {
        GateContext* context = module->getContext();
        if ((context != NULL) &&
            (context->getState() >= LINK_STATE_CONNECTED) &&
            (context->getOption(OPTION_DEXTRA_VERSION_2) != 0) &&
            (context->matchesTo(address)))
        {
           memcpy(route->repeater1, call, LONG_CALLSIGN_LENGTH);
           route->repeater1[MODULE_NAME_POSITION] = context->getModule();
        }
      }
  return G2Link::sendData(address, session, sequence, number, route, frame);
}


void DExtraLink::disconnectAll()
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

void DExtraLink::doBackgroundActivity()
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
