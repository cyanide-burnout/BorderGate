#ifndef SQLITESTORE_H
#define SQLITESTORE_H

#include <stdarg.h>
#include <sqlite3.h>
#include <netinet/in.h>

#include "DDBClient.h"

#define PREPARED_STATEMENT_COUNT  11

class SQLiteStore : public DDBClient::Store
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);

    SQLiteStore(const char* file);
    ~SQLiteStore();

    bool checkConnectionError();

    bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address);

    bool verifyRepeater(const char* call);
    bool verifyAddress(const struct in_addr& address);

    void resetUserState();
    void storeUserServer(const char* nick, const char* server);
    void storeUser(const char* nick, const char* name, const char* address);
    void removeUser(const char* nick);

    char* findActiveBot(const char* server);

    void getLastModifiedDate(int table, char* date);
    size_t getCountByDate(int table, const char* date);

    void storeTableData(int table, const char* date, const char* call1, const char* call2);

    ReportHandler onReport;

  private:

    sqlite3* connection;
    sqlite3_stmt* statements[PREPARED_STATEMENT_COUNT];

    void report(int priority, const char* format, ...);
};

#endif
