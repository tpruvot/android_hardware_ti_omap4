LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
        tiomxplayer.c \
        tiomxplayerutils.c \
        speechutils.c \
        audioutils.c \
        commlistenerthrd.c

LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMX_DEBUG         -fPIC -D_POSIX_SOURCE \
        -DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
        -DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
        -DALSA_DEVICE_DIRECTORY=\"/dev/snd/\"

LOCAL_C_INCLUDES := \
        $(TI_OMX_SYSTEM)/common/inc \
        $(TI_OMX_COMP_C_INCLUDES) \
        external/alsa-lib/include

LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
                        libaudio \
                        libasound \
                        libOMX_Core \
                        libc

LOCAL_MODULE:= tiomxplayer
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
