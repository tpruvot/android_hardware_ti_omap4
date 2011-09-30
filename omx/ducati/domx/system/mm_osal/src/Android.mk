LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	timm_osal_events.c timm_osal_mutex.c timm_osal_task.c \
	timm_osal.c timm_osal_memory.c timm_osal_pipes.c timm_osal_semaphores.c \
	timm_osal_trace.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../inc \
	$(PV_INCLUDES)

LOCAL_SHARED_LIBRARIES := \
	libdl \
	liblog \
	libc

LOCAL_CFLAGS := -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic
LOCAL_CFLAGS += -DOMAP_2430 -DOMX_DEBUG -D_Android -D_POSIX_VERSION_1_

LOCAL_MODULE:= libOMX_CoreOsal
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
