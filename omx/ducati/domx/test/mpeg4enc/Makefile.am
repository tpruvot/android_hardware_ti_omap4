## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libsrc.la
libsrc_la_SOURCES       = \
	src/MPEG4_ILClient.c 

libsrc_la_LIBADD        = @LTLIBOBJS@ \
	$(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)

libsrc_la_CFLAGS        = \
	-I$(top_srcdir)/test/mpeg4enc/inc \
	$(OMXCORE_CFLAGS) $(MMOSAL_CFLAGS) $(SYSLINK_CFLAGS) $(MEMMGR_CFLAGS) -DOMX_TILERTEST -DMPEG4_LINUX_CLIENT

