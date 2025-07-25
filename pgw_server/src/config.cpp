#include "config.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>

Config::Config(const std::string& config_file) {
    // Проверяем существование файла
    if (!std::filesystem::exists(config_file)) {
        throw std::runtime_error("Config file does not exist: " + config_file);
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_file);
    }

    nlohmann::json json;
    file >> json;

    udp_ip_ = json.value("udp_ip", "0.0.0.0");
    udp_port_ = json.value("udp_port", 9000);
    session_timeout_sec_ = json.value("session_timeout_sec", 30);
    cdr_file_ = json.value("cdr_file", "cdr.log");
    http_port_ = json.value("http_port", 8080);
    graceful_shutdown_rate_ = json.value("graceful_shutdown_rate", 10);
    log_file_ = json.value("log_file", "pgw.log");
    log_level_ = json.value("log_level", "INFO");
    blacklist_ = json.value("blacklist", std::vector<std::string>{});
}