#pragma once
#include "config.hpp"
#include "session_manager.hpp"
#include <httplib.h>
#include <thread>
#include <atomic>
#include <functional>
#include "interfaces.hpp"

// Абстрактный интерфейс для управления сессиями
// class ISessionManager {
// public:
//     virtual bool has_session(const std::string& imsi) = 0;
//     virtual void stop() = 0;
//     virtual ~ISessionManager() = default;
// };

// HTTP-сервер для обработки запросов /check_subscriber и /stop
class HTTPServer {
public:
    // Конструктор: принимает конфигурацию, логгер, менеджер сессий и функцию остановки
    HTTPServer(const Config& config, std::shared_ptr<ILogger> logger, std::shared_ptr<ISessionManager> session_manager, 
               std::function<void()> stop_callback, std::atomic<bool>& running);
    ~HTTPServer();

    // Запускает HTTP-сервер
    void run();

    // Останавливает HTTP-сервер
    void stop();

private:
    // Обрабатывает запрос /check_subscriber
    void handle_check_subscriber(const httplib::Request& req, httplib::Response& res);

    // Обрабатывает запрос /stop
    void handle_stop(const httplib::Request& req, httplib::Response& res);

    const Config& config;
    std::shared_ptr<ILogger> logger;
    std::shared_ptr<ISessionManager> session_manager;
    std::function<void()> stop_callback;
    std::unique_ptr<httplib::Server> server;
    std::thread server_thread;
    std::atomic<bool>& running;
    bool running_local;
};