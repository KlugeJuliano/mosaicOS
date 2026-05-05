#!/bin/bash
set -e

L4RE_BUILD_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-build/l4re"
EXPERIMENTS_DIR="/home/julianok/projetos/klugeos/microkernel/experiments"

echo "Building experiments..."

# Build Hello World
cd "$EXPERIMENTS_DIR/hello"
make L4DIR="/home/julianok/.gemini/tmp/klugeos/l4re-source" OBJ32_DIR="$L4RE_BUILD_DIR/obj/l4/x86" OBJ64_DIR="$L4RE_BUILD_DIR/obj/l4/amd64"

# Build IPC Ping
cd "$EXPERIMENTS_DIR/ipc-ping"
make L4DIR="/home/julianok/.gemini/tmp/klugeos/l4re-source" OBJ32_DIR="$L4RE_BUILD_DIR/obj/l4/x86" OBJ64_DIR="$L4RE_BUILD_DIR/obj/l4/amd64"
