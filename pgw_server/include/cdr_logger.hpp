#pragma once

#include "config.hpp"
#include <logger.hpp>
#include <string>
#include <fstream>
#include <mutex>

class CDRLogger {
public:
    explicit CDRLogger(const Config& config);
    ~CDRLogger();

    // Запрещаем копирование
    CDRLogger(const CDRLogger&) = delete;
    CDRLogger& operator=(const CDRLogger&) = delete;

    // Запись события в CDR
    void log(const std::string& imsi, const std::string& action);

private:
    const Config& config_;     // Поле для хранения конфигурации
    std::ofstream file_;       // Файл для записи CDR
    std::mutex mutex_;         // Мьютекс для потокобезопасности
};