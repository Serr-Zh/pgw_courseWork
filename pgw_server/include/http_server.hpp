#pragma once

#include "config.hpp"
#include <logger.hpp>
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "udp_server.hpp"
#include <httplib.h>
#include <memory>
#include <thread>

class HTTPServer {
public:
    HTTPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger, UDPServer& udp_server);
    ~HTTPServer();
    
    HTTPServer(const HTTPServer&) = delete;
    HTTPServer& operator=(const HTTPServer&) = delete;
    
    void run();
    void stop();

private:
    void handle_check_subscriber(const httplib::Request& req, httplib::Response& res);
    void handle_stop(const httplib::Request& req, httplib::Response& res);
    
    const Config& config_;
    SessionManager& session_manager_;
    CDRLogger& cdr_logger_;
    UDPServer& udp_server_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_; // Теперь присоединяемый
    bool running_;
};