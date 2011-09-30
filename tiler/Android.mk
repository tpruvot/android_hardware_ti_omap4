# only include if running on an omap4 platform
ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := memmgr.c tilermgr.c
LOCAL_MODULE := libtimemmgr
LOCAL_MODULE_TAGS := optional tests
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := memmgr_test.c testlib.c
LOCAL_SHARED_LIBRARIES := libtimemmgr
LOCAL_MODULE := memmgr_test
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := utils_test.c testlib.c
LOCAL_SHARED_LIBRARIES := libtimemmgr
LOCAL_MODULE := utils_test
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := tiler_ptest.c
LOCAL_SHARED_LIBRARIES := libtimemmgr
LOCAL_MODULE    := tiler_ptest
LOCAL_MODULE_TAGS:= tests
include $(BUILD_EXECUTABLE)

endif
