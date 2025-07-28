#pragma once

#include "interfaces.hpp" // Включаем interfaces.hpp из pgw_server/include
#include "client_config.hpp"
#include <string>
#include <netinet/in.h>
#include <memory>

// RAII-класс для управления UDP-сокетом
class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();
    int get_fd() const { return fd; }
    void set_timeout(int timeout_ms);

private:
    int fd;
};

// Класс для отправки UDP-запросов с IMSI на сервер
class UDPClient {
public:
    // Конструктор: принимает конфигурацию и логгер
    UDPClient(const ClientConfig& config, std::shared_ptr<ILogger> logger);
    ~UDPClient();

    // Запрещаем копирование
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;

    // Отправляет IMSI в BCD-кодировке и получает ответ
    bool send_imsi(const std::string& imsi, std::string& response, int retries = 3, int timeout_ms = 1000);

private:
    // Преобразует IMSI в BCD-кодировку (TS 29.274 §8.3)
    std::string encode_bcd(const std::string& imsi);

    // Декодирует ответ сервера
    std::string decode_response(const char* buffer, ssize_t length);

    const ClientConfig& config;
    std::shared_ptr<ILogger> logger;
    ClientSocket socket;
    static constexpr size_t BUFFER_SIZE = 256;
    static constexpr int DEFAULT_RETRIES = 3;
    static constexpr int DEFAULT_TIMEOUT_MS = 1000;
};