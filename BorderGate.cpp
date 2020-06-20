#include <sys/select.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef USE_LIBBSD_02
#include <libutil.h>
#else
#include <bsd/libutil.h>
#endif

#include <dbus/dbus.h>
#include <libconfig.h>

#include "G2Link.h"
#include "CCSLink.h"
#include "DCSLink.h"
#include "DExtraLink.h"

#include "CCSModule.h"
#include "DDBModule.h"
#include "RP2CModule.h"
#include "ZoneModule.h"
#include "MasterModule.h"

#include "GateRouter.h"
#include "GateManager.h"

#include "DBusCore.h"
#include "GateService.h"

#include "ReportTools.h"
#include "StringTools.h"


#ifdef LIBCONFIG_VER_MAJOR
typedef int IntegerSetting;
#else
typedef long int IntegerSetting;
#endif

#define VERSION              "BorderGate:20150618"

#define OPTION_DAEMON_MODE   1
#define SECTION_NAME_LENGTH  8

#define POLL_INTERVAL        50000
#define SLEEP_INTERVAL       1500000

#define READ_SET             0
#define WRITE_SET            1
#define EXCEPT_SET           2

#define SET_COUNT            3
#define TRACE_DEPTH          10

#define COUNT(array)         (sizeof(array) / sizeof(array[0]))

volatile bool running = false;

void handleSignal(int signal)
{
  running = false;
}

void handleFault(int signal)
{
  void* buffer[TRACE_DEPTH];
  size_t count = backtrace(buffer, TRACE_DEPTH);
  char** symbols = backtrace_symbols(buffer, count);

  report(LOG_CRIT, "Segmentation fault, %d addresses\n", count);
  if (symbols != NULL)
    for (size_t index = 0; index < count; index ++)
      report(LOG_CRIT, "  at: %s\n", symbols[index]);

  exit(EXIT_FAILURE);
}

void handleReport(int priority, const char* format, va_list arguments)
{
  doReport(priority, format, arguments);
  if (priority < LOG_ERR)
    running = false;
}

char** createStringList(config_setting_t* setting)
{
  size_t length = config_setting_length(setting);
  size_t size = (length + 1) * sizeof(char*);
  char** list = (char**)malloc(size);

  for (size_t index = 0; index < length; index ++)
  {
    const char* value = config_setting_get_string_elem(setting, index);
    list[index] = strdup(value);
  }

  list[length] = NULL;
  return list;
}

void* routine(void* arguments)
{
  GateService* service = (GateService*)arguments;
  GateManager* manager = service->getManager();

  fd_set sets[SET_COUNT];
  timeval timeout;

  DBusConnection* connection = service->getConnection();
  DBusCore core(connection);

  while (running)
  {
    for (size_t index = 0; index < SET_COUNT; index ++)
      FD_ZERO(&sets[index]);

    manager->addDescriptors(ROUTINE_ACTIVITY, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);
    core.applySet(&sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);

    timeout.tv_sec = 0;
    timeout.tv_usec = POLL_INTERVAL;

    core.applyTimeout(&timeout);

    int count = select(FD_SETSIZE, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET], &timeout);

    if (count == EOF)
      break;

    core.handleChanges(&sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);
    manager->processDescriptors(ROUTINE_ACTIVITY, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);
    manager->doBackgroundActivity(ROUTINE_ACTIVITY);

    if (manager->hasLimitExceeded())
    {
      report(LOG_NOTICE, "Limit of registrations number exceeded (%d tps)\n", manager->getOperationCount());
      usleep(SLEEP_INTERVAL);
    }
  }

  pthread_exit(NULL);
}

int main(int argc, const char* argv[])
{
  sigset(SIGSEGV, handleFault);

  printf("\n");
  printf("BorderGate D-STAR call routing proxy\n");
  printf("Copyright 2012-2017 Artem Prilutskiy (R3ABM, cyanide.burnout@gmail.com)\n");
  printf("\n");

  struct option options[] =
  {
    { "config", required_argument, NULL, 'c' },
    { "pidfile", required_argument, NULL, 'p' },
    { "daemon", no_argument, NULL, 'd' },
    { NULL, 0, NULL, 0 }
  };

  char* file = NULL;
  char* identity = NULL;
  int option = 0;

  int selection = 0;
  while ((selection = getopt_long(argc, const_cast<char* const*>(argv), "c:p:d", options, NULL)) != EOF)
    switch (selection)
    {
      case 'c':
        file = optarg;
        break;

      case 'p':
        identity = optarg;
        break;

      case 'd':
        option |= OPTION_DAEMON_MODE;
        break;
    }

  if (file == NULL)
  {
    printf("Usage: %s --config <path to configuration file> [--pidfile <path to PID file>] [--daemon]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (option & OPTION_DAEMON_MODE)
  {
    setReportOutput(LOGGER_OUTPUT_SYSLOG);
    if (daemon(-1, -1) < 0)
    {
      report(LOG_ERR, "Error launching daemon");
      return EXIT_FAILURE;
    }
  }

  config_t configuration;
  config_init(&configuration);

  if (config_read_file(&configuration, file) == CONFIG_FALSE)
  {
    report(LOG_ERR, "Configuration error at %s:%d: %s\n",
      file,
      config_error_line(&configuration),
      config_error_text(&configuration));
    config_destroy(&configuration);
    return EXIT_FAILURE;
  }

  const char* call = NULL;
  const char* address = "*";
  IntegerSetting capability = 0;
  config_setting_t* setting = config_root_setting(&configuration);
  config_setting_lookup_string(setting, "call", &call);
  config_setting_lookup_string(setting, "address", &address);
  config_setting_lookup_int(setting, "capability", &capability);

  if (call == NULL)
  {
    report(LOG_ERR, "Configuration file is incorrect\n");
    config_destroy(&configuration);
    return EXIT_FAILURE;
  }

  char** outcasts = NULL;
  setting = config_lookup(&configuration, "outcasts");
  if (setting != NULL)
  {
    if (config_setting_is_array(setting) == CONFIG_FALSE)
    {
      report(LOG_ERR, "Incorrect syntax of outcasts list\n");
      config_destroy(&configuration);
      return EXIT_FAILURE;
    }
    outcasts = createStringList(setting);
  }

  char** announces = NULL;
  setting = config_lookup(&configuration, "announces");
  if (setting != NULL)
  {
    if (config_setting_is_array(setting) == CONFIG_FALSE)
    {
      report(LOG_ERR, "Incorrect syntax of announces list\n");
      config_destroy(&configuration);
      return EXIT_FAILURE;
    }
    announces = createStringList(setting);
  }

  GateModule* modules = NULL;

  GateManager manager(call, &modules, (GateManager::Capability)capability, announces, outcasts);
  manager.onReport = (GateManager::ReportHandler)handleReport;

  GateRouter router(address, call, &modules);
  router.onReport = (GateRouter::ReportHandler)handleReport;

  G2Link link1(address, &router);
  CCSLink link2(call, address, VERSION, &modules, &router);
  DCSLink link3(call, address, VERSION, &modules, &router);
  DExtraLink link4(call, address, &modules, &router);

  GateLink* links[] =
  {
    &link1,
    &link2,
    &link3,
    &link4
  };

  for (size_t index = 0; index < COUNT(links); index ++)
    if (links[index]->getHandle() < 0)
    {
      report(LOG_ERR, "Error binding socket\n");
      config_destroy(&configuration);
      return EXIT_FAILURE;
    }

  DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  
  if (connection == NULL)
  {
    report(LOG_ERR, "D-Bus not initialized\n");
    config_destroy(&configuration);
    return EXIT_FAILURE;
  }

  GateService service(connection, &manager);

  if (service.getError() != NULL)
  {
    report(LOG_ERR, "Error allocating D-Bus: %s\n", service.getError());
    dbus_connection_unref(connection);
    config_destroy(&configuration);
    return EXIT_FAILURE;
  }

  struct pidfh* handle = NULL;
  if (identity != NULL)
  {
    handle = pidfile_open(identity, 0644, NULL);
    if (handle == NULL)
    {
      report(LOG_ERR, "Can not create PID file");
      dbus_connection_unref(connection);
      config_destroy(&configuration);
      return EXIT_FAILURE;
    }
    pidfile_write(handle);
  }

  for (char letter = '0'; letter <= '5'; letter ++)
  {
    char section[SECTION_NAME_LENGTH];
    sprintf(section, "Z%c", letter);
    setting = config_lookup(&configuration, section);
    if (setting != NULL)
    {
      const char* type = NULL;
      GateLink* link = NULL;
      config_setting_lookup_string(setting, "type", &type);

      if (strcmp(type, "DCS") == 0)
        link = &link3;
      if (strcmp(type, "DExtra") == 0)
        link = &link4;

      GateModule* module = new ZoneModule(setting, link, modules);

      if (!module->isActive())
      {
        report(LOG_ERR, "Zone module %c failed to initialize\n", letter);
        delete module;
        continue;
      }

      modules = module;
    }
  }

  for (int letter = 'A'; letter <= 'E'; letter ++)
  {
    char section[SECTION_NAME_LENGTH];
    sprintf(section, "%c", letter);
    setting = config_lookup(&configuration, section);
    if (setting != NULL)
    {
      const char* type = NULL;
      GateModule* module = NULL;
      config_setting_lookup_string(setting, "type", &type);

      if (strcmp(type, "CCS") == 0)
        module = new CCSModule(setting, &link2, &manager, handleReport, modules);
      if (strcmp(type, "ircDDB") == 0)
        module = new DDBModule(setting, VERSION, &link1, &manager, handleReport, modules);
      if (strcmp(type, "Trust Server") == 0)
        module = new RP2CModule(setting, &link1, &manager, handleReport, modules);
      if (strcmp(type, "BrandMeister") == 0)
        module = new MasterModule(setting, &link1, &manager, handleReport, modules);

      if (module == NULL)
      {
        report(LOG_ERR, "Module %c has invalid type\n", letter);
        continue;
      }
      if (!module->isActive())
      {
        report(LOG_ERR, "Module %c failed to initialize\n", letter);
        delete module;
        continue;
      }
      modules = module;
    }
  }

  config_destroy(&configuration);

  running = true;

  pthread_t thread;
  pthread_create(&thread, NULL, routine, &service);

  sigset(SIGHUP, handleSignal);
  sigset(SIGINT, handleSignal);
  sigset(SIGTERM, handleSignal);

  fd_set sets[SET_COUNT];
  timeval timeout;

  report(LOG_INFO, "Gate started\n");

  while (running)
  {
    for (size_t index = 0; index < COUNT(sets); index ++)
      FD_ZERO(&sets[index]);

    for (size_t index = 0; index < COUNT(links); index ++)
      FD_SET(links[index]->getHandle(), &sets[READ_SET]);

    manager.addDescriptors(REAL_TIME_ACTIVITY, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);

    timeout.tv_sec = 0;
    timeout.tv_usec = POLL_INTERVAL;

    int count = select(FD_SETSIZE, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET], &timeout);

    if (count == EOF)
      break;

    for (size_t index = 0; index < COUNT(links); index ++)
      if (FD_ISSET(links[index]->getHandle(), &sets[READ_SET]))
        links[index]->receiveData();

    manager.processDescriptors(REAL_TIME_ACTIVITY, &sets[READ_SET], &sets[WRITE_SET], &sets[EXCEPT_SET]);

    manager.doBackgroundActivity(REAL_TIME_ACTIVITY);
    router.doBackgroundActivity();

    for (size_t index = 0; index < COUNT(links); index ++)
      links[index]->doBackgroundActivity();
  }

  report(LOG_INFO, "Gate stopped\n");

  running = false;
  pthread_join(thread, NULL);

  if (handle != NULL)
    pidfile_remove(handle);

  for (size_t index = 0; index < COUNT(links); index ++)
    links[index]->disconnectAll();

  manager.removeAllModules();
  release(announces);
  release(outcasts);

  dbus_connection_unref(connection);

  return EXIT_SUCCESS;
}
