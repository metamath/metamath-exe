#!/usr/bin/env python3
"""Shrink a crashing .mm file down to something readable.

Repeatedly deletes chunks of lines, keeping any deletion that still
reproduces the failure.  Fuzzer output is typically a few hundred lines
of mutated database; this usually gets it into single digits.

"Still reproduces" means the marker string still appears in the output.
Use the crashing function name from the sanitizer report -- that keeps
the reduction honest, so it does not wander off into some unrelated
crash.

Example:
    ./minimize.py --input findings/crash_1_279/crash.mm \\
                  --command 'show statement * /alt_html' \\
                  --marker makeSubstUnif
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


def reproduces(binary, workdir, lines, command, marker, timeout):
    path = os.path.join(workdir, 'm.mm')
    with open(path, 'w', errors='surrogateescape') as f:
        f.writelines(lines)
    script = "set scroll continuous\nread m.mm\n" + command + "\nexit\n"
    env = dict(os.environ)
    env['ASAN_OPTIONS'] = 'detect_leaks=0:abort_on_error=0'
    env['UBSAN_OPTIONS'] = 'print_stacktrace=1'
    try:
        proc = subprocess.run([binary], input=script.encode(), cwd=workdir,
                              capture_output=True, timeout=timeout, env=env)
    except subprocess.TimeoutExpired:
        return False
    return marker.encode() in (proc.stdout + proc.stderr)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--binary', default=os.path.join(here, 'metamath-san'))
    parser.add_argument('--input', required=True, help='crashing .mm file')
    parser.add_argument('--command', required=True,
                        help='metamath command to run after READ')
    parser.add_argument('--marker', required=True,
                        help='string that must stay in the output, e.g. the '
                             'crashing function name')
    parser.add_argument('--output', default=None,
                        help='where to write the reduced file '
                             '(default: <input>.min.mm)')
    parser.add_argument('--timeout', type=int, default=30)
    args = parser.parse_args()

    out_path = args.output or (args.input + '.min.mm')
    with open(args.input, errors='surrogateescape') as f:
        lines = f.readlines()

    workdir = tempfile.mkdtemp(prefix='mm-minimize-')
    try:
        if not reproduces(args.binary, workdir, lines, args.command,
                          args.marker, args.timeout):
            sys.exit("input does not reproduce: marker %r not seen. Check "
                     "--command and --marker." % args.marker)
        print("start: %d lines" % len(lines), flush=True)

        changed = True
        while changed:
            changed = False
            i = 0
            while i < len(lines):
                for chunk in (16, 8, 4, 2, 1):
                    if chunk > len(lines) - i:
                        continue
                    trial = lines[:i] + lines[i + chunk:]
                    if reproduces(args.binary, workdir, trial, args.command,
                                  args.marker, args.timeout):
                        lines = trial
                        changed = True
                        print("  %d lines" % len(lines), flush=True)
                        break
                else:
                    i += 1

        with open(out_path, 'w', errors='surrogateescape') as f:
            f.writelines(lines)
        print("\nreduced to %d lines -> %s\n" % (len(lines), out_path))
        sys.stdout.writelines(lines)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


if __name__ == '__main__':
    main()
