#pragma once

#include "Router.h"
#include "ProductService.h"
#include "Validation.h"

class ProductController
{
private:
    ProductService service;
    
    Response get_products(const Request& req);
    Response get_product(const Request& req);
    Response add_product(const Request& req);
    Response delete_product(const Request& req);
    Response update_product(const Request& req);


public:
    void register_routes(Router& router);
    ProductController(ConnectionPool& pool) : service(pool) {}
};