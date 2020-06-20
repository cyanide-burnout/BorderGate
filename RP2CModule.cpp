#include "RP2CModule.h"
#include "NetworkTools.h"
#include "DStarTools.h"
#include <string.h>

#ifdef LIBCONFIG_VER_MAJOR
typedef int IntegerSetting;
#else
typedef long int IntegerSetting;
#endif

RP2CModule::RP2CModule(config_setting_t* setting, GateLink* link, Observer* observer, ReportHandler handler, GateModule* next) :
  observer(observer),
  link(link),
  next(next),
  emulator(NULL),
  directory(NULL),
  client(NULL),
  cache(NULL),
  name(' ')
{
  const char* file = NULL;
  const char* import = NULL;
  const char* address = "*";
  const char* connection = NULL;
  const char* options = NULL;
  IntegerSetting port = RP2C_CONTROLLER_PORT;

  if (setting != NULL)
  {
    name = *config_setting_name(setting);
    config_setting_lookup_string(setting, "file", &file);
    config_setting_lookup_string(setting, "import", &import);
    config_setting_lookup_string(setting, "address", &address);
    config_setting_lookup_string(setting, "connection", &connection);
    config_setting_lookup_string(setting, "cache", &options);
    config_setting_lookup_int(setting, "port", &port);
  }

  if ((connection != NULL) && (file != NULL))
  {
    directory = new RP2CDirectory(connection, file);
    directory->userData = this;
    directory->onReport = (RP2CDirectory::ReportHandler)handler;
    directory->onUpdateData = handleUpdateData;
    if (directory->checkConnectionError())
    {
      delete directory;
      directory = NULL;
    }
  }

  if ((address != NULL) && (directory != NULL))
  {
    emulator = new RP2CEmulator(address, port);
    emulator->onReport = (RP2CEmulator::ReportHandler)handler;
    if (emulator->getHandle() < 0)
    {
      delete directory;
      directory = NULL;
      delete emulator;
      emulator = NULL;
    }
  }

  if ((import != NULL) && (emulator != NULL))
  {
    client = new MonitorClient(import);
    client->onReport = (MonitorClient::ReportHandler)handler;
    client->checkConnectionError();
  }

  if (options != NULL)
  {
    cache = new DataCache(options);
    cache->onReport = (DataCache::ReportHandler)handler;
    if (cache->error())
    {
      delete cache;
      cache = NULL;
    }
  }

}

RP2CModule::~RP2CModule()
{
  if (client != NULL)
    delete client;
  if (emulator != NULL)
    delete emulator;
  if (directory != NULL)
    delete directory;
  if (cache != NULL)
    delete cache;
}

char RP2CModule::getName()
{
  return name;
}

bool RP2CModule::isActive()
{
  return (name > ' ') && (directory != NULL) && (emulator != NULL);
}

GateModule* RP2CModule::getNext()
{
  return next;
}

GateLink* RP2CModule::getLink()
{
  return link;
}

GateContext* RP2CModule::getContext()
{
  return NULL;
}

void RP2CModule::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if (type == REAL_TIME_ACTIVITY)
    FD_SET(emulator->getHandle(), read);
}

void RP2CModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if ((type == REAL_TIME_ACTIVITY) && FD_ISSET(emulator->getHandle(), read))
    emulator->receiveData();
}

bool RP2CModule::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  return directory->findRoute(call, route, address);
}

bool RP2CModule::verifyRepeater(const char* call)
{
  return directory->verifyRepeater(call);
}

bool RP2CModule::verifyAddress(const struct in_addr& address)
{
  return directory->verifyAddress(address);
}

void RP2CModule::publishHeard(const struct DStarRoute& route, const char* addressee, const char* text)
{
  if (emulator->assertConnection())
    emulator->publishHeard(route);
}

void RP2CModule::publishHeard(const struct DStarRoute& route, uint16_t number)
{
  if (emulator->assertConnection())
    emulator->publishHeard(route);
  if (client != NULL)
    client->publishHeard(route);
}

void RP2CModule::doBackgroundActivity(int type)
{
  switch (type)
  {
    case REAL_TIME_ACTIVITY:
      emulator->doBackgroundActivity();
      break;

    case ROUTINE_ACTIVITY:
      if (emulator->isConnected() && !directory->isConnected())
        directory->connect();
      if (!emulator->isConnected() && directory->isConnected())
        directory->disconnect();
      if (directory->isConnected())
        directory->doBackgroundActivity();
      if (client != NULL)
        client->doBackgroundActivity();
      break;
  }
}

void RP2CModule::handleCommand(const char* command)
{
  if (strncmp(command, "initiate ", 9) == 0)
  {
    struct sockaddr_in address;
    ParseAddressString(&address, command + 9);
    emulator->initiateConnection(address);
  }
}

void RP2CModule::handleUpdateData(const char* date, const char* station, const char* repeater, const char* gateway)
{
  observer->handleLocationUpdate(this, station, repeater);
  if ((cache != NULL) && (cache->add(repeater, &name, sizeof(name))))
    observer->handleRoutingUpdate(this, repeater, gateway);
}

void RP2CModule::handleUpdateData(RP2CDirectory* directory, const char* date, const char* call1, const char* call2, const char* call3)
{
  RP2CModule* self = (RP2CModule*)directory->userData;
  self->handleUpdateData(date, call1, call2, call3);
}
