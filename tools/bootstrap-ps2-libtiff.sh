#!/bin/sh
# Original LUNA code: Danny Nunez (dnunezx) 2026

set -eu

: "${PS2DEV:?PS2DEV must point to the PS2 development environment}"
: "${PS2SDK:?PS2SDK must point to the PS2SDK installation}"

install_path="$PS2SDK/ports/lib/libtiff.a"
if [ -f "$install_path" ]; then
  exit 0
fi

work_path="$(mktemp -d)"
trap 'rm -rf "$work_path"' EXIT HUP INT TERM

source_path="$work_path/libtiff"
build_path="$work_path/build"
expected_commit="5fe20d0e9aba49a6a350ed533459d1505203838f"

git clone --depth 1 --branch v4.7.1 \
  https://gitlab.com/libtiff/libtiff.git "$source_path"

actual_commit="$(git -C "$source_path" rev-parse HEAD)"
if [ "$actual_commit" != "$expected_commit" ]; then
  echo "Unexpected libtiff v4.7.1 revision: $actual_commit" >&2
  exit 1
fi

CFLAGS=-Dlfind=bsearch cmake -S "$source_path" -B "$build_path" \
  -DCMAKE_TOOLCHAIN_FILE="$PS2DEV/share/ps2dev.cmake" \
  -DCMAKE_INSTALL_PREFIX="$PS2SDK/ports" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -Dtiff-tools=OFF \
  -Dtiff-tests=OFF \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

cmake --build "$build_path" --parallel 2
cmake --install "$build_path"

