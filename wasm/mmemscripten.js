// SPDX-License-Identifier: GPL-2.0-or-later OR MIT

// JavaScript support library for the WebAssembly build of metamath.
//
// Linked in with emcc's --js-library option.  It implements the program's two
// suspension points:
//   mm_read_line()   -- declared in src/mminou.c; waits for the user to type a
//                        command line.
//   mm_materialize() -- called from __wrap_fopen() (wasm/mmwasm_fs.c) before
//                        every file read, so the page can page a file into the
//                        in-memory filesystem (and decompress it) on demand.
// Both use Emscripten's dual-mode async-import form -- an explicit
// Asyncify.handleAsync(async () => {...}) body plus __async: true -- which the
// SDK compiles to Asyncify glue under -sASYNCIFY and to JSPI stack switching
// under -sJSPI, from the same source.  (A plain async body without handleAsync
// silently breaks -sASYNCIFY: it returns a bare Promise the wasm reads as an
// integer.)

addToLibrary({
  // int mm_read_line(char *buf, int maxLen)
  //
  // Returns 1 and stores a line (with its trailing new-line) in buf, or
  // returns 0 to signal end of input.  Module.mmReadLine is supplied by the
  // hosting page and returns a promise for the next line, or null when the
  // user is finished.
  mm_read_line: function (bufPtr, maxLen) {
    return Asyncify.handleAsync(async function () {
      let line;
      try {
        line = await Module.mmReadLine();
      } catch (e) {
        line = null;
      }
      if (line === null || line === undefined) {
        return 0; // end of input
      }
      // cmdInput() expects fgets() semantics, so the new-line must be present.
      let text = line + "\n";
      // stringToUTF8 always writes a terminating null and never exceeds the
      // size given, so an over-long line is truncated rather than corrupting
      // memory.  Metamath separately detects and reports over-long input.
      stringToUTF8(text, bufPtr, maxLen);
      return 1;
    });
  },
  // Tells Asyncify this import may suspend, so the compiler instruments the
  // call path that reaches it.  Without this the program traps as soon as it
  // tries to unwind.
  mm_read_line__async: true,
  mm_read_line__deps: ["$stringToUTF8"],

  // void mm_materialize(const char *path)
  //
  // Called from __wrap_fopen() before every file read.  It guarantees the file's
  // contents are present before the synchronous read that follows, by asking the
  // hosting page (Module.mmMaterialize) to page the file into the in-memory
  // filesystem and decompress it.  The page returns a promise; we suspend until
  // it resolves.  The command-line test build supplies no mmMaterialize, so
  // there it is simply a no-op.
  mm_materialize: function (pathPtr) {
    return Asyncify.handleAsync(async function () {
      if (Module.mmMaterialize) await Module.mmMaterialize(UTF8ToString(pathPtr));
    });
  },
  mm_materialize__async: true,
  mm_materialize__deps: ["$UTF8ToString"],
});
