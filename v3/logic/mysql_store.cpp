#include "mysql_store.h"

#include <cstdlib>
#include <mysql.h>
#include <sstream>

namespace
{
    const size_t kMaxUsernameLength = 32;
    const size_t kMaxPasswordLength = 64;
}

serverlogic::MysqlStore::MysqlStore(const std::string &host,
                                    unsigned int port,
                                    const std::string &database,
                                    const std::string &user,
                                    const std::string &password_env,
                                    const std::string &table_name)
    : _host(ReadEnvString("BROKEN_SERVER_DB_HOST", host)),
      _port(ReadEnvPort("BROKEN_SERVER_DB_PORT", port)),
      _database(ReadEnvString("BROKEN_SERVER_DB_NAME", database)),
      _user(ReadEnvString("BROKEN_SERVER_DB_USER", user)),
      _password_env(password_env),
      _table_name(ReadEnvString("BROKEN_SERVER_DB_TABLE", table_name)),
      _mutex()
{
}

std::string serverlogic::MysqlStore::ReadEnvString(const char *key, const std::string &fallback) const
{
    const char *value = std::getenv(key);
    if (value == nullptr || value[0] == '\0')
    {
        return fallback;
    }
    return value;
}

unsigned int serverlogic::MysqlStore::ReadEnvPort(const char *key, unsigned int fallback) const
{
    const char *value = std::getenv(key);
    if (value == nullptr || value[0] == '\0')
    {
        return fallback;
    }

    std::istringstream input(value);
    unsigned int port = fallback;
    input >> port;
    if (!input.fail() && port > 0)
    {
        return port;
    }
    return fallback;
}

bool serverlogic::MysqlStore::Init()
{
    MutexlockGuard lock(_mutex);

    MYSQL *server_conn = nullptr;
    if (!OpenServerConnection(server_conn))
    {
        return false;
    }

    std::string create_db_sql =
        "CREATE DATABASE IF NOT EXISTS `" + _database +
        "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
    bool ok = ExecuteSql(server_conn, create_db_sql);
    mysql_close(server_conn);
    if (!ok)
    {
        return false;
    }

    MYSQL *db_conn = nullptr;
    if (!OpenDatabaseConnection(db_conn))
    {
        return false;
    }

    std::string create_table_sql =
        "CREATE TABLE IF NOT EXISTS `" + _table_name + "` ("
        "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "`username` VARCHAR(32) NOT NULL,"
        "`password_hash` CHAR(64) NOT NULL,"
        "`created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "`last_login_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (`id`),"
        "UNIQUE KEY `uniq_username` (`username`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    ok = ExecuteSql(db_conn, create_table_sql);
    mysql_close(db_conn);
    return ok;
}

serverlogic::LoginCheckResult serverlogic::MysqlStore::LoginOrCreate(const std::string &username,
                                                                     const std::string &password) const
{
    if (username.empty() || password.empty())
    {
        return LoginCheckResult(LoginCheckResult::InvalidInput, "Username and password are required.");
    }
    if (username.size() > kMaxUsernameLength || password.size() > kMaxPasswordLength)
    {
        return LoginCheckResult(LoginCheckResult::InvalidInput, "Input is too long.");
    }

    MutexlockGuard lock(_mutex);

    MYSQL *conn = nullptr;
    if (!OpenDatabaseConnection(conn))
    {
        return LoginCheckResult(LoginCheckResult::DatabaseError, "MySQL connection failed.");
    }

    const std::string escaped_username = EscapeString(conn, username);
    const std::string escaped_password = EscapeString(conn, password);

    bool has_row = false;
    std::string success_query =
        "SELECT 1 FROM `" + _table_name + "` WHERE `username`='" + escaped_username +
        "' AND `password_hash` = SHA2('" + escaped_password + "', 256) LIMIT 1";
    if (!QueryHasRow(conn, success_query, has_row))
    {
        const std::string error_message = mysql_error(conn);
        mysql_close(conn);
        return LoginCheckResult(LoginCheckResult::DatabaseError, error_message);
    }

    if (has_row)
    {
        std::string update_sql =
            "UPDATE `" + _table_name + "` SET `last_login_at` = CURRENT_TIMESTAMP "
            "WHERE `username`='" + escaped_username + "'";
        ExecuteSql(conn, update_sql);
        mysql_close(conn);
        return LoginCheckResult(LoginCheckResult::Success, "Login success.");
    }

    std::string user_exists_query =
        "SELECT 1 FROM `" + _table_name + "` WHERE `username`='" + escaped_username + "' LIMIT 1";
    if (!QueryHasRow(conn, user_exists_query, has_row))
    {
        const std::string error_message = mysql_error(conn);
        mysql_close(conn);
        return LoginCheckResult(LoginCheckResult::DatabaseError, error_message);
    }

    if (has_row)
    {
        mysql_close(conn);
        return LoginCheckResult(LoginCheckResult::WrongPassword, "Password is incorrect.");
    }

    std::string insert_sql =
        "INSERT INTO `" + _table_name + "` (`username`, `password_hash`) VALUES ('" +
        escaped_username + "', SHA2('" + escaped_password + "', 256))";
    if (!ExecuteSql(conn, insert_sql))
    {
        const std::string error_message = mysql_error(conn);
        mysql_close(conn);
        return LoginCheckResult(LoginCheckResult::DatabaseError, error_message);
    }

    mysql_close(conn);
    return LoginCheckResult(LoginCheckResult::Created, "Account created and login success.");
}

bool serverlogic::MysqlStore::OpenServerConnection(MYSQL *&conn) const
{
    conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        return false;
    }

    const std::string password = ReadPassword();
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    if (mysql_real_connect(conn,
                           _host.c_str(),
                           _user.c_str(),
                           password.c_str(),
                           nullptr,
                           _port,
                           nullptr,
                           0) == nullptr)
    {
        mysql_close(conn);
        conn = nullptr;
        return false;
    }
    return true;
}

bool serverlogic::MysqlStore::OpenDatabaseConnection(MYSQL *&conn) const
{
    conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        return false;
    }

    const std::string password = ReadPassword();
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    if (mysql_real_connect(conn,
                           _host.c_str(),
                           _user.c_str(),
                           password.c_str(),
                           _database.c_str(),
                           _port,
                           nullptr,
                           0) == nullptr)
    {
        mysql_close(conn);
        conn = nullptr;
        return false;
    }
    return true;
}

bool serverlogic::MysqlStore::ExecuteSql(MYSQL *conn, const std::string &sql) const
{
    return mysql_query(conn, sql.c_str()) == 0;
}

bool serverlogic::MysqlStore::QueryHasRow(MYSQL *conn, const std::string &sql, bool &has_row) const
{
    has_row = false;
    if (mysql_query(conn, sql.c_str()) != 0)
    {
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == nullptr)
    {
        return false;
    }

    has_row = mysql_num_rows(result) > 0;
    mysql_free_result(result);
    return true;
}

std::string serverlogic::MysqlStore::EscapeString(MYSQL *conn, const std::string &value) const
{
    std::string escaped;
    escaped.resize(value.size() * 2 + 1);
    unsigned long escaped_size = mysql_real_escape_string(conn, &escaped[0], value.c_str(), value.size());
    escaped.resize(static_cast<size_t>(escaped_size));
    return escaped;
}

std::string serverlogic::MysqlStore::ReadPassword() const
{
    const char *password = std::getenv(_password_env.c_str());
    if (password != nullptr)
    {
        return password;
    }

    const char *mysql_pwd = std::getenv("MYSQL_PWD");
    if (mysql_pwd != nullptr)
    {
        return mysql_pwd;
    }

    return "";
}
