## Process this file with automake to produce Makefile.in

SUBDIRS                 = src

lib_LTLIBRARIES         = libmmosal.la

libmmosal_la_SOURCES  = 
libmmosal_la_LDFLAGS  = -no-undefined
libmmosal_la_LIBADD   = src/libsrc.la

libmmosal_la_include_HEADERS = inc/timm_osal_events.h inc/timm_osal_memory.h inc/timm_osal_osal.h inc/timm_osal_semaphores.h inc/timm_osal_trace.h \
	inc/timm_osal_error.h inc/timm_osal_interfaces.h inc/timm_osal_mutex.h inc/timm_osal_pipes.h inc/timm_osal_task.h inc/timm_osal_types.h
libmmosal_la_includedir   = $(includedir)/omx/mmosal

pkgconfig_DATA          = libmmosal.pc
pkgconfigdir            = $(libdir)/pkgconfig
