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
	-I $(PROJROOT)/inc


AM_CFLAGS = -Wall -g -O2 $(INCLUDE)
AM_CFLAGS += -DSYSLINK_USE_LOADER
#TODO: Add conditional flags to enable debug here

LDPATH=../../../api/src

API_LIBS = \
	$(LDPATH)/utils/libipcutils.la \
	$(LDPATH)/notify/libsyslinknotify.la \
	$(LDPATH)/ipc/libipc.la \
	$(LDPATH)/procmgr/libprocmgr.la \
	$(LDPATH)/sysmgr/libsysmgr.la \
	$(LDPATH)/rcm/librcm.la


bin_PROGRAMS =  rcm_multitest.out rcm_multithreadtest.out rcm_multiclienttest.out rcm_daemontest.out

rcm_multitest_out_SOURCES = RcmClientServerTest.c
rcm_multithreadtest_out_SOURCES = RcmMultiThreadTest.c
rcm_multiclienttest_out_SOURCES = RcmMultiClientTest.c
rcm_daemontest_out_SOURCES = RcmMultiThreadTest.c

rcm_multitest_out_CPPFLAGS = $(AM_CFLAGS)
rcm_multithreadtest_out_CPPFLAGS = $(AM_CFLAGS)
rcm_multiclienttest_out_CPPFLAGS = $(AM_CFLAGS)
rcm_daemontest_out_CPPFLAGS = $(AM_CFLAGS) -DSYSLINK_USE_DAEMON

rcm_multitest_out_LDADD = $(API_LIBS)
rcm_multithreadtest_out_LDADD = $(API_LIBS)
rcm_multiclienttest_out_LDADD = $(API_LIBS)
rcm_daemontest_out_LDADD = $(API_LIBS)
