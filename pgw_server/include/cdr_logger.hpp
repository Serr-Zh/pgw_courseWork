#pragma once

#include "config.hpp"
#include <logger.hpp>
#include <string>
#include <fstream>
#include <mutex>
#include "interfaces.hpp"

// Реализация CDRLogger для записи событий в файл CDR
class CDRLogger {
public:
    // Конструктор принимает конфигурацию и логгер
    CDRLogger(const Config& config, std::shared_ptr<ILogger> logger);
    ~CDRLogger();

    // Запрещаем копирование для предотвращения дублирования файловых дескрипторов
    CDRLogger(const CDRLogger&) = delete;
    CDRLogger& operator=(const CDRLogger&) = delete;

    // Записывает событие в CDR-файл в формате: timestamp,IMSI,action
    void log(const std::string& imsi, const std::string& action);

    // Возвращает логгер для диагностики
    std::shared_ptr<ILogger> get_logger() const { return logger; }

private:
    const Config& config;               // Конфигурация сервера
    std::ofstream file;                // Файловый поток для CDR
    std::mutex mutex;                  // Мьютекс для потокобезопасности
    std::shared_ptr<ILogger> logger;   // Логгер для диагностики
};