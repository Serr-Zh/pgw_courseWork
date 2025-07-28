#pragma once

#include <string>

// Интерфейс логгера для инверсии зависимостей
class ILogger {
public:
    virtual void debug(const std::string& message, const std::string& arg = "") = 0;
    virtual void info(const std::string& message, const std::string& arg = "") = 0;
    virtual void warn(const std::string& message, const std::string& arg = "") = 0;
    virtual void error(const std::string& message, const std::string& arg = "") = 0;
    virtual void critical(const std::string& message, const std::string& arg = "") = 0;
    virtual void flush() = 0;
    virtual ~ILogger() = default;
};

// Интерфейс для управления сессиями
class ISessionManager {
public:
    virtual bool has_session(const std::string& imsi) = 0;
    virtual void stop() = 0;
    virtual bool create_session(const std::string& imsi) = 0; // Добавлено для UDPServer
    virtual ~ISessionManager() = default;
};