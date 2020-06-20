#include "CCSModule.h"
#include "DStarTools.h"
#include <string.h>

CCSModule::CCSModule(config_setting_t* setting, CCSLink* link, Observer* observer, ReportHandler report, GateModule* next) :
  observer(observer),
  link(link),
  next(next),
  client(NULL),
  context(NULL)
{
  const char* file = NULL;
  const char* server = NULL;
  const char* location = NULL;

  if (setting != NULL)
  {
    name = *config_setting_name(setting);
    config_setting_lookup_string(setting, "file", &file);
    config_setting_lookup_string(setting, "server", &server);
    config_setting_lookup_string(setting, "location", &location);
  }

  if ((link != NULL) &&
      (server != NULL))
  {
    int port = link->getDefaultPort();
    context = new GateContext('A', NULL, server, port);
  }

  if ((file != NULL) &&
      (location != NULL))
  {
    client = new LiveLogClient(location, file);
    client->onUpdateData = handleUpdateData;
    client->onReport = report;
    client->userData = this;
  }
}

CCSModule::~CCSModule()
{
  if (context != NULL)
    delete context;
  if (client != NULL)
    delete client;
}

char CCSModule::getName()
{
  return name;
}

bool CCSModule::isActive()
{
  return (context != NULL) && (client != NULL);
}

GateModule* CCSModule::getNext()
{
  return next;
}

GateLink* CCSModule::getLink()
{
  return link;
}

GateContext* CCSModule::getContext()
{
  return context;
}

void CCSModule::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{

}

void CCSModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{

}

bool CCSModule::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  memset(route->repeater1, ' ', LONG_CALLSIGN_LENGTH);
  memset(route->repeater2, ' ', LONG_CALLSIGN_LENGTH);
  route->repeater1[MODULE_NAME_POSITION] = context->getModule();
  CopyDStarCall(call, route->yourCall, NULL, 0);
  *address = context->getAddress()->sin_addr;
  return true;
}

bool CCSModule::verifyRepeater(const char* call)
{
  return false;
}

bool CCSModule::verifyAddress(const struct in_addr& address)
{
  return (context->getAddress()->sin_addr.s_addr == address.s_addr);
}

void CCSModule::publishHeard(const struct DStarRoute& route, const char* addressee, const char* text)
{

}

void CCSModule::publishHeard(const struct DStarRoute& route, uint16_t number)
{
  const struct sockaddr_in* address = context->getAddress();
  link->publishHeard(*address, route);
}

void CCSModule::doBackgroundActivity(int type)
{
  if (type == ROUTINE_ACTIVITY)
    client->doBackgroundActivity();
}

void CCSModule::handleUpdateData(LiveLogClient* client, const char* call, const char* repeater)
{
  CCSModule* self = (CCSModule*)client->userData;
  self->observer->handleLocationUpdate(self, call, repeater);
}

void CCSModule::handleCommand(const char* command)
{

}