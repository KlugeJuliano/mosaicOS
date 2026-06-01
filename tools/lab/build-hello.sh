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

# Build graphical prototype services (Rust-based)
cd "$REPO_ROOT/graphics/display-server"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$REPO_ROOT/graphics/input-server"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$REPO_ROOT/graphics/compositor"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

cd "$REPO_ROOT/shell/apps/mosaic-hello"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build legacy/simulation components if needed
cd "$EXPERIMENTS_DIR/safe-gui"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"

# Build IPC Ping
cd "$EXPERIMENTS_DIR/ipc-ping"
bid_make L4DIR="$L4RE_DIR" OBJ_BASE="$L4RE_BUILD_DIR"
