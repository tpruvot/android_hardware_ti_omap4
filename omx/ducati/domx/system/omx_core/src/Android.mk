LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	OMX_Core.c \
	OMX_Core_Wrapper.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../inc \
        $(LOCAL_PATH)/../../mm_osal/inc \
	$(PV_INCLUDES) \
	hardware/ti/omap4/omx/ti_omx_config_parser/inc

LOCAL_SHARED_LIBRARIES := \
	libdl \
	libVendor_ti_omx_config_parser \
	liblog \
	libOMX_CoreOsal \
	libhardware_legacy
	
LOCAL_CFLAGS += -pipe -fomit-frame-pointer -Wall -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic
LOCAL_CFLAGS += -DSTATIC_TABLE -D_Android
LOCAL_MODULE:= libOMX_Core
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
