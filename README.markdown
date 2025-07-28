# Мини-PGW: Реализация шлюза пакетной передачи данных

## Обзор
Мини-PGW — это упрощённая реализация сетевого компонента PGW (Packet Gateway) для выпускной работы школы C++. Проект обрабатывает UDP-запросы с IMSI, управляет сессиями абонентов, ведёт журнал CDR, предоставляет HTTP API, поддерживает чёрный список IMSI и обеспечивает корректное завершение работы с постепенной выгрузкой сессий. Включает сервер (`pgw_server`), клиент (`pgw_client`) и юнит-тесты.

## Возможности
- **UDP-сервер**: Приём IMSI в BCD-кодировке, создание/отклонение сессий, отправка ответов `created` или `rejected`.
- **Управление сессиями**: Отслеживание активных сессий с истечением по таймеру.
- **Журнал CDR**: Запись событий сессий (`created`, `deleted`) в файл `cdr.log` с метками времени.
- **HTTP API**:
  - `/check_subscriber?imsi=<IMSI>`: Возвращает `active` или `not active` в зависимости от статуса сессии.
  - `/stop`: Инициирует постепенное завершение работы с выгрузкой сессий.
- **Чёрный список**: Отклонение IMSI из конфигурации с логированием в `pgw.log` без записи в CDR.
- **Конфигурация**: Чтение настроек из JSON-файлов (`config.json` для сервера, `client_config.json` для клиента).
- **Логирование**: Использует `spdlog` для записи в `pgw.log` и `client.log` с уровнями `debug`, `info`, `warn`, `error`, `critical`.
- **Многопоточность**: Пул потоков для обработки UDP-запросов и фоновый поток для очистки сессий.

## Требования
- **ОС**: Linux
- **Компилятор**: Поддержка C++17 (например, GCC 13.3.0)
- **Зависимости** (скачиваются через CMake):
  - `spdlog` (v1.14.1)
  - `nlohmann::json` (v3.11.3)
  - `googletest` (v1.12.1)
  - `cpp-httplib` (v0.15.3)
- **Инструменты**: CMake 3.10+, Make

## Структура директорий
```
pgw_project/
├── pgw_server/
│   ├── include/                # Заголовочные файлы (config.hpp, udp_server.hpp и др.)
│   ├── src/                   # Исходные файлы (main.cpp, config.cpp и др.)
│   └── CMakeLists.txt         # Конфигурация CMake для pgw_server
├── pgw_client/
│   ├── include/               # Заголовочные файлы для клиента
│   ├── src/                  # Исходные файлы (client.cpp)
│   └── CMakeLists.txt        # Конфигурация CMake для pgw_client
├── tests/                    # Юнит-тесты (test_http_server.cpp и др.)
├── scripts/                  # Скрипты тестирования (test_functional.sh, test_blacklist.sh и др.)
├── config.json               # Конфигурация сервера
├── client_config.json        # Конфигурация клиента
└── CMakeLists.txt            # Основная конфигурация CMake
```

## Установка
1. Клонируйте репозиторий:
   ```bash
   git clone https://github.com/Serr-Zh/pgw_courseWork.git
   cd pgw_project
   ```
2. Создайте директорию сборки и перейдите в неё:
   ```bash
   mkdir build && cd build
   ```
3. Настройте и соберите проект:
   ```bash
   cmake ..
   make
   ```

## Конфигурация
- **Сервер (`config.json`)**:
  ```json
  {
    "udp_ip": "0.0.0.0",
    "udp_port": 9000,
    "session_timeout_sec": 30,
    "cdr_file": "cdr.log",
    "http_port": 8080,
    "graceful_shutdown_rate": 10,
    "log_file": "pgw.log",
    "log_level": "INFO",
    "blacklist": ["001010123456789", "001010000000001"]
  }
  ```
- **Клиент (`client_config.json`)**:
  ```json
  {
    "server_ip": "127.0.0.1",
    "server_port": 9000,
    "log_file": "client.log",
    "log_level": "INFO"
  }
  ```

## Использование
1. **Запуск сервера**:
   ```bash
   cd build/pgw_server
   ./pgw_server ../../config.json
   ```
   Логи записываются в `pgw.log` и `cdr.log`.

2. **Запуск клиента**:
   ```bash
   cd build/pgw_client
   ./pgw_client 123456789012345 ../../client_config.json
   ```
   Вывод: `Response: created` или `Response: rejected`
   Логи записываются в `client.log`.

3. **HTTP API**:
   - Проверка статуса сессии:
     ```bash
     curl "http://127.0.0.1:8080/check_subscriber?imsi=123456789012345"
     ```
     Вывод: `active` или `not active`
   - Остановка сервера:
     ```bash
     curl "http://127.0.0.1:8080/stop"
     ```
     Вывод: `Stopping server...`

4. **Запуск тестов**:
   ```bash
   cd build
   ctest -V
   cd ../scripts
   ./test_functional.sh
   ./test_load.sh
   ./test_blacklist.sh
   ```

## Тестирование
- **Юнит-тесты**: Реализованы с GoogleTest в `tests/test_http_server.cpp`.
  - Тесты: `CheckSubscriberActive`, `CheckSubscriberNotActive`, `StopServer`, `StopWithActiveSessions`, `Blacklist`.
- **Функциональные тесты**: `scripts/test_functional.sh` проверяет базовую функциональность.
- **Нагрузочные тесты**: `scripts/test_load.sh` проверяет производительность.
- **Тесты чёрного списка**: `scripts/test_blacklist.sh` проверяет обработку чёрного списка.

