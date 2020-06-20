#include "HSStore.h"

#include <arpa/inet.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>

#include "DStarTools.h"


#define MAXIMUM_SETTING_LENGTH  100
#define MAXIMUM_ADDRESS_LENGTH  16

#define INDEX_TABLE0     0
#define INDEX_TABLE1     1
#define INDEX_GATEWAYS   2
#define INDEX_ADDRESSES  3

#define INDEX_COUNT      4
#define ATTEMPT_COUNT    2


HSStore::HSStore(const char* file) : 
  MySQLStore(file),
  database(NULL),
  password(NULL)
{
  dena::config configuration;

  FILE* handle = fopen(file, "r");
  char buffer[MAXIMUM_SETTING_LENGTH];
  while (fgets(buffer, sizeof(buffer), handle) != NULL)
  {
    char* key = strtok(buffer, "=");
    char* value = strtok(NULL, "\n");
    if (value != NULL)
    {
      if (strcmp(key, "host") == 0)
      {
        configuration["host"] = std::string(value);
        continue;
      }
      if (strcmp(key, "loose_handlersocket_port") == 0)
      {
        configuration["port"] = std::string(value);
        continue;
      }
      if (strcmp(key, "database") == 0)
      {
        database = strdup(value);
        continue;
      }
      if (strcmp(key, "handlersocket_plain_secret") == 0)
      {
        password = strdup(value);
        continue;
      }
    }
  }
  fclose(handle);

  dena::socket_args arguments;
  arguments.set(configuration);
  client = dena::hstcpcli_i::create(arguments);

  if (client.get() != NULL)
    prepareConnection();
}

HSStore::~HSStore()
{
  client.reset();
  free(database);
  free(password);
}

bool HSStore::checkConnectionError()
{
  if (client.get() == NULL)
  {
    report(LOG_ERR, "HandlerSocket not connected.");
    return true;
  }
  if (client->get_error_code() != 0)
  {
    report(LOG_ERR, "HandlerSocket error: %s\n", client->get_error().c_str());
    return true;
  }
  return MySQLStore::checkConnectionError();
}

bool HSStore::prepareConnection()
{
  int number = INDEX_COUNT;

  if (password != NULL)
  {
    client->request_buf_auth(password, NULL);
    number ++;
  }

  client->request_buf_open_index(INDEX_TABLE0, database, "Table0", "PRIMARY", "Call1,Call2");
  client->request_buf_open_index(INDEX_TABLE1, database, "Table1", "PRIMARY", "Call1,Call2");
  client->request_buf_open_index(INDEX_GATEWAYS, database, "Gateways", "PRIMARY", "Call,Address");
  client->request_buf_open_index(INDEX_ADDRESSES, database, "Gateways", "Addresses", "Address,Call");

  int code = client->request_send();
  while ((code == 0) && (number > 0))
  {
    size_t count = 0;
    code = client->response_recv(count);
    client->response_buf_remove();
    number --;
  }

  return (code == 0);
}

bool HSStore::readIndexData(int index, const char* key, size_t length, char* buffer, size_t* size)
{
  *buffer = '\0';

  int code = 0;
  const dena::string_ref operation1("=", 1);
  const dena::string_ref operation2;
  const dena::string_ref keys[] =
  {
    dena::string_ref(key, length)
  };

  for (size_t attempts = 0; attempts < ATTEMPT_COUNT; attempts ++)
  {
    client->request_buf_exec_generic(index, operation1, keys, 1, 1, 0, operation2, 0, 0);
    code = client->request_send();

    if (code == 0)
      break;

    client->reconnect();
    prepareConnection();
  }

  if (code == 0)
  {
    size_t count = 0;
    code = client->response_recv(count);
    if (code == 0)
    {
      const dena::string_ref* row = client->get_next_row();
      if ((count >= 2) && (row != NULL))
      {
        if (*size > row[1].size())
          *size = row[1].size();
        memcpy(buffer, row[1].begin(), *size);
      }
    }
    client->response_buf_remove();
  }
  
  checkConnectionError();
  return (*buffer != '\0');
}

bool HSStore::findRoute(const char* call, struct DStarRoute* route, struct in_addr* address)
{
  char filter[LONG_CALLSIGN_LENGTH + 1];
  CopyDStarCall(call, filter, NULL, COPY_MAKE_STRING);

  char buffer[MAXIMUM_ADDRESS_LENGTH];
  size_t length1 = LONG_CALLSIGN_LENGTH;
  size_t length2 = sizeof(buffer) - 1;

  if (readIndexData(INDEX_TABLE1, filter, length1, route->repeater1, &length1) &&
      readIndexData(INDEX_GATEWAYS, route->repeater1, length1, buffer, &length2) &&
      (length2 > 0))
  {
    buffer[length2] = '\0';
    inet_aton(buffer, address);
    CopyDStarCall(CQCQCQ, route->yourCall, NULL, 0);
    CopyDStarCall(call, route->repeater2, NULL, 0);
    return true;
  }

  if (readIndexData(INDEX_TABLE0, filter, length1, route->repeater2, &length1) &&
      readIndexData(INDEX_TABLE1, route->repeater2, length1, route->repeater1, &length1) &&
      readIndexData(INDEX_GATEWAYS, route->repeater1, length1, buffer, &length2) &&
      (length2 > 0))
  {
    buffer[length2] = '\0';
    inet_aton(buffer, address);
    CopyDStarCall(call, route->yourCall, NULL, 0);
    return true;
  }

  return false;
}

bool HSStore::verifyRepeater(const char* call)
{
  char filter[LONG_CALLSIGN_LENGTH + 1];
  CopyDStarCall(call, filter, NULL, COPY_MAKE_STRING);

  char buffer[MAXIMUM_ADDRESS_LENGTH];
  size_t length1 = LONG_CALLSIGN_LENGTH;
  size_t length2 = sizeof(buffer) - 1;

  return readIndexData(INDEX_TABLE1, filter, length1, buffer, &length2);
}

bool HSStore::verifyAddress(const struct in_addr& address)
{
  char* filter = inet_ntoa(address);
  char buffer[LONG_CALLSIGN_LENGTH];
  size_t length1 = strlen(filter);
  size_t length2 = sizeof(buffer);
  return readIndexData(INDEX_ADDRESSES, filter, length1, buffer, &length2);
}
