#!/bin/sh
# smoke_databases.sh - check metamath against the real databases
#
# Verifies every proof in set.mm and iset.mm, then generates a few web pages
# from each in both output formats.  Intended as a pull request gate: it runs
# in about twenty seconds, where generating every page of set.mm, some fifty
# thousand of them, takes far longer than a pull request should wait.
#
# This does not replace the tests in this directory, and the two are not
# redundant.  Those run against small fixed databases and compare the
# generated output against a recorded expectation, so they notice a page that
# is quietly wrong.  Output cannot be compared that way here, because pages
# built from set.mm embed lists that change with nearly every commit to it, so
# what this checks is that the real databases parse, verify and render without
# failing.  Issue #196 was a case where a page came out wrong rather than
# missing, so keep both.
#
# Usage: smoke_databases.sh METAMATH SETMM_DIR
#
#   METAMATH    the metamath executable to test
#   SETMM_DIR   directory holding set.mm and iset.mm
#
# Set WORK to choose where the generated pages go.

set -eu

if [ $# -ne 2 ]; then
  echo >&2 "Usage: $0 METAMATH SETMM_DIR"
  exit 2
fi

MM=$1
DBDIR=$2
WORK=${WORK:-,smoke}

fail() { printf '%s: %s\n' "$0" "$1" >&2; exit 1; }

# Each database is generated in a working directory of its own, so anything
# named relative to where this was started has to be resolved before then.  A
# bare command name is left alone, so that it keeps being looked up on PATH.
case "$MM" in
  */*)
    mmdir=$(cd "$(dirname "$MM")" 2>/dev/null && pwd) \
      || fail "cannot resolve the path to '$1'"
    MM="$mmdir/$(basename "$MM")"
    ;;
esac
DBDIR=$(cd "$DBDIR" 2>/dev/null && pwd) || fail "no such directory '$2'"

[ -x "$MM" ] || command -v "$MM" >/dev/null 2>&1 \
  || fail "no metamath executable at '$1'"

# The labels cover each kind of page that SHOW STATEMENT builds differently,
# because the kind decides which code runs.  A few arbitrary pages would be a
# much weaker check.  All of them are foundational names that are not going to
# be renamed or removed.
#
#   wi, wceq   syntax statements ($a not starting with "|-")
#   ax-1       axiom, primitive syntax, no hypotheses
#   ax-mp      axiom with $e hypotheses
#   df-bi      definition, that is "|-" with a label not starting with "ax-"
#   df-an      another definition, set.mm only
#   id, a1i    theorems with proofs
#   mp2        theorem built on other theorems
#   alrimiv    theorem carrying a $d condition
#
# Axiom and definition pages are the ones that carry a syntax breakdown, which
# is where issue #196 failed; syntax and theorem pages take other paths.
set_labels="wi wceq ax-1 ax-mp df-bi df-an id a1i mp2 alrimiv"
iset_labels="wi wceq ax-1 ax-mp df-bi id a1i mp2 alrimiv"

# This removes the directory, so refuse the values that would take too much
# with them.
case "$WORK" in
  ""|/|.|..) fail "refusing to use '$WORK' as the working directory" ;;
esac

rm -rf "$WORK"
mkdir -p "$WORK"

# Run one database in one output format.  The two formats get a directory
# each: both write a page under the same name, and running them together would
# leave only the second one to look at, so only one of them would be checked.
# "verify proof *" is asked for in the first format only, being the slow part
# and the same work either way.
run_db_format() {
  db=$1
  labels=$2
  fmt=$3
  verify=$4
  d="$WORK/$db-$fmt"

  mkdir -p "$d"
  # metamath reads a "/" on a command line as the start of a qualifier, so a
  # database with a path in its name cannot be named in a READ command.  Pass
  # it as an argument instead, against a link in the working directory.
  ln -sf "$DBDIR/$db.mm" "$d/$db.mm"

  { echo 'set scroll continuous'
    if [ "$verify" = yes ]; then echo 'verify proof *'; fi
    for lab in $labels; do
      echo "show statement $lab/$fmt"
    done
    echo 'exit'
  } > "$d/cmd.txt"

  ( cd "$d" && "$MM" "$db.mm" < cmd.txt > out.log 2>&1 ) \
    || fail "$db.mm /$fmt: metamath exited nonzero, see $d/out.log"

  if [ "$verify" = yes ]; then
    grep -q 'All proofs in the database were verified' "$d/out.log" \
      || fail "$db.mm: proofs were not all verified, see $d/out.log"
  fi

  # A bug check that gets answered, or an error that does not stop the run,
  # can still leave the exit status at zero, so look at what was reported.
  if grep -qE '\?BUG CHECK|\?Error' "$d/out.log"; then
    fail "$db.mm /$fmt: $(grep -m 1 -E '\?BUG CHECK|\?Error' "$d/out.log")"
  fi

  # A page that comes out as an empty shell would otherwise pass unnoticed.
  for lab in $labels; do
    [ -f "$d/$lab.html" ] || fail "$db.mm /$fmt: no page was generated for $lab"
    grep -q 'Detailed syntax breakdown\|Proof of Theorem\|Syntax Definition' \
      "$d/$lab.html" \
      || fail "$db.mm /$fmt: $lab.html has neither a breakdown nor a proof"
  done
}

run_db() {
  db=$1
  labels=$2

  [ -f "$DBDIR/$db.mm" ] || fail "$DBDIR/$db.mm not found"
  run_db_format "$db" "$labels" html yes
  run_db_format "$db" "$labels" alt_html no

  count=0
  for lab in $labels; do
    count=$((count + 1))
  done
  echo "$db.mm ok: all proofs verified, $count pages in 2 formats"
}

run_db set "$set_labels"
run_db iset "$iset_labels"
echo "smoke_databases: passed"
