#include "udp_server.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <thread>

UDPServer::UDPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger)
    : config_(config), session_manager_(session_manager), cdr_logger_(cdr_logger), sockfd_(-1), running_(false) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        Logger::get()->error("Failed to create socket: {}", strerror(errno));
        throw std::runtime_error("Socket creation failed");
    }

    server_addr_ = {};
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(config_.get_udp_port());
    if (inet_pton(AF_INET, config_.get_udp_ip().c_str(), &server_addr_.sin_addr) <= 0) {
        Logger::get()->error("Invalid UDP IP address: {}", config_.get_udp_ip());
        close(sockfd_);
        throw std::runtime_error("Invalid UDP IP address");
    }

    if (bind(sockfd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        Logger::get()->error("Failed to bind socket: {}", strerror(errno));
        close(sockfd_);
        throw std::runtime_error("Socket bind failed");
    }

    // Устанавливаем таймаут для сокета
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    Logger::get()->info("UDP Server started on {}:{}", config_.get_udp_ip(), config_.get_udp_port());
}

UDPServer::~UDPServer() {
    stop();
}

void UDPServer::run() {
    running_ = true;
    handle_request();
}

void UDPServer::stop() {
    if (running_) {
        running_ = false;
        close(sockfd_);
        Logger::get()->info("UDP Server stopped");
    }
}

void UDPServer::handle_request() {
    char buffer[256];
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (running_) {
        ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&client_addr, &client_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // Таймаут, продолжаем цикл
            }
            Logger::get()->error("Failed to receive data: {}", strerror(errno));
            break;
        }

        buffer[n] = '\0';
        std::string imsi(buffer);
        Logger::get()->info("Received IMSI: {}", imsi);

        std::string response = "rejected";
        if (session_manager_.create_session(imsi)) {
            cdr_logger_.log(imsi, "created");
            response = "created";
        }

        Logger::get()->info("Sent response: {} for IMSI: {}", response, imsi);
        sendto(sockfd_, response.c_str(), response.size(), 0,
            (struct sockaddr*)&client_addr, client_len);
    }
}