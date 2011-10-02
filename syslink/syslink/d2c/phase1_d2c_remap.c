/*
 *  phase1_d2c_remap.c
 *
 *  Ducati to Chiron Tiler block remap functions for TI OMAP processors.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
 *  All rights reserved.
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

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tiler.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#define __DEBUG__
#define __DEBUG_ASSERT__
#define  __DEBUG0__
#define __DEBUG_ENTRY__

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif
#include "mem_types.h"
#include "utils.h"
#include "debug_utils.h"
#include "list_utils.h"
#include "tilermem_utils.h"
#include "phase1_d2c_remap.h"
#include "memmgr.h"
#include "tiler.h"

/* ----- START debug only methods ----- */

#ifdef __DEBUG0__
static bytes_t def_bpp(pixel_fmt_t pixelFormat);
static void __dump_block(struct tiler_block_info *blk, char *prefix, char *suffix)
{
    switch (blk->fmt)
    {
    case TILFMT_PAGE:
        P("%s [p=%p(0x%x),l=0x%x,s=%d]%s", prefix, blk->ptr, blk->ssptr,
          blk->dim.len, blk->stride, suffix);
        break;
    case TILFMT_8BIT:
    case TILFMT_16BIT:
    case TILFMT_32BIT:
        P("%s [p=%p(0x%x),%d*%d*%d,s=%d]%s", prefix, blk->ptr, blk->ssptr,
          blk->dim.area.width, blk->dim.area.height, def_bpp(blk->fmt) * 8,
          blk->stride, suffix);
        break;
    default:
        P("%s*[p=%p(0x%x),l=0x%x,s=%d,fmt=0x%x]%s", prefix, blk->ptr,
          blk->ssptr, blk->dim.len, blk->stride, blk->fmt, suffix);
    }
}

static void __dump_buf(struct tiler_buf_info* buf, char* prefix)
{
    P("%sbuf={n=%d,id=0x%x,", prefix, buf->num_blocks, buf->offset);
    int ix = 0;
    for (ix = 0; ix < buf->num_blocks; ix++)
    {
        __dump_block(buf->blocks + ix, "", ix + 1 == buf->num_blocks ? "}" : "");
    }
}
#define P0 P
#define DP0 DP
#else
#define P0(fmt, ...)
#define DP0(fmt, ...)
#define __dump_block(blk, ...)
#define __dump_buf(buf, ...)
#endif

/* ----- END debug only methods ----- */

#ifdef STUB_SYSLINK
static int SysLinkMemUtils_virtToPhys(uint32_t remoteAddr, uint32_t *physAddr,
                                      int procId)
{
    P0("%p=%d", physAddr, *physAddr);
    *physAddr = remoteAddr;
    P0("%p=%d", physAddr, *physAddr);
    return 1;
}
#define PROC_APPM3 3
#else
#include <SysLinkMemUtils.h>
#endif

struct _ReMapData {
    void     *bufPtr;
    uint32_t  tiler_id;
    struct _ReMapList {
        struct _ReMapList *next, *last;
        struct _ReMapData *me;
    } link;
};
static struct _ReMapList bufs;
static int bufs_inited = 0;
static pthread_mutex_t che_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Returns the bytes per pixel for the pixel format.
 *
 * @author a0194118 (9/4/2009)
 *
 * @param pixelFormat   Pixelformat
 *
 * @return Bytes per pixel
 */
static bytes_t def_bpp(pixel_fmt_t pixelFormat)
{
    return (pixelFormat == PIXEL_FMT_32BIT ? 4 :
            pixelFormat == PIXEL_FMT_16BIT ? 2 : 1);
}

/**
 * Returns the default page stride for this block
 *
 * @author a0194118 (9/4/2009)
 *
 * @param width  Width of 2D container
 *
 * @return Stride
 */
static bytes_t def_stride(pixels_t width)
{
    return (PAGE_SIZE - 1 + (bytes_t)width) & ~(PAGE_SIZE - 1);
}

/**
 * Initializes the static structures
 *
 * @author a0194118 (9/8/2009)
 */
static void init()
{
    if (!bufs_inited)
    {
        DLIST_INIT(bufs);
        bufs_inited = 1;
    }
}

/**
 * Records a buffer-pointer -- tiler-ID mapping.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param bufPtr    Buffer pointer
 * @param tiler_id  Tiler ID
 *
 * @return 0 on success, -ENOMEM on memory allocation error
 */
static int remap_cache_add(void *bufPtr, uint32_t tiler_id)
{
    pthread_mutex_lock(&che_mutex);
    init();
    struct _ReMapData *ad = NEW(struct _ReMapData);
    if (ad)
    {
	    ad->bufPtr = bufPtr;
	    ad->tiler_id = tiler_id;
	    DLIST_MADD_BEFORE(bufs, ad, link);
    }
    pthread_mutex_unlock(&che_mutex);
    return ad == NULL ? -ENOMEM : 0;
}

/**
 * Retrieves the tiler ID for given buffer pointer from the
 * records. If the tiler ID is found, it is removed from the
 * records as well.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param bufPtr    Buffer pointer
 *
 * @return Tiler ID on success, 0 on failure.
 */
static uint32_t remap_cache_del(void *bufPtr)
{
    struct _ReMapData *ad;
    pthread_mutex_lock(&che_mutex);
    init();
    DLIST_MLOOP(bufs, ad, link) {
        if (ad->bufPtr == bufPtr) {
            uint32_t tiler_id = ad->tiler_id;
            DLIST_REMOVE(ad->link);
            FREE(ad);
            pthread_mutex_unlock(&che_mutex);
            return tiler_id;
        }
    }
    pthread_mutex_unlock(&che_mutex);
    return 0;
}

/**
 * Returns the size of the supplied block
 *
 * @author a0194118 (9/4/2009)
 *
 * @param blk    Pointer to the tiler_block_info struct
 *
 * @return size of the block in bytes
 */
static bytes_t def_size(struct tiler_block_info *blk)
{
    return (blk->fmt == PIXEL_FMT_PAGE ?
            blk->dim.len :
            blk->dim.area.height * def_stride(blk->dim.area.width * def_bpp(blk->fmt)));
}

void *tiler_assisted_phase1_D2CReMap(int num_blocks, DSPtr dsptrs[],
                                     bytes_t lengths[])
{
    IN;
    void *bufPtr = NULL;

#ifndef STUB_TILER
    /* we can only remap up to the TILER supported number of blocks */
    if (NOT_I(num_blocks,>,0) || NOT_I(num_blocks,<=,TILER_MAX_NUM_BLOCKS))
        return R_P(NULL);

    struct tiler_buf_info buf;
    ZERO(buf);
    buf.num_blocks = num_blocks;
    int ix, res;
    bytes_t size = 0;

    /* need tiler driver */
    int td = open("/dev/tiler", O_RDWR | O_SYNC);
    if (NOT_I(td,>=,0)) return R_P(NULL);

    /* for each block */
    for (ix = 0; ix < num_blocks; ix++)
    {
        /* check the length of each block */
        if (NOT_I(lengths[ix] & (PAGE_SIZE - 1),==,0)) goto FAILURE;

        /* convert DSPtrs to SSPtrs using SysLink */
        uint32_t ssptr = 0;
        if (NOT_I(SysLinkMemUtils_virtToPhys(dsptrs[ix], &ssptr, PROC_APPM3),>,0))
            goto FAILURE;

        __dump_block(buf.blocks + ix, "<=v2s==", "");
        buf.blocks[ix].ssptr = ssptr;
        if (NOT_P(ssptr,!=,0)) {
            P("for dsptrs[%d]=0x%x", ix, dsptrs[ix]);
            goto FAILURE;
        }

        /* query tiler driver for details on these blocks, such as
           width/height/len/fmt */
        __dump_block(buf.blocks + ix, "=(qb)=>", "");
        res = ioctl(td, TILIOC_QBLK, buf.blocks + ix);
        __dump_block(buf.blocks + ix, "<=(qb)=", "");

        if (NOT_I(res,==,0) || NOT_I(buf.blocks[ix].ssptr,!=,0))
        {
            P("tiler did not allocate dsptr[%d]=0x%x ssptr=0x%x", ix, dsptrs[ix], ssptr);
            goto FAILURE;
        }

        /* :TODO: for now we fix width to have 4K stride, and get length and
           height from the passed-in length parameters.  This is because
           tiler driver does not store any of this information. */
        if (buf.blocks[ix].fmt == TILFMT_PAGE)
        {
            buf.blocks[ix].dim.len = lengths[ix];
        }
        else
        {
            /* :TRICKY: get number of horizontal pages in the 2d area from the
               length, then get the height of the buffer.  The width of the
               buffer will always be rounded up to the 8-bit stride (64 tiled
               pages), and the exact width is not tracked by tiler. */

            /* get the minimum and maximum allocation size for a 1-page
               virtual width buffer, and use this and the size of a block to
               establish limits on the stride */
            bytes_t max_alloc_size = (bytes_t)buf.blocks[ix].dim.area.height * PAGE_SIZE;
            bytes_t min_alloc_size = max_alloc_size - (buf.blocks[ix].fmt == TILFMT_8BIT ? 63 : 31) * PAGE_SIZE;
            int min_page_width = (lengths[ix] + max_alloc_size - 1) / max_alloc_size;
            int max_page_width = lengths[ix] / min_alloc_size;

            /* use tiler-returned stride as the another max, but note that
               this is calculated based on the allocated length.

               :TODO: this should really not be calculated by the driver as
               it does not have this information. */
            int limit = buf.blocks[ix].stride / PAGE_SIZE;
            if (max_page_width > limit)
            {
                P0("lowering max_page_width from %d to %d", max_page_width,
                   limit);
                max_page_width = limit;
            }

            /* use the width of the tiler allocation (tiler-returned stride)
               to establish a minimum stride that would justify such an allocation. */
            limit -= buf.blocks[ix].fmt == TILFMT_8BIT ? 0 : 1;
            if (min_page_width < limit)
            {
                P0("raising min_page_width from %d to %d", min_page_width,
                   limit);
                min_page_width = limit;
            }
            CHK_I(min_page_width,<=,max_page_width);

            /* it is possible that there are more solutions?  Give warning */
            if (min_page_width != max_page_width)
            {
                DP("WARNING: cannot resolve stride (%d-%d). Choosing the smaller.",
                   min_page_width * PAGE_SIZE, max_page_width * PAGE_SIZE);
            }
            buf.blocks[ix].dim.area.height = lengths[ix] / PAGE_SIZE / min_page_width;
            buf.blocks[ix].stride = PAGE_SIZE * min_page_width;
            buf.blocks[ix].dim.area.width = buf.blocks[ix].stride / def_bpp(buf.blocks[ix].fmt);
        }
        CHK_I(def_size(buf.blocks + ix),==,lengths[ix]);

        /* add up size of buffer after remap */
        size += def_size(buf.blocks + ix);
    }

    /* register this buffer and/or query last registration */
    __dump_buf(&buf, "==(RBUF)=>");
    res = ioctl(td, TILIOC_RBUF, &buf);
    __dump_buf(&buf, "<=(RBUF)==");
    if (NOT_I(res,==,0) || NOT_P(buf.offset,!=,0)) goto FAILURE;

    /* map blocks to process space */
    bufPtr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        td, buf.offset);
    if (bufPtr == MAP_FAILED){
        bufPtr = NULL;
    } else {
        bufPtr += buf.blocks[0].ssptr & (PAGE_SIZE - 1);
    }
    DP0("ptr=%p", bufPtr);

    /* if failed to map: unregister buffer */
    if (NOT_P(bufPtr,!=,NULL) ||
	/* or failed to cache tiler ID for buffer */
	NOT_I(remap_cache_add(bufPtr, buf.offset),==,0))
    {
        A_I(ioctl(td, TILIOC_URBUF, &buf),==,0);
    }
    /* otherwise, fill out pointers */
    else
    {
        /* fill out pointers */
        for (size = ix = 0; ix < num_blocks; ix++)
        {
            buf.blocks[ix].ptr = bufPtr + size;
            /* P("   [0x%p]", blks[ix].ptr); */
            size += def_size(buf.blocks + ix);
        }
    }

FAILURE:
    close(td);
#endif
    return R_P(bufPtr);
}

int tiler_assisted_phase1_DeMap(void *bufPtr)
{
    IN;

    int ret = REMAP_ERR_GENERIC;
#ifndef STUB_TILER
    struct tiler_buf_info buf;
    ZERO(buf);
    /* need tiler driver */
    int ix, td = open("/dev/tiler", O_RDWR | O_SYNC);
    if (NOT_I(td,>=,0)) return R_I(ret);

    /* retrieve registered buffers from vsptr */
    /* :NOTE: if this succeeds, Memory Allocator stops tracking this buffer */
    buf.offset = remap_cache_del(bufPtr);

    if (A_L(buf.offset,!=,0))
    {
        /* get block information for the buffer */
        __dump_buf(&buf, "==(QBUF)=>");
        ret = A_I(ioctl(td, TILIOC_QBUF, &buf),==,0);
        __dump_buf(&buf, "<=(QBUF)==");

        /* unregister buffer, and free tiler chunks even if there is an
           error */
        if (!ret)
        {
            __dump_buf(&buf, "==(URBUF)=>");
            ret = A_I(ioctl(td, TILIOC_URBUF, &buf),==,0);
            __dump_buf(&buf, "<=(URBUF)==");

            /* unmap buffer */
            bytes_t size = 0;
            for (ix = 0; ix < buf.num_blocks; ix++)
            {
                size += def_size(buf.blocks + ix);
            }
            bufPtr = (void *)ROUND_DOWN_TO2POW((uint32_t)bufPtr, PAGE_SIZE);
            ERR_ADD(ret, munmap(bufPtr, size));
        }
    }

    close(td);
#endif
    return R_I(ret);
}
