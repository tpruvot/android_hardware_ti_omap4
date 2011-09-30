## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES      = libsrc.la
libsrc_la_SOURCES       = timm_osal_events.c timm_osal_mutex.c timm_osal_task.c \
	timm_osal.c timm_osal_memory.c timm_osal_pipes.c timm_osal_semaphores.c \
	timm_osal_trace.c
libsrc_la_LIBADD        = @LTLIBOBJS@
libsrc_la_CFLAGS        = -I. $(MMOSAL_CFLAGS) $(WARN_CFLAGS)

