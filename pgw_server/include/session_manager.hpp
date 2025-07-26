#pragma once
#include "config.hpp"
#include "cdr_logger.hpp"
#include <map>
#include <mutex>
#include <string>
#include <chrono>
#include <thread>

class SessionManager {
public:
    struct Session {
        std::chrono::system_clock::time_point creation_time;
    };

    SessionManager(const Config& config, CDRLogger& cdr_logger);
    ~SessionManager();
    void run(); // Добавляем декларацию метода run
    void stop();
    bool create_session(const std::string& imsi);
    bool has_session(const std::string& imsi);
private:
    void cleanup_expired_sessions();
    
    const Config& config_;
    CDRLogger& cdr_logger_;
    std::map<std::string, Session> sessions_;
    std::mutex mutex_;
    bool running_;
    std::thread cleanup_thread_;
};