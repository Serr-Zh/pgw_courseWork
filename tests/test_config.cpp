#include <gtest/gtest.h>
#include <logger.hpp>
#include "config.hpp"
#include <fstream>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаём временный конфигурационный файл
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
            "blacklist": ["001010123456789", "001010000000001"]
        })";
        config_file.close();

        Logger::init("test.log", "INFO");
    }

    void TearDown() override {
        std::remove("test_config.json");
        std::remove("test.log");
    }
};

TEST_F(ConfigTest, LoadConfig) {
    Config config("test_config.json");

    EXPECT_EQ(config.get_udp_ip(), "127.0.0.1");
    EXPECT_EQ(config.get_udp_port(), 9000);
    EXPECT_EQ(config.get_session_timeout_sec(), 30);
    EXPECT_EQ(config.get_cdr_file(), "test_cdr.log");
    EXPECT_EQ(config.get_http_port(), 8080);
    EXPECT_EQ(config.get_graceful_shutdown_rate(), 10);
    EXPECT_EQ(config.get_log_file(), "test.log");
    EXPECT_EQ(config.get_log_level(), "INFO");

    const auto& blacklist = config.get_blacklist();
    ASSERT_EQ(blacklist.size(), 2);
    EXPECT_EQ(blacklist[0], "001010123456789");
    EXPECT_EQ(blacklist[1], "001010000000001");
}