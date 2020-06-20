#include "StoreFactory.h"
#include "SQLiteStore.h"
#include "MySQLStore.h"
#ifdef USE_HANDLERSOCKET
#include "HSStore.h"
#endif

#include <string.h>

#define CREATE_STORE(name, class)                     \
  if (strcmp(type, name) == 0)                        \
  {                                                   \
    class* store = new class(file);                   \
    store->onReport = (class::ReportHandler)handler;  \
    if (store->checkConnectionError())                \
    {                                                 \
      delete store;                                   \
      return NULL;                                    \
    }                                                 \
    return store;                                     \
  }


DDBClient::Store* StoreFactory::createStore(const char* type, const char* file, DDBClient::ReportHandler handler)
{
  CREATE_STORE("SQLite3", SQLiteStore);
  CREATE_STORE("MySQL", MySQLStore);
#ifdef USE_HANDLERSOCKET
  CREATE_STORE("HandlerSocket", HSStore);
#endif
  return NULL;
}
