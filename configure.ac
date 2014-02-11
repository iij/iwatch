#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([watch], [1.0], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AC_CONFIG_SRCDIR([watch.c])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AX_WITH_CURSES
if test "x$ax_cv_ncursesw" != xyes; then
    AC_MSG_ERROR([requires NcursesW library])
fi

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h string.h sys/param.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 memset select setlocale wcwidth])

AC_CONFIG_FILES([GNUmakefile])
AC_OUTPUT
