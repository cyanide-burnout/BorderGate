#include "MasterModule.h"
#include "DStarTools.h"
#include <sys/eventfd.h>
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
  handle1(EOF),
  handle2(EOF),
  connection(NULL)
{
  const char* server = NULL;
  const char* queue  = NULL;
  const char* file   = NULL;

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
    handle1 = open(file, O_CREAT | O_RDWR, 0644);

    if (handle1 != EOF)
    {
      ftruncate(handle1, sizeof(time_t));
      date = (time_t*)mmap(NULL, sizeof(time_t), PROT_READ | PROT_WRITE, MAP_SHARED, handle1, 0);
    }
  }

  if (queue != NULL)
  {
    int port   = MOSQUITTO_DEFAULT_PORT;
    char* host = strdup(queue);

    char* delimiter = strchr(host, ':');
    if (delimiter != NULL)
    {
      *delimiter = '\0';
      port = atoi(delimiter + 1);
    }

    connection = mosquitto_new(CLIENT_IDENTIFIER, true, this);

    mosquitto_reconnect_delay_set(connection, 2, 10, false);
    mosquitto_connect_callback_set(connection, handleConnect);
    mosquitto_message_callback_set(connection, handleMessage);

    int result = mosquitto_connect(connection, host, port, CLIENT_KEEPALIVE);

    free(host);

    if (result != MOSQ_ERR_SUCCESS)
    {
      checkMosquittoError(result);
      return;
    }

    mosquitto_loop_start(connection);

    handle2 = eventfd(0, 0);
    sem_init(&semaphore, 0, 0);
  }
}

MasterModule::~MasterModule()
{
  if (handle2 != EOF)
  {
    mosquitto_disconnect(connection);
    sem_post(&semaphore);
    sem_destroy(&semaphore);
    close(handle2);
  }

  if (connection != NULL)
  {
    mosquitto_loop_stop(connection, false);
    mosquitto_destroy(connection);
  }

  if (date != NULL)
  {
    munmap(date, sizeof(time_t));
    close(handle1);
  }
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
    (remote.s_addr != htonl(INADDR_NONE)) &&
    (mosquitto_socket(connection) >= 0);
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
    FD_SET(handle2, read);
}

void MasterModule::processDescriptors(int type, fd_set* read, fd_set* write, fd_set* except)
{
  if ((type == ROUTINE_ACTIVITY) &&
      (FD_ISSET(handle2, read)))
  {
    uint64_t value;
    ::read(handle2, &value, sizeof(uint64_t));
    handleMessage((const char*)value);
    sem_post(&semaphore);
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
  write(self->handle2, message->payload, sizeof(uint64_t));
  sem_wait(&self->semaphore);
}
