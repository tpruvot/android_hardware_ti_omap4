/*
 *  tilermgr.c
 *
 *  TILER library support functions for TI OMAP processors.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>   /* strerror() */
#include <fcntl.h>    /* open() */
#include <unistd.h>   /* close() */
#include <errno.h>
#include <sys/ioctl.h>
#include <tiler.h>
#include "tilermgr.h"
#include "mem_types.h"


#define TILERMGR_ERROR() \
	fprintf(stderr, "%s()::%d: errno(%d) - \"%s\"\n", \
			__FUNCTION__, __LINE__, errno, strerror(errno)); \
	fflush(stderr);

static int fd;

int TilerMgr_Close()
{
    close(fd);
    return TILERMGR_ERR_NONE;
}

int TilerMgr_Open()
{
    fd = open(TILER_DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        TILERMGR_ERROR();
        return TILERMGR_ERR_GENERIC;
    }

    return TILERMGR_ERR_NONE;
}

SSPtr TilerMgr_Alloc(enum pixel_fmt_t pixfmt, pixels_t width, pixels_t height)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if (pixfmt < PIXEL_FMT_8BIT || pixfmt > PIXEL_FMT_32BIT)
        return 0x0;
    if (width <= 0 || width > TILER_WIDTH * 64)
        return 0x0;
    if (height <= 0 || height > TILER_HEIGHT * 64)
        return 0x0;

    block.fmt = pixfmt;
    block.dim.area.width = width;
    block.dim.area.height = height;

    ret = ioctl(fd, TILIOC_GBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return 0x0;
    }
    return block.ssptr;
}

int TilerMgr_Free(SSPtr addr)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if (addr < TILER_MEM_8BIT || addr >= TILER_MEM_PAGED)
        return TILERMGR_ERR_GENERIC;

    block.ssptr = addr;

    ret = ioctl(fd, TILIOC_FBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return TILERMGR_ERR_GENERIC;
    }
    return TILERMGR_ERR_NONE;
}

SSPtr TilerMgr_PageModeAlloc(bytes_t len)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if(len < 0 || len > TILER_LENGTH)
        return 0x0;

    block.fmt = TILFMT_PAGE;
    block.dim.len = len;

    ret = ioctl(fd, TILIOC_GBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return 0x0;
    }
    return block.ssptr;
}

int TilerMgr_PageModeFree(SSPtr addr)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if (addr < TILER_MEM_PAGED || addr >= TILER_MEM_END)
        return TILERMGR_ERR_GENERIC;

    block.ssptr = addr;

    ret = ioctl(fd, TILIOC_FBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return TILERMGR_ERR_GENERIC;
    }
    return TILERMGR_ERR_NONE;
}

SSPtr TilerMgr_VirtToPhys(void *ptr)
{
    int ret = -1;
    unsigned long tmp = 0x0;

    if(ptr == NULL)
        return 0x0;

    tmp = (unsigned long)ptr;
    ret = ioctl(fd, TILIOC_GSSP, tmp);

    return (SSPtr)ret;
}

SSPtr TilerMgr_Map(void *ptr, bytes_t len)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if (len < 0 || len > TILER_LENGTH)
        return 0x0;

    block.fmt = TILFMT_PAGE;
    block.dim.len = len;
    block.ptr = ptr;

    ret = ioctl(fd, TILIOC_MBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return 0x0;
    }
    return block.ssptr;
}

int TilerMgr_Unmap(SSPtr addr)
{
    int ret = -1;
    struct tiler_block_info block = {0};

    if (addr < TILER_MEM_PAGED || addr >= TILER_MEM_END)
        return TILERMGR_ERR_GENERIC;

    block.ssptr = addr;

    ret = ioctl(fd, TILIOC_UMBLK, (unsigned long)(&block));
    if (ret < 0) {
        TILERMGR_ERROR();
        return TILERMGR_ERR_GENERIC;
    }
    return TILERMGR_ERR_NONE;
}

