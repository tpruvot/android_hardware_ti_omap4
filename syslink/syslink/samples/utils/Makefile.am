#
#  Copyright 2001-2010 Texas Instruments - http://www.ti.com/
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
#TODO: Add conditional flags to enable debug here

LDPATH=../../../api/src

API_LIBS = \
	$(LDPATH)/utils/libutils.la

bin_PROGRAMS =  utilsApp.out

utilsApp_out_SOURCES = \
	utilsApp.c \
	GateTest.c \
	MemoryTest.c \
	ListTest.c

utilsApp_out_CPPFLAGS = $(AM_CFLAGS)

utilsApp_out_LDADD = $(API_LIBS)
