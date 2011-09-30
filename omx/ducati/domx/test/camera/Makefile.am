## Process this file with automake to produce Makefile.in

COMMON_FLAGS = \
        -I$(top_srcdir)/test/camera/inc \
        $(OMXCORE_CFLAGS) $(MMOSAL_CFLAGS) $(SYSLINK_CFLAGS) $(MEMMGR_CFLAGS)
noinst_LTLIBRARIES      = libsrc.la libsrc_pre.la libsrc_imgcap.la libsrc_vidcap.la libsrc_zoom.la libsrc_fps.la libsrc_3a.la libsrc_no_vnf.la
libsrc_la_SOURCES       = src/camera_il_client.c
libsrc_la_LIBADD        = @LTLIBOBJS@ \
	$(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_pre_la_SOURCES       = src/cam_pre_test.c src/dss_lib.c
libsrc_pre_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_pre_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_vidcap_la_SOURCES       = src/cam_vidcap_test.c src/dss_lib.c
libsrc_vidcap_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_vidcap_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_imgcap_la_SOURCES       = src/cam_imgcap_test.c src/dss_lib.c
libsrc_imgcap_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_imgcap_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_zoom_la_SOURCES       = src/cam_zoom_test.c src/dss_lib.c
libsrc_zoom_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_zoom_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_fps_la_SOURCES       = src/cam_fps_test.c src/dss_lib.c
libsrc_fps_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_fps_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_3a_la_SOURCES       = src/cam_3a_test.c src/dss_lib.c
libsrc_3a_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_3a_la_CFLAGS        = $(COMMON_FLAGS)

libsrc_no_vnf_la_SOURCES       = src/cam_vidcap_no_vnf_test.c src/dss_lib.c
libsrc_no_vnf_la_LIBADD        = @LTLIBOBJS@ \
        $(OMXCORE_LIBS) $(MMOSAL_LIBS) $(SYSLINK_LIBS) $(MEMMGR_LIBS)
libsrc_no_vnf_la_CFLAGS        = $(COMMON_FLAGS)
