#include "cdr_logger.hpp"
#include <chrono>
#include <iomanip>

CDRLogger::CDRLogger(const Config& config)
    : config_(config) {
    file_.open(config_.get_cdr_file(), std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open CDR log file: " + config_.get_cdr_file());
    }
    Logger::get()->info("CDRLogger initialized with file: {}", config_.get_cdr_file());
}

CDRLogger::~CDRLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void CDRLogger::log(const std::string& imsi, const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "," << imsi << "," << status;
    file_ << ss.str() << std::endl;
    file_.flush(); // —брасываем буфер
    Logger::get()->info("CDR logged: {}", ss.str());
}