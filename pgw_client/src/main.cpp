#include "client_config.hpp"
#include "udp_client.hpp"
#include "logger.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <imsi> [config_file]" << std::endl;
            return 1;
        }

        std::string imsi = argv[1];
        std::string config_path = (argc > 2) ? argv[2] : "client_config.json";

        ClientConfig config(config_path);
        Logger::init(config.get_log_file(), config.get_log_level());

        Logger::get()->info("Starting PGW Client...");
        UDPClient client(config);
        std::string response = client.send_imsi(imsi);

        std::cout << "Response: " << response << std::endl;

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