#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([metamath], [0.114], [nm.NOSPAM@alum.mit.edu])
AC_CONFIG_SRCDIR([metamath.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h])
AC_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strchr strcspn strstr])

# Enable gcc warning flags, but only if they seem to work
new_CFLAGS="-Wall -Wextra"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $new_CFLAGS"
AC_MSG_CHECKING([[for gcc warning flags]])
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

echo "CFLAGS=$CFLAGS"
AC_SUBST([AM_CFLAGS])
AC_SUBST([CFLAGS])

AC_OUTPUT
