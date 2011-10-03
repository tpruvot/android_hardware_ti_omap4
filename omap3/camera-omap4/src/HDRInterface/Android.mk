
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ARCSOFT_LIB_PATH:= ../../../../../../../vendor/arcsoft/HDRCapture
LOCAL_PREBUILT_LIBS :=  \
    $(ARCSOFT_LIB_PATH)/lib/android/libarcsoft_hdr.a \
    $(ARCSOFT_LIB_PATH)/lib/android/libarcsoft_jpgcodec.a \
    $(ARCSOFT_LIB_PATH)/lib/android/libmpbase_hdr.a \
    $(ARCSOFT_LIB_PATH)/lib/android/libmpkernel_hdr.a \
    $(ARCSOFT_LIB_PATH)/lib/android/libmputility_hdr.a \

LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= MotHDRInterface.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../inc/HDRInterface \
    $(LOCAL_PATH)/../../inc \
    $(LOCAL_PATH)/../../inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../../../../tiler/memmgr \
    $(LOCAL_PATH)/../../../libtiutils \
    $(LOCAL_PATH)/../../../liboverlay \
    external/libxml2/include \
    external/icu4c/common \
    frameworks/base/include/utils \
    vendor/arcsoft/HDRCapture/inc \
    $(call include-path-for, system-core)/cutils

LOCAL_STATIC_LIBRARIES := \
    libarcsoft_hdr \
    libarcsoft_jpgcodec \
    libmpbase_hdr \
    libmputility_hdr \
    libmpkernel_hdr

LOCAL_MODULE := libhdr_interface
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES := libdl libcamera libcutils libutils liblog libbinder

include $(BUILD_SHARED_LIBRARY)
