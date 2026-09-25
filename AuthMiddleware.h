#pragma once
#include "Structs.h"
#include "SessionStore.h"
#include <nlohmann/json.hpp>

struct AuthCheckResult
{
    bool ok;
    std::string username;
    std::string role;
    Response error;
};

inline AuthCheckResult require_auth(const Request& req, const std::string& required_role = "")
{
    auto it = req.headers.find("authorization");

    if (it == req.headers.end() || it->second.rfind("Bearer ", 0) != 0)
    {
        return { false, "", "", Response{
            R"({"error":"Missing Authorization header"})",
            "application/json", "401 Unauthorized"
        } };
    }

    std::string token = it->second.substr(7);

    auto session = SessionStore::instance().find(token);

    if (!session)
    {
        return { false, "", "", Response{
            R"({"error":"Invalid or expired token"})",
            "application/json", "401 Unauthorized"
        } };
    }

    if (!required_role.empty() && session->role != required_role)
    {
        return { false, "", "", Response{
            R"({"error":"Insufficient permissions"})",
            "application/json", "403 Forbidden"
        } };
    }

    return { true, session->username, session->role, {} };
}