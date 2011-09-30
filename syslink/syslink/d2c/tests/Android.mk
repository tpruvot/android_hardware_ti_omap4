
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	d2c_test.c \
	testlib.c

LOCAL_C_INCLUDES += \
	hardware/ti/omap4/tiler \
	$(LOCAL_PATH)/..

LOCAL_SHARED_LIBRARIES := \
	libd2cmap \
	libtimemmgr

LOCAL_CFLAGS += -MD -pipe  -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -DTMS32060 -D_DB_TIOMAP  -DSYSLINK_USE_SYSMGR -DSYSLINK_USE_LOADER

LOCAL_MODULE:= d2c_test

include $(BUILD_EXECUTABLE)
