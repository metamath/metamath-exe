#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# seems to be created by autoscan and adapted to the needs of Metamath

    # the version of autoconf needed to translate this file into a portable
    # configure script.  As of this writing version 2.71 is published.
AC_PREREQ([2.65])

    # program name, version, email to report bugs
AC_INIT([metamath], [m4_esyscmd_s([./build.sh -va])], [nm.NOSPAM@alum.mit.edu])

    # Put auxiliary autoconf scripts in config/
AC_CONFIG_AUX_DIR([config])

    # Do not expect full GNU conformance (files like NEWS might be missing).
    # Minimal checks only.
AM_INIT_AUTOMAKE([foreign])

    # unique file name likely not existing outside the source tree.
    # catches incorrect --srcdir parameter in autoconf call.
AC_CONFIG_SRCDIR([src/metamath.c])

    # requires a config.h.in file, that is used as a template for a config.h,
    # modified according to following commands.  A script created by autoconf
    # (configure) will finally create the desired config.h.
    # Metamath sources do not include this config.h so far, although it is
    # common practise to do so.
AC_CONFIG_HEADERS([config.h])

    # requires a Makefile.am, that is modified according to following commands.
    # AC_OUTPUT creates a Makefile.in based on this modifications..
AC_CONFIG_FILES([Makefile src/Makefile])

# Checks for programs.

    # add this to support the tools folder
# AC_PROG_AWK

    # test existence of a C compiler
AC_PROG_CC

    # find an appropriate install executable
    # should be replaced with AC_PROG_MAKE_SET
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.

    # Fill config.h.in with macros HAVE_<header>_H and define them to 1,
    # if a standard complient header is found.
AC_CHECK_HEADERS([limits.h stdlib.h string.h])

   # config.h: set HAVE_STDBOOL_H
AC_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strchr strcspn strstr])

    # copied to Makefile.am
# Enable gcc warning flags, but only if they seem to work
new_CFLAGS="-Wall -Wextra"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $new_CFLAGS"

    # configure displays this message
AC_MSG_CHECKING([[for gcc warning flags]])

    # compile this program with new_CFLAGS enabled, see if flags are
    # accepted, provide them to automake
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[
#include <stdio.h>
int f() {
  return 0;
}
]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
   AM_CFLAGS="$AM_CFLAGS $new_CFLAGS"])

    # compile the following program with a bunch of optimization flags.
    # If accepted, provide them to automake.
    # Take care of a possible collision with flag -O2.
# Try to optimize.
AC_MSG_CHECKING([[for optimization flags]])
new_CFLAGS="-O3 -funroll-loops -finline-functions -fomit-frame-pointer"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $new_CFLAGS"
# Remove any existing "-O2", or it will override what we're doing.
CFLAGS=$( printf "%s" "$CFLAGS" | sed -e 's/ -O2/ /' )
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[
#include <stdio.h>
int f() {
  return 0;
}
]])],
  [AC_MSG_RESULT([yes])
   CFLAGS="$saved_CFLAGS"
   CFLAGS=$( printf "%s" "$CFLAGS" | sed -e 's/ -O2/ /' )
   AM_CFLAGS="$AM_CFLAGS $new_CFLAGS"],
  [AC_MSG_RESULT([no])
   CFLAGS="$saved_CFLAGS"])

# Can we use "inline"? We don't use AC_C _INLINE because metamath.c
# does not include the autoconf-generated file.
AC_MSG_CHECKING([[for 'inline' support]])
new_CFLAGS="-DINLINE=inline"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $new_CFLAGS"
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    inline int f(void) {}
  ]])],
  [AC_MSG_RESULT([yes])
   AM_CFLAGS="$AM_CFLAGS $new_CFLAGS"],
  [AC_MSG_RESULT([no])])
CFLAGS="$saved_CFLAGS"

    # replace all @AM_CFLAGS@ and @CFLAGS@ variables in Makefile.am
    # with values found here
echo "CFLAGS=$CFLAGS"
AC_SUBST([AM_CFLAGS])
AC_SUBST([CFLAGS])

AC_OUTPUT
