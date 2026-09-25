#include "ProductRepository.h"
#include <iostream>

using json = nlohmann::json;
static void print_odbc_error(SQLSMALLINT handle_type, SQLHANDLE handle)
{
    SQLCHAR sqlstate[6];
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native_error;
    SQLSMALLINT message_len;
    SQLSMALLINT i = 1;

    while (SQLGetDiagRecA(
        handle_type,
        handle,
        i,
        sqlstate,
        &native_error,
        message,
        sizeof(message),
        &message_len
    ) == SQL_SUCCESS)
    {
        std::cerr << "ODBC Error [" << sqlstate << "] " << message << std::endl;
        i++;
    }
}
SQLHDBC Sconnect_to_database()
{
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    SQLAllocHandle(
        SQL_HANDLE_ENV,
        SQL_NULL_HANDLE,
        &env
    );

    SQLSetEnvAttr(
        env,
        SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3,
        0
    );

    SQLAllocHandle(
        SQL_HANDLE_DBC,
        env,
        &dbc
    );

    SQLCHAR connection_string[] =
        "Driver={ODBC Driver 18 for SQL Server};"
        "Server=devilkirya;"
        "Database=Studydb;"
        "Trusted_Connection=yes;"
        "TrustServerCertificate=yes;";

    SQLRETURN result = SQLDriverConnectA(
        dbc,
        nullptr,
        connection_string,
        SQL_NTS,
        nullptr,
        0,
        nullptr,
        SQL_DRIVER_NOPROMPT
    );

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);

        return nullptr;
    }

    return dbc;
}
static DatabaseConnection connect_to_database()
{
    DatabaseConnection connection;

    SQLRETURN result;

    result = SQLAllocHandle(
        SQL_HANDLE_ENV,
        SQL_NULL_HANDLE,
        &connection.env
    );

    if (!SQL_SUCCEEDED(result))
        return {};

    result = SQLSetEnvAttr(
        connection.env,
        SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3,
        0
    );

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(
            SQL_HANDLE_ENV,
            connection.env
        );

        return {};
    }

    result = SQLAllocHandle(
        SQL_HANDLE_DBC,
        connection.env,
        &connection.dbc
    );

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(
            SQL_HANDLE_ENV,
            connection.env
        );

        return {};
    }

    SQLCHAR connection_string[] =
        "Driver={ODBC Driver 18 for SQL Server};"
        "Server=devilkirya;"
        "Database=Studydb;"
        "Trusted_Connection=yes;"
        "TrustServerCertificate=yes;";
        "Encrypt=no;";

    result = SQLDriverConnectA(
        connection.dbc,
        nullptr,
        connection_string,
        SQL_NTS,
        nullptr,
        0,
        nullptr,
        SQL_DRIVER_NOPROMPT
    );

    SQLHSTMT check_stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &check_stmt);

    const char* identity_query = "SELECT SUSER_SNAME(), USER_NAME(), SCHEMA_NAME()";
    SQLExecDirectA(check_stmt, (SQLCHAR*)identity_query, SQL_NTS);

    SQLCHAR login[128] = {}, user[128] = {}, schema[128] = {};
    SQLLEN login_ind = 0, user_ind = 0, schema_ind = 0;

    SQLBindCol(check_stmt, 1, SQL_C_CHAR, login, sizeof(login), &login_ind);
    SQLBindCol(check_stmt, 2, SQL_C_CHAR, user, sizeof(user), &user_ind);
    SQLBindCol(check_stmt, 3, SQL_C_CHAR, schema, sizeof(schema), &schema_ind);

    if (SQLFetch(check_stmt) == SQL_SUCCESS)
    {
        std::cerr << "APP LOGIN: [" << login << "] USER: [" << user << "] SCHEMA: [" << schema << "]" << std::endl;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, check_stmt);

    SQLCHAR server_name[128] = {};
    SQLCHAR db_name[128] = {};
    SQLSMALLINT out_len = 0;

    SQLGetInfoA(connection.dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &out_len);
    SQLGetInfoA(connection.dbc, SQL_DATABASE_NAME, db_name, sizeof(db_name), &out_len);

    std::cerr << "CONNECTED TO SERVER: [" << server_name << "] DATABASE: [" << db_name << "]" << std::endl;

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(
            SQL_HANDLE_DBC,
            connection.dbc
        );

        SQLFreeHandle(
            SQL_HANDLE_ENV,
            connection.env
        );

        return {};
    }

    return connection;
}
//std::string ProductRepository::get_products()
//{
//    //json products = json::array({
//    //    {
//    //        {"id", 1},
//    //        {"name", "Filter"},
//    //        {"price", 500}
//    //    }
//    //    });
//
//    //return products.dump();
//    DatabaseConnection connection = connect_to_database();
//
//    if (connection.dbc == nullptr)
//    {
//        return R"({"error":"Database connection failed"})";
//    }
//
//    SQLHSTMT stmt = nullptr;
//    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &stmt);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Failed to allocate statement handle"})";
//    }
//
//    // ВРЕМЕННО тестируем на реальной таблице Teachers
//    const char* query = "SELECT name, salary FROM Teachers";
//
//    std::cerr << "QUERY: [" << query << "]" << std::endl;
//
//    result = SQLExecDirectA(stmt, (SQLCHAR*)query, SQL_NTS);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_STMT, stmt);
//
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Query execution failed"})";
//    }
//
//    json products = json::array();
//
//    SQLCHAR name[256] = {};
//    SQLINTEGER salary = 0;
//
//    SQLLEN name_ind = 0, salary_ind = 0;
//
//    SQLBindCol(stmt, 1, SQL_C_CHAR, name, sizeof(name), &name_ind);
//    SQLBindCol(stmt, 2, SQL_C_LONG, &salary, 0, &salary_ind);
//
//    while (SQLFetch(stmt) == SQL_SUCCESS)
//    {
//        json item;
//        item["name"] = std::string((char*)name);
//        item["salary"] = salary;
//
//        products.push_back(item);
//    }
//
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//
//    return products.dump();
//}
//
//std::string ProductRepository::get_product(int id)
//{
//    DatabaseConnection connection = connect_to_database();
//
//    if (connection.dbc == nullptr)
//    {
//        return R"({"error":"Database connection failed"})";
//    }
//
//    SQLHSTMT stmt = nullptr;
//    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &stmt);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Failed to allocate statement handle"})";
//    }
//
//    const char* query = "SELECT name, salary FROM Teachers WHERE salary >= ?";
//
//    std::cerr << "QUERY: [" << query << "] min_salary=" << id << std::endl;
//
//    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_STMT, stmt);
//
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Failed to prepare statement"})";
//    }
//
//    SQLLEN salary_param_ind = 0;
//    SQLBindParameter(
//        stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
//        10, 2, &id, 0, &salary_param_ind
//    );
//
//    result = SQLExecute(stmt);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_STMT, stmt);
//
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Query execution failed"})";
//    }
//
//    json teachers = json::array();
//
//    SQLCHAR name[256] = {};
//    SQLINTEGER salary = 0;
//
//    SQLLEN name_ind = 0, salary_ind = 0;
//
//    SQLBindCol(stmt, 1, SQL_C_CHAR, name, sizeof(name), &name_ind);
//    SQLBindCol(stmt, 2, SQL_C_LONG, &salary, 0, &salary_ind);
//
//    while (SQLFetch(stmt) == SQL_SUCCESS)
//    {
//        json item;
//        item["name"] = std::string((char*)name);
//        item["salary"] = salary;
//
//        teachers.push_back(item);
//    }
//
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//
//    return teachers.dump();
//}

//std::string ProductRepository::add_product(const std::string& name, int salary)
//{
    //DatabaseConnection connection = connect_to_database();

    //if (connection.dbc == nullptr)
    //{
    //    return R"({"error":"Database connection failed"})";
    //}

    //SQLHSTMT stmt = nullptr;
    //SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &stmt);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
    //    return R"({"error":"Failed to allocate statement handle"})";
    //}

    //const char* query = "INSERT INTO Teachers (name, salary) VALUES (?, ?)";

    //std::cerr << "QUERY: [" << query << "] name=" << name << " salary=" << salary << std::endl;

    //result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    print_odbc_error(SQL_HANDLE_STMT, stmt);

    //    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
    //    return R"({"error":"Failed to prepare statement"})";
    //}

    //// Параметр 1: name (NVARCHAR)
    //SQLLEN name_len = SQL_NTS;
    //SQLBindParameter(
    //    stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
    //    name.size(), 0, (SQLPOINTER)name.c_str(), 0, &name_len
    //);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    print_odbc_error(SQL_HANDLE_STMT, stmt);

    //    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);

    //    return R"({"error":"Name parameter binding failed"})";
    //}

    //// Параметр 2: salary (INT)
    //SQLLEN salary_ind = 0;
    //SQLBindParameter(
    //    stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    //    0, 0, &salary, 0, &salary_ind
    //);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    print_odbc_error(SQL_HANDLE_STMT, stmt);

    //    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);

    //    return R"({"error":"Salary parameter binding failed"})";
    //}

    //result = SQLExecute(stmt);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    print_odbc_error(SQL_HANDLE_STMT, stmt);

    //    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
    //    return R"({"error":"Insert failed"})";
    //}



    //SQLLEN rows = 0;

    //SQLRowCount(stmt, &rows);

    //std::cerr << "ROWS INSERTED: " << rows << std::endl;

    //result = SQLEndTran(
    //    SQL_HANDLE_DBC,
    //    connection.dbc,
    //    SQL_COMMIT
    //);

    //SQLHSTMT check_stmt = nullptr;
    //SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &check_stmt);

    //const char* check_query = "SELECT COUNT(*) FROM Teachers WHERE name = ?";
    //SQLPrepareA(check_stmt, (SQLCHAR*)check_query, SQL_NTS);

    //SQLLEN check_name_len = SQL_NTS;
    //SQLBindParameter(check_stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
    //    name.size(), 0, (SQLPOINTER)name.c_str(), 0, &check_name_len);

    //SQLExecute(check_stmt);

    //SQLINTEGER count = 0;
    //SQLLEN count_ind = 0;
    //SQLBindCol(check_stmt, 1, SQL_C_LONG, &count, 0, &count_ind);
    //SQLFetch(check_stmt);

    //std::cerr << "VERIFY IN SAME SESSION: found " << count << " rows with name=" << name << std::endl;

    //SQLFreeHandle(SQL_HANDLE_STMT, check_stmt);

    //if (!SQL_SUCCEEDED(result))
    //{
    //    print_odbc_error(SQL_HANDLE_DBC, connection.dbc);

    //    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);

    //    return R"({"error":"Commit failed"})";
    //}

    //std::cerr << "COMMIT: OK" << std::endl;
    //SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    //SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
    //SQLFreeHandle(SQL_HANDLE_ENV, connection.env);

    //DatabaseConnection verify_connection = connect_to_database();

    //if (verify_connection.dbc != nullptr)
    //{
    //    SQLHSTMT verify_stmt = nullptr;
    //    SQLAllocHandle(SQL_HANDLE_STMT, verify_connection.dbc, &verify_stmt);

    //    const char* verify_query = "SELECT COUNT(*) FROM Teachers WHERE name = 'Filter3'";
    //    SQLExecDirectA(verify_stmt, (SQLCHAR*)verify_query, SQL_NTS);

    //    SQLINTEGER verify_count = 0;
    //    SQLLEN verify_ind = 0;
    //    SQLBindCol(verify_stmt, 1, SQL_C_LONG, &verify_count, 0, &verify_ind);
    //    SQLFetch(verify_stmt);

    //    std::cerr << "VERIFY WITH NEW CONNECTION: found " << verify_count << " rows" << std::endl;

    //    SQLFreeHandle(SQL_HANDLE_STMT, verify_stmt);
    //    SQLFreeHandle(SQL_HANDLE_DBC, verify_connection.dbc);
    //    SQLFreeHandle(SQL_HANDLE_ENV, verify_connection.env);
    //}
    //return R"({"status":"teacher added"})";
//DatabaseConnection connection = connect_to_database();
//
//if (connection.dbc == nullptr)
//{
//    return R"({"error":"Database connection failed"})";
//}
//
//SQLHSTMT stmt = nullptr;
//SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &stmt);
//
//if (!SQL_SUCCEEDED(result))
//{
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//    return R"({"error":"Failed to allocate statement handle"})";
//}
//
//const char* query = "INSERT INTO Teachers (name, salary) VALUES (?, ?)";
//
//result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);
//
//if (!SQL_SUCCEEDED(result))
//{
//    print_odbc_error(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//    return R"({"error":"Failed to prepare statement"})";
//}
//
//SQLLEN name_len = SQL_NTS;
//SQLBindParameter(
//    stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
//    name.size(), 0, (SQLPOINTER)name.c_str(), 0, &name_len
//);
//
//SQLLEN salary_ind = 0;
//SQLBindParameter(
//    stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
//    0, 0, &salary, 0, &salary_ind
//);
//
//result = SQLExecute(stmt);
//
//if (!SQL_SUCCEEDED(result))
//{
//    print_odbc_error(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//    return R"({"error":"Insert failed"})";
//}
//
//result = SQLEndTran(SQL_HANDLE_DBC, connection.dbc, SQL_COMMIT);
//
//if (!SQL_SUCCEEDED(result))
//{
//    print_odbc_error(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//    return R"({"error":"Commit failed"})";
//}
//
//SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//
//return R"({"status":"teacher added"})";
//}
//
//std::string ProductRepository::delete_product(const std::string& name)
//{
//    DatabaseConnection connection = connect_to_database();
//
//    if (connection.dbc == nullptr)
//    {
//        return R"({"error":"Database connection failed"})";
//    }
//
//    SQLHSTMT stmt = nullptr;
//    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, connection.dbc, &stmt);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Failed to allocate statement handle"})";
//    }
//
//    const char* query = "DELETE FROM Teachers WHERE name = ?";
//
//    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Failed to prepare statement"})";
//    }
//
//    SQLLEN name_len = SQL_NTS;
//    SQLBindParameter(
//        stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
//        name.size(), 0, (SQLPOINTER)name.c_str(), 0, &name_len
//    );
//
//    result = SQLExecute(stmt);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Delete failed"})";
//    }
//
//    SQLLEN rows = 0;
//    SQLRowCount(stmt, &rows);
//
//    result = SQLEndTran(SQL_HANDLE_DBC, connection.dbc, SQL_COMMIT);
//
//    if (!SQL_SUCCEEDED(result))
//    {
//        print_odbc_error(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//        SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//        SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//        return R"({"error":"Commit failed"})";
//    }
//
//    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
//    SQLFreeHandle(SQL_HANDLE_DBC, connection.dbc);
//    SQLFreeHandle(SQL_HANDLE_ENV, connection.env);
//
//    if (rows == 0)
//    {
//        return R"({"error":"Teacher not found"})";
//    }
//
//    return R"({"status":"teacher deleted"})";
//}
std::string ProductRepository::get_products()
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query = "SELECT id, type, product_name, price, stock_quantity FROM Products";

    result = SQLExecDirectA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Query execution failed"})";
    }

    json products = json::array();

    SQLINTEGER id = 0;
    SQLCHAR type[64] = {};
    SQLCHAR name[128] = {};
    SQLDOUBLE price = 0.0;
    SQLINTEGER stock = 0;

    SQLLEN id_ind = 0, type_ind = 0, name_ind = 0, price_ind = 0, stock_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_LONG, &id, 0, &id_ind);
    SQLBindCol(stmt, 2, SQL_C_CHAR, type, sizeof(type), &type_ind);
    SQLBindCol(stmt, 3, SQL_C_CHAR, name, sizeof(name), &name_ind);
    SQLBindCol(stmt, 4, SQL_C_DOUBLE, &price, 0, &price_ind);
    SQLBindCol(stmt, 5, SQL_C_LONG, &stock, 0, &stock_ind);

    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        json item;
        item["id"] = id;
        item["type"] = std::string((char*)type);
        item["name"] = std::string((char*)name);
        item["price"] = price;
        item["stock_quantity"] = stock;
        products.push_back(item);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return products.dump();
}
std::string ProductRepository::get_product(int id)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query = "SELECT id, type, product_name, price, stock_quantity FROM Products WHERE id = ?";

    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to prepare statement"})";
    }

    SQLLEN id_param_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, &id_param_ind);

    result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Query execution failed"})";
    }

    SQLINTEGER out_id = 0;
    SQLCHAR type[64] = {};
    SQLCHAR name[128] = {};
    SQLDOUBLE price = 0.0;
    SQLINTEGER stock = 0;

    SQLLEN id_ind = 0, type_ind = 0, name_ind = 0, price_ind = 0, stock_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_LONG, &out_id, 0, &id_ind);
    SQLBindCol(stmt, 2, SQL_C_CHAR, type, sizeof(type), &type_ind);
    SQLBindCol(stmt, 3, SQL_C_CHAR, name, sizeof(name), &name_ind);
    SQLBindCol(stmt, 4, SQL_C_DOUBLE, &price, 0, &price_ind);
    SQLBindCol(stmt, 5, SQL_C_LONG, &stock, 0, &stock_ind);

    json product;

    if (SQLFetch(stmt) == SQL_SUCCESS)
    {
        product["id"] = out_id;
        product["type"] = std::string((char*)type);
        product["name"] = std::string((char*)name);
        product["price"] = price;
        product["stock_quantity"] = stock;
    }
    else
    {
        product = { {"error", "Product not found"} };
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return product.dump();
}

std::string ProductRepository::add_product(const std::string& type, const std::string& name, double cost, double price, int stock)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query = "INSERT INTO Products (type, product_name, cost, price, stock_quantity) VALUES (?, ?, ?, ?, ?)";

    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to prepare statement"})";
    }

    SQLLEN type_len = SQL_NTS;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        type.size(), 0, (SQLPOINTER)type.c_str(), 0, &type_len);

    SQLLEN name_len = SQL_NTS;
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        name.size(), 0, (SQLPOINTER)name.c_str(), 0, &name_len);

    SQLLEN cost_ind = 0;
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 2, &cost, 0, &cost_ind);

    SQLLEN price_ind = 0;
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 2, &price, 0, &price_ind);

    SQLLEN stock_ind = 0;
    SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &stock, 0, &stock_ind);

    result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Insert failed"})";
    }

    result = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Commit failed"})";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return R"({"status":"product added"})";
}


std::string ProductRepository::delete_product(int id)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query = "DELETE FROM Products WHERE id = ?";

    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to prepare statement"})";
    }

    SQLLEN id_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, &id_ind);

    result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);

        return R"({"error":"Delete failed (product may be referenced in existing orders)";
    }


    SQLLEN rows = 0;
    SQLRowCount(stmt, &rows);

    result = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);

    if (!SQL_SUCCEEDED(result))
    {
      print_odbc_error(SQL_HANDLE_DBC, dbc);
      SQLFreeHandle(SQL_HANDLE_STMT, stmt);
      return R"({"error":"Commit failed"})";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    if (rows == 0)
    {
      return R"({"error":"Product not found"})";
    }

    return R"({"status":"product deleted"})";
 }

std::string ProductRepository::update_product(int id, double price, int stock)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLRETURN result = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    if (!SQL_SUCCEEDED(result))
    {
        return R"({"error":"Failed to allocate statement handle"})";
    }

    const char* query = "UPDATE Products SET price = ?, stock_quantity = ? WHERE id = ?";

    result = SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to prepare statement"})";
    }

    SQLLEN price_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 2, &price, 0, &price_ind);

    SQLLEN stock_ind = 0;
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &stock, 0, &stock_ind);

    SQLLEN id_ind = 0;
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, &id_ind);

    result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Update failed"})";
    }

    SQLLEN rows = 0;
    SQLRowCount(stmt, &rows);

    result = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Commit failed"})";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    if (rows == 0)
    {
        return R"({"error":"Product not found"})";
    }

    return R"({"status":"product updated"})";
}
