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
    printf 'Graphical recovery test output kept at %s\n' "$OUTPUT_FILE"
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
timeout "$QEMU_TIMEOUT" "$LAB_DIR/run-qemu.sh" mosaicos-graphical-recovery >"$OUTPUT_FILE" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  printf 'QEMU exited unexpectedly with status %s\n' "$status" >&2
  printf 'Full output: %s\n' "$OUTPUT_FILE" >&2
  exit "$status"
fi

require_line "mosaic-init: loaded 6 service(s)"
require_line "mosaic-init: service 'compositor' max_restarts 3"
require_line "mosaic-init: service 'compositor' fallback 'safe-gui'"
require_line "MosaicOS Graphics: compositor controlled crash requested"
require_line "[CRASH] service=compositor reason=TaskExit restart_count=1 action=restart"
require_line "[CRASH] service=compositor reason=TaskExit restart_count=2 action=restart"
require_line "[CRASH] service=compositor reason=TaskExit restart_count=3 action=restart"
require_line "[CRASH] service=compositor reason=TaskExit restart_count=3 action=failed"
require_line "[CRASH] service=compositor reason=TaskExit restart_count=3 action=fallback:safe-gui"
require_line "MosaicOS Graphics: Safe Mode. Compositor unavailable."
require_line "mosaic-init: status compositor   failed"
require_line "mosaic-init: status safe-gui     running"

printf 'Graphical recovery test passed.\n'
