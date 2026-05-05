#!/bin/bash
set -e

# Configuration
L4RE_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-source"
FIASCO_DIR="$L4RE_DIR/src/kernel/fiasco"
BUILD_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-build"
KERNEL_BUILD_DIR="$BUILD_DIR/fiasco"

mkdir -p "$L4RE_DIR"
mkdir -p "$BUILD_DIR"

# Download L4Re if not exists
if [ ! -d "$L4RE_DIR/.git" ]; then
    echo "Downloading L4Re source..."
    git clone https://github.com/kernkonzept/l4re-core.git "$L4RE_DIR"
fi

# Download Fiasco if not exists
if [ ! -d "$FIASCO_DIR/.git" ]; then
    echo "Downloading Fiasco.OC source..."
    git clone https://github.com/kernkonzept/fiasco.git "$FIASCO_DIR"
fi

echo "Building Fiasco.OC kernel..."
mkdir -p "$KERNEL_BUILD_DIR"

# Basic configuration for x86_64
make -C "$FIASCO_DIR" BUILDDIR="$KERNEL_BUILD_DIR" olddefconfig

# Build from the build directory (Fiasco creates it inside the source by default in some versions)
cd "$FIASCO_DIR/build"
make -j$(nproc)
