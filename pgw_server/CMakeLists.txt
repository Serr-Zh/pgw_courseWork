add_executable(pgw_server
  src/main.cpp
  src/config.cpp
  src/udp_server.cpp
  src/session_manager.cpp
  src/cdr_logger.cpp
  src/http_server.cpp
  ../common/src/logger.cpp
)

target_include_directories(pgw_server PRIVATE 
  include 
  ../common/include 
  ${httplib_SOURCE_DIR}
)

target_link_libraries(pgw_server PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  Threads::Threads
)