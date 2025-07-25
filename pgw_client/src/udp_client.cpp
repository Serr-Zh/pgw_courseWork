#include "udp_client.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>

UDPClient::UDPClient(const ClientConfig& config) : config_(config), sockfd_(-1) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        Logger::get()->error("Failed to create socket: {}", strerror(errno));
        throw std::runtime_error("Socket creation failed");
    }

    server_addr_ = {};
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(config_.get_server_port());
    if (inet_pton(AF_INET, config_.get_server_ip().c_str(), &server_addr_.sin_addr) <= 0) {
        Logger::get()->error("Invalid server IP address: {}", config_.get_server_ip());
        close(sockfd_);
        throw std::runtime_error("Invalid server IP address");
    }

    Logger::get()->info("UDP Client initialized for server {}:{}", config_.get_server_ip(), config_.get_server_port());
}

UDPClient::~UDPClient() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

std::string UDPClient::send_imsi(const std::string& imsi) {
    // Устанавливаем таймаут для получения ответа
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Отправляем IMSI
    if (sendto(sockfd_, imsi.c_str(), imsi.size(), 0,
        (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        Logger::get()->error("Failed to send IMSI: {}", strerror(errno));
        throw std::runtime_error("Failed to send IMSI");
    }

    Logger::get()->info("Sent IMSI: {}", imsi);

    // Получаем ответ
    char buffer[256];
    sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0,
        (struct sockaddr*)&from_addr, &from_len);
    if (n < 0) {
        Logger::get()->error("Failed to receive response: {}", strerror(errno));
        throw std::runtime_error("Failed to receive response");
    }

    buffer[n] = '\0';
    std::string response(buffer);
    Logger::get()->info("Received response: {} for IMSI: {}", response, imsi);
    return response;
}