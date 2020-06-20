#include "DDBModule.h"
#include "StoreFactory.h"

#ifdef LIBCONFIG_VER_MAJOR
typedef int IntegerSetting;
#else
typedef long int IntegerSetting;
#endif

DDBModule::DDBModule(config_setting_t* setting, const char* version, GateLink* link, Observer* observer, ReportHandler handler, GateModule* next) :
  observer(observer),
  link(link),
  next(next),
  client(NULL),
  store(NULL),
  name(' ')
{
  const char* server = NULL;;
  const char* user = NULL;
  const char* password = NULL;
  const char* file = NULL;
  const char* type = NULL;
  IntegerSetting port = DDB_DEFAULT_PORT;
  
  if (setting != NULL)
  {
    name = *config_setting_name(setting);
    config_setting_lookup_string(setting, "store", &type);
    config_setting_lookup_string(setting, "file", &file);
    config_setting_lookup_string(setting, "server", &server);
    config_setting_lookup_string(setting, "name", &user);
    config_setting_lookup_string(setting, "password", &password);
    config_setting_lookup_int(setting, "port", &port);
  }

  if ((type != NULL) && (file != NULL))
    store = StoreFactory::createStore(type, file, handler);

  if ((server != NULL) && (user != NULL) && (password != NULL) && (store != NULL))
  {
    client = new DDBClient(store, server, port, user, password, version);
    client->userData = this;
    client->onReport = handler;
    client->onUpdateCommand = handleUpdateCommand;
  }

  time_t now = ::time(NULL);
  time1 = now + DDB_TOUCH_INTERVAL;
  time2 = now + DDB_RECONNECT_INTERVAL;
}

DDBModule::~DDBModule()
{
  if (client != NULL)
    delete client;
  if (store != NULL)
    delete store;
}

char DDBModule::getName()
{
  return name;
}

bool DDBModule::isActive()
{
  return (name > ' ') && (store != NULL) && (client != NULL);
}

GateModule* DDBModule::getNext()
{
  return next;
}

GateLink* DDBModule::getLink()
{
  return link;
}

GateContext* DDBModule::getContext()
{
  return NULL;
}

void DDBModule::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if (type == ROUTINE_ACTIVITY)
  {
    int descriptors = FD_SETSIZE;
    irc_add_select_descriptors(client->getSession(), read, write, &descriptors);
  }
}

void DDBModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if (type == ROUTINE_ACTIVITY)
  {
    if (irc_process_select_descriptors(client->getSession(), read, write) != 0)
    {
      client->disconnect();
      client->connect();
    }
  }
}

bool DDBModule::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  return store->findRoute(call, route, address);
}

bool DDBModule::verifyRepeater(const char* call)
{
  // TODO: Implement this!
  // return store->verifyRepeater(call);
  return false;
}

bool DDBModule::verifyAddress(const struct in_addr& address)
{
  return store->verifyAddress(address);
}

void DDBModule::publishHeard(const struct DStarRoute& route, const char* addressee, const char* text)
{
  client->publishHeard(route, addressee, text);
}

void DDBModule::publishHeard(const struct DStarRoute& route, uint16_t number)
{
  client->publishHeard(route, number);
}

void DDBModule::handleUpdateCommand(DDBClient* client, int table, const char* date, const char* call1, const char* call2)
{
  DDBModule* self = (DDBModule*)client->userData;
  Observer* observer = self->observer;

  switch (table)
  {
    case DDB_TABLE_0:
      observer->handleLocationUpdate(self, call1, call2);
      break;

    case DDB_TABLE_1:
      observer->handleRoutingUpdate(self, call1, call2);
      break;
    
    case DDB_TABLE_2:
      bool visible = (*call2 == 'X');
      observer->handleVisibilityUpdate(self, call1, visible);
      break;
  }
}

void DDBModule::doBackgroundActivity(int type)
{
  if (type == ROUTINE_ACTIVITY)
  {
    time_t now = ::time(NULL);

    if (now > time1)
    {
      client->touch(now);
      time1 = now + DDB_TOUCH_INTERVAL;
    }

    if (now > time2)
    {
      client->disconnect();
      client->connect();
      time2 = now + DDB_RECONNECT_INTERVAL;
    }
  }
}

void DDBModule::handleCommand(const char* command)
{
  if (strcmp(command, "reconnect") == 0)
  {
    client->disconnect();
    client->connect();
  }
}