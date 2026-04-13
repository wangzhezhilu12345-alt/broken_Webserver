#pragma once

#include "../broken_include/Mutexlock.h"

#include <string>

struct MYSQL;

namespace serverlogic
{
    struct LoginCheckResult
    {
        enum Status
        {
            Success,
            Created,
            WrongPassword,
            InvalidInput,
            DatabaseError
        };

        LoginCheckResult(Status init_status = DatabaseError, const std::string &init_message = "")
            : status(init_status), message(init_message)
        {
        }

        Status status;
        std::string message;
    };

    class MysqlStore
    {
    public:
        MysqlStore(const std::string &host = "localhost",
                   unsigned int port = 3306,
                   const std::string &database = "storiesdb",
                   const std::string &user = "root",
                   const std::string &password_env = "BROKEN_SERVER_DB_PASSWORD",
                   const std::string &table_name = "game_accounts");

        bool Init();
        LoginCheckResult LoginOrCreate(const std::string &username, const std::string &password) const;

    private:
        std::string ReadEnvString(const char *key, const std::string &fallback) const;
        unsigned int ReadEnvPort(const char *key, unsigned int fallback) const;
        bool OpenServerConnection(MYSQL *&conn) const;
        bool OpenDatabaseConnection(MYSQL *&conn) const;
        bool ExecuteSql(MYSQL *conn, const std::string &sql) const;
        bool QueryHasRow(MYSQL *conn, const std::string &sql, bool &has_row) const;
        std::string EscapeString(MYSQL *conn, const std::string &value) const;
        std::string ReadPassword() const;

    private:
        std::string _host;
        unsigned int _port;
        std::string _database;
        std::string _user;
        std::string _password_env;
        std::string _table_name;
        mutable Mutexlock _mutex;
    };
}
