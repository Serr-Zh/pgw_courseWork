#include "config.hpp"
#include <logger.hpp>
#include "udp_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "http_server.hpp"
#include <iostream>
#include <thread>
#include <csignal>

void signal_handler(int) {
    // ������ �� ������, ������ ��������� main
}

int main(int argc, char* argv[]) {
    try {
        // ������������� ���������� ��������
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        std::string config_path = (argc > 1) ? argv[1] : "config.json";
        Config config(config_path);
        Logger::init(config.get_log_file(), config.get_log_level());

        Logger::get()->info("Starting PGW Server...");
        CDRLogger cdr_logger(config); // ������ CDRLogger ����� SessionManager
        SessionManager session_manager(config, cdr_logger);
        UDPServer udp_server(config, session_manager, cdr_logger);
        HTTPServer http_server(config, session_manager, cdr_logger, udp_server);

        // ��������� UDP-������ � ��������� ������
        std::thread udp_thread([&udp_server]() { udp_server.run(); });

        // ��������� HTTP-������
        http_server.run();

        // ��� ���������� UDP-�������
        if (udp_thread.joinable()) {
            udp_thread.join();
        }

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