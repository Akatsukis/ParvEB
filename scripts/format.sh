#!/usr/bin/env bash

set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "Error: clang-format is not installed or not on PATH." >&2
  exit 1
fi

# Gather all tracked files that we own and that clang-format understands.
readarray -t files < <(git ls-files \
  -- '*.c' '*.cc' '*.cxx' '*.cpp' '*.h' '*.hh' '*.hpp' '*.hxx' \
  ':(exclude)third_party/**')

if [[ "${#files[@]}" -eq 0 ]]; then
  echo "No C/C++ sources were found to format."
  exit 0
fi

if [[ "${CHECK_ONLY:-0}" != "0" ]]; then
  echo "Running clang-format in check mode..."
  clang-format --dry-run -Werror "${files[@]}"
else
  echo "Formatting ${#files[@]} file(s) with clang-format..."
  clang-format -i "${files[@]}"
fi
