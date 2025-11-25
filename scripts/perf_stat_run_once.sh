#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
RUN_BIN="$REPO_ROOT/build/release/run_veb"
LOG_DIR="$REPO_ROOT/logs/perf"

# Tunables (override via env)
SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
BUILD_TYPE="${PARVEB_BUILD_TYPE:-RelWithDebInfo}"
DEBUG_FLAGS="${PARVEB_DEBUG_FLAGS:--g -fno-omit-frame-pointer}"
NUM_INSERTS="${PARVEB_RUN_INSERTS:-${PARVEB_RUN_KEYS:-10000000}}"
TRIALS="${PARVEB_RUN_TRIALS:-5}"
SEED="${PARVEB_RUN_SEED:-0}"
BITS="${PARVEB_RUN_BITS:-48}"
CPU_BIND="${PARVEB_CPU:-0}"
MEMNODE_BIND="${PARVEB_MEMNODE:-0}"
PERF_EVENTS="${PERF_EVENTS:-cycles,instructions,branch-misses,cache-misses,L1-dcache-load-misses,dTLB-load-misses,LLC-load-misses}"
PERF_OUTPUT="${PERF_OUTPUT:-$LOG_DIR/perf_stat.txt}"
PERF_FREQ="${PERF_FREQ:-4000}"
PERF_DATA="${PERF_DATA:-$LOG_DIR/perf.data}"

mkdir -p "$LOG_DIR"

if ! command -v perf >/dev/null 2>&1; then
    echo "perf is required but not found in PATH" >&2
    exit 1
fi

echo "Building ($BUILD_TYPE) with debug flags '$DEBUG_FLAGS'..."
export CXXFLAGS="$DEBUG_FLAGS ${CXXFLAGS:-}"
export CFLAGS="$DEBUG_FLAGS ${CFLAGS:-}"
PARVEB_BUILD_TYPE="$BUILD_TYPE" PARVEB_ENABLE_SIMD="$SIMD_OPTION" "$BUILD_SCRIPT"

if [[ ! -x "$RUN_BIN" ]]; then
    echo "Runner binary not found at $RUN_BIN" >&2
    exit 1
fi

RUN_PREFIX=(taskset -c "$CPU_BIND" numactl --cpunodebind="$MEMNODE_BIND" --membind="$MEMNODE_BIND")

echo "Collecting perf stat to $PERF_OUTPUT"
{
    echo "# perf stat"
    echo "timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    echo "simd=$SIMD_OPTION"
    echo "build_type=$BUILD_TYPE"
    echo "events=$PERF_EVENTS"
    echo "num_inserts=$NUM_INSERTS"
    echo "trials=$TRIALS"
    echo "seed=$SEED"
    echo "bits=$BITS"
    echo "cpu_bind=$CPU_BIND"
    echo "memnode_bind=$MEMNODE_BIND"
    echo
    perf stat -e "$PERF_EVENTS" \
        "${RUN_PREFIX[@]}" "$RUN_BIN" --num_inserts="$NUM_INSERTS" --trials="$TRIALS" --seed="$SEED" --bits="$BITS"
} 2>&1 | tee "$PERF_OUTPUT"

echo "Recording perf sample to $PERF_DATA"
perf record --call-graph=dwarf -F "$PERF_FREQ" -o "$PERF_DATA" -- \
    "${RUN_PREFIX[@]}" "$RUN_BIN" --num_inserts="$NUM_INSERTS" --trials="$TRIALS" --seed="$SEED" --bits="$BITS"

echo "Done. perf stat: $PERF_OUTPUT"
echo "Perf data: $PERF_DATA (view with scripts/perf_report_veb.sh or 'perf report -g -i $PERF_DATA')"
