#include "udp_client.hpp"
#include "client_config.hpp"
#include "logger.hpp"
#include <iostream>
#include <regex>

// ����� ����� �������
int main(int argc, char* argv[]) {
    try {
        // ��������� ��������� ��������� ������
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <imsi> [config_file]" << std::endl;
            return 1;
        }

        std::string imsi = argv[1];
        // ��������� ������ IMSI
        if (!std::regex_match(imsi, std::regex("^[0-9]{15}$"))) {
            std::cerr << "Error: IMSI must be 15 digits" << std::endl;
            return 1;
        }

        std::string config_path = (argc > 2) ? argv[2] : "client_config.json";
        ClientConfig config(config_path);

        // �������������� ������
        Logger::init(config.get_log_file(), config.get_log_level());
        auto logger = Logger::get();
        logger->info("Starting PGW Client...", ""); // ����� �������������� � std::string

        // ������ � ��������� ������
        UDPClient client(config, logger);
        std::string response;
        if (client.send_imsi(imsi, response)) {
            std::cout << "Response: " << response << std::endl;
            logger->info("Request sent for IMSI: {}", imsi);
            return 0;
        }
        else {
            std::cerr << "Error: Failed to receive response" << std::endl;
            logger->error("Client failed: Failed to receive response for IMSI: {}", imsi);
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (Logger::get()) {
            Logger::get()->error("Client failed: {}", e.what());
        }
        return 1;
    }

    return 0;
}