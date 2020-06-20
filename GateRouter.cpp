#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include "GateRouter.h"
#include "DStarTools.h"
#include "NetworkTools.h"

GateRouter::GateRouter(const char* address, const char* call, GateModule** modules) :
  onReport(NULL),
  modules(modules),
  sessions(NULL),
  enumeration(0)
{
  CopyDStarCall(call, this->call, NULL, COPY_MAKE_STRING);
}

GateRouter::~GateRouter()
{
  removeAllSessions();
}

void GateRouter::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

GateSession* GateRouter::createSession(
    GateModule* module, const struct DStarRoute& heard, const struct DStarRoute& target,
    uint16_t number, const struct sockaddr_in& source, const struct sockaddr_in& destination)
{
  enumeration ++;
  sessions = new GateSession(module, heard, target, source, number, destination, enumeration, sessions);
  return sessions;
}

GateSession* GateRouter::findSession(const struct sockaddr_in& address, uint16_t number)
{
  GateSession* session = sessions;
  while ((session != NULL) && (!session->matchesTo(address, number)))
    session = *session->getNext();
  return session;
}

void GateRouter::removeExpiredSessions(time_t time, time_t interval)
{
  time_t threshold = time - interval;
  GateSession** pointer = &sessions;
  while (*pointer != NULL)
  {
    GateSession* session = *pointer;
    if (session->getLastAccessTime() < threshold)
    {
      report(LOG_WARNING, "%04x: call timed out\n", session->getNumber());
      *pointer = *session->getNext();
      delete session;
      continue;
    }
    pointer = session->getNext();
  }
}

void GateRouter::removeSession(GateSession* object)
{
  GateSession** pointer = &sessions;
  while (*pointer != NULL)
  {
    GateSession* session = *pointer;
    if (session == object)
    {
      *pointer = *session->getNext();
      delete session;
      break;
    }
    pointer = session->getNext();
  }
}

void GateRouter::removeAllSessions()
{
  GateSession** pointer = &sessions;
  while (*pointer != NULL)
  {
    GateSession* session = *pointer;
    *pointer = *session->getNext();
    delete session;
  }
}

void GateRouter::doBackgroundActivity()
{
  time_t now = ::time(NULL);
  removeExpiredSessions(now);
}

GateModule* GateRouter::getModule(char name)
{
  GateModule* module = *modules;
  while (module != NULL)
  {
    if (module->getName() == name)
      return module;
    module = module->getNext();
  }
  return NULL;
}

GateModule* GateRouter::getModuleByRepeater(const char* call)
{
  GateModule* module = *modules;
  while (module != NULL)
  {
    if (module->verifyRepeater(call))
      return module;
    module = module->getNext();
  }
  return NULL;
}

GateModule* GateRouter::getDestinationModule(struct DStarRoute* route)
{
  if (strncmp(route->repeater2, call, MODULE_NAME_POSITION) == 0)
  {
    char name = route->repeater2[MODULE_NAME_POSITION];
    if ((*route->yourCall == '/') ||
        (strncmp(route->yourCall, CQCQCQ, LONG_CALLSIGN_LENGTH) == 0))
      name += '0' - 'A';
    return getModule(name);
  }
  return getModuleByRepeater(route->repeater2);
}

bool GateRouter::verifyAddress(const struct in_addr& address)
{
  GateModule* module = *modules;
  while (module != NULL)
  {
    if (module->verifyAddress(address))
      return true;
    module = module->getNext();
  }
  return false;
}

void GateRouter::processData(const struct sockaddr_in& address, uint16_t session, uint8_t sequence, struct DStarRoute* route, struct DStarDVFrame* frame)
{
  GateSession* context = findSession(address, session);

  if ((context == NULL) &&
      (route != NULL) &&
      (verifyAddress(address.sin_addr)))
  {
    GateModule* module = getDestinationModule(route);

    report(LOG_INFO, "%c: processing call %.8s (%s) -> %.8s\n",
      (module != NULL) ? module->getName() : '?',
      route->ownCall1,
      inet_ntoa(address.sin_addr),
      route->yourCall);

    struct DStarRoute heard;
    struct DStarRoute target;

    struct in_addr source;
    struct sockaddr_in destination;

    memcpy(&heard, route, sizeof(struct DStarRoute));
    memcpy(&target, route, sizeof(struct DStarRoute));

    if ((module != NULL) &&
        (module->findRoute(route->ownCall1, &heard, &source)) &&
        (module->findRoute(route->yourCall, &target, &destination.sin_addr)) &&
        (memcmp(route->repeater1, target.repeater1, LONG_CALLSIGN_LENGTH) != 0))
    {
      memcpy(heard.yourCall, route->yourCall, LONG_CALLSIGN_LENGTH);
      SwapDStarCalls(heard.repeater1, heard.repeater2);

      destination.sin_family = AF_INET;
      destination.sin_port = htons(module->getLink()->getDefaultPort());

      context = createSession(module, heard, target, session, address, destination);
      report(LOG_INFO, "%04x: call routed through %.8s\n", context->getNumber(), target.repeater2);
    }
  }

  if ((context != NULL) &&
      (frame != NULL) &&
      (!context->forward(sequence, frame)))
  {
    report(LOG_INFO, "%04x: call ended\n", context->getNumber());
    removeSession(context);
  }
}
