#!/bin/bash

# Проверяет функциональность pgw_server и pgw_client

BUILD_DIR=~/pgw_project/build
CONFIG_FILE=../config.json
CLIENT_CONFIG_FILE=../client_config.json
CDR_LOG=~/pgw_project/scripts/cdr.log
VALID_IMSI=123456789012345
BLACKLIST_IMSI=001010123456789

echo "Starting functional test..."

# Убиваем старые процессы pgw_server
pkill -f pgw_server

# Очищаем логи
rm -f $BUILD_DIR/pgw.log $BUILD_DIR/client.log $CDR_LOG

# Запускаем сервер в фоновом режиме
$BUILD_DIR/pgw_server/pgw_server $CONFIG_FILE &
SERVER_PID=$!
sleep 2

# Проверяем, запустился ли сервер
if ! ps -p $SERVER_PID > /dev/null; then
    echo "FAIL: Server failed to start"
    exit 1
fi

# Проверяем /check_subscriber перед созданием сессии
echo "Checking subscriber before session creation..."
curl -s "http://127.0.0.1:8080/check_subscriber?imsi=$VALID_IMSI" | grep -q "not active"
if [ $? -eq 0 ]; then
    echo "PASS: Subscriber is not active"
else
    echo "FAIL: Subscriber should be not active"
    kill $SERVER_PID
    exit 1
fi

# Отправляем валидный IMSI
echo "Sending valid IMSI: $VALID_IMSI"
$BUILD_DIR/pgw_client/pgw_client $VALID_IMSI $CLIENT_CONFIG_FILE | grep -q "Response: created"
if [ $? -eq 0 ]; then
    echo "PASS: Session created for valid IMSI"
else
    echo "FAIL: Session creation failed"
    kill $SERVER_PID
    exit 1
fi

# Проверяем /check_subscriber после создания сессии
echo "Checking subscriber after session creation..."
curl -s "http://127.0.0.1:8080/check_subscriber?imsi=$VALID_IMSI" | grep -q "active"
if [ $? -eq 0 ]; then
    echo "PASS: Subscriber is active"
else
    echo "FAIL: Subscriber should be active"
    kill $SERVER_PID
    exit 1
fi

# Ждём истечения сессии (30 секунд)
echo "Waiting for session expiration (30 seconds)..."
sleep 32

# Проверяем /check_subscriber после истечения
curl -s "http://127.0.0.1:8080/check_subscriber?imsi=$VALID_IMSI" | grep -q "not active"
if [ $? -eq 0 ]; then
    echo "PASS: Subscriber is not active after expiration"
else
    echo "FAIL: Subscriber should be not active after expiration"
    kill $SERVER_PID
    exit 1
fi

# Проверяем CDR-лог
echo "Checking cdr.log..."
if [ ! -f $CDR_LOG ]; then
    echo "FAIL: CDR log file does not exist"
    kill $SERVER_PID
    exit 1
fi
grep -q "$VALID_IMSI,created" $CDR_LOG && grep -q "$VALID_IMSI,deleted" $CDR_LOG
if [ $? -eq 0 ]; then
    echo "PASS: CDR log contains created and deleted entries"
else
    echo "FAIL: CDR log is incomplete"
    kill $SERVER_PID
    exit 1
fi

# Отправляем IMSI из чёрного списка
echo "Sending blacklisted IMSI: $BLACKLIST_IMSI"
$BUILD_DIR/pgw_client/pgw_client $BLACKLIST_IMSI $CLIENT_CONFIG_FILE | grep -q "Response: rejected"
if [ $? -eq 0 ]; then
    echo "PASS: Blacklisted IMSI rejected"
else
    echo "FAIL: Blacklisted IMSI not rejected"
    kill $SERVER_PID
    exit 1
fi

# Останавливаем сервер
echo "Stopping server..."
curl -s "http://127.0.0.1:8080/stop" | grep -q "Stopping server..."
if [ $? -eq 0 ]; then
    echo "PASS: Server stopped gracefully"
else
    echo "FAIL: Server stop failed"
    kill $SERVER_PID
    exit 1
fi

# Ждём завершения сервера
wait $SERVER_PID
echo "Functional test completed successfully!"