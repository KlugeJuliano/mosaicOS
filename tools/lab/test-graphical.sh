#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

OUTPUT_FILE="${OUTPUT_FILE:-$(mktemp)}"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-120}"
QEMU_OPTIONS="${QEMU_OPTIONS:--monitor none -serial stdio -m 512 -M q35 -nographic -no-reboot}"
export QEMU_OPTIONS

cleanup()
{
  if [ -z "${KEEP_OUTPUT:-}" ]; then
    rm -f "$OUTPUT_FILE"
  else
    printf 'Graphical test output kept at %s\n' "$OUTPUT_FILE"
  fi
}

require_line()
{
  local pattern="$1"

  if ! grep -F "$pattern" "$OUTPUT_FILE" >/dev/null; then
    printf 'Missing expected output: %s\n' "$pattern" >&2
    printf 'Full output: %s\n' "$OUTPUT_FILE" >&2
    return 1
  fi
}

trap cleanup EXIT

"$LAB_DIR/build-hello.sh"

set +e
timeout "$QEMU_TIMEOUT" "$LAB_DIR/run-qemu.sh" mosaicos-graphical >"$OUTPUT_FILE" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  printf 'QEMU exited unexpectedly with status %s\n' "$status" >&2
  printf 'Full output: %s\n' "$OUTPUT_FILE" >&2
  exit "$status"
fi

require_line "mosaic-init: loaded 7 service(s)"
require_line "mosaic-init: service 'display' capability 'fb'"
require_line "MosaicOS Graphics: framebuffer owner ready"
require_line "MosaicOS Graphics: input-server online keyboard=ps2 mouse=ps2"
require_line "MosaicOS Graphics: compositor online target_fps=30"
require_line "MosaicOS Graphics: compositor window id=1 title='MosaicOS Hello' geometry=400x300+120+50"
require_line "MosaicOS Graphics: compositor frame 1 clear=#101020 window=#1a1a2e text='MosaicOS Lab' cursor=visible"
require_line "MosaicOS Graphics: app mosaic-hello create-window width=400 height=300 title='MosaicOS Hello'"
require_line "MosaicOS Graphics: app mosaic-hello input KeyDown key=Q action=exit"
require_line "mosaic-init: status hello-gui    running"

printf 'Graphical test passed.\n'
