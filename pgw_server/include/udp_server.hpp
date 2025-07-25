#pragma once

#include "config.hpp"
#include <logger.hpp>
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include <string>
#include <vector>
#include <netinet/in.h>
#include <atomic>

class UDPServer {
public:
    UDPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger);
    ~UDPServer();
    
    UDPServer(const UDPServer&) = delete;
    UDPServer& operator=(const UDPServer&) = delete;
    
    void run();
    void stop();

private:
    void handle_request();
    
    const Config& config_;
    SessionManager& session_manager_;
    CDRLogger& cdr_logger_;
    int sockfd_;
    sockaddr_in server_addr_;
    std::atomic<bool> running_;
};