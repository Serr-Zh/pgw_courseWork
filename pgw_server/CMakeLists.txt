add_executable(pgw_server
  src/main.cpp
  src/config.cpp
  src/logger.cpp
  src/udp_server.cpp
  src/session_manager.cpp
  src/cdr_logger.cpp
)

target_include_directories(pgw_server PRIVATE include)
target_link_libraries(pgw_server PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog httplib::httplib)