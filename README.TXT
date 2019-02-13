This package contains the metamath program and several Metamath
databases.


Copyright
---------

The metamath program is copyright under the terms of the GNU GPL license
version 2 or later.  See the file LICENSE.TXT in this directory.

Individual databases (*.mm files) are either public domain or under
the GNU GPL, as indicated by comments in their headers.

See http://us.metamath.org/copyright.html for further license and
copyright information that applies to the content of this package.


Instructions
------------

For Windows, click on "metamath.exe" and type "read set.mm".

For Unix/Linux/Cygwin/MacOSX using the gcc compiler, compile with the
command "gcc m*.c -o metamath", then type "./metamath set.mm" to run.

As an alternative, if you have autoconf, automake, and a C compiler, you
can compile with the command "autoreconf -i && ./configure && make".
This "autoconf" approach automatically finds your compiler and its
options, and configure takes the usual options (e.g., "--prefix=/usr").
The resulting executable will typically be faster because it will check for
and enable available optimizations; tests found that the "improve"
command ran 28% faster on gcc when using an autoconf-generated "configure".
You can again type "./metamath set.mm" to run.  After "make" you may
install it elsewhere using "sudo make install" (note that this installs
".mm" files in the pkgdata directory, by default
"/usr/local/share/metamath/").  If you install it this way, you can then
run metamath as "metamath /usr/share/metamath/set.mm", copy set.mm
locally (cp /usr/share/metamath/set.mm . ; metamath set.mm), or run
metamath and type:  read "/usr/share/metamath/set.mm" (note that inside
metamath, filenames containing "/" must be quoted).


Optional enhancements
---------------------

For optimized performance under gcc, you can compile as follows:

  gcc m*.c -o metamath -O3 -funroll-loops -finline-functions \
     -fomit-frame-pointer -Wall -pedantic

If your compiler supports it, you can also add the option -DINLINE=inline
to achieve the 28% performance increase described above.


On Linux/MacOSX/Unix, the Metamath program will be more pleasant to use
if you run it inside of rlwrap http://utopia.knoware.nl/~hlub/rlwrap/
(checked 3-Jun-2015) which provides up-arrow command history and other
command-line editing features.  After you install rlwrap per its
instructions, invoke the Metamath program with "rlwrap ./metamath
set.mm".


In some Linux distributions (such as Debian Woody), if the Backspace
key doesn't delete characters typed after the "MM>" prompt, try adding
this line to your ~/.bash_profile file:

  stty echoe echok echoctl echoke

Using rlwrap as described above will also solve this problem.


Additional MacOSX information
-----------------------------

On MacOSX, select the Terminal application from Applications/Utilities
to get to the command line.  On recent versions of MacOSX, you need to
install gcc separately.  Typing "whereis gcc" will return "/usr/bin/gcc"
if it is installed.  The XCode package is typically used to install it,
but it can also be installed without XCode; see

  https://github.com/kennethreitz/osx-gcc-installer/ (checked 15-Feb-2014)


Optional rlwrap user interface enhancement
------------------------------------------

On Linux/MacOSX/Unix, the Metamath program will be more pleasant to use
if you run it inside of rlwrap:

  http://utopia.knoware.nl/~hlub/uck/rlwrap/ (checked 15-Feb-2014)

which provides up-arrow command history and other command-line editing
features.  After you install rlwrap per its instructions, invoke the
Metamath program with "rlwrap ./metamath set.mm".

(The Windows version of the Metamath program was compiled with lcc,
which has similar features built-in.)


Windows Compilation
-------------------

To reproduce the included metamath.exe for Windows, use lcc-win32
version 3.8, with the following command:

  lc -O m*.c -o metamath.exe


Further suggestions
-------------------

Once in the program, use the "help" command to guide you.  For more
information, see the Metamath book available at http://metamath.org .


To uninstall
------------

To uninstall, just delete the "metamath" directory - nothing else
(Registry, etc.) is touched in your system.

If you used autoconf's "make install" to install it in system locations,
you can use "make uninstall" to remove it.


List of databases
-----------------

The data base files included are:

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
      domain but is copyright under the terms of the GNU GPL license)
