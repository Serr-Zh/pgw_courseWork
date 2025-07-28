#include <gtest/gtest.h>
#include <logger.hpp>
#include "cdr_logger.hpp"
#include "config.hpp"
#include <fstream>

class CDRLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::ofstream config_file("test_config.json");
        config_file << R"({
            "udp_ip": "127.0.0.1",
            "udp_port": 9000,
            "session_timeout_sec": 30,
            "cdr_file": "test_cdr.log",
            "http_port": 8080,
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
    }

    void TearDown() override {
        cdr_logger_.reset();
        config_.reset();
        logger_.reset();
        std::remove("test_config.json");
        std::remove("test.log");
        std::remove("test_cdr.log");
    }

    std::shared_ptr<Config> config_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<CDRLogger> cdr_logger_;
};

TEST_F(CDRLoggerTest, LogCreated) {
    std::string imsi = "123456789012345";
    cdr_logger_->log(imsi, "created");

    std::ifstream file("test_cdr.log");
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find(imsi + ",created") != std::string::npos);
}

TEST_F(CDRLoggerTest, LogDeleted) {
    std::string imsi = "123456789012345";
    cdr_logger_->log(imsi, "deleted");

    std::ifstream file("test_cdr.log");
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find(imsi + ",deleted") != std::string::npos);
}