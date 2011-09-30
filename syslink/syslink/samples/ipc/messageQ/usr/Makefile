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
#   @file   Makefile
#
#   @brief  Makefile for user-side MessageQ samples
#
#   ============================================================================

PROJROOT=../../..

include $(PROJROOT)/make/start.mk

INCLUDE=-I $(PROJROOT)/../api/include -I $(PROJROOT)/inc
LDPATH=$(TARGETDIR)/lib $(TARGETDIR)/usr/lib
LDFLAGS = $(addprefix -L, $(LDPATH))

CFLAGS=-Wall -g -O2 $(INCLUDE) -finline-functions -D$(PROCFAMILY) $(LDFLAGS)
CFLAGS += -DSYSLINK_TRACE_ENABLE
CFLAGS += -DSYSLINK_USE_LOADER


LIBS = -lipcutils -lsyslinknotify -lipc -lprocmgr
LIBS += -lsysmgr
MEMMGRLIBS = -ltimemmgr

all: messageQApp.out

messageQApp.out:
	$(CC) $(CFLAGS) -o messageQApp.out MessageQAppOS.c MessageQApp.c $(LIBS) $(MEMMGRLIBS)

messageQinstall1: messageQApp.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<

install: messageQinstall1

clean:
	\rm -f messageQApp.out
