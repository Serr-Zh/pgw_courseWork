#pragma once
#include "config.hpp"
#include "cdr_logger.hpp"
#include "interfaces.hpp"
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

// Структура для хранения данных сессии
struct Session {
    std::chrono::system_clock::time_point creation_time;
};

// Класс для управления сессиями абонентов
class SessionManager : public ISessionManager {
public:
    // Конструктор: принимает конфигурацию и логгер CDR
    SessionManager(const Config& config, std::shared_ptr<CDRLogger> cdr_logger);
    ~SessionManager();

    // Запускает фоновую очистку истёкших сессий
    void run();

    // Останавливает менеджер сессий
    void stop() override;

    // Создаёт сессию для IMSI, если разрешено
    bool create_session(const std::string& imsi) override;

    // Проверяет наличие активной сессии для IMSI
    bool has_session(const std::string& imsi) override;

    // Удаляет истёкшие сессии
    void cleanup_expired_sessions();

private:
    const Config& config;
    std::shared_ptr<CDRLogger> cdr_logger;
    std::map<std::string, Session> sessions;
    std::mutex mutex;
    std::thread cleanup_thread;
    bool running;
};