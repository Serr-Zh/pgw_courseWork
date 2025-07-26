add_executable(test_config
  test_config.cpp
  ../pgw_server/src/config.cpp
  ../common/src/logger.cpp
)

add_executable(test_session_manager
  test_session_manager.cpp
  ../pgw_server/src/config.cpp
  ../pgw_server/src/session_manager.cpp
  ../pgw_server/src/cdr_logger.cpp
  ../common/src/logger.cpp
)

add_executable(test_cdr_logger
  test_cdr_logger.cpp
  ../pgw_server/src/config.cpp
  ../pgw_server/src/cdr_logger.cpp
  ../common/src/logger.cpp
)

add_executable(test_udp_server
  test_udp_server.cpp
  ../pgw_server/src/config.cpp
  ../pgw_server/src/udp_server.cpp
  ../pgw_server/src/session_manager.cpp
  ../pgw_server/src/cdr_logger.cpp
  ../common/src/logger.cpp
)

add_executable(test_http_server
  test_http_server.cpp
  ../pgw_server/src/config.cpp
  ../pgw_server/src/http_server.cpp
  ../pgw_server/src/udp_server.cpp
  ../pgw_server/src/session_manager.cpp
  ../pgw_server/src/cdr_logger.cpp
  ../common/src/logger.cpp
)

add_executable(test_udp_client
  test_udp_client.cpp
  ../pgw_server/src/config.cpp
  ../pgw_server/src/udp_server.cpp
  ../pgw_server/src/session_manager.cpp
  ../pgw_server/src/cdr_logger.cpp
  ../pgw_client/src/client_config.cpp
  ../pgw_client/src/udp_client.cpp
  ../common/src/logger.cpp
)

target_include_directories(test_config PRIVATE 
  ../pgw_server/include 
  ../common/include
)

target_include_directories(test_session_manager PRIVATE 
  ../pgw_server/include 
  ../common/include
)

target_include_directories(test_cdr_logger PRIVATE 
  ../pgw_server/include 
  ../common/include
)

target_include_directories(test_udp_server PRIVATE 
  ../pgw_server/include 
  ../common/include
)

target_include_directories(test_http_server PRIVATE 
  ../pgw_server/include 
  ../common/include
  ${httplib_SOURCE_DIR}
)

target_include_directories(test_udp_client PRIVATE 
  ../pgw_server/include 
  ../pgw_client/include
  ../common/include
)

target_link_libraries(test_config PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

target_link_libraries(test_session_manager PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

target_link_libraries(test_cdr_logger PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

target_link_libraries(test_udp_server PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

target_link_libraries(test_http_server PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

target_link_libraries(test_udp_client PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

add_test(NAME ConfigTest COMMAND test_config)
add_test(NAME SessionManagerTest COMMAND test_session_manager)
add_test(NAME CDRLoggerTest COMMAND test_cdr_logger)
add_test(NAME UDPServerTest COMMAND test_udp_server)
add_test(NAME HTTPServerTest COMMAND test_http_server)
add_test(NAME UDPClientTest COMMAND test_udp_client)