#!/usr/bin/env python3
"""Mutation fuzzer for metamath.

Mutates .mm files from a seed corpus, feeds each one to a metamath
binary along with a randomly chosen command, and reports anything the
sanitizers complain about.  See README.md in this directory.

The binary under test MUST be built with AddressSanitizer and/or
UndefinedBehaviorSanitizer -- they are the oracle.  Without them a
malformed .mm just produces an error message, which is correct
behavior, and this script will never find anything.

Each run picks a random base seed and prints it, so repeat runs explore
new ground by default.  Pass that seed back with --seed to replay a run
exactly.

Example:
    ./build-sanitizer.sh
    ./fuzz.py --workers 6 --duration 2h
    ./fuzz.py --workers 6 --duration 2h --seed 819273465   # replay it
"""

import argparse
import os
import random
import shutil
import subprocess
import sys
import time

# Bytes worth substituting.  Biasing toward characters that mean
# something to metamath is what makes a fuzzer this simple productive;
# uniformly random bytes are almost always rejected by the tokenizer.
BYTE_ALPHABET = b'$ \n\t()[]{}?.abc-_0123456789\x00'

# Token-sized insertions, for the same reason.
TOKEN_INSERTS = [
    b'$(', b'$)', b'$[', b'$]', b'${', b'$}', b'$c', b'$v', b'$e',
    b'$f', b'$d', b'$a', b'$p', b'$.', b'$=', b'$$', b'$t', b']$',
    b'\n\n', b'  ', b'\x00', b'?', b'|-',
]

# Commands to run after READ.  Each entry is a complete command script
# fragment; "exit" is appended by the driver.  Extend this list to
# reach code that is not currently covered -- most bugs found so far
# were reachable only from one specific command.
COMMANDS = [
    "verify proof *\n",
    "verify markup *\n",
    "show statement * /full\n",
    "show statement * /comment\n",
    "show statement * /html\n",
    "show statement * /alt_html\n",
    "show statement * /tex\n",
    "show statement * /mnemonics\n",
    "show proof * /normal\n",
    "show proof * /compressed\n",
    "show proof * /lemmon/renumber\n",
    "show proof * /packed/essential\n",
    "show usage *\n",
    "show trace_back *\n",
    "show discouraged\n",
    "search * \"*\" /all\n",
    "write source out.mm\n",
    "write source out.mm /rewrap\n",
    "write source out.mm /extract *\n",
    "write theorem_list\n",
    "minimize_with *\n",
    "prove *\nshow new_proof /unknown\nexit\n",
]

# A seed that always parses, used if a mutation deletes everything.
FALLBACK = b'$c a $.\n'

# Strings in the output that mean we found something.
MARKERS = (b'runtime error', b'AddressSanitizer', b'SEGV', b'LeakSanitizer')


def parse_duration(text):
    """Accepts 90, 90s, 15m, 2h, 1.5h.  Returns seconds."""
    units = {'s': 1, 'm': 60, 'h': 3600}
    given = text  # keep the original for the error message
    scale = 1
    if text and text[-1].lower() in units:
        scale = units[text[-1].lower()]
        text = text[:-1]
    try:
        value = float(text)
    except ValueError:
        raise argparse.ArgumentTypeError(
            "expected something like 90s, 15m or 2h, or a plain number"
            " of seconds, not %r" % given)
    if value <= 0:
        raise argparse.ArgumentTypeError("duration must be positive")
    return value * scale


def load_seeds(seed_dir):
    seeds = []
    for name in sorted(os.listdir(seed_dir)):
        if name.endswith('.mm'):
            with open(os.path.join(seed_dir, name), 'rb') as f:
                data = f.read()
            if data:
                seeds.append(data)
    if not seeds:
        sys.exit("no .mm seeds found in %s" % seed_dir)
    return seeds


def mutate(rng, seed):
    """Apply 1-6 random edits to a copy of seed."""
    buf = bytearray(seed)
    for _ in range(rng.randint(1, 6)):
        if not buf:
            buf = bytearray(FALLBACK)
        pos = rng.randrange(len(buf))
        roll = rng.random()
        if roll < 0.40:
            buf[pos] = rng.choice(BYTE_ALPHABET)
        elif roll < 0.70:
            del buf[pos:pos + rng.randint(1, 20)]
        else:
            buf[pos:pos] = rng.choice(TOKEN_INSERTS)
    return bytes(buf) if buf else FALLBACK


def run_once(binary, workdir, mm_bytes, command, timeout):
    """Returns the combined output, or None if the run timed out."""
    with open(os.path.join(workdir, 't.mm'), 'wb') as f:
        f.write(mm_bytes)
    script = "set scroll continuous\nread t.mm\n" + command + "exit\n"
    env = dict(os.environ)
    # Leaks are not what we are looking for and metamath has many.
    env['ASAN_OPTIONS'] = 'detect_leaks=0:abort_on_error=0'
    env['UBSAN_OPTIONS'] = 'print_stacktrace=1'
    try:
        proc = subprocess.run([binary], input=script.encode(), cwd=workdir,
                              capture_output=True, timeout=timeout, env=env)
    except subprocess.TimeoutExpired:
        return None
    return proc.stdout + proc.stderr


def save_finding(outdir, tag, mm_bytes, command, output):
    where = os.path.join(outdir, tag)
    os.makedirs(where, exist_ok=True)
    with open(os.path.join(where, 'crash.mm'), 'wb') as f:
        f.write(mm_bytes)
    with open(os.path.join(where, 'crash.cmd'), 'w') as f:
        f.write("set scroll continuous\nread crash.mm\n" + command + "exit\n")
    with open(os.path.join(where, 'output.txt'), 'wb') as f:
        f.write(output)
    return where


def worker(args, worker_id):
    # Worker N draws from seed+N, so reporting the base seed is enough to
    # replay every worker.  Print this one too, so a finding can be traced
    # to the exact stream that produced it.
    seed = args.seed + worker_id
    print('worker %d: seed %d' % (worker_id, seed), flush=True)
    rng = random.Random(seed)
    seeds = load_seeds(args.seeds)
    workdir = os.path.join(args.output, 'work%d' % worker_id)
    os.makedirs(workdir, exist_ok=True)
    # The deadline is only checked between runs, so a run already under way
    # can overshoot it by up to --timeout.
    deadline = None if args.duration is None else time.monotonic() + args.duration
    found = 0
    done = 0
    while True:
        if args.iterations is not None and done >= args.iterations:
            break
        if deadline is not None and time.monotonic() >= deadline:
            break
        mm_bytes = mutate(rng, rng.choice(seeds))
        command = rng.choice(COMMANDS)
        output = run_once(args.binary, workdir, mm_bytes, command,
                          args.timeout)
        tag = 'crash_%d_%d' % (worker_id, done)
        done += 1
        if output is None:
            continue  # timeout; not interesting by itself
        if any(m in output for m in MARKERS):
            found += 1
            where = save_finding(args.output, tag, mm_bytes, command, output)
            print('FOUND %s' % where, flush=True)
            if found >= args.max_findings:
                break
    print('worker %d done: %d iterations, %d findings, seed %d'
          % (worker_id, done, found, seed), flush=True)
    return found


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--binary', default=os.path.join(here, 'metamath-san'),
                        help='sanitizer-built metamath (default: ./metamath-san)')
    parser.add_argument('--seeds',
                        default=os.path.join(here, os.pardir, 'tests'),
                        help='directory of .mm seed files (default: ../tests)')
    parser.add_argument('--output', default=os.path.join(here, 'findings'),
                        help='where to write findings (default: ./findings)')
    parser.add_argument('--iterations', type=int, default=None,
                        help='iterations per worker (default: 1000, or'
                             ' unlimited if --duration is given)')
    parser.add_argument('--duration', type=parse_duration, default=None,
                        help='wall-clock limit per worker, e.g. 90s, 15m, 2h;'
                             ' checked between runs, so the last run can'
                             ' overshoot by up to --timeout')
    parser.add_argument('--workers', type=int, default=1,
                        help='parallel workers (default: 1)')
    parser.add_argument('--seed', type=int, default=None,
                        help='base RNG seed; workers use seed+N.  Default is'
                             ' a fresh random seed each run, reported on'
                             ' startup so it can be passed back here to'
                             ' replay that run exactly')
    parser.add_argument('--timeout', type=int, default=25,
                        help='seconds per run (default: 25)')
    parser.add_argument('--max-findings', type=int, default=40,
                        help='stop a worker after this many (default: 40)')
    parser.add_argument('--worker-id', type=int, default=None,
                        help=argparse.SUPPRESS)  # used when re-execing
    args = parser.parse_args()

    if not os.access(args.binary, os.X_OK):
        sys.exit("%s is not executable; run ./build-sanitizer.sh first"
                 % args.binary)
    os.makedirs(args.output, exist_ok=True)

    # Neither limit given: keep the historical iteration count.  With only
    # --duration, run until the clock says stop rather than capping at it.
    if args.iterations is None and args.duration is None:
        args.iterations = 1000

    # A fixed default seed would make every repeat run re-test exactly what
    # the last one did.  Pick a fresh one and say what it was, so repeating
    # explores new ground and replaying stays one flag away.
    if args.seed is None:
        args.seed = random.SystemRandom().randrange(1 << 30)
        print('base seed %d (pass --seed %d to replay this run)'
              % (args.seed, args.seed), flush=True)

    if args.worker_id is not None:
        sys.exit(0 if worker(args, args.worker_id) == 0 else 1)

    if args.workers == 1:
        worker(args, 0)
        return

    # Re-exec ourselves once per worker so they run in parallel.  The seed
    # and both limits are passed down already resolved, so every worker
    # agrees with what was reported above.
    procs = []
    for n in range(args.workers):
        cmd = [sys.executable, os.path.abspath(__file__),
               '--binary', args.binary, '--seeds', args.seeds,
               '--output', args.output,
               '--seed', str(args.seed), '--timeout', str(args.timeout),
               '--max-findings', str(args.max_findings), '--worker-id', str(n)]
        if args.iterations is not None:
            cmd += ['--iterations', str(args.iterations)]
        if args.duration is not None:
            cmd += ['--duration', str(args.duration)]
        procs.append(subprocess.Popen(cmd))
    for p in procs:
        p.wait()


if __name__ == '__main__':
    main()
