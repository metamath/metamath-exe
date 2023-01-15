#!/bin/sh

usage() {
  cat >&2 <<"HELP"
Usage: run_test [-c CMD] [--bless] TESTS...
Run tests from the test suite.

Each TEST should be the name of a TEST.in file in the current directory.  It
calls 'metamath TEST.mm < TEST.in > TEST.produced' and compares TEST.produced
with TEST.expected, printing a test failure report.

If TEST.mm does not exist, then it just calls metamath without arguments.

The 'metamath' command can be modified by setting the METAMATH environment
variable, or via the '-c CMD' option (which takes priority).

The --bless option can be used to update the TEST.expected file to match
TEST.produced.  Always review the changes after a call to run_test --bless.

TEST.in files have the syntax of metamath scripts, so leading ! can be used to
write comments.  This script contains some special directives in tests:

* '! run_test' means that the output will be ignored: TEST.produced will not be
  created and TEST.expected does not have to exist.
* '! expect_fail' means that we expect metamath to return exit code 1 instead
  of 0 on this input.
HELP
}

if [ "$1" = "--help" ]; then usage; exit; fi

# Allow overriding the 'metamath' command using the METAMATH env variable
cmd="${METAMATH:-metamath}"

# Alternatively, use the '-c metamath' argument which overrides the env variable
if [ "$1" = "-c" ]; then shift; cmd=$1; shift; fi

if [ "$1" = "--bless" ]; then bless=1; shift; fi

# Check that the 'metamath' command actually exists
if ! [ -x "$(command -v "$cmd")" ]; then
  echo >&2 "'$cmd' not found on the PATH."
  if [ "$METAMATH" != "" ]; then
    echo >&2 "note: The METAMATH environment variable is set to '$METAMATH'."
  fi
  echo >&2 "Try 'METAMATH=path/to/metamath ./run_test.sh ...', or check your installation"
  exit 2
fi

# color codes
escape=$(printf '\033')
red="$escape[0;31m"
green="$escape[0;32m"
cyan="$escape[0;36m"
white="$escape[0;97m"
off="$escape[0m"

# set to 1 if any test fails
exit_code=0
# set to 1 if rerunning with --bless would help
needs_bless=0

for test in "$@"; do
  # strip .in from the test name if necessary
  test=$(basename "$test" .in)

  # The test file input is the only required part.
  # If it doesn't exist then it's a hard error and we abort the test run
  if [ ! -f "$test.in" ]; then echo >&2 "$test.in ${red}missing${off}"; exit 2; fi

  # For tests with `! run_test` in them, we want to ignore the output
  if grep -q "! run_test" "$test.in"; then
    outfile="/dev/null"
  else
    outfile="$test.produced"
  fi

  # For tests with `! expect_fail` we expect exit code 1 instead of 0
  ! grep -q "! expect_fail" "$test.in"; expect=$?

  # Actually run the program
  if [ -f "$test.mm" ]; then
    echo -n "test ${white}$test${off}.in + $test.mm: "
    result=$(echo 'set scroll continuous' |
      cat - "$test.in" |
      "$cmd" "$test.mm")
  else
    echo -n "test ${white}$test${off}.in: "
    result=$(echo 'set scroll continuous' |
      cat - "$test.in" |
      "$cmd")
  fi
  result_code=$?

  if [ $result_code -ne $expect ]; then
    # If the exit code is wrong, the test is a failure and --bless won't help
    echo "${red}failed${off} (exit code = $result_code)"; exit_code=1
    if [ "$outfile" != "/dev/null" ]; then
      echo "---------------------------------------"
      cat "$test.produced"
      echo "---------------------------------------\n"
    fi
    continue
  fi

  # Strip the first and last line of the output.
  # The first line is always something like:
  #   Metamath - Version 0.199.pre 7-Aug-2021  Type HELP for help, EXIT to exit.
  # which is problematic because we would have to fix all the tests every time
  # a new version comes out.
  # The last line is always 'MM> EXIT' which is not relevant.
  echo "$result" | head -n -1 | tail -n +2 > "$outfile"

  if [ "$outfile" = "/dev/null" ]; then
    # If this is a run_test then we're done
    echo "${green}ok${off}"
  elif [ ! -f "$test.expected" ]; then
    # If the $test.expected file doesn't exist:
    if [ "$bless" = "1" ]; then
      # if we are in bless mode then copy the $test.produced and report success
      echo "${cyan}blessed${off}"
      cp "$test.produced" "$test.expected"
      echo "$test.expected:"
    else
      # otherwise this is a fail, and blessing will help
      echo "${red}failed${off}"; exit_code=1; needs_bless=1
      echo "$test.expected missing, $test.produced:"
    fi
    echo -n "---------------------------------------\n${green}"
    cat "$test.produced"
    echo "${off}---------------------------------------\n"
  # call diff and put the diff output in $diff_result
  elif diff_result=$(diff "$test.expected" "$outfile" --color=always); then
    # If it succeeded (the files are the same), then the test passed
    echo "${green}ok${off}"
  else
    if [ "$bless" = "1" ]; then
      # If the files are different but we are in bless mode then it's still okay,
      # but report that we've modified the $test.expected file
      echo "${cyan}blessed${off}"
      cp "$test.produced" "$test.expected"
      echo "$test.expected changes:"
    else
      # If the files are different and we aren't in bless mode
      # then this is a fail, but blessing will help
      echo "${red}failed${off}"; exit_code=1; needs_bless=1

      # Give a verbose error report, including the file names being diffed
      echo "$test.expected and $test.produced differ"
    fi
    echo "---------------------------------------"
    echo "$diff_result"
    echo "---------------------------------------\n"
  fi
done

# Remind the user about the --bless option if it looks like it might help
if [ $needs_bless -eq 1 ]; then
  echo "run './run_test.sh --bless *.in' to update the test suite"
fi

exit $exit_code
