#include <gtest/gtest.h>
#include <logger.hpp>
#include "http_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "udp_server.hpp"
#include "config.hpp"
#include <httplib.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <set>
#include <atomic>
#include <iostream>

class HTTPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::ofstream config_file("test_config.json");
        config_file << R"({
            "udp_ip": "127.0.0.1",
            "udp_port": 19000,
            "session_timeout_sec": 2,
            "cdr_file": "test_cdr.log",
            "http_port": 18080,
            "graceful_shutdown_rate": 10,
            "log_file": "test.log",
            "log_level": "INFO",
            "blacklist": []
        })";
        config_file.close();

        Logger::init("test.log", "INFO");
        config_ = std::make_shared<Config>("test_config.json");
        logger_ = Logger::get();
        cdr_logger_ = std::make_shared<CDRLogger>(*config_, logger_);
        session_manager_ = std::make_shared<SessionManager>(*config_, cdr_logger_);
        udp_server_ = std::make_shared<UDPServer>(*config_, session_manager_, cdr_logger_);
        http_server_ = std::make_shared<HTTPServer>(*config_, logger_, session_manager_, [this]() { udp_server_->stop(); }, running_);

        // Запускаем серверы
        session_thread_ = std::thread([this]() { session_manager_->run(); });
        udp_thread_ = std::thread([this]() { udp_server_->run(); });
        http_thread_ = std::thread([this]() { http_server_->run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        http_server_->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Увеличиваем задержку
        if (session_thread_.joinable()) {
            session_thread_.join();
        }
        if (udp_thread_.joinable()) {
            udp_thread_.join();
        }
        if (http_thread_.joinable()) {
            http_thread_.join();
        }
        http_server_.reset();
        udp_server_.reset();
        session_manager_.reset();
        cdr_logger_.reset();
        config_.reset();
        logger_.reset();
        std::remove("test_config.json");
        std::remove("test_cdr.log");
        // Не удаляем test.log сразу, чтобы проверить его содержимое
    }

    std::atomic<bool> running_{ true };
    std::shared_ptr<Config> config_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<CDRLogger> cdr_logger_;
    std::shared_ptr<SessionManager> session_manager_;
    std::shared_ptr<UDPServer> udp_server_;
    std::shared_ptr<HTTPServer> http_server_;
    std::thread session_thread_;
    std::thread udp_thread_;
    std::thread http_thread_;
};

TEST_F(HTTPServerTest, CheckSubscriberActive) {
    session_manager_->create_session("123456789012345");

    httplib::Client cli("127.0.0.1", 18080);
    auto res = cli.Get("/check_subscriber?imsi=123456789012345");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "active");
}

TEST_F(HTTPServerTest, CheckSubscriberNotActive) {
    httplib::Client cli("127.0.0.1", 18080);
    auto res = cli.Get("/check_subscriber?imsi=999999999999999");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "not active");
}

TEST_F(HTTPServerTest, StopServer) {
    httplib::Client cli("127.0.0.1", 18080);
    auto res = cli.Get("/stop");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "Stopping server...");

    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Увеличиваем задержку

    auto res_after = cli.Get("/check_subscriber?imsi=123456789012345");
    EXPECT_TRUE(res_after == nullptr); // Сервер должен быть остановлен

    // Проверяем содержимое test.log
    std::ifstream log_file("test.log");
    std::string log_content;
    std::string line;
    while (std::getline(log_file, line)) {
        log_content += line + "\n";
    }
    log_file.close();
    std::cout << "test.log content:\n" << log_content << std::endl;

    // Проверяем, что сервер корректно остановился
    log_file.open("test.log");
    bool found_stop = false;
    bool found_session_manager_stop = false;
    while (std::getline(log_file, line)) {
        if (line.find("HTTP Server stopped") != std::string::npos) {
            found_stop = true;
        }
        if (line.find("SessionManager stopped") != std::string::npos) {
            found_session_manager_stop = true;
        }
    }
    EXPECT_TRUE(found_stop) << "HTTP Server stop not logged";
    EXPECT_TRUE(found_session_manager_stop) << "SessionManager stop not logged";

    // Удаляем test.log после проверки
    std::remove("test.log");
}

TEST_F(HTTPServerTest, StopWithActiveSessions) {
    const int num_sessions = 10;
    std::vector<std::string> imsies(num_sessions);

    // Создаём 10 сессий
    for (int i = 0; i < num_sessions; ++i) {
        imsies[i] = "123456789" + std::to_string(300000 + i);
        session_manager_->create_session(imsies[i]);
    }

    httplib::Client cli("127.0.0.1", 18080);
    auto res = cli.Get("/stop");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "Stopping server...");

    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Увеличиваем задержку

    // Проверяем, что все сессии удалены
    for (const auto& imsi : imsies) {
        EXPECT_FALSE(session_manager_->has_session(imsi)) << "Session not deleted for IMSI: " << imsi;
    }

    // Проверяем, что в cdr.log есть записи deleted
    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    std::set<std::string> deleted_imsies;
    while (std::getline(cdr_file, line)) {
        for (const auto& imsi : imsies) {
            if (line.find(imsi + ",deleted") != std::string::npos) {
                deleted_imsies.insert(imsi);
            }
        }
    }
    EXPECT_EQ(deleted_imsies.size(), num_sessions) << "Not all IMSI have 'deleted' in cdr.log";
}