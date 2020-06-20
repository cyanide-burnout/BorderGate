#ifndef GATESERVICE_H
#define GATESERVICE_H

#include <dbus/dbus.h>
#include "GateManager.h"

class GateService
{
  public:

    GateService(DBusConnection* connection, GateManager* manager);
    ~GateService();

    const char* getError();
    GateManager* getManager();
    DBusConnection* getConnection();

  private:

    DBusConnection* connection;
    GateManager* manager;

    DBusError error;
    DBusObjectPathVTable table;

    static DBusHandlerResult handleMethodCall(DBusConnection* connection, DBusMessage* message, void* data);

};

#endif
