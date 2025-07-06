#!/bin/bash

# Define pipe directory (optional)
PIPE_DIR="/tmp/chat_pipes"
mkdir -p $PIPE_DIR

# Set path for registration FIFO
REG_FIFO="$PIPE_DIR/registration_fifo"

# Remove old registration FIFO
if [ -p "$REG_FIFO" ]; then
    echo "Removing old registration FIFO..."
    rm "$REG_FIFO"
fi

# Compile the server
echo "Compiling server..."
gcc -o server server.c

# Export path so the server knows where to find FIFO
export CHAT_PIPE_DIR="$PIPE_DIR"

# Run the server
echo "Starting server..."
./server
