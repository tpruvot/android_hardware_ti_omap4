## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = \
	libh264e.la \
	libh264d.la \
	libmpeg4d.la \
	libvideodecoder.la \
	libcamera.la \
	libjpegd.la \
	libsample.la \
	libmpeg4e.la \
	libvp6d.la \
	libvp7d.la

common_libadd = \
	$(MMOSAL_LIBS) \
	$(OMXCORE_LIBS) \
	$(SYSLINK_LIBS)

common_cflags = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir)/system/domx \
	-I$(top_srcdir)/system/domx/omx_rpc/inc \
	$(MMOSAL_CFLAGS) \
	$(OMXCORE_CFLAGS) \
	$(SYSLINK_CFLAGS) \
	$(MEMMGR_CFLAGS)

libh264e_la_SOURCES     = omx_proxy_h264enc.c
libh264e_la_LIBADD      = $(common_libadd)
libh264e_la_CFLAGS      = $(common_cflags)

libh264d_la_SOURCES     = omx_proxy_h264dec.c
libh264d_la_LIBADD      = $(common_libadd)
libh264d_la_CFLAGS      = $(common_cflags)

libmpeg4d_la_SOURCES     = omx_proxy_mpeg4dec.c
libmpeg4d_la_LIBADD      = $(common_libadd)
libmpeg4d_la_CFLAGS      = $(common_cflags)

libvideodecoder_la_SOURCES     = omx_proxy_videodec.c
libvideodecoder_la_LIBADD      = $(common_libadd)
libvideodecoder_la_CFLAGS      = $(common_cflags)

libvp6d_la_SOURCES      = omx_proxy_vp6dec.c
libvp6d_la_LIBADD       = $(common_libadd)
libvp6d_la_CFLAGS       = $(common_cflags)

libvp7d_la_SOURCES      = omx_proxy_vp7dec.c
libvp7d_la_LIBADD       = $(common_libadd)
libvp7d_la_CFLAGS       = $(common_cflags)

libcamera_la_SOURCES    = omx_proxy_camera.c
libcamera_la_LIBADD     = $(common_libadd)
libcamera_la_CFLAGS     = $(common_cflags)

libjpegd_la_SOURCES     = omx_proxy_jpeg_dec.c
libjpegd_la_LIBADD      = $(common_libadd)
libjpegd_la_CFLAGS      = $(common_cflags)

libsample_la_SOURCES    = omx_proxy_sample.c
libsample_la_LIBADD     = $(common_libadd)
libsample_la_CFLAGS     = $(common_cflags)

libmpeg4e_la_SOURCES     = omx_proxy_mpeg4enc.c
libmpeg4e_la_LIBADD      = $(common_libadd)
libmpeg4e_la_CFLAGS      = $(common_cflags)


