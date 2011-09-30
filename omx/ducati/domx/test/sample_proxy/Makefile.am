## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libsrc.la
libsrc_la_SOURCES       = test_sample_proxy.c
libsrc_la_LIBADD        = @LTLIBOBJS@ \
	$(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_la_CFLAGS        = \
	$(OMXCORE_CFLAGS) $(MMOSAL_CFLAGS) $(SYSLINK_CFLAGS) $(MEMMGR_CFLAGS)

