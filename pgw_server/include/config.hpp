#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <filesystem>

class Config {
public:
    explicit Config(const std::string& config_file);

    // Геттеры для доступа к настройкам
    std::string get_udp_ip() const { return udp_ip_; }
    int get_udp_port() const { return udp_port_; }
    int get_session_timeout_sec() const { return session_timeout_sec_; }
    std::string get_cdr_file() const { return cdr_file_; }
    int get_http_port() const { return http_port_; }
    int get_graceful_shutdown_rate() const { return graceful_shutdown_rate_; }
    std::string get_log_file() const { return log_file_; }
    std::string get_log_level() const { return log_level_; }
    const std::vector<std::string>& get_blacklist() const { return blacklist_; }

private:
    std::string udp_ip_;
    int udp_port_;
    int session_timeout_sec_;
    std::string cdr_file_;
    int http_port_;
    int graceful_shutdown_rate_;
    std::string log_file_;
    std::string log_level_;
    std::vector<std::string> blacklist_;
};