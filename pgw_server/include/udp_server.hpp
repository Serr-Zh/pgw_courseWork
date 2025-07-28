#pragma once
#include "config.hpp"
#include "interfaces.hpp"
#include "cdr_logger.hpp"
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>

// RAII-класс для управления UDP-сокетом
class Socket {
public:
    Socket();
    ~Socket();
    int get_fd() const { return fd; }
    void set_non_blocking();
    void bind(const std::string& ip, int port);

private:
    int fd;
};

// UDP-сервер для обработки запросов с IMSI
class UDPServer {
public:
    // Конструктор: принимает конфигурацию, менеджер сессий и логгер
    UDPServer(const Config& config, std::shared_ptr<ISessionManager> session_manager, std::shared_ptr<CDRLogger> cdr_logger);
    ~UDPServer();

    // Запускает сервер и пул потоков
    void run();

    // Останавливает сервер и потоки
    void stop();

private:
    // Обрабатывает запросы из очереди
    void worker_thread();

    // Обрабатывает запрос IMSI от клиента
    void process_request(const std::string& imsi, const struct sockaddr_in& client_addr, socklen_t addr_len);

    // Декодирует BCD-кодировку IMSI
    std::string decode_bcd(const char* buffer, ssize_t length);

    const Config& config;
    std::shared_ptr<ISessionManager> session_manager;
    std::shared_ptr<CDRLogger> cdr_logger;
    Socket socket;
    bool running;
    std::vector<std::thread> workers;
    std::queue<std::tuple<std::string, struct sockaddr_in, socklen_t>> request_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cond;
    static constexpr size_t NUM_THREADS = 4;
    static constexpr size_t BUFFER_SIZE = 256;
};