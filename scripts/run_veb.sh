#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
RUN_BIN="$REPO_ROOT/build/release/run_veb"

SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
NUM_INSERTS="${PARVEB_RUN_INSERTS:-${PARVEB_RUN_KEYS:-10000000}}"
TRIALS="${PARVEB_RUN_TRIALS:-5}"
SEED="${PARVEB_RUN_SEED:-0}"
BITS="${PARVEB_RUN_BITS:-48}"

PARVEB_ENABLE_SIMD="$SIMD_OPTION" "$BUILD_SCRIPT"

if [[ ! -x "$RUN_BIN" ]]; then
    echo "Runner binary not found at $RUN_BIN" >&2
    exit 1
fi

"$RUN_BIN" --num_inserts="$NUM_INSERTS" --trials="$TRIALS" --seed="$SEED" --bits="$BITS" "$@"
