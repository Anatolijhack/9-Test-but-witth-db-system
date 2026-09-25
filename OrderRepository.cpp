#include "OrderRepository.h"
#include <iostream>

static void print_odbc_error(SQLSMALLINT handle_type, SQLHANDLE handle)
{
    SQLCHAR sqlstate[6];
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native_error;
    SQLSMALLINT message_len;
    SQLSMALLINT i = 1;

    while (SQLGetDiagRecA(handle_type, handle, i, sqlstate, &native_error,
        message, sizeof(message), &message_len) == SQL_SUCCESS)
    {
        std::cerr << "ODBC Error [" << sqlstate << "] " << message << std::endl;
        i++;
    }
}

// Получить актуальную цену товара и остаток напрямую из БД (никогда не доверяем цене от клиента!)
struct ProductPriceInfo
{
    double price;
    std::string name;
    int stock;
};

static std::optional<ProductPriceInfo> get_product_price(SQLHDBC dbc, int product_id)
{
    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* query = "SELECT product_name, price, stock_quantity FROM Products WHERE id = ?";
    SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    SQLLEN id_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &product_id, 0, &id_ind);

    SQLExecute(stmt);

    SQLCHAR name[128] = {};
    SQLDOUBLE price = 0.0;
    SQLINTEGER stock = 0;
    SQLLEN name_ind = 0, price_ind = 0, stock_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_CHAR, name, sizeof(name), &name_ind);
    SQLBindCol(stmt, 2, SQL_C_DOUBLE, &price, 0, &price_ind);
    SQLBindCol(stmt, 3, SQL_C_LONG, &stock, 0, &stock_ind);

    std::optional<ProductPriceInfo> result;

    if (SQLFetch(stmt) == SQL_SUCCESS)
    {
        result = ProductPriceInfo{ price, std::string((char*)name), (int)stock };
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

std::string OrderRepository::create_order(int user_id, const std::vector<OrderItemInput>& items)
{
    if (items.empty())
    {
        return R"({"error":"Order must contain at least one item"})";
    }

    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    // 1. Проверяем наличие товаров и остатков ДО создания заказа
    struct ResolvedItem { int product_id; int quantity; double price; };
    std::vector<ResolvedItem> resolved;

    for (const auto& item : items)
    {
        if (item.quantity <= 0)
        {
            return R"({"error":"Quantity must be positive"})";
        }

        auto info = get_product_price(dbc, item.product_id);

        if (!info)
        {
            json err = { {"error", "Product not found"}, {"product_id", item.product_id} };
            return err.dump();
        }

        if (info->stock < item.quantity)
        {
            json err = {
                {"error", "Insufficient stock"},
                {"product_id", item.product_id},
                {"available", info->stock}
            };
            return err.dump();
        }

        resolved.push_back({ item.product_id, item.quantity, info->price });
    }

    // 2. Создаём запись в Orders
    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* insert_order = "INSERT INTO Orders (user_id, status) VALUES (?, 'New')";
    SQLPrepareA(stmt, (SQLCHAR*)insert_order, SQL_NTS);

    SQLLEN user_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &user_id, 0, &user_ind);

    SQLRETURN result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Failed to create order"})";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    // 3. Получаем сгенерированный order_id
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    SQLExecDirectA(stmt, (SQLCHAR*)"SELECT SCOPE_IDENTITY()", SQL_NTS);

    SQLDOUBLE order_id_raw = 0.0;
    SQLLEN oid_ind = 0;
    SQLBindCol(stmt, 1, SQL_C_DOUBLE, &order_id_raw, 0, &oid_ind);
    SQLFetch(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    int order_id = static_cast<int>(order_id_raw);

    // 4. Вставляем позиции заказа и списываем остаток
    double total = 0.0;

    for (const auto& item : resolved)
    {
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        const char* insert_item = "INSERT INTO OrderItems (order_id, product_id, quantity, price) VALUES (?, ?, ?, ?)";
        SQLPrepareA(stmt, (SQLCHAR*)insert_item, SQL_NTS);

        SQLLEN oid_ind2 = 0, pid_ind = 0, qty_ind = 0, price_ind = 0;

        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &order_id, 0, &oid_ind2);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (void*)&item.product_id, 0, &pid_ind);
        SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (void*)&item.quantity, 0, &qty_ind);
        SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 2, (void*)&item.price, 0, &price_ind);

        SQLRETURN item_result = SQLExecute(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);

        if (!SQL_SUCCEEDED(item_result))
        {
            SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_ROLLBACK);
            return R"({"error":"Failed to add order item"})";
        }

        // Списываем остаток
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        const char* update_stock = "UPDATE Products SET stock_quantity = stock_quantity - ? WHERE id = ?";
        SQLPrepareA(stmt, (SQLCHAR*)update_stock, SQL_NTS);

        SQLLEN qty_ind2 = 0, pid_ind2 = 0;
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (void*)&item.quantity, 0, &qty_ind2);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (void*)&item.product_id, 0, &pid_ind2);

        SQLExecute(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);

        total += item.price * item.quantity;
    }

    SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);

    json response = {
        {"order_id", order_id},
        {"status", "New"},
        {"total", total}
    };

    return response.dump();
}

std::string OrderRepository::get_order(int order_id)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* query = "SELECT order_id, user_id, status, created_at FROM Orders WHERE order_id = ?";
    SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    SQLLEN id_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &order_id, 0, &id_ind);
    SQLExecute(stmt);

    SQLINTEGER out_order_id = 0, out_user_id = 0;
    SQLCHAR status[32] = {};
    SQL_TIMESTAMP_STRUCT created_at{};
    SQLLEN oid_ind = 0, uid_ind = 0, status_ind = 0, created_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_LONG, &out_order_id, 0, &oid_ind);
    SQLBindCol(stmt, 2, SQL_C_LONG, &out_user_id, 0, &uid_ind);
    SQLBindCol(stmt, 3, SQL_C_CHAR, status, sizeof(status), &status_ind);
    SQLBindCol(stmt, 4, SQL_C_TYPE_TIMESTAMP, &created_at, 0, &created_ind);

    if (SQLFetch(stmt) != SQL_SUCCESS)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Order not found"})";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    // Получаем позиции заказа с названиями товаров
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    const char* items_query =
        "SELECT oi.product_id, p.product_name, oi.quantity, oi.price "
        "FROM OrderItems oi "
        "JOIN Products p ON oi.product_id = p.id "
        "WHERE oi.order_id = ?";

    SQLPrepareA(stmt, (SQLCHAR*)items_query, SQL_NTS);
    SQLLEN oid_param_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &order_id, 0, &oid_param_ind);
    SQLExecute(stmt);

    SQLINTEGER product_id = 0, quantity = 0;
    SQLCHAR product_name[128] = {};
    SQLDOUBLE price = 0.0;
    SQLLEN pid_ind = 0, pname_ind = 0, qty_ind = 0, price_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_LONG, &product_id, 0, &pid_ind);
    SQLBindCol(stmt, 2, SQL_C_CHAR, product_name, sizeof(product_name), &pname_ind);
    SQLBindCol(stmt, 3, SQL_C_LONG, &quantity, 0, &qty_ind);
    SQLBindCol(stmt, 4, SQL_C_DOUBLE, &price, 0, &price_ind);

    json items = json::array();
    double total = 0.0;

    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        json item;
        item["product_id"] = product_id;
        item["product_name"] = std::string((char*)product_name);
        item["quantity"] = quantity;
        item["price"] = price;
        items.push_back(item);
        total += price * quantity;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    json response;
    response["order_id"] = out_order_id;
    response["user_id"] = out_user_id;
    response["status"] = std::string((char*)status);
    response["items"] = items;
    response["total"] = total;

    return response.dump();
}

std::string OrderRepository::get_orders_by_user(int user_id)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* query = "SELECT order_id, status, created_at FROM Orders WHERE user_id = ? ORDER BY created_at DESC";
    SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    SQLLEN uid_ind = 0;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &user_id, 0, &uid_ind);
    SQLExecute(stmt);

    SQLINTEGER order_id = 0;
    SQLCHAR status[32] = {};
    SQL_TIMESTAMP_STRUCT created_at{};
    SQLLEN oid_ind = 0, status_ind = 0, created_ind = 0;

    SQLBindCol(stmt, 1, SQL_C_LONG, &order_id, 0, &oid_ind);
    SQLBindCol(stmt, 2, SQL_C_CHAR, status, sizeof(status), &status_ind);
    SQLBindCol(stmt, 3, SQL_C_TYPE_TIMESTAMP, &created_at, 0, &created_ind);

    json orders = json::array();

    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        json item;
        item["order_id"] = order_id;
        item["status"] = std::string((char*)status);
        orders.push_back(item);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return orders.dump();
}

std::string OrderRepository::update_order_status(int order_id, const std::string& status)
{
    auto leased = pool.acquire();
    SQLHDBC dbc = leased.handle();

    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char* query = "UPDATE Orders SET status = ? WHERE order_id = ?";
    SQLPrepareA(stmt, (SQLCHAR*)query, SQL_NTS);

    SQLLEN status_len = SQL_NTS;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        status.size(), 0, (SQLPOINTER)status.c_str(), 0, &status_len);

    SQLLEN id_ind = 0;
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &order_id, 0, &id_ind);

    SQLRETURN result = SQLExecute(stmt);

    if (!SQL_SUCCEEDED(result))
    {
        print_odbc_error(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return R"({"error":"Update failed"})";
    }

    SQLLEN rows = 0;
    SQLRowCount(stmt, &rows);

    SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    if (rows == 0)
    {
        return R"({"error":"Order not found"})";
    }

    return R"({"status":"order updated"})";
}