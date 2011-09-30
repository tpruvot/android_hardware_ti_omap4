#
#  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

#   ============================================================================
#   @file   Makefile.am
#
#   @brief  Makefile for user-side PmGeneric sample
#
#   ============================================================================

PROJROOT=$(top_srcdir)/samples
INCLUDE = \
	-I $(PROJROOT)/../api/include \
	-I $(PROJROOT)/inc


AM_CFLAGS = -Wall -g -O2 $(INCLUDE)-finline-functions -D$(PROCFAMILY)
AM_CFLAGS += -DSYSLINK_TRACE_ENABLE
#TO disable loader comment the below flag.
AM_CFLAGS += -DSYSLINK_USE_LOADER

LDPATH=../../../api/src

API_LIBS = \
	$(LDPATH)/utils/libipcutils.la \
	$(LDPATH)/notify/libsyslinknotify.la \
	$(LDPATH)/ipc/libipc.la \
	$(LDPATH)/procmgr/libprocmgr.la \
	$(LDPATH)/sysmgr/libsysmgr.la


bin_PROGRAMS = slpmtest.out

slpmtest_out_SOURCES = \
	slpmtestOS.c \
	slpmtestApp.c

slpmtest_out_CPPFLAGS = $(AM_CFLAGS)

slpmtest_out_LDADD = $(API_LIBS)
