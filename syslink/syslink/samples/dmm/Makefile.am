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
#

PROJROOT=$(top_srcdir)/samples
INCLUDE = \
	-I $(PROJROOT)/../api/include \
	-I $(PROJROOT)/inc \
	$(MEMMGR_CFLAGS)


AM_CFLAGS = -Wall -g -O2 $(INCLUDE)

LDPATH=../../api/src

API_LIBS = \
	$(LDPATH)/utils/libipcutils.la \
	$(LDPATH)/notify/libsyslinknotify.la \
	$(LDPATH)/ipc/libipc.la \
	$(LDPATH)/procmgr/libprocmgr.la \
	$(LDPATH)/sysmgr/libsysmgr.la \
	$(LDPATH)/rcm/librcm.la \
	$(MEMMGR_LIBS)


bin_PROGRAMS =  dmm_daemontest.out

dmm_daemontest_out_SOURCES = dmmtestapp.c

dmm_daemontest_out_CPPFLAGS = $(AM_CFLAGS)

dmm_daemontest_out_LDADD = $(API_LIBS)
