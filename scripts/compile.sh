#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_ROOT/build"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$BUILD_DIR" --config RelWithDebInfo
