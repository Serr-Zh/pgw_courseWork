#include <gtest/gtest.h>
#include <logger.hpp>
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "config.hpp"
#include <thread>
#include <chrono>
#include <fstream>

class SessionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::ofstream config_file("test_config.json");
        config_file << R"({
            "udp_ip": "127.0.0.1",
            "udp_port": 19000,
            "session_timeout_sec": 2,
            "cdr_file": "test_cdr.log",
            "http_port": 8080,
            "graceful_shutdown_rate": 10,
            "log_file": "test.log",
            "log_level": "INFO",
            "blacklist": ["001010123456789"]
        })";
        config_file.close();

        Logger::init("test.log", "INFO");
        config_ = std::make_unique<Config>("test_config.json");
        cdr_logger_ = std::make_unique<CDRLogger>(*config_);
        session_manager_ = std::make_unique<SessionManager>(*config_, *cdr_logger_);
    }

    void TearDown() override {
        session_manager_->stop();
        session_manager_.reset();
        cdr_logger_.reset();
        config_.reset();
        std::remove("test_config.json");
        std::remove("test.log");
        std::remove("test_cdr.log");
    }

    std::unique_ptr<Config> config_;
    std::unique_ptr<CDRLogger> cdr_logger_;
    std::unique_ptr<SessionManager> session_manager_;
};

TEST_F(SessionManagerTest, CreateAndCheckSession) {
    std::string imsi = "123456789012345";
    EXPECT_TRUE(session_manager_->create_session(imsi));
    EXPECT_TRUE(session_manager_->has_session(imsi));

    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    bool found = false;
    while (std::getline(cdr_file, line)) {
        if (line.find(imsi + ",created") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(SessionManagerTest, BlacklistSession) {
    std::string imsi = "001010123456789";
    EXPECT_FALSE(session_manager_->create_session(imsi));
    EXPECT_FALSE(session_manager_->has_session(imsi));
}

TEST_F(SessionManagerTest, SessionExpiration) {
    std::string imsi = "123456789012345";
    session_manager_->create_session(imsi);
    session_manager_->run(); // Запускаем cleanup_thread_

    // Ждём 2.5 секунды, чтобы сессия точно истекла
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // Проверяем, что сессия удалена
    EXPECT_FALSE(session_manager_->has_session(imsi)) << "Session still exists for IMSI: " << imsi;

    // Проверяем, что в cdr.log есть запись deleted
    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    bool found_created = false;
    bool found_deleted = false;
    while (std::getline(cdr_file, line)) {
        if (line.find(imsi + ",created") != std::string::npos) {
            found_created = true;
        }
        if (line.find(imsi + ",deleted") != std::string::npos) {
            found_deleted = true;
        }
    }
    EXPECT_TRUE(found_created) << "No 'created' entry for IMSI: " << imsi;
    EXPECT_TRUE(found_deleted) << "No 'deleted' entry for IMSI: " << imsi;

    session_manager_->stop(); // Останавливаем cleanup_thread_
}