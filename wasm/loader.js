// SPDX-License-Identifier: GPL-2.0-or-later OR MIT

// Bootstrap for the Metamath web page.  Split out of metamath.html (along with
// the CSS and the app script) so the page carries no inline JavaScript or CSS
// and can be served under a strict Content-Security-Policy (script-src 'self').
//
// Loads the WebAssembly build the browser supports, then the app script.  We
// feature-detect the newer JSPI API that Emscripten emits (WebAssembly.Suspending
// and .promising), not the legacy Suspender, so a browser with only the old API
// correctly falls back to the ASYNCIFY build.  Both builds export
// createMetamath(); metamath.js is loaded only after the chosen module has, so
// createMetamath is defined by the time the app runs.
(function () {
  "use strict";

  function loadScript(src, onload) {
    var s = document.createElement("script");
    s.src = src;
    if (onload) s.onload = onload;
    s.onerror = function () {
      var t = document.getElementById("term");
      if (t) t.textContent += "\nFailed to load " + src + "\n";
    };
    document.body.appendChild(s);
  }

  var jspi = (typeof WebAssembly !== "undefined") &&
             ("Suspending" in WebAssembly) && ("promising" in WebAssembly);
  var moduleSrc = jspi ? "./metamath-browser-jspi.js" : "./metamath-browser.js";
  loadScript(moduleSrc, function () { loadScript("./metamath.js"); });
})();
