#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "ConnectionPool.h"
//#include <windows.h> 
//#include <sql.h>
//#include <sqlext.h>
//struct databaseconnection
//{
//    sqlhenv env = nullptr;
//    sqlhdbc dbc = nullptr;
//};
struct ProductRecord
{
    int id;
    std::string type;
    std::string product_name;
    double price;
    int stock_quantity;
};
class ProductRepository
{
public:
    ProductRepository(ConnectionPool& pool) : pool(pool) {}

    std::string get_products();
    std::string get_product(int id);
    std::string add_product(const std::string& type, const std::string& name, double cost, double price, int stock);
    std::string update_product(int id, double price, int stock);
    std::string delete_product(int id);

private:
    ConnectionPool& pool;
};