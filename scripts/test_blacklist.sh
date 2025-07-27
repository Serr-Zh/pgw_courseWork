#!/bin/bash

# Проверяет чёрный список IMSI

BUILD_DIR=~/pgw_project/build
CONFIG_FILE=../config.json
CLIENT_CONFIG_FILE=../client_config.json
CDR_LOG=~/pgw_project/scripts/cdr.log
BLACKLIST_IMSI=001010123456789

echo "Starting blacklist test..."

# Убиваем старые процессы pgw_server
pkill -f pgw_server
sleep 1 # Даём время на завершение процессов

# Очищаем логи
rm -f $BUILD_DIR/pgw.log $BUILD_DIR/client.log $CDR_LOG

# Запускаем сервер
$BUILD_DIR/pgw_server/pgw_server $CONFIG_FILE &
SERVER_PID=$!
sleep 2

# Проверяем, запустился ли сервер
if ! ps -p $SERVER_PID > /dev/null; then
    echo "FAIL: Server failed to start"
    exit 1
fi

# Отправляем IMSI из чёрного списка
echo "Sending blacklisted IMSI: $BLACKLIST_IMSI"
$BUILD_DIR/pgw_client/pgw_client $BLACKLIST_IMSI $CLIENT_CONFIG_FILE | grep -q "Response: rejected"
if [ $? -eq 0 ]; then
    echo "PASS: Blacklisted IMSI rejected"
else
    echo "FAIL: Blacklisted IMSI not rejected"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Проверяем, что сессия не создана
curl -s "http://127.0.0.1:8080/check_subscriber?imsi=$BLACKLIST_IMSI" | grep -q "not active"
if [ $? -eq 0 ]; then
    echo "PASS: Blacklisted IMSI is not active"
else
    echo "FAIL: Blacklisted IMSI should be not active"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Проверяем CDR-лог
if [ -f $CDR_LOG ] && grep -q "$BLACKLIST_IMSI" $CDR_LOG; then
    echo "FAIL: CDR log should not contain blacklisted IMSI"
    kill $SERVER_PID 2>/dev/null
    exit 1
else
    echo "PASS: CDR log does not contain blacklisted IMSI"
fi

# Останавливаем сервер
curl -s "http://127.0.0.1:8080/stop"
wait $SERVER_PID
echo "Blacklist test completed successfully!"