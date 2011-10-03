ifdef BOARD_USES_TI_CAMERA_HAL

ifeq ($(TARGET_BOARD_PLATFORM),omap4)

################################################

#Camera HAL

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)


LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
    CameraHalM.cpp \
    CameraHal.cpp \
    CameraHalUtilClasses.cpp \
    AppCallbackNotifier.cpp \
    MemoryManager.cpp	\
    OverlayDisplayAdapter.cpp \
    CameraProperties.cpp \
    CameraKPI.cpp \
    TICameraParameters.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../inc \
    $(LOCAL_PATH)/../../libtiutils \
    $(LOCAL_PATH)/../../liboverlay \
    $(LOCAL_PATH)/../../../tiler \
    $(LOCAL_PATH)/../../../../../../external/libxml2/include \
    external/icu4c/common \
    $(LOCAL_PATH)/../inc/HDRInterface \
    vendor/arcsoft/HDRCapture/inc \
    hardware/ti/omap4/syslink/ipc-listener


LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libtimemmgr \
    libicuuc \
    libcamera_client \
    libsyslink_ipc_listener \

LOCAL_STATIC_LIBRARIES:= \
    libxml2 \


LOCAL_C_INCLUDES += \
        bionic/libc/include \
	frameworks/base/include/ui \
	hardware/ti/omap4/omap3/liboverlay \
	hardware/ti/omap4/omap3/libtiutils \
	frameworks/base/include/utils \

LOCAL_CFLAGS += -fno-short-enums -DCOPY_IMAGE_BUFFER -DTARGET_OMAP4 -mfpu=neon

LOCAL_MODULE:= libcamera
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)


################################################

#Fake Camera Adapter

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	BaseCameraAdapter.cpp \
	FakeCameraAdapter.cpp \

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../inc \
    $(LOCAL_PATH)/../../libtiutils \
    $(LOCAL_PATH)/../../liboverlay \
    $(LOCAL_PATH)/../../../../../../external/icu4c/common \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libcamera \
    libicuuc \
    libcamera_client \

LOCAL_C_INCLUDES += \
        bionic/libc/include \
	frameworks/base/include/ui \
	hardware/ti/omap4/omap3/liboverlay \
	hardware/ti/omap4/omap3/libtiutils \
	frameworks/base/include/utils \
	$(LOCAL_PATH)/../../../../../../external/libxml2/include \

LOCAL_CFLAGS += -fno-short-enums -DTARGET_OMAP4

LOCAL_MODULE:= libfakecameraadapter
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

################################################

#OMX Camera Adapter

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	BaseCameraAdapter.cpp \
	OMXCameraAdapter/OMXCap.cpp \
	OMXCameraAdapter/OMXCameraAdapter.cpp \


LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../inc/ \
    $(LOCAL_PATH)/../../libtiutils \
    $(LOCAL_PATH)/../../liboverlay \
    $(LOCAL_PATH)/../inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../../../../../../external/icu4c/common \
    $(LOCAL_PATH)/../inc/HDRInterface \
    vendor/arcsoft/HDRCapture/inc \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libOMX_CoreOsal \
    libOMX_Core \
    libsysmgr \
    librcm \
    libipc \
    libcamera \
    libicuuc \
    libcamera_client \
    libomx_rpc \
    libhardware_legacy \
    libhdr_interface

LOCAL_C_INCLUDES += \
    bionic/libc/include \
    frameworks/base/include/ui \
    hardware/ti/omap4/omap3/liboverlay \
    hardware/ti/omap4/omap3/libtiutils \
    frameworks/base/include/utils \
    hardware/ti/omap4/omx/ducati/domx/system/omx_core/inc \
    hardware/ti/omap4/omx/ducati/domx/system/mm_osal/inc \
    $(LOCAL_PATH)/../../../../../../external/libxml2/include \



LOCAL_CFLAGS += -fno-short-enums -DTARGET_OMAP4

LOCAL_MODULE:= libomxcameraadapter
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

################################################

#USB Camera Adapter

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	BaseCameraAdapter.cpp \
	V4LCameraAdapter/V4LCameraAdapter.cpp \


LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../inc/ \
    $(LOCAL_PATH)/../inc/V4LCameraAdapter \
    external/icu4c/common \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libsysmgr \
    librcm \
    libipc \
    libcamera \
    libicuuc \
    libcamera_client \
    libomx_rpc \

LOCAL_C_INCLUDES += \
    kernel/android-2.6.29/include \
    frameworks/base/include/ui \
    hardware/ti/omap4/omap3/liboverlay \
    hardware/ti/omap4/omap3/libtiutils \
    frameworks/base/include/utils \
    hardware/ti/omap4/omx/ducati/domx/system/omx_core/inc \
    hardware/ti/omap4/omx/ducati/domx/system/mm_osal/inc \
    $(LOCAL_PATH)/../../../../../../external/libxml2/include \



LOCAL_CFLAGS += -fno-short-enums -DTARGET_OMAP4

LOCAL_MODULE:= libusbcameraadapter
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
######################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= CameraCalibration/CameraCalibration.cpp

LOCAL_C_INCLUDES += \
    bionic/libc/include \
    $(LOCAL_PATH)/../inc/ \
    $(LOCAL_PATH)/../inc/OMXCameraAdapter \
    external/icu4c/common \
    $(LOCAL_PATH)/../../../tiler/memmgr \
    $(LOCAL_PATH)/../../libtiutils \
    $(LOCAL_PATH)/../../liboverlay \
    frameworks/base/include/ui \
    frameworks/base/include/utils \
    hardware/ti/omap4/omx/ducati/domx/system/omx_core/inc \
    hardware/ti/omap4/omx/ducati/domx/system/mm_osal/inc \
    external/libxml2/include \
    $(LOCAL_PATH)/../inc/HDRInterface \
    vendor/arcsoft/HDRCapture/inc

LOCAL_SHARED_LIBRARIES:= \
    libomxcameraadapter \
    libcamera_client \
    libcamera \
    liblog \
    libtiutils \
    libutils \

LOCAL_MODULE:= CameraCal
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_BIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_BIN_UNSTRIPPED)

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    LOCAL_CFLAGS += -DTARGET_OMAP4
endif

include $(BUILD_EXECUTABLE)

# [HASHCODE] FIXME: Disabled due to /vendor/arcsoft reference.  Copying phone file.
#include $(LOCAL_PATH)/HDRInterface/Android.mk

endif  # remove calibration for now 

endif
