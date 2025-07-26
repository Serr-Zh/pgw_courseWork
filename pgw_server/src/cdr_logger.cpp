#include "cdr_logger.hpp"
#include <chrono>
#include <iomanip>

CDRLogger::CDRLogger(const Config& config) : config_(config) {
    file_.open(config_.get_cdr_file(), std::ios::app);
    if (!file_.is_open()) {
        Logger::get()->error("Failed to open CDR file: {}", config_.get_cdr_file());
    }
    else {
        Logger::get()->info("CDRLogger initialized with file: {}", config_.get_cdr_file());
    }
}

CDRLogger::~CDRLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void CDRLogger::log(const std::string& imsi, const std::string& status) {
    if (!file_.is_open()) {
        Logger::get()->error("CDR file is not open");
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "," << imsi << "," << status;

    std::lock_guard<std::mutex> lock(mutex_);
    file_ << ss.str() << "\n";
    file_.flush(); // —брасываем данные на диск
    Logger::get()->info("CDR logged: {}", ss.str());
}