#!/bin/bash

KERNEL_BIN="/home/julianok/.gemini/tmp/klugeos/l4re-build/fiasco/fiasco"
L4RE_BIN_DIR="/home/julianok/.gemini/tmp/klugeos/l4re-build/l4re/bin/amd64_l4f"

echo "Running QEMU..."

qemu-system-x86_64 \
  -kernel "$KERNEL_BIN" \
  -append "serial esc" \
  -serial stdio \
  -m 256M \
  -nographic \
  -no-reboot
  # Note: A real L4Re run requires a boot image (ISO/GRUB) with bootstrap, moe, and modules.
  # This is a placeholder for the final command once the image is built.
