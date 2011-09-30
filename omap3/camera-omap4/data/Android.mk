LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := TICameraCameraProperties.xml
LOCAL_MODULE := TICameraCameraProperties.xml
LOCAL_MODULE_CLASS := ETC

#TODO: revisit with right LOCAL_MODULE_TAGS usage,
#this flag is not populated to all Android.mk as intended
#LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)
