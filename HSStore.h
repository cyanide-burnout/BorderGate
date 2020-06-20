#ifndef HSSTORE_H
#define HSSTORE_H

#include <handlersocket/hstcpcli.hpp>

#include "DDBClient.h"
#include "MySQLStore.h"

class HSStore : public MySQLStore
{
  public:

    HSStore(const char* file);
    ~HSStore();

    bool checkConnectionError();

    bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address);

    bool verifyRepeater(const char* call);
    bool verifyAddress(const struct in_addr& address);

  private:

    char* database;
    char* password;
    dena::hstcpcli_ptr client;

    bool prepareConnection();
    bool readIndexData(int index, const char* key, size_t length, char* buffer, size_t* size);
};

#endif