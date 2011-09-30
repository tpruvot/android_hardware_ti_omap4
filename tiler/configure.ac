#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(timemmgr, 2.00, http://www.ti.com)
AC_CONFIG_SRCDIR([memmgr.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([timemmgr], [2.00])
LT_INIT

# Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_MMAP
AC_CHECK_FUNCS([munmap strerror])

AC_ARG_ENABLE(tilermgr,
[  --enable-tilermgr    Include TilerMgr headers],
[case "${enableval}" in
  yes) tilermgr=true ;;
  no)  tilermgr=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-tilermgr) ;;
esac],[tilermgr=true])

AC_ARG_ENABLE(tests,
[  --enable-tests    Build unit tests],
[case "${enableval}" in
  yes) tests=true ;;
  no)  tests=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-tests) ;;
esac],[tests=false])

AM_CONDITIONAL(UNIT_TESTS, test x$tests = xtrue)

AM_CONDITIONAL(TILERMGR, test x$tilermgr != xfalse)

AC_ARG_ENABLE(stub,
[  --enable-stub    Stubbing tiler and syslink on none-arm system],
[case "${enableval}" in
  yes) stub_tiler=true ;;
  no)  stub_tiler=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-stub) ;;
esac],[stub_tiler=false])

AM_CONDITIONAL(STUB_TILER, test x$stub_tiler = xtrue)

if test x$stub_tiler = xtrue; then
AC_DEFINE([STUB_TILER],[1],[Use tiler stub])
fi

# Project build flags
MEMMGR_CFLAGS="-Werror -Wall -pipe -ansi"
AC_SUBST(MEMMGR_CFLAGS)

AC_CONFIG_FILES([libtimemmgr.pc Makefile])
AC_OUTPUT

