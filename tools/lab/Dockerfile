FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    bison \
    ca-certificates \
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
    xorriso \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/klugeos

CMD ["bash"]
