#!/bin/bash
set -e

L4RE_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-source"
BUILD_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-build"
L4RE_BUILD_DIR="$BUILD_DIR/l4re"

echo "Building L4Re userland..."
mkdir -p "$L4RE_BUILD_DIR"

cd "$L4RE_DIR"
# Initial configuration
make setup BUILDDIR="$L4RE_BUILD_DIR"

cd "$L4RE_BUILD_DIR"
# Select a default config for x86_64 if possible
# make olddefconfig
make -j$(nproc)
