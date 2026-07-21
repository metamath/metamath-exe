// Runs the WebAssembly build as an ordinary command line program under node,
// so that the existing test suite can be run against it.  This is how the
// browser build is tested before it is published; there is no other way to
// exercise wasm32, where a long is 32 bits rather than 64.
//
// Input piped on standard input is handed over one line at a time through
// Module.mmReadLine, the same hook the web page uses.  Output is collected a
// byte at a time through FS.init so that unterminated prompts such as "MM> "
// appear exactly as they do in a native build.
//
// Usage (normally through the wrapper that build-wasm.sh generates):
//   node wasm/run-node.js [FILE.mm] < commands

const fs = require('fs');
const path = require('path');

const modulePath = process.env.MM_WASM_MODULE ||
  path.resolve(__dirname, '..', 'build-wasm', 'metamath-node.js');
const createMetamath = require(modulePath);

let input = '';
try {
  input = fs.readFileSync(0, 'utf8');
} catch (e) {
  input = ''; // no standard input
}
const lines = input.split('\n');
if (lines.length > 0 && lines[lines.length - 1] === '') lines.pop();
let next = 0;

const out = [];

(async function () {
  let reportExit;
  const exited = new Promise(function (resolve) { reportExit = resolve; });

  const Module = await createMetamath({
    mmReadLine: async function () {
      return next < lines.length ? lines[next++] : null;
    },
    preRun: [function (Module) {
      const sink = function (byte) { if (byte !== null) out.push(byte); };
      Module.FS.init(null, sink, sink);
    }],
    onExit: function (code) { reportExit(code || 0); },
  });

  try {
    Module.callMain(process.argv.slice(2));
  } catch (e) {
    if (e && e.name === 'ExitStatus') reportExit(e.status);
    else throw e;
  }

  // Asyncify means main() keeps running after callMain() returns, so wait for
  // the program to finish.  Exiting any earlier truncates the output.
  const code = await Promise.race([
    exited,
    new Promise(function (resolve) { setTimeout(function () { resolve(99); }, 300000); }),
  ]);

  process.stdout.write(Buffer.from(out));
  process.exit(code);
})();
