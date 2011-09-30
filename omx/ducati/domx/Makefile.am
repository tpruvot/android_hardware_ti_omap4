# Process this file with automake to produce Makefile.in

SUBDIRS = \
	system/mm_osal \
	system/omx_core \
	system/domx \
	video/omx_proxy_component/src

lib_LTLIBRARIES = \
	libdomx.la \
	libOMX.TI.DUCATI1.VIDEO.H264E.la \
	libOMX.TI.DUCATI1.VIDEO.H264D.la \
	libOMX.TI.DUCATI1.VIDEO.MPEG4D.la \
	libOMX.TI.DUCATI1.VIDEO.DECODER.la \
	libOMX.TI.DUCATI1.VIDEO.CAMERA.la \
	libOMX.TI.DUCATI1.IMAGE.JPEGD.la \
	libOMX.TI.DUCATI1.MISC.SAMPLE.la \
	libOMX.TI.DUCATI1.VIDEO.MPEG4E.la \
	libOMX.TI.DUCATI1.VIDEO.VP6D.la \
	libOMX.TI.DUCATI1.VIDEO.VP7D.la

# core DOMX:
libdomx_la_SOURCES      =
libdomx_la_LDFLAGS      = -no-undefined
libdomx_la_LIBADD       = system/domx/libsrc.la

# proxy components:
libOMX_TI_DUCATI1_VIDEO_H264E_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_H264E_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_H264E_la_LIBADD   = video/omx_proxy_component/src/libh264e.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_H264D_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_H264D_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_H264D_la_LIBADD   = video/omx_proxy_component/src/libh264d.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_MPEG4D_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_MPEG4D_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_MPEG4D_la_LIBADD   = video/omx_proxy_component/src/libmpeg4d.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_DECODER_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_DECODER_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_DECODER_la_LIBADD   = video/omx_proxy_component/src/libvideodecoder.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_CAMERA_la_SOURCES =
libOMX_TI_DUCATI1_VIDEO_CAMERA_la_LDFLAGS = -no-undefined
libOMX_TI_DUCATI1_VIDEO_CAMERA_la_LIBADD  = video/omx_proxy_component/src/libcamera.la libdomx.la

libOMX_TI_DUCATI1_IMAGE_JPEGD_la_SOURCES  =
libOMX_TI_DUCATI1_IMAGE_JPEGD_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_IMAGE_JPEGD_la_LIBADD   = video/omx_proxy_component/src/libjpegd.la libdomx.la

libOMX_TI_DUCATI1_MISC_SAMPLE_la_SOURCES  =
libOMX_TI_DUCATI1_MISC_SAMPLE_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_MISC_SAMPLE_la_LIBADD   = video/omx_proxy_component/src/libsample.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_MPEG4E_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_MPEG4E_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_MPEG4E_la_LIBADD   = video/omx_proxy_component/src/libmpeg4e.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_VP6D_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_VP6D_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_VP6D_la_LIBADD   = video/omx_proxy_component/src/libvp6d.la libdomx.la

libOMX_TI_DUCATI1_VIDEO_VP7D_la_SOURCES  =
libOMX_TI_DUCATI1_VIDEO_VP7D_la_LDFLAGS  = -no-undefined
libOMX_TI_DUCATI1_VIDEO_VP7D_la_LIBADD   = video/omx_proxy_component/src/libvp7d.la libdomx.la

# test apps:
if BUILD_TESTS
  SUBDIRS += test/camera test/h264dec test/h264play test/h264play2 test/h264enc test/jpegdec test/sample_proxy test/mpeg4dec test/mpeg4enc

  bin_PROGRAMS = omx_camera_test omx_h264dec_test omx_h264play_test omx_h264play2_test omx_h264enc_test omx_jpegdec_test omx_sample_test omx_cam_pre_test omx_cam_capt_test omx_cam_vid_cap_test omx_cam_zoom_test omx_cam_fps_test omx_cam_3a_test omx_cam_vid_no_vnf_test omx_mpeg4dec_test omx_mpeg4enc_test

  omx_camera_test_SOURCES      =
  omx_camera_test_LDFLAGS      = -no-undefined
  omx_camera_test_LDADD        = test/camera/libsrc.la libdomx.la

  omx_h264play_test_SOURCES     =
  omx_h264play_test_LDFLAGS     = -no-undefined
  omx_h264play_test_LDADD       = test/h264play/libsrc.la libdomx.la

  omx_h264play2_test_SOURCES     =
  omx_h264play2_test_LDFLAGS     = -no-undefined
  omx_h264play2_test_LDADD       = test/h264play2/libsrc.la libdomx.la

  omx_h264dec_test_SOURCES     =
  omx_h264dec_test_LDFLAGS     = -no-undefined
  omx_h264dec_test_LDADD       = test/h264dec/libsrc.la libdomx.la

  omx_mpeg4dec_test_SOURCES     =
  omx_mpeg4dec_test_LDFLAGS     = -no-undefined
  omx_mpeg4dec_test_LDADD       = test/mpeg4dec/libsrc.la libdomx.la

  omx_h264enc_test_SOURCES     =
  omx_h264enc_test_LDFLAGS     = -no-undefined
  omx_h264enc_test_LDADD       = test/h264enc/libsrc.la libdomx.la

  omx_jpegdec_test_SOURCES     =
  omx_jpegdec_test_LDFLAGS     = -no-undefined
  omx_jpegdec_test_LDADD       = test/jpegdec/libsrc.la libdomx.la

  omx_sample_test_SOURCES      =
  omx_sample_test_LDFLAGS      = -no-undefined
  omx_sample_test_LDADD        = test/sample_proxy/libsrc.la libdomx.la

  omx_cam_pre_test_SOURCES      =
  omx_cam_pre_test_LDFLAGS      = -no-undefined
  omx_cam_pre_test_LDADD        = test/camera/libsrc_pre.la libdomx.la

  omx_cam_capt_test_SOURCES      =
  omx_cam_capt_test_LDFLAGS      = -no-undefined
  omx_cam_capt_test_LDADD        = test/camera/libsrc_imgcap.la libdomx.la

  omx_cam_vid_cap_test_SOURCES      =
  omx_cam_vid_cap_test_LDFLAGS      = -no-undefined
  omx_cam_vid_cap_test_LDADD        = test/camera/libsrc_vidcap.la libdomx.la

  omx_cam_zoom_test_SOURCES      =
  omx_cam_zoom_test_LDFLAGS      = -no-undefined
  omx_cam_zoom_test_LDADD        = test/camera/libsrc_zoom.la libdomx.la

  omx_cam_fps_test_SOURCES      =
  omx_cam_fps_test_LDFLAGS      = -no-undefined
  omx_cam_fps_test_LDADD        = test/camera/libsrc_fps.la libdomx.la

  omx_cam_3a_test_SOURCES      =
  omx_cam_3a_test_LDFLAGS      = -no-undefined
  omx_cam_3a_test_LDADD        = test/camera/libsrc_3a.la libdomx.la

  omx_cam_vid_no_vnf_test_SOURCES      =
  omx_cam_vid_no_vnf_test_LDFLAGS      = -no-undefined
  omx_cam_vid_no_vnf_test_LDADD        = test/camera/libsrc_no_vnf.la libdomx.la

  omx_mpeg4enc_test_SOURCES     =
  omx_mpeg4enc_test_LDFLAGS     = -no-undefined
  omx_mpeg4enc_test_LDADD       = test/mpeg4enc/libsrc.la libdomx.la

endif
