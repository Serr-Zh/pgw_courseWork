#include "udp_server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex>
#include <sstream>

// ����������� Socket: ������ UDP-�����
Socket::Socket() : fd(socket(AF_INET, SOCK_DGRAM, 0)) {
    if (fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    // ������������� SO_REUSEADDR
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }
}

// ���������� Socket: ��������� �����
Socket::~Socket() {
    if (fd >= 0) {
        close(fd);
    }
}

// ������������� ����� � ������������� �����
void Socket::set_non_blocking() {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to set socket to non-blocking: " + std::string(strerror(errno)));
    }
}

// ����������� ����� � IP � �����
void Socket::bind(const std::string& ip, int port) {
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(port);

    if (::bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}

// �����������: �������������� UDP-������
UDPServer::UDPServer(const Config& config, std::shared_ptr<ISessionManager> session_manager, std::shared_ptr<CDRLogger> cdr_logger)
    : config(config), session_manager(session_manager), cdr_logger(cdr_logger), running(false) {
    std::stringstream ss;
    ss << "Initializing UDP Server on " << config.get_udp_ip() << ":" << config.get_udp_port();
    cdr_logger->get_logger()->info(ss.str());
}

// ����������: ������������� ������
UDPServer::~UDPServer() {
    stop();
}

// ���������� BCD-��������� IMSI
std::string UDPServer::decode_bcd(const char* buffer, ssize_t length) {
    std::string imsi;
    for (ssize_t i = 0; i < length; ++i) {
        char high = (buffer[i] >> 4) & 0xF;
        char low = buffer[i] & 0xF;
        if (high != 0xF) imsi.push_back(high + '0');
        if (low != 0xF) imsi.push_back(low + '0');
    }
    cdr_logger->get_logger()->info("Decoded IMSI", imsi);
    return imsi;
}

// ��������� ������ � ��� �������
void UDPServer::run() {
    socket.set_non_blocking();
    socket.bind(config.get_udp_ip(), config.get_udp_port());
    std::stringstream ss;
    ss << "UDP Server started on " << config.get_udp_ip() << ":" << config.get_udp_port();
    cdr_logger->get_logger()->info(ss.str());
    running = true;

    // ��������� ��� ������� �������
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(&UDPServer::worker_thread, this);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // �������� ���� ����� UDP-��������
    while (running) {
        ssize_t n = recvfrom(socket.get_fd(), buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (running) {
                cdr_logger->get_logger()->error("Failed to receive data", strerror(errno));
            }
            continue;
        }

        buffer[n] = '\0'; // ��������� ������
        std::string imsi = decode_bcd(buffer, n);
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            request_queue.emplace(imsi, client_addr, addr_len);
            cdr_logger->get_logger()->info("Enqueued IMSI", imsi);
        }
        queue_cond.notify_one();
    }
}

// ������������ ������� �� �������
void UDPServer::worker_thread() {
    while (running) {
        std::tuple<std::string, struct sockaddr_in, socklen_t> request;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cond.wait(lock, [this] { return !request_queue.empty() || !running; });
            if (!running && request_queue.empty()) return;
            request = request_queue.front();
            request_queue.pop();
        }
        process_request(std::get<0>(request), std::get<1>(request), std::get<2>(request));
    }
}

// ������������ ������ IMSI
void UDPServer::process_request(const std::string& imsi, const struct sockaddr_in& client_addr, socklen_t addr_len) {
    std::regex imsi_regex("^[0-9]{15}$");
    if (!std::regex_match(imsi, imsi_regex)) {
        cdr_logger->get_logger()->info("Invalid IMSI format", imsi);
        sendto(socket.get_fd(), "rejected", strlen("rejected"), 0, (struct sockaddr*)&client_addr, addr_len);
        return;
    }

    bool created = session_manager->create_session(imsi);
    std::string response = created ? "created" : "rejected";
    std::stringstream ss;
    ss << "Processed IMSI: " << imsi << ", response: " << response;
    cdr_logger->get_logger()->info(ss.str());
    sendto(socket.get_fd(), response.c_str(), response.size(), 0, (struct sockaddr*)&client_addr, addr_len);
}

// ������������� ������ � ������
void UDPServer::stop() {
    if (running) {
        running = false;
        queue_cond.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers.clear();
        cdr_logger->get_logger()->info("UDP Server stopped");
        cdr_logger->get_logger()->flush();
    }
}