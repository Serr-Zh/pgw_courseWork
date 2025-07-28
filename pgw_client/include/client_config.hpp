#pragma once

#include <nlohmann/json.hpp>
#include <string>

// Класс для хранения конфигурации клиента, загружаемой из JSON
class ClientConfig {
public:
    // Конструктор: загружает конфигурацию из файла
    explicit ClientConfig(const std::string& config_file);

    // Геттеры для параметров конфигурации
    std::string get_server_ip() const { return server_ip; }
    int get_server_port() const { return server_port; }
    std::string get_log_file() const { return log_file; }
    std::string get_log_level() const { return log_level; }

private:
    // Значения по умолчанию
    static constexpr const char* DEFAULT_SERVER_IP = "127.0.0.1";
    static constexpr int DEFAULT_SERVER_PORT = 9000;
    static constexpr const char* DEFAULT_LOG_FILE = "client.log";
    static constexpr const char* DEFAULT_LOG_LEVEL = "INFO";

    std::string server_ip;
    int server_port;
    std::string log_file;
    std::string log_level;
};