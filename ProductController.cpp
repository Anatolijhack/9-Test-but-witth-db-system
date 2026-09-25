#include "ProductController.h"
#include "AuthMiddleware.h"
using json = nlohmann::json;


void ProductController::register_routes(Router& router)
{
    router.add(
        "GET",
        "/products",
        [this](const Request& req)
        {
            return get_products(req);
        }
    );

    router.add(
        "GET",
        "/products/:id",
        [this](const Request& req)
        {
            return get_product(req);
        }
    );

    router.add(
        "POST",
        "/products",
        [this](const Request& req)
        {
            return add_product(req);
        }
    );

    router.add(
        "DELETE",
        "/products/:id",
        [this](const Request& req)
        {
            return delete_product(req);
        }
    );

    router.add(
        "PUT",
        "/products/:id",
        [this](const Request& req)
        {
            return update_product(req);
        }
    );
}


Response ProductController::get_products(const Request& req)
{
    return Response{
        service.get_products(),
        "application/json"
    };
}


Response ProductController::get_product(const Request& req)
{
    try
    {
        int id = std::stoi(req.params.at("id"));

        return Response{
            service.get_product(id),
            "application/json"
        };
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid product id"})",
            "application/json",
            "400 Bad Request"
        };
    }
}


Response ProductController::add_product(const Request& req)
{
    auto auth = require_auth(req);

    if (!auth.ok)
    {
        return auth.error;
    }

    try
    {
        auto data = nlohmann::json::parse(req.body);

        std::string type =
            data.at("type").get<std::string>();

        std::string name =
            data.at("name").get<std::string>();

        double cost =
            data.at("cost").get<double>();

        double price =
            data.at("price").get<double>();

        int stock =
            data.at("stock_quantity").get<int>();

        return Response{
            service.add_product(
                type,
                name,
                cost,
                price,
                stock
            ),
            "application/json",
            "201 Created"
        };
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid JSON or missing fields"})",
            "application/json",
            "400 Bad Request"
        };
    }
}


Response ProductController::delete_product(const Request& req)
{
    auto auth = require_auth(req, "admin");

    if (!auth.ok)
    {
        return auth.error;
    }

    int id;

    try
    {
        id = std::stoi(req.params.at("id"));
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid product id"})",
            "application/json",
            "400 Bad Request"
        };
    }

    std::string result =
        service.delete_product(id);

    json result_json;

    try
    {
        result_json = json::parse(result);
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid service response"})",
            "application/json",
            "500 Internal Server Error"
        };
    }

    std::string status_code = "200 OK";

    if (result_json.contains("error"))
    {
        if (result_json["error"] == "Product not found")
        {
            status_code = "404 Not Found";
        }
        else
        {
            status_code = "400 Bad Request";
        }
    }

    return Response{
        result_json.dump(),
        "application/json",
        status_code
    };
}


Response ProductController::update_product(const Request& req)
{
    int id;

    try
    {
        id = std::stoi(req.params.at("id"));
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid product id"})",
            "application/json",
            "400 Bad Request"
        };
    }

    json body;

    try
    {
        body = json::parse(req.body);
    }
    catch (...)
    {
        return Response{
            R"({"error":"Invalid JSON"})",
            "application/json",
            "400 Bad Request"
        };
    }

    try
    {
        double price =
            body.at("price").get<double>();

        int stock =
            body.at("stock_quantity").get<int>();

        std::string result =
            service.update_product(
                id,
                price,
                stock
            );

        json result_json =
            json::parse(result);

        std::string status_code = "200 OK";

        if (result_json.contains("error"))
        {
            if (result_json["error"] == "Product not found")
            {
                status_code = "404 Not Found";
            }
            else
            {
                status_code = "400 Bad Request";
            }
        }

        return Response{
            result_json.dump(),
            "application/json",
            status_code
        };
    }
    catch (...)
    {
        return Response{
            R"({"error":"Missing or invalid fields: price, stock_quantity"})",
            "application/json",
            "400 Bad Request"
        };
    }
}