# Fuzzing metamath

A small mutation fuzzer, a test-case reducer, and the crashing inputs
found so far.  Everything here is a developer tool; nothing in this
directory is built or installed as part of metamath.

The fuzzer is deliberately simple: it mutates `.mm` files, runs the
real `metamath` binary on each one, and looks for sanitizer reports.
There is no instrumentation, no coverage feedback, and no in-process
harness.  That was still enough to turn up most of the bugs listed in
[Known cases](#known-cases), but see [Limitations](#limitations) before
assuming it is enough for more.

## Quick start

```sh
cd fuzz
./build-sanitizer.sh          # builds ./metamath-san from ../src
./run-cases.sh                # re-check the known bugs
./fuzz.py --iterations 2000 --workers 6
```

Findings land in `findings/<crash_worker_iteration>/`, each holding the
mutated `crash.mm`, the `crash.cmd` command script that triggered it,
and the sanitizer `output.txt`.

## How it works

**The sanitizers are the oracle.** This is the key point. A malformed
`.mm` file normally just makes metamath print an error and exit 0,
which is correct behavior -- there is nothing for a fuzzer to detect.
Building with AddressSanitizer and UndefinedBehaviorSanitizer turns
out-of-bounds accesses into loud, greppable reports, and that is what
`fuzz.py` watches for. Run it against a normal build and it will find
nothing, no matter how long you leave it.

Each iteration:

1. Picks a seed `.mm` at random (by default every `.mm` in `../tests`).
2. Applies 1-6 random edits: replace a byte with one drawn from a
   Metamath-flavored alphabet, delete a 1-20 byte chunk, or insert a
   token such as `$(`, `$p`, `$=` or a NUL. Biasing toward things that
   mean something to metamath is what makes a fuzzer this crude
   productive -- uniformly random bytes are rejected by the tokenizer
   almost immediately.
3. Picks one command from a list (`verify proof *`, `show statement *
   /alt_html`, `write source ... /extract *`, and about twenty others).
4. Runs `metamath` with `read t.mm` plus that command, 25s timeout.
5. Greps the output for `runtime error`, `AddressSanitizer`, `SEGV`.

The command list matters more than the mutations. Most bugs found so
far were reachable from exactly one command -- `/extract` or
`/alt_html` or `/lemmon/renumber` -- and would never have shown up
under `verify proof *` alone. **If you want to find more, add commands
to `COMMANDS` in `fuzz.py` before tuning anything else.**

## Working a finding

Raw findings are a few hundred lines of mutated database. Reduce first:

```sh
./minimize.py --input findings/crash_0_54/crash.mm \
              --command 'write source out.mm /extract *' \
              --marker writeExtractedSource
```

`--marker` should be the crashing function name from the sanitizer
report. Insisting that exact string keeps reappearing stops the
reduction from wandering into some other crash. This routinely takes
300 lines to under 10.

### Unmasking

A single shallow bug will swamp everything behind it. The first
campaign here produced 167 findings, of which 108 were one bug and 51
were another; nothing else was visible.

The way through is to fix, or temporarily patch out, what you have
already found, then fuzz again:

```sh
cp -r ../src /tmp/src-patched
# patch the known bugs in /tmp/src-patched
gcc -g -O1 -fsanitize=address,undefined -o metamath-san2 /tmp/src-patched/*.c
./fuzz.py --binary ./metamath-san2 --output findings2
```

Each round exposes the next layer. Across four rounds the finding
counts went 167, 8, 13, 18 -- and each round found *different* bugs,
not more of the same.

## Known cases

`cases/` holds a minimal reproducer for every bug found so far, as a
`.mm` file (where one is needed) plus a `.cmd` script of the commands
to run after startup. `./run-cases.sh` runs them all.

Cases named `open-*` are known-unfixed and are **expected** to trip the
sanitizers; everything else is fixed and expected to be clean. The
script fails if a fixed case regresses, and tells you to rename a case
if an `open-` one starts passing.

| Case | Bug | Status |
| --- | --- | --- |
| `symbol-len-exists` | `symbolLenExists[]` indexed by unclamped token length, `parseStatements()` | fixed |
| `error-message-empty-line` | `line[strlen(line)-1]` on an empty line, `errorMessage()` | fixed |
| `read-empty-file` | `fileBuf[-1]` on a zero-byte file, `readFileToString()` | fixed |
| `cmdinput-nul-byte` | `g[-1]` read and write on a NUL-led command line, `cmdInput()` | fixed |
| `tools-nul-byte` | `f[SIZE_MAX]` via `strlen(f)-1`, `linput()` | fixed |
| `extract-dollar-t` | `dollarTCmt[-2]` when `rinstr()` returns 0, `writeExtractedSource()` | fixed |
| `empty-math-string` | `g_MathToken[-1]` from `mathString[0]` on a symbol-less `$a` | fixed |
| `edit-tab-clean` | `sout[-1]` in the tab path, `edit()`, via `TOOLS` `CLEAN f "T"` | fixed |
| `edit-untab-overflow` | heap overflow write in `edit()`, tab flag with no enlarged buffer | fixed |
| `edit-untab-many-tabs` | heap overflow write in `edit()`, 7x buffer too small for many tabs | fixed |
| `open-statement-array-overflow` | `g_Statement[]` heap overflow from the `$$` miscount, `parseKeywords()` | open |
| `open-prove-floating` | `g_MathToken[-1]` from a stale `.tmp`, `proveFloating()`/`makeSubstUnif()` | open |
| `open-wrkproof-null` | null writes when the first proof needs zero space, `parseProof()` | open |
| `open-uninit-token-statement` | `extractNeeded[]` indexed by an uninitialized `.statement`, `writeExtractedSource()` | open |

The open ones are analyzed in `,fixes.txt` at the top of the repo,
except `open-uninit-token-statement`, which was found later and is
described below.

### open-uninit-token-statement

Found by this fuzzer immediately after it was checked in, so it has not
been through the same analysis as the others.

When `parseStatements()` meets an undeclared math symbol it invents a
placeholder token (mmpars.c, near line 1183) and sets `tokenName`,
`length`, `tokenType` and `tmp` -- but leaves `active`, `scope`,
`statement` and `endStatement` holding whatever `realloc()` left there.
`writeExtractedSource()` later does

```c
extractNeeded[g_MathToken[...].statement] = 'Y';   // mmcmds.c:3910
```

so an uninitialized `.statement` becomes a wild array index, written to
and then read back. Two lines are enough, because with no `$c` or `$v`
every symbol becomes a placeholder:

```
bad $p |- x x $=
  xf x? ax-1 $.
```

This is a cousin of the `open-prove-floating` bug: both come from the
spare `g_MathToken[]` slots used for error recovery not being set up as
carefully as real tokens.

## Limitations

Worth being honest about, especially if you are weighing afl++:

- **Not coverage-guided.** It cannot find inputs that need several
  coordinated mutations, so it plateaus. The wins so far came from
  breadth of *commands*, not depth of mutation.
- **Process per iteration.** Roughly 40 executions/second/worker, most
  of it process startup and re-reading the database. A persistent-mode
  harness would be orders of magnitude faster.
- **One shape of input.** Always `read` one file, then one command.
  It never exercises multi-file `$[ ... $]` includes, `submit` scripts,
  or long interactive sessions.
- **Crash-only oracle.** It cannot see wrong *output*, only memory
  errors. Differential testing against a known-good build would catch
  more.

## Toward afl++ or libFuzzer

The obvious next step is an in-process harness: a
`LLVMFuzzerTestOneInput` that takes a byte buffer, parses it as a
database, and returns -- letting afl++ fork-server or persistent mode
run thousands of iterations a second with coverage feedback.

The hard part is **global state**. metamath keeps its database in
globals (`g_Statement`, `g_MathToken`, `g_mathKey`, `g_WrkProof`, the
suballocator pools in mmdata.c) and the fuzzer would have to return
them to a clean state between iterations or the runs contaminate each
other. `eraseSource()` in mmcmds.c is the closest thing to a reset and
is the place to start, but it has not been audited for completeness --
in particular `g_wrkProofMaxSize`, `g_dummyVars`, and the free/used
pools would all need checking. Note that `open-wrkproof-null` is
already a bug about exactly one of those globals starting at zero, so
this is not hypothetical.

Until that exists, the cases in `cases/` are useful as an afl++ input
corpus: they are small, they parse far enough to reach interesting
code, and each one is known to touch a distinct code path.
