#
#   Syslink-IPC for TI OMAP Processors
#
#   Copyright (c) 2008-2010, Texas Instruments Incorporated
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#   *  Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#   *  Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#   *  Neither the name of Texas Instruments Incorporated nor the names of
#      its contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

PROJROOT = ../../..

include $(PROJROOT)/api/make/start.mk

# Do not change above "include" line(s)

# Arguments to tools, will move to make system once finalized.
CFLAGS         = -DSYSLINK_USE_SYSMGR
#CFLAGS        =
CDEFS          = ARM_TARGET C60_TARGET
ifeq ($(BUILD),udeb)
CDEFS          += DEBUG 
endif

EXEC_ARGS      = 
ST_LIB_ARGS    = 
SH_LIB_ARGS    = 

# Define this macro if target runs in kernel mode
#__KERNEL__ = 1

# Target name and extension
# static library        (ST_LIB): filename.a
# shared library soname (SH_LIB): filename.so.maj_ver.min_ver
# executable            (EXEC)  : filename.out

TARGETNAME  = libprocmgr
#TARGETNAME  = libprocmgr_nonsysmgr


# TARGETTYPE must be EXEC, ST_LIB or SH_LIB in upper case.

TARGETTYPE  = SH_LIB

# install directory relative to the HOSTTARGET directory
HOSTRELEASE = lib

# install directory relative to the root filesystem
ROOTFSRELEASE = lib

# For shared object library, soname is filename.so.maj_ver

SH_SONAME = libprocmgr.so.2
#SH_SONAME = libprocmgr_nonsysmgr.so.2

# Folders in which gmake will run before building current target

SUBMODULES  = \
procmgr4430

# Filename must not begin with '.', '/' or '\'

SOURCES     = \
ProcMgr.c \
ProcMgrDrvUsr.c \
ProcMMU.c \
ProcDEH.c \
SysLinkMemUtils.c \
elfload/arm_dynamic.c \
elfload/c60_dynamic.c \
elfload/arm_reloc.c \
elfload/c60_reloc.c \
elfload/ArrayList.c \
elfload/dload4430.c \
elfload/dload.c \
elfload/dload_endian.c \
elfload/dlw_client.c \
elfload/dlw_debug.c \
elfload/dlw_dsbt.c \
elfload/dlw_trgmem.c \
elfload/elf32.c \
elfload/symtab.c


# Search path for include files

INCLUDES    = \
    $(PROJROOT)/api/include \
    $(PROJROOT)/api/inc \
    $(PROJROOT)/api/include/linux\
    $(TILER_INC_PATH)

# Libraries needed for linking.

#SH_LIBS        = pthread utils
SH_LIBS        = ipcutils sysmgr timemmgr
#SH_LIBS        = utils
ST_LIBS        = gcc

# Search path for library (and linker command) files.
# Current folder and target folder are included by default.

LIBINCLUDES += \
	  $(PROJROOT)/api/src/utils \
	  $(PROJROOT)/api/src/sysmgr
	
#LIBINCLUDES += \
#    $(PROJROOT)/api/src/utils

LIBINCLUDES += $(TILER_INC_PATH)/../ $(TILER_INC_PATH)

# Do not change below "include" line(s)

include $(PROJROOT)/api/make/build.mk
