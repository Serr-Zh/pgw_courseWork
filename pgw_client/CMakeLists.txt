add_executable(pgw_client
  src/main.cpp
  src/client_config.cpp
  src/udp_client.cpp
  ../common/src/logger.cpp
)

target_include_directories(pgw_client PRIVATE 
  include 
  ../common/include
)

target_link_libraries(pgw_client PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog
)