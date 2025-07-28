#pragma once

#include "interfaces.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

// Реализация логгера на основе spdlog
class Logger : public ILogger {
public:
    // Инициализация логгера с файлом и уровнем логирования
    static void init(const std::string& log_file, const std::string& log_level);

    // Получение экземпляра логгера (синглтон)
    static std::shared_ptr<Logger> get();

    // Реализация методов ILogger
    void debug(const std::string& message, const std::string& arg = "") override;
    void info(const std::string& message, const std::string& arg = "") override;
    void info(const std::string& message); // Перегрузка для одного аргумента, не override
    void warn(const std::string& message, const std::string& arg = "") override;
    void error(const std::string& message, const std::string& arg = "") override;
    void critical(const std::string& message, const std::string& arg = "") override;
    void flush() override;

    Logger(const std::string& log_file, const std::string& log_level); // Публичный конструктор для компиляции

private:
    static std::shared_ptr<Logger> instance_;
    std::shared_ptr<spdlog::logger> logger_;
};