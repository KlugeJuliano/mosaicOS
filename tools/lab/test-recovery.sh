#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

OUTPUT_FILE="${OUTPUT_FILE:-$(mktemp)}"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-120}"

cleanup()
{
  if [ -z "${KEEP_OUTPUT:-}" ]; then
    rm -f "$OUTPUT_FILE"
  else
    printf 'Recovery test output kept at %s\n' "$OUTPUT_FILE"
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
timeout "$QEMU_TIMEOUT" "$LAB_DIR/run-qemu.sh" mosaicos-recovery >"$OUTPUT_FILE" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  printf 'QEMU exited unexpectedly with status %s\n' "$status" >&2
  printf 'Full output: %s\n' "$OUTPUT_FILE" >&2
  exit "$status"
fi

require_line "mosaic-init: loaded 4 service(s)"
require_line "mosaic-init: service 'crashy' max_restarts 2"
require_line "mosaic-init: service 'crashy' fallback 'safe-gui'"
require_line "mosaic-init: starting service 'crashy'"
require_line "MosaicOS Lab: controlled crash service exiting with failure"
require_line "mosaic-init: service 'crashy' failed during attempt 1"
require_line "[CRASH] service=crashy reason=TaskExit restart_count=1 action=restart"
require_line "mosaic-init: restarting service 'crashy' (1/2)"
require_line "mosaic-init: service 'crashy' failed during attempt 2"
require_line "[CRASH] service=crashy reason=TaskExit restart_count=2 action=restart"
require_line "mosaic-init: restarting service 'crashy' (2/2)"
require_line "mosaic-init: service 'crashy' failed during attempt 3"
require_line "[CRASH] service=crashy reason=TaskExit restart_count=2 action=failed"
require_line "mosaic-init: service 'crashy' marked failed after 3 attempt(s)"
require_line "mosaic-init: fallback 'safe-gui' is declared for service 'crashy'"
require_line "[CRASH] service=crashy reason=TaskExit restart_count=2 action=fallback:safe-gui"
require_line "mosaic-init: starting fallback 'safe-gui' for service 'crashy'"
require_line "MosaicOS Lab: safe-gui fallback active"
require_line "mosaic-init: fallback 'safe-gui' started"
require_line "mosaic-init: status crashy       failed"
require_line "mosaic-init: status safe-gui     running"
require_line "mosaic-init: status status       running"
require_line "MosaicOS Lab: status service online"

printf 'Recovery test passed.\n'
