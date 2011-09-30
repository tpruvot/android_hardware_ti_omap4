LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	omx_proxy_common.c \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../ \
	$(LOCAL_PATH)/../../omx_rpc/inc \
	$(LOCAL_PATH)/../../../domx \
	hardware/ti/omap4/omx/ducati/domx/system/omx_core/inc \
	hardware/ti/omap4/omx/ducati/domx/system/mm_osal/inc \
	hardware/ti/omap4/tiler \
	hardware/ti/omap4/syslink/syslink/d2c \
	hardware/ti/omap4/syslink/syslink/api/include \

LOCAL_CFLAGS := -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic
LOCAL_CFLAGS += -D_Android

LOCAL_SHARED_LIBRARIES := \
		libOMX_CoreOsal \
		libipcutils \
		libsysmgr \
		libipc \
		librcm \
		libnotify \
		libc \
		liblog	\
		libtimemmgr \
		libd2cmap \
		libomx_rpc

LOCAL_MODULE:= libomx_proxy_common
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
