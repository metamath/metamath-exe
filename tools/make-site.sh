#!/bin/sh
# make-site.sh - assemble the metamath-exe web site from a finished build.
#
# The site is small on purpose.  It offers the two things a visitor cannot get
# by cloning the repository: a program built for them, and a way to run it
# without installing anything.  Nothing else.  Anyone who wants the source is
# one link away from it.
#
# Layout.  One directory per build, each holding everything about that build:
#
#   /index.html    a short page naming what is below
#   /style.css     shared by both pages
#   /main/index.html   what that build is, and how to check it
#   /main/         built from the main line of development
#     metamath.exe             the Cosmopolitan build, under a fixed name
#     metamath-<version>-<sha>.exe   the same file, named for a person
#     run/                     the WebAssembly build, to run in a browser
#     SHA256SUMS, build-info.json
#
# A directory per build leaves room for a /release/ beside it, holding the most
# recent tagged version, without moving anything or making this page describe
# two builds at once.  Until there is one, there is simply no such directory.
#
# When /release/ is built, it should carry the release's own file names as they
# are, rather than names made up here to match the ones below.  One file should
# have one name however someone came by it, whether from the release or from
# the site.
#
# Usage: make-site.sh APE WASM_DIST OUT [COMMIT]
#
#   APE        the Cosmopolitan executable to publish
#   WASM_DIST  the directory build-wasm.sh produced
#   OUT        directory to build the site in; it is REPLACED
#   COMMIT     commit the build came from (default: git rev-parse HEAD)
#
# The workflow and anyone checking the site locally both run this, so what
# gets published is decided in a file that can be read and tested, rather than
# inside a workflow that only ever runs on a server.  To look at the result,
# serve it; a page cannot load WebAssembly over a file:// URL:
#
#   tools/make-site.sh metamath wasm-dist ,site
#   (cd ,site && python3 -m http.server 8000)

set -eu

fail() { printf '%s: %s\n' "$0" "$1" >&2; exit 1; }

case $# in
  3|4) ;;
  *) fail "usage: $0 APE WASM_DIST OUT [COMMIT]" ;;
esac

top_dir=$(cd "$(dirname "$0")/.." && pwd)
ape=$1
wasm_dist=$2
out=$3
commit=${4:-$(git -C "$top_dir" rev-parse HEAD 2>/dev/null)} \
  || fail "not a git checkout; pass COMMIT"

[ -f "$ape" ] || fail "no such file: $ape"
[ -d "$wasm_dist" ] || fail "no such directory: $wasm_dist"
[ -n "$commit" ] || fail "no commit given, and none found"

# OUT is removed and rebuilt, so refuse the values that would take too much
# with them.
case "$out" in
  ""|/|.|..) fail "refusing to use '$out' as the output directory" ;;
esac

built=$(date -u '+%Y-%m-%d %H:%M UTC')
short=$(printf '%s' "$commit" | cut -c1-7)

# The version metamath reports at startup, read from the source it was built
# from, so the page and the program cannot disagree about it.
version=$("$top_dir/build.sh" -v) || fail "could not read the version"

rm -rf "$out"

# Everything about this build lives in one directory.
dir="$out/main"
mkdir -p "$dir/run"

# The browser build, minus what is not the web site: the command line build
# used for testing, its wrapper (which would also publish a path from the
# build machine), and the local development server.  -L because build-wasm.sh
# links the checked-in files in from wasm/, and a symlink out of the site is
# useless once published.
for f in "$wasm_dist"/*; do
  case "$(basename "$f")" in
    serve|metamath-node.*|metamath-wasm) continue ;;
  esac
  cp -L "$f" "$dir/run/"
done
[ -f "$dir/run/index.html" ] || fail "no index.html in $wasm_dist"

# One program under two names: metamath.exe never changes, so a script can
# fetch the current build from a fixed URL, and the version-named copy is what
# the page offers a person, since a browser saves a file under the name in the
# URL.  Windows treats an unsigned program with a bare name more suspiciously,
# and the commit says which build it is months later, which the version cannot:
# that does not change between builds.  The date beside the version in MVERSION
# is left out, since in a file name it would read as the day the file was
# built, which it is not.
versioned="metamath-$(printf '%s' "$version" | cut -d' ' -f1)-${short}.exe"
cp "$ape" "$dir/metamath.exe"
cp "$ape" "$dir/$versioned"
chmod +x "$dir/metamath.exe" "$dir/$versioned"

# Hashed exactly as published.  Running the program does not alter it, so these
# stay true however long someone keeps the file.  Both lines carry the same
# hash, because it is one program under two names.
sha256=$(cd "$dir" && { sha256sum metamath.exe 2>/dev/null \
  || shasum -a 256 metamath.exe; } | cut -d' ' -f1)
[ -n "$sha256" ] || fail "could not hash the program"
{ printf '%s  metamath.exe\n' "$sha256"
  printf '%s  %s\n' "$sha256" "$versioned"
} > "$dir/SHA256SUMS"

bytes=$(wc -c < "$dir/metamath.exe" | tr -d ' ')

# The same facts a script can read, so that automating a download does not
# mean scraping the page.  It sits in the build's own directory and describes
# that build, so a /release/ beside it would carry its own copy of this rather
# than the two having to share one file.
cat > "$dir/build-info.json" <<JSON
{
  "version": "$version",
  "commit": "$commit",
  "built": "$built",
  "metamath.exe": {
    "size": $bytes,
    "sha256": "$sha256",
    "versioned_name": "$versioned"
  }
}
JSON

# The pages, each one filled in and then checked for a name this script does
# not know how to replace.  The top page has none of them, and should keep
# none: it says nothing that changes from one build to the next.
size=$(awk -v b="$bytes" 'BEGIN { printf "%.1f MB", b/1048576 }')
render() {
  sed -e "s|@VERSION@|$version|g" \
      -e "s|@APE_VERSIONED@|$versioned|g" \
      -e "s|@COMMIT@|$commit|g" \
      -e "s|@COMMIT_SHORT@|$short|g" \
      -e "s|@BUILT@|$built|g" \
      -e "s|@SHA256@|$sha256|g" \
      -e "s|@APE_SIZE@|$size|g" \
      "$1" > "$2"
  ! grep -q '@[A-Z_]*@' "$2" \
    || fail "unreplaced placeholder in $2: $(grep -o '@[A-Z_]*@' "$2" | head -1)"
}

cp "$top_dir/site/style.css" "$out/style.css"
render "$top_dir/site/index.html" "$out/index.html"
render "$top_dir/site/main/index.html" "$dir/index.html"

echo "Site assembled in $out"
echo "  $version, commit $short, built $built"
echo "  sha256 $sha256"
