cmake_minimum_required(VERSION 3.10)
project(PGWProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение FetchContent для загрузки библиотек
include(FetchContent)

# Подключение nlohmann::json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Подключение spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# Подключение cpp-httplib
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.0
)
FetchContent_MakeAvailable(httplib)

# Подключение googletest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Включение тестирования
enable_testing()

# Включение подпроектов
add_subdirectory(pgw_server)
add_subdirectory(pgw_client)
add_subdirectory(tests)