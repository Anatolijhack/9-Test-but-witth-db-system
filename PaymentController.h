#pragma once
#include "OrderRepository.h"
#include "PaymentRepository.h"
#include "LiqPayHelper.h"
#include "Router.h"

class PaymentController
{
public:
    PaymentController(ConnectionPool& pool, const std::string& public_key, const std::string& private_key,
        const std::string& result_url, const std::string& server_url)
        : order_repository(pool), payment_repository(pool), liqpay(public_key, private_key),
        result_url(result_url), server_url(server_url) {}

    void register_routes(Router& router);

private:
    Response checkout(const Request& req);
    Response payment_callback(const Request& req);
    Response get_order(const Request& req);

    OrderRepository order_repository;
    PaymentRepository payment_repository;
    LiqPayHelper liqpay;
    std::string result_url;
    std::string server_url;
};