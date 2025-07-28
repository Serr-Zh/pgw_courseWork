#include "session_manager.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>

// Конструктор: инициализирует менеджер сессий
SessionManager::SessionManager(const Config& config, std::shared_ptr<CDRLogger> cdr_logger)
    : config(config), cdr_logger(cdr_logger), running(false) {
    std::stringstream ss;
    ss << "SessionManager initialized with timeout: " << config.get_session_timeout_sec();
    cdr_logger->get_logger()->info(ss.str());
}

// Деструктор: останавливает менеджер
SessionManager::~SessionManager() {
    stop();
}

// Запускает фоновую очистку сессий
void SessionManager::run() {
    running = true;
    cleanup_thread = std::thread([this]() {
        while (running) {
            cleanup_expired_sessions();
            std::this_thread::sleep_for(std::chrono::milliseconds(config.get_graceful_shutdown_rate()));
        }
        });
}

// Останавливает менеджер и очищает все сессии
void SessionManager::stop() {
    if (running) {
        running = false;
        if (cleanup_thread.joinable()) {
            cleanup_thread.join();
        }
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& session : sessions) {
            cdr_logger->log(session.first, "deleted");
            std::stringstream ss;
            ss << "Deleted session for IMSI: " << session.first;
            cdr_logger->get_logger()->info(ss.str());
            std::this_thread::sleep_for(std::chrono::milliseconds(config.get_graceful_shutdown_rate()));
        }
        sessions.clear();
        cdr_logger->get_logger()->info("SessionManager stopped");
        cdr_logger->get_logger()->flush();
    }
}

// Создаёт сессию для IMSI, если не в чёрном списке и не существует
bool SessionManager::create_session(const std::string& imsi) {
    if (std::find(config.get_blacklist().begin(), config.get_blacklist().end(), imsi) != config.get_blacklist().end()) {
        cdr_logger->get_logger()->info("Session creation rejected for IMSI (in blacklist)", imsi);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);
    if (sessions.find(imsi) != sessions.end()) {
        cdr_logger->log(imsi, "rejected: session already exists");
        cdr_logger->get_logger()->info("Session creation rejected for IMSI (already exists)", imsi);
        return false;
    }

    sessions[imsi] = Session{ std::chrono::system_clock::now() };
    cdr_logger->log(imsi, "created");
    cdr_logger->get_logger()->info("Session created for IMSI", imsi);
    return true;
}

// Проверяет наличие активной сессии
bool SessionManager::has_session(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(mutex);
    return sessions.find(imsi) != sessions.end();
}

// Удаляет истёкшие сессии
void SessionManager::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(mutex);
    auto now = std::chrono::system_clock::now();
    for (auto it = sessions.begin(); it != sessions.end();) {
        if (now - it->second.creation_time > std::chrono::seconds(config.get_session_timeout_sec())) {
            cdr_logger->log(it->first, "deleted");
            std::stringstream ss;
            ss << "Expired session deleted for IMSI: " << it->first;
            cdr_logger->get_logger()->info(ss.str());
            it = sessions.erase(it);
        }
        else {
            ++it;
        }
    }
}