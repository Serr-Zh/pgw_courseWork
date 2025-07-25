#include "config.hpp"
#include "logger.hpp"
#include "udp_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        std::string config_path = (argc > 1) ? argv[1] : "config.json";
        Config config(config_path);
        Logger::init(config.get_log_file(), config.get_log_level());

        Logger::get()->info("Starting PGW Server...");
        SessionManager session_manager(config);
        CDRLogger cdr_logger(config);
        UDPServer server(config, session_manager, cdr_logger);
        server.run();

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (Logger::get()) {
            Logger::get()->error("Server failed: {}", e.what());
        }
        return 1;
    }

    return 0;
}