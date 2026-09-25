#pragma once
#include <functional>
#include <string>
#include <iostream>
#include <unordered_map>
#include "Structs.h"
#include <sstream>
#include <vector>



using Handler = std::function<Response(const Request&)>;

class Router {
private:
    struct Route {
        std::string method;
        std::string path;
        std::function<Response(const Request&)> handler;
    };

    std::vector<Route> routes;

    bool match(const std::string& route_path,
        const std::string& request_path,
        Request& req)
    {
        std::istringstream r(route_path);
        std::istringstream p(request_path);

        std::string r_seg, p_seg;

        while (true)
        {
            bool route_ok = static_cast<bool>(
                std::getline(r, r_seg, '/')
                );

            bool request_ok = static_cast<bool>(
                std::getline(p, p_seg, '/')
                );

            // Оба пути закончились одновременно
            if (!route_ok && !request_ok)
                return true;

            // Один закончился раньше другого
            if (!route_ok || !request_ok)
                return false;

            // Параметр :id
            if (!r_seg.empty() && r_seg[0] == ':')
            {
                std::string param_name = r_seg.substr(1);

                if (param_name.empty())
                    return false;

                req.params[param_name] = p_seg;
            }
            else
            {
                if (r_seg != p_seg)
                    return false;
            }
        }
    }

public:
    void add(const std::string& method,
        const std::string& path,
        std::function<Response(const Request&)> handler)
    {
        routes.push_back({ method, path, handler });
    }

    Response route(Request req)
    {
        std::cout << "ROUTER: " << req.method
            << " " << req.path << std::endl;

        for (const auto& r : routes)
        {
            req.params.clear();

            std::cout << "CHECK: "
                << r.method << " "
                << r.path << std::endl;

            if (r.method != req.method)
                continue;

            if (match(r.path, req.path, req))
            {
                std::cout << "MATCH!" << std::endl;

                try
                {
                    Response response = r.handler(req);

                    std::cout << "HANDLER DONE" << std::endl;

                    return response;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "HANDLER EXCEPTION: "
                        << e.what()
                        << std::endl;

                    return Response{
                        "Internal Server Error",
                        "text/plain",
                        "500 Internal Server Error"
                    };
                }
            }
        }

        std::cout << "NOT FOUND" << std::endl;

        return Response{
            "Not Found",
            "text/plain",
            "404 Not Found"
        };
    }
};