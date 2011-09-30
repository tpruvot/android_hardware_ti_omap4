/*
 *  tiler_ptest.c
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

#undef __WRITE_IN_STRIDE__
#undef STAR_TRACE_MEM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <ctype.h>

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

#define FALSE 0

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

enum ptr_type {
    ptr_empty = 0,
    ptr_alloced,
    ptr_tiler_alloced,
};

struct ptr_info {
	int num_blocks;
	struct tiler_block_info blocks[TILER_MAX_NUM_BLOCKS];
	int ptr;
    short type;
    uint16_t val;
};

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

static void dump_slot(struct ptr_info* buf, char* prefix)
{
    P("%sbuf={n=%d,ptr=0x%x,type=%d,", prefix, buf->num_blocks, buf->ptr,
      buf->type);
    int ix = 0;
    for (ix = 0; ix < buf->num_blocks; ix++)
    {
        dump_block(buf->blocks + ix, "", ix + 1 == buf->num_blocks ? "}" : "");
    }
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
 * This method allocates a tiled buffer composed of an arbitrary
 * set of tiled blocks. If successful, it checks
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
 * @param num_blocks  Number of blocks in the buffer
 * @param blocks      Block information
 * @param val         Fill start value
 * @param bufPtr      Pointer to the allocated buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
void *alloc_buf(int num_blocks, MemAllocBlock blocks[], uint16_t val)
{
    void *bufPtr = MemMgr_Alloc(blocks, num_blocks);
    void *ptr = bufPtr;
    int i;

    for (i = 0; i < num_blocks; i++)
    {
        if (bufPtr)
        {
            pixel_fmt_t fmt = blocks[i].pixelFormat;
            bytes_t cstride = (fmt == PIXEL_FMT_PAGE  ? PAGE_SIZE :
                               fmt == PIXEL_FMT_8BIT  ? TILER_STRIDE_8BIT :
                               fmt == PIXEL_FMT_16BIT ? TILER_STRIDE_16BIT :
                               TILER_STRIDE_32BIT);
            if (NOT_P(blocks[i].ptr,==,ptr) ||
                NOT_I(MemMgr_IsMapped(ptr),!=,0) ||
                NOT_I(MemMgr_Is1DBlock(ptr),==,fmt == PIXEL_FMT_PAGE ? 1 : 0) ||
                NOT_I(MemMgr_Is2DBlock(ptr),==,fmt == PIXEL_FMT_PAGE ? 0 : 1) ||
                NOT_I(MemMgr_GetStride(bufPtr),==,blocks[i].stride) ||
                NOT_P(TilerMem_VirtToPhys(ptr),==,blocks[i].reserved) ||
                NOT_I(TilerMem_GetStride(TilerMem_VirtToPhys(ptr)),==,cstride) ||
                NOT_L((PAGE_SIZE - 1) & (long)ptr,==,(PAGE_SIZE - 1) & blocks[i].reserved) ||
                (fmt == PIXEL_FMT_PAGE || NOT_I(blocks[i].stride,!=,0)))
            {
                P("  for block %d", i);
                MemMgr_Free(bufPtr);
                return NULL;
            }
            fill_mem(val, blocks + i);
            if (blocks[i].pixelFormat != PIXEL_FMT_PAGE)
            {
                ptr += def_stride(blocks[i].dim.area.width *
                                  def_bpp(blocks[i].pixelFormat)) * blocks[i].dim.area.height;
            }
            else
            {
                ptr += def_stride(blocks[i].dim.len);
            }
        }
        else
        {
            A_P(blocks[i].ptr,==,ptr);
            A_I(blocks[i].reserved,==,0);
        }
    }
    return bufPtr;
}

/**
 * This method frees a tiled buffer composed of an arbitrary set
 * of tiled blocks. The given start value is used to verify that
 * the buffer is still correctly filled.  In the event of any
 * errors, the error value is returned.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param num_blocks  Number of blocks in the buffer
 * @param blocks      Block information
 * @param val         Fill start value
 * @param bufPtr      Pointer to the allocated buffer
 *
 * @return 0 on success, non-0 error value on failure
 */
int free_buf(int num_blocks, MemAllocBlock blocks[], uint16_t val, void *bufPtr)
{
    MemAllocBlock blk;
    void *ptr = bufPtr;
    int ret = 0, i;
    for (i = 0; i < num_blocks; i++)
    {
        blk = blocks[i];
        blk.ptr = ptr;
        ERR_ADD(ret, check_mem(val, &blk));
        if (blk.pixelFormat != PIXEL_FMT_PAGE)
        {
            ptr += def_stride(blk.dim.area.width *
                              def_bpp(blk.pixelFormat)) * blk.dim.area.height;
        }
        else
        {
            ptr += def_stride(blk.dim.len);
        }
        blk.reserved = 0;
    }

    ERR_ADD(ret, MemMgr_Free(bufPtr));
    return ret;
}

#if 0
struct slot {
    int      op;
    SSPtr    ssptr;
    void    *buffer;
    void    *dataPtr;
} *slots = NULL;

enum op_enum {
    op_map_1d,
    op_alloc_1d,
    op_alloc_8,
    op_alloc_16,
    op_alloc_32,
    op_alloc_nv12,
    op_alloc_gen,
};

static int free_slot(int ix)
{
    if (slots[ix].bufPtr)
    {
        /* check memory fill */
        switch (slots[ix].op)
        {
        case op_map_1d:
            res = unmap_1D(slot[ix].dataPtr, slot[ix].length, 0, slot[ix].val, slot[ix].bufPtr);
            FREE(mem[ix].buffer);
            break;
        case op_alloc_1d:
            res = free_1D(slot[ix].length, 0, slot[ix].val, mem[ix].bufPtr);
            break;
        case op_alloc_8:
            res = free_2D(slot[ix].width, slot[ix].height, PIXEL_FMT_8BIT, 0, slot[ix].val, slot[ix].bufPtr);
            break;
        case op_alloc_16:
            res = free_2D(slot[ix].width, slot[ix].height, PIXEL_FMT_16BIT, 0, slot[ix].val, slot[ix].bufPtr);
            break;
        case op_alloc_32:
            res = free_2D(slot[ix].width, slot[ix].height, PIXEL_FMT_32BIT, 0, slot[ix].val, slot[ix].bufPtr);
            break;
        case op_alloc_nv12:
            res = free_NV12(slot[ix].width, slot[ix].height, slot[ix].val, slot[ix].bufPtr);
            break;
        }
        P("%s[%p]", mem[ix].op ? "free" : "unmap", mem[ix].bufPtr);
        ZERO(slot[ix]);
    }
}
#endif

#include <tilermgr.h>

char *parse_num(char *p, int *tgt)
{
	int len;
	if (!strncmp(p, "0x", 2)) { /* hex number */
		if (NOT_I(sscanf(p, "0x%x%n", tgt, &len),==,1))
			return NULL;
		return p + len;
	} else {
		if (NOT_I(sscanf(p, "%d%n", tgt, &len),==,1))
			return NULL;
		return p + len;
	}
}

/**
 * Parametric memmgr test.  This is similar to the star test
 * except the operations are read from the command line:
 *
 * [#=]a:w*h*bits[,w*h*bits...] allocates a list of blocks as
 * buffer # (it frees any previously allocated/mapped buffer
 * f:# frees a buffer
 *
 *
 * @author a0194118 (11/4/2009)
 *
 * @param argc
 * @param argv
 */
int param_test(int argc, char **argv)
{
    int delta_slots = 16;
	int ix, i, n, t, type, max_n, num_slots = delta_slots;
    struct ptr_info *slots;
    ALLOCN(slots, num_slots);
    if (NOT_P(slots,!=,NULL)) return 1;

    int res = TilerMgr_Open();
    for (i = 1; i < argc && !res; i++)
	{
        uint16_t val = (uint16_t) rand();

		char *p = argv[i], *q;
        struct ptr_info buf;
        ZERO(buf);

		/* read slot */
        ix = -1;
        if (isdigit(*p))
        {
            res = 1;
            q = parse_num(p, &ix);
            if (NOT_P(q,!=,NULL) || NOT_I(ix,>,0)) break;
            p = q;
            if (NOT_I(*p++,==,'.')) break;
            res = 0;
            ix--;
        }

        type = *p++;
        /* get default slot */
        if (ix < 0)
        {
            switch (type)
            {
            /* allocation defaults to the 1st free slot */
            case 'a':
            case 'A':
                for (ix = 0; ix < num_slots && slots[ix].type; ix++);
                break;

            /* frees default to the 1st used block */
            case 'f':
            case 'F':
                for (ix = 0; ix < num_slots && !slots[ix].type; ix++);
                if (NOT_I(ix,<,num_slots)) res = 1;
                break;
            }
        }
        if (res) break;

        /* allocate more slots if needed */
        if (ix >= num_slots)
        {
            int more_slots = ROUND_UP_TO(ix + 1, delta_slots);
            struct ptr_info *new_slots;
            ALLOCN(new_slots, more_slots);
            if (NOT_P(new_slots,!=,NULL)) break;
            memcpy(new_slots, slots, sizeof(*slots) * num_slots);
            FREE(slots);
            slots = new_slots;
            num_slots = more_slots;
        }

        /* perform opertaion */
        res = 1;  /* assume failure */
		switch (type)
		{
        case 'a': /* allocate */
        case 'A': /* tiler-allocate */
            switch (type)
            {
            case 'a': buf.type = ptr_alloced; max_n = TILER_MAX_NUM_BLOCKS; break;
            case 'A': buf.type = ptr_tiler_alloced; max_n = 1; break;
            }
            if (ix < num_slots && NOT_I(slots[ix].type,==,ptr_empty)) break;
			if (NOT_I(*p++,==,':')) break;
            for (n = 0; *p && n < max_n; n++) {
                /* read length or width */
                p = parse_num(p, (int *) &buf.blocks[n].dim.len);
                if (NOT_P(p,!=,NULL)) break;
                if (*p == '*') { /* 2d block */
                    buf.blocks[n].dim.area.width = (uint16_t) buf.blocks[n].dim.len;
                    /* read height */
                    p = parse_num(++p, &t);
                    if (NOT_P(p,!=,NULL) || NOT_I(*p++,==,'*')) break;
                    buf.blocks[n].dim.area.height = (uint16_t) t;
                    /* read bits */
                    p = parse_num(p, &t);
                    if (NOT_P(p,!=,NULL)) break;
                    /* handle nv12 */
                    if (t == 12 && n + 1 < max_n) {
                        buf.blocks[n + 1].dim.area.width = buf.blocks[n].dim.area.width >> 1;
                        buf.blocks[n + 1].dim.area.height = buf.blocks[n].dim.area.height >> 1;
                        buf.blocks[n].fmt = TILFMT_8BIT;
                        t = 16;
                        n++;
                    }

                    buf.blocks[n].fmt = (t == 8 ? TILFMT_8BIT :
                                         t == 16 ? TILFMT_16BIT :
                                         t == 32 ? TILFMT_32BIT : TILFMT_INVALID);
                    if (NOT_I(buf.blocks[n].fmt,!=,TILFMT_INVALID)) break;
                } else { /* 1d block */
                    buf.blocks[n].fmt = TILFMT_PAGE;
                }
                if (*p && NOT_I(*p++,==,',')) break;
                /* we're OK */
                res = 0;
            }
            if (res || *p) break;
            /* allocate buffer */

            buf.num_blocks = n;
            buf.val = val;
            if (buf.type == ptr_alloced)
            {
                dump_slot(&buf, "==(alloc)=>");
                buf.ptr = (int) alloc_buf(n, (MemAllocBlock *) buf.blocks, val);
                dump_slot(&buf, "<=(alloc)==");
            }
            else
            {
                dump_slot(&buf, "==(tiler_alloc)=>");
                if (buf.blocks[0].fmt == TILFMT_PAGE)
                {
                    buf.ptr = (int) TilerMgr_PageModeAlloc(buf.blocks[0].dim.len);
                }
                else
                {
                    buf.ptr =(int)  TilerMgr_Alloc(buf.blocks[0].fmt,
                                                   buf.blocks[0].dim.area.width,
                                                   buf.blocks[0].dim.area.height);
                }
                buf.blocks[0].ssptr = (unsigned long) buf.ptr;
                dump_slot(&buf, "<=(tiler_alloc)==");
            }
            if (NOT_I(buf.ptr,!=,0)) res = 1;
            else memcpy(slots + ix, &buf, sizeof(buf));
			break;

        case 'f': /* free */
        case 'F': /* tiler-free */
            memcpy(&buf, slots + ix, sizeof(buf));
            switch (type)
            {
            case 'f':
                if (NOT_I(buf.type,==,ptr_alloced)) break;
                dump_slot(&buf, "==(free)=>");
                res = free_buf(buf.num_blocks, (MemAllocBlock *) buf.blocks,
                         buf.val, (void *) buf.ptr);
                P("<=(free)==: %d", res);
                break;
            case 'F':
                if (NOT_I(buf.type,==,ptr_tiler_alloced)) break;
                dump_slot(&buf, "==(tiler_free)=>");
                if (buf.blocks[0].fmt == TILFMT_PAGE)
                {
                    res = TilerMgr_PageModeFree((SSPtr) buf.ptr);
                }
                else
                {
                    res = TilerMgr_Free((SSPtr) buf.ptr);
                }
                P("<=(tiler_free)==: %d", res);
                break;
            }
            ZERO(slots[ix]);
            break;
        }
    }

    /* free any memmgr allocated blocks */
    for (ix = 0; ix < num_slots; ix++)
    {
        if (slots[ix].type == ptr_alloced)
        {
            dump_slot(slots + ix, "==(free)=>");
            int res_free = free_buf(slots[ix].num_blocks, (MemAllocBlock *) slots[ix].blocks,
                                    slots[ix].val, (void *) slots[ix].ptr);
            P("<=(free)==: %d", res_free);
            ERR_ADD(res, res_free);
        }
    }
    ERR_ADD(res, TilerMgr_Close());

    FREE(slots);
	return res;
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
    int res = param_test(argc, argv);
    P(res ? "FAILURE: %d" : "SUCCESS", res);
    return res;
}

