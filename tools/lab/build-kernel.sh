#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/env.sh"

mkdir -p "$LAB_WORKDIR" "$BUILD_DIR"
ensure_l4re_sources

echo "Building Fiasco.OC kernel..."

if [ -f "$KERNEL_BUILD_DIR/Makefile" ] \
   && ! grep -q "srcdir  := $FIASCO_DIR/src" "$KERNEL_BUILD_DIR/Makefile"; then
    rm -rf "$KERNEL_BUILD_DIR"
fi

if [ ! -f "$KERNEL_BUILD_DIR/Makefile" ]; then
    make -C "$FIASCO_DIR" B="$KERNEL_BUILD_DIR"
fi

configure_tool()
{
    local file="$KERNEL_BUILD_DIR/globalconfig.out"
    local key value

    [ -f "$file" ] || return 0

    for key in CC CXX LD HOST_CC HOST_CXX; do
        value="${!key:-}"
        [ -n "$value" ] || continue

        if grep -q "^CONFIG_$key=" "$file"; then
            sed -i "s|^CONFIG_$key=.*|CONFIG_$key=\"$value\"|" "$file"
        else
            printf 'CONFIG_%s="%s"\n' "$key" "$value" >> "$file"
        fi
    done
}

configure_tool
make -C "$KERNEL_BUILD_DIR" olddefconfig
make -C "$KERNEL_BUILD_DIR" -j"$JOBS"
