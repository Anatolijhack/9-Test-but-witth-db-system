#include "UserRepository.h"



std::optional<UserRecord> UserRepository::find_by_username(const std::string& username)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* query =
        "SELECT login, hash_password, role "
        "FROM Users "
        "WHERE login = ?";
    SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    SQLLEN name_len = SQL_NTS;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        username.size(), 0, (SQLPOINTER)username.c_str(), 0, &name_len);

    SQLExecute(stmt);

    SQLCHAR out_username[64] = {}, out_hash[256] = {}, out_role[32] = {};
    SQLLEN u_ind = 0, p_ind = 0, r_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_CHAR, out_username, sizeof(out_username), &u_ind);
    SQLBindCol(stmt, 2, SQL_C_CHAR, out_hash, sizeof(out_hash), &p_ind);
    SQLBindCol(stmt, 3, SQL_C_CHAR, out_role, sizeof(out_role), &r_ind);

    std::optional<UserRecord> result;

    if (SQLFetch(stmt) == SQL_SUCCESS)
    {
        result = UserRecord{
            std::string((char*)out_username),
            std::string((char*)out_hash),   
            std::string((char*)out_role)
        };
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

std::string UserRepository::create_user(const std::string& username, const std::string& password_hash, const std::string& role)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query =
        "INSERT INTO Users (login, hash_password, role) "
        "VALUES (?, ?, ?)";

    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to prepare statement"})";
    }

    SQLLEN username_len = SQL_NTS;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        username.size(), 0, (SQLPOINTER)username.c_str(), 0, &username_len);

    SQLLEN hash_len = SQL_NTS;
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        password_hash.size(), 0, (SQLPOINTER)password_hash.c_str(), 0, &hash_len);

    SQLLEN role_len = SQL_NTS;
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        role.size(), 0, (SQLPOINTER)role.c_str(), 0, &role_len);

    result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        SQLCHAR sqlstate[6];
        SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER native_error;
        SQLSMALLINT message_len;

        SQLGetDiagRecA(SQL_HANDLE_STMT, stmt, 1, sqlstate, &native_error, message, sizeof(message), &message_len);

        std::string sqlstate_str((char*)sqlstate);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);

        if (sqlstate_str == "23000") // нарушение уникальности
        {
            return R"({"error":"Username already exists"})";
        }

        return R"({"error":"Failed to create user"})";
    }

    result = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Commit failed"})";
    }

    return R"({"status":"user created"})";
}
