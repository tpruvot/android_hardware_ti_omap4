
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
MemMgrServer.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../api/include \
	$(LOCAL_PATH)/../../api/include/ti/ipc \
	$(LOCAL_PATH)/../inc \
	hardware/ti/omap4/tiler

LOCAL_SHARED_LIBRARIES := \
	libipcutils \
	libipc \
	librcm \
	libnotify \
	libsysmgr \
	libtimemmgr


LOCAL_CFLAGS += -MD -pipe  -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -DTMS32060 -D_DB_TIOMAP -DSYSLINK_USE_LOADER
#LOCAL_CFLAGS += -DSYSLINK_USE_DAEMON

LOCAL_MODULE:= memmgrserver.out

include $(BUILD_EXECUTABLE)
