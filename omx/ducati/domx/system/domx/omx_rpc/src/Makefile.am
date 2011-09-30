## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libomx_rpc.la
libomx_rpc_la_SOURCES       = \
	omx_rpc_config.c \
	omx_rpc_platform.c \
	omx_rpc_skel.c \
	omx_rpc_stub.c \
	omx_rpc_utils.c \
	omx_rpc.c

libomx_rpc_la_LIBADD        = \
	$(MMOSAL_LIBS) \
	$(OMXCORE_LIBS) \
	$(SYSLINK_LIBS) \
	@LTLIBOBJS@

libomx_rpc_la_CFLAGS        = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir)/system/domx \
	-I$(top_srcdir)/system/domx/omx_rpc/inc \
	$(MMOSAL_CFLAGS) \
	$(OMXCORE_CFLAGS) \
	$(SYSLINK_CFLAGS)

