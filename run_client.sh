#!/bin/bash

export REGPIPE="/path"

# Compile the client
echo "Compiling client..."
gcc -o client client.c

./client