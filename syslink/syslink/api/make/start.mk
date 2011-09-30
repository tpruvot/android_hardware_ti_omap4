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

# make sure we have a prefix
ifndef PREFIX
$(error Error: variable PREFIX not defined)
endif

CMDDEFS =
CMDDEFS_START =


CROSS=arm-none-linux-gnueabi-
PROCFAMILY=OMAP_4430


ifndef PROCFAMILY
$(error Error: PROCFAMILY can not be determined from Kernel .config)
endif

ifndef TARGETDIR
TARGETDIR=$(PREFIX)/target
endif



#default (first) target should be "all"
#make sure the target directories are created
#all: $(HOSTDIR) $(ROOTFSDIR) $(TARGETDIR)
all: $(TARGETDIR)

CONFIG_SHELL := /bin/bash

SHELL := $(CONFIG_SHELL)

# Current version of gmake (3.79.1) cannot run windows shell's internal commands
# We need to invoke command interpreter explicitly to do so.
# for winnt it is cmd /c <command>
SHELLCMD:=

ifneq ($(SHELL),$(CONFIG_SHELL))
CHECKSHELL:=SHELLERR
else
CHECKSHELL:=
endif

# Error string to generate fatal error and abort gmake
ERR = $(error Makefile generated fatal error while building target "$@")

CP  :=   cp

MAKEFLAGS = r

QUIET := &> /dev/null

# Should never be :=
RM    = rm $(1) 
MV    = mv $(1) $(2)
RMDIR = rm -r $(1)
MKDIR = mkdir -p $(1)
INSTALL = install

# Current Makefile directory
MAKEDIR := $(CURDIR)

# Implicit rule search not needed for *.d, *.c, *.h
%.d:
%.c:
%.h:

#   Tools
CC	:= $(CROSS)gcc
AR	:= $(CROSS)ar
LD	:= $(CROSS)ld
STRIP	:= $(CROSS)strip

