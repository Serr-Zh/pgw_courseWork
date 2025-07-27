#include "udp_client.hpp"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "logger.hpp"

UDPClient::UDPClient(const ClientConfig& config) : config_(config), sockfd_(-1) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        Logger::get()->error("Failed to create socket");
        throw std::runtime_error("Failed to create socket");
    }
    Logger::get()->info("UDP Client initialized for server {}:{}", config_.get_server_ip(), config_.get_server_port());
}

UDPClient::~UDPClient() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool UDPClient::send_imsi(const std::string& imsi, std::string& response, int retries, int timeout_ms) {
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config_.get_server_ip().c_str());
    server_addr.sin_port = htons(config_.get_server_port());

    for (int attempt = 0; attempt < retries; ++attempt) {
        Logger::get()->info("Sending IMSI: {} (attempt {}/{})", imsi, attempt + 1, retries);
        if (sendto(sockfd_, imsi.c_str(), imsi.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::get()->error("Failed to send IMSI: {}", strerror(errno));
            continue;
        }

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        char buffer[256];
        ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
        if (n < 0) {
            Logger::get()->error("Failed to receive response: {}", strerror(errno));
            continue;
        }

        buffer[n] = '\0';
        response = buffer;
        Logger::get()->info("Received response: {} for IMSI: {}", response, imsi);
        return true;
    }

    Logger::get()->error("Client failed: Failed to receive response after {} attempts", retries);
    return false;
}