#!/bin/sh
# Run every case in cases/ against a sanitizer-built metamath and report
# whether the sanitizers still complain.
#
# Cases named "open-*" are known-unfixed bugs and are EXPECTED to fail.
# Every other case is a fixed bug and is expected to be clean; if one of
# those starts failing again, something regressed.
#
# Usage:
#   ./run-cases.sh [path-to-sanitizer-metamath]

set -u

here=$(cd "$(dirname "$0")" && pwd)
binary=${1:-"$here/metamath-san"}

if [ ! -x "$binary" ]; then
  echo "$binary is not executable; run ./build-sanitizer.sh first" >&2
  exit 2
fi

ASAN_OPTIONS=detect_leaks=0:abort_on_error=0
UBSAN_OPTIONS=print_stacktrace=1
export ASAN_OPTIONS UBSAN_OPTIONS

workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT
cp "$here"/cases/* "$workdir"/

status=0
unexpected=0

for cmd in "$here"/cases/*.cmd; do
  name=$(basename "$cmd" .cmd)
  # Each .cmd holds the commands to run after startup.
  out=$( { printf 'set scroll continuous\n'; cat "$cmd"; printf 'exit\n'; } \
         | (cd "$workdir" && timeout 60 "$binary" 2>&1) )

  if printf '%s' "$out" \
       | grep -qE 'runtime error|AddressSanitizer|SEGV'; then
    result=DETECTED
  else
    result=clean
  fi

  case "$name" in
    open-*)
      if [ "$result" = DETECTED ]; then
        echo "known-open  $name: $result (expected)"
      else
        echo "FIXED?      $name: clean -- was expected to still fail"
        unexpected=1
      fi
      ;;
    *)
      if [ "$result" = clean ]; then
        echo "ok          $name"
      else
        echo "REGRESSION  $name: $result"
        status=1
      fi
      ;;
  esac
done

if [ $unexpected -ne 0 ]; then
  echo
  echo "Note: an 'open' case now passes.  If it was fixed on purpose,"
  echo "rename it to drop the 'open-' prefix so it becomes a regression"
  echo "case, and update fuzz/README.md."
fi

exit $status
