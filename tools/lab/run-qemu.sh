#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

ENTRY="${1:-$LAB_ENTRY}"
MODULES_LIST="${MODULES_LIST:-$LAB_CONF_DIR/mosaicos-lab.list}"
MODULE_SEARCH_PATH="${MODULE_SEARCH_PATH:-$LAB_CONF_DIR:$EXPERIMENTS_DIR/hello:$EXPERIMENTS_DIR/init:$EXPERIMENTS_DIR/log:$EXPERIMENTS_DIR/status:$EXPERIMENTS_DIR/ipc-ping:$KERNEL_BUILD_DIR}"
QEMU_OPTIONS="${QEMU_OPTIONS:--monitor none -serial stdio -m 512 -M q35 -nographic -no-reboot}"

export MODULES_LIST MODULE_SEARCH_PATH QEMU_OPTIONS

echo "Running L4Re entry '$ENTRY' in QEMU..."
cd "$L4RE_BUILD_DIR"
bid_make qemu E="$ENTRY"
