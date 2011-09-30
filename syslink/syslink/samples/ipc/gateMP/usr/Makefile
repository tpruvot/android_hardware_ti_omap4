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

PROJROOT=../../..

include $(PROJROOT)/make/start.mk

INCLUDE=-I $(PROJROOT)/../api/include -I $(PROJROOT)/inc
LDPATH=$(TARGETDIR)/lib $(TARGETDIR)/usr/lib
LDFLAGS = $(addprefix -L, $(LDPATH))

CFLAGS=-Wall -g -O2 $(INCLUDE) -finline-functions -D$(PROCFAMILY) $(LDFLAGS)
CFLAGS += -DSYSLINK_TRACE_ENABLE

LIBS =  -lipcutils -lipc -lprocmgr -lsysmgr -lsyslinknotify
MEMMGRLIBS = -ltimemmgr

all: gateMPApp.out

gateMPApp.out:
	$(CC) $(CFLAGS) -o gateMPApp.out GateMPApp.c $(LIBS) $(MEMMGRLIBS)

install: gateMPApp.out
	$(INSTALL) -D $< $(TARGETDIR)/syslink/$<
	$(STRIP) -s $(TARGETDIR)/syslink/$<
clean:
	\rm -f gateMPApp.out
