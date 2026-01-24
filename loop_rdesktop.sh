#!/bin/bash

# Loop rdesktop connection with retry on failure
# Usage: ./loop_rdesktop.sh [host:port] [delay_seconds]

HOST="${1:-192.168.1.195:13389}"
DELAY="${2:-2}"

echo "Starting rdesktop loop to $HOST (retry delay: ${DELAY}s)"
echo "Press Ctrl+C to stop"

while true; do
    echo "$(date): Connecting to $HOST..."
    ./rdesktop -v "$HOST"
    EXIT_CODE=$?
    
    echo "$(date): Connection failed with exit code $EXIT_CODE"
    echo "Retrying in ${DELAY} seconds..."
    sleep "$DELAY"
done