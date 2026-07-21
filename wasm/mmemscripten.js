// JavaScript support library for the WebAssembly build of metamath.
//
// Linked in with emcc's --js-library option.  It implements mm_read_line(),
// declared in src/mminou.c, which is the one place the program suspends while
// it waits for the user to type a command.  Everything else (parsing, proof
// verification) runs straight through, so Asyncify only has to instrument the
// short path from main() down to cmdInput().

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
});
