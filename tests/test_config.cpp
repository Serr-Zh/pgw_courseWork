#include "config.hpp"
#include "logger.hpp"
#include <gtest/gtest.h>
#include <fstream>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаём временный config.json для тестов
        std::ofstream config_file("test_config.json");
        config_file << R"({
            "udp_ip": "0.0.0.0",
            "udp_port": 9000,
            "session_timeout_sec": 30,
            "cdr_file": "cdr.log",
            "http_port": 8080,
            "graceful_shutdown_rate": 10,
            "log_file": "pgw.log",
            "log_level": "INFO",
            "blacklist": [
                "001010123456789",
                "001010000000001"
            ]
        })";
        config_file.close();
    }

    void TearDown() override {
        // Удаляем временный файл
        std::filesystem::remove("test_config.json");
    }
};

TEST_F(ConfigTest, LoadConfig) {
    Config config("test_config.json");
    EXPECT_EQ(config.get_udp_ip(), "0.0.0.0");
    EXPECT_EQ(config.get_udp_port(), 9000);
    EXPECT_EQ(config.get_session_timeout_sec(), 30);
    EXPECT_EQ(config.get_cdr_file(), "cdr.log");
    EXPECT_EQ(config.get_http_port(), 8080);
    EXPECT_EQ(config.get_graceful_shutdown_rate(), 10);
    EXPECT_EQ(config.get_log_file(), "pgw.log");
    EXPECT_EQ(config.get_log_level(), "INFO");
    EXPECT_EQ(config.get_blacklist().size(), 2);
    EXPECT_EQ(config.get_blacklist()[0], "001010123456789");
}

TEST_F(ConfigTest, LoadNonExistentConfig) {
    EXPECT_THROW(Config("nonexistent.json"), std::runtime_error);
}

TEST(LoggerTest, InitializeLogger) {
    Logger::init("test.log", "INFO");
    auto logger = Logger::get();
    ASSERT_NE(logger, nullptr);
    logger->info("Test log message");
    logger->debug("This should not appear in INFO level");
}