#pragma once
#include <string>
#include <optional>

struct ValidationError
{
    std::string field;
    std::string message;
};

namespace validation
{
    inline bool contains_dangerous_chars(const std::string& value)
    {
        static const std::string dangerous = "<>;\"\\%";

        for (char c : value)
        {
            if (dangerous.find(c) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    inline std::optional<ValidationError> validate_name(const std::string& name, size_t max_length = 40)
    {
        if (name.empty())
        {
            return ValidationError{ "name", "Name cannot be empty" };
        }

        if (name.size() > max_length)
        {
            return ValidationError{ "name", "Name exceeds maximum length of " + std::to_string(max_length) + " characters" };
        }

        // Проверка на то, что строка не состоит только из пробелов
        bool only_spaces = true;
        for (char c : name)
        {
            if (c != ' ')
            {
                only_spaces = false;
                break;
            }
        }

        if (only_spaces)
        {
            return ValidationError{ "name", "Name cannot consist only of spaces" };
        }

        if (contains_dangerous_chars(name))
        {
            return ValidationError{ "name", "Name contains disallowed characters" };
        }

        return std::nullopt;
    }

    inline std::optional<ValidationError> validate_salary(int salary)
    {
        if (salary < 0)
        {
            return ValidationError{ "salary", "Salary cannot be negative" };
        }

        if (salary > 10'000'000)
        {
            return ValidationError{ "salary", "Salary exceeds reasonable maximum" };
        }

        return std::nullopt;
    }

    inline std::optional<ValidationError> validate_price(double price)
    {
        if (price < 0)
        {
            return ValidationError{ "price", "Price cannot be negative" };
        }

        if (price > 1'000'000)
        {
            return ValidationError{ "price", "Price exceeds reasonable maximum" };
        }

        return std::nullopt;
    }

    // Проверка логина — только буквы, цифры, подчёркивание и точка
    inline std::optional<ValidationError> validate_username(const std::string& username, size_t max_length = 50)
    {
        if (username.empty())
        {
            return ValidationError{ "username", "Username cannot be empty" };
        }

        if (username.size() < 3)
        {
            return ValidationError{ "username", "Username must be at least 3 characters long" };
        }

        if (username.size() > max_length)
        {
            return ValidationError{ "username", "Username exceeds maximum length of " + std::to_string(max_length) + " characters" };
        }

        for (char c : username)
        {
            bool ok = std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
            if (!ok)
            {
                return ValidationError{ "username", "Username may only contain letters, digits, underscore, and dot" };
            }
        }

        return std::nullopt;
    }

    inline std::optional<ValidationError> validate_password(const std::string& password)
    {
        if (password.size() < 6)
        {
            return ValidationError{ "password", "Password must be at least 6 characters long" };
        }

        if (password.size() > 128)
        {
            return ValidationError{ "password", "Password exceeds maximum length" };
        }

        return std::nullopt;
    }
}