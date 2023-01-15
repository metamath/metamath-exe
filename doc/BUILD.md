# Building the Metamath executable from sources

## Introductory notes

In this file we describe how to build the executable metamath on a __Unix/Linux__
system from scratch using only the sources and a few auxiliary files, all
distributed through the Github Metamath git repository.

### Git folder of source files

[Git](https://en.wikipedia.org/wiki/Git) is widespread version control system.
It bundles files and subdirectories into a single folder called __repository__.
Unlike a simple folder a repository tracks changes to its contents, so you can
checkout older versions of a file, or different versions if multiple persons
work on files simultaneously.  It is out of the scope of this particular
document to detail on how this system works.  Instead we assume you have a
fresh set of metamath source files ready in a single folder.  It is of no
importance here how you got it.  The name of the folder is arbitrary, but for
the sake of simplicity we call it throughout this file __metamath-exe__.

## Unix/Linux deployment

Although we restrict the build process on Unix/Linux, there exist numerous
variants of this operating system (__OS__) as well as lots of flavours of
installed [C](https://en.wikipedia.org/wiki/C_(programming_language))
compilers.  It was once a tedious task to provide a build process general
enough to cope with this variety.  Determining the particular properties of
your OS on your computer (such as the installed libraries) was an essential
first step in any build process.

The situation has changed quite a bit since around 1995 when the metamath
executable was designed.  C meanwhile matured and was subject to a couple of
standardizations like ISO/IEC 9899:1999.  In effect the variety of C
installations has shrunk considerably. Any modern
[C99](https://en.wikipedia.org/wiki/C99) (or later) compiler along with their
libraries should now compile the metamath executable out of the box.

Nevertheless, from a user perspective, C programs are deployed with an
installation routine comprising a configure shell script and a make file.  We
briefly introduce their concepts here.

### configure

A particular [shell script](https://en.wikipedia.org/wiki/Unix_shell) called
[configure](https://en.wikipedia.org/wiki/Configure_script) tests your OS for
its features.  Simply determining _all_ possible properties would be extremely
excessive.  For example the Metamath build need not know anything about your
network connections.  In fact, only a tiny selection of properties is
essential.  In Metamath this selection is encoded in a file __configure.h.in__.
_configure_ then performs all necessary checks based on its contents and at the
end issues a C header file __config.h__.  This header file defines lots of
C macros each reflecting a particular test result.

We give an example here:
In _config.h.in_ we find a line
```
/* Define to 1 if stdbool.h conforms to C99. */
#undef HAVE_STDBOOL_H
```
stdbool.h is a header file nowadays found in virtually every C installation.
A few decades ago that was less obvious, and the system had to be checked for
its existence.  The shell script _configure_ carries out this check, and it has
know-how built in to deal with lots of variants of both the file itself, and
where to find it on a system.  The test result is then issued to the _config.h_
result.  Either the above is copied verbatim, or
```
/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1
```
If the sources include this file then code alternatives can be established, as
in
```
#include <config.h>
#if HAVE_STDBOOL_H
  #include <stdbool.h>
#else
  typedef unsigned char bool;
#endif
```
Peculiar as it is, the Metamath executable never includes this generated file,
meaning its results are prescribed anyway, and must not vary with the system
(if not irreleveant).

### make

The _make_ program is usually part of your OS.  This executable is not
deployed, but the file it operates upon, the __Makefile__ is.  In a nutshell,
the _Makefile_ contains all information necessary to compile, link and install
the Metamath executable.  Besides this primary __goal__ a _Makefile_ may
support others as well, such as uninstalling, generating help and so on.

The Metamath makefile actually supports a plethora of _goals_, some of them
have are standardized meaning, many are for partial results, not meant to be
invoked by the user.

The metamath executable is simple enough to be made without the support of
_make_.  Nevertheless, some users expect there to be a _Makefile_.  If
metamath happens to be part of a distribution, then a _Makefile_ is not
dispensable any more.

## autotools

Writing any of the above files, _configure_ and _Makefile_, is all but easy, in
particular in a portable way.  A set of tools was written to support developers
with this task.  This set is called
[Autotools](https://en.wikipedia.org/wiki/GNU_Autotools).  The idea is to allow
the developer write replacements for _configure_ and _Makefile_ in a language
hiding portability issues.  The necessary know-how is built into these tools
and is automatically applied on translation to properly designed _configure_
and _Makefile_.  From a distributors point of view the Autotools are applied as
described in the following paragraphs.

Executing ```autoconf --version``` shows whether you have _Autotools_
installed on your computer.

### autoscan

The first step is to identify all constructs subject to portability issues.
_autoscan_ provides you with a suggestion in a __configure.scan__ file.
This file is meant to be renamed to __autoconf.ac__, possibly extended with
extra commands.  In Metamath commands for strengthening compiler flags
are added.

### configure.ac

This script file encodes the features the OS needs to be tested for in a
portable way.  When processed by _autoconf_ a _configure_ script along with its
input file _config.h.in_ is created.  A few instructions set project data like
name or version number.  A couple of other instructions aim at patching system
dependent variables in _Makefile.am_, later used to support _make_.

The script language used for encoding this is [M4]
(https://www.gnu.org/software/m4/manual) using a couple of built-in commands of
__autoconf__.  This language is designed to provide cross-platform descriptions
of features of the OS.

## autoreconf

__autoreconf__ manages the complete build process and finally produces a
_configure_ and _Makefile_ ready for use.  It is an alternative to the
following chain of script calls doing the same, but allowing for manual
intervention that can resolve difficult cases.  So far Metamath does not need
this intervention, so _autoreconf_ is usually a correct shortcut.

Internally _autoreconf_ calls the following autotools in the right order and
with the right input files.

#### aclocal

A program that looks at _configure.ac_ and provides a file __aclocal.m4__ with
_M4_ macros adapted to _automake_ directed commands.  This extends the
functionality of _autoconf_ in a way that it can parse and process the
_configure.ac_ file.

_autoreconf_ automatically calls this program during the build process.

#### aclocal.m4

An _M4_ library used by _autoconf_.  Its contents is of technical nature and
contains functions adapted to your system and your _configure.ac_.  To experts
editing this file may fix or fine tune particular operations.  Ordinary users
do not fiddle with it.

### autoheader

This program from _Autotools_ creates a _config.h.in_ from a _configure.ac_
input file.

If you use _autoreconf_ this is called automatically.

### config.h.in

This file is created by _autoheader_ and mostly contains a list of features the
_configure_ tests the OS for.  The encoding is macro instructions in
[C](https://en.wikipedia.org/wiki/C_(programming_language)).  Its contents
looks mostly like:
```
/* Define to 1 if stdbool.h conforms to C99. */
#undef HAVE_STDBOOL_H
```
It appears as a translation of the programmer supplied _configure.ac_ with all
_autoconf_ specific instructions stripped, and is used as a template for the
final _config.h_.

Although _config.h.in_ is valid C code, you must not use it in your software
directly.  Its sole purpose is to be read by _configure_.  It instructs this
script what OS tests to carry out, and how to encode their result.  If you
deploy _configure_ as part of your distribution then this file should be
included.

#### autom4te.cache

Read 'automate'.  Allows to share results between the individual _autotools_
members.  Do not interfere with its contents.

### autoconf

This Unix program called __autoconf__, or its sibling __autoreconf__, is
capable of generating a _configure_ shell script from the input _configure.ac_.
It uses functions defined in _aclocal.m4_ to perform its task.

[Documentation](https://www.gnu.org/software/autoconf/)

### Makefile.am

### automake

[Documentation](https://www.gnu.org/software/automake/manual)

### invoking configure

In the chain of build events _configure_ is now invoked.  It reads the
_config.h.in_ file, executes tests described there and compiled in.  As a
result, a new executable __config.status__ is created and invoked.

### config.status

Executable that creates the file _config.h_ based on _config.h.in_.  The
individual settings evaluated by _configure_ are compiled in.  So this script
can recreate _config.h_ multiple times without carrying out the OS checks.

### config.h

... to be continued
