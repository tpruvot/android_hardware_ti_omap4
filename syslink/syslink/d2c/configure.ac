#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(d2cmap, 1.00, http://www.ti.com)
AC_CONFIG_SRCDIR([phase1_d2c_remap.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_SYSTEM
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([d2cmap], [1.00])
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

AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])

PKG_CHECK_MODULES([SYSLINK], [syslink], [
  echo "SYSLINK found"
], [
  echo "SYSLINK not found. Stub enabled."
  AC_DEFINE([STUB_SYSLINK],[1])
  stub_syslink=true
])

# Check for libraries
PKG_CHECK_MODULES([MEMMGR], [libtimemmgr])

AC_SUBST(STUB_SYSLINK)

# Project build flags
# :NOTE: syslink does not compile with -ansi
D2C_CFLAGS="-Wall -Werror -pipe"
AC_SUBST(D2C_CFLAGS)

AC_ARG_ENABLE(tests,
[  --enable-tests    Build unit tests],
[case "${enableval}" in
  yes) tests=true ;;
  no)  tests=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-tests) ;;
esac],[tests=false])

AM_CONDITIONAL(UNIT_TESTS, test x$tests = xtrue)

AC_ARG_ENABLE(stub,
[  --enable-stub    Stubbing tiler and syslink on none-arm system],
[case "${enableval}" in
  yes) stub_syslink=true;  stub_tiler=true ;;
  no)  stub_syslink=false; stub_tiler=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-stub) ;;
esac],[stub_tiler=false])

if test x$stub_syslink = xtrue ; then
echo "Stubbing syslink on none-arm system."
AC_DEFINE([STUB_SYSLINK],[1],[Use syslink stub])
fi

if test x$stub_tiler = xtrue ; then
echo "Stubbing tiler on none-arm system."
AC_DEFINE([STUB_TILER],[1],[Use tiler stub])
fi

AM_CONDITIONAL(STUB_SYSLINK, test x$stub_syslink = xtrue)

AC_CONFIG_FILES([Makefile tests/Makefile libd2cmap.pc])
AC_OUTPUT
