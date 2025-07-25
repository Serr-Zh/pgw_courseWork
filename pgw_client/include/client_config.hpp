#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>

class ClientConfig {
public:
    explicit ClientConfig(const std::string& config_file);
    
    std::string get_server_ip() const { return server_ip_; }
    int get_server_port() const { return server_port_; }
    std::string get_log_file() const { return log_file_; }
    std::string get_log_level() const { return log_level_; }

private:
    std::string server_ip_;
    int server_port_;
    std::string log_file_;
    std::string log_level_;
};