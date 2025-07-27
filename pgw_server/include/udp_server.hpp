#pragma once
#include "config.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h> // Добавлено для socklen_t

class UDPServer {
public:
    UDPServer(const Config& config, SessionManager& session_manager, CDRLogger& cdr_logger);
    ~UDPServer();
    void run();
    void stop();

private:
    void worker_thread();
    void process_request(const std::string& imsi, const struct sockaddr_in& client_addr, socklen_t addr_len);

    const Config& config_;
    SessionManager& session_manager_;
    CDRLogger& cdr_logger_;
    int sockfd_;
    bool running_;
    std::vector<std::thread> workers_;
    std::queue<std::tuple<std::string, struct sockaddr_in, socklen_t>> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cond_;
    static const size_t NUM_THREADS = 4; // Количество потоков в пуле
};