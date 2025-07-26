#include "udp_server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex>

UDPServer::UDPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger)
    : config_(config), session_manager_(session_manager), cdr_logger_(cdr_logger), sockfd_(-1), running_(false) {
}

UDPServer::~UDPServer() {
    stop();
}

void UDPServer::run() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        Logger::get()->error("Failed to create socket");
        return;
    }

    // Устанавливаем сокет в неблокирующий режим
    if (fcntl(sockfd_, F_SETFL, O_NONBLOCK) < 0) {
        Logger::get()->error("Failed to set socket to non-blocking");
        close(sockfd_);
        return;
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config_.get_udp_ip().c_str());
    server_addr.sin_port = htons(config_.get_udp_port());

    if (bind(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::get()->error("Failed to bind socket");
        close(sockfd_);
        return;
    }

    Logger::get()->info("UDP Server started on {}:{}", config_.get_udp_ip(), config_.get_udp_port());
    running_ = true;

    char buffer[256];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (running_) {
        ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (running_) {
                Logger::get()->error("Failed to receive data: {}", strerror(errno));
            }
            continue;
        }

        buffer[n] = '\0';
        std::string imsi(buffer);
        Logger::get()->info("Received IMSI: {}", imsi);

        // Проверка формата IMSI: 15 цифр
        std::regex imsi_regex("^[0-9]{15}$");
        if (!std::regex_match(imsi, imsi_regex)) {
            Logger::get()->warn("Invalid IMSI format: {}", imsi);
            sendto(sockfd_, "rejected", 8, 0, (struct sockaddr*)&client_addr, addr_len);
            continue;
        }

        bool created = session_manager_.create_session(imsi);
        std::string response = created ? "created" : "rejected";
        Logger::get()->info("Sent response: {} for IMSI: {}", response, imsi);
        sendto(sockfd_, response.c_str(), response.size(), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}

void UDPServer::stop() {
    if (running_) {
        running_ = false;
        if (sockfd_ >= 0) {
            // Отправляем фиктивный пакет для разблокировки recvfrom
            struct sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            addr.sin_port = htons(config_.get_udp_port());
            sendto(sockfd_, "stop", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
            close(sockfd_);
            sockfd_ = -1;
        }
        Logger::get()->info("UDP Server stopped");
    }
}