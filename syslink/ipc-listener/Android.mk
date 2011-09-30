#TODO add license

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := \
	ISyslinkIpcListener.cpp \
	SyslinkIpcListener.cpp

LOCAL_SHARED_LIBRARIES:= \
    libutils \
    libbinder

#LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := libsyslink_ipc_listener
include $(BUILD_SHARED_LIBRARY)
