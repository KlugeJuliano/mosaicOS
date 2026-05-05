# MosaicOS Lab - L4Re Setup Guide

This document describes how to set up the L4Re (L4 Runtime Environment) laboratory for MosaicOS research.

## Host System Requirements
- Linux x86-64
- Build tools: `make`, `gcc`, `g++`, `git`, `flex`, `bison`, `gawk`, `pkg-config`
- QEMU: `qemu-system-x86_64`
- Multi-lib support (for 32-bit/64-bit cross-compilation if needed by L4Re)

## Build Process

### 1. Download L4Re and Fiasco.OC
The build scripts in `tools/lab/` will automatically download the necessary sources if they are not present.

### 2. Building the Kernel
```bash
./tools/lab/build-kernel.sh
```
This builds the Fiasco.OC microkernel binary.

### 3. Building L4Re Userland
```bash
./tools/lab/build-l4re.sh
```
This builds the core L4Re libraries and services.

### 4. Running the Hello World Experiment
```bash
./tools/lab/build-hello.sh
./tools/lab/run-qemu.sh
```

## Experiments

### Hello World
Location: `microkernel/experiments/hello/`
Prints a simple message to the serial console and exits.

### IPC Ping
Location: `microkernel/experiments/ipc-ping/`
Demonstrates basic IPC between a sender and a receiver task.

## Common Errors
- **Missing dependencies:** Ensure all required packages are installed on your host.
- **QEMU issues:** Ensure KVM is available if using `-enable-kvm`. For basic lab work, KVM is optional.
