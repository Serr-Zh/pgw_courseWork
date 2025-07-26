#include "session_manager.hpp"
#include <thread>
#include <chrono>

SessionManager::SessionManager(const Config& config, CDRLogger& cdr_logger)
    : config_(config), cdr_logger_(cdr_logger), running_(false) {
    Logger::get()->info("SessionManager initialized with timeout: {} seconds", config_.get_session_timeout_sec());
}

SessionManager::~SessionManager() {
    stop();
}

void SessionManager::run() {
    running_ = true;
    cleanup_thread_ = std::thread([this]() {
        while (running_) {
            cleanup_expired_sessions();
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.get_graceful_shutdown_rate()));
        }
        });
}

void SessionManager::stop() {
    if (running_) {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& session : sessions_) {
                cdr_logger_.log(session.first, "deleted");
            }
            sessions_.clear();
        }
        Logger::get()->info("SessionManager stopped");
        Logger::get()->flush();
    }
}

bool SessionManager::create_session(const std::string& imsi) {
    if (std::find(config_.get_blacklist().begin(), config_.get_blacklist().end(), imsi) != config_.get_blacklist().end()) {
        Logger::get()->info("IMSI {} is in blacklist", imsi);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.find(imsi) != sessions_.end()) {
        Logger::get()->info("Session already exists for IMSI: {}", imsi);
        return false;
    }

    sessions_[imsi] = Session{ std::chrono::system_clock::now() };
    Logger::get()->info("Session created for IMSI: {}", imsi);
    cdr_logger_.log(imsi, "created");
    return true;
}

bool SessionManager::has_session(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(imsi) != sessions_.end();
}

void SessionManager::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (now - it->second.creation_time > std::chrono::seconds(config_.get_session_timeout_sec())) {
            Logger::get()->info("Session expired for IMSI: {}", it->first);
            cdr_logger_.log(it->first, "deleted");
            it = sessions_.erase(it);
        }
        else {
            ++it;
        }
    }
}