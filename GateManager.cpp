#include <string.h>
#include <syslog.h>
#include "GateManager.h"
#include "DStarTools.h"

#define DDB_PUBLISH_LIMIT  15

GateManager::GateManager(const char* call, GateModule** modules, Capability capability, char** announces, char** outcasts) :
  onReport(NULL),
  modules(modules),
  outcasts(outcasts),
  announces(announces),
  capability(capability),
  count(0),
  time(0)
{
  CopyDStarCall(call, this->call, NULL, COPY_CLEAR_MODULE);
}

void GateManager::report(int priority, const char* format, ...)
{
  if (onReport != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    onReport(priority, format, arguments);
    va_end(arguments);
  }
}

GateModule** GateManager::getModuleList()
{
  return modules;
}

void GateManager::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    module->addDescriptors(type, read, write, except);
}

void GateManager::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    module->processDescriptors(type, read, write, except);
}

void GateManager::doBackgroundActivity(int type)
{
  for (GateModule* module = *modules; module != NULL; module = module->getNext())
    module->doBackgroundActivity(type);
}

void GateManager::removeAllModules()
{
  GateModule** pointer = modules;
  while (*pointer != NULL)
  {
    GateModule* module = *pointer;
    *pointer = module->getNext();
    delete module;
  }
}

bool GateManager::hasLimitExceeded()
{
  time_t now;
  if ((count > DDB_PUBLISH_LIMIT) && (::time(&now) > time))
  {
    time = now;
    count = 0;
  }
  return (count > DDB_PUBLISH_LIMIT);
}

int GateManager::getOperationCount()
{
  return count;
}

void GateManager::considerOperation(int number)
{
  time_t now;
  if (::time(&now) > time)
  {
    time = now;
    count = 0;
  }
  count += number;
}

bool GateManager::isBanned(const char* call)
{
  if (outcasts != NULL)
    for (size_t index = 0; outcasts[index] != NULL; index ++)
      if (CompareDStarCall(outcasts[index], call, COMPARE_SKIP_MODULE) == 0)
        return true;
  return false;
}

void GateManager::publishHeard(char origin, const char* area, const char* call1, const char* call2, Scope scope)
{
  struct DStarRoute route;
  memset(&route, 0, sizeof(route));

  if ((CopyDStarCall(call2, route.yourCall, NULL, 0) == 0) &&
      (CopyDStarCall(call1, route.ownCall1, NULL, 0) == 0) &&
      (*route.ownCall1 != ' '))
  {
    memset(route.ownCall2, ' ', SHORT_CALLSIGN_LENGTH);
    memcpy(route.repeater2, call, LONG_CALLSIGN_LENGTH);
    route.repeater2[MODULE_NAME_POSITION] = 'G';

    if ((area == NULL) ||
        (CopyDStarCall(area, route.repeater1, NULL, 0) != 0))
    {
      memcpy(route.repeater1, call, LONG_CALLSIGN_LENGTH);
      route.repeater1[MODULE_NAME_POSITION] = origin;
    }

    GateModule* module = *modules;
    while (module != NULL)
    {
      if (((origin >= 'A') && (strchr(announces[origin - 'A'], module->getName()) != NULL)) ||
          (scope == All))
      {
        report(LOG_INFO, "%c: registering %.8s / %.8s (heard at %c)\n", module->getName(), route.ownCall1, route.repeater1, origin);
        module->publishHeard(route, 0);
      }
      module = module->getNext();
    }
  }
}

void GateManager::handleRoutingUpdate(GateModule* module, const char* repeater, const char* gateway)
{
  /*

  // Warning: extended and full capability are not applicable now!

  if ((capability >= Extended) &&
      (CompareDStarCall(gateway, call, COMPARE_SKIP_MODULE) != 0) &&
      (!isBanned(gateway)))
  {
    char origin = module->getName();
    publishHeard(origin, NULL, repeater, VISIBILITY_ON, LocalAndGroup);
    publishHeard(origin, NULL, repeater, CQCQCQ, LocalAndGroup);
    considerOperation(2);
    if ((capability == Full) &&
        (origin != 'A'))
    {
      publishHeard(origin, repeater, call, CQCQCQ, Group);
      considerOperation();
    }
  }
  */
}

void GateManager::handleLocationUpdate(GateModule* module, const char* station, const char* repeater)
{
  if ((repeater == NULL) ||
      (CompareDStarCall(station, call, COMPARE_SKIP_MODULE) != 0) &&
      (CompareDStarCall(repeater, call, COMPARE_SKIP_MODULE) != 0) &&
      (!isBanned(station)) &&
      (!isBanned(repeater)))
  {
    publishHeard(module->getName(), NULL, station, CQCQCQ, Group);
    considerOperation();
  }
}

void GateManager::handleVisibilityUpdate(GateModule* module, const char* station, bool state)
{
  struct in_addr address;
  struct DStarRoute route;
  if ((module->findRoute(station, &route, &address)) &&
      (CompareDStarCall(route.repeater2, call, COMPARE_SKIP_MODULE) != 0) &&
      (!isBanned(route.repeater2)))
  {
    const char* value = state ? VISIBILITY_ON : VISIBILITY_OFF;
    publishHeard(module->getName(), NULL, station, value, Group);
    considerOperation();
  }
}

void GateManager::setCallVisibility(const char* station, bool state)
{
  const char* value = state ? VISIBILITY_ON : VISIBILITY_OFF;
  publishHeard('A', NULL, station, value, All);
  considerOperation();
}

void GateManager::invokeModuleCommand(char name, const char* command)
{
  GateModule* module = *modules;
  while (module != NULL)
  {
    if (module->getName() == name)
      module->handleCommand(command);
    module = module->getNext();
  }
}
