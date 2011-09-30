#
#  Makefile.am
#
#  TI OMAP Processor Memory Allocator Interface functions makefile.
#
#  Copyright (C) 2009-2011 Texas Instruments, Inc.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  *  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  *  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  *  Neither the name of Texas Instruments Incorporated nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

AUTOMAKE_OPTIONS = foreign

## sources

h_sources = memmgr.h tilermem.h mem_types.h tiler.h tilermem_utils.h
if STUB_TILER
c_sources = memmgr.c
else
c_sources = memmgr.c tilermgr.c
endif

if TILERMGR
library_include_HEADERS = $(h_sources) tilermgr.h
else
library_include_HEADERS = $(h_sources)
noinst_HEADERS = tilermgr.h
endif

#Install the headers in library directory - e.g. examplelib-1.0:
library_includedir=$(includedir)/timemmgr

# library sources
lib_LTLIBRARIES= libtimemmgr.la
libtimemmgr_la_SOURCES = $(h_sources) $(c_sources)
libtimemmgr_la_CFLAGS  = $(MEMMGR_CFLAGS) -fpic -ansi
libtimemmgr_la_LIBTOOLFLAGS = --tag=disable-static
libtimemmgr_la_LDFLAGS = -version-info 2:0:0

if UNIT_TESTS
bin_PROGRAMS = utils_test memmgr_test tiler_ptest

utils_testdir = .
utils_test_SOURCES = utils_test.c testlib.c

memmgr_testdir = .
memmgr_test_SOURCES = memmgr_test.c testlib.c
memmgr_test_LDADD = libtimemmgr.la

tiler_ptest_SOURCES = tiler_ptest.c
tiler_ptest_LDADD = libtimemmgr.la
endif

pkgconfig_DATA = libtimemmgr.pc
pkgconfigdir   = $(libdir)/pkgconfig

