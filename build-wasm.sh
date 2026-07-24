#!/bin/bash
# Build the WebAssembly version of metamath that runs in a web browser.
# Uses bash because the Emscripten SDK's emsdk_env.sh requires it.
#
# Prerequisite:  ./get-emsdk.sh   (installs the pinned Emscripten SDK)
#
# Usage:
#   ./build-wasm.sh          # build into ./build-wasm
#   ./build-wasm.sh -s       # build, then serve it on http://localhost:8000
#   ./build-wasm.sh -t       # build, then run the test suite against it
#
# Output (in ./build-wasm):
#   metamath-browser.js    Emscripten loader        (generated here by emcc)
#   metamath-browser.wasm  the compiled program     (generated here by emcc)
#   index.html             the page                 (staged from wasm/metamath.html)
#   serve                  local test web server    (staged from wasm/serve)
#
# Source of truth lives in wasm/ (checked in).  The two staged files are linked
# in from there with a symlink when the filesystem supports one, so editing
# wasm/metamath.html shows up on the next browser reload with no rebuild; on
# other filesystems they are copied instead.  Either way ./build-wasm holds
# nothing irreplaceable: it is intentionally NOT checked in (see .gitignore),
# so "rm -rf build-wasm" is always a safe clean.

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

# Stage a checked-in source file from wasm/ into the (disposable) build
# directory.  Prefer a relative symlink, so that editing the source is reflected
# on the next browser reload with no rebuild and the source stays the single
# copy; fall back to a plain copy on filesystems without symlink support.
#   $1  file name in wasm/
#   $2  name to create in build-wasm/ (the page must be served as index.html)
stage_from_wasm() {
  rm -f "$out_dir/$2"
  ln -s "../wasm/$1" "$out_dir/$2" 2>/dev/null || cp "$top_dir/wasm/$1" "$out_dir/$2"
}

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
#   -sASYNCIFY_ONLY=[...]
#              In principle this restricts ASYNCIFY's instrumentation to just
#              the functions that can be on the stack at a yield, leaving hot
#              code uninstrumented.  In THIS program it buys little: print2()
#              yields (see mm_browser_yield() in src/mminou.c), and bug() (the
#              assertion used throughout the code) calls print2(), so nearly
#              every function can transitively reach a yield, including the hot
#              proof verifier.  The whitelist would therefore be almost the
#              whole program.  It need not be written by hand (build once with
#              -sASYNCIFY_ADVISE and Emscripten prints the list), but any new
#              code path that reaches print2() then silently TRAPS at run time
#              until the list is regenerated; an omission is not a compile
#              error.  Low payoff, real maintenance risk; deferred.
#   -sJSPI     Replaces ASYNCIFY with the browser's native stack switching:
#              zero instrumentation, no whitelist, hot code runs as plain wasm.
#              This is the better long-term target.  Browser support (2026-07):
#                - Chrome:  on by default since 137 (mid-2025).
#                - Firefox: enabled in 153 (2026-07-21).  New; the installed
#                  base needs time to update before we can rely on it.
#                - Safari:  not yet (objection dropped late 2025, not shipped).
#              IMPORTANT: a -sJSPI build contains NO ASYNCIFY fallback.  On a
#              browser without JSPI it does not run slower; it TRAPS the first
#              time it tries to suspend (i.e. as soon as the user is prompted
#              for input).  A JSPI-only build would simply break on older
#              Firefox, all Safari, and older Chrome, which is why we do not
#              just switch the flag.
#
# Long-term plan (OPTION 2): ship BOTH builds and feature-detect on the page,
# so JSPI-capable browsers get the fast, uninstrumented build while everyone
# else falls back to the ASYNCIFY build.  NOT implemented yet; recorded here so
# the switch is mechanical once JSPI's installed base is wide enough:
#
#   1. Build twice from $common_opts below.  Keep the current ASYNCIFY build
#      (-> metamath-browser.js / .wasm) and add a second build that swaps
#      -sASYNCIFY for -sJSPI, written to distinct names, e.g.
#      metamath-browser-jspi.js / .wasm.  Keep -sEXPORT_NAME=createMetamath and
#      -sEXPORTED_RUNTIME_METHODS identical in both, so the page's code after
#      the module loads is the same no matter which build it picked.
#
#   2. The suspend shim in wasm/mmemscripten.js needs a JSPI-compatible form.
#      mm_read_line() currently calls Asyncify.handleAsync(), which is ASYNCIFY-
#      only API.  Under JSPI an async import is expressed by marking it
#      mm_read_line__async = true (already set) and returning a Promise directly
#      (no Asyncify.handleAsync wrapper).  Confirm the exact form against the
#      pinned Emscripten SDK's docs at implementation time; the two builds may
#      need slightly different --js-library shims, or one shim written to work
#      for both.  print2()'s yield needs no change: emscripten_sleep() is
#      supported under both ASYNCIFY and JSPI.
#
#   3. In wasm/metamath.html, feature-detect and load exactly one script:
#          const jspi = typeof WebAssembly.Suspending === "function";
#          // load metamath-browser-jspi.js if jspi, else metamath-browser.js
#      Only one .wasm is downloaded per visitor.  There is no single binary that
#      auto-falls-back; this page-level choice IS the fallback.
#
#   4. Cost: doubles build time and stores two .wasm files on the server (each
#      visitor still downloads only one).  Once JSPI is universal (Safari ships
#      and the update tail passes), drop the ASYNCIFY build and this note.
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
  -lidbfs.js \
  --js-library "$top_dir/wasm/mmemscripten.js"

stage_from_wasm metamath.html index.html
stage_from_wasm serve serve
chmod +x "$out_dir/serve" 2>/dev/null || true

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
  echo "Serving http://localhost:8000/  (press Control-C to stop)"
  cd "$out_dir"
  exec python3 -m http.server 8000
else
  echo "To try it:  ./build-wasm.sh -s   then open http://localhost:8000/"
  echo "To iterate: run ./build-wasm/serve in another terminal (http://localhost:8000/),"
  echo "            edit wasm/metamath.html, and reload (no rebuild needed)."
fi
