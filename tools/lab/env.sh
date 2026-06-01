#!/usr/bin/env bash
set -euo pipefail

LAB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$LAB_DIR/../.." && pwd)"

LAB_WORKDIR="${LAB_WORKDIR:-$REPO_ROOT/.lab}"
L4RE_DIR="${L4RE_DIR:-$LAB_WORKDIR/l4}"
FIASCO_DIR="${FIASCO_DIR:-$LAB_WORKDIR/fiasco}"
BUILD_DIR="${BUILD_DIR:-$LAB_WORKDIR/build}"
KERNEL_BUILD_DIR="${KERNEL_BUILD_DIR:-$BUILD_DIR/fiasco}"
L4RE_BUILD_DIR="${L4RE_BUILD_DIR:-$BUILD_DIR/l4re}"
LAB_CONF_DIR="${LAB_CONF_DIR:-$REPO_ROOT/tools/lab/conf}"
EXPERIMENTS_DIR="${EXPERIMENTS_DIR:-$REPO_ROOT/microkernel/experiments}"
HAM_REPO="${HAM_REPO:-https://github.com/L4Re/ham.git}"
L4RE_MANIFEST_REPO="${L4RE_MANIFEST_REPO:-https://github.com/L4Re/manifest.git}"
L4RE_ARCH="${L4RE_ARCH:-amd64}"
L4RE_PLATFORM="${L4RE_PLATFORM:-pc}"
LAB_ENTRY="${LAB_ENTRY:-mosaicos-hello}"
JOBS="${JOBS:-$(nproc)}"

if [ -z "${CC:-}" ]; then
  for candidate in gcc-15 gcc15 gcc-14 gcc14 gcc-13 gcc13 gcc-12 gcc12 gcc-11 gcc11; do
    if command -v "$candidate" >/dev/null 2>&1; then
      CC="$candidate"
      HOST_CC="$candidate"
      break
    fi
  done
  if [ -z "${CC:-}" ]; then
    for candidate in clang-21 clang21 clang-20 clang20 clang-19 clang19 clang-18 clang18 clang-17 clang17 clang-16 clang16 clang-15 clang15 clang-14 clang14 clang-13 clang13 clang-12 clang12 clang-11 clang11; do
      if command -v "$candidate" >/dev/null 2>&1; then
        CC="$candidate"
        HOST_CC="$candidate"
        break
      fi
    done
  fi
  if [ -z "${CC:-}" ] && command -v clang >/dev/null 2>&1; then
    clang_major="$(clang --version | sed -n 's/.*clang version \([0-9][0-9]*\).*/\1/p' | head -n 1)"
    if [ -n "$clang_major" ] && [ "$clang_major" -le 21 ]; then
      CC="clang"
      HOST_CC="clang"
    fi
  fi
fi

if [ -z "${CXX:-}" ]; then
  for candidate in g++-15 g++15 g++-14 g++14 g++-13 g++13 g++-12 g++12 g++-11 g++11; do
    if command -v "$candidate" >/dev/null 2>&1; then
      CXX="$candidate"
      HOST_CXX="$candidate"
      break
    fi
  done
  if [ -z "${CXX:-}" ]; then
    for candidate in clang++-21 clang++21 clang++-20 clang++20 clang++-19 clang++19 clang++-18 clang++18 clang++-17 clang++17 clang++-16 clang++16 clang++-15 clang++15 clang++-14 clang++14 clang++-13 clang++13 clang++-12 clang++12 clang++-11 clang++11; do
      if command -v "$candidate" >/dev/null 2>&1; then
        CXX="$candidate"
        HOST_CXX="$candidate"
        break
      fi
    done
  fi
  if [ -z "${CXX:-}" ] && command -v clang++ >/dev/null 2>&1; then
    clangxx_major="$(clang++ --version | sed -n 's/.*clang version \([0-9][0-9]*\).*/\1/p' | head -n 1)"
    if [ -n "$clangxx_major" ] && [ "$clangxx_major" -le 21 ]; then
      CXX="clang++"
      HOST_CXX="clang++"
    fi
  fi
fi

if [ -z "${LD:-}" ]; then
  for candidate in ld.lld-21 ld.lld21 ld.lld-20 ld.lld20 ld.lld; do
    if command -v "$candidate" >/dev/null 2>&1; then
      LD="$candidate"
      HOST_LD="$candidate"
      break
    fi
  done
fi

export LAB_DIR REPO_ROOT LAB_WORKDIR L4RE_DIR FIASCO_DIR BUILD_DIR
export KERNEL_BUILD_DIR L4RE_BUILD_DIR LAB_CONF_DIR EXPERIMENTS_DIR
export HAM_REPO L4RE_MANIFEST_REPO L4RE_ARCH L4RE_PLATFORM LAB_ENTRY JOBS

bid_make()
{
  local make_args=(CROSS_COMPILE=)

  if [ -n "${CC:-}" ]; then make_args+=(CC="$CC"); fi
  if [ -n "${CXX:-}" ]; then make_args+=(CXX="$CXX"); fi
  if [ -n "${LD:-}" ]; then make_args+=(LD="$LD"); fi

  env -u CC -u CXX -u LD -u HOST_CC -u HOST_CXX -u HOST_LD \
    make "${make_args[@]}" "$@"
}

ensure_l4re_sources()
{
  local ham

  if [ -d "$L4RE_DIR/mk" ] && [ -d "$FIASCO_DIR/src" ]; then
    return 0
  fi

  mkdir -p "$LAB_WORKDIR"

  if command -v ham >/dev/null 2>&1; then
    ham="$(command -v ham)"
  else
    ham="$LAB_WORKDIR/ham/ham"
    if [ ! -x "$ham" ]; then
      git clone "$HAM_REPO" "$LAB_WORKDIR/ham"
    fi
  fi

  if [ ! -d "$LAB_WORKDIR/.ham" ]; then
    (cd "$LAB_WORKDIR" && "$ham" init -u "$L4RE_MANIFEST_REPO")
  fi

  (cd "$LAB_WORKDIR" && "$ham" sync)
}
