## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libsrc.la
libsrc_la_SOURCES       = src/omx_jpegd.c src/omx_jpegd_test_main.c src/omx_jpegd_testcases.c
libsrc_la_LIBADD        = @LTLIBOBJS@ \
	$(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_la_CFLAGS        = \
	-I$(top_srcdir)/test/jpegdec/inc \
	$(OMXCORE_CFLAGS) $(MMOSAL_CFLAGS) $(SYSLINK_CFLAGS) $(MEMMGR_CFLAGS)

