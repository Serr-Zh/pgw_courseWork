#pragma once

#include "config.hpp"
#include <logger.hpp>
#include "cdr_logger.hpp"
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>

class SessionManager {
public:
    SessionManager(const Config& config, CDRLogger& cdr_logger);
    ~SessionManager();
    
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    bool create_session(const std::string& imsi);
    bool has_session(const std::string& imsi);
    void delete_session(const std::string& imsi);
    std::vector<std::string> get_active_sessions();
    void stop();

private:
    void cleanup_expired_sessions();
    
    struct Session {
        std::chrono::system_clock::time_point start_time;
    };
    
    const Config& config_;
    CDRLogger& cdr_logger_;
    std::unordered_map<std::string, Session> sessions_;
    std::mutex mutex_;
    std::thread cleanup_thread_;
    std::atomic<bool> running_;
};