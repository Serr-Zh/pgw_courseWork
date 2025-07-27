#include <gtest/gtest.h>
#include <logger.hpp>
#include "udp_client.hpp"
#include "client_config.hpp"
#include "config.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "udp_server.hpp"
#include <thread>
#include <chrono>

class UDPClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Конфигурация сервера
        std::ofstream server_config_file("test_server_config.json");
        server_config_file << R"({
            "udp_ip": "127.0.0.1",
            "udp_port": 19000,
            "session_timeout_sec": 2,
            "cdr_file": "test_cdr.log",
            "http_port": 8080,
            "graceful_shutdown_rate": 10,
            "log_file": "test.log",
            "log_level": "INFO",
            "blacklist": []
        })";
        server_config_file.close();

        // Конфигурация клиента
        std::ofstream client_config_file("test_client_config.json");
        client_config_file << R"({
            "server_ip": "127.0.0.1",
            "server_port": 19000,
            "log_file": "test_client.log",
            "log_level": "INFO"
        })";
        client_config_file.close();

        Logger::init("test.log", "INFO");
        server_config_ = std::make_unique<Config>("test_server_config.json");
        cdr_logger_ = std::make_unique<CDRLogger>(*server_config_);
        session_manager_ = std::make_unique<SessionManager>(*server_config_, *cdr_logger_);
        udp_server_ = std::make_unique<UDPServer>(*server_config_, *session_manager_, *cdr_logger_);
        client_config_ = std::make_unique<ClientConfig>("test_client_config.json");
        udp_client_ = std::make_unique<UDPClient>(*client_config_);

        // Запускаем сервер
        server_thread_ = std::thread([this]() { udp_server_->run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        udp_server_->stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        udp_client_.reset();
        client_config_.reset();
        udp_server_.reset();
        session_manager_.reset();
        cdr_logger_.reset();
        server_config_.reset();
        std::remove("test_server_config.json");
        std::remove("test_client_config.json");
        std::remove("test.log");
        std::remove("test_client.log");
        std::remove("test_cdr.log");
    }

    std::unique_ptr<Config> server_config_;
    std::unique_ptr<CDRLogger> cdr_logger_;
    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<UDPServer> udp_server_;
    std::unique_ptr<ClientConfig> client_config_;
    std::unique_ptr<UDPClient> udp_client_;
    std::thread server_thread_;
};

TEST_F(UDPClientTest, SendValidIMSI) {
    std::string imsi = "123456789012345";
    std::string response;
    bool success = udp_client_->send_imsi(imsi, response);
    EXPECT_TRUE(success);
    EXPECT_EQ(response, "created");
}