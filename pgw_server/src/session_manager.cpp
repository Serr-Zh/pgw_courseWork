#include "session_manager.hpp"
#include <algorithm>

SessionManager::SessionManager(const Config& config) : config_(config), running_(true) {
    // Запускаем поток для очистки устаревших сессий
    cleanup_thread_ = std::thread([this]() { cleanup_expired_sessions(); });
    Logger::get()->info("SessionManager initialized with timeout: {} seconds", config_.get_session_timeout_sec());
}

SessionManager::~SessionManager() {
    stop();
}

bool SessionManager::create_session(const std::string& imsi) {
    // Проверяем чёрный список
    const auto& blacklist = config_.get_blacklist();
    if (std::find(blacklist.begin(), blacklist.end(), imsi) != blacklist.end()) {
        Logger::get()->warn("Session creation rejected: IMSI {} is blacklisted", imsi);
        return false;
    }

    // Проверяем, существует ли сессия
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sessions_.find(imsi) != sessions_.end()) {
            Logger::get()->warn("Session already exists for IMSI: {}", imsi);
            return false;
        }

        // Создаём новую сессию
        sessions_[imsi] = Session{ std::chrono::system_clock::now() };
        Logger::get()->info("Session created for IMSI: {}", imsi);
    }
    return true;
}

bool SessionManager::has_session(const std::string& imsi) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(imsi) != sessions_.end();
}

void SessionManager::stop() {
    if (running_) {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sessions_.clear();
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