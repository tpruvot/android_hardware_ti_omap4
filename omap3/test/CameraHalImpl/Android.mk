# TODO: comment out camerahal test  -  TI gingerbread branch does not have this folder.  Resolve this when merging TI # branch 
ifdef 0
ifdef BOARD_USES_TI_CAMERA_HAL

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	camera_impl_test.cpp

LOCAL_SHARED_LIBRARIES:= \
	libdl \
	libui \
	libutils \
	libcutils \
	libbinder \
	libmedia \
	libcamera \

# CameraHal and CameraHalImpl are here: /data/Motorola/repoAndroid/hardware/ti/omap3/camera-omap4/src/

LOCAL_C_INCLUDES += \
        bionic/libc/include \
	frameworks/base/include/ui \
	frameworks/base/include/utils \
	frameworks/base/include/media \
	hardware/ti/omap4/omap3/camera-omap4/inc \
	hardware/ti/omap4/omap3/liboverlay \
	hardware/ti/omap4/omap3/libtiutils \
	hardware/ti/omap4/tiler/memmgr \
	external/libxml2/include \
	external/icu4c/common \
	$(PV_INCLUDES)

LOCAL_MODULE:= camera_impl_test

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

include $(BUILD_EXECUTABLE)

endif # BOARD_USES_TI_CAMERA_HAL

endif  #  Comment this makefile until TI gingerbread merge