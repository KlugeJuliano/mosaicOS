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

# Build service manager validation services
cd "$EXPERIMENTS_DIR/log"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$EXPERIMENTS_DIR/status"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build controlled failure service for recovery experiments
cd "$EXPERIMENTS_DIR/crash"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build fallback service for recovery experiments
cd "$EXPERIMENTS_DIR/safe-gui"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build graphical prototype services
cd "$EXPERIMENTS_DIR/display"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$EXPERIMENTS_DIR/input"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$EXPERIMENTS_DIR/compositor"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$EXPERIMENTS_DIR/compositor-crash"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$EXPERIMENTS_DIR/graphical-hello"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build IPC Ping
cd "$EXPERIMENTS_DIR/ipc-ping"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"
