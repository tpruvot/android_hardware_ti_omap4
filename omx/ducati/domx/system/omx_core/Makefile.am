## Process this file with automake to produce Makefile.in

SUBDIRS                 = src

lib_LTLIBRARIES         = libOMX_Core.la

libOMX_Core_la_SOURCES  = 
libOMX_Core_la_LDFLAGS  = -no-undefined
libOMX_Core_la_LIBADD   = src/libsrc.la

libOMX_Core_la_include_HEADERS = inc/OMX_ComponentRegistry.h inc/OMX_Debug.h inc/OMX_Image.h inc/OMX_Types.h \
	inc/OMX_Audio.h inc/OMX_ContentPipe.h inc/OMX_DebugMem.h inc/OMX_Index.h inc/OMX_Video.h \
	inc/OMX_Component.h inc/OMX_Core.h inc/OMX_IVCommon.h inc/OMX_Other.h inc/OMX_TI_Common.h \
	inc/OMX_TI_IVCommon.h inc/OMX_TI_Index.h inc/OMX_TI_Video.h inc/OMX_TI_Core.h \
	inc/OMX_ComponentExt.h inc/OMX_CoreExt.h inc/OMX_IndexExt.h inc/OMX_VideoExt.h
libOMX_Core_la_includedir   = $(includedir)/omx

pkgconfig_DATA          = libOMX_Core.pc
pkgconfigdir            = $(libdir)/pkgconfig
