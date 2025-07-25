#include "logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::init(const std::string& log_file, const std::string& log_level) {
    // ������ �����: ������� � ���� � �������� (����. 5 ��, 3 �����)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file, 5 * 1024 * 1024, 3);

    // ������ ������ � ����������� �������
    logger_ = std::make_shared<spdlog::logger>("pgw_server", spdlog::sinks_init_list{ console_sink, file_sink });

    // ������������� ������� �����������
    if (log_level == "DEBUG") {
        logger_->set_level(spdlog::level::debug);
    }
    else if (log_level == "WARN") {
        logger_->set_level(spdlog::level::warn);
    }
    else if (log_level == "ERROR") {
        logger_->set_level(spdlog::level::err);
    }
    else if (log_level == "CRITICAL") {
        logger_->set_level(spdlog::level::critical);
    }
    else {
        logger_->set_level(spdlog::level::info); // �� ��������� INFO
    }

    // ������ �����: [�����] [�������] ���������
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    // ������������ ������ ��� ����������
    spdlog::set_default_logger(logger_);

    logger_->info("Logger initialized with file: {} and level: {}", log_file, log_level);
}