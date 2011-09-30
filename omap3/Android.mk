LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    SUBDIRS := camera-omap3 \
               camera-omap4/src \
               camera-omap4/data \
               dspbridge \
               gsm0710muxd \
               liblights \
               libopencorehw \
               liboverlay \
               libskiahw-omap3 \
               libskiahw-omap4 \
               libstagefrighthw \
               libtiutils \
               test
    include $(call all-named-subdir-makefiles, $(SUBDIRS))
endif
