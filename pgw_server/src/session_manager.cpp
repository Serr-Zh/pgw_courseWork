#include "session_manager.hpp"
#include <algorithm>

SessionManager::SessionManager(const Config& config, CDRLogger& cdr_logger)
    : config_(config), cdr_logger_(cdr_logger), running_(true) {
    Logger::get()->info("SessionManager initialized with timeout: {} seconds", config_.get_session_timeout_sec());

    cleanup_thread_ = std::thread([this]() { cleanup_expired_sessions(); });
}

SessionManager::~SessionManager() {
    stop();
}

bool SessionManager::create_session(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (sessions_.find(imsi) != sessions_.end()) {
        Logger::get()->info("Session already exists for IMSI: {}", imsi);
        return false;
    }

    const auto& blacklist = config_.get_blacklist();
    if (std::find(blacklist.begin(), blacklist.end(), imsi) != blacklist.end()) {
        Logger::get()->info("IMSI {} is in blacklist", imsi);
        return false;
    }

    sessions_[imsi] = { std::chrono::system_clock::now() };
    Logger::get()->info("Session created for IMSI: {}", imsi);
    return true;
}

bool SessionManager::has_session(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(imsi) != sessions_.end();
}

void SessionManager::delete_session(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.erase(imsi)) {
        Logger::get()->info("Session deleted for IMSI: {}", imsi);
        cdr_logger_.log(imsi, "deleted");
    }
}

std::vector<std::string> SessionManager::get_active_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> active_sessions;
    for (const auto& [imsi, session] : sessions_) {
        active_sessions.push_back(imsi);
    }
    return active_sessions;
}

void SessionManager::stop() {
    if (running_) {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        Logger::get()->info("SessionManager stopped");
    }
}

void SessionManager::cleanup_expired_sessions() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::system_clock::now();
            auto timeout = std::chrono::seconds(config_.get_session_timeout_sec());

            for (auto it = sessions_.begin(); it != sessions_.end();) {
                if (now - it->second.start_time > timeout) {
                    Logger::get()->info("Session expired for IMSI: {}", it->first);
                    cdr_logger_.log(it->first, "deleted");
                    it = sessions_.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}