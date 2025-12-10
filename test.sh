#!/bin/bash
TARGET_SESSIONS=50
BIN="./build/test_bot"

echo "Running $TARGET_SESSIONS masters (and $(((TARGET_SESSIONS * 3))) childs)..."

for i in $(seq 1 $TARGET_SESSIONS); do
    $BIN > /dev/null 2>&1 & 
    PID=$!
    echo "Created session #$i (PID: $PID)"
    sleep 0.1 
done

echo "All bots are running press ENTER to kill all"
read
pkill -f "$BIN"
