#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
BENCH_BIN="$REPO_ROOT/build/release/veb_benchmark"

SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
POOL_OPTION="${PARVEB_ENABLE_POOL:-ON}"
HOST_FULL="$(hostname -s 2>/dev/null || hostname)"
HOSTNAME="${HOST_FULL%%.*}"
LOG_DIR="$REPO_ROOT/logs/$HOSTNAME"

mkdir -p "$LOG_DIR"

PARVEB_ENABLE_SIMD="$SIMD_OPTION" PARVEB_ENABLE_POOL="$POOL_OPTION" "$BUILD_SCRIPT"

if [[ ! -x "$BENCH_BIN" ]]; then
    echo "Benchmark binary not found at $BENCH_BIN" >&2
    exit 1
fi

COMMIT_HASH="$(git -C "$REPO_ROOT" rev-parse --short HEAD)"
TIMESTAMP_UTC="$(date -u +"%Y%m%d_%H%M%S")"
LOG_FILE="$LOG_DIR/benchmark_${TIMESTAMP_UTC}_${COMMIT_HASH}.log"

{
    echo "# Benchmark log"
    echo "timestamp_utc=$TIMESTAMP_UTC"
    echo "commit=$COMMIT_HASH"
    echo "simd=$SIMD_OPTION"
    echo "memory_pool=$POOL_OPTION"
    echo "build_type=Release"
    echo "hostname=$HOSTNAME"
    echo
    echo "## git diff"
    git -C "$REPO_ROOT" diff
    echo
} | tee "$LOG_FILE"

run_benchmark_case() {
    local -a run_args=("$@")
    local args_serialized="${run_args[*]}"
    local run_timestamp="$(date -u +"%Y%m%d_%H%M%S")"

    {
        echo "## benchmark_run"
        echo "run_timestamp_utc=$run_timestamp"
        echo "args=$args_serialized"
        echo
        "$BENCH_BIN" "${run_args[@]}"
        echo
    } | tee -a "$LOG_FILE"
}

if [[ $# -gt 0 ]]; then
    run_benchmark_case "$@"
    echo "Benchmark output saved to $LOG_FILE"
    exit 0
fi

run_benchmark_case --distribution=uniform --skew=1.0

exp_skews=(0.2 0.4 0.6 0.8 1.0)
for skew in "${exp_skews[@]}"; do
    run_benchmark_case --distribution=exponential --skew="$skew"
done

zipf_skews=(0.6 0.8 1.0 1.2 1.5)
for skew in "${zipf_skews[@]}"; do
    run_benchmark_case --distribution=zipfian --skew="$skew"
done

echo "Benchmark output saved to $LOG_FILE"
