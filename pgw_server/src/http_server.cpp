#include "http_server.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

HTTPServer::HTTPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger, UDPServer& udp_server, std::atomic<bool>& running)
    : config_(config), session_manager_(session_manager), cdr_logger_(cdr_logger), udp_server_(udp_server), running_(running), running_local_(false) {
    server_ = std::make_unique<httplib::Server>();

    server_->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
        handle_check_subscriber(req, res);
        });

    server_->Get("/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handle_stop(req, res);
        });

    Logger::get()->info("HTTP Server initialized on port: {}", config_.get_http_port());
}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::run() {
    running_local_ = true;
    Logger::get()->info("Starting HTTP Server on port: {}", config_.get_http_port());

    server_thread_ = std::thread([this]() {
        if (!server_->listen("0.0.0.0", config_.get_http_port())) {
            Logger::get()->error("Failed to start HTTP server on port: {}", config_.get_http_port());
        }
        });
}

void HTTPServer::stop() {
    if (running_local_) {
        running_local_ = false;
        server_->stop();

        // Останавливаем UDP-сервер
        udp_server_.stop();

        // Останавливаем SessionManager
        session_manager_.stop();

        // Ждём завершения потока сервера
        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        Logger::get()->info("HTTP Server stopped");
        Logger::get()->flush();
    }
}

void HTTPServer::handle_check_subscriber(const httplib::Request& req, httplib::Response& res) {
    auto imsi = req.get_param_value("imsi");
    if (imsi.empty()) {
        res.status = 400;
        res.set_content("Missing IMSI parameter", "text/plain");
        Logger::get()->warn("Check subscriber request failed: missing IMSI");
        return;
    }

    bool has_session = session_manager_.has_session(imsi);
    res.set_content(has_session ? "active" : "not active", "text/plain");
    Logger::get()->info("Check subscriber request for IMSI: {}, result: {}", imsi, has_session ? "active" : "not active");
}

void HTTPServer::handle_stop(const httplib::Request& req, httplib::Response& res) {
    res.set_content("Stopping server...", "text/plain");
    Logger::get()->info("Received stop request");

    // Устанавливаем running = false для завершения main
    running_ = false;

    // Запускаем stop асинхронно, чтобы дать время на отправку ответа
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop();
        }).detach();
}