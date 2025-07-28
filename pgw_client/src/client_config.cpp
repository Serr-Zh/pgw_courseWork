#include "client_config.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>

// Конструктор: загружает и парсит JSON-конфигурацию
ClientConfig::ClientConfig(const std::string& config_file) {
    // Проверяем существование файла
    if (!std::filesystem::exists(config_file)) {
        throw std::runtime_error("Client config file does not exist: " + config_file);
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open client config file: " + config_file);
    }

    nlohmann::json json;
    try {
        file >> json;
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse client config file: " + std::string(e.what()));
    }

    // Проверяем типы и устанавливаем значения
    if (json.contains("server_ip") && json["server_ip"].is_string()) {
        server_ip = json["server_ip"];
    }
    else {
        server_ip = DEFAULT_SERVER_IP;
    }
    if (json.contains("server_port") && json["server_port"].is_number_integer()) {
        server_port = json["server_port"];
    }
    else {
        server_port = DEFAULT_SERVER_PORT;
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
}