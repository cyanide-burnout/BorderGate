#include "MasterModule.h"
#include "DStarTools.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

#define MOSQUITTO_DEFAULT_PORT  1883

#define CLIENT_IDENTIFIER       "BorderGate"
#define CLIENT_KEEPALIVE        60

MasterModule::MasterModule(config_setting_t* setting, GateLink* link, Observer* observer, ReportHandler handler, GateModule* next) :
  observer(observer),
  handler(handler),
  link(link),
  next(next),
  name(' '),
  date(NULL),
  handle(EOF),
  connection(NULL)
{
  const char* server = NULL;
  const char* queue = NULL;
  const char* file = NULL;

  if (setting != NULL)
  {
    name = *config_setting_name(setting);
    config_setting_lookup_string(setting, "server", &server);
    config_setting_lookup_string(setting, "queue", &queue);
    config_setting_lookup_string(setting, "file", &file);
  }

  remote.s_addr = htonl(INADDR_NONE);

  if (server != NULL)
  {
    struct hostent* entry = gethostbyname(server);
    if ((entry != NULL) && (entry->h_addrtype == AF_INET))
      memcpy(&remote.s_addr, entry->h_addr, entry->h_length);
  }

  if (file != NULL)
  {
    handle = open(file, O_CREAT | O_RDWR, 0644);
    if (handle != EOF)
    {
      struct stat status;
      fstat(handle, &status);
      if (status.st_size < sizeof(time_t))
      {
        time_t now = 0;
        write(handle, &now, sizeof(time_t));
      }
      date = (time_t*)mmap(NULL, sizeof(time_t), PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
    }
  }

  if (queue != NULL)
  {
    int port = MOSQUITTO_DEFAULT_PORT;
    char* host = strdup(queue);
    // Parse port number if exists
    char* delimiter = strchr(host, ':');
    if (delimiter != NULL)
    {
      *delimiter = '\0';
      port = atoi(delimiter + 1);
    }
    // Create connection
    connection = mosquitto_new(CLIENT_IDENTIFIER, true, this);
    int result = mosquitto_connect(connection, host, port, CLIENT_KEEPALIVE);
    // Release string
    free(host);

    mosquitto_connect_callback_set(connection, handleConnect);
    mosquitto_message_callback_set(connection, handleMessage);

    checkMosquittoError(result);
  }
}

MasterModule::~MasterModule()
{
  mosquitto_destroy(connection);
  munmap(date, sizeof(time_t));
  close(handle);
}

char MasterModule::getName()
{
  return name;
}

bool MasterModule::isActive()
{
  return
    (name > ' ') &&
    (connection != NULL) &&
    (remote.s_addr != htonl(INADDR_NONE));
}

GateModule* MasterModule::getNext()
{
  return next;
}

GateLink* MasterModule::getLink()
{
  return link;
}

GateContext* MasterModule::getContext()
{
  return NULL;
}

void MasterModule::addDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if (type == ROUTINE_ACTIVITY)
  {
    int handle = mosquitto_socket(connection);
    FD_SET(handle, read);
    if (mosquitto_want_write(connection))
      FD_SET(handle, write);
  }
}

void MasterModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if (type == ROUTINE_ACTIVITY)
  {
    int result = MOSQ_ERR_SUCCESS;
    int handle = mosquitto_socket(connection);
    if (FD_ISSET(handle, read))
      result = mosquitto_loop_read(connection, 1);
    if (FD_ISSET(handle, write))
      result = mosquitto_loop_write(connection, 1);
    if (result == MOSQ_ERR_CONN_LOST)
      mosquitto_reconnect_async(connection);
  }
}

bool MasterModule::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  memset(route->repeater1, ' ', LONG_CALLSIGN_LENGTH);
  memset(route->repeater2, ' ', LONG_CALLSIGN_LENGTH);
  address->s_addr = remote.s_addr;
  return true;
}

bool MasterModule::verifyRepeater(const char* call)
{
  return false;
}

bool MasterModule::verifyAddress(const struct in_addr& address)
{
  return (address.s_addr == remote.s_addr);
}

void MasterModule::publishHeard(const struct DStarRoute& route, const char* addressee, const char* text)
{

}

void MasterModule::publishHeard(const struct DStarRoute& route, uint16_t number)
{
  char* topic;
  char* message;
  asprintf(&topic, "Gate/%c/Announce/Outgoing", name);
  asprintf(&message, "%.8s%.8s", route.ownCall1, route.repeater1);
  int result = mosquitto_publish(
    connection, NULL, topic,
    strlen(message), message,
    1, false);
  checkMosquittoError(result);
  free(message);
  free(topic);
}

void MasterModule::doBackgroundActivity(int type)
{
  if (type == ROUTINE_ACTIVITY)
    mosquitto_loop_misc(connection);
}

void MasterModule::handleCommand(const char* command)
{

}

void MasterModule::handleMessage(const char* data)
{
  observer->handleLocationUpdate(this, data, "UNKNOWNA");

  time(date);
  msync(date, sizeof(time_t), MS_ASYNC);
}

void MasterModule::report(int priority, const char* format, ...)
{
  if (handler != NULL)
  {
    va_list arguments;
    va_start(arguments, format);
    handler(priority, format, arguments);
    va_end(arguments);
  }
}

void MasterModule::checkMosquittoError(int result)
{
  if (result == MOSQ_ERR_ERRNO)
  {
    const char* error = strerror(errno);
    report(LOG_CRIT, "Mosquitto error: %s\n", error);
    return;
  }
  if (result != MOSQ_ERR_SUCCESS)
  {
    const char* error = mosquitto_strerror(result);
    report(LOG_CRIT, "Mosquitto error: %s\n", error);
  }
}

void MasterModule::handleConnect(struct mosquitto* connection, void* user, int result)
{
  MasterModule* module = (MasterModule*)user;
  if (result == MOSQ_ERR_SUCCESS)
  {
    char* topic;
    asprintf(&topic, "Gate/%c/Announce/Incoming", module->name);
    result = mosquitto_subscribe(connection, NULL, topic, 1);
    free(topic);
  }
  module->checkMosquittoError(result);
}

void MasterModule::handleMessage(struct mosquitto* connection, void* user, const struct mosquitto_message* message)
{
  MasterModule* self = (MasterModule*)user;
  self->handleMessage((const char*)message->payload);
}
