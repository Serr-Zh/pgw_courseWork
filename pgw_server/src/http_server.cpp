#include "http_server.hpp"
#include <thread>
#include <chrono>
#include <sstream>

// Конструктор: инициализирует HTTP-сервер с зависимостями
HTTPServer::HTTPServer(const Config& config, std::shared_ptr<ILogger> logger,
    std::shared_ptr<ISessionManager> session_manager, std::function<void()> stop_callback,
    std::atomic<bool>& running)
    : config(config), logger(logger), session_manager(session_manager), stop_callback(stop_callback),
    running(running), running_local(false) {
    server = std::make_unique<httplib::Server>();

    // Регистрируем обработчики HTTP-запросов
    server->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
        handle_check_subscriber(req, res);
        });
    server->Get("/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handle_stop(req, res);
        });

    logger->info("HTTP Server initialized on port: {}", std::to_string(config.get_http_port()));
}

// Деструктор: останавливает сервер
HTTPServer::~HTTPServer() {
    stop();
}

// Запускает HTTP-сервер в отдельном потоке
void HTTPServer::run() {
    running_local = true;
    logger->info("Starting HTTP Server on port: {}", std::to_string(config.get_http_port()));

    server_thread = std::thread([this]() {
        if (!server->listen("0.0.0.0", config.get_http_port())) {
            logger->error("Failed to start HTTP server on port: {}", std::to_string(config.get_http_port()));
        }
        });
}

// Останавливает HTTP-сервер и все зависимости
void HTTPServer::stop() {
    if (running_local) {
        running_local = false;
        server->stop();
        session_manager->stop();
        stop_callback(); // Вызываем внешнюю функцию остановки (например, для UDP-сервера)

        if (server_thread.joinable()) {
            server_thread.join();
        }

        logger->info("HTTP Server stopped");
        logger->flush(); // Гарантируем запись
    }
}

// Обрабатывает запрос /check_subscriber
void HTTPServer::handle_check_subscriber(const httplib::Request& req, httplib::Response& res) {
    auto imsi = req.get_param_value("imsi");
    if (imsi.empty()) {
        res.status = 400;
        res.set_content("Missing IMSI parameter", "text/plain");
        logger->warn("Check subscriber request failed: missing IMSI");
        return;
    }

    bool has_session = session_manager->has_session(imsi);
    std::string result = has_session ? "active" : "not active";
    res.set_content(result, "text/plain");
    logger->info("Check subscriber request", "IMSI: " + imsi + ", result: " + result); // Конкатенация
}

// Обрабатывает запрос /stop
void HTTPServer::handle_stop(const httplib::Request& req, httplib::Response& res) {
    res.set_content("Stopping server...", "text/plain");
    logger->info("Received stop request");

    running = false;
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop();
        }).detach();
}