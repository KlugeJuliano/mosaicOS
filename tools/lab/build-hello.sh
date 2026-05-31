#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

echo "Building experiments..."

# Build Hello World
cd "$EXPERIMENTS_DIR/hello"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build Minimal Init
cd "$EXPERIMENTS_DIR/init"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build IPC Ping
cd "$EXPERIMENTS_DIR/ipc-ping"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"
