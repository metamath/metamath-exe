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

# Each case runs with the working directory set to $workdir, so the
# binary needs a path that still resolves there.  A relative one does
# not, and it fails quietly: the exec error goes to $out, matches
# nothing the grep below looks for, and the case reads as ok.  $PWD is
# enough, since this wants an absolute path and not a canonical one.
case $binary in
  /*) ;;
  *) binary=$PWD/$binary ;;
esac

# macOS ships no timeout(1) at all.  Homebrew's coreutils installs it
# as gtimeout, so take that when it is the one present, and say so
# plainly when neither is: a case that never runs has to look like a
# failure here, not like a pass.
if command -v timeout >/dev/null 2>&1; then
  timeout_cmd=timeout
elif command -v gtimeout >/dev/null 2>&1; then
  timeout_cmd=gtimeout
else
  echo "no timeout found; on macOS, brew install coreutils" >&2
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
         | (cd "$workdir" && "$timeout_cmd" 60 "$binary" 2>&1) )
  rc=$?

  # "?BUG CHECK" counts too.  bug() is the program catching itself in a
  # state it thought impossible.  Nothing has been misused in memory, so
  # the sanitizers stay quiet, and this judges by output alone: without
  # this a case that trips bug() and nothing else was reported passing,
  # whatever the program then did.
  if printf '%s' "$out" \
       | grep -qE 'runtime error|AddressSanitizer|SEGV|\?BUG CHECK'; then
    result=DETECTED
  # metamath exits 0 or 1 and nothing else, so any other status means the
  # run did not finish: killed by the timeout, killed by a signal, or
  # never started.  Such a run prints nothing the grep matches, so it has
  # to be caught here or it reads as ok.
  elif [ "$rc" -gt 1 ]; then
    result="did not finish (status $rc)"
  else
    result=clean
  fi

  case "$name" in
    open-*)
      if [ "$result" != clean ]; then
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
