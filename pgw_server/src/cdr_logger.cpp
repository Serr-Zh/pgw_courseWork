#include "cdr_logger.hpp"
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>

// Конструктор: инициализирует CDRLogger с конфигурацией и логгером
CDRLogger::CDRLogger(const Config& config, std::shared_ptr<ILogger> logger)
    : config(config), logger(logger) {
    // Проверяем существование директории для CDR-файла
    auto parent_path = std::filesystem::path(config.get_cdr_file()).parent_path();
    if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
        std::filesystem::create_directories(parent_path);
    }

    file.open(config.get_cdr_file(), std::ios::app);
    if (!file.is_open()) {
        logger->error("Failed to open CDR log file", config.get_cdr_file());
        throw std::runtime_error("Failed to open CDR log file: " + config.get_cdr_file());
    }
    logger->info("CDRLogger initialized with file", config.get_cdr_file());
}

// Деструктор: закрывает файл
CDRLogger::~CDRLogger() {
    if (file.is_open()) {
        file.flush();
        file.close();
    }
}

// Записывает событие в CDR-файл
void CDRLogger::log(const std::string& imsi, const std::string& action) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!file.is_open()) {
        logger->error("CDR file is not open", config.get_cdr_file());
        file.open(config.get_cdr_file(), std::ios::app);
        if (!file.is_open()) {
            logger->error("Failed to reopen CDR file", config.get_cdr_file());
            return;
        }
    }

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    file << ss.str() << "," << imsi << "," << action << "\n";
    file.flush(); // Гарантируем запись
    std::stringstream log_ss;
    log_ss << "CDR logged: IMSI: " << imsi << ", action: " << action;
    logger->info(log_ss.str());
}

