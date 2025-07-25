add_executable(test_config
  test_config.cpp
  ../pgw_server/src/config.cpp
  ../common/src/logger.cpp
)

target_include_directories(test_config PRIVATE 
  ../pgw_server/include 
  ../common/include
)

target_link_libraries(test_config PRIVATE 
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  GTest::gtest 
  GTest::gtest_main
)

add_test(NAME ConfigTest COMMAND test_config)