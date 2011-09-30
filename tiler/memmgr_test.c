/*
 *  memmgr_test.c
 *
 *  Memory Allocator Interface tests.
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

/* retrieve type definitions */
#define __DEBUG__
#undef __DEBUG_ENTRY__
#define __DEBUG_ASSERT__

#define __MAP_OK__
#undef __WRITE_IN_STRIDE__
#undef STAR_TRACE_MEM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif
#include <utils.h>
#include <list_utils.h>
#include <debug_utils.h>
#include <memmgr.h>
#include <tilermem.h>
#include <tilermem_utils.h>
#include <testlib.h>

/* for star_tiler_test */
#include <fcntl.h>     /* open() */
#include <unistd.h>    /* close() */
#include <sys/ioctl.h> /* ioctl() */

#define FALSE 0
#define TESTERR_NOTIMPLEMENTED -65378

#define MAX_ALLOCS 512

#define TESTS\
    T(alloc_1D_test(4096, 0))\
    T(alloc_2D_test(64, 64, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(64, 64, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(64, 64, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(64, 64))\
    T(map_1D_test(4096, 0))\
    T(alloc_1D_test(176 * 144 * 2, 0))\
    T(alloc_2D_test(176, 144, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(176, 144, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(176, 144, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(176, 144))\
    T(map_1D_test(176 * 144 * 2, 0))\
    T(alloc_1D_test(640 * 480 * 2, 0))\
    T(alloc_2D_test(640, 480, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(640, 480, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(640, 480, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(640, 480))\
    T(map_1D_test(640 * 480 * 2, 0))\
    T(alloc_1D_test(848 * 480 * 2, 0))\
    T(alloc_2D_test(848, 480, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(848, 480, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(848, 480, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(848, 480))\
    T(map_1D_test(848 * 480 * 2, 0))\
    T(alloc_1D_test(1280 * 720 * 2, 0))\
    T(alloc_2D_test(1280, 720, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(1280, 720, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(1280, 720, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(1280, 720))\
    T(map_1D_test(1280 * 720 * 2, 0))\
    T(alloc_1D_test(1920 * 1080 * 2, 0))\
    T(alloc_2D_test(1920, 1080, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(1920, 1080, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(1920, 1080, PIXEL_FMT_32BIT))\
    T(alloc_NV12_test(1920, 1080))\
    T(map_1D_test(1920 * 1080 * 2, 0))\
    T(map_1D_test(4096, 0))\
    T(map_1D_test(8192, 0))\
    T(map_1D_test(16384, 0))\
    T(map_1D_test(32768, 0))\
    T(map_1D_test(65536, 0))\
    T(neg_alloc_tests())\
    T(neg_free_tests())\
    T(neg_map_tests())\
    T(neg_unmap_tests())\
    T(neg_check_tests())\
    T(page_size_test())\
    T(maxalloc_2D_test(2500, 32, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(2500, 16, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(1250, 16, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(5000, 32, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(5000, 16, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(2500, 16, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(alloc_2D_test(8193, 16, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(8193, 16, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(4097, 16, PIXEL_FMT_32BIT))\
    T(alloc_2D_test(16384, 16, PIXEL_FMT_8BIT))\
    T(alloc_2D_test(16384, 16, PIXEL_FMT_16BIT))\
    T(alloc_2D_test(8192, 16, PIXEL_FMT_32BIT))\
    T(!alloc_2D_test(16385, 16, PIXEL_FMT_8BIT))\
    T(!alloc_2D_test(16385, 16, PIXEL_FMT_16BIT))\
    T(!alloc_2D_test(8193, 16, PIXEL_FMT_32BIT))\
    T(maxalloc_1D_test(4096, MAX_ALLOCS))\
    T(maxalloc_2D_test(64, 64, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(64, 64, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(64, 64, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(64, 64, MAX_ALLOCS))\
    T(maxmap_1D_test(4096, MAX_ALLOCS))\
    T(maxalloc_1D_test(176 * 144 * 2, MAX_ALLOCS))\
    T(maxalloc_2D_test(176, 144, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(176, 144, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(176, 144, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(176, 144, MAX_ALLOCS))\
    T(maxmap_1D_test(176 * 144 * 2, MAX_ALLOCS))\
    T(maxalloc_1D_test(640 * 480 * 2, MAX_ALLOCS))\
    T(maxalloc_2D_test(640, 480, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(640, 480, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(640, 480, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(640, 480, MAX_ALLOCS))\
    T(maxmap_1D_test(640 * 480 * 2, MAX_ALLOCS))\
    T(maxalloc_1D_test(848 * 480 * 2, MAX_ALLOCS))\
    T(maxalloc_2D_test(848, 480, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(848, 480, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(848, 480, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(848, 480, MAX_ALLOCS))\
    T(maxmap_1D_test(848 * 480 * 2, MAX_ALLOCS))\
    T(maxalloc_1D_test(1280 * 720 * 2, MAX_ALLOCS))\
    T(maxalloc_2D_test(1280, 720, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(1280, 720, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(1280, 720, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(1280, 720, MAX_ALLOCS))\
    T(maxmap_1D_test(1280 * 720 * 2, MAX_ALLOCS))\
    T(maxalloc_1D_test(1920 * 1080 * 2, MAX_ALLOCS))\
    T(maxalloc_2D_test(1920, 1080, PIXEL_FMT_8BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(1920, 1080, PIXEL_FMT_16BIT, MAX_ALLOCS))\
    T(maxalloc_2D_test(1920, 1080, PIXEL_FMT_32BIT, MAX_ALLOCS))\
    T(maxalloc_NV12_test(1920, 1080, 2))\
    T(maxalloc_NV12_test(1920, 1080, MAX_ALLOCS))\
    T(maxmap_1D_test(1920 * 1080 * 2, MAX_ALLOCS))\
    T(star_tiler_test(1000, 10))\
    T(star_tiler_test(1000, 30))\
    T(star_test(100, 10))\
    T(star_test(1000, 10))\

/* this is defined in memmgr.c, but not exported as it is for internal
   use only */
extern int __test__MemMgr();

/**
 * Returns the default page stride for this block
 *
 * @author a0194118 (9/4/2009)
 *
 * @param width  Width of 2D container
 *
 * @return Stride
 */
static bytes_t def_stride(bytes_t width)
{
    return ROUND_UP_TO2POW(width, PAGE_SIZE);
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
 * This method allocates a 1D tiled buffer of the given length
 * and stride using MemMgr_Alloc.  If successful, it checks
 * that the block information was updated with the pointer to
 * the block.  Additionally, it verifies the correct return
 * values for MemMgr_IsMapped, MemMgr_Is1DBlock,
 * MemMgr_Is2DBlock, MemMgr_GetStride, TilerMem_GetStride.  It
 * also verifies TilerMem_VirtToPhys using an internally stored
 * value of the ssptr. If any of these verifications fail, the
 * buffer is freed.  Otherwise, it is filled using the given
 * start value.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 * @param stride   Buffer stride
 * @param val      Fill start value
 *
 * @return pointer to the allocated buffer, or NULL on failure
 */
void *alloc_1D(bytes_t length, bytes_t stride, uint16_t val)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = PIXEL_FMT_PAGE;
    block.dim.len = length;
    block.stride = stride;

    void *bufPtr = MemMgr_Alloc(&block, 1);
    CHK_P(bufPtr,==,block.ptr);
    if (bufPtr) {
        if (NOT_I(MemMgr_IsMapped(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is1DBlock(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is2DBlock(bufPtr),==,0) ||
            NOT_I(MemMgr_GetStride(bufPtr),==,block.stride) ||
            NOT_P(TilerMem_VirtToPhys(bufPtr),==,block.reserved) ||
            NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(bufPtr)),==,PAGE_SIZE) ||
            NOT_L((PAGE_SIZE - 1) & (long)bufPtr,==,(PAGE_SIZE - 1) & block.reserved))
        {
            MemMgr_Free(bufPtr);
            return NULL;
        }
        fill_mem(val, &block);
    }
    return bufPtr;
}

/**
 * This method frees a 1D tiled buffer.  The given length,
 * stride and start values are used to verify that the buffer is
 * still correctly filled.  In the event of any errors, the
 * error value is returned.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 * @param stride   Buffer stride
 * @param val      Fill start value
 * @param bufPtr   Pointer to the allocated buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
int free_1D(bytes_t length, bytes_t stride, uint16_t val, void *bufPtr)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = PIXEL_FMT_PAGE;
    block.dim.len = length;
    block.stride = stride;
    block.ptr = bufPtr;

    int ret = A_I(check_mem(val, &block),==,0);
    ERR_ADD(ret, MemMgr_Free(bufPtr));
    return ret;
}

/**
 * This method allocates a 2D tiled buffer of the given width,
 * height, stride and pixel format using
 * MemMgr_Alloc. If successful, it checks that the block
 * information was updated with the pointer to the block.
 * Additionally, it verifies the correct return values for
 * MemMgr_IsMapped, MemMgr_Is1DBlock, MemMgr_Is2DBlock,
 * MemMgr_GetStride, TilerMem_GetStride.  It also verifies
 * TilerMem_VirtToPhys using an internally stored value of the
 * ssptr. If any of these verifications fail, the buffer is
 * freed.  Otherwise, it is filled using the given start value.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param fmt      Pixel format
 * @param stride   Buffer stride
 * @param val      Fill start value
 *
 * @return pointer to the allocated buffer, or NULL on failure
 */
void *alloc_2D(pixels_t width, pixels_t height, pixel_fmt_t fmt, bytes_t stride,
               uint16_t val)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = fmt;
    block.dim.area.width  = width;
    block.dim.area.height = height;
    block.stride = stride;

    void *bufPtr = MemMgr_Alloc(&block, 1);
    CHK_P(bufPtr,==,block.ptr);
    if (bufPtr) {
        bytes_t cstride = (fmt == PIXEL_FMT_8BIT  ? TILER_STRIDE_8BIT :
                           fmt == PIXEL_FMT_16BIT ? TILER_STRIDE_16BIT :
                           TILER_STRIDE_32BIT);

        if (NOT_I(MemMgr_IsMapped(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is1DBlock(bufPtr),==,0) ||
            NOT_I(MemMgr_Is2DBlock(bufPtr),!=,0) ||
            NOT_I(block.stride,!=,0) ||
            NOT_I(MemMgr_GetStride(bufPtr),==,block.stride) ||
            NOT_P(TilerMem_VirtToPhys(bufPtr),==,block.reserved) ||
            NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(bufPtr)),==,cstride) ||
            NOT_L((PAGE_SIZE - 1) & (long)bufPtr,==,(PAGE_SIZE - 1) & block.reserved))
        {
            MemMgr_Free(bufPtr);
            return NULL;
        }
        fill_mem(val, &block);
    }
    return bufPtr;
}

/**
 * This method frees a 2D tiled buffer.  The given width,
 * height, pixel format, stride and start values are used to
 * verify that the buffer is still correctly filled.  In the
 * event of any errors, the error value is returned.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param fmt      Pixel format
 * @param stride   Buffer stride
 * @param val      Fill start value
 * @param bufPtr   Pointer to the allocated buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
int free_2D(pixels_t width, pixels_t height, pixel_fmt_t fmt, bytes_t stride,
            uint16_t val, void *bufPtr)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = fmt;
    block.dim.area.width  = width;
    block.dim.area.height = height;
    block.stride = def_stride(width * def_bpp(fmt));
    block.ptr = bufPtr;

    int ret = A_I(check_mem(val, &block),==,0);
    ERR_ADD(ret, MemMgr_Free(bufPtr));
    return ret;
}

/**
 * This method allocates an NV12 tiled buffer of the given width
 * and height using MemMgr_Alloc. If successful, it checks that
 * the block informations were updated with the pointers to the
 * individual blocks. Additionally, it verifies the correct
 * return values for MemMgr_IsMapped, MemMgr_Is1DBlock,
 * MemMgr_Is2DBlock, MemMgr_GetStride, TilerMem_GetStride for
 * both blocks. It also verifies TilerMem_VirtToPhys using an
 * internally stored values of the ssptr. If any of these
 * verifications fail, the buffer is freed.  Otherwise, it is
 * filled using the given start value.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param val      Fill start value
 *
 * @return pointer to the allocated buffer, or NULL on failure
 */
void *alloc_NV12(pixels_t width, pixels_t height, uint16_t val)
{
    MemAllocBlock blocks[2];
    ZERO(blocks);

    blocks[0].pixelFormat = PIXEL_FMT_8BIT;
    blocks[0].dim.area.width  = width;
    blocks[0].dim.area.height = height;
    blocks[1].pixelFormat = PIXEL_FMT_16BIT;
    blocks[1].dim.area.width  = width >> 1;
    blocks[1].dim.area.height = height >> 1;

    void *bufPtr = MemMgr_Alloc(blocks, 2);
    CHK_P(blocks[0].ptr,==,bufPtr);
    if (bufPtr) {
        void *buf2 = bufPtr + blocks[0].stride * height;
        if (NOT_P(blocks[1].ptr,==,buf2) ||
            NOT_I(MemMgr_IsMapped(bufPtr),!=,0) ||
            NOT_I(MemMgr_IsMapped(buf2),!=,0) ||
            NOT_I(MemMgr_Is1DBlock(bufPtr),==,0) ||
            NOT_I(MemMgr_Is1DBlock(buf2),==,0) ||
            NOT_I(MemMgr_Is2DBlock(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is2DBlock(buf2),!=,0) ||
            NOT_I(blocks[0].stride,!=,0) ||
            NOT_I(blocks[1].stride,!=,0) ||
            NOT_I(MemMgr_GetStride(bufPtr),==,blocks[0].stride) ||
            NOT_I(MemMgr_GetStride(buf2),==,blocks[1].stride) ||
            NOT_P(TilerMem_VirtToPhys(bufPtr),==,blocks[0].reserved) ||
            NOT_P(TilerMem_VirtToPhys(buf2),==,blocks[1].reserved) ||
            NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(bufPtr)),==,TILER_STRIDE_8BIT) ||
            NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(buf2)),==,TILER_STRIDE_16BIT) ||
            NOT_L((PAGE_SIZE - 1) & (long)blocks[0].ptr,==,(PAGE_SIZE - 1) & blocks[0].reserved) ||
            NOT_L((PAGE_SIZE - 1) & (long)blocks[1].ptr,==,(PAGE_SIZE - 1) & blocks[1].reserved))
        {
            MemMgr_Free(bufPtr);
            return NULL;
        }

        fill_mem(val, blocks);
        fill_mem(val, blocks + 1);
    } else {
        CHK_P(blocks[1].ptr,==,NULL);
    }

    return bufPtr;
}

/**
 * This method frees an NV12 tiled buffer.  The given width,
 * height and start values are used to verify that the buffer is
 * still correctly filled.  In the event of any errors, the
 * error value is returned.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param val      Fill start value
 * @param bufPtr   Pointer to the allocated buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
int free_NV12(pixels_t width, pixels_t height, uint16_t val, void *bufPtr)
{
    MemAllocBlock blocks[2];
    memset(blocks, 0, sizeof(blocks));

    blocks[0].pixelFormat = PIXEL_FMT_8BIT;
    blocks[0].dim.area.width  = width;
    blocks[0].dim.area.height = height;
    blocks[0].stride = def_stride(width);
    blocks[0].ptr = bufPtr;
    blocks[1].pixelFormat = PIXEL_FMT_16BIT;
    blocks[1].dim.area.width  = width >> 1;
    blocks[1].dim.area.height = height >> 1;
    blocks[1].stride = def_stride(width);
    blocks[1].ptr = bufPtr + blocks[0].stride * height;

    int ret = A_I(check_mem(val, blocks),==,0);
    ERR_ADD(ret, check_mem(val, blocks + 1));
    ERR_ADD(ret, MemMgr_Free(bufPtr));
    return ret;
}

/**
 * This method maps a preallocated 1D buffer of the given length
 * and stride into tiler space using MemMgr_Map.  The mapped
 * address must differ from the supplied address is successful.
 * Moreover, it checks that the block information was
 * updated with the pointer to the block. Additionally, it
 * verifies the correct return values for MemMgr_IsMapped,
 * MemMgr_Is1DBlock, MemMgr_Is2DBlock, MemMgr_GetStride,
 * TilerMem_GetStride.  It also verifies TilerMem_VirtToPhys
 * using an internally stored value of the ssptr. If any of
 * these verifications fail, the buffer is unmapped. Otherwise,
 * the original buffer is filled using the given start value.
 *
 * :TODO: how do we verify the mapping?
 *
 * @author a0194118 (9/7/2009)
 *
 * @param dataPtr  Pointer to the allocated buffer
 * @param length   Buffer length
 * @param stride   Buffer stride
 * @param val      Fill start value
 *
 * @return pointer to the mapped buffer, or NULL on failure
 */
void *map_1D(void *dataPtr, bytes_t length, bytes_t stride, uint16_t val)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = PIXEL_FMT_PAGE;
    block.dim.len = length;
    block.stride = stride;
    block.ptr    = dataPtr;

    void *bufPtr = MemMgr_Map(&block, 1);
    CHK_P(bufPtr,==,block.ptr);
    if (bufPtr) {
        if (NOT_P(bufPtr,!=,dataPtr) ||
            NOT_I(MemMgr_IsMapped(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is1DBlock(bufPtr),!=,0) ||
            NOT_I(MemMgr_Is2DBlock(bufPtr),==,0) ||
            NOT_I(MemMgr_GetStride(bufPtr),==,block.stride) ||
            NOT_P(TilerMem_VirtToPhys(bufPtr),==,block.reserved) ||
            NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(bufPtr)),==,PAGE_SIZE) ||
            NOT_L((PAGE_SIZE - 1) & (long)bufPtr,==,0) ||
            NOT_L((PAGE_SIZE - 1) & block.reserved,==,0))
        {
            MemMgr_UnMap(bufPtr);
            return NULL;
        }
        block.ptr = dataPtr;
        fill_mem(val, &block);
    }
    return bufPtr;
}

/**
 * This method unmaps a 1D tiled buffer.  The given data
 * pointer, length, stride and start values are used to verify
 * that the buffer is still correctly filled.  In the event of
 * any errors, the error value is returned.
 *
 * :TODO: how do we verify the mapping?
 *
 * @author a0194118 (9/7/2009)
 *
 * @param dataPtr  Pointer to the preallocated buffer
 * @param length   Buffer length
 * @param stride   Buffer stride
 * @param val      Fill start value
 * @param bufPtr   Pointer to the mapped buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
int unmap_1D(void *dataPtr, bytes_t length, bytes_t stride, uint16_t val, void *bufPtr)
{
    MemAllocBlock block;
    memset(&block, 0, sizeof(block));

    block.pixelFormat = PIXEL_FMT_PAGE;
    block.dim.len = length;
    block.stride = stride;
    block.ptr = dataPtr;
    int ret = A_I(check_mem(val, &block),==,0);
    ERR_ADD(ret, MemMgr_UnMap(bufPtr));
    return ret;
}

/**
 * Tests the MemMgr_PageSize method.
 *
 * @author a0194118 (9/15/2009)
 *
 * @return 0 on success, non-0 error value on failure.
 */
int page_size_test()
{
    return NOT_I(MemMgr_PageSize(),==,PAGE_SIZE);
}

/**
 * This method tests the allocation and freeing of a 1D tiled
 * buffer.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 * @param stride   Buffer stride
 *
 * @return 0 on success, non-0 error value on failure
 */
int alloc_1D_test(bytes_t length, bytes_t stride)
{
    printf("Allocate & Free %ub 1D buffer\n", length);

    uint16_t val = (uint16_t) rand();
    void *ptr = alloc_1D(length, stride, val);
    if (!ptr) return 1;
    int res = free_1D(length, stride, val, ptr);
    return res;
}

/**
 * This method tests the allocation and freeing of a 2D tiled
 * buffer.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param fmt      Pixel format
 *
 * @return 0 on success, non-0 error value on failure
 */
int alloc_2D_test(pixels_t width, pixels_t height, pixel_fmt_t fmt)
{
    printf("Allocate & Free %ux%ux%ub 1D buffer\n", width, height, def_bpp(fmt));

    uint16_t val = (uint16_t) rand();
    void *ptr = alloc_2D(width, height, fmt, 0, val);
    if (!ptr) return 1;
    int res = free_2D(width, height, fmt, 0, val, ptr);
    return res;
}

/**
 * This method tests the allocation and freeing of an NV12 tiled
 * buffer.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 *
 * @return 0 on success, non-0 error value on failure
 */
int alloc_NV12_test(pixels_t width, pixels_t height)
{
    printf("Allocate & Free %ux%u NV12 buffer\n", width, height);

    uint16_t val = (uint16_t) rand();
    void *ptr = alloc_NV12(width, height, val);
    if (!ptr) return 1;
    int res = free_NV12(width, height, val, ptr);
    return res;
}

/**
 * This method tests the mapping and unmapping of a 1D buffer.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 * @param stride   Buffer stride
 *
 * @return 0 on success, non-0 error value on failure
 */
int map_1D_test(bytes_t length, bytes_t stride)
{
    length = (length + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1);
    printf("Mapping and UnMapping 0x%xb 1D buffer\n", length);

#ifdef __MAP_OK__
    /* allocate aligned buffer */
    void *buffer = malloc(length + PAGE_SIZE - 1);
    void *dataPtr = (void *)(((uint32_t)buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
    uint16_t val = (uint16_t) rand();
    void *ptr = map_1D(dataPtr, length, stride, val);
    if (!ptr) return 1;
    int res = unmap_1D(dataPtr, length, stride, val, ptr);
    FREE(buffer);
#else
    int res = TESTERR_NOTIMPLEMENTED;
#endif
    return res;
}

/**
 * This method tests the allocation and freeing of a number of
 * 1D tiled buffers (up to MAX_ALLOCS)
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 *
 * @return 0 on success, non-0 error value on failure
 */
int maxalloc_1D_test(bytes_t length, int max_allocs)
{
    printf("Allocate & Free max # of %ub 1D buffers\n", length);

    struct data {
        uint16_t val;
        void    *bufPtr;
    } *mem;

    /* allocate as many buffers as we can */
    mem = NEWN(struct data, max_allocs);
    void *ptr = (void *)mem;
    int ix, res = 0;
    for (ix = 0; ptr && ix < max_allocs;)
    {
        uint16_t val = (uint16_t) rand();
        ptr = alloc_1D(length, 0, val);
        if (ptr)
        {
            mem[ix].val = val;
            mem[ix].bufPtr = ptr;
            ix++;
        }
    }

    P(":: Allocated %d buffers", ix);

    while (ix--)
    {
        ERR_ADD(res, free_1D(length, 0, mem[ix].val, mem[ix].bufPtr));
    }
    FREE(mem);
    return res;
}

/**
 * This method tests the allocation and freeing of a number of
 * 2D tiled buffers (up to MAX_ALLOCS)
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 * @param fmt      Pixel format
 *
 * @return 0 on success, non-0 error value on failure
 */
int maxalloc_2D_test(pixels_t width, pixels_t height, pixel_fmt_t fmt, int max_allocs)
{
    printf("Allocate & Free max # of %ux%ux%ub 1D buffers\n", width, height, def_bpp(fmt));

    struct data {
        uint16_t  val;
        void     *bufPtr;
    } *mem;

    /* allocate as many buffers as we can */
    mem = NEWN(struct data, max_allocs);
    void *ptr = (void *)mem;
    int ix, res = 0;
    for (ix = 0; ptr && ix < max_allocs;)
    {
        uint16_t val = (uint16_t) rand();
        ptr = alloc_2D(width, height, fmt, 0, val);
        if (ptr)
        {
            mem[ix].val = val;
            mem[ix].bufPtr = ptr;
            ix++;
        }
    }

    P(":: Allocated %d buffers", ix);

    while (ix--)
    {
        ERR_ADD(res, free_2D(width, height, fmt, 0, mem[ix].val, mem[ix].bufPtr));
    }
    FREE(mem);
    return res;
}

/**
 * This method tests the allocation and freeing of a number of
 * NV12 tiled buffers (up to MAX_ALLOCS)
 *
 * @author a0194118 (9/7/2009)
 *
 * @param width    Buffer width
 * @param height   Buffer height
 *
 * @return 0 on success, non-0 error value on failure
 */
int maxalloc_NV12_test(pixels_t width, pixels_t height, int max_allocs)
{
    printf("Allocate & Free max # of %ux%u NV12 buffers\n", width, height);

    struct data {
        uint16_t     val;
        void        *bufPtr;
    } *mem;

    /* allocate as many buffers as we can */
    mem = NEWN(struct data, max_allocs);
    void *ptr = (void *)mem;
    int ix, res = 0;
    for (ix = 0; ptr && ix < max_allocs;)
    {
        uint16_t val = (uint16_t) rand();
        ptr = alloc_NV12(width, height, val);
        if (ptr)
        {
            mem[ix].val = val;
            mem[ix].bufPtr = ptr;
            ix++;
        }
    }

    P(":: Allocated %d buffers", ix);

    while (ix--)
    {
        ERR_ADD(res, free_NV12(width, height, mem[ix].val, mem[ix].bufPtr));
    }
    FREE(mem);
    return res;
}

/**
 * This method tests the mapping and unnapping of a number of
 * 1D buffers (up to MAX_ALLOCS)
 *
 * @author a0194118 (9/7/2009)
 *
 * @param length   Buffer length
 *
 * @return 0 on success, non-0 error value on failure
 */
int maxmap_1D_test(bytes_t length, int max_maps)
{
    length = (length + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1);
    printf("Map & UnMap max # of %xb 1D buffers\n", length);

#ifdef __MAP_OK__
    struct data {
        uint16_t val;
        void    *bufPtr, *buffer, *dataPtr;
    } *mem;

    /* map as many buffers as we can */
    mem = NEWN(struct data, max_maps);
    void *ptr = (void *)mem;
    int ix, res = 0;
    for (ix = 0; ptr && ix < max_maps;)
    {
        /* allocate aligned buffer */
        ptr = malloc(length + PAGE_SIZE - 1);
        if (ptr)
        {
            void *buffer = ptr;
            void *dataPtr = (void *)(((uint32_t)buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
            uint16_t val = (uint16_t) rand();
            ptr = map_1D(dataPtr, length, 0, val);
            if (ptr)
            {
                mem[ix].val = val;
                mem[ix].bufPtr = ptr;
                mem[ix].buffer = buffer;
                mem[ix].dataPtr = dataPtr;
                ix++;
            }
            else
            {
                FREE(buffer);
                break;
            }
        }
    }

    P(":: Mapped %d buffers", ix);

    while (ix--)
    {
        ERR_ADD(res, unmap_1D(mem[ix].dataPtr, length, 0, mem[ix].val, mem[ix].bufPtr));
        FREE(mem[ix].buffer);
    }
#else
    int res = TESTERR_NOTIMPLEMENTED;
#endif
    return res;
}

/**
 * This stress tests allocates/maps/frees/unmaps buffers at
 * least num_ops times.  The test maintains a set of slots that
 * are initially NULL.  For each operation, a slot is randomly
 * selected.  If the slot is not used, it is filled randomly
 * with a 1D, 2D, NV12 or mapped buffer.  If it is used, the
 * slot is cleared by freeing/unmapping the buffer already
 * there.  The buffers are filled on alloc/map and this is
 * checked on free/unmap to verify that there was no memory
 * corruption.  Failed allocation and maps are ignored as we may
 * run out of memory.  The return value is the first error code
 * encountered, or 0 on success.
 *
 * This test sets the seed so that it produces reproducible
 * results.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param num_ops    Number of operations to perform
 * @param num_slots  Number of slots to maintain
 *
 * @return 0 on success, non-0 error value on failure
 */
int star_test(uint32_t num_ops, uint16_t num_slots)
{
    printf("Random set of %d Allocs/Maps and Frees/UnMaps for %d slots\n", num_ops, num_slots);
    srand(0x4B72316A);
    struct data {
        int      op;
        uint16_t val;
        pixels_t width, height;
        bytes_t  length;
        void    *bufPtr;
        void    *buffer;
        void    *dataPtr;
    } *mem;

    /* allocate memory state */
    mem = NEWN(struct data, num_slots);
    if (!mem) return NOT_P(mem,!=,NULL);

    /* perform alloc/free/unmaps */
    int res = 0, ix;
    while (!res && num_ops--)
    {
        ix = rand() % num_slots;
        /* see if we need to free/unmap data */
        if (mem[ix].bufPtr)
        {
            /* check memory fill */
            switch (mem[ix].op)
            {
            case 0: res = unmap_1D(mem[ix].dataPtr, mem[ix].length, 0, mem[ix].val, mem[ix].bufPtr);
                FREE(mem[ix].buffer);
                break;
            case 1: res = free_1D(mem[ix].length, 0, mem[ix].val, mem[ix].bufPtr); break;
            case 2: res = free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_8BIT, 0, mem[ix].val, mem[ix].bufPtr); break;
            case 3: res = free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_16BIT, 0, mem[ix].val, mem[ix].bufPtr); break;
            case 4: res = free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_32BIT, 0, mem[ix].val, mem[ix].bufPtr); break;
            case 5: res = free_NV12(mem[ix].width, mem[ix].height, mem[ix].val, mem[ix].bufPtr); break;
            }
            P("%s[%p]", mem[ix].op ? "free" : "unmap", mem[ix].bufPtr);
            ZERO(mem[ix]);
        }
        /* we need to allocate/map data */
        else
        {
            int op = rand();
            /* set width */
            pixels_t width, height;
            switch ("AAAABBBBCCCDDEEF"[op & 15]) {
            case 'F': width = 1920; height = 1080; break;
            case 'E': width = 1280; height = 720; break;
            case 'D': width = 640; height = 480; break;
            case 'C': width = 848; height = 480; break;
            case 'B': width = 176; height = 144; break;
            case 'A': width = height = 64; break;
            }
            mem[ix].length = (bytes_t)width * height;
            mem[ix].width = width;
            mem[ix].height = height;
            mem[ix].val = ((uint16_t)rand());

            /* perform operation */
            mem[ix].op = "AAABBBBCCCCDDDDE"[(op >> 4) & 15] - 'A';
            switch (mem[ix].op)
            {
            case 0: /* map 1D buffer */
#ifdef __MAP_OK__
                /* allocate aligned buffer */
                mem[ix].length = (mem[ix].length + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1);
                mem[ix].buffer = malloc(mem[ix].length + PAGE_SIZE - 1);
                if (mem[ix].buffer)
                {
                    mem[ix].dataPtr = (void *)(((uint32_t)mem[ix].buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
                    mem[ix].bufPtr = map_1D(mem[ix].dataPtr, mem[ix].length, 0, mem[ix].val);
                    if (!mem[ix].bufPtr) FREE(mem[ix].buffer);
                }
                P("map[l=0x%x] = %p", mem[ix].length, mem[ix].bufPtr);
                break;
#else
                mem[ix].op = 1;
#endif
            case 1:
                mem[ix].bufPtr = alloc_1D(mem[ix].length, 0, mem[ix].val);
                P("alloc[l=0x%x] = %p", mem[ix].length, mem[ix].bufPtr);
                break;
            case 2:
                mem[ix].bufPtr = alloc_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_8BIT, 0, mem[ix].val);
                P("alloc[%d*%d*8] = %p", mem[ix].width, mem[ix].height, mem[ix].bufPtr);
                break;
            case 3:
                mem[ix].bufPtr = alloc_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_16BIT, 0, mem[ix].val);
                P("alloc[%d*%d*16] = %p", mem[ix].width, mem[ix].height, mem[ix].bufPtr);
                break;
            case 4:
                mem[ix].bufPtr = alloc_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_32BIT, 0, mem[ix].val);
                P("alloc[%d*%d*32] = %p", mem[ix].width, mem[ix].height, mem[ix].bufPtr);
                break;
            case 5:
                mem[ix].bufPtr = alloc_NV12(mem[ix].width, mem[ix].height, mem[ix].val);
                P("alloc[%d*%d*NV12] = %p", mem[ix].width, mem[ix].height, mem[ix].bufPtr);
                break;
            }

            /* check all previous buffers */
#ifdef STAR_TRACE_MEM
            for (ix = 0; ix < num_slots; ix++)
            {
                MemAllocBlock blk;
                if (mem[ix].bufPtr)
                {
                    if(0) P("ptr=%p, op=%d, w=%d, h=%d, l=%x, val=%x",
                      mem[ix].bufPtr, mem[ix].op, mem[ix].width, mem[ix].height,
                      mem[ix].length, mem[ix].val);
                    switch (mem[ix].op)
                    {
                    case 0: case 1:
                        blk.pixelFormat = PIXEL_FMT_PAGE;
                        blk.dim.len = mem[ix].length;
                        break;
                    case 5:
                        blk.pixelFormat = PIXEL_FMT_16BIT;
                        blk.dim.area.width = mem[ix].width >> 1;
                        blk.dim.area.height = mem[ix].height >> 1;
                        blk.stride = def_stride(mem[ix].width);  /* same for Y and UV */
                        blk.ptr = mem[ix].bufPtr + mem[ix].height * blk.stride;
                        check_mem(mem[ix].val, &blk);
                    case 2:
                        blk.pixelFormat = PIXEL_FMT_8BIT;
                        blk.dim.area.width = mem[ix].width;
                        blk.dim.area.height = mem[ix].height;
                        blk.stride = def_stride(mem[ix].width);
                        break;
                    case 3:
                        blk.pixelFormat = PIXEL_FMT_16BIT;
                        blk.dim.area.width = mem[ix].width;
                        blk.dim.area.height = mem[ix].height;
                        blk.stride = def_stride(mem[ix].width * 2);
                        break;
                    case 4:
                        blk.pixelFormat = PIXEL_FMT_32BIT;
                        blk.dim.area.width = mem[ix].width;
                        blk.dim.area.height = mem[ix].height;
                        blk.stride = def_stride(mem[ix].width * 4);
                        break;
                    }
                    blk.ptr = mem[ix].bufPtr;
                    check_mem(mem[ix].val, &blk);
                }
            }
#endif
        }
    }

    /* unmap and free everything */
    for (ix = 0; ix < num_slots; ix++)
    {
        if (mem[ix].bufPtr)
        {
            /* check memory fill */
            switch (mem[ix].op)
            {
            case 0: ERR_ADD(res, unmap_1D(mem[ix].dataPtr, mem[ix].length, 0, mem[ix].val, mem[ix].bufPtr));
                FREE(mem[ix].buffer);
                break;
            case 1: ERR_ADD(res, free_1D(mem[ix].length, 0, mem[ix].val, mem[ix].bufPtr)); break;
            case 2: ERR_ADD(res, free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_8BIT, 0, mem[ix].val, mem[ix].bufPtr)); break;
            case 3: ERR_ADD(res, free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_16BIT, 0, mem[ix].val, mem[ix].bufPtr)); break;
            case 4: ERR_ADD(res, free_2D(mem[ix].width, mem[ix].height, PIXEL_FMT_32BIT, 0, mem[ix].val, mem[ix].bufPtr)); break;
            case 5: ERR_ADD(res, free_NV12(mem[ix].width, mem[ix].height, mem[ix].val, mem[ix].bufPtr)); break;
            }
        }
    }
    FREE(mem);

    return res;
}

/**
 * This stress tests allocates/maps/frees/unmaps buffers at
 * least num_ops times.  The test maintains a set of slots that
 * are initially NULL.  For each operation, a slot is randomly
 * selected.  If the slot is not used, it is filled randomly
 * with a 1D, 2D, NV12 or mapped buffer.  If it is used, the
 * slot is cleared by freeing/unmapping the buffer already
 * there.  The buffers are filled on alloc/map and this is
 * checked on free/unmap to verify that there was no memory
 * corruption.  Failed allocation and maps are ignored as we may
 * run out of memory.  The return value is the first error code
 * encountered, or 0 on success.
 *
 * This test sets the seed so that it produces reproducible
 * results.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param num_ops    Number of operations to perform
 * @param num_slots  Number of slots to maintain
 *
 * @return 0 on success, non-0 error value on failure
 */
int star_tiler_test(uint32_t num_ops, uint16_t num_slots)
{
    printf("Random set of %d tiler Allocs/Maps and Frees/UnMaps for %d slots\n", num_ops, num_slots);
    srand(0x4B72316A);
    struct data {
        int      op;
        struct tiler_block_info blk;
        void    *buffer;
    } *mem;

    /* allocate memory state */
    mem = NEWN(struct data, num_slots);
    if (!mem) return NOT_P(mem,!=,NULL);

    /* perform alloc/free/unmaps */
    int ix, td = A_S(open("/dev/tiler", O_RDWR),>=,0), res = td < 0 ? td : 0;
    while (!res && num_ops--)
    {
        ix = rand() % num_slots;
        /* see if we need to free/unmap data */
        if (mem[ix].blk.id)
        {
            P("free [0x%x(0x%x)]", mem[ix].blk.id, mem[ix].blk.ssptr);
            res = A_S(ioctl(td, TILIOC_FBLK, &mem[ix].blk),==,0);
            FREE(mem[ix].buffer);
            ZERO(mem[ix]);
        }
        /* we need to allocate/map data */
        else
        {
            int op = rand();
            /* set width */
            pixels_t width, height;
            switch ("AAAABBBBCCCDDEEF"[op & 15]) {
            case 'F': width = 1920; height = 1080; break;
            case 'E': width = 1280; height = 720; break;
            case 'D': width = 640; height = 480; break;
            case 'C': width = 848; height = 480; break;
            case 'B': width = 176; height = 144; break;
            case 'A': width = height = 64; break;
            }
            bytes_t length = (bytes_t)width * height;

            /* perform operation */
            mem[ix].op = "AAABBBBCCCCDDDDE"[(op >> 4) & 15] - 'A';
            switch (mem[ix].op)
            {
            case 0: /* map 1D buffer */
                /* allocate aligned buffer */
                length = (length + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1);
                mem[ix].buffer = malloc(length + PAGE_SIZE - 1);
                if (mem[ix].buffer)
                {
                    mem[ix].blk.dim.len = length;
                    mem[ix].blk.fmt = TILFMT_PAGE;
                    mem[ix].blk.ptr = (void *)(((uint32_t)mem[ix].buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
                    res = A_S(ioctl(td, TILIOC_MBLK, &mem[ix].blk),==,0);
                    if (res)
                        FREE(mem[ix].buffer);
                }
                P("map[l=0x%x] = 0x%x(0x%x)", length, mem[ix].blk.id, mem[ix].blk.ssptr);
                break;
            case 1:
                mem[ix].blk.dim.len = length;
                mem[ix].blk.fmt = TILFMT_PAGE;
                res = A_S(ioctl(td, TILIOC_GBLK, &mem[ix].blk),==,0);
                P("alloc[l=0x%x] = 0x%x(0x%x)", length, mem[ix].blk.id, mem[ix].blk.ssptr);
                break;
            case 2: case 3: case 4:
                mem[ix].blk.dim.area.width = width;
                mem[ix].blk.dim.area.height = height;
                mem[ix].blk.fmt = TILFMT_8BIT + mem[ix].op - 2;
                res = A_S(ioctl(td, TILIOC_GBLK, &mem[ix].blk),==,0);
                P("alloc[%d*%d*%d] = 0x%x(0x%x)", width, height, 8 << (mem[ix].op -2), mem[ix].blk.id, mem[ix].blk.ssptr);
                break;
            }
        }
    }

    /* unmap and free everything */
    for (ix = 0; ix < num_slots; ix++)
    {
        if (mem[ix].blk.id)
        {
            res = A_S(ioctl(td, TILIOC_FBLK, &mem[ix].blk),==,0);
            FREE(mem[ix].buffer);
        }
    }
    ERR_ADD_S(res, close(td));
    FREE(mem);

    return res;
}

#define NEGA(exp) E_ { void *__ptr__ = A_P(exp,==,NULL); if (__ptr__) MemMgr_Free(__ptr__); __ptr__ != NULL; } _E

/**
 * Performs negative tests for MemMgr_Alloc.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure
 */
int neg_alloc_tests()
{
    printf("Negative Alloc tests\n");

    MemAllocBlock block[2], *blk;
    memset(&block, 0, sizeof(block));

    int ret = 0, num_blocks;

    for (num_blocks = 1; num_blocks < 3; num_blocks++)
    {
        blk = block + num_blocks - 1;

        P("/* bad pixel format */");
        blk->pixelFormat = PIXEL_FMT_MIN - 1;
        blk->dim.len = PAGE_SIZE;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));
        blk->pixelFormat = PIXEL_FMT_MAX + 1;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        P("/* bad 1D stride */");
        blk->pixelFormat = PIXEL_FMT_PAGE;
        blk->stride = PAGE_SIZE - 1;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        P("/* 0 1D length */");
        blk->dim.len = blk->stride = 0;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        P("/* bad 2D stride */");
        blk->pixelFormat = PIXEL_FMT_8BIT;
        blk->dim.area.width = PAGE_SIZE - 1;
        blk->stride = PAGE_SIZE - 1;
        blk->dim.area.height = 16;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        P("/* bad 2D width */");
        blk->stride = blk->dim.area.width = 0;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        P("/* bad 2D height */");
        blk->dim.area.height = 0;
        blk->dim.area.width = 16;
        ret |= NEGA(MemMgr_Alloc(block, num_blocks));

        /* good 2D block */
        blk->dim.area.height = 16;
    }

    block[0].pixelFormat = block[1].pixelFormat = PIXEL_FMT_8BIT;
    block[0].dim.area.width = 16384;
    block[0].dim.area.height = block[1].dim.area.width = 16;
    block[1].dim.area.height = 8192;
    ret |= NEGA(MemMgr_Alloc(block, 2));

    return ret;
}

/**
 * Performs negative tests for MemMgr_Free.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure
 */
int neg_free_tests()
{
    printf("Negative Free tests\n");

    void *ptr = alloc_2D(2500, 10, PIXEL_FMT_16BIT, 2 * PAGE_SIZE, 0);
    int ret = 0;

    MemMgr_Free(ptr);

    P("/* free something twice */");
    ret |= NOT_I(MemMgr_Free(ptr),!=,0);

    P("/* free NULL */");
    ret |= NOT_I(MemMgr_Free(NULL),!=,0);

    P("/* free arbitrary value */");
    ret |= NOT_I(MemMgr_Free((void *)0x12345678),!=,0);

    P("/* free mapped buffer */");
    void *buffer = malloc(PAGE_SIZE * 2);
    void *dataPtr = (void *)(((uint32_t)buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
    ptr = map_1D(dataPtr, PAGE_SIZE, 0, 0);
    ret |= NOT_I(MemMgr_Free(ptr),!=,0);

    MemMgr_UnMap(ptr);

    return ret;
}

#define NEGM(exp) E_ { void *__ptr__ = A_P(exp,==,NULL); if (__ptr__) MemMgr_UnMap(__ptr__); __ptr__ != NULL; } _E

/**
 * Performs negative tests for MemMgr_Map.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure
 */
int neg_map_tests()
{
    printf("Negative Map tests\n");

    MemAllocBlock block[2], *blk;
    memset(&block, 0, sizeof(block));

    int ret = 0, num_blocks;

    for (num_blocks = 1; num_blocks < 3; num_blocks++)
    {
        blk = block + num_blocks - 1;

        P("/* bad pixel format */");
        blk->pixelFormat = PIXEL_FMT_MIN - 1;
        blk->dim.len = PAGE_SIZE;
        ret |= NEGM(MemMgr_Map(block, num_blocks));
        blk->pixelFormat = PIXEL_FMT_MAX + 1;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        P("/* bad 1D stride */");
        blk->pixelFormat = PIXEL_FMT_PAGE;
        blk->stride = PAGE_SIZE - 1;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        P("/* 0 1D length */");
        blk->dim.len = blk->stride = 0;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        P("/* bad 2D stride */");
        blk->pixelFormat = PIXEL_FMT_8BIT;
        blk->dim.area.width = PAGE_SIZE - 1;
        blk->stride = PAGE_SIZE - 1;
        blk->dim.area.height = 16;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        P("/* bad 2D width */");
        blk->stride = blk->dim.area.width = 0;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        P("/* bad 2D height */");
        blk->dim.area.height = 0;
        blk->dim.area.width = 16;
        ret |= NEGM(MemMgr_Map(block, num_blocks));

        /* good 2D block */
        blk->dim.area.height = 16;
    }

    P("/* 2 buffers */");
    ret |= NEGM(MemMgr_Map(block, 2));

    P("/* 1 2D buffer */");
    ret |= NEGM(MemMgr_Map(block, 1));

    P("/* 1 1D buffer with no address */");
    block[0].pixelFormat = PIXEL_FMT_PAGE;
    block[0].dim.len = 2 * PAGE_SIZE;
    block[0].ptr = NULL;
    ret |= NEGM(MemMgr_Map(block, 1));

    P("/* 1 1D buffer with not aligned start address */");
    void *buffer = malloc(3 * PAGE_SIZE);
    void *dataPtr = (void *)(((uint32_t)buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
    block[0].ptr = dataPtr + 3;
    ret |= NEGM(MemMgr_Map(block, 1));

    P("/* 1 1D buffer with not aligned length */");
    block[0].ptr = dataPtr;
    block[0].dim.len -= 5;
    ret |= NEGM(MemMgr_Map(block, 1));

#if 0 /* TODO: it's possible that our va falls within the TILER addr range */
    P("/* Mapping a tiled 1D buffer */");
    void *ptr = alloc_1D(PAGE_SIZE * 2, 0, 0);
    dataPtr = (void *)(((uint32_t)ptr + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
    block[0].ptr = dataPtr;
    block[0].dim.len = PAGE_SIZE;
    ret |= NEGM(MemMgr_Map(block, 1));

    MemMgr_Free(ptr);
#endif

    return ret;
}

/**
 * Performs negative tests for MemMgr_UnMap.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure
 */
int neg_unmap_tests()
{
    printf("Negative Unmap tests\n");

    void *ptr = alloc_1D(PAGE_SIZE, 0, 0);
    int ret = 0;

    P("/* unmap alloced buffer */");
    ret |= NOT_I(MemMgr_UnMap(ptr),!=,0);

    MemMgr_Free(ptr);

    void *buffer = malloc(PAGE_SIZE * 2);
    void *dataPtr = (void *)(((uint32_t)buffer + PAGE_SIZE - 1) &~ (PAGE_SIZE - 1));
    ptr = map_1D(dataPtr, PAGE_SIZE, 0, 0);
    MemMgr_UnMap(ptr);

    P("/* unmap something twice */");
    ret |= NOT_I(MemMgr_UnMap(ptr),!=,0);

    P("/* unmap NULL */");
    ret |= NOT_I(MemMgr_UnMap(NULL),!=,0);

    P("/* unmap arbitrary value */");
    ret |= NOT_I(MemMgr_UnMap((void *)0x12345678),!=,0);

    return ret;
}

/**
 * Performs negative tests for MemMgr_Is.. functions.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure
 */
int neg_check_tests()
{
    printf("Negative Is... tests\n");
    void *ptr = malloc(32);

    int ret = 0;

    ret |= NOT_I(MemMgr_Is1DBlock(NULL),==,FALSE);
    ret |= NOT_I(MemMgr_Is1DBlock((void *)0x12345678),==,FALSE);
    ret |= NOT_I(MemMgr_Is1DBlock(ptr),==,FALSE);
    ret |= NOT_I(MemMgr_Is2DBlock(NULL),==,FALSE);
    ret |= NOT_I(MemMgr_Is2DBlock((void *)0x12345678),==,FALSE);
    ret |= NOT_I(MemMgr_Is2DBlock(ptr),==,FALSE);
    ret |= NOT_I(MemMgr_IsMapped(NULL),==,FALSE);
    ret |= NOT_I(MemMgr_IsMapped((void *)0x12345678),==,FALSE);
    ret |= NOT_I(MemMgr_IsMapped(ptr),==,FALSE);

    ret |= NOT_I(MemMgr_GetStride(NULL),==,0);
    ret |= NOT_I(MemMgr_GetStride((void *)0x12345678),==,0);
    ret |= NOT_I(MemMgr_GetStride(ptr),==,PAGE_SIZE);

    ret |= NOT_P(TilerMem_VirtToPhys(NULL),==,0);
    ret |= NOT_P(TilerMem_VirtToPhys((void *)0x12345678),==,0);
    ret |= NOT_P(TilerMem_VirtToPhys(ptr),!=,0);

    ret |= NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(NULL)),==,0);
    ret |= NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys((void *)0x12345678)),==,0);
    ret |= NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(ptr)),==,0);

    FREE(ptr);

    return ret;
}

DEFINE_TESTS(TESTS)

/**
 * We run the same identity check before and after running the
 * tests.
 *
 * @author a0194118 (9/12/2009)
 */
void memmgr_identity_test(void *ptr)
{
    /* also execute internal unit tests - this also verifies that we did not
       keep any references */
    __test__MemMgr();
}

/**
 * Main test function. Checks arguments for test case ranges,
 * runs tests and prints usage or test list if required.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param argc   Number of arguments
 * @param argv   Arguments
 *
 * @return -1 on usage or test list, otherwise # of failed
 *         tests.
 */
int main(int argc, char **argv)
{
    return TestLib_Run(argc, argv,
                       memmgr_identity_test, memmgr_identity_test, NULL);
}

