#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

ENTRY="${1:-$LAB_ENTRY}"
MODULES_LIST="${MODULES_LIST:-$LAB_CONF_DIR/mosaicos-lab.list}"
MODULE_SEARCH_PATH="${MODULE_SEARCH_PATH:-$LAB_CONF_DIR:$EXPERIMENTS_DIR/hello:$EXPERIMENTS_DIR/init:$EXPERIMENTS_DIR/log:$EXPERIMENTS_DIR/status:$EXPERIMENTS_DIR/crash:$EXPERIMENTS_DIR/safe-gui:$EXPERIMENTS_DIR/display:$EXPERIMENTS_DIR/input:$EXPERIMENTS_DIR/compositor:$EXPERIMENTS_DIR/compositor-crash:$EXPERIMENTS_DIR/graphical-hello:$EXPERIMENTS_DIR/ipc-ping:$KERNEL_BUILD_DIR}"
if [ -z "${QEMU_OPTIONS:-}" ]; then
  case "$ENTRY" in
    mosaicos-graphical|mosaicos-graphical-recovery)
      QEMU_OPTIONS="-monitor none -serial stdio -m 512 -M q35 -device VGA,vgamem_mb=16 -display gtk -no-reboot"
      ;;
    *)
      QEMU_OPTIONS="-monitor none -serial stdio -m 512 -M q35 -nographic -no-reboot"
      ;;
  esac
fi

export MODULES_LIST MODULE_SEARCH_PATH QEMU_OPTIONS

echo "Running L4Re entry '$ENTRY' in QEMU..."
cd "$L4RE_BUILD_DIR"
env -u CC -u CXX -u LD -u HOST_CC -u HOST_CXX -u HOST_LD make qemu E="$ENTRY"
