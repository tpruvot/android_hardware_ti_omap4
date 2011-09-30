LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
LOCAL_CFLAGS := -DTARGET_OMAP4 -DOMAP_ENHANCEMENT
endif

LOCAL_SRC_FILES := \
    stagefright_overlay_output.cpp \
    TIHardwareRenderer.cpp \
    TIOMXPlugin.cpp
#    TIOMXCodec.cpp
# TODO: TIOMXCodec introduced in 1.21p3
# revisit and include for flash player support

LOCAL_CFLAGS +:= $(PV_CFLAGS_MINUS_VISIBILITY)

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/base/include/media/stagefright/openmax \
        $(TOP)/hardware/ti/omap4/omap3/liboverlay

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl					\
        libsurfaceflinger_client \
        libstagefright \
        libmedia \
        liblog \

LOCAL_MODULE := libstagefrighthw
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
