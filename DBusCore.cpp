/******************************************************************************
 *                                                                            *
 *  DBus Core for POSIX select                                                *
 *  Copyright 2012 by Artem Prilutskiy (artem@prilutskiy.ru)                  *
 *                                                                            *
 ******************************************************************************/

// http://lists.freedesktop.org/archives/dbus/2005-June/002764.html
// http://illumination.svn.sourceforge.net/viewvc/illumination/trunk/Illumination/src/Lum/OS/X11/Display.cpp?revision=808&view=markup

#include "DBusCore.h"
#include <stdlib.h>

#ifdef DBUSCORE_H

DBusCore::DBusCore(DBusConnection* connection) :
  connection(connection),
  status(DBUS_DISPATCH_DATA_REMAINS),
  watchers(NULL),
  timers(NULL),
  counter(0)
{
  dbus_connection_ref(connection);
  dbus_connection_set_dispatch_status_function(connection, dispatchStatus, this, NULL);
  dbus_connection_set_watch_functions(connection, addWatch, removeWatch, watchToggled, this, NULL);
  dbus_connection_set_timeout_functions(connection, addTimeout, removeTimeout, timeoutToggled, this, NULL);
}

DBusCore::~DBusCore()
{
  Timer* timer = timers;
  while (timer != NULL)
  {
    void* block = timer;
    timer = timer->next;
    free(block);
  }

  Watcher* watcher = watchers;
  while (watcher != NULL)
  {
    void* block = watcher;
    watcher = watcher->next;
    free(block);
  }

  dbus_connection_set_timeout_functions(connection, NULL, NULL, NULL, NULL, NULL);
  dbus_connection_set_watch_functions(connection, NULL, NULL, NULL, NULL, NULL);
  dbus_connection_set_dispatch_status_function(connection, NULL, NULL, NULL);
  dbus_connection_unref(connection);
}

void DBusCore::applyTimeout(struct timeval* timeout)
{
  struct timeval time;
  gettimeofday(&time, NULL);

  for (Timer* timer = timers; timer != NULL; timer = timer->next)
  {
    if (timer->enabled && timercmp(&time, &timer->time, >))
    {
      struct timeval interval;
      timersub(&timer->time, &time, &interval);
      if (timercmp(&interval, timeout, <))
        *timeout = interval;
    }
  }
}

void DBusCore::applySet(fd_set* read, fd_set* write, fd_set* except)
{
  for (Watcher* watcher = watchers; watcher != NULL; watcher = watcher->next)
  {
    if ((read != NULL) && (watcher->enabled) && (watcher->flags & DBUS_WATCH_READABLE))
      FD_SET(watcher->handle, read);
    if ((write != NULL) && (watcher->enabled) && (watcher->flags & DBUS_WATCH_WRITABLE))
      FD_SET(watcher->handle, write);
    if ((except != NULL) && (watcher->enabled))
      FD_SET(watcher->handle, except);
  }
}

void DBusCore::handleChanges(fd_set* read, fd_set* write, fd_set* except)
{
  counter ++;

  struct timeval time;
  gettimeofday(&time, NULL);
  Timer* timer = timers;
  while (timer != NULL)
  {
    if ((timer->enabled) && (timer->mark != counter) && (timercmp(&time, &timer->time, >=)))
    {
      timer->mark = counter;
      dbus_timeout_handle(timer->timeout);
      // The call of handler could add or remove entries in timers.
      // So we should restart walk each time when handler called.
      timer = timers;
      continue;
    }
    timer = timer->next;
  }

  Watcher* watcher = watchers;
  while (watcher != NULL)
  {
    int condition = 0;
    if ((read != NULL) && FD_ISSET(watcher->handle, read))
      condition |= watcher->flags & DBUS_WATCH_READABLE;
    if ((write != NULL) && FD_ISSET(watcher->handle, write))
      condition |= watcher->flags & DBUS_WATCH_WRITABLE;
    if ((except != NULL) && FD_ISSET(watcher->handle, except))
      condition |= DBUS_WATCH_ERROR | DBUS_WATCH_HANGUP;

    if ((condition > 0) && (watcher->mark != counter))
    {
      watcher->mark = counter;
      dbus_watch_handle(watcher->watch, condition);
      // The call of handler could add or remove entries in watchers.
      // So we should restart walk each time when handler called.
      watcher = watchers;
      continue;
    }

    watcher = watcher->next;
  }

  if (status != DBUS_DISPATCH_COMPLETE)
    dbus_connection_dispatch(connection);
}

dbus_bool_t DBusCore::addWatch(DBusWatch* watch)
{
  Watcher* watcher = (Watcher*)malloc(sizeof(Watcher));

  if (watcher == NULL)
    return FALSE;

  watcher->watch = watch;
  watcher->handle = dbus_watch_get_unix_fd(watch);
  watcher->enabled = dbus_watch_get_enabled(watch);
  watcher->flags = dbus_watch_get_flags(watch);
  watcher->mark = counter;
  watcher->next = watchers;
  watchers = watcher;

  return TRUE;
}

void DBusCore::removeWatch(DBusWatch* watch)
{
  Watcher** pointer = &watchers;
  while (*pointer != NULL)
  {
    Watcher* watcher = *pointer;
    if (watcher->watch == watch)
    {
      *pointer = watcher->next;
      free(watcher);
      break;
    }
    pointer = &watcher->next;
  }
}

void DBusCore::watchToggled(DBusWatch* watch)
{
  Watcher* watcher = watchers;
  while (watcher != NULL)
  {
    if (watcher->watch == watch)
    {
      watcher->enabled = dbus_watch_get_enabled(watch);
      break;
    }
    watcher = watcher->next;
  }
}


dbus_bool_t DBusCore::addTimeout(DBusTimeout* timeout)
{
  Timer* timer = (Timer*)malloc(sizeof(Timer));

  if (timer == NULL)
    return FALSE;

  struct timeval time, interval;
  gettimeofday(&time, NULL);
  timerclear(&interval);
  interval.tv_usec = dbus_timeout_get_interval(timeout) * 1000;
  timeradd(&time, &interval, &timer->time);

  timer->timeout = timeout;
  timer->enabled = dbus_timeout_get_enabled(timeout);
  timer->mark = counter;
  timer->next = timers;
  timers = timer;

  return TRUE;
}

void DBusCore::removeTimeout(DBusTimeout* timeout)
{
  Timer** pointer = &timers;
  while (*pointer != NULL)
  {
    Timer* timer = *pointer;
    if (timer->timeout == timeout)
    {
      *pointer = timer->next;
      free(timer);
      break;
    }
    pointer = &timer->next;
  }
}

void DBusCore::timeoutToggled(DBusTimeout* timeout)
{
  Timer* timer = timers;
  while (timer != NULL)
  {
    if (timer->timeout == timeout)
    {
      timer->enabled = dbus_timeout_get_enabled(timeout);
      break;
    }
    timer = timer->next;
  }
}

dbus_bool_t DBusCore::addWatch(DBusWatch* watch, void* data)
{
  DBusCore* self = (DBusCore*)data;
  return self->addWatch(watch);
}

void DBusCore::removeWatch(DBusWatch* watch, void* data)
{
  DBusCore* self = (DBusCore*)data;
  self->removeWatch(watch);
}

void DBusCore::watchToggled(DBusWatch* watch, void* data)
{
  DBusCore* self = (DBusCore*)data;
  self->watchToggled(watch);
}

dbus_bool_t DBusCore::addTimeout(DBusTimeout* timeout, void* data)
{
  DBusCore* self = (DBusCore*)data;
  return self->addTimeout(timeout);
}

void DBusCore::removeTimeout(DBusTimeout* timeout, void* data)
{
  DBusCore* self = (DBusCore*)data;
  self->removeTimeout(timeout);
}

void DBusCore::timeoutToggled(DBusTimeout* timeout, void* data)
{
  DBusCore* self = (DBusCore*)data;
  self->timeoutToggled(timeout);
}

void DBusCore::dispatchStatus(DBusConnection* connection, DBusDispatchStatus status, void* data)
{
  DBusCore* self = (DBusCore*)data;
  self->status = status;
}

#endif
