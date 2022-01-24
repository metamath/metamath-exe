# Building the Metamath executable from sources

## Introductionary notes

In this file we describe how to build the executable metamath on a __Unix/Linux__
system from scratch using only the sources and a few auxiliary files, all
distributed through the Github Metamath git repository.

### Git folder of source files

[Git](https://en.wikipedia.org/wiki/Git) is widespread version control system.
It bundles files and subdirectories into a single folder called __repository__.
Unlike a simple folder a repository tracks changes to its contents, so you can
checkout older versions of a file, or different versions if multiple persons
work on files simultaniously.  It is out of the scope of this particular
document to detail on how this system works.  Instead we assume you have a
fresh set of metamath source files ready in a single folder.  It is of no
importance here how you got it.  The name of the folder is arbitrary, but for
the sake of simplicity we call it throughout this file __metamath-exe__.

## Unix/Linux environment

Although we restrict the build process on Unix/Linux, there exist numerous
variants of this operating system (__OS__).  It has always been a tedious task
to provide a general build process running on most of these flavours.
Determining the particular properties of your OS on your computer (such as the
installed build software) is a necessary prerequisite before we can execute the
first build step at all.

### configure

A particular [shell script](https://en.wikipedia.org/wiki/Unix_shell) called
[configure](https://en.wikipedia.org/wiki/Configure_script) serves this need.
Simply determining _all_ possible properties would be extremely excessive.  For
example the Metamath build need not know anything about your network
connections.  In fact, only a tiny selection of properties need to be known.
This selection is encoded in an OS independent manner in a file called
__configure.ac__.  A Unix program called __autoconf__, or its sibling
__autoreconf__, is capable of generating a _configure_ shell script from this
selection.  The resulting _configure_ script is portable.  So once generated
and available, the use of _auto(re)conf_ is dispensable for Metamath users.

### autotools

We assume here that you are not in the lucky situation to have a _configure_
script at hand.  The you must create one from _configure.ac_ file.  This
requires [Autotools](https://en.wikipedia.org/wiki/GNU_Autotools), a set of
executables on Unix/Linux supporting a build process, in particular generating
a _configure_ script from an input file like _configure.ac_.  You need to have
at least some of these programs installed on your computer to execute the full
build.  Executing ```autoconf --version``` shows whether you have _Autotools_
installed on your computer.  A description of these programs are found
[here](https://www.gnu.org/software/autoconf/manual/).

## configure.ac 

In other projects this file can have the alternative name _configure.in_.  It
is written in a particular script language called __M4__.  We provide an
annotated version _configure.ac.annotated_ in the documentation.

... to be continued
