LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	List.c \
	Memory.c \
	Trace.c \
	TraceDrv.c \
	Gate.c \
	String.c \
	MemoryOS.c \
	OsalPrint.c \
	Heap.c \
	OsalDrv.c \
	OsalMutex.c \
	GateMutex.c \
	OsalSemaphore.c \
	UsrUtilsDrv.c \
	Cache.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/../../../ \
	$(LOCAL_PATH)/../../include/ti/ipc

LOCAL_SHARED_LIBRARIES := \
        liblog


LOCAL_CFLAGS += -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic

LOCAL_MODULE    := libipcutils

include $(BUILD_SHARED_LIBRARY)
