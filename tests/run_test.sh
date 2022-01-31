#!/bin/sh

usage() {
  cat << HELP
Usage: run_test [--bless] TESTS...
Run tests from the test suite.

Each TEST should be the name of a TEST.in file in the current directory.
It calls 'metamath TEST.mm < TEST.in > TEST.produced' and compares
TEST.produced with TEST.expected, printing a test failure report.

If TEST.mm does not exist, then it just calls metamath without arguments.

The 'metamath' command can be modified by setting the METAMATH environment
variable.

The --bless option can be used to update the TEST.expected file to match
TEST.produced. Always review the changes after a call to run_test --bless.

TEST.in files have the syntax of metamath scripts, so leading ! can be used to
write comments. This script contains some special directives in tests:

* '! run_test' means that the output will be ignored: TEST.produced will not be
  created and TEST.expected does not have to exist.
* '! expect_fail' means that we expect metamath to return exit code 1
  instead of 0 on this input.
HELP
}

if [ "$1" = "--help" ]; then usage; exit; fi
if [ "$1" = "--bless" ]; then bless=1; shift; fi

cmd="${METAMATH:-metamath}"
red='\033[0;31m'
cyan='\033[0;36m'
green='\033[0;32m'
off='\033[0m'
exit_code=0
needs_bless=0

for test in "$@"; do
  test="`basename "$test" .in`"
  if [ ! -f "$test.in" ]; then echo "$test.in ${red}missing${off}"; exit 2; fi
  if grep -q "! run_test" "$test.in"; then
    outfile="/dev/null"
  else
    outfile="$test.produced"
  fi
  ! grep -q "! expect_fail" "$test.in"; expect=$?
  run_cmd() { if [ -f "$test.mm" ]; then "$cmd" "$test.mm"; else "$cmd"; fi }
  echo -n "running test $test.in: "
  result=$(run_cmd < "$test.in")
  if [ $? -ne $expect ]; then echo "${red}failed${off}"; exit_code=1; continue; fi
  echo "$result" | head -n -1 | tail -n +2 > "$outfile"
  if [ "$outfile" = "/dev/null" ]; then
    echo "${green}ok${off}"
  elif [ ! -f "$test.expected" ]; then
    if [ "$bless" = "1" ]; then
      echo "${cyan}blessed${off}"
      cp "$test.produced" "$test.expected"
    else
      echo "${red}failed${off}"; exit_code=1; needs_bless=1
    fi
  elif diff_result=$(diff "$outfile" "$test.expected" --color=always); then
    echo "${green}ok${off}"
  elif [ "$bless" = "1" ]; then
    echo "${cyan}blessed${off}"
    cp "$test.produced" "$test.expected"
  else
    echo "${red}failed${off}"; exit_code=1; needs_bless=1
    echo "$diff_result\n"
  fi
done

if [ $needs_bless -eq 1 ]; then
  echo "rerun with --bless to update the tests"
fi

exit $exit_code
