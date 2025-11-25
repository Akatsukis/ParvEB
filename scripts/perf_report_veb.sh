#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$REPO_ROOT/logs/perf"
PERF_DATA="${PERF_DATA:-$LOG_DIR/perf.data}"

if ! command -v perf >/dev/null 2>&1; then
    echo "perf is required but not found in PATH" >&2
    exit 1
fi

if [[ ! -f "$PERF_DATA" ]]; then
    echo "perf data file not found at $PERF_DATA" >&2
    exit 1
fi

perf report -g -i "$PERF_DATA" "$@"
