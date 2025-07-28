#include "logger.hpp"
#include <stdexcept>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/common.h>

std::shared_ptr<Logger> Logger::instance_ = nullptr;

Logger::Logger(const std::string& log_file, const std::string& log_level) {
    // Создаём список синков: файл и консоль
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file, 5 * 1024 * 1024, 3);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::vector<spdlog::sink_ptr> sinks = { file_sink, console_sink };

    // Настраиваем синхронный логгер
    logger_ = std::make_shared<spdlog::logger>("pgw_logger", sinks.begin(), sinks.end());
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger_->flush_on(spdlog::level::info); // Автоматический flush для уровня info

    // Устанавливаем уровень логирования
    if (log_level == "DEBUG") {
        logger_->set_level(spdlog::level::debug);
    }
    else if (log_level == "INFO") {
        logger_->set_level(spdlog::level::info);
    }
    else if (log_level == "WARN") {
        logger_->set_level(spdlog::level::warn);
    }
    else if (log_level == "ERROR") {
        logger_->set_level(spdlog::level::err);
    }
    else if (log_level == "CRITICAL") {
        logger_->set_level(spdlog::level::critical);
    }
    else {
        throw std::runtime_error("Invalid log level: " + log_level);
    }
}

void Logger::init(const std::string& log_file, const std::string& log_level) {
    if (!instance_) {
        instance_ = std::make_shared<Logger>(log_file, log_level);
    }
}

std::shared_ptr<Logger> Logger::get() {
    if (!instance_) {
        throw std::runtime_error("Logger not initialized");
    }
    return instance_;
}

void Logger::debug(const std::string& message, const std::string& arg) {
    logger_->debug(arg.empty() ? message : fmt::format(message, arg));
}

void Logger::info(const std::string& message, const std::string& arg) {
    logger_->info(arg.empty() ? message : fmt::format(message, arg));
}

void Logger::info(const std::string& message) {
    info(message, "");
}

void Logger::warn(const std::string& message, const std::string& arg) {
    logger_->warn(arg.empty() ? message : fmt::format(message, arg));
}

void Logger::error(const std::string& message, const std::string& arg) {
    logger_->error(arg.empty() ? message : fmt::format(message, arg));
}

void Logger::critical(const std::string& message, const std::string& arg) {
    logger_->critical(arg.empty() ? message : fmt::format(message, arg));
}

void Logger::flush() {
    logger_->flush();
}