#pragma once
#include <string>
#include "ProductRepository.h"

class ProductService
{
private:
    ProductRepository repository;
public:
    ProductService(ConnectionPool& pool) : repository(pool) {}
    std::string get_products();
    std::string get_product(int id);
    std::string add_product(const std::string& type, const std::string& name, double cost, double price, int stock);
    std::string update_product(int id, double price, int stock);
    std::string delete_product(int id);
};