#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <sstream>
#include <optional>
#include <iomanip>


struct AuthSession
{
    std::string username;
    std::string role;
};

class SessionStore
{
public:
    static SessionStore& instance()
    {
        static SessionStore store;
        return store;
    }

    std::string create_token(const std::string& username, const std::string& role)
    {
        std::string token = generate_token();

        std::lock_guard<std::mutex> lock(mtx);
        sessions[token] = { username, role };

        return token;
    }

    std::optional<AuthSession> find(const std::string& token)
    {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = sessions.find(token);
        if (it == sessions.end())
            return std::nullopt;

        return it->second;
    }

    void remove(const std::string& token)
    {
        std::lock_guard<std::mutex> lock(mtx);
        sessions.erase(token);
    }

private:
    std::mutex mtx;
    std::unordered_map<std::string, AuthSession> sessions;

    std::string generate_token()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dist;

        std::ostringstream oss;
        oss << std::hex << dist(gen) << dist(gen);

        return oss.str();
    }
};