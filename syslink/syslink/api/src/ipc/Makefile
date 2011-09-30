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

CFLAGS         = 
CDEFS          = 
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

TARGETNAME  = libipc


# TARGETTYPE must be EXEC, ST_LIB or SH_LIB in upper case.

TARGETTYPE  = SH_LIB

# install directory relative to the HOSTTARGET directory
HOSTRELEASE = lib

# install directory relative to the root filesystem
ROOTFSRELEASE = lib

# For shared object library, soname is filename.so.maj_ver

SH_SONAME = libipc.so.0

# Folders in which gmake will run before building current target

SUBMODULES  = \

# Filename must not begin with '.', '/' or '\'

SOURCES     = \
IPCManager.c \
SharedRegionDrv.c \
SharedRegion.c\
HeapMemMPDrv.c \
HeapMemMP.c \
GateMPDrv.c \
GateMP.c \
HeapBufMPDrv.c \
HeapBufMP.c \
NameServerDrv.c \
NameServer.c \
ListMPDrv.c \
ListMP.c \
MessageQDrv.c \
MessageQ.c \
MultiProcDrv.c \
MultiProc.c

# Search path for include files

INCLUDES    = \
    $(PROJROOT)/api/include \

# Libraries needed for linking.

ST_LIBS        =
SH_LIBS        = rt pthread

# Search path for library (and linker command) files.
# Current folder and target folder are included by default.

LIBINCLUDES =


# Do not change below "include" line(s)

include $(PROJROOT)/api/make/build.mk
