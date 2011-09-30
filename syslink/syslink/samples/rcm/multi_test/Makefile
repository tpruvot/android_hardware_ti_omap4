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
#   @brief  Makefile for user-side RCM Multi Test samples
#
#   ============================================================================

PROJROOT = ../..

include $(PROJROOT)/make/start.mk

INCLUDE=-I $(PROJROOT)/../api/include -I $(PROJROOT)/inc
LDPATH=$(TARGETDIR)/lib $(TARGETDIR)/usr/lib
LDFLAGS = $(addprefix -L, $(LDPATH))

CFLAGS=-Wall -g -O2 $(INCLUDE) -finline-functions -D$(PROCFAMILY) $(LDFLAGS)
CFLAGS += -DSYSLINK_USE_LOADER


LIBS = -lipcutils -lsyslinknotify -lipc -lprocmgr -lrcm -lpthread -lrt
LIBS += -lsysmgr
MEMMGRLIBS = -ltimemmgr

all: rcm_multitest.out rcm_multithreadtest.out rcm_multiclienttest.out rcm_daemontest.out

rcm_multitest.out:
	$(CC) $(CFLAGS) -o rcm_multitest.out RcmClientServerTest.c $(LIBS) $(MEMMGRLIBS)

rcm_multithreadtest.out:
	$(CC) $(CFLAGS) -o rcm_multithreadtest.out RcmMultiThreadTest.c $(LIBS) $(MEMMGRLIBS)

rcm_multiclienttest.out:
	$(CC) $(CFLAGS) -o rcm_multiclienttest.out RcmMultiClientTest.c $(LIBS) $(MEMMGRLIBS)

rcm_daemontest.out:
	$(CC) $(CFLAGS) -DSYSLINK_USE_DAEMON -o rcm_daemontest.out RcmMultiThreadTest.c $(LIBS) $(MEMMGRLIBS)

install1: rcm_multitest.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<

install2: rcm_multithreadtest.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<

install3: rcm_multiclienttest.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<

install4: rcm_daemontest.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<

install: install1 install2 install3 install4

clean:
	\rm -f rcm_multitest.out
	\rm -f rcm_multithreadtest.out
	\rm -f rcm_multiclienttest.out
	\rm -f rcm_daemontest.out
