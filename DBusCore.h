/******************************************************************************
 *                                                                            *
 *  DBus Core for POSIX select                                                *
 *  Copyright 2012 by Artem Prilutskiy (artem@prilutskiy.ru)                  *
 *                                                                            *
 ******************************************************************************/

#ifndef DBUSCORE_H
#define DBUSCORE_H

#include <sys/select.h>
#include <sys/time.h>
#include <dbus/dbus.h>

class DBusCore
{
  public:

    DBusCore(DBusConnection* connection);
    ~DBusCore();

    void applyTimeout(struct timeval* timeout);
    void applySet(fd_set* read, fd_set* write, fd_set* except);
    void handleChanges(fd_set* read, fd_set* write, fd_set* except);

  private:

    DBusConnection* connection;
    DBusDispatchStatus status;

    struct Watcher
    {
      DBusWatch* watch;
      int handle;
      int flags;
      bool enabled;
      long mark;
      Watcher* next;
    };

    struct Timer
    {
      DBusTimeout* timeout;
      struct timeval time;
      bool enabled;
      long mark;
      Timer* next;
    };

    Watcher* watchers;
    Timer* timers;
    volatile long counter;

    dbus_bool_t addWatch(DBusWatch* watch);
    void removeWatch(DBusWatch* watch);
    void watchToggled(DBusWatch* watch);

    dbus_bool_t addTimeout(DBusTimeout* timeout);
    void removeTimeout(DBusTimeout* timeout);
    void timeoutToggled(DBusTimeout* timeout);

    static dbus_bool_t addWatch(DBusWatch* watch, void* data);
    static void removeWatch(DBusWatch* watch, void* data);
    static void watchToggled(DBusWatch* watch, void* data);
    static dbus_bool_t addTimeout(DBusTimeout* timeout, void* data);
    static void removeTimeout(DBusTimeout* timeout, void* data);
    static void timeoutToggled(DBusTimeout* timeout, void* data);
    static void dispatchStatus(DBusConnection* connection, DBusDispatchStatus status, void* data);

};

#endif
