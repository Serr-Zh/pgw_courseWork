#include "udp_server.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>

UDPServer::UDPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger)
    : config_(config), session_manager_(session_manager), cdr_logger_(cdr_logger), running_(false), sockfd_(-1) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        Logger::get()->error("Failed to create socket: {}", strerror(errno));
        throw std::runtime_error("Socket creation failed");
    }

    server_addr_ = {};
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(config_.get_udp_port());
    if (inet_pton(AF_INET, config_.get_udp_ip().c_str(), &server_addr_.sin_addr) <= 0) {
        Logger::get()->error("Invalid IP address: {}", config_.get_udp_ip());
        close(sockfd_);
        throw std::runtime_error("Invalid IP address");
    }

    if (bind(sockfd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        Logger::get()->error("Failed to bind socket: {}", strerror(errno));
        close(sockfd_);
        throw std::runtime_error("Socket bind failed");
    }

    Logger::get()->info("UDP Server started on {}:{}", config_.get_udp_ip(), config_.get_udp_port());
}

UDPServer::~UDPServer() {
    stop();
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

void UDPServer::run() {
    running_ = true;
    char buffer[256];
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (running_) {
        ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&client_addr, &client_len);
        if (n < 0) {
            Logger::get()->error("Failed to receive data: {}", strerror(errno));
            continue;
        }

        buffer[n] = '\0';
        std::string imsi(buffer);
        Logger::get()->info("Received IMSI: {}", imsi);

        std::string response = session_manager_.create_session(imsi) ? "created" : "rejected";
        if (response == "created") {
            cdr_logger_.log(imsi, "created");
        }

        if (sendto(sockfd_, response.c_str(), response.size(), 0,
            (struct sockaddr*)&client_addr, client_len) < 0) {
            Logger::get()->error("Failed to send response: {}", strerror(errno));
        }
        else {
            Logger::get()->info("Sent response: {} for IMSI: {}", response, imsi);
        }
    }
}

void UDPServer::stop() {
    if (running_) {
        running_ = false;
        Logger::get()->info("UDP Server stopped");
    }
}

std::vector<uint8_t> UDPServer::to_bcd(const std::string& imsi) const {
    std::vector<uint8_t> bcd;
    for (size_t i = 0; i < imsi.size(); i += 2) {
        uint8_t byte = (imsi[i] - '0') << 4;
        if (i + 1 < imsi.size()) {
            byte |= (imsi[i + 1] - '0');
        }
        else {
            byte |= 0xF;
        }
        bcd.push_back(byte);
    }
    return bcd;
}

bool UDPServer::is_blacklisted(const std::string& imsi) const {
    const auto& blacklist = config_.get_blacklist();
    return std::find(blacklist.begin(), blacklist.end(), imsi) != blacklist.end();
}