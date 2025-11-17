#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

"$SCRIPT_DIR/compile.sh"
ctest --test-dir "$REPO_ROOT/build" --output-on-failure
