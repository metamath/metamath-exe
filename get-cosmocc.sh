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
# This toolchain is large and is intentionally NOT checked in (see .gitignore).

set -eu

# Pinned for reproducible builds.  Bump deliberately (keep in sync with the
# COSMOCC_VERSION in .github/workflows/release.yml).  4.0.2 ships GCC 14.1.0.
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
