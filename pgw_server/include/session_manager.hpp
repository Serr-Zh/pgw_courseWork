#pragma once
#include "config.hpp"
#include "cdr_logger.hpp"
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

struct Session {
    std::chrono::system_clock::time_point creation_time;
};

class SessionManager {
public:
    SessionManager(const Config& config, CDRLogger& cdr_logger);
    ~SessionManager();
    void run();
    void stop();
    bool create_session(const std::string& imsi);
    bool has_session(const std::string& imsi);
    void cleanup_expired_sessions();

private:
    const Config& config_;
    CDRLogger& cdr_logger_;
    std::map<std::string, Session> sessions_;
    std::mutex mutex_;
    std::thread cleanup_thread_;
    bool running_;
};