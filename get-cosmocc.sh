#!/bin/sh
# Download and set up the Cosmopolitan C toolchain (cosmocc) under ./cosmocc,
# so it is easy to (re)create the environment for building the portable
# "Actually Portable Executable" (APE) metamath binary.
#
# Usage:
#   ./get-cosmocc.sh            # install the pinned version if not present
#   ./get-cosmocc.sh --force    # re-download even if already present
#   COSMOCC_VERSION=4.0.2 ./get-cosmocc.sh   # override the version
#
# After running, build the portable binary with:
#   PATH="$PWD/cosmocc/bin:$PATH" CC=cosmocc ./build.sh
#
# CC=cosmocc builds in build-cosmo/ rather than the native build/, so the two
# toolchains never share objects.  The executable still lands in the top folder
# as ./metamath, so whichever build ran last is the one sitting there; run
# "file metamath" if you need to know which.
#
# A single build already produces a "fat" APE: cosmocc compiles the program
# twice (x86-64 and aarch64) and bundles both native code images into the one
# file, and the loader picks the slice matching the host CPU.  Arm64 machines
# therefore run native Arm64 code at full speed, there is no emulation and no
# separate ARM build step.  (The cost is that the file is about twice the size
# of a single-architecture build.)
#
# This toolchain is large and is intentionally NOT checked in (see .gitignore).

set -eu

# Pinned for reproducible builds, and pinned only here: the workflows key
# their cached copy of the toolchain off this file, so bumping the version
# below is the whole change.  4.0.2 ships GCC 14.1.0.
COSMOCC_VERSION="${COSMOCC_VERSION:-4.0.2}"

force=0
[ "${1:-}" = "--force" ] && force=1

# Work relative to this script's directory (the repo top folder).
cd "$(dirname "$0")"

dest="cosmocc"
url="https://cosmo.zip/pub/cosmocc/cosmocc-${COSMOCC_VERSION}.zip"

if [ "$force" -eq 0 ] && [ -x "$dest/bin/cosmocc" ]; then
  echo "cosmocc already present at $dest/bin/cosmocc:"
  "$dest/bin/cosmocc" --version | head -n 1
  echo "(use --force to re-download)"
  exit 0
fi

command -v curl >/dev/null 2>&1 || { echo >&2 "error: curl is required"; exit 1; }
command -v unzip >/dev/null 2>&1 || { echo >&2 "error: unzip is required"; exit 1; }

echo "Downloading cosmocc ${COSMOCC_VERSION} ..."
rm -rf "$dest"
mkdir -p "$dest"
curl -fSL -o "$dest/cosmocc.zip" "$url"

echo "Unpacking ..."
unzip -q -o "$dest/cosmocc.zip" -d "$dest"
rm -f "$dest/cosmocc.zip"

echo "Installed:"
"$dest/bin/cosmocc" --version | head -n 1
echo "Done.  Build with: PATH=\"\$PWD/$dest/bin:\$PATH\" CC=cosmocc ./build.sh"
echo "That builds in build-cosmo/, leaving the native build/ untouched."
