#pragma once

#include "client_config.hpp"
#include <logger.hpp>
#include <string>
#include <netinet/in.h>

class UDPClient {
public:
    UDPClient(const ClientConfig& config);
    ~UDPClient();
    
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;
    
    // std::string send_imsi(const std::string& imsi);
    bool send_imsi(const std::string& imsi, std::string& response, int retries = 3, int timeout_ms = 1000);

private:
    int sockfd_;
    sockaddr_in server_addr_;
    const ClientConfig& config_;
};