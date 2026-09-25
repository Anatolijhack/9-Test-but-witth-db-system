// UserRepository.h
#pragma once
#include <string>
#include <optional>
#include "ConnectionPool.h"

struct UserRecord
{
    std::string username;
    std::string password_hash;;
    std::string role;
};

class UserRepository
{
public:
    UserRepository(ConnectionPool& pool) : pool(pool) {}

    std::optional<UserRecord> find_by_username(const std::string& username);
    std::string create_user(const std::string& username, const std::string& password_hash, const std::string& role);

private:
    ConnectionPool& pool;
};