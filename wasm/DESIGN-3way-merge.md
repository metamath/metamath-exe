# Design (possible extension): 3-way merge for database updates

Status: **not implemented.**  This records what we could build, with enough
code detail to pick it up later, and why this shape is the right one.

## Goal

Make it painless to carry local edits forward when an upstream `.mm` database is
re-downloaded.  Concretely, given three files in `/work`:

- `FILENAME.orig` — the previous upstream version = the **base** (common ancestor)
- `FILENAME`      — the working copy with the user's local edits = **ours**
- `FILENAME.new`  — the freshly downloaded upstream version = **theirs**

produce an updated `FILENAME` that combines both sets of changes, marking any
spot where the local edits and the upstream edits touch the same region so a
human can resolve it.

This is the textbook 3-way merge case: all three *full* files are on disk, so we
have a real common ancestor and never have to guess.

## Why 3-way, not the current patch

`applyPatch` in `metamath.js` (~line 1148) is deliberately strict and all-or-
nothing:

- It applies each hunk at *exactly* the line number in the `@@` header — no
  offset search, no fuzz.
- The first context or `-` line that doesn't match the target throws
  ("hunk #N does not match the target at line …") and nothing is written.

That guarantees a patch is never applied wrong, but it is not graceful: any real
movement in the target makes it refuse outright, and it never emits conflict
markers.  For the update scenario we would want, at minimum, fuzz + offset
matching (what GNU `patch` does) and `patch --merge`-style conflict markers.

But a plain unified diff only carries *two* of the three inputs (the diff's old
side and new side, plus the target on disk) — its "base" is just fragments of
context around each hunk.  Since the update scenario *does* keep the full base
(`FILENAME.orig`), we can do better than patch: a true diff3 merge, which gives
trustworthy conflicts instead of best-effort ones reconstructed from hunks.

## The engine is already here

`metamath.js` already has a real Myers diff and a clean op-list primitive:

```js
// diffOps(a, b) -> array of [tag, line], tag in " " | "-" | "+"
//   " " line is common (present in both a and b)
//   "-" line is in a only (deleted going a -> b)
//   "+" line is in b only (added   going a -> b)
var ops = diffOps(aLines, bLines);      // ~line 1021, Myers with prefix/suffix trim + slideRuns
```

`unifiedDiff` (line 1039) is built on top of it.  A diff3 merge needs nothing
more than two `diffOps` calls against the base plus a merge walk — the hard part
(the diff itself) is done.

## Algorithm (diff3)

```
base  = FILENAME.orig split into lines
ours  = FILENAME       split into lines
theirs= FILENAME.new   split into lines

A = diffOps(base, ours)     // how the local copy diverged from base
B = diffOps(base, theirs)   // how upstream diverged from base
```

Both A and B are expressed against the *same* base coordinate, so base lines
that are common (" ") in **both** A and B are synchronization anchors.  Walk the
two op lists together, advancing on base lines:

1. While the next op in both A and B is a common (" ") base line, emit that line
   (it is unchanged on both sides) and advance all three cursors.
2. Otherwise collect a *divergent chunk*: consume A and B up to the next shared
   base anchor, giving three slices for the chunk — `baseSlice`, `oursSlice`,
   `theirsSlice`.  Classify:
   - `oursSlice === baseSlice`   -> only upstream changed here  -> emit `theirsSlice`
   - `theirsSlice === baseSlice` -> only local changed here     -> emit `oursSlice`
   - `oursSlice === theirsSlice` -> both made the same change   -> emit once
   - otherwise                   -> **conflict**, emit markers (see below)

(`===` here means element-wise line equality.)

Sketch:

```js
// Returns { lines: [...], conflicts: n }.
function merge3(base, ours, theirs) {
  var A = diffOps(base, ours), B = diffOps(base, theirs);
  var out = [], conflicts = 0;
  var ia = 0, ib = 0;                       // cursors into A and B

  // Helper: read the next chunk from one op list up to (not including) the next
  // base-common line, returning {base:[], side:[], next:index}.
  function nextChunk(ops, i) {
    var bslice = [], sslice = [];
    while (i < ops.length && ops[i][0] !== " ") {
      if (ops[i][0] === "-") bslice.push(ops[i][1]);   // base line, removed on this side
      else                   sslice.push(ops[i][1]);   // "+": line added on this side
      i++;
    }
    return { base: bslice, side: sslice, next: i };
  }

  while (ia < A.length || ib < B.length) {
    var ca = A[ia], cb = B[ib];
    if (ca && cb && ca[0] === " " && cb[0] === " ") {  // common base line on both sides
      out.push(ca[1]); ia++; ib++; continue;
    }
    // At least one side diverges over the next stretch of base lines.  Pull a
    // chunk from each so both cover the same base span, then classify.
    var ka = nextChunk(A, ia), kb = nextChunk(B, ib);
    ia = ka.next; ib = kb.next;
    var baseSlice = ka.base;                 // == kb.base when spans align (see note)
    var oursSlice = ka.side, theirsSlice = kb.side;

    if (eq(oursSlice, baseSlice))        push(out, theirsSlice);
    else if (eq(theirsSlice, baseSlice)) push(out, oursSlice);
    else if (eq(oursSlice, theirsSlice)) push(out, oursSlice);
    else { conflicts++; push(out, conflictBlock(oursSlice, baseSlice, theirsSlice)); }
  }
  return { lines: out, conflicts: conflicts };
}
```

**Note on span alignment (the fiddly part).** The one real subtlety is that a
divergent chunk from A and one from B may cover *different* numbers of base
lines.  The robust way is to sync on base coordinates: track how many base lines
each side has consumed (each `" "` or `"-"` op advances the base cursor; `"+"`
does not), and keep extending whichever chunk is behind in base coordinate until
both chunks end on the same base line.  The sketch above assumes aligned spans
for readability; the real implementation must extend chunks to a shared base
boundary before classifying.  This is the classic diff3 chunking and is the bulk
of the ~100-150 lines this feature needs.

## Conflict markers

Prefer diff3-style three-way markers (they show the base, which helps a reader
see what upstream actually did — valuable in a proof database):

```
<<<<<<< FILENAME (local)
...oursSlice...
||||||| FILENAME.orig (base)
...baseSlice...
=======
...theirsSlice...
>>>>>>> FILENAME.new (upstream)
```

Git-style two-way (drop the base section) is an option if the three-way form is
too noisy, but three-way is the recommended default here.

## Workflow and the baseline-advance gotcha

Suggested entry point: a new "Update" action taking `FILENAME` + `FILENAME.new`,
using `FILENAME.orig` as base.  It writes the merged `FILENAME`, reports
`N conflicts`, and opens the result in the editor when there are conflicts to fix.

- **The `.diff` file is not needed for the merge.** diff3 runs on the three full
  files directly.  We can still *produce* `FILENAME.diff` (base -> new) as a
  human-readable "what upstream changed" artifact, and "`.new` equals `.orig`?"
  is a cheap early-out (nothing to merge), but the merge must not depend on the
  diff — reconstructing from hunks would discard the base information that makes
  this the good case.

- **Advance the baseline — easy to get wrong.** After a *clean* merge (zero
  conflicts), replace `FILENAME.orig` with the `FILENAME.new` contents, because
  the downloaded version is now the common ancestor for the next update.  If
  `.orig` is left at the old version, every future update re-derives the local
  edits as "changes" and conflicts snowball.  When the merge has unresolved
  conflicts, leave `.orig` alone until the human has resolved and re-saved, then
  advance it (or have them re-run once resolved).

## Rough size / risk

- New code: the merge walk (~100-150 lines) plus a small dialog and the
  baseline-advance/cleanup bookkeeping.
- Reuses `diffOps` / Myers wholesale — no new diff engine.
- Main risk is the chunk span-alignment logic; worth a handful of unit tests
  (both-changed-same, both-changed-different, adjacent edits, edits at EOF,
  insert-vs-delete on the same span).
