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

#define __DEBUG__
#define __DEBUG_ASSERT__

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif
#include <memmgr.h>
#include <phase1_d2c_remap.h>
#include <utils.h>
#include <debug_utils.h>
#include <tilermem_utils.h>
#include <tilermem.h>
#include <tiler.h>
#include <testlib.h>

#define TESTS\
    T(test_d2c(176, 144, 1))\
    T(test_d2c(176, 144, 2))\
    T(test_d2c(176, 144, 3))\
    T(test_d2c(176, 144, 4))\
    T(test_d2c(640, 480, 1))\
    T(test_d2c(640, 480, 2))\
    T(test_d2c(640, 480, 3))\
    T(test_d2c(640, 480, 4))\
    T(test_d2c(848, 480, 1))\
    T(test_d2c(848, 480, 2))\
    T(test_d2c(848, 480, 3))\
    T(test_d2c(848, 480, 4))\
    T(test_d2c(1280, 720, 1))\
    T(test_d2c(1280, 720, 2))\
    T(test_d2c(1920, 1080, 1))\
    T(test_d2c(1920, 1080, 2))

static int lets_pause = 0;

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
 * This method fills up a range of memory using a start address
 * and start value.  The method of filling ensures that
 * accidentally overlapping regions have minimal chances of
 * matching, even if the same starting value is used.  This is
 * because the difference between successive values varies as
 * such.  This series only repeats after 704189 values, so the
 * probability of a match for a range of at least 2 values is
 * less than 2*10^-11.
 *
 * V(i + 1) - V(i) = { 1, 2, 3, ..., 65535, 2, 4, 6, 8 ...,
 * 65534, 3, 6, 9, 12, ..., 4, 8, 12, 16, ... }
 *
 * @author a0194118 (9/6/2009)
 *
 * @param start   start value
 * @param block   pointer to block info strucure
 */
void fill_mem(uint16_t start, MemAllocBlock *block)
{
    IN;
    uint16_t *ptr = (uint16_t *)block->ptr, delta = 1, step = 1;
    bytes_t height, width, stride, i;
    if (block->pixelFormat == PIXEL_FMT_PAGE)
    {
        height = 1;
        stride = width = block->dim.len;
    }
    else
    {
        height = block->dim.area.height;
        width = block->dim.area.width;
        stride = block->stride;
    }
    width *= def_bpp(block->pixelFormat);
    bytes_t size = height * stride;

    P("(%p,0x%x*0x%x,s=0x%x)=0x%x", block->ptr, width, height, stride, start);

    CHK_I(width,<=,stride);
    uint32_t *ptr32 = (uint32_t *)ptr;
    while (height--)
    {
        if (block->pixelFormat == PIXEL_FMT_32BIT)
        {
            for (i = 0; i < width; i += sizeof(uint32_t))
            {
                uint32_t val = (start & 0xFFFF) | (((uint32_t)(start + delta) & 0xFFFF) << 16);
                *ptr32++ = val;
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
            }
#ifdef __WRITE_IN_STRIDE__
            while (i < stride && (height || ((PAGE_SIZE - 1) & (uint32_t)ptr32)))
            {
                *ptr32++ = 0;
                i += sizeof(uint32_t);
            }
#else
            ptr32 += (stride - i) / sizeof(uint32_t);
#endif
        }
        else
        {
            for (i = 0; i < width; i += sizeof(uint16_t))
            {
                *ptr++ = start;
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
            }
#ifdef __WRITE_IN_STRIDE__
            while (i < stride && (height || ((PAGE_SIZE - 1) & (uint32_t)ptr)))
            {
                *ptr++ = 0;
                i += sizeof(uint16_t);
            }
#else
            ptr += (stride - i) / sizeof(uint16_t);
#endif

        }
    }
    CHK_P((block->pixelFormat == PIXEL_FMT_32BIT ? (void *)ptr32 : (void *)ptr),==,
          (block->ptr + size));
    OUT;
}

/**
 * This verifies if a range of memory at a given address was
 * filled up using the start value.
 *
 * @author a0194118 (9/6/2009)
 *
 * @param start   start value
 * @param block   pointer to block info strucure
 *
 * @return 0 on success, non-0 error value on failure
 */
int check_mem(uint16_t start, MemAllocBlock *block)
{
    IN;
    uint16_t *ptr = (uint16_t *)block->ptr, delta = 1, step = 1;
    bytes_t height, width, stride, r, i;
    if (block->pixelFormat == PIXEL_FMT_PAGE)
    {
        height = 1;
        stride = width = block->dim.len;
    }
    else
    {
        height = block->dim.area.height;
        width = block->dim.area.width;
        stride = block->stride;
    }
    width *= def_bpp(block->pixelFormat);

    CHK_I(width,<=,stride);
    uint32_t *ptr32 = (uint32_t *)ptr;
    for (r = 0; r < height; r++)
    {
        if (block->pixelFormat == PIXEL_FMT_32BIT)
        {
            for (i = 0; i < width; i += sizeof(uint32_t))
            {
                uint32_t val = (start & 0xFFFF) | (((uint32_t)(start + delta) & 0xFFFF) << 16);
                if (*ptr32++ != val) {
                    DP("assert: val[%u,%u] (=0x%x) != 0x%x", r, i, *--ptr32, val);
                    return R_I(MEMMGR_ERR_GENERIC);
                }
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
            }
#ifdef __WRITE_IN_STRIDE__
            while (i < stride && ((r < height - 1) || ((PAGE_SIZE - 1) & (uint32_t)ptr32)))
            {
                if (*ptr32++) {
                    DP("assert: val[%u,%u] (=0x%x) != 0", r, i, *--ptr32);
                    return R_I(MEMMGR_ERR_GENERIC);
                }
                i += sizeof(uint32_t);
            }
#else
            ptr32 += (stride - i) / sizeof(uint32_t);
#endif
        }
        else
        {
            for (i = 0; i < width; i += sizeof(uint16_t))
            {
                if (*ptr++ != start) {
                    DP("assert: val[%u,%u] (=0x%x) != 0x%x", r, i, *--ptr, start);
                    return R_I(MEMMGR_ERR_GENERIC);
                }
                start += delta;
                delta += step;
                /* increase step if overflown */
                if (delta < step) delta = ++step;
            }
#ifdef __WRITE_IN_STRIDE__
            while (i < stride && ((r < height - 1) || ((PAGE_SIZE - 1) & (uint32_t)ptr)))
            {
                if (*ptr++) {
                    DP("assert: val[%u,%u] (=0x%x) != 0", r, i, *--ptr);
                    return R_I(MEMMGR_ERR_GENERIC);
                }
                i += sizeof(uint16_t);
            }
#else
            ptr += (stride - i) / sizeof(uint16_t);
#endif
        }
    }
    return R_I(MEMMGR_ERR_NONE);
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

static void dump_block(struct tiler_block_info *blk, char *prefix, char *suffix)
{
    switch (blk->fmt)
    {
    case PIXEL_FMT_PAGE:
        P("%s [p=%p(0x%x),l=0x%x,s=%d]%s", prefix, blk->ptr, blk->ssptr,
          blk->dim.len, blk->stride, suffix);
        break;
    case PIXEL_FMT_8BIT:
    case PIXEL_FMT_16BIT:
    case PIXEL_FMT_32BIT:
        P("%s [p=%p(0x%x),%d*%d*%d,s=%d]%s", prefix, blk->ptr, blk->ssptr,
          blk->dim.area.width, blk->dim.area.height, def_bpp(blk->fmt) * 8,
          blk->stride, suffix);
        break;
    default:
        P("%s*[p=%p(0x%x),l=0x%x,s=%d,fmt=0x%x]%s", prefix, blk->ptr,
          blk->ssptr, blk->dim.len, blk->stride, blk->fmt, suffix);
    }
}

void check_xy(int td, MemAllocBlock *blk, int offset)
{
    struct tiler_block_info cblk;
    memcpy(&cblk, blk, sizeof(struct tiler_block_info));

    cblk.ssptr += def_bpp(cblk.fmt) * offset;
    A_I(ioctl(td, TILIOC_QUERY_BLK, &cblk),==,0);
    A_I(cblk.fmt,==,blk->pixelFormat);
    if (cblk.fmt == TILFMT_PAGE)
    {
        A_I(cblk.dim.len,==,blk->dim.len);
    }
    else
    {
        A_I(cblk.dim.area.width,==,blk->dim.area.width);
        A_I(cblk.dim.area.height,==,blk->dim.area.height);
    }
    A_I(cblk.stride,==,blk->stride);
    A_I(cblk.ssptr,==,blk->reserved);
}

void check_bad(int td, MemAllocBlock *blk, int offset)
{
    struct tiler_block_info cblk;
    memcpy(&cblk, blk, sizeof(struct tiler_block_info));

    cblk.ssptr += def_bpp(cblk.fmt) * offset;
    A_I(ioctl(td, TILIOC_QUERY_BLK, &cblk),==,0);
    A_I(cblk.fmt,==,TILFMT_INVALID);
    A_I(cblk.dim.len,==,0);
    A_I(cblk.stride,==,0);
    A_I(cblk.ssptr,==,0);
}

int test_d2c(pixels_t width, pixels_t height, int N)
{
    int res = 0, ix;

    struct tiler_block_info tblk[4], txblk[4], tx;
    MemAllocBlock *blk = (MemAllocBlock *)tblk, *x = (MemAllocBlock *)&tx;
    void *buf[4];
    ZERO(tblk);
    ZERO(txblk);
    ZERO(tx);

    for (ix = 0; ix < N; ix++)
    {
        if (ix == 3)
        {
            blk[ix].pixelFormat = PIXEL_FMT_PAGE;
            blk[ix].dim.len = ROUND_DOWN_TO2POW(width * height * 2, PAGE_SIZE);
        }
        else
        {
            blk[ix].pixelFormat = (ix == 2 ? PIXEL_FMT_32BIT :
                                  ix ? PIXEL_FMT_16BIT : PIXEL_FMT_8BIT);
            blk[ix].dim.area.width = width;
            blk[ix].dim.area.height = height;

            /* emulate NV12 for N == 2 */
            if (N == 2 && ix)
            {
                blk[ix].dim.area.height >>= 1;
                blk[ix].dim.area.width >>= 1;
            }
        }

        blk[ix].stride = 0;

        buf[ix] = MemMgr_Alloc(blk + ix,  1);
        CHK_P(buf[ix],==,blk[ix].ptr);
        res |= NOT_P(buf[ix],!=,NULL);
    }

#ifndef STUB_TILER
    int td = A_I(open("/dev/tiler", O_RDWR | O_SYNC),>=,0);

    /* query tiler driver for detailson these blocks, such as
       width/height/len/fmt */
    for (ix = 0; ix < N; ix++)
    {
        memcpy(txblk + ix, blk + ix, sizeof(*blk));
        dump_block(txblk + ix, "=(qb)=>", "");
        A_I(ioctl(td, TILIOC_QUERY_BLK, txblk + ix),==,0);
        dump_block(txblk + ix, "<=(qb)=", "");
    }

    /* try remapping and unmapping buffers */
    DSPtr dsptrs[4];
    bytes_t sizes[4], size;
    for (ix = 0; ix < N; ix++)
    {
        CHK_P(buf[ix],==,blk[ix].ptr);
        dsptrs[ix] = TilerMem_VirtToPhys(buf[ix]);
        sizes[ix] = def_size(tblk + ix);
        printf("%x %x ", dsptrs[ix], sizes[ix]);
    }

    for (ix = 0; ix < N; ix++)
    {
        fill_mem(ix * 0x1234, blk + ix);
    }

    if (lets_pause)
    {
        printf("Press enter to continue...");
        char line[80];
        if(fgets(line,80,stdin)) ;
    }

    void *bufPtr = tiler_assisted_phase1_D2CReMap(N, dsptrs, sizes);
    res |= NOT_P(bufPtr,!=,NULL);

    for (size = ix = 0; ix < N; ix++)
    {
        tx = tblk[ix];
        tx.ptr = bufPtr + size;
        res |= NOT_I(check_mem(ix * 0x1234, x),==,0);
        size += sizes[ix];
    }

    res |= NOT_I(tiler_assisted_phase1_DeMap(bufPtr),==,0);

    /* this section of the tests works on internal pointers, which are not
       yet supported by the algorithm */
#if 0

    int x = 0, y = 0;
    /* check valid X, Y coordinates */
    while (y < 1080)
    {
        check_xy(td, &blk[0], x + y * 4096);
        check_xy(td, &blk[1], x + y * 2048);
        check_xy(td, &blk[2], x + y * 2048);
        if (x + y * 1920 < 4096 * 128)
        {
            check_xy(td, &blk[3], x + y * 1920);
        }

        x += 1900;
        if (x >= 1920)
        {
            x -= 1920; y++;
        }
    }


    /* check invalid X, Y coordinates */
    check_bad(td, &blk[0], 4096 * 4096);
    check_bad(td, &blk[1], 4096 * 2048);
    check_bad(td, &blk[2], 4096 * 2048);
    check_bad(td, &blk[3], 4096 * 128);
#endif

    close(td);
#endif

    for (ix = 0; ix < N; ix++)
    {
        DP("%p", buf[ix]);
        res |= A_I(MemMgr_Free(buf[ix]),==,0);
    }

    return res;
}

int test_d2c_cli(int argc, char **argv)
{
    DP("argc=%d", argc);
    if (argc < 5) return 1;

    pixels_t width = atoi(argv[1]), height = atoi(argv[2]);
    DP("width=%d, height=%d", width, height);

    /* try remapping and unmapping buffers */
    int res = 0, ix, n = (argc - 3) >> 1;
    DP("n=%d", n);

    DSPtr *dsptrs;
    bytes_t *sizes, size;
    dsptrs = NEWN(DSPtr, n);
    sizes = NEWN(bytes_t, n);

    for (ix = 0; ix < n; ix++)
    {
        sscanf(argv[(ix << 1) + 3], "%x", dsptrs + ix);
        sscanf(argv[(ix << 1) + 4], "%x", sizes + ix);

        printf("%x %x ", dsptrs[ix], sizes[ix]);
    }

    void *bufPtr = tiler_assisted_phase1_D2CReMap(n, dsptrs, sizes);
    res |= NOT_P(bufPtr,!=,NULL);

    for (size = ix = 0; ix < n; ix++)
    {
        MemAllocBlock blk;
        ZERO(blk);
        if (ix == 3)
        {
            blk.pixelFormat = PIXEL_FMT_PAGE;
            blk.dim.len = ROUND_DOWN_TO2POW(width * height * 2, PAGE_SIZE);
            blk.stride = 0;
        }
        else
        {
            blk.pixelFormat = (ix == 2 ? PIXEL_FMT_32BIT :
                                  ix ? PIXEL_FMT_16BIT : PIXEL_FMT_8BIT);
            blk.dim.area.width = width;
            blk.dim.area.height = height;
            blk.stride = def_stride(def_bpp(blk.pixelFormat) * blk.dim.area.width);

            /* emulate NV12 for N == 2 */
            if (n == 2 && ix)
            {
                blk.dim.area.height >>= 1;
                blk.dim.area.width >>= 1;
            }
        }
        blk.ptr = bufPtr + size;
        res |= NOT_I(check_mem(ix * 0x1234, &blk),==,0);
        size += sizes[ix];
    }

    res |= NOT_I(tiler_assisted_phase1_DeMap(bufPtr),==,0);
    return res;
}

DEFINE_TESTS(TESTS)

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "="))
    {
        /* run process test */
        return test_d2c_cli(argc - 1, argv + 1);
    }

    if (argc > 1 && !strcmp(argv[1], "!"))
    {
        /* run in pause mode */
        lets_pause = 1;
        argc--;
        argv++;
    }
    return TestLib_Run(argc, argv, nullfn, nullfn, NULL);
}
