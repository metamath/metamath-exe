#!/bin/sh
# Build metamath with sanitizers:
#
#   metamath-san   AddressSanitizer + UndefinedBehaviorSanitizer, with $CC
#   metamath-msan  MemorySanitizer, which reports reads of uninitialized
#                  memory, with $MSAN_CC.  It cannot share a binary with
#                  AddressSanitizer, and is skipped where the compiler lacks
#                  it (gcc, or clang on macOS).
#
# The sanitizers are the fuzzing oracle: without them a malformed .mm
# file just produces an error message, which is correct behavior, and
# the fuzzer has nothing to detect.
#
# Usage:
#   ./build-sanitizer.sh [output-binary] [extra compiler flags...]
#
# By default this writes ./metamath-san, which is what fuzz.py expects.
# The extra flags go to it only.  ./metamath-msan is always written here.

set -eu

here=$(cd "$(dirname "$0")" && pwd)
src="$here/../src"
out=${1:-"$here/metamath-san"}
[ $# -gt 0 ] && shift || true

CC=${CC:-gcc}
MSAN_CC=${MSAN_CC:-clang}

# -O1 keeps the sanitizer build reasonably fast while still giving
# usable line numbers.  -fno-omit-frame-pointer gives readable stacks.
# Note we deliberately do NOT use -fno-sanitize-recover here: letting a
# UBSan report continue lets one run surface more than one problem.
$CC -g -O1 -fno-omit-frame-pointer \
    -fsanitize=address,undefined \
    -o "$out" "$src"/*.c "$@"

echo "built $out"

# The sources have just compiled, so a failure here means that $MSAN_CC
# cannot build with MemorySanitizer.  Remove any older binary first, so
# that run-cases.sh does not run a stale one.
rm -f "$here/metamath-msan"
if "$MSAN_CC" -g -O1 -fno-omit-frame-pointer \
     -fsanitize=memory,undefined -fsanitize-memory-track-origins=2 \
     -o "$here/metamath-msan" "$src"/*.c 2>/dev/null; then
  echo "built $here/metamath-msan"
else
  echo "skipped $here/metamath-msan: $MSAN_CC cannot build with MemorySanitizer"
fi
