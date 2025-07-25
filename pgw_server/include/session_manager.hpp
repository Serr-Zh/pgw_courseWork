#pragma once

#include "config.hpp"
#include "logger.hpp"
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>

class SessionManager {
public:
    explicit SessionManager(const Config& config);
    ~SessionManager();

    // Запрещаем копирование
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    // Создание сессии для IMSI
    bool create_session(const std::string& imsi);

    // Проверка существования сессии
    bool has_session(const std::string& imsi) const;

    // Остановка менеджера (удаление всех сессий)
    void stop();

private:
    // Структура сессии
    struct Session {
        std::chrono::system_clock::time_point start_time;
    };

    // Очистка устаревших сессий
    void cleanup_expired_sessions();

    const Config& config_;                          // Ссылка на конфигурацию
    std::unordered_map<std::string, Session> sessions_; // Активные сессии
    mutable std::mutex mutex_;                     // Мьютекс для потокобезопасности
    std::thread cleanup_thread_;                   // Поток для очистки сессий
    bool running_;                                 // Флаг работы
};