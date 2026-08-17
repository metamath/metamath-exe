#!/bin/bash
# Uses bash because the Emscripten SDK's emsdk_env.sh requires it.
# Download and set up the Emscripten SDK under ./emsdk, so it is easy to
# (re)create the environment for building the WebAssembly version of metamath
# that runs in a web browser.
#
# Usage:
#   ./get-emsdk.sh            # install the pinned version if not present
#   ./get-emsdk.sh --force    # reinstall even if already present
#   EMSDK_VERSION=6.0.3 ./get-emsdk.sh   # override the version
#
# After running, build the browser version with:
#   ./build-wasm.sh
#
# This SDK is large and is intentionally NOT checked in (see .gitignore).

set -eu

# Pinned for reproducible builds, and pinned only here: the workflows key
# their cached copy of the SDK off this file, so bumping the version below is
# the whole change.
EMSDK_VERSION="${EMSDK_VERSION:-6.0.3}"

force=0
[ "${1:-}" = "--force" ] && force=1

cd "$(dirname "$0")"

dest="emsdk"

if [ "$force" -eq 0 ] && [ -f "$dest/emsdk_env.sh" ]; then
  echo "emsdk already present in $dest"
  # shellcheck disable=SC1091
  . "$dest/emsdk_env.sh" >/dev/null 2>&1 || true
  emcc --version 2>/dev/null | head -n 1 || true
  echo "(use --force to reinstall)"
  exit 0
fi

command -v git >/dev/null 2>&1 || { echo >&2 "error: git is required"; exit 1; }

if [ ! -d "$dest" ]; then
  echo "Cloning emsdk ..."
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$dest"
fi

echo "Installing Emscripten ${EMSDK_VERSION} (this downloads a lot) ..."
( cd "$dest" && ./emsdk install "$EMSDK_VERSION" && ./emsdk activate "$EMSDK_VERSION" )

# shellcheck disable=SC1091
. "$dest/emsdk_env.sh" >/dev/null 2>&1 || true
echo "Installed:"
emcc --version 2>/dev/null | head -n 1 || true
echo "Done.  Now run: ./build-wasm.sh"
