#include "client_config.hpp"
#include <fstream>
#include <stdexcept>

ClientConfig::ClientConfig(const std::string& config_file) {
    if (!std::filesystem::exists(config_file)) {
        throw std::runtime_error("Client config file does not exist: " + config_file);
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open client config file: " + config_file);
    }

    nlohmann::json json;
    file >> json;

    server_ip_ = json.value("server_ip", "127.0.0.1");
    server_port_ = json.value("server_port", 9000);
    log_file_ = json.value("log_file", "client.log");
    log_level_ = json.value("log_level", "INFO");
}