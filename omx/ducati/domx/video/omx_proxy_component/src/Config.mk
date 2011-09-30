include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../inc \
	$(LOCAL_PATH)/../../../system/domx/ \
	$(LOCAL_PATH)/../../../system/domx/omx_rpc/inc/ \
	hardware/ti/omap4/omx/ducati/domx/system/mm_osal/inc/ \
	hardware/ti/omap4/omx/ducati/domx/system/omx_core/inc/ \
	hardware/ti/omap4/syslink/syslink/api/include \
	hardware/ti/omap4/tiler \
	hardware/ti/omap4/syslink/syslink/d2c

LOCAL_SHARED_LIBRARIES := \
		libOMX_CoreOsal \
		libipcutils \
		libsysmgr \
		libipc \
		librcm \
		libnotify \
		libc \
		libOMX_Core \
		liblog	\
		libtimemmgr \
		libd2cmap \
		libomx_proxy_common \
		libomx_rpc \

LOCAL_CFLAGS += -MD -pipe  -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -DTMS32060 -D_DB_TIOMAP  -DSYSLINK_USE_SYSMGR -DSYSLINK_USE_LOADER
LOCAL_CFLAGS += -D_Android
