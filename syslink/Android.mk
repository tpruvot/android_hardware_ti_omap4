# only include if running on an omap4 platform & OMX is enabled
ifdef HARDWARE_OMX
ifeq ($(TARGET_BOARD_PLATFORM),omap4)
include $(call first-makefiles-under,$(call my-dir))
endif
endif
