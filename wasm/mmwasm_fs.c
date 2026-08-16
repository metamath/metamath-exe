// mmwasm_fs.c -- WebAssembly build only.
//
// This file is compiled ONLY by build-wasm.sh (with emcc).  It is deliberately
// not in src/Makefile.am and lives in wasm/, so ordinary native builds -- the
// autotools build, build.sh, cosmocc, and even a bare "compile src/*.c" -- never
// see it.  The whole body is guarded by #ifdef __EMSCRIPTEN__ as well, so if it
// ever were compiled by another toolchain it would produce an empty object file.
//
// It intercepts fopen() via the linker option -Wl,--wrap=fopen.  With that
// option every call to fopen() is renamed to __wrap_fopen(), and the real libc
// function is available as __real_fopen().  Before metamath opens a file for
// reading, we call mm_materialize(), a hook implemented in JavaScript
// (wasm/mmemscripten.js), which makes sure the file's contents are present in
// the in-memory filesystem (paging them in and decompressing them on demand).
// metamath opens every file with fopen() -- there is no freopen/open/fdopen --
// so this one wrapper covers all reads.

#ifdef __EMSCRIPTEN__

#include <stdio.h>

FILE *__real_fopen(const char *path, const char *mode);
extern void mm_materialize(const char *path);

FILE *__wrap_fopen(const char *path, const char *mode) {
  // Only a read needs the contents present first; writes create a new file.
  if (mode && mode[0] == 'r')
    mm_materialize(path);
  return __real_fopen(path, mode);
}

#endif // __EMSCRIPTEN__
