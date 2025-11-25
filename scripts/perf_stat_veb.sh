#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
RUN_BIN="$REPO_ROOT/build/release/run_veb"
LOG_DIR="$REPO_ROOT/logs/perf"

SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
NUM_KEYS="${PARVEB_RUN_KEYS:-100000000}"
TRIALS="${PARVEB_RUN_TRIALS:-5}"
SEED="${PARVEB_RUN_SEED:-0}"
BITS="${PARVEB_RUN_BITS:-48}"
BUILD_TYPE="${PARVEB_BUILD_TYPE:-RelWithDebInfo}"
DEBUG_FLAGS="${PARVEB_DEBUG_FLAGS:--g -fno-omit-frame-pointer}"
PERF_EVENTS="${PERF_EVENTS:-cycles,instructions,branch-misses,cache-misses,L1-dcache-load-misses,dTLB-load-misses,LLC-load-misses}"
PERF_OUTPUT="${PERF_OUTPUT:-$LOG_DIR/perf_stat.txt}"
PERF_STAT_ARGS="${PERF_STAT_ARGS:---per-socket}"

mkdir -p "$LOG_DIR"

if ! command -v perf >/dev/null 2>&1; then
    echo "perf is required but not found in PATH" >&2
    exit 1
fi

export CXXFLAGS="$DEBUG_FLAGS ${CXXFLAGS:-}"
export CFLAGS="$DEBUG_FLAGS ${CFLAGS:-}"
PARVEB_BUILD_TYPE="$BUILD_TYPE" PARVEB_ENABLE_SIMD="$SIMD_OPTION" "$BUILD_SCRIPT"

if [[ ! -x "$RUN_BIN" ]]; then
    echo "Runner binary not found at $RUN_BIN" >&2
    exit 1
fi

echo "Collecting perf stat to $PERF_OUTPUT"
{
    echo "# perf stat"
    echo "timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    echo "simd=$SIMD_OPTION"
    echo "build_type=$BUILD_TYPE"
    echo "events=$PERF_EVENTS"
    echo "num_keys=$NUM_KEYS"
    echo "trials=$TRIALS"
    echo "seed=$SEED"
    echo "bits=$BITS"
    echo
    perf stat -e "$PERF_EVENTS" $PERF_STAT_ARGS \
        "$RUN_BIN" --num_keys="$NUM_KEYS" --trials="$TRIALS" --seed="$SEED" --bits="$BITS" "$@"
} | tee "$PERF_OUTPUT"

echo "Perf stat saved to $PERF_OUTPUT"
