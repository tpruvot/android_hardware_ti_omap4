LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm



LOCAL_SRC_FILES := \
IPCManager.c \
MultiProcDrv.c \
MultiProc.c \
SharedRegionDrv.c \
SharedRegion.c\
HeapMemMPDrv.c \
HeapMemMP.c \
GateMPDrv.c \
GateMP.c \
HeapBufMPDrv.c \
HeapBufMP.c \
NameServerDrv.c \
NameServer.c \
ListMPDrv.c \
ListMP.c \
MessageQDrv.c \
MessageQ.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/../../include/ti/ipc


LOCAL_CFLAGS += -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic

LOCAL_SHARED_LIBRARIES += \
		libc \
		libipcutils \
		liblog

LOCAL_MODULE    := libipc
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
