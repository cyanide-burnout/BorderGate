#include "GateService.h"
#include <string.h>

#define INTERFACE_NAME      "me.burnaway.BorderGate"
#define SERVICE_PATH        "/me/burnaway/BorderGate"
#define SERVICE_NAME        INTERFACE_NAME

GateService::GateService(DBusConnection* connection, GateManager* manager) :
  connection(connection),
  manager(manager)
{
  dbus_error_init(&error);
  dbus_connection_ref(connection);

  bzero(&table, sizeof(table));
  table.message_function = &handleMethodCall;

  int result = dbus_bus_request_name(connection,
    SERVICE_NAME,
    DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE,
    &error);

  if (!dbus_error_is_set(&error) &&
      !dbus_connection_register_object_path(connection, SERVICE_PATH, &table, this))
    dbus_set_error(&error, DBUS_ERROR_OBJECT_PATH_IN_USE, NULL);
}

GateService::~GateService()
{
  dbus_bus_release_name(connection, SERVICE_NAME, NULL);
  dbus_connection_unref(connection);
  dbus_error_free(&error);
}

const char* GateService::getError()
{
  return error.name;
}

GateManager* GateService::getManager()
{
  return manager;
}

DBusConnection* GateService::getConnection()
{
  return connection;
}

DBusHandlerResult GateService::handleMethodCall(DBusConnection* connection, DBusMessage* message, void* data)
{
  GateService* self = (GateService*)data;
  GateManager* manager = self->manager;

  char* call = NULL;
  dbus_bool_t state;

  if (dbus_message_is_method_call(message, INTERFACE_NAME, "setCallVisibility") &&
    dbus_message_get_args(message, NULL,
      DBUS_TYPE_STRING, &call,
      DBUS_TYPE_BOOLEAN, &state,
      DBUS_TYPE_INVALID))
  {
    manager->setCallVisibility(call, state);
    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_connection_flush(connection);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  char* module = NULL;
  char* command = NULL;

  if (dbus_message_is_method_call(message, INTERFACE_NAME, "invokeModuleCommand") &&
    dbus_message_get_args(message, NULL,
      DBUS_TYPE_STRING, &module,
      DBUS_TYPE_STRING, &command,
      DBUS_TYPE_INVALID))
  {
    manager->invokeModuleCommand(*module, command);
    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_connection_flush(connection);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}