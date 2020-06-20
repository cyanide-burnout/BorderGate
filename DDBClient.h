#ifndef DDBCLIENT_H
#define DDBCLIENT_H

#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <netinet/in.h>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

// #include <libircclient/libircclient.h>

#include "DStar.h"

#define DDB_DEFAULT_PORT    9007

#define DDB_TABLE_0         0
#define DDB_TABLE_1         1
#define DDB_TABLE_2         2
#define DDB_TABLE_COUNT     3

#define DDB_DATE_LENGTH     19

#define DDB_TOUCH_INTERVAL      300
#define DDB_RECONNECT_INTERVAL  21600

class DDBClient
{
  public:

    typedef void (*ReportHandler)(int priority, const char* format, va_list arguments);
    typedef void (*UpdateCommandHandler)(DDBClient* client, int table, const char* date, const char* call1, const char* call2);

    class Store
    {
      public:

        virtual bool findRoute(const char* call, struct DStarRoute* route, struct in_addr* address) = 0;

        virtual bool verifyRepeater(const char* call) = 0;
        virtual bool verifyAddress(const struct in_addr& address) = 0;

        virtual void resetUserState() = 0;
        virtual void storeUserServer(const char* nick, const char* server) = 0;
        virtual void storeUser(const char* nick, const char* name, const char* address) = 0;
        virtual void removeUser(const char* nick) = 0;

        virtual char* findActiveBot(const char* server) = 0;

        virtual void getLastModifiedDate(int table, char* date) = 0;
        virtual size_t getCountByDate(int table, const char* date) = 0;

        virtual void storeTableData(int table, const char* date, const char* call1, const char* call2) = 0;
    };

    DDBClient(Store* store,
      const char* server, int port,
      const char* name, const char* password,
      const char* version);
    ~DDBClient();

    void setUpdateCommandHandler(UpdateCommandHandler handler, void* data);

    void connect();
    void disconnect();

    void publishHeard(const struct DStarRoute& route, const char* addressee, const char* text);
    void publishHeard(const struct DStarRoute& route, uint16_t number);
    void touch(time_t now);

    inline Store* getStore()            { return store; };
    inline irc_session_t* getSession()  { return session; };

    inline bool isConnected()           { return irc_is_connected(session); };
    inline bool isReadOnly()            { return strchr(name, '-'); };

    ReportHandler onReport;
    UpdateCommandHandler onUpdateCommand;

    void* userData;

  private:

    Store* store;

    char* server;
    char* alias;
    char* name;
    char* password;
    char* version;
    int attempt;
    int number;
    int port;

    pthread_mutex_t lock;
    irc_session_t* session;
    irc_callbacks_t handlers;

    char* volatile bot;
    int state;

    void report(int priority, const char* format, ...);
    time_t parseData(const char* value);

    char* generateNick();
    void sendCommand(const char* command);
    void setConnectedServer(const char* server);

    void storeUserServer(const char* nick, const char* server);
    void storeUser(const char* nick, const char* name, const char* address);
    void removeUser(const char* nick);

    void requestForUpdate();
    void preventDataLoop(int table, char* date);
    void storeTableData(int table, const char* date, const char* call1, const char* call2);

    static void handleConnect(irc_session_t* session, const char* event, const char* origin, const char** parameters, unsigned int count);
    static void handleJoin(irc_session_t* session, const char* event, const char* origin, const char** parameters, unsigned int count);
    static void handleQuit(irc_session_t* session, const char* event, const char* origin, const char** parameters, unsigned int count);
    static void handlePrivateMessage(irc_session_t* session, const char* event, const char* origin, const char** parameters, unsigned int count);
    static void handleEventCode(irc_session_t* session, unsigned int event, const char* origin, const char** parameters, unsigned int count);
};

#endif
