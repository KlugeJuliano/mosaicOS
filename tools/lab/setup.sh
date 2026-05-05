#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
  SUDO=""
else
  SUDO="sudo"
fi

if command -v apt-get >/dev/null 2>&1; then
  $SUDO apt-get update
  $SUDO apt-get install -y \
    build-essential \
    bison \
    device-tree-compiler \
    dialog \
    doxygen \
    flex \
    gawk \
    gcc-14 \
    g++-14 \
    git \
    graphviz \
    grub-pc-bin \
    libc6-dev-i386 \
    libgit-repository-perl \
    libncurses-dev \
    liburi-perl \
    libxml-parser-perl \
    mtools \
    ninja-build \
    pkg-config \
    python3 \
    qemu-system-x86 \
    xorriso
elif command -v pacman >/dev/null 2>&1; then
  $SUDO pacman -S --needed --noconfirm \
    base-devel \
    bison \
    clang \
    dtc \
    flex \
    gawk \
    gcc14 \
    git \
    grub \
    lld \
    mtools \
    ninja \
    pkgconf \
    python \
    qemu-system-x86 \
    xorriso
else
  echo "setup.sh supports hosts with apt-get or pacman." >&2
  exit 1
fi

"$(dirname "$0")/build-kernel.sh"
"$(dirname "$0")/build-l4re.sh"
"$(dirname "$0")/build-hello.sh"
