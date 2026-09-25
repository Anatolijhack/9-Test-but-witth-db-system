#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

enum class LogLevel
{
    Info,
    Warning,
    Error
};

class Logger
{
public:
    static Logger& instance()
    {
        static Logger logger;
        return logger;
    }

    void init(const std::string& file_path)
    {
        std::lock_guard<std::mutex> lock(mtx);
        log_file.open(file_path, std::ios::app);
    }

    void log(LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mtx);

        std::string line = format_line(level, message);

        std::cout << line << std::endl;

        if (log_file.is_open())
        {
            log_file << line << std::endl;
            log_file.flush();
        }
    }

    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }

private:
    std::mutex mtx;
    std::ofstream log_file;

    Logger() = default;

    std::string level_to_string(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        }
        return "UNKNOWN";
    }

    std::string format_line(LogLevel level, const std::string& message)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm_buf;
        localtime_s(&tm_buf, &time); // Windows-специфично; дл€ Linux Ч localtime_r

        std::ostringstream oss;
        oss << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << level_to_string(level) << "] "
            << message;

        return oss.str();
    }
};

// ”добные макросы
#define LOG_INFO(msg)    Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg)   Logger::instance().error(msg)