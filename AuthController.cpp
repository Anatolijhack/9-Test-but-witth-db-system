// AuthController.cpp
#include "AuthController.h"
#include "SessionStore.h"
#include <nlohmann/json.hpp>
#include "PasswordHash.h" 
#include "Validation.h"
using json = nlohmann::json;

void AuthController::register_routes(Router& router)
{
    router.add("POST", "/login", [this](const Request& req)
        {
            return login(req);
        }
    );

    router.add("POST", "/register", [this](const Request& req)
        {
            return register_user(req);
        }
    );

}

Response AuthController::login(const Request& req)
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

    std::string username, password;

    try
    {
        username = body.at("username").get<std::string>();
        password = body.at("password").get<std::string>();
    }
    catch (...)
    {
        return Response{ R"({"error":"Missing username or password"})", "application/json", "400 Bad Request" };
    }

    auto user = user_repository.find_by_username(username);

    if (!user || !PasswordHash::verify_password(password, user->password_hash))
    {
        return Response{ R"({"error":"Invalid credentials"})", "application/json", "401 Unauthorized" };
    }

    std::string token = SessionStore::instance().create_token(user->username, user->role);

    json response = { {"token", token}, {"role", user->role} };

    return Response{ response.dump(), "application/json" };
}

Response AuthController::register_user(const Request& req)
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

    std::string username, password, role;

    try
    {
        username = body.at("username").get<std::string>();
        password = body.at("password").get<std::string>();
        role = body.value("role", "employee"); // если роль не указана — по умолчанию employee
    }
    catch (...)
    {
        return Response{ R"({"error":"Missing username or password"})", "application/json", "400 Bad Request" };
    }

    // Валидация
    if (auto error = validation::validate_username(username))
    {
        json err = { {"error", error->message}, {"field", error->field} };
        return Response{ err.dump(), "application/json", "400 Bad Request" };
    }

    if (auto error = validation::validate_password(password))
    {
        json err = { {"error", error->message}, {"field", error->field} };
        return Response{ err.dump(), "application/json", "400 Bad Request" };
    }

    if (password.size() < 8)
    {
        return Response{
            R"({"error":"Password must be at least 6 characters"})",
            "application/json", "400 Bad Request"
        };
    }

    // Ограничиваем допустимые роли, чтобы нельзя было прислать что угодно
    if (role != "admin" && role != "employee")
    {
        return Response{
            R"({"error":"Invalid role. Must be 'admin' or 'employee'"})",
            "application/json", "400 Bad Request"
        };
    }

    std::string password_hash = PasswordHash::hash_password(password);

    std::string result = user_repository.create_user(username, password_hash, role);
    json result_json = json::parse(result);

    std::string status_code = "201 Created";
    if (result_json.contains("error"))
    {
        status_code = result_json["error"] == "Username already exists" ? "409 Conflict" : "400 Bad Request";
    }

    return Response{ result_json.dump(), "application/json", status_code };
}

//Response AuthController::register_user(const Request& req)
//{
//    json body;
//
//    try
//    {
//        body = json::parse(req.body);
//    }
//    catch (...)
//    {
//        return Response{ R"({"error":"Invalid JSON"})", "application/json", "400 Bad Request" };
//    }
//
//    std::string username, password;
//    std::string role = "employee"; // фиксированная роль для саморегистрации
//
//    try
//    {
//        username = body.at("username").get<std::string>();
//        password = body.at("password").get<std::string>();
//    }
//    catch (...)
//    {
//        return Response{ R"({"error":"Missing username or password"})", "application/json", "400 Bad Request" };
//    }
//
//    if (auto error = validation::validate_username(username))
//    {
//        json err = { {"error", error->message}, {"field", error->field} };
//        return Response{ err.dump(), "application/json", "400 Bad Request" };
//    }
//
//    if (auto error = validation::validate_password(password))
//    {
//        json err = { {"error", error->message}, {"field", error->field} };
//        return Response{ err.dump(), "application/json", "400 Bad Request" };
//    }
//
//    std::string password_hash = PasswordHash::hash_password(password);
//
//    std::string result = user_repository.create_user(username, password_hash, role);
//    json result_json = json::parse(result);
//
//    std::string status_code = "201 Created";
//    if (result_json.contains("error"))
//    {
//        status_code = result_json["error"] == "Username already exists" ? "409 Conflict" : "400 Bad Request";
//    }
//
//    return Response{ result_json.dump(), "application/json", status_code };
//}
