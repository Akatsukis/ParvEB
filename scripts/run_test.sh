#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

"$SCRIPT_DIR/build_with_debug.sh"
ctest --test-dir "$REPO_ROOT/build/debug" --output-on-failure
