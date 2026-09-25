// AuthController.h
#pragma once
#include "UserRepository.h"
#include "Router.h"

class AuthController
{
public:
    AuthController(ConnectionPool& pool) : user_repository(pool) {}

    void register_routes(Router& router);

private:
    Response register_user(const Request& req);
    Response login(const Request& req);
    UserRepository user_repository;
};