#pragma once

#include <spdlog/spdlog.h>
#include <string>

class Logger {
public:
    // Инициализация логгера с параметрами из конфигурации
    static void init(const std::string& log_file, const std::string& log_level);
    
    // Получение глобального логгера
    static std::shared_ptr<spdlog::logger> get() { return logger_; }

private:
    static std::shared_ptr<spdlog::logger> logger_;
};