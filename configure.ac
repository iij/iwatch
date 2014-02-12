#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([watch], [1.0], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# wcwidth requires _XOPEN_SOURCE
AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AX_WITH_CURSES
if test "x$ax_cv_ncursesw" != xyes; then
	AC_MSG_ERROR([requires NcursesW library])
fi

# Checks for header files.
AC_CHECK_HEADERS([sys/param.h])

# Checks for library functions.
AC_CHECK_FUNCS([strlcat])

AC_CONFIG_FILES([GNUmakefile])

AC_OUTPUT
