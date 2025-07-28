#include "config.hpp"
#include "logger.hpp"
#include "udp_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "http_server.hpp"
#include <iostream>
#include <thread>
#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Глобальный флаг для управления завершением
std::atomic<bool> running(true);

// Обработчик сигналов SIGINT/SIGTERM
void signal_handler(int) {
    running = false;
}

// Точка входа приложения
int main(int argc, char* argv[]) {
    try {
        // Устанавливаем обработчики сигналов
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Загружаем конфигурацию
        std::string config_path = (argc > 1) ? argv[1] : "config.json";
        Config config(config_path);

        // Проверяем, заняты ли порты
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Error: Failed to create socket for port check" << std::endl;
            return 1;
        }
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(config.get_http_port());
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Error: HTTP port " << config.get_http_port() << " already in use" << std::endl;
            close(sock);
            return 1;
        }
        close(sock);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Error: Failed to create socket for UDP port check" << std::endl;
            return 1;
        }
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        addr.sin_port = htons(config.get_udp_port());
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Error: UDP port " << config.get_udp_port() << " already in use" << std::endl;
            close(sock);
            return 1;
        }
        close(sock);

        // Инициализируем логгер
        Logger::init(config.get_log_file(), config.get_log_level());
        auto logger = Logger::get();
        logger->info("Starting PGW Server...", "");

        // Создаём компоненты
        auto cdr_logger = std::make_shared<CDRLogger>(config, logger);
        auto session_manager = std::make_shared<SessionManager>(config, cdr_logger);
        auto udp_server = std::make_shared<UDPServer>(config, session_manager, cdr_logger);
        HTTPServer http_server(config, logger, session_manager, [udp_server]() { udp_server->stop(); }, running);

        // Запускаем компоненты в отдельных потоках
        std::thread session_thread([&session_manager]() { session_manager->run(); });
        std::thread udp_thread([&udp_server]() { udp_server->run(); });
        std::thread http_thread([&http_server]() { http_server.run(); });

        // Ожидаем сигнала завершения
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Останавливаем компоненты
        http_server.stop();
        if (udp_thread.joinable()) {
            udp_thread.join();
        }
        if (session_thread.joinable()) {
            session_thread.join();
        }
        if (http_thread.joinable()) {
            http_thread.join();
        }

        logger->info("PGW Server stopped", "");
        logger->flush();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (Logger::get()) {
            Logger::get()->error("Server failed: {}", e.what());
            Logger::get()->flush();
        }
        return 1;
    }

    return 0;
}