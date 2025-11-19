#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$REPO_ROOT/scripts/build_release.sh"
BENCH_BIN="$REPO_ROOT/build/release/veb_benchmark"

SIMD_OPTION="${PARVEB_ENABLE_SIMD:-ON}"
HOST_FULL="$(hostname -s 2>/dev/null || hostname)"
HOSTNAME="${HOST_FULL%%.*}"
LOG_DIR="$REPO_ROOT/logs/$HOSTNAME"

mkdir -p "$LOG_DIR"

PARVEB_ENABLE_SIMD="$SIMD_OPTION" "$BUILD_SCRIPT"

if [[ ! -x "$BENCH_BIN" ]]; then
    echo "Benchmark binary not found at $BENCH_BIN" >&2
    exit 1
fi

TIMESTAMP_UTC="$(date -u +"%Y%m%d_%H%M%S")"
COMMIT_HASH="$(git -C "$REPO_ROOT" rev-parse --short HEAD)"
LOG_FILE="$LOG_DIR/benchmark_${TIMESTAMP_UTC}_${COMMIT_HASH}.log"

{
    echo "# Benchmark log"
    echo "timestamp_utc=$TIMESTAMP_UTC"
    echo "commit=$COMMIT_HASH"
    echo "simd=$SIMD_OPTION"
    echo "build_type=Release"
    echo "hostname=$HOSTNAME"
    echo
    "$BENCH_BIN" "$@"
} | tee "$LOG_FILE"

echo "Benchmark output saved to $LOG_FILE"
