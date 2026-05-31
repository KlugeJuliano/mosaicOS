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
    printf 'Service manager test output kept at %s\n' "$OUTPUT_FILE"
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
timeout "$QEMU_TIMEOUT" "$LAB_DIR/run-qemu.sh" mosaicos-init >"$OUTPUT_FILE" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  printf 'QEMU exited unexpectedly with status %s\n' "$status" >&2
  printf 'Full output: %s\n' "$OUTPUT_FILE" >&2
  exit "$status"
fi

require_line "mosaic-init: loaded 3 service(s)"
require_line "mosaic-init: startup round 1"
require_line "mosaic-init: starting service 'logger'"
require_line "mosaic-init: startup round 2"
require_line "mosaic-init: starting service 'hello'"
require_line "mosaic-init: starting service 'status'"
require_line "mosaic-init: status logger       running"
require_line "mosaic-init: status hello        running"
require_line "mosaic-init: status status       running"
require_line "MosaicOS Lab: log service online"
require_line "MosaicOS Lab: hello from L4Re"
require_line "MosaicOS Lab: status service online"

printf 'Service manager test passed.\n'
