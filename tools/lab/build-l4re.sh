#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

echo "Building L4Re userland..."
ensure_l4re_sources

if [ -f "$L4RE_BUILD_DIR/Makefile" ] \
   && ! grep -q "SRC := $L4RE_DIR" "$L4RE_BUILD_DIR/Makefile"; then
  rm -rf "$L4RE_BUILD_DIR"
fi

if [ ! -f "$L4RE_BUILD_DIR/Makefile" ]; then
  bid_make -C "$L4RE_DIR" B="$L4RE_BUILD_DIR" PLATFORM_TYPE="$L4RE_PLATFORM"
fi

bid_make -C "$L4RE_DIR" O="$L4RE_BUILD_DIR" olddefconfig
bid_make -C "$L4RE_DIR" O="$L4RE_BUILD_DIR" -j"$JOBS"
