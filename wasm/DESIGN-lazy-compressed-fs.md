# Design: lazy, compressed `/work` filesystem for the WASM build

## Goal

Store the files in `/work` **compressed** and keep them out of RAM until
something actually reads them, then drop the uncompressed copy again.  Reads are
rare; databases are 50+ MiB and we often hold several (a working copy plus its
`.orig` baseline, plus other databases).  Compressing both the in-RAM and the
persisted copy:

- shrinks the resident heap (idle files cost ~their gzip size, ~4–6× smaller),
- shrinks IndexedDB usage, which lowers the chance the browser evicts our
  storage.

Compression is real **gzip** via the browser-native `CompressionStream` /
`DecompressionStream` — no library, no hand-written DEFLATE.  Those APIs are
async, which is fine: we page files in at points that can suspend.

## Principles

1. **Source of truth is a JS "store" of compressed blobs + metadata**, not the
   Emscripten FS.  The FS `/work` tree is a *bridge* that exists so the C
   program can open/read/write files with ordinary stdio.
2. **Two consumers, two page-in mechanisms** (this is the crux):
   - **metamath (C, synchronous `fread`)** cannot await, so it pages a file in
     by *suspending* at a C→JS async import — using Emscripten's mode-agnostic
     `__async: true` primitive so the **same source compiles under both
     `-sASYNCIFY` and `-sJSPI`** (see "JSPI parity" below).
   - **Explorer / editor / diff / patch (JS)** can await, so they decompress
     directly in JS and **never populate the FS node at all**.
3. **Every C file open is a page-in point.**  metamath opens *all* files through
   `fopen` (directly or via `fSafeOpen`; there is no `freopen`/`open`/`fdopen`),
   so a single linker interposition `-Wl,--wrap=fopen` covers 100% of C reads —
   READ, INCLUDE, bibliography, SUBMIT scripts, TOOLS readers, and probes alike.
   Because *any* read materializes first, eviction is safe at **any** size; the
   eviction threshold is a performance knob, not a correctness guarantee.
4. **Evict everything, at idle.**  Every file is dropped from RAM when metamath
   returns to its prompt — no size threshold.  Eviction happens at idle, not at
   `fclose`, so a file materialized during a command stays resident for the rest
   of that command (repeated opens/probes inflate it at most once); only the
   *next* command that touches it re-inflates, which for a small file is
   microseconds.
5. **Compress unless it doesn't help.**  gzip is byte-exact on any content, so we
   never gate on file type: we compress and keep the result only if it is
   smaller, else store the original with a `raw` flag.  Below `COMPRESS_MIN`
   (~1 KiB) we skip gzip entirely (its header isn't worth it) and store `raw`.
   A separate `binary` flag (a `\0` in the first few KiB) is recorded only for
   the editor's benefit.
6. **Keep data compressed wherever reasonable.**  A file is uncompressed in RAM
   only while it is genuinely in use (being read by metamath this command, or
   open in the editor as `docLines`).

## Data model

A single JS map is the authority:

```
store: Map<name, {
  blob:   Uint8Array | null, // stored bytes: gzip if raw===false, else the
                             //   original.  null only for a resident file just
                             //   written by metamath, not yet compressed.
  raw:    boolean,           // true = blob is uncompressed (gzip didn't shrink it)
  size:   number,            // UNCOMPRESSED length in bytes
  mtime:  number,            // ms
  mode:   number,            // FS mode bits
  binary: boolean,           // a \0 in the first few KiB (for the editor only)
  dirty:  boolean,           // needs (re)compress + persist at next idle
}>
```

One threshold: `COMPRESS_MIN` ≈ 1 KiB (below it, store `raw` — gzip's header
isn't worth it).  There is no eviction threshold: all files are evicted at idle.

Each entry has a matching node in the MEMFS `/work` tree so C code sees it via
`readdir`/`stat`/`fopen`.  A node is in one of two states:

- **evicted** (the resting state for every file): `node.contents` is empty;
  `getattr` reports `size` from the store.  Costs ~0 uncompressed RAM.
- **resident**: `node.contents` holds the uncompressed bytes — only from the
  first read/write of a command until the next idle evict.

Because `--wrap=fopen` materializes before every read, correctness never depends
on residency.

## Files to modify

| File | Change |
|---|---|
| `wasm/mmemscripten.js` | Add the new async import `mm_materialize` using the dual-mode `Asyncify.handleAsync(async …) + __async: true` form (same form `mm_read_line` already uses -- do **not** use a plain-`async` body, which breaks ASYNCIFY). |
| `wasm/metamath.html` | Replace the IDBFS mount + `syncfs` persistence with the lazy-compressed backend + own IndexedDB store; add a small `/work` access API (`readWorkFile`/`writeWorkFile`/`statWork`/`listWork`/…); refactor Explorer, editor, diff, patch, download, and "add from computer" to use it. |
| `src/mmwasm_fs.c` (new) | Wasm-only file holding `__wrap_fopen` (guarded by `#ifdef __EMSCRIPTEN__`); added to the emcc compile list. No existing `.c` files change. |
| `build-wasm.sh` | Add `-Wl,--wrap=fopen`; add a second `-sJSPI` build target (Option 2 already sketched there). |

## C side

### The `fopen` wrapper (single hook, covers every read)

Every open in metamath is `fopen`/`fSafeOpen` — there is no `freopen`, `open`,
or `fdopen` (verified) — so one interposition covers all reads.  Build with
`-Wl,--wrap=fopen` and provide:

```c
#include <string.h>
#include <stdio.h>
FILE *__real_fopen(const char *path, const char *mode);
extern void mm_materialize(const char *path);   // JS import; suspends; no-op if
                                                 //   resident, missing, or write
FILE *__wrap_fopen(const char *path, const char *mode) {
  if (mode && (mode[0] == 'r'))    // read/read-update: needs contents present
    mm_materialize(path);
  return __real_fopen(path, mode);
}
```

`mm_materialize` is declared in `wasm/mmemscripten.js` with `__async: true`, so
the call **suspends** metamath while JS reads the stored blob and
`DecompressionStream`s it into the node's `contents` (or copies it when `raw`).
When it returns, the file is resident and the synchronous `fread` runs
unchanged.  It is idempotent and a no-op when the file is already resident or
not in the store (a genuinely missing file then makes `__real_fopen` return NULL
as before).

Consequences that fall out for free:

- **Probes** (`fGetTmpName`, the include-deletion scan, `fSafeOpen`'s backup
  reads, the READ probe) call `mm_materialize` harmlessly — the file is small or
  absent, so it is a cheap no-op — then behave exactly as today.
- **`readFileToString`'s sizing open** (`fseek`/`ftell`) also materializes; the
  second open's `fread` then hits resident bytes.  (`getattr` already serves the
  size from the store, so even without this the sizing open would work — but
  materializing once per open is simplest.)
- **Writes** (`fopen("w"/"a")`) skip materialize; they create a resident
  uncompressed node, which the idle pass compresses + persists + evicts.
- **`fGetTmpName`'s probe loop** can call `fopen` up to ~1000 times; each is a
  no-op materialize on a non-existent name.  Acceptable (temp-name generation is
  rare), but noted.

## JS side (`wasm/metamath.html`)

### The lazy-compressed backend

Base it on MEMFS and customize the minimum:

- **`getattr`**: for an evicted file, report `size`/`mtime`/`mode` from the
  store instead of the empty `node.contents`.
- **`materialize(name)`** (async, JS): if resident, return; else set
  `node.contents` to `store[name].raw ? blob : gunzip(blob)` → mark resident.
  This is what the `mm_materialize` import calls.
- **`compress(bytes)`** (async, JS): below `COMPRESS_MIN`, return
  `{blob:bytes, raw:true}`; else `g = gzip(bytes)` and return the smaller of
  `{blob:g, raw:false}` and `{blob:bytes, raw:true}`.  Also set `binary` from a
  `\0` scan of the first few KiB.
- **`evict(name)`** (async, JS): if `dirty` or `blob === null`,
  `compress(node.contents)` → store; then clear `node.contents` (evicted).
  Applies to every resident file at idle.
- No need to override `stream_ops.read`/`write`: metamath only reads a resident
  node (materialize ran first) and writes create resident nodes.

### The `/work` access API (used by all JS, never `FS.*` for `/work`)

```
async readWorkFile(name)  -> Uint8Array   // resident node if present & newer, else gunzip(store)
async writeWorkFile(name, bytes)          // gzip into store, mark dirty, update metadata; no resident copy
      statWork(name) / listWork()         // from the store (cheap, no inflate)
async renameWork/copyWork/deleteWork/newWork
```

`readWorkFile` prefers a live resident node when one exists (covers the window
after a metamath write but before the idle evict), otherwise decompresses from
the store — **without** creating an FS node.  So editor/diff/patch reads never
inflate a file into the FS.

### Explorer / editor / diff / patch refactor

- **List** (`renderExplore`, currently `FS.readdir`/`FS.stat` at :704/:710):
  use `listWork()`/`statWork()` — no inflation, so listing 50 MiB databases is
  free.
- **Editor open** (`openEditor` :1043): make `async`; `bytes = await
  readWorkFile(name)`; decode to `docLines`; **that is the only uncompressed
  copy** — the store stays compressed and no FS node is created, exactly the
  "flush the uncompressed RAM once we have the array" behavior requested.
- **Editor segments**: unchanged — pure `docLines` manipulation, no storage
  access, no page-in/evict between segments.
- **Editor save** (`editorSave` :1057): `await writeWorkFile(name,
  docLines.join)`; mark dirty; trigger the idle/flush persist.
- **Editor close**: drop `docLines` (already done at :1074).  Nothing resident.
- **Diff/Patch** (:1411/:1513): `await readWorkFile(...)` for inputs,
  `await writeWorkFile(...)` for outputs.
- **Download** (`fetchDb` :508) and **Add from computer** (:666): `await
  writeWorkFile(name, buf)` and `writeWorkFile(name + ".orig", buf)` instead of
  `FS.writeFile`; both are large → compressed immediately, never resident.

### Persistence (replaces IDBFS)

Replace the IDBFS mount (:1596) and `syncfs` calls (:1577/:1611) with our own
IndexedDB object store keyed by name holding `{comp, size, mtime, mode, small}`.

- **Boot**: open DB, read all records into `store`, create an **evicted** MEMFS
  node per file (metadata only).  Zero uncompressed RAM at startup; nothing is
  decompressed until read.
- **Idle flush** (`flushPersist` :1571, still driven by the existing snapshot
  diff): for each file whose snapshot changed or `dirty`: if it is a resident
  node written by metamath, `compress` it into the store; `put` the record into
  IndexedDB; then `evict` every resident node.  Only changed files are
  re-compressed/persisted (same snapshot-diff policy as today).
- **Unload** (:1585): best-effort final flush.

## Lifecycle / order of events

**Boot**
1. Open IndexedDB; load metadata + gzip blobs into `store`.
2. Mount `/work` with the lazy backend; create one evicted node per file.
3. `navigator.storage.persist()`.  Ready.  (No inflate yet.)

**metamath READ "set.mm"**
1. Command delivered via `mm_read_line` (suspends/resumes; unchanged).
2. `readFileToString`'s first `fopen` → `__wrap_fopen` → `mm_materialize("set.mm")`
   **suspends**; JS decompresses the blob into the node (resident); resumes.
3. Sizing open + `fread` run synchronously against resident bytes.
4. INCLUDEs — each `readFileToString`/`fopen` repeats step 2 for its file.
5. At next idle, `flushPersist` re-evicts the resident files (drop uncompressed
   RAM).  metamath keeps its own parsed copy regardless — unavoidable and
   separate.

**metamath WRITE SOURCE "out.mm"**
1. Write creates a resident uncompressed node.
2. Idle flush: gzip → store → IndexedDB → evict.

**Editor open/edit/save**
1. `openEditor`: `await readWorkFile` → gunzip once → `docLines`.  No FS node,
   no resident copy; store stays compressed.
2. Segment navigation: all in `docLines`; no storage access.
3. `editorSave`: `await writeWorkFile(docLines.join)` → gzip into store, dirty.
4. Idle flush persists it.  `editorClose` drops `docLines`.

## JSPI parity

The only mechanism that must be JSPI-portable is the C→JS suspend.  **Verified
against Emscripten 6.0.3 (`src/lib/libasync.js`): the portable form is the one
`mm_read_line` already uses** -- an explicit `Asyncify.handleAsync(async () =>
{...})` body plus the `__async: true` attribute:

- Under `-sASYNCIFY` (mode 1), the import wrappers only do state-checking; they
  do **not** consume a returned Promise.  So the body *must* call
  `Asyncify.handleAsync`, which drives the unwind/await/rewind and returns a real
  i32.  A plain `async` function (no `handleAsync`) returns a bare Promise that
  the wasm coerces to garbage -- it silently breaks (confirmed empirically: the
  program stops reading commands and its output truncates).
- Under `-sJSPI` (mode 2), an `__async: true` import is auto-wrapped in
  `WebAssembly.Suspending`, which *does* consume a returned Promise; and
  `Asyncify.handleAsync` is *also defined* in this mode (it just adds keepalive
  push/pop around `await startAsync()` and returns a Promise).  So the very same
  `handleAsync` body works here too.

So `Asyncify.handleAsync(async …) + __async: true` is the single dual-mode form;
the plain-`async`-only form is **not** portable (it is JSPI-only).  Both suspend
points must use the `handleAsync` form:

- `mm_read_line` already does -- **no change needed** (this was the original
  step 1; it collapses to "confirm and document").  The stale comment in
  `build-wasm.sh` claiming the shim "needs a JSPI-compatible form" is wrong for
  6.0.3 and should be corrected.
- Write `mm_materialize` the same way: `mm_materialize: (…) =>
  Asyncify.handleAsync(async () => { … })`, with `mm_materialize__async: true`.

Then the ASYNCIFY vs JSPI choice is purely the compile flag: build twice
(current ASYNCIFY output + a second `-sJSPI` output under distinct names) and
feature-detect on the page.  All JS above (store, backend, Explorer API,
DecompressionStream) is identical for both builds.  (Runtime JSPI behavior
still needs confirming by actually building `-sJSPI` and running it -- that is
step 2.)

## Eviction and the one threshold

- **No eviction threshold**: every file is evicted at the idle flush.  Because
  eviction is at idle (metamath parked at its prompt) and not at `fclose`, a file
  materialized during a command stays resident for the rest of that command, so
  repeated opens/probes inflate it at most once; only the next command that
  touches it re-inflates.  We also never drop a node's contents while a C
  `FILE *` might be open on it.
- **`COMPRESS_MIN`** (≈ 1 KiB) is the only knob: at or above it, try gzip and
  keep it if it shrinks (`raw` records the outcome); below it, store `raw`.  It
  applies to the in-RAM blob *and* the IndexedDB record, so small files still
  compress — it only avoids gzip's header overhead on tiny data.
- Correctness is independent of it: `--wrap=fopen` materializes before every
  read.  `COMPRESS_MIN` is a pure size knob.

## Risks / edge cases

- **Broadened suspend surface**: `--wrap=fopen` makes every open a potential
  suspend point.  The build already suspends in `print2` (reached from `bug()`
  almost everywhere) under whole-program ASYNCIFY, so this adds little; under
  JSPI no instrumentation is involved.  Still, it means the UB-sensitive code
  now unwinds/rewinds at more points — worth a verification pass (VERIFY PROOF
  `*` on set.mm) after wiring it up.
- **Path normalization**: map the C `fileName` (cwd is `/work`; `g_rootDirectory`
  is empty in the browser) to a store key = the `/work`-relative name.  Verify
  INCLUDE paths (`fullIncludeFn`) normalize correctly.
- **Write-then-read window**: `readWorkFile` prefers a live resident node over
  the (possibly stale) store blob, so a file metamath just wrote reads back
  correctly before it is flushed.
- **Compression Streams availability**: assume present (Chrome 80+, FF 113+,
  Safari 16.4+ — below the JSPI floor).  If absent, fall back to storing
  uncompressed (never evict); everything else works.
- **`beforeunload` async**: gzip may not finish during unload; the idle flush
  remains the real guarantee, as it is today.

## Suggested implementation order (each step shippable)

Status: steps 1-4 DONE (2026-07-25).  ASYNCIFY node suite 31/31; JSPI build
runtime-verified in Chrome 150.  Step 3: IDBFS replaced by our own IndexedDB
store.  Step 4: gzip-at-rest via CompressionStream with `raw`/`COMPRESS_MIN`
(1 KiB); DB version bumped to 2 (clears old records, no migration).  Verified
end-to-end in Chrome under a strict CSP: a compressible >1 KiB file is stored
gzip-compressed (3200 -> 79 B) and restored byte-identical; a sub-threshold file
is stored raw.  Also: the page's CSS/JS were moved into their own files
(metamath.css / loader.js / metamath.js) so it serves under a strict CSP.
Next: step 5 (`__wrap_fopen` + `mm_materialize`, still eager) then step 6 (lazy
nodes + evict-at-idle).

1. **Confirm `mm_read_line` is already dual-mode** (it uses
   `Asyncify.handleAsync` + `__async: true`, the verified portable form) and fix
   the stale `build-wasm.sh` comment.  No code change to the shim -- an attempted
   "port" to a plain-`async` body was tried and reverted because it silently
   breaks the ASYNCIFY build (bare Promise coerced to garbage; output truncates).
2. **Add the second `-sJSPI` build** + page feature-detect.  Proves parity at
   runtime before building on it.
3. **Replace IDBFS with the own-IndexedDB store, still storing uncompressed and
   resident.**  Pure persistence refactor; behavior identical.
4. **Add gzip-at-rest with the `raw`/`COMPRESS_MIN` policy** (compress the store
   + IndexedDB records, still materialize eagerly at boot).  Shrinks IndexedDB.
5. **Add `__wrap_fopen` + the `mm_materialize` import** (`-Wl,--wrap=fopen`), with
   materialize still eager/no-op.  Proves the suspend hook end-to-end (run a
   VERIFY PROOF `*` pass) before anything can be evicted.
6. **Add lazy nodes + `getattr` override + evict-everything-at-idle**, and switch
   boot to lazy nodes.  Full on-demand paging.
7. **Refactor Explorer/editor/diff/patch onto the `/work` API** so JS reads
   never inflate into the FS.

Steps 1–3 are low-risk and independently valuable; 4–7 deliver the RAM win.
