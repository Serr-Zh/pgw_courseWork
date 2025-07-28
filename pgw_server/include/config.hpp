#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Класс для хранения конфигурации сервера, загружаемой из JSON
class Config {
public:
    // Конструктор: загружает конфигурацию из файла
    explicit Config(const std::string& config_file);

    // Геттеры для параметров конфигурации
    std::string get_udp_ip() const { return udp_ip; }
    int get_udp_port() const { return udp_port; }
    int get_session_timeout_sec() const { return session_timeout_sec; }
    std::string get_cdr_file() const { return cdr_file; }
    int get_http_port() const { return http_port; }
    int get_graceful_shutdown_rate() const { return graceful_shutdown_rate; }
    std::string get_log_file() const { return log_file; }
    std::string get_log_level() const { return log_level; }
    const std::vector<std::string>& get_blacklist() const { return blacklist; }

private:
    // Значения по умолчанию
    static constexpr const char* DEFAULT_UDP_IP = "0.0.0.0";
    static constexpr int DEFAULT_UDP_PORT = 9000;
    static constexpr int DEFAULT_SESSION_TIMEOUT = 30;
    static constexpr const char* DEFAULT_CDR_FILE = "cdr.log";
    static constexpr int DEFAULT_HTTP_PORT = 8080;
    static constexpr int DEFAULT_SHUTDOWN_RATE = 10;
    static constexpr const char* DEFAULT_LOG_FILE = "pgw.log";
    static constexpr const char* DEFAULT_LOG_LEVEL = "INFO";

    std::string udp_ip;
    int udp_port;
    int session_timeout_sec;
    std::string cdr_file;
    int http_port;
    int graceful_shutdown_rate;
    std::string log_file;
    std::string log_level;
    std::vector<std::string> blacklist;
};