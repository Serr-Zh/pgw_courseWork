#!/bin/bash

# Проверяет нагрузку на pgw_server с 1000 клиентами

BUILD_DIR=~/pgw_project/build
CONFIG_FILE=~/pgw_project/config.json
CLIENT_CONFIG_FILE=~/pgw_project/client_config.json
CDR_LOG=~/pgw_project/scripts/cdr.log
START_IMSI=123456789100000
NUM_CLIENTS=1000

echo "Starting load test at $(date)..."

# Убиваем старые процессы pgw_server
pkill -f pgw_server
sleep 3 # Увеличена задержка для полного завершения

# Очищаем логи перед тестом
rm -f $BUILD_DIR/pgw.log $BUILD_DIR/client.log $CDR_LOG
echo "Logs cleared: pgw.log, client.log, cdr.log"

# Запускаем сервер
echo "Starting pgw_server..."
$BUILD_DIR/pgw_server/pgw_server $CONFIG_FILE &
SERVER_PID=$!
sleep 3 # Увеличена задержка для инициализации сервера

# Проверяем, запустился ли сервер
if ! ps -p $SERVER_PID > /dev/null; then
    echo "FAIL: Server failed to start"
    exit 1
fi
echo "Server started with PID $SERVER_PID"

# Отправляем 1000 запросов
echo "Sending $NUM_CLIENTS requests..."
for ((i=0; i<NUM_CLIENTS; i++)); do
    IMSI=$(($START_IMSI + $i))
    $BUILD_DIR/pgw_client/pgw_client $IMSI $CLIENT_CONFIG_FILE >> $BUILD_DIR/client.log 2>&1 &
    CLIENT_PIDS[$i]=$!
done

# Ждём завершения всех клиентов
echo "Waiting for all clients to complete..."
for pid in "${CLIENT_PIDS[@]}"; do
    wait $pid 2>/dev/null
done
echo "All clients completed"

# Даём дополнительное время на запись в CDR
echo "Waiting 15 seconds for CDR logging..."
sleep 15

# Проверяем CDR-лог на записи created
echo "Checking cdr.log for $NUM_CLIENTS created entries..."
if [ ! -f $CDR_LOG ]; then
    echo "FAIL: CDR log file does not exist ($CDR_LOG)"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi
echo "Contents of cdr.log before checking created:"
cat $CDR_LOG
CREATED_COUNT=$(grep -c ",created$" $CDR_LOG)
echo "Found $CREATED_COUNT created entries"
if [ $CREATED_COUNT -eq $NUM_CLIENTS ]; then
    echo "PASS: $CREATED_COUNT sessions created"
else
    echo "FAIL: Expected $NUM_CLIENTS created entries, got $CREATED_COUNT"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Ждём истечения сессий (30 секунд + запас)
echo "Waiting 35 seconds for session expiration..."
sleep 35

# Проверяем CDR-лог на записи deleted
echo "Contents of cdr.log before checking deleted:"
cat $CDR_LOG
DELETED_COUNT=$(grep -c ",deleted$" $CDR_LOG)
echo "Found $DELETED_COUNT deleted entries"
if [ $DELETED_COUNT -eq $NUM_CLIENTS ]; then
    echo "PASS: $DELETED_COUNT sessions deleted"
else
    echo "FAIL: Expected $NUM_CLIENTS deleted entries, got $DELETED_COUNT"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Останавливаем сервер
echo "Stopping server..."
curl -s "http://127.0.0.1:8080/stop"
wait $SERVER_PID 2>/dev/null
echo "Load test completed successfully at $(date)!"