#!/bin/bash
# Build the WebAssembly version of metamath that runs in a web browser.
# Uses bash because the Emscripten SDK's emsdk_env.sh requires it.
#
# Prerequisite:  ./get-emsdk.sh   (installs the pinned Emscripten SDK)
#
# Usage:
#   ./build-wasm.sh          # build into ./build-wasm
#   ./build-wasm.sh -s       # build, then serve it on http://localhost:8765
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
[ "${1:-}" = "-s" ] && serve=1

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
echo "Compiling ..."
emcc "$top_dir"/src/*.c \
  -o "$out_dir/metamath-browser.js" \
  -O2 -DINLINE=inline \
  -sASYNCIFY \
  -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=64MB \
  -sMODULARIZE=1 \
  -sEXPORT_NAME=createMetamath \
  -sEXPORTED_RUNTIME_METHODS=callMain,FS \
  -sINVOKE_RUN=0 \
  -sEXIT_RUNTIME=1 \
  -sFORCE_FILESYSTEM=1 \
  --js-library "$top_dir/wasm/mmemscripten.js"

cp "$top_dir/wasm/metamath.html" "$out_dir/index.html"

echo "Built in $out_dir"
ls -l "$out_dir/metamath-browser.wasm" | awk '{printf "  metamath-browser.wasm  %.0f KB\n", $5/1024}'

if [ "$serve" -eq 1 ]; then
  echo "Serving http://localhost:8765/  (press Control-C to stop)"
  cd "$out_dir"
  exec python3 -m http.server 8765
else
  echo "To try it:  ./build-wasm.sh -s   then open http://localhost:8765/"
fi
