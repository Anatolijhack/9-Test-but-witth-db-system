#include "PaymentController.h"
#include "FormParser.h"
#include "Logger.h"
#include "Validation.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <cmath>

using json = nlohmann::json;

void PaymentController::register_routes(Router& router)
{
    router.add("POST", "/checkout", [this](const Request& req)
        {
            return checkout(req);
        });

    router.add("GET", "/orders/:id", [this](const Request& req)
        {
            return get_order(req);
        });

    router.add("POST", "/payment/callback", [this](const Request& req)
        {
            return payment_callback(req);
        });
}

static std::string generate_payment_ref()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << "PAY-" << std::hex << dist(gen);
    return oss.str();
}

// POST /checkout Ч создаЄт заказ из позиций и сразу генерирует форму оплаты
Response PaymentController::checkout(const Request& req)
{
    json body;

    try
    {
        body = json::parse(req.body);
    }
    catch (...)
    {
        return Response{ R"({"error":"Invalid JSON"})", "application/json", "400 Bad Request" };
    }

    int user_id;
    std::vector<OrderItemInput> items;

    try
    {
        user_id = body.at("user_id").get<int>();

        for (const auto& raw_item : body.at("items"))
        {
            OrderItemInput item;
            item.product_id = raw_item.at("product_id").get<int>();
            item.quantity = raw_item.at("quantity").get<int>();
            items.push_back(item);
        }
    }
    catch (...)
    {
        return Response{ R"({"error":"Missing or invalid fields: user_id, items"})", "application/json", "400 Bad Request" };
    }

    if (items.empty())
    {
        return Response{ R"({"error":"Order must contain at least one item"})", "application/json", "400 Bad Request" };
    }

    // 1. —оздаЄм заказ Ч цены подт€гиваютс€ из Ѕƒ внутри create_order, остаток провер€етс€ и списываетс€
    std::string order_result_str = order_repository.create_order(user_id, items);
    json order_result = json::parse(order_result_str);

    if (order_result.contains("error"))
    {
        LOG_ERROR("Order creation failed: " + order_result_str);
        return Response{ order_result.dump(), "application/json", "400 Bad Request" };
    }

    int order_id = order_result.at("order_id").get<int>();
    double total = order_result.at("total").get<double>();

    LOG_INFO("Order created: order_id=" + std::to_string(order_id) + " total=" + std::to_string(total));

    // 2. —оздаЄм запись платежа, прив€занную к заказу
    std::string payment_ref = generate_payment_ref();
    payment_repository.create_payment(order_id, payment_ref, total);

    // 3. √енерируем форму LiqPay
    try
    {
        auto form = liqpay.create_payment(payment_ref, total, "Auto parts order #" + std::to_string(order_id), result_url, server_url);

        json response = {
            {"order_id", order_id},
            {"payment_ref", payment_ref},
            {"total", total},
            {"data", form.data},
            {"signature", form.signature},
            {"checkout_url", "https://www.liqpay.ua/api/3/checkout"}
        };

        return Response{ response.dump(), "application/json" };
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(std::string("liqpay.create_payment threw: ") + e.what());
        return Response{ R"({"error":"Payment form generation failed"})", "application/json", "500 Internal Server Error" };
    }
}

// GET /orders/:id Ч посмотреть состав и статус заказа
Response PaymentController::get_order(const Request& req)
{
    int order_id;

    try
    {
        order_id = std::stoi(req.params.at("id"));
    }
    catch (...)
    {
        return Response{ R"({"error":"Invalid order id"})", "application/json", "400 Bad Request" };
    }

    std::string result = order_repository.get_order(order_id);
    json result_json = json::parse(result);

    std::string status_code = result_json.contains("error") ? "404 Not Found" : "200 OK";

    return Response{ result_json.dump(), "application/json", status_code };
}

// POST /payment/callback Ч обратный вызов от LiqPay
Response PaymentController::payment_callback(const Request& req)
{
    LOG_INFO("Payment callback raw body: " + req.body);

    auto form_data = parse_form_urlencoded(req.body);

    auto data_it = form_data.find("data");
    auto sig_it = form_data.find("signature");

    if (data_it == form_data.end() || sig_it == form_data.end())
    {
        return Response{ "Bad Request", "text/plain", "400 Bad Request" };
    }

    if (!liqpay.verify_signature(data_it->second, sig_it->second))
    {
        LOG_ERROR("LiqPay callback: invalid signature Ч possible forged request");
        return Response{ "Forbidden", "text/plain", "403 Forbidden" };
    }

    json payment_data = liqpay.decode_data(data_it->second);

    std::string payment_ref = payment_data.at("order_id").get<std::string>(); // это payment_ref, а не order_id заказа!
    std::string status = payment_data.at("status").get<std::string>();
    double callback_amount = payment_data.value("amount", 0.0);

    auto payment = payment_repository.find_by_ref(payment_ref);

    if (!payment)
    {
        LOG_ERROR("Payment callback: unknown payment_ref=" + payment_ref);
        return Response{ "Payment not found", "text/plain", "404 Not Found" };
    }

    if (payment->status == "Paid")
    {
        LOG_INFO("Payment callback: payment=" + payment_ref + " already processed, skipping");
        return Response{ "OK", "text/plain" };
    }

    if (std::abs(callback_amount - payment->amount) > 0.01)
    {
        LOG_ERROR("Payment amount mismatch: payment=" + payment_ref);
        return Response{ "Amount mismatch", "text/plain", "400 Bad Request" };
    }

    std::string internal_status = (status == "success" || status == "sandbox") ? "Paid" : "Failed";

    payment_repository.update_payment_status(payment_ref, internal_status);

    // —инхронизируем статус самого заказа
    if (internal_status == "Paid")
    {
        order_repository.update_order_status(payment->order_id, "Processing");
    }
    else
    {
        order_repository.update_order_status(payment->order_id, "Cancelled");
    }

    LOG_INFO("Payment callback: payment=" + payment_ref + " order=" + std::to_string(payment->order_id) + " status=" + internal_status);

    return Response{ "OK", "text/plain" };
}