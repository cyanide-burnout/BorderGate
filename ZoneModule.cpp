#include "ZoneModule.h"
#include "DStarTools.h"
#include <string.h>

ZoneModule::ZoneModule(config_setting_t* setting, GateLink* link, GateModule* next) :
  link(link),
  next(next),
  context(NULL)
{
  const char* call = NULL;
  const char* server = NULL;

  const char* section = config_setting_name(setting);
  size_t length = strlen(section);
  name = section[length - 1];

  config_setting_lookup_string(setting, "call", &call);
  config_setting_lookup_string(setting, "server", &server);

  if ((link != NULL) &&
      (call != NULL) &&
      (server != NULL))
  {
    char letter = name - '0' + 'A';
    int port = link->getDefaultPort();
    context = new GateContext(letter, call, server, port);
  }
}

ZoneModule::~ZoneModule()
{
  if (context != NULL)
    delete context;
}

char ZoneModule::getName()
{
  return name;
}

bool ZoneModule::isActive()
{
  return (context != NULL);
}

GateModule* ZoneModule::getNext()
{
  return next;
}

GateLink* ZoneModule::getLink()
{
  return link;
}

GateContext* ZoneModule::getContext()
{
  return context;
}

void ZoneModule::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{

}

void ZoneModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{

}

bool ZoneModule::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  const char* reflector = context->getCall();
  memcpy(route->repeater1, reflector, LONG_CALLSIGN_LENGTH);
  memcpy(route->repeater2, reflector, LONG_CALLSIGN_LENGTH);
  memcpy(route->yourCall, CQCQCQ, LONG_CALLSIGN_LENGTH);
  route->repeater1[MODULE_NAME_POSITION] = 'G';
  *address = context->getAddress()->sin_addr;
  return true;
}

bool ZoneModule::verifyRepeater(const char* call)
{
  return
    (CompareDStarCall(call, link->getCall(), COMPARE_SKIP_MODULE) == 0) &&
    (GetDStarModule(call) == context->getModule());
}

bool ZoneModule::verifyAddress(const struct in_addr& address)
{
  return (context->getAddress()->sin_addr.s_addr == address.s_addr);
}

void ZoneModule::publishHeard(const struct DStarRoute& route, const char* addressee, const char* text)
{

}

void ZoneModule::publishHeard(const struct DStarRoute& route, uint16_t number)
{

}

void ZoneModule::doBackgroundActivity(int type)
{

}

void ZoneModule::handleCommand(const char* command)
{

}