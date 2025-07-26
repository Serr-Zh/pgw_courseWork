#include <gtest/gtest.h>
#include <logger.hpp>
#include <cstring>
#include <regex>
#include "udp_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "config.hpp"
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <set>

class UDPServerTest : public ::testing::Test {
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
            "blacklist": ["001010123456789", "001010123456790", "001010123456791"]
        })";
        config_file.close();

        Logger::init("test.log", "INFO");
        config_ = std::make_unique<Config>("test_config.json");
        cdr_logger_ = std::make_unique<CDRLogger>(*config_);
        session_manager_ = std::make_unique<SessionManager>(*config_, *cdr_logger_);
        udp_server_ = std::make_unique<UDPServer>(*config_, *session_manager_, *cdr_logger_);

        server_thread_ = std::thread([this]() { udp_server_->run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        udp_server_->stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        session_manager_->stop();
        udp_server_.reset();
        session_manager_.reset();
        cdr_logger_.reset();
        config_.reset();
        std::remove("test_config.json");
        std::remove("test.log");
        std::remove("test_cdr.log");
    }

    std::string send_udp_message(const std::string& message) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            Logger::get()->error("Failed to create socket: {}", strerror(errno));
            return "";
        }

        struct sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(19000);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        char buffer[256];
        struct timeval tv = { 2, 0 };
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
        close(sockfd);

        if (n < 0) {
            Logger::get()->error("Failed to receive response: {}", strerror(errno));
            return "";
        }
        buffer[n] = '\0';
        return std::string(buffer);
    }

    std::unique_ptr<Config> config_;
    std::unique_ptr<CDRLogger> cdr_logger_;
    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<UDPServer> udp_server_;
    std::thread server_thread_;
};

TEST_F(UDPServerTest, SendValidIMSI) {
    std::string imsi = "123456789012345";
    std::string response = send_udp_message(imsi);
    EXPECT_EQ(response, "created");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    bool found = false;
    while (std::getline(cdr_file, line)) {
        if (line.find(imsi + ",created") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "No 'created' entry for IMSI: " << imsi;
}

TEST_F(UDPServerTest, SendBlacklistedIMSI) {
    std::string imsi = "001010123456789";
    std::string response = send_udp_message(imsi);
    EXPECT_EQ(response, "rejected");
}

TEST_F(UDPServerTest, InvalidIMSI) {
    std::vector<std::string> invalid_imsies = {
        "", // Пустой IMSI
        "123", // Короткий IMSI
        "12345678901234567890", // Длинный IMSI
        "abcde12345" // Нечисловой IMSI
    };

    for (const auto& imsi : invalid_imsies) {
        std::string response = send_udp_message(imsi);
        EXPECT_EQ(response, "rejected") << "Failed for invalid IMSI: " << imsi;
    }
}

TEST_F(UDPServerTest, Handle1000Users) {
    const int num_users = 100;
    std::vector<std::string> imsies(num_users);
    std::vector<std::thread> clients;

    // Генерируем 100 уникальных IMSI и отправляем параллельно
    for (int i = 0; i < num_users; ++i) {
        imsies[i] = "123456789" + std::to_string(100000 + i);
        clients.emplace_back([this, imsi = imsies[i]]() {
            std::string response = send_udp_message(imsi);
            EXPECT_EQ(response, "created") << "Failed for IMSI: " << imsi;
            });
    }

    for (auto& client : clients) {
        if (client.joinable()) {
            client.join();
        }
    }

    // Проверяем, что все сессии созданы
    for (const auto& imsi : imsies) {
        EXPECT_TRUE(session_manager_->has_session(imsi)) << "Session missing for IMSI: " << imsi;
    }

    // Проверяем cdr.log
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    std::set<std::string> logged_imsies;
    while (std::getline(cdr_file, line)) {
        for (const auto& imsi : imsies) {
            if (line.find(imsi + ",created") != std::string::npos) {
                logged_imsies.insert(imsi);
            }
        }
    }
    EXPECT_EQ(logged_imsies.size(), num_users) << "Not all IMSI logged in cdr.log";
}

TEST_F(UDPServerTest, ConcurrentRequests) {
    const int num_clients = 50;
    std::vector<std::thread> clients;
    std::vector<std::string> imsies(num_clients);

    for (int i = 0; i < num_clients; ++i) {
        imsies[i] = "123456789" + std::to_string(200000 + i);
        clients.emplace_back([this, imsi = imsies[i]]() {
            std::string response = send_udp_message(imsi);
            EXPECT_EQ(response, "created") << "Failed for IMSI: " << imsi;
            });
    }

    for (auto& client : clients) {
        if (client.joinable()) {
            client.join();
        }
    }

    // Проверяем, что все сессии созданы
    for (const auto& imsi : imsies) {
        EXPECT_TRUE(session_manager_->has_session(imsi)) << "Session missing for IMSI: " << imsi;
    }
}

TEST_F(UDPServerTest, MultipleBlacklistedIMSIs) {
    std::vector<std::string> blacklisted_imsies = {
        "001010123456789",
        "001010123456790",
        "001010123456791"
    };

    for (const auto& imsi : blacklisted_imsies) {
        std::string response = send_udp_message(imsi);
        EXPECT_EQ(response, "rejected") << "Failed for blacklisted IMSI: " << imsi;
    }
}