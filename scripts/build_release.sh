#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_ROOT/build/release"
SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
BUILD_TYPE="${PARVEB_BUILD_TYPE:-Release}"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DPARVEB_ENABLE_SIMD="$SIMD_OPTION"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -- -j"$(nproc)"
