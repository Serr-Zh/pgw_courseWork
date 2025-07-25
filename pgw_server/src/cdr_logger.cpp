#include "cdr_logger.hpp"
#include <chrono>
#include <iomanip>

CDRLogger::CDRLogger(const Config& config) {
    file_.open(config.get_cdr_file(), std::ios::app);
    if (!file_.is_open()) {
        Logger::get()->error("Failed to open CDR file: {}", config.get_cdr_file());
        throw std::runtime_error("Failed to open CDR file");
    }
    Logger::get()->info("CDRLogger initialized with file: {}", config.get_cdr_file());
}

CDRLogger::~CDRLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void CDRLogger::log(const std::string& imsi, const std::string& action) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "," << imsi << "," << action;

    std::lock_guard<std::mutex> lock(mutex_);
    file_ << ss.str() << std::endl;
    Logger::get()->info("CDR logged: {}", ss.str());
}