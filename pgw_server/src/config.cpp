#include "config.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>

// Конструктор: загружает и парсит JSON-конфигурацию
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
    try {
        file >> json;
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
    }

    // Проверяем типы и устанавливаем значения
    if (json.contains("udp_ip") && json["udp_ip"].is_string()) {
        udp_ip = json["udp_ip"];
    }
    else {
        udp_ip = DEFAULT_UDP_IP;
    }
    if (json.contains("udp_port") && json["udp_port"].is_number_integer()) {
        udp_port = json["udp_port"];
    }
    else {
        udp_port = DEFAULT_UDP_PORT;
    }
    if (json.contains("session_timeout_sec") && json["session_timeout_sec"].is_number_integer()) {
        session_timeout_sec = json["session_timeout_sec"];
    }
    else {
        session_timeout_sec = DEFAULT_SESSION_TIMEOUT;
    }
    if (json.contains("cdr_file") && json["cdr_file"].is_string()) {
        cdr_file = json["cdr_file"];
    }
    else {
        cdr_file = DEFAULT_CDR_FILE;
    }
    if (json.contains("http_port") && json["http_port"].is_number_integer()) {
        http_port = json["http_port"];
    }
    else {
        http_port = DEFAULT_HTTP_PORT;
    }
    if (json.contains("graceful_shutdown_rate") && json["graceful_shutdown_rate"].is_number_integer()) {
        graceful_shutdown_rate = json["graceful_shutdown_rate"];
    }
    else {
        graceful_shutdown_rate = DEFAULT_SHUTDOWN_RATE;
    }
    if (json.contains("log_file") && json["log_file"].is_string()) {
        log_file = json["log_file"];
    }
    else {
        log_file = DEFAULT_LOG_FILE;
    }
    if (json.contains("log_level") && json["log_level"].is_string()) {
        log_level = json["log_level"];
    }
    else {
        log_level = DEFAULT_LOG_LEVEL;
    }
    if (json.contains("blacklist") && json["blacklist"].is_array()) {
        for (const auto& item : json["blacklist"]) {
            if (item.is_string()) {
                blacklist.push_back(item);
            }
        }
    }
}