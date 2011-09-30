## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libsrc.la
libsrc_la_SOURCES       = src/H264_Playback_ILClient_TestCases.c src/H264_Playback_ILClient.c src/v4l2_display.c
libsrc_la_LIBADD        = @LTLIBOBJS@ \
	$(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_la_CFLAGS        = \
	-I$(top_srcdir)/test/h264play/inc \
	$(OMXCORE_CFLAGS) $(MMOSAL_CFLAGS) $(SYSLINK_CFLAGS) $(MEMMGR_CFLAGS)

