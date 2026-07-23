#!/bin/bash
# Build the WebAssembly version of metamath that runs in a web browser.
# Uses bash because the Emscripten SDK's emsdk_env.sh requires it.
#
# Prerequisite:  ./get-emsdk.sh   (installs the pinned Emscripten SDK)
#
# Usage:
#   ./build-wasm.sh          # build into ./build-wasm
#   ./build-wasm.sh -s       # build, then serve it on http://localhost:8765
#   ./build-wasm.sh -t       # build, then run the test suite against it
#
# Output (in ./build-wasm):
#   index.html             the page, copied from wasm/metamath.html
#   metamath-browser.js    Emscripten loader
#   metamath-browser.wasm  the compiled program
#
# The build directory is intentionally NOT checked in (see .gitignore).

set -eu

cd "$(dirname "$0")"
top_dir="$(pwd)"
out_dir="$top_dir/build-wasm"

serve=0
runtests=0
[ "${1:-}" = "-s" ] && serve=1
[ "${1:-}" = "-t" ] && runtests=1

if [ ! -f emsdk/emsdk_env.sh ]; then
  echo >&2 "Emscripten SDK not found.  Run ./get-emsdk.sh first."
  exit 1
fi

# shellcheck disable=SC1091
. ./emsdk/emsdk_env.sh >/dev/null 2>&1

if ! command -v emcc >/dev/null 2>&1; then
  echo >&2 "error: emcc still not on PATH after sourcing emsdk/emsdk_env.sh."
  echo >&2 "Try re-running ./get-emsdk.sh"
  exit 1
fi

mkdir -p "$out_dir"

# Notes on the options below:
#   ASYNCIFY              lets cmdInput() wait for the user without blocking the
#                         browser, and lets print2() yield so progress appears
#                         during long commands such as VERIFY PROOF *.
#   --js-library          supplies mm_read_line(), declared in src/mminou.c.
#   MODULARIZE/EXPORT_NAME  the page creates the instance itself.
#   INVOKE_RUN=0          the page starts main() after it has set things up.
#   EXIT_RUNTIME=1        so EXIT reports cleanly to the page.
#   FORCE_FILESYSTEM      the in-memory filesystem holds uploaded .mm files.
#   ALLOW_MEMORY_GROWTH   set.mm is about 50 MB, and grows over time.
#
# Performance options we have deliberately NOT taken (speed matters here, so
# these are documented in case a future maintainer wants to revisit them):
#   -flto      Link-time optimization would let the optimizer see across all
#              translation units.  We avoid it: the code has some undefined
#              behavior, and LTO's whole-program view lets the compiler exploit
#              UB more aggressively (and drop code it "proves" unreachable),
#              which risks miscompiles.  Revisit once the UB is cleaned up.
#   -sASYNCIFY_ONLY / -sJSPI
#              Plain -sASYNCIFY (used below) instruments EVERY function that
#              could be on the stack at a yield, which slows all hot code
#              (e.g. the proof verifier), not just the I/O that actually yields.
#              -sASYNCIFY_ONLY=[...] restricts instrumentation to the functions
#              on the yield path (cmdInput()/print2() and their callers), which
#              reclaims most of the overhead -- but it requires hand-maintaining
#              that whitelist, and omitting a function is a runtime error.
#              -sJSPI replaces ASYNCIFY with the browser's native stack
#              switching (no instrumentation overhead) but is not yet supported
#              across all browsers.  Both are deferred as additional work.
# Options shared by both builds below.
common_opts="-O3 -DINLINE=inline
  -sASYNCIFY
  -sALLOW_MEMORY_GROWTH=1
  -sMODULARIZE=1
  -sEXPORT_NAME=createMetamath
  -sEXPORTED_RUNTIME_METHODS=callMain,FS
  -sINVOKE_RUN=0
  -sEXIT_RUNTIME=1"

echo "Compiling the browser build ..."
# shellcheck disable=SC2086
emcc "$top_dir"/src/*.c \
  -o "$out_dir/metamath-browser.js" \
  $common_opts \
  -sINITIAL_MEMORY=64MB \
  -sFORCE_FILESYSTEM=1 \
  --js-library "$top_dir/wasm/mmemscripten.js"

cp "$top_dir/wasm/metamath.html" "$out_dir/index.html"

echo "Built in $out_dir"
ls -l "$out_dir/metamath-browser.wasm" | awk '{printf "  metamath-browser.wasm  %.0f KB\n", $5/1024}'

# The test suite needs a build that can reach real files and be driven from a
# terminal, which the browser build deliberately cannot do.
if [ "$runtests" -eq 1 ]; then
  echo "Compiling the command line build used for testing ..."
  # shellcheck disable=SC2086
  emcc "$top_dir"/src/*.c \
    -o "$out_dir/metamath-node.js" \
    $common_opts \
    -sNODERAWFS=1 \
    --js-library "$top_dir/wasm/mmemscripten.js"

  # run_test.sh runs the command as a single word, so wrap it in a script.
  cat > "$out_dir/metamath-wasm" <<WRAPPER
#!/bin/sh
exec node "$top_dir/wasm/run-node.js" "\$@"
WRAPPER
  chmod +x "$out_dir/metamath-wasm"

  echo "Running the test suite against the WebAssembly build ..."
  # Tests that hand a command to the operating system are skipped, because a
  # web browser has no shell to hand it to.
  ( cd "$top_dir/tests" &&
    env METAMATH="$out_dir/metamath-wasm" ./run_test.sh --no-shell ./*.in )
  echo "Tests passed."
fi

if [ "$serve" -eq 1 ]; then
  echo "Serving http://localhost:8765/  (press Control-C to stop)"
  cd "$out_dir"
  exec python3 -m http.server 8765
else
  echo "To try it:  ./build-wasm.sh -s   then open http://localhost:8765/"
fi
