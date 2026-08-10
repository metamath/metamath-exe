#!/bin/sh
# Build a metamath binary with AddressSanitizer + UndefinedBehaviorSanitizer.
#
# The sanitizers are the fuzzing oracle: without them a malformed .mm
# file just produces an error message, which is correct behavior, and
# the fuzzer has nothing to detect.
#
# Usage:
#   ./build-sanitizer.sh [output-binary] [extra compiler flags...]
#
# By default this writes ./metamath-san, which is what fuzz.py expects.

set -eu

here=$(cd "$(dirname "$0")" && pwd)
src="$here/../src"
out=${1:-"$here/metamath-san"}
[ $# -gt 0 ] && shift || true

CC=${CC:-gcc}

# -O1 keeps the sanitizer build reasonably fast while still giving
# usable line numbers.  -fno-omit-frame-pointer gives readable stacks.
# Note we deliberately do NOT use -fno-sanitize-recover here: letting a
# UBSan report continue lets one run surface more than one problem.
$CC -g -O1 -fno-omit-frame-pointer \
    -fsanitize=address,undefined \
    -o "$out" "$src"/*.c "$@"

echo "built $out"
