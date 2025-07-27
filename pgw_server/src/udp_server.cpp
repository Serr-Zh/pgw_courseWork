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
        Logger::get()->error("Failed to bind socket: {}", strerror(errno));
        close(sockfd_);
        return;
    }

    Logger::get()->info("UDP Server started on {}:{}", config_.get_udp_ip(), config_.get_udp_port());
    running_ = true;

    // Запускаем пул потоков
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        workers_.emplace_back(&UDPServer::worker_thread, this);
    }

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
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            request_queue_.emplace(imsi, client_addr, addr_len);
        }
        queue_cond_.notify_one();
    }
}

void UDPServer::worker_thread() {
    while (running_) {
        std::tuple<std::string, struct sockaddr_in, socklen_t> request;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cond_.wait(lock, [this] { return !request_queue_.empty() || !running_; });
            if (!running_ && request_queue_.empty()) return;
            request = request_queue_.front();
            request_queue_.pop();
        }
        process_request(std::get<0>(request), std::get<1>(request), std::get<2>(request));
    }
}

void UDPServer::process_request(const std::string& imsi, const struct sockaddr_in& client_addr, socklen_t addr_len) {
    Logger::get()->info("Received IMSI: {}", imsi);

    std::regex imsi_regex("^[0-9]{15}$");
    if (!std::regex_match(imsi, imsi_regex)) {
        Logger::get()->warn("Invalid IMSI format: {}", imsi);
        sendto(sockfd_, "rejected", 8, 0, (struct sockaddr*)&client_addr, addr_len);
        return;
    }

    bool created = session_manager_.create_session(imsi);
    std::string response = created ? "created" : "rejected";
    Logger::get()->info("Sent response: {} for IMSI: {}", response, imsi);
    sendto(sockfd_, response.c_str(), response.size(), 0, (struct sockaddr*)&client_addr, addr_len);
}

void UDPServer::stop() {
    if (running_) {
        running_ = false;
        queue_cond_.notify_all(); // Разблокируем потоки
        if (sockfd_ >= 0) {
            struct sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            addr.sin_port = htons(config_.get_udp_port());
            sendto(sockfd_, "stop", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
            close(sockfd_);
            sockfd_ = -1;
        }
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
        Logger::get()->info("UDP Server stopped");
    }
}