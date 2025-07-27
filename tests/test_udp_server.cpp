#include <gtest/gtest.h>
#include <logger.hpp>
#include "udp_server.hpp"
#include "session_manager.hpp"
#include "cdr_logger.hpp"
#include "config.hpp"
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class UDPServerTest : public ::testing::Test {
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
            "blacklist": ["001010123456789"]
        })";
        config_file.close();

        Logger::init("test.log", "INFO");
        config_ = std::make_unique<Config>("test_config.json");
        cdr_logger_ = std::make_unique<CDRLogger>(*config_);
        session_manager_ = std::make_unique<SessionManager>(*config_, *cdr_logger_);
        udp_server_ = std::make_unique<UDPServer>(*config_, *session_manager_, *cdr_logger_);
    }

    void TearDown() override {
        udp_server_->stop();
        udp_server_.reset();
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
    std::unique_ptr<UDPServer> udp_server_;
};

TEST_F(UDPServerTest, MultiThreadedRequestHandling) {
    std::thread server_thread([this]() { udp_server_->run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(sockfd, 0);

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(19000);

    std::vector<std::thread> clients;
    const int num_clients = 10;
    char buffer[256];

    for (int i = 0; i < num_clients; ++i) {
        std::string imsi = "12345678901234" + std::to_string(i);
        clients.emplace_back([imsi, sockfd, &server_addr]() {
            sendto(sockfd, imsi.c_str(), imsi.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            char response[256];
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);
            ssize_t n = recvfrom(sockfd, response, sizeof(response) - 1, 0, (struct sockaddr*)&from, &from_len);
            if (n > 0) {
                response[n] = '\0';
                EXPECT_STREQ(response, "created");
            }
            });
    }

    for (auto& client : clients) {
        if (client.joinable()) {
            client.join();
        }
    }

    close(sockfd);
    udp_server_->stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    std::ifstream cdr_file("test_cdr.log");
    std::string line;
    int created_count = 0;
    while (std::getline(cdr_file, line)) {
        if (line.find(",created") != std::string::npos) {
            created_count++;
        }
    }
    EXPECT_EQ(created_count, num_clients);
}