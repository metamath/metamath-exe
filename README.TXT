This package contains the metamath program and several Metamath databases.


Copyright
---------

The metamath program is copyright under the terms of the GNU GPL license
version 2 or later.  See the file LICENSE.TXT in this directory.

Individual databases (*.mm files) are either public domain or under the GNU
GPL, as indicated by comments in their headers.

See http://us.metamath.org/copyright.html for further license and copyright
information that applies to the content of this package.


Instructions
------------

Prebuilt portable executable (easiest, no compiler needed)
----------------------------------------------------------

Each release provides a single "portable executable" that runs on Windows,
macOS, Linux, and the BSDs (both x86-64 and Arm64) without installation.  It is
built with the Cosmopolitan toolchain as an "Actually Portable Executable".

The one file contains native machine code for both x86-64 and Arm64; it is not
an emulator.  The Cosmopolitan toolchain compiles the program twice (once per
architecture) and bundles both into the single file, and the loader runs the
slice that matches the host CPU.  So Arm64 machines (for example Apple Silicon
and Arm Linux) run native Arm64 code at full speed, with no emulation penalty.
The trade-off is that the file is roughly twice the size of a single-CPU build.

This one URL always downloads the newest release, and works the same in a web
browser, in curl, in wget, and in scripts:

  https://github.com/metamath/metamath-exe/releases/latest/download/metamath.exe

Each release also contains the identical program under a name that includes the
version, such as "metamath-v0.199.exe", for people who prefer to keep the
version in the file name.  Both are on the Releases page:

  https://github.com/metamath/metamath-exe/releases

The program always reports its own version when it starts, so a copy named just
"metamath.exe" is never ambiguous.

Then run it and type "read set.mm" (or point it at a database, see below):

  Windows:  double-click metamath-vX.Y.Z.exe, or from a command prompt run
            "metamath-vX.Y.Z.exe".  Because the file is not code-signed, you
            may see a Microsoft Defender SmartScreen "Windows protected your
            PC" prompt; click "More info" then "Run anyway".  (Downloading the
            version-numbered filename, rather than a plain "metamath.exe",
            reduces the chance of a false-positive antivirus warning.)

  macOS:    the file is not notarized, so Gatekeeper quarantines downloads.
            Clear the quarantine flag once, then run it from Terminal:

              xattr -d com.apple.quarantine metamath-vX.Y.Z.exe
              ./metamath-vX.Y.Z.exe set.mm

  Linux/BSD: make it executable, then run it:

              chmod +x metamath-vX.Y.Z.exe
              ./metamath-vX.Y.Z.exe set.mm

The same file runs on every platform regardless of the ".exe" name.  Database
(.mm) files are not bundled with this executable; download the current ones,
e.g. set.mm and iset.mm, from https://us.metamath.org/metamath/set.mm and
https://us.metamath.org/metamath/iset.mm, or from the metamath/set.mm
repository, and place them next to the executable (or give a full path).

To compile it yourself instead, see the source-build instructions below.


Running metamath-exe in a web browser
-------------------------------------

Metamath-exe can also be run inside a web browser, with nothing installed at
all:

  https://metamath.github.io/metamath-exe/

Every release is also kept permanently at its own address, so that work done
with a particular version stays reproducible:

  https://metamath.github.io/metamath-exe/v0.199/

The page provides a terminal (with up and down arrow command history), buttons
to download set.mm or iset.mm into the virtual filesystem, a way to
add your own .mm file from your computer, a listing of that filesystem, and a
way to save files the program produced back to your computer.  A file added
this way stays in your browser; it is not sent anywhere.

Note that the databases live only in the virtual filesystem, which is this
page's own storage.  Downloading set.mm does not read it into metamath; you
still have to type

  read "set.mm"

To build this browser version yourself you need the Emscripten SDK.  From the
top folder:

  ./get-emsdk.sh       # installs a pinned Emscripten SDK into ./emsdk
  ./build-wasm.sh -s   # builds into ./build-wasm and serves it

then open http://localhost:8765/ in a browser.  Omit the -s option to build
without starting a web server.  Neither the SDK nor the build output is checked
in.

The browser version behaves slightly differently from the command line version:
operating system commands (a command line in quotes) are not available, since a
browser has no shell, and output scrolls continuously because the browser
supplies its own scrollback.


Building from source
--------------------

For Windows, click on "metamath.exe" and type "read set.mm".

For Unix/Linux/Cygwin/MacOSX using the gcc compiler, compile with the command
"cd src && gcc m*.c -o metamath", then type "./metamath set.mm" to run.

As an alternative, if you have autoconf, automake, and a C compiler, you can
compile with the command "autoreconf -i && ./configure && make".  This
"autoconf" approach automatically finds your compiler and its options, and
configure takes the usual options (e.g., "--prefix=/usr").  The resulting
executable will typically be faster because it will check for and enable
available optimizations; tests found that the "improve" command ran 28% faster
on gcc when using an autoconf-generated "configure".  You can again type
"./metamath set.mm" to run.  After "make" you may install it elsewhere using
"sudo make install" (note that this installs ".mm" files in the pkgdata
directory, by default "/usr/local/share/metamath/").  If you install it this
way, you can then run metamath as "metamath /usr/share/metamath/set.mm", copy
set.mm locally (cp /usr/share/metamath/set.mm . ; metamath set.mm), or run
metamath and type:  read "/usr/share/metamath/set.mm" (note that inside
metamath, filenames containing "/" must be quoted).

Known issues with the autoconf approach:  On Intel type processors x86 the
configure script might want you to support 32-bit code, even if your system
is natively 64-bit.  This is known as cross-compiling and on Debian you need
the package gcc-multilib installed.  For other Linux OS a similar extension
might be in order.

Building the portable executable yourself:  the single "Actually Portable
Executable" described near the top of this file is built with the Cosmopolitan
toolchain (cosmocc).  You do not need it installed on your system; a helper
script downloads a pinned copy into ./cosmocc.  From the top folder:

  ./get-cosmocc.sh                              # installs cosmocc into ./cosmocc
  PATH="$PWD/cosmocc/bin:$PATH" CC=cosmocc ./build.sh

The single build produces a "fat" binary that contains native x86-64 and Arm64
code and runs on Windows, macOS, Linux, and the BSDs; there is no separate ARM
build step and no emulation.  Neither the toolchain nor the build output is
checked in.  See the comments in get-cosmocc.sh for details.


Optional enhancements
---------------------

For optimized performance under gcc, you can compile as follows:

  gcc m*.c -o metamath -O3 -funroll-loops -finline-functions \
     -fomit-frame-pointer -Wall -pedantic

If your compiler supports it, you can also add the option -DINLINE=inline to
achieve the 28% performance increase described above.

On Linux/MacOSX/Unix, the Metamath program will be more pleasant to use if you
run it inside of https://github.com/hanslub42/rlwrap (checked 18-Sep-2024)
which provides up-arrow command history and other command-line editing
features.  After you install rlwrap per its instructions (see below), invoke
the Metamath program with "rlwrap ./metamath set.mm".

In some Linux distributions (such as Debian Woody), if the Backspace key does
not delete characters typed after the "MM>" prompt, try adding this line to
your ~/.bash_profile file:

  stty echoe echok echoctl echoke

Using rlwrap as described below will also solve this problem.


Additional MacOSX information
-----------------------------

On MacOSX, select the Terminal application from Applications/Utilities to get
to the command line.  On recent versions of MacOSX, you need to install gcc
separately.  Typing "whereis gcc" will return "/usr/bin/gcc" if it is
installed.  The XCode package is typically used to install it, but it can also
be installed without XCode; see

  https://github.com/kennethreitz/osx-gcc-installer/ (checked 15-Feb-2014)


Optional rlwrap user interface enhancement
------------------------------------------

On Linux/MacOSX/Unix, the Metamath program will be more pleasant to use if you
run it inside of rlwrap:

  https://github.com/hanslub42/rlwrap (checked 18-Sep-2024)

which provides up-arrow command history and other command-line editing
features.  After you install rlwrap per its instructions, invoke the Metamath
program with "rlwrap ./metamath set.mm".

On Debian flavoured Linux you can use:

sudo apt-get install rlwrap

Other Linux OS might require you to build this program from sources.  Be
prepared to have a curses and readline library ready then.

The Windows version of the Metamath program was compiled with lcc, which has
similar features built-in.)


Windows Compilation
-------------------

To reproduce the included metamath.exe for Windows, use lcc-win32 version 3.8,
with the following command:

  lc -O m*.c -o metamath.exe


Further suggestions
-------------------

Once in the program, use the "help" command to guide you.  For more
information, see the Metamath book available at http://metamath.org .


To uninstall
------------

To uninstall, just delete the "metamath" directory - nothing else (Registry,
etc.) is touched in your system.

If you used autoconf's "make install" to install it in system locations, you
can use "make uninstall" to remove it.


List of databases
-----------------

The database files included are:

  set.mm - logic and set theory database (see Ch. 3 of the Metamath book).
      The Metamath Proof Explorer pages were generated from this database.
  nf.mm - logic and set theory database for Quine's New Foundations set
      theory.
  hol.mm - higher order logic (simple type theory) database.
  iset.mm - intuitionistic logic database.
  ql.mm - quantum logic database.  The Quantum Logic Explorer pages were
      generated from this database.
  demo0.mm - demo of simple formal system (see Ch. 2 of the Metamath book)
  miu.mm - Hofstadter's MIU-system (see Appendix D of the Metamath book)
  big-unifier.mm - A unification stress test (see comments in the file).
  peano.mm - A nicely commented presentation of Peano arithmetic, written
      by Robert Solovay (unlike the ones above, this database is NOT public
      domain but is copyright under the terms of the GNU GPL license).


Source code documentation
-------------------------

Many comments in the source code are written in

https://doxygen.nl/index.html

Doxygen (Qt variant) style.  If you have Doxygen installed on your computer,
you may generate HTML documentation from them with its root page placed in
build/html/index.html by running build.sh with the -d option.
