#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
RUN_BIN="$REPO_ROOT/build/release/run_veb"
LOG_DIR="$REPO_ROOT/logs/perf"

SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
NUM_INSERTS="${PARVEB_RUN_INSERTS:-${PARVEB_RUN_KEYS:-10000000}}"
TRIALS="${PARVEB_RUN_TRIALS:-5}"
SEED="${PARVEB_RUN_SEED:-0}"
BITS="${PARVEB_RUN_BITS:-48}"
PERF_OUTPUT="${PERF_OUTPUT:-$LOG_DIR/perf.data}"
PERF_FREQ="${PERF_FREQ:-4000}"
PERF_RECORD_ARGS="${PERF_RECORD_ARGS:-}"
BUILD_TYPE="${PARVEB_BUILD_TYPE:-RelWithDebInfo}"
DEBUG_FLAGS="${PARVEB_DEBUG_FLAGS:--g -fno-omit-frame-pointer}"
BIND_CPU="${PARVEB_CPU:-}"
BIND_MEMNODE="${PARVEB_MEMNODE:-}"

RUN_PREFIX=()
if [[ -n "$BIND_MEMNODE" ]]; then
    RUN_PREFIX+=(numactl --cpunodebind="$BIND_MEMNODE" --membind="$BIND_MEMNODE")
fi
if [[ -n "$BIND_CPU" ]]; then
    RUN_PREFIX+=(taskset -c "$BIND_CPU")
fi

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

echo "Recording perf with dwarf call graph to $PERF_OUTPUT"
perf record --call-graph=dwarf -F "$PERF_FREQ" -o "$PERF_OUTPUT" $PERF_RECORD_ARGS \
    -- "${RUN_PREFIX[@]}" "$RUN_BIN" --num_inserts="$NUM_INSERTS" --trials="$TRIALS" --seed="$SEED" --bits="$BITS" "$@"

if [[ "${PERF_REPORT:-0}" == "1" ]]; then
    REPORT_FILE="$LOG_DIR/perf_report.txt"
    perf report --stdio --input="$PERF_OUTPUT" --call-graph --no-children >"$REPORT_FILE"
    echo "Perf report written to $REPORT_FILE"
fi

echo "Perf data recorded at $PERF_OUTPUT"
