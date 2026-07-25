(function () {
  "use strict";

  var termEl = document.getElementById("term");
  var cmdEl = document.getElementById("cmd");
  var statusEl = document.getElementById("status");
  var statusTextEl = document.getElementById("statustext");
  var busybarEl = document.getElementById("busybar");
  var inputrowEl = document.getElementById("inputrow");

  // ---- current-state indicator ----------------------------------------
  // One place that reflects "what is metamath doing right now":
  //   starting  wasm still loading, first prompt not shown yet
  //   ready     waiting for you to type a command
  //   busy      running a command (the main thread may be fully blocked)
  //   download  fetching a database over the network
  //   exited    the program has quit
  //   error     something failed
  // The header shows only a small icon per state; the words are in its
  // tooltip, so the state costs almost no horizontal space.
  var STATE_TITLES = {
    starting: "Starting metamath…",
    ready:    "Ready for a command",
    busy:     "Working…",
    download: "Downloading…",
    exited:   "Exited (reload the page to restart)",
    error:    "Error"
  };
  function setState(s, pct) {
    statusEl.dataset.state = s;
    statusEl.title = STATE_TITLES[s] || s;
    statusEl.setAttribute("aria-label", STATE_TITLES[s] || s);
    // Top bar: it slides while working (length unknown) and fills while
    // downloading (percentage known); hidden otherwise.
    if (s === "busy" || s === "starting") {
      busybarEl.className = "active indeterminate";
      statusTextEl.textContent = "";
    } else if (s === "download" && pct != null) {
      busybarEl.className = "active determinate";
      busybarEl.style.setProperty("--pct", Math.round(pct) + "%");
      statusTextEl.textContent = Math.round(pct) + "%";
    } else if (s === "download") {
      busybarEl.className = "active indeterminate";  // no content-length
      statusTextEl.textContent = "";
    } else {
      busybarEl.className = "";
      statusTextEl.textContent = "";
    }
    // The prompt spinner and the progress cursor mean "a command is running",
    // so they follow busy/starting, not downloading: you can still type while
    // a download is in progress.
    var computing = (s === "busy" || s === "starting");
    inputrowEl.classList.toggle("busy", computing);
    document.body.classList.toggle("busy", computing);
  }
  function goBusy() { setState("busy"); }
  function goReady() { setState("ready"); flushPersist(); }
  setState("starting");

  // ---- terminal output ------------------------------------------------
  // Metamath-exe writes prompts such as "MM> " with no trailing new-line, so the
  // output is collected character by character (see FS.init below) and flushed
  // on an animation frame.  Line-buffered output would hide the prompts.
  var pending = "";
  var flushQueued = false;
  function flush() {
    flushQueued = false;
    if (!pending) return;
    var atBottom =
      termEl.scrollHeight - termEl.scrollTop - termEl.clientHeight < 40;
    termEl.appendChild(document.createTextNode(pending));
    pending = "";
    if (atBottom) termEl.scrollTop = termEl.scrollHeight;
  }
  var decoder = new TextDecoder("utf-8");
  var utf8buf = [];
  function outByte(b) {
    if (b === null) return;
    utf8buf.push(b);
    // Decode once a character looks complete (ASCII fast path, else on 0x80<).
    if (b < 0x80 || utf8buf.length >= 4) {
      pending += decoder.decode(new Uint8Array(utf8buf), { stream: true });
      utf8buf.length = 0;
    }
    if (!flushQueued) { flushQueued = true; requestAnimationFrame(flush); }
  }
  function echo(text) { pending += text; if (!flushQueued) { flushQueued = true; requestAnimationFrame(flush); } }

  // ---- keyboard input, with command history ---------------------------
  var waiting = null;   // resolver for a pending mmReadLine()
  var queued = [];      // lines typed before metamath asked for them
  var history = [];
  var histPos = 0;

  function submit(line) {
    if (waiting) { var r = waiting; waiting = null; goBusy(); r(line); }
    else queued.push(line);
  }

  function mmReadLine() {
    return new Promise(function (resolve) {
      // A line already typed ahead: metamath keeps working on the next one.
      if (queued.length) { goBusy(); resolve(queued.shift()); }
      // Nothing queued: metamath is now idle, waiting for the user.
      else { goReady(); waiting = resolve; }
    });
  }

  function sendCurrentLine() {
    var line = cmdEl.value;
    cmdEl.value = "";
    if (line.trim() !== "") {
      history.push(line);
      if (history.length > 500) history.shift();
    }
    histPos = history.length;
    echo(line + "\n");   // local echo, since the program does not echo input
    submit(line);
  }

  // The handler is on the document, not just the input box, so that typing
  // anywhere on the page goes to the command line.  Users should not have to
  // find the one text field first.
  document.addEventListener("keydown", function (e) {
    var tag = e.target.tagName;
    // Leave the other controls (buttons, the file chooser) alone.
    if (e.target !== cmdEl &&
        (tag === "INPUT" || tag === "SELECT" || tag === "TEXTAREA" ||
         tag === "BUTTON")) return;
    if (e.ctrlKey || e.metaKey || e.altKey) return;  // let shortcuts through
    if (cmdEl.disabled) return;

    if (e.key === "Enter") {
      sendCurrentLine();
      e.preventDefault();
    } else if (e.key === "ArrowUp") {
      cmdEl.focus();
      if (histPos > 0) { histPos--; cmdEl.value = history[histPos] || ""; }
      e.preventDefault();
    } else if (e.key === "ArrowDown") {
      cmdEl.focus();
      if (histPos < history.length) {
        histPos++;
        cmdEl.value = histPos === history.length ? "" : (history[histPos] || "");
      }
      e.preventDefault();
    } else if (e.target !== cmdEl && e.key.length === 1) {
      // A printable character typed while the transcript had focus: move it
      // to the command line rather than dropping it.
      cmdEl.focus();
      cmdEl.value += e.key;
      e.preventDefault();
    } else if (e.target !== cmdEl && e.key === "Backspace") {
      cmdEl.focus();
      cmdEl.value = cmdEl.value.slice(0, -1);
      e.preventDefault();
    }
  });

  // Clicking the transcript focuses the command line, unless text is being
  // selected for copying.
  termEl.addEventListener("mouseup", function () {
    if (String(window.getSelection()) === "") cmdEl.focus();
  });

  // ---- databases ------------------------------------------------------
  var Mod = null;

  async function fetchDb(url, name) {
    if (!Mod) return;
    setState("download", null);
    try {
      var resp = await fetch(url);            // plain GET: avoids a preflight
      if (!resp.ok) throw new Error("HTTP " + resp.status);
      var total = +resp.headers.get("content-length") || 0;
      var reader = resp.body.getReader();
      var chunks = [], got = 0;
      for (;;) {
        var r = await reader.read();
        if (r.done) break;
        chunks.push(r.value); got += r.value.length;
        setState("download", total ? (got * 100 / total) : null);
      }
      var buf = new Uint8Array(got), off = 0;
      chunks.forEach(function (c) { buf.set(c, off); off += c.length; });
      // Keep a pristine baseline as name.orig, then copy it to the working
      // file.  The baseline lets the user diff against or reset to the original.
      var orig = name + ".orig";
      Mod.FS.writeFile(orig, buf);
      Mod.FS.writeFile(name, buf);   // working copy of the pristine .orig
      flushPersist();   // UI write while metamath is idle: persist it now
      // The download is done; metamath itself was idle at its prompt.
      setState(waiting ? "ready" : "busy");
      // Downloading only puts the file in the virtual filesystem.
      // The user still has to tell metamath to read it, which is easy to miss,
      // so say so in a dialog as well as in the transcript.
      echo('\n[' + name + ' downloaded into the virtual filesystem' +
           ' (' + Math.round(got / 1048576) + ' MB).' +
           '\n A pristine copy was kept as ' + orig + '.' +
           '\n Now type:  read "' + name + '"  ]\n');
      offerRead(name, "Download complete", "Downloading");
    } catch (err) {
      // metamath is still fine and waiting; report the failure in the
      // transcript and return to the normal state.
      setState(waiting ? "ready" : "busy");
      echo("\n[could not download " + name + ": " + err.message +
           ". You can use \"Add from computer\" instead.]\n");
    }
  }

  // ---- offer to read a file that just arrived --------------------------
  // Putting a file in the virtual filesystem is not the same as metamath
  // reading it, which is easy to miss, so offer the read straight away.  Both
  // ways of getting a file, downloading it and adding it from your computer,
  // use this.
  var doneDlg = document.getElementById("done-dlg");
  var doneTitle = document.getElementById("done-title");
  var doneMsg = document.getElementById("done-msg");
  var pendingRead = "";

  function offerRead(name, heading, action) {
    pendingRead = 'read "' + name + '"';
    doneTitle.textContent = heading;
    doneMsg.innerHTML =
      "<strong>" + name + "</strong> is now in the virtual filesystem. " +
      action + " does not read it into metamath, so run " +
      "<code>" + pendingRead + "</code> to do that.";
    // Name the command on the button, so that it is clear what will be run.
    doneRun.textContent = 'Run ' + pendingRead;
    if (typeof doneDlg.showModal === "function") doneDlg.showModal();
  }

  var doneRun = document.getElementById("done-run");
  document.getElementById("done-close").onclick = function () { doneDlg.close(); };
  doneRun.onclick = function () {
    doneDlg.close();
    cmdEl.value = pendingRead;
    sendCurrentLine();
    cmdEl.focus();
  };

  // Before downloading, guard against clobbering an existing file that may hold
  // the user's work.  The check happens FIRST, so a download is never started
  // if the user is only going to cancel it.
  function fileExists(name) {
    if (!Mod) return false;
    try { Mod.FS.stat("/work/" + name); return true; }
    catch (e) { return false; }
  }
  // A non-colliding backup name: name.bak, then name.bak.2, name.bak.3, ...
  function backupName(name) {
    var base = name + ".bak";
    if (!fileExists(base)) return base;
    for (var i = 2; ; i++) { if (!fileExists(base + "." + i)) return base + "." + i; }
  }

  var conflictDlg = document.getElementById("conflict-dlg");
  var conflictMsg = document.getElementById("conflict-msg");
  var pendingDownload = null;   // {url, name} awaiting a conflict choice

  // The files a download will write: the pristine baseline and the working copy.
  function downloadTargets(name) { return [name + ".orig", name]; }

  function startDownload(url, name) {
    if (!Mod) return;
    var existing = downloadTargets(name).filter(fileExists);
    if (existing.length === 0) { fetchDb(url, name); return; }  // nothing to clobber
    // One or more target files already exist and may hold the user's work.
    // Overwriting name.orig would also destroy the baseline used for diffs, so
    // ask about ANY existing target before touching the network.
    pendingDownload = { url: url, name: name, existing: existing };
    document.getElementById("conflict-title").textContent =
      existing.length > 1 ? "Replace existing files?" : "Replace existing file?";
    conflictMsg.innerHTML =
      "Downloading a fresh copy would replace " +
      existing.map(function (n) { return "<strong>" + n + "</strong>"; }).join(" and ") +
      ", which may contain your work.";
    if (typeof conflictDlg.showModal === "function") conflictDlg.showModal();
    else fetchDb(url, name);   // no <dialog> support: fall back to downloading
  }

  document.getElementById("conflict-cancel").onclick = function () {
    conflictDlg.close(); pendingDownload = null;
  };
  document.getElementById("conflict-overwrite").onclick = function () {
    conflictDlg.close();
    var d = pendingDownload; pendingDownload = null;
    if (d) fetchDb(d.url, d.name);
  };
  document.getElementById("conflict-rename").onclick = function () {
    conflictDlg.close();
    var d = pendingDownload; pendingDownload = null;
    if (!d) return;
    // Preserve every existing target under a new name, so nothing is lost.
    try {
      d.existing.forEach(function (n) {
        var bak = backupName(n);
        Mod.FS.rename("/work/" + n, "/work/" + bak);
        echo("\n[renamed existing " + n + " to " + bak + "]");
      });
      echo("\n");
    } catch (e) {
      echo("\n[could not rename: " + e.message + "]\n");
      return;
    }
    flushPersist();
    fetchDb(d.url, d.name);
  };

  // Databases offered by the "Download database" picker.  Add entries here to
  // list more; the picker and the download flow need no other change.
  var DATABASES = [
    { name: "set.mm",  url: "https://us.metamath.org/metamath/set.mm",
      desc: "Logic and ZFC set theory (the Metamath Proof Explorer)" },
    { name: "iset.mm", url: "https://us.metamath.org/metamath/iset.mm",
      desc: "Intuitionistic logic and set theory" }
  ];
  var dbDlg = document.getElementById("db-dlg");
  var dbList = document.getElementById("db-list");

  function openDbPicker() {
    if (!Mod) return;
    dbList.innerHTML = "";
    DATABASES.forEach(function (db) {
      var b = document.createElement("button");
      b.className = "dbrow";
      b.title = "Download " + db.name;
      var n = document.createElement("span"); n.className = "dbname"; n.textContent = db.name;
      var d = document.createElement("span"); d.className = "dbdesc"; d.textContent = db.desc;
      b.appendChild(n); b.appendChild(d);
      b.onclick = function () { dbDlg.close(); startDownload(db.url, db.name); };
      dbList.appendChild(b);
    });
    if (typeof dbDlg.showModal === "function") dbDlg.showModal();
  }
  document.getElementById("load-db").onclick = openDbPicker;
  document.getElementById("db-close").onclick = function () { dbDlg.close(); };

  document.getElementById("add-btn").onclick = function () {
    document.getElementById("addfile").click();
  };
  document.getElementById("addfile").onchange = async function (e) {
    var f = e.target.files[0];
    if (!f || !Mod) return;
    var buf = new Uint8Array(await f.arrayBuffer());
    Mod.FS.writeFile(f.name, buf);
    flushPersist();   // UI write while metamath is idle: persist it now
    // "Added", not "uploaded": the file is copied into the virtual
    // filesystem and is not sent anywhere.
    echo('\n[' + f.name + ' added to the virtual filesystem.' +
         ' It was not sent anywhere.' +
         '\n Now type:  read "' + f.name + '"  ]\n');
    // Only a database is worth offering to read; a file added for some other
    // purpose, such as a command file for SUBMIT, is not.
    if (/\.mm$/i.test(f.name)) {
      offerRead(f.name, "File added", "Adding it");
    }
    // Let the same file be chosen again later.
    e.target.value = "";
  };

  // ---- download files the program wrote -------------------------------
  // FS.stat reports mtime as a Date in some builds and a number in others;
  // normalise it to milliseconds.
  function mtimeMs(st) {
    var m = st && st.mtime;
    if (m == null) return 0;
    if (typeof m === "number") return m;
    return m.getTime ? m.getTime() : (+m || 0);
  }
  // Local time as a fixed-width "YYYY-MM-DD HH:mm:ss": 24-hour, sorts visually,
  // understood everywhere, and the fixed width keeps the columns aligned.
  function fmtTime(ms) {
    if (!ms) return "";
    var d = new Date(ms), p = function (n) { return (n < 10 ? "0" : "") + n; };
    return d.getFullYear() + "-" + p(d.getMonth() + 1) + "-" + p(d.getDate()) +
      " " + p(d.getHours()) + ":" + p(d.getMinutes()) + ":" + p(d.getSeconds());
  }
  // The files metamath can see, which is what a listing or a save offers.
  function listWorkFiles() {
    if (!Mod) return [];
    var names;
    try {
      names = Mod.FS.readdir("/work")
        .filter(function (n) { return n !== "." && n !== ".."; });
    } catch (e) { return []; }
    return names.sort().map(function (n) {
      var size = "?", mtime = 0;
      try {
        var st = Mod.FS.stat("/work/" + n);
        size = st.size; mtime = mtimeMs(st);
      } catch (e) { /* keep defaults */ }
      return { name: n, size: size, mtime: mtime };
    });
  }

  function saveToComputer(name) {
    ensureLoaded([name]).then(function () {
      var data = Mod.FS.readFile("/work/" + name);
      var url = URL.createObjectURL(new Blob([data]));
      var a = document.createElement("a");
      a.href = url;
      a.download = name;
      a.click();
      setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
    });
  }

  // ---- virtual file explorer ------------------------------------------
  // A small file manager standing in for the missing OS shell: list the files
  // in /work with their size and modified time, and rename / copy / delete /
  // save the selected one.  (The text editor and diff/patch will add buttons
  // to the same action row.)  Metamath-exe itself has no command that lists or
  // manages files, so the page provides this.
  var exploreDlg = document.getElementById("explore-dlg");
  var exploreList = document.getElementById("explore-list");
  var exploreMsg = document.getElementById("explore-msg");
  var exploreSel = null;   // name of the selected file, or null
  var exBtn = {
    edit:   document.getElementById("explore-edit"),
    diff:   document.getElementById("explore-diff"),
    patch:  document.getElementById("explore-patch"),
    rename: document.getElementById("explore-rename"),
    copy:   document.getElementById("explore-copy"),
    save:   document.getElementById("explore-save"),
    del:    document.getElementById("explore-delete")
  };

  // Only touch /work while metamath is parked at its prompt, so a file
  // operation never races a running command.
  function metamathIdle() { return statusEl.dataset.state === "ready"; }
  function requireIdle() {
    if (metamathIdle()) return true;
    exploreMsg.textContent = "Wait until metamath is idle (ready) before changing files.";
    return false;
  }

  function exploreSelect(name) {
    exploreSel = name;
    var rows = exploreList.querySelectorAll(".filerow");
    for (var i = 0; i < rows.length; i++) {
      rows[i].classList.toggle("sel", rows[i].dataset.name === name);
    }
    var on = !!name;
    exBtn.edit.disabled = !on;
    exBtn.diff.disabled = !on;
    exBtn.patch.disabled = !on;
    exBtn.rename.disabled = !on;
    exBtn.copy.disabled = !on;
    exBtn.save.disabled = !on;
    exBtn.del.disabled = !on;
  }

  function renderExplore() {
    var files = listWorkFiles();
    exploreList.innerHTML = "";
    if (!files.length) {
      var none = document.createElement("div");
      none.className = "filenone";
      none.textContent = "(no files yet; download set.mm or add a file to begin)";
      exploreList.appendChild(none);
      exploreSelect(null);
      return;
    }
    files.forEach(function (f) {
      var row = document.createElement("button");
      row.className = "filerow";
      row.dataset.name = f.name;
      var fn = document.createElement("span"); fn.className = "fn"; fn.textContent = f.name;
      var sz = document.createElement("span"); sz.className = "sz"; sz.textContent = f.size + " B";
      var mt = document.createElement("span"); mt.className = "mt";
      mt.textContent = fmtTime(f.mtime);
      row.appendChild(fn); row.appendChild(sz); row.appendChild(mt);
      row.onclick = function () { exploreSelect(f.name); };
      exploreList.appendChild(row);
    });
    // Keep the previous selection if that file still exists.
    if (exploreSel && files.some(function (f) { return f.name === exploreSel; })) {
      exploreSelect(exploreSel);
    } else {
      exploreSelect(null);
    }
  }

  function openExplore() {
    if (!Mod) return;
    exploreMsg.textContent = "Select a file, then choose an action.";
    renderExplore();
    if (typeof exploreDlg.showModal === "function") exploreDlg.showModal();
  }

  document.getElementById("explore").onclick = openExplore;
  document.getElementById("explore-close").onclick = function () { exploreDlg.close(); };
  document.getElementById("explore-new").onclick = function () {
    if (!requireIdle()) return;
    var name = window.prompt("New file name:", "");
    if (name == null) return;
    name = name.trim();
    if (!name) return;
    if (name.indexOf("/") !== -1) { exploreMsg.textContent = "Name cannot contain '/'."; return; }
    if (fileExists(name)) { exploreMsg.textContent = name + " already exists."; return; }
    try { Mod.FS.writeFile("/work/" + name, ""); }
    catch (e) { exploreMsg.textContent = "Could not create " + name + ": " + e.message; return; }
    echo("\n[created " + name + "]\n");
    exploreSel = name;   // select it so Edit and the rest act on it
    flushPersist();
    renderExplore();
  };

  exBtn.save.onclick = function () {
    if (exploreSel) saveToComputer(exploreSel);   // read-only: no idle guard needed
  };
  exBtn.del.onclick = function () {
    if (!exploreSel || !requireIdle()) return;
    var name = exploreSel;
    if (!window.confirm("Delete " + name + "?  This cannot be undone.")) return;
    try { Mod.FS.unlink("/work/" + name); }
    catch (e) { exploreMsg.textContent = "Could not delete " + name + ": " + e.message; return; }
    echo("\n[deleted " + name + "]\n");
    flushPersist();
    renderExplore();
  };
  exBtn.rename.onclick = function () {
    if (!exploreSel || !requireIdle()) return;
    var name = exploreSel;
    var to = window.prompt("Rename " + name + " to:", name);
    if (to == null) return;                 // cancelled
    to = to.trim();
    if (!to || to === name) return;
    if (to.indexOf("/") !== -1) { exploreMsg.textContent = "Name cannot contain '/'."; return; }
    if (fileExists(to)) { exploreMsg.textContent = to + " already exists."; return; }
    try { Mod.FS.rename("/work/" + name, "/work/" + to); }
    catch (e) { exploreMsg.textContent = "Could not rename: " + e.message; return; }
    echo("\n[renamed " + name + " to " + to + "]\n");
    exploreSel = to;
    flushPersist();
    renderExplore();
  };
  exBtn.copy.onclick = function () {
    if (!exploreSel || !requireIdle()) return;
    var name = exploreSel;
    var to = window.prompt("Copy " + name + " to:", name + ".copy");
    if (to == null) return;
    to = to.trim();
    if (!to || to === name) return;
    if (to.indexOf("/") !== -1) { exploreMsg.textContent = "Name cannot contain '/'."; return; }
    if (fileExists(to)) { exploreMsg.textContent = to + " already exists."; return; }
    ensureLoaded([name]).then(function () {
      try {
        var data = Mod.FS.readFile("/work/" + name);
        Mod.FS.writeFile("/work/" + to, data);
      } catch (e) { exploreMsg.textContent = "Could not copy: " + e.message; return; }
      echo("\n[copied " + name + " to " + to + "]\n");
      exploreSel = to;
      flushPersist();
      renderExplore();
    });
  };
  exBtn.edit.onclick = function () {
    if (exploreSel && requireIdle()) openEditor(exploreSel);
  };

  // ---- text editor (segmented) -------------------------------------------
  // A textarea cannot hold a large file (set.mm is ~50 MB), so the editor keeps
  // the whole file in memory as an array of lines and shows only one SEGMENT of
  // them at a time.  Editing a segment is committed back into that array when you
  // navigate or save; the readout shows the caret's line and column within the
  // WHOLE file, reflecting edits made so far.  Text is UTF-8 in and out.  Cut,
  // copy, and paste are the textarea's own shortcuts (Ctrl/Cmd-X/C/V).
  var editorDlg = document.getElementById("editor-dlg");
  var editorArea = document.getElementById("editor-area");
  var editorTitle = document.getElementById("editor-title");
  var findText = document.getElementById("editor-findtext");
  var replText = document.getElementById("editor-repltext");
  var findInfo = document.getElementById("editor-findinfo");
  var posEl = document.getElementById("editor-pos");
  var segInfoEl = document.getElementById("editor-seginfo");
  var prevBtn = document.getElementById("editor-prev");
  var nextBtn = document.getElementById("editor-next");
  var gotoInput = document.getElementById("editor-gotoline");
  var saveBtn = document.getElementById("editor-save");

  var SEGMENT = 2000;      // lines shown in one segment
  var docLines = null;     // the whole file, split into lines
  var editorName = null;   // file being edited, or null
  var editorDirty = false; // unsaved changes since the file was last written
  var segStart = 0;       // index in docLines of the first line of the segment
  var segLines = 0;       // how many docLines the current segment covers

  function setEditorInfo(t) { findInfo.textContent = t || ""; }
  function updateEditorTitle() {
    editorTitle.textContent = "Edit: " + editorName + (editorDirty ? "  *" : "");
    saveBtn.disabled = !editorDirty;   // nothing to save when not dirty
  }

  // Count of "\n" in text[0..pos); and the 1-based column at pos.
  function newlinesBefore(text, pos) {
    var n = 0, p = text.indexOf("\n");
    while (p !== -1 && p < pos) { n++; p = text.indexOf("\n", p + 1); }
    return n;
  }
  function columnAt(text, pos) {
    return pos - text.lastIndexOf("\n", pos - 1);   // lastIndexOf is -1 on line 1
  }
  // Char offset of the start of line `line` (0-based, within the segment text).
  function offsetOfLine(text, line) {
    var off = 0;
    for (var i = 0; i < line; i++) {
      var nl = text.indexOf("\n", off);
      if (nl === -1) return text.length;
      off = nl + 1;
    }
    return off;
  }

  // Cached caret position (whole-file line/column, and 0-based line within the
  // segment), refreshed only when the caret actually moves.
  var caretGLine = 1, caretCol = 1, caretSLine = 0;
  var lineHeightPx = 0;
  function lineHeight() {
    if (!lineHeightPx) lineHeightPx = parseFloat(getComputedStyle(editorArea).lineHeight) || 16;
    return lineHeightPx;
  }
  function updateCaret() {
    var text = editorArea.value, pos = editorArea.selectionStart;
    caretSLine = newlinesBefore(text, pos);     // 0-based line within the segment
    caretGLine = segStart + caretSLine + 1;     // whole-file line
    caretCol = columnAt(text, pos);
    refreshReadout();
  }
  // Keep the readout current: the caret's line:column when the caret is on
  // screen, or the file line at the top of the view when the caret is scrolled
  // out of sight (restored as soon as the caret moves back into view).
  function refreshReadout() {
    var lh = lineHeight(), top = editorArea.scrollTop;
    var caretY = caretSLine * lh;
    if (caretY >= top - 1 && caretY < top + editorArea.clientHeight) {
      posEl.textContent = caretGLine + ":" + caretCol;                  // caret on screen
    } else {
      posEl.textContent = String(segStart + Math.floor(top / lh) + 1);  // top of view
    }
  }
  function updateSegmentInfo() {
    var total = docLines ? docLines.length : 0;
    segInfoEl.textContent = "(lines " + (total ? segStart + 1 : 0).toLocaleString() +
      "-" + (segStart + segLines).toLocaleString() + " of " + total.toLocaleString() + ")";
    prevBtn.disabled = segStart <= 0;
    nextBtn.disabled = segStart + segLines >= total;
  }

  // Put the edited segment back into docLines.
  function commitSegment() {
    if (docLines == null) return;
    var lines = editorArea.value.split("\n");
    if (lines.length === segLines) {
      for (var i = 0; i < lines.length; i++) docLines[segStart + i] = lines[i];
    } else {
      // Line count changed: rebuild around the segment (avoids huge argument spreads).
      docLines = docLines.slice(0, segStart).concat(lines, docLines.slice(segStart + segLines));
    }
    segLines = lines.length;
  }

  function scrollCaretIntoView() {
    var line = newlinesBefore(editorArea.value, editorArea.selectionStart);
    editorArea.scrollTop = Math.max(0, line * lineHeight() - editorArea.clientHeight / 2);
  }
  // Show the segment starting at docLines[start]; optionally place the caret at a
  // whole-file line/column.
  function loadSegment(start, caretLine, caretCol) {
    if (start < 0) start = 0;
    if (start > docLines.length - 1) start = Math.max(0, docLines.length - 1);
    lineHeightPx = 0;   // recompute the line height (handles a zoom change)
    segStart = start;
    segLines = Math.min(SEGMENT, docLines.length - segStart);
    editorArea.value = docLines.slice(segStart, segStart + segLines).join("\n");
    editorArea.focus();
    var off = 0;
    if (caretLine != null && caretLine >= segStart && caretLine < segStart + segLines) {
      var ls = offsetOfLine(editorArea.value, caretLine - segStart);
      var le = editorArea.value.indexOf("\n", ls);
      if (le === -1) le = editorArea.value.length;
      off = Math.min(ls + (caretCol || 0), le);   // clamp the caret within the line
    }
    editorArea.setSelectionRange(off, off);
    scrollCaretIntoView();
    updateSegmentInfo();
    updateCaret();
  }

  function prevSegment() {
    if (segStart <= 0) return;
    commitSegment();
    loadSegment(Math.max(0, segStart - SEGMENT));
  }
  function nextSegment() {
    if (segStart + segLines >= docLines.length) return;
    commitSegment();
    loadSegment(segStart + segLines);
  }
  function gotoLine(line1) {                       // 1-based whole-file line
    commitSegment();
    loadSegment(Math.max(0, (line1 - 1) - 5), line1 - 1, 0);   // a little context above
  }

  function hasNext() { return docLines && segStart + segLines < docLines.length; }
  function hasPrev() { return docLines && segStart > 0; }
  function caretLineInSegment() { return newlinesBefore(editorArea.value, editorArea.selectionStart); }
  function caretColInLine() { return columnAt(editorArea.value, editorArea.selectionStart) - 1; }
  // Turn to the adjacent segment, placing the caret on the first (or last) line
  // so that Arrow/Page keys glide across the segment boundary with only slight
  // resistance.
  function turnToNext(col) {
    if (!hasNext()) return;
    commitSegment();
    var start = segStart + segLines;
    loadSegment(start, start, col || 0);
  }
  function turnToPrev(col, toBottom) {
    if (!hasPrev()) return;
    commitSegment();
    var start = Math.max(0, segStart - SEGMENT);
    var span = Math.min(SEGMENT, docLines.length - start);
    loadSegment(start, toBottom ? start + span - 1 : start, col || 0);
  }

  function openEditor(name) {
    ensureLoaded([name]).then(function () {
      var bytes;
      try { bytes = Mod.FS.readFile("/work/" + name); }
      catch (e) { exploreMsg.textContent = "Could not open " + name + ": " + e.message; return; }
      // docLines is now the working copy; drop the file's bytes from /work again.
      unloadFile(name);
      editorName = name;
      docLines = new TextDecoder("utf-8").decode(bytes).split("\n");
      editorDirty = false;
      setEditorInfo(bytes.indexOf(0) !== -1
        ? "warning: this file contains NUL bytes and may be binary" : "");
      editorDlg.showModal();
      loadSegment(0);
      updateEditorTitle();
    });
  }

  function editorSave() {
    if (!editorName || !editorDirty) return;   // nothing to save
    if (!metamathIdle()) { setEditorInfo("Wait until metamath is idle to save."); return; }
    commitSegment();
    var bytes = new TextEncoder().encode(docLines.join("\n"));   // whole file, UTF-8
    try { Mod.FS.writeFile("/work/" + editorName, bytes); }
    catch (e) { setEditorInfo("Could not save: " + e.message); return; }
    editorDirty = false;
    updateEditorTitle();
    setEditorInfo("saved (" + bytes.length + " bytes)");
    echo("\n[saved " + editorName + " (" + bytes.length + " bytes)]\n");
    flushPersist();
  }

  function editorClose() {
    if (editorDirty && !window.confirm("Discard unsaved changes to " + editorName + "?")) return;
    editorDlg.close();
    editorName = null; docLines = null;
    renderExplore();   // reflect any new size/time in the explorer beneath
  }

  function markEdited() {
    if (!editorDirty) { editorDirty = true; updateEditorTitle(); }
    updateCaret();
  }
  editorArea.addEventListener("input", markEdited);
  editorArea.addEventListener("keyup", updateCaret);
  editorArea.addEventListener("mouseup", updateCaret);
  editorArea.addEventListener("focus", updateCaret);
  editorArea.addEventListener("scroll", refreshReadout);

  // Glide across segment boundaries: Down on the last line moves to the next
  // segment, Up on the first line to the previous, and Page Down/Up turn the
  // segment once the current one is scrolled to its end.
  editorArea.addEventListener("keydown", function (e) {
    if (e.ctrlKey || e.metaKey || e.altKey) return;   // Ctrl-nav handled on the dialog
    if (e.key === "ArrowDown" && !e.shiftKey && caretLineInSegment() === segLines - 1 && hasNext()) {
      e.preventDefault(); turnToNext(caretColInLine());
    } else if (e.key === "ArrowUp" && !e.shiftKey && caretLineInSegment() === 0 && hasPrev()) {
      e.preventDefault(); turnToPrev(caretColInLine(), true);
    } else if (e.key === "PageDown" && hasNext() &&
               editorArea.scrollTop + editorArea.clientHeight >= editorArea.scrollHeight - 2) {
      e.preventDefault(); turnToNext(0);
    } else if (e.key === "PageUp" && hasPrev() && editorArea.scrollTop <= 0) {
      e.preventDefault(); turnToPrev(0, true);
    }
  });

  // Scroll-wheel past the segment edge: when the textarea is pinned at its top
  // or bottom the wheel cannot scroll it, so watch that wasted scrolling and
  // turn the segment after about one extra wheel notch in the same direction
  // (within a short window).  A sub-notch dribble does nothing; one deliberate
  // extra notch turns the segment.
  var WHEEL_TURN_PX = 40;   // ~one wheel notch of past-edge scroll turns a segment
  var overscroll = 0, overscrollDir = 0, overscrollTimer = null;
  function wheelPixels(e) {
    var d = e.deltaY;
    if (e.deltaMode === 1) d *= 16;                            // lines -> px
    else if (e.deltaMode === 2) d *= editorArea.clientHeight;  // pages -> px
    return d;
  }
  editorArea.addEventListener("wheel", function (e) {
    var d = wheelPixels(e);
    if (!d) return;
    var atBottom = editorArea.scrollTop + editorArea.clientHeight >= editorArea.scrollHeight - 1;
    var atTop = editorArea.scrollTop <= 0;
    var pastEdge = (d > 0 && atBottom && hasNext()) || (d < 0 && atTop && hasPrev());
    if (!pastEdge) { overscroll = 0; overscrollDir = 0; return; }   // normal scrolling
    e.preventDefault();                          // own the gesture at the edge
    var dir = d > 0 ? 1 : -1;
    if (dir !== overscrollDir) { overscroll = 0; overscrollDir = dir; }
    overscroll += Math.abs(d);
    clearTimeout(overscrollTimer);
    overscrollTimer = setTimeout(function () { overscroll = 0; overscrollDir = 0; }, 500);
    if (overscroll >= WHEEL_TURN_PX) {   // about one extra notch past the edge
      overscroll = 0; overscrollDir = 0;
      if (dir > 0) turnToNext(0); else turnToPrev(0, true);
    }
  }, { passive: false });

  document.getElementById("editor-save").onclick = editorSave;
  document.getElementById("editor-close").onclick = editorClose;
  prevBtn.onclick = prevSegment;
  nextBtn.onclick = nextSegment;
  function doGoto() { var n = parseInt(gotoInput.value, 10); if (n >= 1) gotoLine(n); }
  document.getElementById("editor-goto").onclick = doGoto;
  gotoInput.addEventListener("keydown", function (e) {
    if (e.key === "Enter") { e.preventDefault(); doGoto(); }
  });

  // Escape runs the same unsaved-changes guard as Close.
  editorDlg.addEventListener("cancel", function (e) { e.preventDefault(); editorClose(); });
  // Keyboard shortcuts within the editor.
  editorDlg.addEventListener("keydown", function (e) {
    var mod = e.ctrlKey || e.metaKey;
    if (mod && (e.key === "s" || e.key === "S")) { e.preventDefault(); editorSave(); }
    else if (mod && (e.key === "f" || e.key === "F")) { e.preventDefault(); findText.focus(); findText.select(); }
    else if (mod && e.key === "PageDown") { e.preventDefault(); nextSegment(); }
    else if (mod && e.key === "PageUp") { e.preventDefault(); prevSegment(); }
  });

  // ---- find and replace across the whole file ----
  // Literal (not regex), case-sensitive; a match lies within a single line.
  function docFind(q, fromLine, fromCol) {          // forwards, incl. fromCol
    for (var i = fromLine; i < docLines.length; i++) {
      var col = docLines[i].indexOf(q, i === fromLine ? fromCol : 0);
      if (col !== -1) return { line: i, col: col };
    }
    return null;
  }
  function docFindPrev(q, fromLine, fromCol) {       // backwards, before fromCol
    for (var i = fromLine; i >= 0; i--) {
      var hay = (i === fromLine) ? docLines[i].slice(0, fromCol) : docLines[i];
      var col = hay.lastIndexOf(q);
      if (col !== -1) return { line: i, col: col };
    }
    return null;
  }
  function showMatch(hit, len) {
    if (hit.line < segStart || hit.line >= segStart + segLines) {
      loadSegment(Math.max(0, hit.line - 5));
    }
    var off = offsetOfLine(editorArea.value, hit.line - segStart) + hit.col;
    editorArea.focus();
    editorArea.setSelectionRange(off, off + len);
    scrollCaretIntoView();
    updateCaret();
    setEditorInfo("");
  }
  function editorFindNext() {
    var q = findText.value;
    if (!q) { setEditorInfo(""); return; }
    commitSegment();
    var text = editorArea.value, pos = editorArea.selectionEnd;
    var fromLine = segStart + newlinesBefore(text, pos);
    var hit = docFind(q, fromLine, columnAt(text, pos) - 1) || docFind(q, 0, 0);  // then wrap
    if (hit) showMatch(hit, q.length); else setEditorInfo("no matches");
  }
  function editorFindPrev() {
    var q = findText.value;
    if (!q) { setEditorInfo(""); return; }
    commitSegment();
    var text = editorArea.value, pos = editorArea.selectionStart;
    var fromLine = segStart + newlinesBefore(text, pos);
    var last = docLines.length - 1;
    var hit = docFindPrev(q, fromLine, columnAt(text, pos) - 1) ||
              docFindPrev(q, last, docLines[last].length);                        // then wrap
    if (hit) showMatch(hit, q.length); else setEditorInfo("no matches");
  }
  function editorReplaceOne() {
    var q = findText.value;
    if (!q) return;
    var sel = editorArea.value.substring(editorArea.selectionStart, editorArea.selectionEnd);
    if (sel === q) {
      editorArea.setRangeText(replText.value, editorArea.selectionStart, editorArea.selectionEnd, "end");
      markEdited();
    }
    editorFindNext();
  }
  function editorReplaceAll() {
    var q = findText.value;
    if (!q) return;
    commitSegment();
    var count = 0;
    for (var i = 0; i < docLines.length; i++) {
      if (docLines[i].indexOf(q) === -1) continue;
      var parts = docLines[i].split(q);
      count += parts.length - 1;
      docLines[i] = parts.join(replText.value);
    }
    // If the replacement introduced newlines, re-split so each array element is
    // one line again (keeps line numbering correct).
    if (count && replText.value.indexOf("\n") !== -1) {
      docLines = docLines.join("\n").split("\n");
    }
    if (count) { editorDirty = true; updateEditorTitle(); loadSegment(segStart); }
    setEditorInfo(count ? (count + (count === 1 ? " replacement" : " replacements")) : "no matches");
  }
  document.getElementById("editor-findnext").onclick = editorFindNext;
  document.getElementById("editor-findprev").onclick = editorFindPrev;
  document.getElementById("editor-replace").onclick = editorReplaceOne;
  document.getElementById("editor-replaceall").onclick = editorReplaceAll;
  findText.addEventListener("keydown", function (e) {
    if (e.key === "Enter") { e.preventDefault(); if (e.shiftKey) editorFindPrev(); else editorFindNext(); }
  });

  // ---- diff -----------------------------------------------------------
  // Write a unified (patch-compatible) diff of two files in /work.  By default
  // it compares a file with its .orig baseline and saves to name.diff, but any
  // of the three names can be changed, so any two files can be diffed.

  // Myers greedy line diff of arrays a,b -> ops [type,text], ' '|'-'|'+'.  The
  // number of edit steps is bounded so a pathological input cannot hang the tab.
  function myersDiff(a, b) {
    var n = a.length, m = b.length;
    if (n === 0 && m === 0) return [];
    if (n === 0) return b.map(function (l) { return ["+", l]; });
    if (m === 0) return a.map(function (l) { return ["-", l]; });
    var max = n + m;
    var dmax = Math.max(2000, Math.floor(200000000 / max));
    if (dmax > max) dmax = max;
    var v = {}; v[1] = 0;
    var trace = [], found = false, dEnd = -1;
    for (var d = 0; d <= dmax && !found; d++) {
      trace.push(Object.assign({}, v));
      for (var k = -d; k <= d; k += 2) {
        var x;
        if (k === -d || (k !== d && (v[k - 1] || 0) < (v[k + 1] || 0))) x = (v[k + 1] || 0);
        else x = (v[k - 1] || 0) + 1;
        var y = x - k;
        while (x < n && y < m && a[x] === b[y]) { x++; y++; }
        v[k] = x;
        if (x >= n && y >= m) { found = true; dEnd = d; break; }
      }
    }
    if (!found) throw new Error("the two files differ too much to diff here");
    var ops = [], px = n, py = m;
    for (var dd = dEnd; dd > 0; dd--) {
      var vv = trace[dd];
      var kk = px - py;
      var prevK = (kk === -dd || (kk !== dd && (vv[kk - 1] || 0) < (vv[kk + 1] || 0))) ? kk + 1 : kk - 1;
      var prevX = vv[prevK] || 0, prevY = prevX - prevK;
      while (px > prevX && py > prevY) { ops.push([" ", a[px - 1]]); px--; py--; }
      if (px === prevX) ops.push(["+", b[py - 1]]); else ops.push(["-", a[px - 1]]);
      px = prevX; py = prevY;
    }
    while (px > 0) { ops.push([" ", a[px - 1]]); px--; py--; }
    ops.reverse();
    return ops;
  }

  // Boundary sliding: a run of inserted/deleted lines can be shifted across
  // identical neighbouring lines without changing the result (only which
  // identical line is labelled "changed" vs "context").  Among the equivalent
  // positions, pick the one whose surrounding lines are the best block
  // boundaries, so edits land on blank lines and metamath delimiters ($}, ${,
  // $(, ...) rather than tearing blocks apart.
  function boundaryStrength(text) {
    if (text === undefined) return 2;            // start/end of file
    var t = text.replace(/^\s+/, "");            // ignore indentation
    if (t === "") return 2;                       // blank line
    if (t.charAt(0) === "$") return 1;            // metamath delimiter / keyword
    return 0;
  }
  function runScore(ops, i, j) {
    return boundaryStrength(i > 0 ? ops[i - 1][1] : undefined) +
           boundaryStrength(j + 1 < ops.length ? ops[j + 1][1] : undefined);
  }
  function slideRuns(ops) {
    var i = 0;
    while (i < ops.length) {
      var t = ops[i][0];
      if (t === " ") { i++; continue; }
      var j = i;
      while (j + 1 < ops.length && ops[j + 1][0] === t) j++;   // maximal run of type t
      var s = i, e = j;
      while (s > 0 && ops[s - 1][0] === " " && ops[s - 1][1] === ops[e][1]) {
        ops[s - 1][0] = t; ops[e][0] = " "; s--; e--;          // slide up as far as possible
      }
      var bestStart = s, bestScore = runScore(ops, s, e);
      while (e + 1 < ops.length && ops[e + 1][0] === " " && ops[e + 1][1] === ops[s][1]) {
        ops[s][0] = " "; ops[e + 1][0] = t; s++; e++;          // scan down, scoring each spot
        var sc = runScore(ops, s, e);
        if (sc > bestScore) { bestScore = sc; bestStart = s; }
      }
      while (s > bestStart) {
        ops[s - 1][0] = t; ops[e][0] = " "; s--; e--;          // settle at the best spot
      }
      i = e + 1;
    }
  }

  // Trim the common prefix and suffix (so a localized edit stays cheap), and
  // shortcut a pure insertion or deletion, then Myers on what is left.
  function diffOps(a, b) {
    var n = a.length, m = b.length, s = 0;
    while (s < n && s < m && a[s] === b[s]) s++;
    var ea = n, eb = m;
    while (ea > s && eb > s && a[ea - 1] === b[eb - 1]) { ea--; eb--; }
    var midA = a.slice(s, ea), midB = b.slice(s, eb), mid;
    if (midA.length === 0) mid = midB.map(function (l) { return ["+", l]; });
    else if (midB.length === 0) mid = midA.map(function (l) { return ["-", l]; });
    else mid = myersDiff(midA, midB);
    var ops = [], i;
    for (i = 0; i < s; i++) ops.push([" ", a[i]]);
    for (i = 0; i < mid.length; i++) ops.push(mid[i]);
    for (i = ea; i < n; i++) ops.push([" ", a[i]]);
    slideRuns(ops);
    return ops;
  }

  // Format the ops as a unified diff, grouped into hunks with `context` lines.
  function unifiedDiff(aLines, bLines, oldName, newName, context) {
    context = context == null ? 3 : context;
    var ops = diffOps(aLines, bLines), items = [], ai = 0, bi = 0, i;
    for (i = 0; i < ops.length; i++) {
      items.push({ t: ops[i][0], text: ops[i][1], a: ai, b: bi });
      if (ops[i][0] === " ") { ai++; bi++; } else if (ops[i][0] === "-") ai++; else bi++;
    }
    var changed = [];
    for (i = 0; i < items.length; i++) if (items[i].t !== " ") changed.push(i);
    if (!changed.length) return { text: "", adds: 0, dels: 0 };
    var groups = [], gs = changed[0], ge = changed[0];
    for (i = 1; i < changed.length; i++) {
      if (changed[i] - ge <= context * 2 + 1) ge = changed[i];   // merge like GNU diff
      else { groups.push([gs, ge]); gs = changed[i]; ge = changed[i]; }
    }
    groups.push([gs, ge]);
    var out = ["--- " + oldName, "+++ " + newName], adds = 0, dels = 0;
    groups.forEach(function (g) {
      var lo = Math.max(0, g[0] - context), hi = Math.min(items.length - 1, g[1] + context);
      var aStart = items[lo].a, bStart = items[lo].b, aCount = 0, bCount = 0, body = [], j;
      for (j = lo; j <= hi; j++) {
        var it = items[j];
        if (it.t === " ") { aCount++; bCount++; body.push(" " + it.text); }
        else if (it.t === "-") { aCount++; dels++; body.push("-" + it.text); }
        else { bCount++; adds++; body.push("+" + it.text); }
      }
      out.push("@@ -" + (aCount ? aStart + 1 : aStart) + "," + aCount +
               " +" + (bCount ? bStart + 1 : bStart) + "," + bCount + " @@");
      for (j = 0; j < body.length; j++) out.push(body[j]);
    });
    return { text: out.join("\n") + "\n", adds: adds, dels: dels };
  }

  var diffDlg = document.getElementById("diff-dlg");
  var diffMsg = document.getElementById("diff-msg");
  var diffOld = document.getElementById("diff-old");
  var diffNew = document.getElementById("diff-new");
  var diffOut = document.getElementById("diff-out");

  function openDiff(name) {
    var base = (name.slice(-5) === ".orig") ? name.slice(0, -5) : name;
    diffOld.value = base + ".orig";
    diffNew.value = base;
    diffOut.value = base + ".diff";
    diffMsg.textContent =
      "Write a unified diff of two files. The defaults compare a file with its .orig baseline.";
    if (typeof diffDlg.showModal === "function") diffDlg.showModal();
  }
  exBtn.diff.onclick = function () { if (exploreSel) openDiff(exploreSel); };
  document.getElementById("diff-close").onclick = function () { diffDlg.close(); };
  document.getElementById("diff-swap").onclick = function () {
    var t = diffOld.value; diffOld.value = diffNew.value; diffNew.value = t;
  };
  document.getElementById("diff-run").onclick = function () {
    var oldName = diffOld.value.trim(), newName = diffNew.value.trim(), outName = diffOut.value.trim();
    if (!oldName || !newName || !outName) { diffMsg.textContent = "Fill in all three names."; return; }
    if (outName.indexOf("/") !== -1) { diffMsg.textContent = "Output name cannot contain '/'."; return; }
    if (!metamathIdle()) { diffMsg.textContent = "Wait until metamath is idle."; return; }
    if (!fileExists(oldName)) { diffMsg.textContent = oldName + " does not exist."; return; }
    if (!fileExists(newName)) { diffMsg.textContent = newName + " does not exist."; return; }
    ensureLoaded([oldName, newName]).then(function () {
      var oldText, newText;
      try {
        oldText = new TextDecoder("utf-8").decode(Mod.FS.readFile("/work/" + oldName));
        newText = new TextDecoder("utf-8").decode(Mod.FS.readFile("/work/" + newName));
      } catch (e) { diffMsg.textContent = "Could not read: " + e.message; return; }
      var result;
      try { result = unifiedDiff(oldText.split("\n"), newText.split("\n"), oldName, newName); }
      catch (e) { diffMsg.textContent = e.message; return; }
      if (!result.text) {
        diffMsg.textContent = oldName + " and " + newName + " are identical; nothing written.";
        return;
      }
      if (fileExists(outName) && !window.confirm(outName + " exists.  Overwrite it?")) return;
      try { Mod.FS.writeFile("/work/" + outName, new TextEncoder().encode(result.text)); }
      catch (e) { diffMsg.textContent = "Could not write " + outName + ": " + e.message; return; }
      flushPersist();
      echo("\n[diff " + oldName + " vs " + newName + ": +" + result.adds + " -" + result.dels +
           " lines, written to " + outName + "]\n");
      diffDlg.close();
      renderExplore();
    });
  };

  // ---- patch ----------------------------------------------------------
  // Apply a unified diff to a file.  Each hunk's context and deleted lines must
  // match the target exactly, or the patch is refused (never applied wrong).
  // This reads the same format the Diff action writes.
  function parseHunks(diffText) {
    var lines = diffText.split("\n");
    if (lines.length && lines[lines.length - 1] === "") lines.pop();
    var hunks = [], i = 0;
    while (i < lines.length && lines[i].slice(0, 2) !== "@@") i++;
    while (i < lines.length) {
      var mo = /^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@/.exec(lines[i]);
      if (!mo) throw new Error("malformed hunk header: " + lines[i]);
      var h = { oldStart: parseInt(mo[1], 10),
                oldCount: mo[2] === undefined ? 1 : parseInt(mo[2], 10), body: [] };
      i++;
      while (i < lines.length && lines[i].slice(0, 2) !== "@@") {
        var ln = lines[i], tag = ln.charAt(0);
        if (tag === " " || tag === "+" || tag === "-") h.body.push([tag, ln.slice(1)]);
        else if (ln === "") h.body.push([" ", ""]);   // whitespace-stripped empty context line
        i++;
      }
      hunks.push(h);
    }
    return hunks;
  }
  function applyPatch(targetLines, diffText) {
    var hunks = parseHunks(diffText), out = [], pos = 0;
    for (var hi = 0; hi < hunks.length; hi++) {
      var h = hunks[hi];
      var start = h.oldCount === 0 ? h.oldStart : h.oldStart - 1;
      if (start < pos) throw new Error("hunk #" + (hi + 1) + " is out of order");
      while (pos < start) { out.push(targetLines[pos]); pos++; }
      for (var bi = 0; bi < h.body.length; bi++) {
        var tag = h.body[bi][0], text = h.body[bi][1];
        if (tag === "+") { out.push(text); continue; }
        if (pos >= targetLines.length || targetLines[pos] !== text)
          throw new Error("hunk #" + (hi + 1) + " does not match the target at line " + (pos + 1));
        if (tag === " ") out.push(targetLines[pos]);
        pos++;
      }
    }
    while (pos < targetLines.length) { out.push(targetLines[pos]); pos++; }
    return out;
  }

  var patchDlg = document.getElementById("patch-dlg");
  var patchMsg = document.getElementById("patch-msg");
  var patchFile = document.getElementById("patch-file");
  var patchTarget = document.getElementById("patch-target");
  var patchOut = document.getElementById("patch-out");

  function openPatch(name) {
    var patchName = (name.slice(-5) === ".diff") ? name : name + ".diff";
    ensureLoaded([patchName]).then(function () {
      var base = (patchName.slice(-5) === ".diff") ? patchName.slice(0, -5) : patchName;
      var target = base + ".orig", out = base;
      if (fileExists(patchName)) {
        try {
          var lines = new TextDecoder("utf-8").decode(Mod.FS.readFile("/work/" + patchName)).split("\n");
          if (lines[0] && lines[0].slice(0, 4) === "--- " && lines[1] && lines[1].slice(0, 4) === "+++ ") {
            target = lines[0].slice(4).trim();   // the diff's own old-file name
            out = lines[1].slice(4).trim();      // the diff's own new-file name
          }
        } catch (e) { /* keep the fallback defaults */ }
      }
      patchFile.value = patchName;
      patchTarget.value = target;
      patchOut.value = out;
      patchMsg.textContent =
        "Apply a unified diff to a file. Target and output default to the diff's own --- and +++ names.";
      if (typeof patchDlg.showModal === "function") patchDlg.showModal();
    });
  }
  exBtn.patch.onclick = function () { if (exploreSel) openPatch(exploreSel); };
  document.getElementById("patch-close").onclick = function () { patchDlg.close(); };
  document.getElementById("patch-run").onclick = function () {
    var pName = patchFile.value.trim(), tName = patchTarget.value.trim(), oName = patchOut.value.trim();
    if (!pName || !tName || !oName) { patchMsg.textContent = "Fill in all three names."; return; }
    if (oName.indexOf("/") !== -1) { patchMsg.textContent = "Output name cannot contain '/'."; return; }
    if (!metamathIdle()) { patchMsg.textContent = "Wait until metamath is idle."; return; }
    if (!fileExists(pName)) { patchMsg.textContent = pName + " does not exist."; return; }
    if (!fileExists(tName)) { patchMsg.textContent = tName + " does not exist."; return; }
    ensureLoaded([pName, tName]).then(function () {
      var diffText, targetText;
      try {
        diffText = new TextDecoder("utf-8").decode(Mod.FS.readFile("/work/" + pName));
        targetText = new TextDecoder("utf-8").decode(Mod.FS.readFile("/work/" + tName));
      } catch (e) { patchMsg.textContent = "Could not read: " + e.message; return; }
      var result;
      try { result = applyPatch(targetText.split("\n"), diffText).join("\n"); }
      catch (e) { patchMsg.textContent = e.message; return; }   // e.g. "hunk #2 does not match..."
      if (fileExists(oName) && !window.confirm(oName + " exists.  Overwrite it?")) return;
      try { Mod.FS.writeFile("/work/" + oName, new TextEncoder().encode(result)); }
      catch (e) { patchMsg.textContent = "Could not write " + oName + ": " + e.message; return; }
      flushPersist();
      echo("\n[patched " + tName + " with " + pName + " -> " + oName + "]\n");
      patchDlg.close();
      renderExplore();
    });
  };

  // ---- persistence (our own IndexedDB store) --------------------------
  // /work is a normal in-memory filesystem; we persist it to IndexedDB
  // ourselves so its files survive a reload, tab close, or crash.  We keep our
  // own object store rather than Emscripten's IDBFS so we can gzip each file on
  // the way out and gunzip it on the way in (see encodeFor / gunzip), which
  // shrinks storage and lowers the chance the browser evicts it.  Restore once
  // at boot, then flush at each idle when metamath returns to its prompt, and on
  // unload.  Works in every browser that has IndexedDB (effectively all of them).
  var persistAvailable = false;
  try { persistAvailable = (typeof indexedDB !== "undefined" && indexedDB !== null); }
  catch (e) { persistAvailable = false; }  // some locked-down modes throw here

  // One object store, keyed by file name.  Each record is
  //   { name, data: Uint8Array, raw: bool, size: uncompressed length, mtime, mode }.
  // data is gzip-compressed unless raw is true (a small or incompressible file,
  // or a browser without CompressionStream), in which case it is the verbatim
  // bytes.  size is always the uncompressed length.
  var DB_NAME = "metamath-work", DB_STORE = "files", DB_VERSION = 2, dbPromise = null;
  function openWorkDB() {
    if (dbPromise) return dbPromise;
    dbPromise = new Promise(function (resolve, reject) {
      var req = indexedDB.open(DB_NAME, DB_VERSION);
      req.onupgradeneeded = function () {
        // The record shape changed when compression was added; nothing is
        // shipped, so start the store fresh rather than migrate old records.
        var db = req.result;
        if (db.objectStoreNames.contains(DB_STORE)) db.deleteObjectStore(DB_STORE);
        db.createObjectStore(DB_STORE, { keyPath: "name" });
      };
      req.onsuccess = function () { resolve(req.result); };
      req.onerror = function () { reject(req.error); };
    });
    return dbPromise;
  }
  function txDone(tx) {
    return new Promise(function (resolve, reject) {
      tx.oncomplete = function () { resolve(); };
      tx.onerror = function () { reject(tx.error); };
      tx.onabort = function () { reject(tx.error); };
    });
  }

  // Files are stored gzip-compressed, using the browser-native Compression
  // Streams API (real gzip, no library).  Below COMPRESS_MIN, or when gzip would
  // not actually shrink the file, or when the API is absent, they are stored
  // verbatim (raw: true).  Compression happens only here, at the persistence
  // boundary; /work itself always holds the uncompressed bytes.
  var COMPRESS_MIN = 1024;
  var compressAvailable = (typeof CompressionStream !== "undefined" &&
                           typeof DecompressionStream !== "undefined");
  function streamBytes(bytes, transform) {  // Uint8Array -> stream -> Uint8Array
    var out = new Response(bytes).body.pipeThrough(transform);
    return new Response(out).arrayBuffer().then(function (buf) {
      return new Uint8Array(buf);
    });
  }
  function gzip(bytes)   { return streamBytes(bytes, new CompressionStream("gzip")); }
  function gunzip(bytes) { return streamBytes(bytes, new DecompressionStream("gzip")); }
  // Encode a file for storage; resolves to { data, raw, size }, keeping gzip
  // only when it is actually smaller than the original.
  function encodeFor(bytes) {
    if (!compressAvailable || bytes.length < COMPRESS_MIN) {
      return Promise.resolve({ data: bytes, raw: true, size: bytes.length });
    }
    return gzip(bytes).then(function (gz) {
      return gz.length < bytes.length
        ? { data: gz, raw: false, size: bytes.length }
        : { data: bytes, raw: true, size: bytes.length };
    });
  }

  // ---- load / unload a file's contents --------------------------------
  // `store` holds every saved file's compressed bytes + metadata in RAM
  // (mirroring IndexedDB).  A file's uncompressed bytes live in its /work node
  // only while "loaded"; at idle we unload them, so idle RAM is just the
  // compressed sizes.  An unloaded node keeps node.usedBytes = size -- so stat,
  // readdir, the persistence snapshot and the Explorer listing all still see the
  // real size -- but has no contents.  INVARIANT: never read a node's bytes
  // while unloaded; load it first.  metamath does (fopen -> mm_materialize ->
  // loadFile); the page's own reads call ensureLoaded first.
  var store = new Map();   // name -> { comp, raw, size, mtime, mode }

  function workNode(name) {
    try { return Mod.FS.lookupPath("/work/" + name).node; } catch (e) { return null; }
  }

  // Create the /work node for a saved file with its real size and mtime but no
  // contents; node.mmLoaded stays false until something reads it.
  function makeUnloadedNode(name) {
    var rec = store.get(name), path = "/work/" + name;
    Mod.FS.writeFile(path, new Uint8Array(0));
    var node = workNode(name);
    if (rec.mode) node.mode = rec.mode;
    node.usedBytes = rec.size;      // report the true, uncompressed size
    node.mmLoaded = false;          // ...but the bytes are not in RAM yet
    if (rec.mtime) Mod.FS.utime(path, rec.mtime, rec.mtime);
  }

  // Bring a file's bytes into its node (gunzip from the store).  A no-op if the
  // file is already loaded, or is not one of ours.
  function loadFile(name) {
    var node = workNode(name);
    if (!node || node.mmLoaded !== false) return Promise.resolve();  // already loaded
    var rec = store.get(name);
    if (!rec) return Promise.resolve();
    var bytesP = rec.raw ? Promise.resolve(rec.comp) : gunzip(rec.comp);
    return bytesP.then(function (bytes) {
      var path = "/work/" + name;
      Mod.FS.writeFile(path, bytes);
      Mod.FS.utime(path, rec.mtime, rec.mtime);   // a read must not look like a change
      workNode(name).mmLoaded = true;
    });
  }

  // Free a loaded file's bytes but keep its reported size.  Only at idle, and
  // only for files already saved in the store.
  function unloadFile(name) {
    var node = workNode(name);
    if (!node || node.mmLoaded === false) return;   // absent or already unloaded
    var size = node.usedBytes;
    node.contents = new Uint8Array(0);
    node.usedBytes = size;
    node.mmLoaded = false;
  }
  function unloadAll() {
    var names;
    try { names = Mod.FS.readdir("/work"); } catch (e) { return; }
    names.forEach(function (n) {
      if (n !== "." && n !== ".." && store.has(n)) unloadFile(n);
    });
  }

  // Ensure the named files' bytes are in /work before the page reads them.
  function ensureLoaded(names) {
    return Promise.all(names.map(function (n) { return loadFile(n); }));
  }

  // Map a C fopen path to a store key.  metamath's cwd is /work, so a path is a
  // bare name ("set.mm") or "/work/set.mm"; anything else is not one of ours.
  function nameFromWorkPath(path) {
    if (path.indexOf("/") === -1) return path;
    if (path.lastIndexOf("/work/", 0) === 0) return path.slice(6);
    return null;
  }
  // Called (via the mm_materialize import / __wrap_fopen) before metamath reads
  // a file, so its bytes are present for the synchronous read that follows.
  function mmMaterialize(path) {
    var name = nameFromWorkPath(path);
    return name ? loadFile(name) : Promise.resolve();
  }

  // Most metamath commands (verify, search, show, ...) never touch /work, so we
  // must not write to IndexedDB after every command.  flushPersist() first
  // takes a cheap in-memory snapshot of /work (names + mtime + size) and only
  // writes the files whose snapshot changed (and deletes those that vanished).
  // This catches both metamath's own writes and the page's writes, yet touches
  // IndexedDB only when a file actually changed.
  var lastSnapshot = null;   // snapshot of what is currently persisted
  var syncing = false, syncAgain = false;

  function workSnapshot() {
    var snap = {};
    if (!Mod) return snap;
    var names;
    try { names = Mod.FS.readdir("/work"); } catch (e) { return snap; }
    for (var i = 0; i < names.length; i++) {
      var n = names[i];
      if (n === "." || n === "..") continue;
      try {
        var st = Mod.FS.stat("/work/" + n);
        snap[n] = mtimeMs(st) + ":" + st.size;   // changes if content changes
      } catch (e) { /* skip an unreadable entry */ }
    }
    return snap;
  }
  function sameSnapshot(a, b) {
    if (!a || !b) return false;
    var ka = Object.keys(a), kb = Object.keys(b);
    if (ka.length !== kb.length) return false;
    for (var i = 0; i < ka.length; i++) { if (a[ka[i]] !== b[ka[i]]) return false; }
    return true;
  }

  // Write the files that changed since oldSnap; delete those that vanished.
  // Compression is async and an IndexedDB transaction cannot span an await, so
  // we read + encode every changed file first (reads happen synchronously here,
  // capturing a consistent snapshot), then do all puts/deletes in one
  // transaction so a crash mid-flush leaves a consistent store.
  function persistChanges(oldSnap, newSnap) {
    var jobs = [], dels = [], name;
    for (name in newSnap) {
      if (!oldSnap || oldSnap[name] !== newSnap[name]) {
        jobs.push((function (nm) {
          var path = "/work/" + nm, st = Mod.FS.stat(path), bytes = Mod.FS.readFile(path);
          return encodeFor(bytes).then(function (enc) {
            return { name: nm, data: enc.data, raw: enc.raw,
                     size: enc.size, mtime: mtimeMs(st), mode: st.mode };
          });
        })(name));
      }
    }
    if (oldSnap) { for (name in oldSnap) { if (!(name in newSnap)) dels.push(name); } }
    return Promise.all(jobs).then(function (recs) {
      return openWorkDB().then(function (db) {
        var tx = db.transaction(DB_STORE, "readwrite");
        var os = tx.objectStore(DB_STORE);
        recs.forEach(function (r) {
          os.put(r);
          store.set(r.name, { comp: r.data, raw: r.raw, size: r.size,
                              mtime: r.mtime, mode: r.mode });
        });
        dels.forEach(function (n) { os.delete(n); store.delete(n); });
        return txDone(tx);
      });
    });
  }

  function flushPersist() {
    if (!persistAvailable || !Mod) return;
    var snap = workSnapshot();
    if (sameSnapshot(snap, lastSnapshot)) {
      unloadAll();   // nothing to persist, but free the bytes of any file just read
      return;
    }
    if (syncing) { syncAgain = true; return; }       // coalesce overlapping flushes
    syncing = true;
    persistChanges(lastSnapshot, snap).then(function () {
      lastSnapshot = snap;                           // remember what we persisted
    }, function (err) {
      if (window.console) console.warn("metamath: persist failed", err);
    }).then(function () {
      unloadAll();                                   // free bytes once safely saved
      syncing = false;
      if (syncAgain) { syncAgain = false; flushPersist(); }
    });
  }

  // Load the saved index into `store` and create one unloaded /work node per
  // file (real size + mtime, no contents).  Contents are paged in on demand, so
  // boot uses no uncompressed RAM and does not gunzip anything.
  function restoreWork() {
    return openWorkDB().then(function (db) {
      return new Promise(function (resolve, reject) {
        var tx = db.transaction(DB_STORE, "readonly");
        var rq = tx.objectStore(DB_STORE).getAll();
        rq.onsuccess = function () { resolve(rq.result); };
        rq.onerror = function () { reject(rq.error); };
      });
    }).then(function (recs) {
      recs.forEach(function (rec) {
        store.set(rec.name, { comp: rec.data, raw: rec.raw, size: rec.size,
                              mtime: rec.mtime, mode: rec.mode });
        try { makeUnloadedNode(rec.name); }
        catch (e) { store.delete(rec.name); /* skip a bad record */ }
      });
    });
  }

  // Best-effort final flush; async writes may not finish during unload, which
  // is why the idle flush is the real guarantee.
  window.addEventListener("beforeunload", function () { flushPersist(); });

  // ---- start ----------------------------------------------------------
  createMetamath({
    mmReadLine: mmReadLine,
    mmMaterialize: mmMaterialize,   // page a file in before metamath reads it
    preRun: [function (Module) {
      // Work inside /work so added files and outputs are easy to enumerate.
      // /work is a plain in-memory filesystem; we persist it to IndexedDB
      // ourselves (see restoreWork / flushPersist), not via IDBFS.
      Module.FS.mkdir("/work");
      Module.FS.chdir("/work");
      // Character-level stdout/stderr so prompts appear immediately.
      Module.FS.init(null, outByte, outByte);
    }],
    onExit: function () {
      echo("\n[metamath exited. Reload the page to start again.]\n");
      cmdEl.disabled = true;
      setState("exited");
    },
  }).then(function (Module) {
    Mod = Module;
    // Restore any previously persisted files from IndexedDB before starting.
    var restored = persistAvailable
      ? restoreWork().catch(function (err) {
          if (window.console) console.warn("metamath: restore failed", err);
        })
      : Promise.resolve();
    return restored.then(function () {
      // The just-restored files are already what is persisted, so record them
      // as the baseline; the first idle then flushes nothing.
      lastSnapshot = workSnapshot();
      // Ask the browser to keep our storage (reduces eviction; may prompt).
      if (persistAvailable && navigator.storage && navigator.storage.persist) {
        navigator.storage.persist();
      }
      cmdEl.disabled = false;
      cmdEl.focus();
      // The state becomes "ready" on its own once metamath prints its first
      // prompt and asks for input (see mmReadLine).
      echo("[Type commands in the highlighted box at the bottom of the page." +
           "  Try:  help\n" +
           "\n Files live in three places:\n" +
           "   the Metamath website   - use \"Download set.mm\" to copy a database here\n" +
           "   your own computer      - use \"Add from computer\" and \"Save to computer\"\n" +
           "   the virtual filesystem - this page's own storage, kept across reloads\n" +
           "\n A file in the virtual filesystem is not read into metamath until you" +
           " run, for example:  read \"set.mm\"]\n\n");
      if (!persistAvailable) {
        echo("[Note: this browser is not storing files persistently" +
             " (private mode?).  Use \"Save to computer\" to keep your work.]\n\n");
      }
      Module.callMain([]);
    });
  }).catch(function (err) {
    setState("error");
    echo("Failed to start metamath: " + err + "\n");
  });
})();
