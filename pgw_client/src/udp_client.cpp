#include "udp_client.hpp"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex>

// Конструктор ClientSocket: создаёт UDP-сокет
ClientSocket::ClientSocket() : fd(socket(AF_INET, SOCK_DGRAM, 0)) {
    if (fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
}

// Деструктор ClientSocket: закрывает сокет
ClientSocket::~ClientSocket() {
    if (fd >= 0) {
        close(fd);
    }
}

// Устанавливает таймаут для сокета
void ClientSocket::set_timeout(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        throw std::runtime_error("Failed to set socket timeout: " + std::string(strerror(errno)));
    }
}

// Конструктор: инициализирует UDP-клиент
UDPClient::UDPClient(const ClientConfig& config, std::shared_ptr<ILogger> logger)
    : config(config), logger(logger) {
    logger->info("UDP Client initialized for server",
        config.get_server_ip() + ":" + std::to_string(config.get_server_port()));
}

// Деструктор: ничего не делает, сокет управляется RAII
UDPClient::~UDPClient() = default;

// Преобразует IMSI в BCD-кодировку (TS 29.274 §8.3)
std::string UDPClient::encode_bcd(const std::string& imsi) {
    if (!std::regex_match(imsi, std::regex("^[0-9]{15}$"))) {
        logger->error("Invalid IMSI format: {}", imsi);
        throw std::invalid_argument("Invalid IMSI format: must be 15 digits");
    }

    std::string bcd;
    for (size_t i = 0; i < imsi.size(); i += 2) {
        char byte = ((imsi[i] - '0') << 4);
        if (i + 1 < imsi.size()) {
            byte |= (imsi[i + 1] - '0');
        }
        else {
            byte |= 0xF; // Заполняем F для нечётной длины
        }
        bcd.push_back(byte);
    }
    return bcd;
}

// Декодирует ответ сервера
std::string UDPClient::decode_response(const char* buffer, ssize_t length) {
    std::string response(buffer, length);
    return response;
}

// Отправляет IMSI и получает ответ
bool UDPClient::send_imsi(const std::string& imsi, std::string& response, int retries, int timeout_ms) {
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config.get_server_ip().c_str());
    server_addr.sin_port = htons(config.get_server_port());

    socket.set_timeout(timeout_ms);
    std::string bcd_imsi = encode_bcd(imsi);

    for (int attempt = 0; attempt < retries; ++attempt) {
        logger->info("Sending IMSI",
            imsi + " (attempt " + std::to_string(attempt + 1) + "/" + std::to_string(retries) + ")");
        if (sendto(socket.get_fd(), bcd_imsi.c_str(), bcd_imsi.size(), 0,
            (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            logger->error("Failed to send IMSI: {}", strerror(errno));
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t n = recvfrom(socket.get_fd(), buffer, BUFFER_SIZE - 1, 0, nullptr, nullptr);
        if (n < 0) {
            logger->error("Failed to receive response: {}", strerror(errno));
            continue;
        }

        response = decode_response(buffer, n);
        logger->info("Received response", response + " for IMSI: " + imsi);
        return true;
    }

    logger->error("Failed to receive response after {} attempts", std::to_string(retries));
    return false;
}