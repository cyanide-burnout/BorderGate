#include "CCSLink.h"
#include "DStarTools.h"
#include "NetworkTools.h"
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <endian.h>

#include "Dump.h"

CCSLink::CCSLink(const char* call, const char* address, const char* version, GateModule** modules, GateLink::Observer* observer) :
  modules(modules),
  observer(observer)
{
  CopyDStarCall(call, this->call, NULL, COPY_CLEAR_MODULE);
  CopyDStarString(version, this->version, CCS_VERSION_LENGTH);
  memset(location, ' ', CCS_LOCATION_LENGTH);
  handle = OpenUDPSocket(address, CCS_DEFAULT_PORT);
}

CCSLink::~CCSLink()
{
  close(handle);
}

int CCSLink::getHandle()
{
  return handle;
}

int CCSLink::getDefaultPort()
{
  return CCS_DEFAULT_PORT;
}

const char* CCSLink::getCall()
{
  return call;
}

bool CCSLink::sendData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, uint32_t number, struct DStarRoute* route, struct DStarDVFrame* frame)
{
  if ((route != NULL) && (frame != NULL))
  {
    CCSData data;
    memset(&data, 0, sizeof(struct CCSData));

    memcpy(data.sign, CCS_DATA_SIGN, CCS_SIGN_LENGTH);
    memset(data.route.repeater2, ' ', LONG_CALLSIGN_LENGTH);
    memcpy(data.route.repeater1, call, LONG_CALLSIGN_LENGTH);
    data.route.repeater1[MODULE_NAME_POSITION] = route->repeater1[MODULE_NAME_POSITION];
    memcpy(data.route.ownCall1, route->ownCall1, LONG_CALLSIGN_LENGTH);
    memcpy(data.route.ownCall2, route->ownCall2, SHORT_CALLSIGN_LENGTH);
    memcpy(data.route.yourCall, route->yourCall, LONG_CALLSIGN_LENGTH);
    memcpy(&data.frame, frame, sizeof(struct DStarDVFrame));
    memset(data.text, ' ', SLOW_DATA_TEXT_LENGTH);
    memcpy(data.number, &number, CCS_NUMBER_SIZE);
    data.session = session;
    data.sequence = sequence;
    data.version = CCS_DATA_VERSION;
    data.dataType = CCS_DATA_TYPE;
    data.sourceType = CCS_SOURCE_TYPE_RF;

    return (sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) > 0);
  }
  return false;
}

bool CCSLink::publishHeard(const struct sockaddr_in& address, const struct DStarRoute& route)
{
  if (route.repeater1[MODULE_NAME_POSITION] != ' ')
  {
    CCSData data;
    memset(&data, 0, sizeof(struct CCSData));

    memcpy(data.sign, CCS_DATA_SIGN, CCS_SIGN_LENGTH);
    memset(data.route.repeater2, ' ', LONG_CALLSIGN_LENGTH);
    memcpy(data.route.repeater1, call, LONG_CALLSIGN_LENGTH);
    data.route.repeater1[MODULE_NAME_POSITION] = route.repeater1[MODULE_NAME_POSITION];
    memcpy(data.route.ownCall1, route.ownCall1, LONG_CALLSIGN_LENGTH);
    memcpy(data.route.ownCall2, route.ownCall2, SHORT_CALLSIGN_LENGTH);
    memcpy(data.route.yourCall, CQCQCQ, LONG_CALLSIGN_LENGTH);
    memset(data.text, ' ', SLOW_DATA_TEXT_LENGTH);
    data.version = CCS_DATA_VERSION;
    data.dataType = CCS_DATA_TYPE;
    data.sourceType = CCS_SOURCE_TYPE_RF;

    return (sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) > 0);
  }
  return false;
}

void CCSLink::connect(GateContext* context)
{
  struct CCSConnect data;
  memcpy(data.call, call, LONG_CALLSIGN_LENGTH);
  memcpy(data.version, version, CCS_VERSION_LENGTH);
  memcpy(data.location, location, CCS_LOCATION_LENGTH);
  data.module = context->getModule();
  data.spacer1 = 'A';
  data.spacer2 = '@';
  data.spacer3 = ' ';
  data.spacer4 = '@';
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_CONNECTING);

  observer->report(LOG_INFO, "%c: connecting to %s\n", context->getModule(), context->getServer());
}

void CCSLink::disconnect(GateContext* context)
{
  struct CCSDisonnect data;
  memcpy(data.call, call, LONG_CALLSIGN_LENGTH);
  memset(data.spacer, ' ', sizeof(data.spacer));
  data.module = context->getModule();
  sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)context->getAddress(), sizeof(struct sockaddr_in));
  context->setState(LINK_STATE_DISCONNECTED);
}

void CCSLink::handlePoll(const struct sockaddr_in& address)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((context != NULL) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0))
      {
        struct CCSPollReply data;
        memcpy(data.call, call, LONG_CALLSIGN_LENGTH);
        memset(data.contact, ' ', CCS_CONTACT_LENGTH);
        sendto(handle, &data, sizeof(data), 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
        context->touch();
      }
    }
}

void CCSLink::handleConnectReply(const struct sockaddr_in& address, struct CCSConnectReply* data)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    if (module->getLink() == this)
    {
      GateContext* context = module->getContext();
      if ((data->module == context->getModule()) &&
          (CompareSocketAddress(&address, context->getAddress()) == 0) &&
          (memcmp(data->status, CCS_STATUS_ACK, CCS_STATUS_LENGTH) == 0))
      {
        context->setState(handle);
        observer->report(LOG_INFO, "%c: connected to %s\n", context->getModule(), context->getServer());
      }
    }
}

void CCSLink::handleStatusQuery(const struct sockaddr_in& address, struct CCSStatusQuery* query)
{
  memcpy(query->sign, CCS_QUERY_RES_SIGN, CCS_SIGN_LENGTH);
  memset(query->reflectorCall, ' ', LONG_CALLSIGN_LENGTH);
  memset(query->clientCall, ' ', LONG_CALLSIGN_LENGTH);
  memset(query->timeout, ' ', CCS_TIMEOUT_LENGTH);
  memset(query->zero, 0, sizeof(query->zero));
  query->timeout[0] = '0';
  query->status = CCS_QUERY_STATUS_OK;
  sendto(handle, query, CCS_QUERY_V2_SIZE, 0, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
}

bool CCSLink::receiveData()
{
  char buffer[CCS_BUFFER_SIZE];
  struct sockaddr_in address;
  socklen_t size = sizeof(address);

  int length = recvfrom(handle, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, &size);

  switch (length)
  {
    case CCS_POLL_SIZE:
      handlePoll(address);
      break;

    case CCS_DATA_SIZE:
      if (strncmp(buffer, CCS_DATA_SIGN, CCS_SIGN_LENGTH) == 0)
      {
        CCSData* data = (CCSData*)buffer;
        observer->processData(address, data->session, data->sequence, &data->route, &data->frame);
        break;
      }
      if (strncmp(buffer + LONG_CALLSIGN_LENGTH, CCS_QUERY_REQ_SIGN, CCS_SIGN_LENGTH) == 0)
      {
        handleStatusQuery(address, (CCSStatusQuery*)buffer);
        break;
      }
      break;

    case sizeof(CCSConnectReply):
      handleConnectReply(address, (CCSConnectReply*)buffer);
      break;

    default:
      // TODO: Remove this section
      /*
      if (strncmp(buffer, CCS_STATUS_SIGN, CCS_SIGN_LENGTH) != 0)
        dump("CCS", buffer, length);
      */
      break;
  }

  return (length > 0);
}

void CCSLink::disconnectAll()
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

void CCSLink::doBackgroundActivity()
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
