# WebAssembly (browser) build

This directory holds the checked-in source for running metamath-exe inside a web
browser, compiled to WebAssembly with Emscripten. The program itself is the
ordinary metamath-exe C source in [`../src`](../src); the files here are the
extra browser-specific pieces plus the helper scripts that build and serve them.

## The three directories

Three directories are involved, and only this one is checked in:

- **`wasm/`** (this directory, checked in): the browser build's own source.
- **`build-wasm/`** (generated, gitignored): the build output, containing
  everything the browser actually loads. It is created by
  [`../build-wasm.sh`](../build-wasm.sh) and holds nothing irreplaceable, so it
  is safe to delete at any time (`rm -rf build-wasm`); the next build recreates
  it.
- **`emsdk/`** (installed, gitignored): the pinned Emscripten SDK, placed at the
  repository top level by [`../get-emsdk.sh`](../get-emsdk.sh).

Keeping generated files out of this source directory is deliberate: a clean is
just `rm -rf build-wasm`, no generated `.wasm` can be committed by accident, and
`git status` stays quiet. (An earlier note about `-sASYNCIFY_ONLY`, `-sJSPI`,
and the future two-build plan lives in the comments of `build-wasm.sh`.)

## Files in this directory

- **`metamath.html`**: the page itself, that is, the whole browser UI. The build
  serves it as `index.html`.
- **`mmemscripten.js`**: the Emscripten `--js-library` shim. It implements
  `mm_read_line()` (declared in `../src/mminou.c`), the one place the program
  suspends to wait for the user to type a line.
- **`run-node.js`**: a small Node harness that drives the same WebAssembly build
  from the command line, so the existing test suite can exercise it headless.
- **`serve`**: a tiny Python web server (port 8000) for the edit/reload loop
  described below.
- **`README.md`**: this file.

## Building

From the repository top level:

    ./get-emsdk.sh       # install the pinned Emscripten SDK into ./emsdk (once)
    ./build-wasm.sh      # build into ./build-wasm
    ./build-wasm.sh -s   # build, then serve on http://localhost:8000/
    ./build-wasm.sh -t   # build, then run the test suite against the build

`build-wasm.sh` compiles `../src/*.c`, writes `metamath-browser.js` and
`metamath-browser.wasm` into `build-wasm/`, and stages `index.html` and `serve`
into `build-wasm/` from here: as symlinks where the filesystem allows (so edits
to the source appear without another build), or as copies otherwise.

## Updating the pinned Emscripten version

The Emscripten version is pinned so that every developer, and CI, builds with
the same toolchain. It is written in two places, which must be kept in sync:

- `../get-emsdk.sh`: the `EMSDK_VERSION` default (currently `6.0.3`).
- `../.github/workflows/release.yml`: the `EMSDK_VERSION` environment variable.

To move to a new version:

1. Choose the version. The installable versions are listed by `./emsdk/emsdk
   list` (run `git -C emsdk pull` first to refresh that list), and the release
   notes are in the Emscripten
   [ChangeLog](https://github.com/emscripten-core/emscripten/blob/main/ChangeLog.md).
2. Set that same version string in both files listed above.
3. Reinstall the SDK: `./get-emsdk.sh --force`.
4. Rebuild and run the test suite: `./build-wasm.sh -t`.
5. Once the tests pass, commit the two edited files.

To try a candidate version without editing anything, override it for a single
install: `EMSDK_VERSION=6.0.4 ./get-emsdk.sh`. The `EMSDK_VERSION` environment
variable wins over the default in `get-emsdk.sh`, so nothing is pinned to the
new version until you edit the two files; this is a safe way to test a version
before committing to it.

## The edit/reload loop

For UI work (HTML, CSS, JavaScript) you do not need to rebuild:

1. Start the server once, in its own terminal:

       ./build-wasm/serve            # http://localhost:8000/

2. Edit `metamath.html` here, then reload the page.

Because `build-wasm/index.html` links back to this file, the reload shows your
edit immediately. Rebuild with `./build-wasm.sh` only when you change the C
source in `../src`; after a rebuild, a hard reload (Ctrl-Shift-R) avoids a stale
cached `.wasm`.
