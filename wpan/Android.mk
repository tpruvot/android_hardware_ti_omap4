LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    SUBDIRS := fmradio \
               ti_st
    include $(call all-named-subdir-makefiles, $(SUBDIRS))
endif
