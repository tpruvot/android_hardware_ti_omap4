#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Syslink, 0.1, http://www.ti.com)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([-Wall])
# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([clock_gettime],[rt])

#To disable buidling static libraries
AC_DISABLE_STATIC
#The below MACRO is required to build Shared libraries
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h] )

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Checks for library functions.

AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([clock_gettime getpagesize memset munmap strcasecmp strncasecmp strtol])
AC_CHECK_FUNCS([posix_memalign])

# Currently Build for 4430 Proc Family. Please change this
# to build it for other Proc Family
AC_SUBST(PROCFAMILY, OMAP_4430)
AM_CONDITIONAL(PROCFAMILY_IS_4430, [test "x$PROCFAMILY" = xOMAP_4430])
AM_CONDITIONAL(PROCFAMILY_IS_3430, [test "x$PROCFAMILY" = xOMAP_3430])
AM_CONDITIONAL(PROCFAMILY_IS_2430, [test "x$PROCFAMILY" = xOMAP_2430])

CFLAGS=
#Compiling with Debug enabled by default. Change it to false to disable.
default_debug=false
AC_ARG_ENABLE([debug],
	[  --enable-debug          Turn on debugging,[default=false]],
	[case "${enableval}" in
	yes) debug=true
		CFLAGS="$CFLAGS -g";;
	no)  debug=false
		CFLAGS="$CFLAGS  -fomit-frame-pointer";;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug])
		CFLAGS="$CFLAGS  -fomit-frame-pointer";;
	esac],[debug=$default_debug])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])


# Check for kernel src path:
AC_ARG_WITH(kpath,[  --with-kpath             specify the path to the kernel modules],[
  kpath="$withval"
],[
  # default to checking $KRNLSRC environment variable, for backwards compatibility
  kpath=$KRNLSRC
])
AM_CONDITIONAL([KRNLSRC], [test x$kpath != x])
AC_SUBST(KRNLSRC)

#Warning Flags
CFLAGS="$CFLAGS  -Wall  -Wno-trigraphs -Werror-implicit-function-declaration "

#Optimization Flags
CFLAGS="$CFLAGS -fno-strict-aliasing"

if test x$PROCFAMILY = xOMAP_3430; then
CFLAGS="$CFLAGS -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -march=armv7-a -msoft-float -Uarm -D__LINUX_ARM_ARCH__=7"
fi

if test x$PROCFAMILY = xOMAP_4430; then
CFLAGS="$CFLAGS  -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -march=armv7-a -msoft-float -Uarm -D__LINUX_ARM_ARCH__=7 "
fi

# Code generation flags
CFLAGS="$CFLAGS -fno-common"
#Macros
CFLAGS="$CFLAGS -DLINUX"

#if __KERNEL__
#CFLAGS="$CFLAGS -D__KERNEL__  -fno-builtin"
#endif

#AC_C_COMPILE_FLAGS(-Wextra)

# Check for pkgconfig first
AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])

# Check for libraries
PKG_CHECK_MODULES([MEMMGR], [libtimemmgr])

AC_CONFIG_FILES([syslink.pc
		Makefile
		api/Makefile
		api/include/Makefile
		api/include/ti/ipc/Makefile
		api/make/Makefile
		api/src/Makefile
		api/src/ipc/Makefile
		api/src/notify/Makefile
		api/src/procmgr/Makefile
		api/src/procmgr/procmgr4430/Makefile
		api/src/rcm/Makefile
		api/src/sysmgr/Makefile
		api/src/utils/Makefile
		daemons/Makefile
		daemons/inc/Makefile
		daemons/syslink/Makefile
		daemons/trace/Makefile
		samples/Makefile
		samples/make/Makefile
		samples/ipc/Makefile
		samples/ipc/gateMP/Makefile
		samples/ipc/gateMP/usr/Makefile
		samples/ipc/heapBufMP/Makefile
		samples/ipc/heapBufMP/usr/Makefile
		samples/ipc/heapMemMP/Makefile
		samples/ipc/heapMemMP/usr/Makefile
		samples/ipc/listMP/Makefile
		samples/ipc/listMP/usr/Makefile
		samples/ipc/messageQ/Makefile
		samples/ipc/messageQ/usr/Makefile
		samples/ipc/nameServer/Makefile
		samples/ipc/nameServer/usr/Makefile
		samples/ipc/sharedRegion/Makefile
		samples/ipc/sharedRegion/usr/Makefile
		samples/notify/Makefile
		samples/notify/notifyping/Makefile
		samples/procmgr/Makefile
		samples/procmgr/ducati_load/Makefile
		samples/procmgr/procmgrapp/Makefile
		samples/rcm/Makefile
		samples/rcm/multi_test/Makefile
		samples/rcm/single_test/Makefile
		samples/tiler/Makefile
		samples/pwrmgmt/Makefile
		samples/pwrmgmt/pmtransport/Makefile
		samples/pwrmgmt/pmresources/Makefile
		samples/pwrmgmt/pmgeneric/Makefile
		samples/interm3/Makefile
		samples/event_listener/Makefile
		samples/dmm/Makefile
		samples/memmgr/Makefile
		samples/deh/Makefile])

AC_OUTPUT
echo "
	Syslink Database Library ($PACKAGE_NAME) version $PACKAGE_VERSION
	Prefix...........: $prefix
	Debug Build......: $debug
	Processor Family.: $PROCFAMILY
	C Compiler.......: $CC
	Linker...........: $LD $LDFLAGS $LIBS
"
