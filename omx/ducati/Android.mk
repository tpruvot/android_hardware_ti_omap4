
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TI_DOMX_TOP := hardware/ti/omap4/omx/ducati/domx

TI_OMX_CFLAGS := -Wall -fpic -pipe -DSTATIC_TABLE -O0

#call build domx infrastructure
include $(TI_DOMX_TOP)/system/mm_osal/src/Android.mk
include $(TI_DOMX_TOP)/system/omx_core/src/Android.mk

include $(TI_DOMX_TOP)/system/domx/omx_proxy_common/src/Android.mk
include $(TI_DOMX_TOP)/system/domx/omx_rpc/src/Android.mk

#call build proxy
include $(TI_DOMX_TOP)/video/omx_proxy_component/src/Android.mk


