/*
 *  memmgr.c
 *
 *  Memory Allocator Interface functions for TI OMAP processors.
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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>

#define BUF_ALLOCED 1
#define BUF_MAPPED  2
#define BUF_ANY     ~0

#include <tiler.h>

typedef struct tiler_block_info tiler_block_info;

#define __DEBUG__
#undef  __DEBUG_ENTRY__
#define __DEBUG_ASSERT__

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif
#include "utils.h"
#include "list_utils.h"
#include "debug_utils.h"
#include "tilermem.h"
#include "tilermem_utils.h"
#include "memmgr.h"

/* list of allocations */
struct _AllocData {
    struct tiler_buf_info buf;
    int       buf_type;
    struct _AllocList {
        struct _AllocList *next, *last;
        struct _AllocData *me;
    } link;
};
static struct _AllocList bufs = {0};
static int bufs_inited = 0;

typedef struct _AllocList _AllocList;
typedef struct _AllocData _AllocData;

static int refCnt = 0;
static int td = -1;
static pthread_mutex_t ref_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t che_mutex = PTHREAD_MUTEX_INITIALIZER;

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
 * Increases the reference count.  Initialized tiler if this was
 * the first reference
 *
 * @author a0194118 (9/2/2009)
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int inc_ref()
{
    /* initialize tiler on first call */
    pthread_mutex_lock(&ref_mutex);

    int res = MEMMGR_ERR_NONE;

    if (!refCnt++) {
        /* initialize lists */
        init();
#ifndef STUB_TILER
        td = open("/dev/tiler", O_RDWR | O_SYNC);
        if (NOT_I(td,>=,0)) res = MEMMGR_ERR_GENERIC;
#else
        td = 2;
        res = MEMMGR_ERR_NONE;
#endif
    }
    if (res)
    {
        refCnt--;
    }

    pthread_mutex_unlock(&ref_mutex);
    return res;
}

/**
 * Decreases the reference count.  Deinitialized tiler if this
 * was the last reference
 *
 * @author a0194118 (9/2/2009)
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int dec_ref()
{
    pthread_mutex_lock(&ref_mutex);

    int res = MEMMGR_ERR_NONE;;

    if (refCnt <= 0) res = MEMMGR_ERR_GENERIC;
    else if (!--refCnt) {
#ifndef STUB_TILER
        close(td);
        td = -1;
#endif
    }

    pthread_mutex_unlock(&ref_mutex);
    return res;
}

/**
 * Returns the default page stride for this block
 *
 * @author a0194118 (9/4/2009)
 *
 * @param width  Width of 2D container in bytes
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
 * Returns the size of the supplied block
 *
 * @author a0194118 (9/4/2009)
 *
 * @param blk    Pointer to the tiler_block_info struct
 *
 * @return size of the block in bytes
 */
static bytes_t def_size(tiler_block_info *blk)
{
    return (blk->fmt == TILFMT_PAGE ?
            def_stride(blk->offs + blk->dim.len) :
            blk->dim.area.height * def_stride(blk->offs + blk->dim.area.width * def_bpp(blk->fmt)));
}

/**
 * Returns the map size based on an offset and buffer size
 *
 * @author a0194118 (7/8/2010)
 *
 * @param size   Size of buffer
 * @param offs   Buffer offset
 *
 * @return (page aligned) size for mapping
 */
static bytes_t map_size(bytes_t size, bytes_t offs)
{
    return def_stride(size + (offs & (PAGE_SIZE - 1)));
}

/**
 * Records a buffer-pointer -- tiler-ID mapping for a specific
 * buffer type.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param bufPtr    Buffer pointer
 * @param tiler_id  Tiler ID
 * @param buf_type  Buffer type: BUF_ALLOCED or BUF_MAPPED
 *
 * @return 0 on success, -ENOMEM on memory allocation failure
 */
static int buf_cache_add(struct tiler_buf_info *buf, int buf_type)
{
    pthread_mutex_lock(&che_mutex);
    _AllocData *ad = NEW(_AllocData);
    if (ad)
    {
        memcpy(&ad->buf, buf, sizeof(ad->buf));
        ad->buf_type = buf_type;
        DLIST_MADD_BEFORE(bufs, ad, link);
    }
    pthread_mutex_unlock(&che_mutex);
    return ad == NULL ? -ENOMEM : 0;
}

/**
 * Retrieves the tiler ID for given pointer and buffer type from
 * the records.  If the pointer lies within a tracked buffer,
 * the tiler ID is returned.  Otherwise 0 is returned.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param bufPtr    Buffer pointer
 * @param buf_type  Buffer type: BUF_ALLOCED or BUF_MAPPED
 *
 * @return Tiler ID on success, 0 on failure.
 */
static void buf_cache_query(void *ptr, struct tiler_buf_info *buf,
                            int buf_type_mask)
{
    IN;
    if(0) DP("in(p=%p,t=%d,bp*=%p)", ptr, buf_type_mask, buf->blocks[0].ptr);
    _AllocData *ad;
    pthread_mutex_lock(&che_mutex);
    DLIST_MLOOP(bufs, ad, link) {
        if(0) {
            DP("got(%p-%p,%d)", ad->buf.blocks->ptr, ad->buf.blocks->ptr + ad->buf.length, ad->buf_type);
            CHK_P(ad->buf.blocks->ptr,<=,ptr);
            CHK_P(ptr,<,ad->buf.blocks->ptr + ad->buf.length);
            CHK_I(ad->buf_type & buf_type_mask,!=,0);
            CHK_P(buf->blocks->ptr,!=,0);
        }
        if ((ad->buf_type & buf_type_mask) &&
            ad->buf.blocks->ptr <= ptr && ptr < ad->buf.blocks->ptr + ad->buf.length) {
            memcpy(buf, &ad->buf, sizeof(*buf));
            pthread_mutex_unlock(&che_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&che_mutex);
    ZERO(*buf);
    OUT;
}

/**
 * Retrieves the tiler ID for given buffer pointer and buffer
 * type from the records.  If the tiler ID is found, it is
 * removed from the records as well.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param bufPtr    Buffer pointer
 * @param buf_type  Buffer type: BUF_ALLOCED or BUF_MAPPED
 *
 * @return Tiler ID on success, 0 on failure.
 */
static void buf_cache_del(void *bufPtr, struct tiler_buf_info *buf,
                          int buf_type)
{
    _AllocData *ad;
    pthread_mutex_lock(&che_mutex);
    DLIST_MLOOP(bufs, ad, link) {
        if (ad->buf.blocks->ptr == bufPtr && ad->buf_type == buf_type) {
            memcpy(buf, &ad->buf, sizeof(*buf));
            DLIST_REMOVE(ad->link);
            FREE(ad);
            pthread_mutex_unlock(&che_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&che_mutex);
    OUT;
    return;
}

/**
 * Checks the consistency of the internal record cache.  The
 * number of elements in the cache should equal to the number of
 * references.
 *
 * @author a0194118 (9/7/2009)
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int cache_check()
{
    int num_bufs = 0;
    pthread_mutex_lock(&che_mutex);

    init();

    _AllocData *ad;
    DLIST_MLOOP(bufs, ad, link) { num_bufs++; }

    pthread_mutex_unlock(&che_mutex);
    return (num_bufs == refCnt) ? MEMMGR_ERR_NONE : MEMMGR_ERR_GENERIC;
}

static void dump_block(struct tiler_block_info *blk, char *prefix, char *suffix)
{
#if 0
    switch (blk->fmt)
    {
    case TILFMT_PAGE:
        P("%s [%d:(%d,%08x), p=%p(0x%x),l=0x%x,s=%d,%d+%d]%s", prefix, blk->group_id, blk->key, blk->id, blk->ptr, blk->ssptr,
          blk->dim.len, blk->stride, blk->align, blk->offs, suffix);
        break;
    case TILFMT_8BIT:
    case TILFMT_16BIT:
    case TILFMT_32BIT:
        P("%s [%d:(%d,%08x), p=%p(0x%x),%d*%d*%d,s=%d,%d+%d]%s", prefix, blk->group_id, blk->key, blk->id, blk->ptr, blk->ssptr,
          blk->dim.area.width, blk->dim.area.height, def_bpp(blk->fmt) * 8,
          blk->stride, blk->align, blk->offs, suffix);
        break;
    default:
        P("%s*[%d:(%d,%08x), p=%p(0x%x),l=0x%x,s=%d,%d+%d,fmt=0x%x]%s", prefix, blk->group_id, blk->key, blk->id, blk->ptr,
          blk->ssptr, blk->dim.len, blk->stride, blk->align, blk->offs, blk->fmt, suffix);
    }
#endif
}

static void dump_buf(struct tiler_buf_info* buf, char* prefix)
{
#if 0
    P("%sbuf={n=%d,id=0x%x,len=0x%x", prefix, buf->num_blocks, buf->offset, buf->length);
    int ix = 0;
    for (ix = 0; ix < buf->num_blocks; ix++)
    {
        dump_block(buf->blocks + ix, "", ix + 1 == buf->num_blocks ? "}" : "");
    }
#endif
}

/**
 * Returns the tiler format for an address
 *
 * @author a0194118 (9/7/2009)
 *
 * @param ssptr   Address
 *
 * @return The tiler format
 */
static enum tiler_fmt tiler_get_fmt(SSPtr ssptr)
{
#ifndef STUB_TILER
    return (ssptr == 0              ? TILFMT_INVALID :
            ssptr < TILER_MEM_8BIT  ? TILFMT_NONE :
            ssptr < TILER_MEM_16BIT ? TILFMT_8BIT :
            ssptr < TILER_MEM_32BIT ? TILFMT_16BIT :
            ssptr < TILER_MEM_PAGED ? TILFMT_32BIT :
            ssptr < TILER_MEM_END   ? TILFMT_PAGE : TILFMT_NONE);
#else
    /* if emulating, we need to get through all allocated memory segments */
    pthread_mutex_lock(&che_mutex);
    init();
    _AllocData *ad;
    void *ptr = (void *) ssptr;
    if (!ptr) return TILFMT_INVALID;
    /* P("?%p", (void *)ssptr); */
    DLIST_MLOOP(bufs, ad, link) {
        int ix;
        struct tiler_buf_info *buf = (struct tiler_buf_info *) ad->buf.offset;
        /* P("buf[%d]", buf->num_blocks); */
        for (ix = 0; ix < buf->num_blocks; ix++)
        {
            /* P("block[%p-%p]", buf->blocks[ix].ptr, buf->blocks[ix].ptr + def_size(buf->blocks + ix)); */
            if (ptr >= buf->blocks[ix].ptr &&
                ptr < buf->blocks[ix].ptr + def_size(buf->blocks + ix)) {
                enum tiler_fmt fmt = buf->blocks[ix].fmt;
                pthread_mutex_unlock(&che_mutex);
                return fmt;
            }
        }
    }
    pthread_mutex_unlock(&che_mutex);
    return TILFMT_NONE;
#endif
}


/**
 * Allocates a memory block using tiler
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blk    Pointer to the block info
 *
 * @return ssptr of block allocated, or 0 on error
 */
static int tiler_alloc(struct tiler_block_info *blk)
{
    dump_block(blk, "=(ta)=>", "");
    blk->ptr = NULL;
    int ret = A_S(ioctl(td, TILIOC_GBLK, blk),==,0);
    dump_block(blk, "alloced: ", "");
    return R_I(ret);
}

/**
 * Frees a memory block using tiler
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blk    Pointer to the block info
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int tiler_free(struct tiler_block_info *blk)
{
    return R_I(ioctl(td, TILIOC_FBLK, blk));
}

/**
 * Maps a memory block into tiler using tiler
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blk    Pointer to the block info
 *
 * @return ssptr of block mapped, or 0 on error
 */
static int tiler_map(struct tiler_block_info *blk)
{
    dump_block(blk, "=(tm)=>", "");
    int ret = A_S(ioctl(td, TILIOC_MBLK, blk),==,0);
    dump_block(blk, "mapped: ", "");
    return R_I(ret);
}

/**
 * Unmaps a memory block from tiler using tiler
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blk    Pointer to the block info
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int tiler_unmap(struct tiler_block_info *blk)
{
    return ioctl(td, TILIOC_UMBLK, blk);
}

/**
 * Registers a buffer structure with tiler, and maps the buffer
 * into memory using tiler. On success, it writes the tiler ID
 * of the buffer into the area pointed by tiler ID.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blks        Pointer to array of block info structures
 * @param num_blocks  Number of blocks
 * @param tiler_id      Pointer to tiler ID.
 *
 * @return pointer to the mapped buffer.
 */
static void *tiler_mmap(struct tiler_block_info *blks, int num_blocks,
                        int buf_type)
{
    IN;

    /* get size */
    int ix;
    bytes_t size;

    /* register buffer with tiler */
    struct tiler_buf_info buf;
    buf.num_blocks = num_blocks;
    /* work on copy in buf */
    memcpy(buf.blocks, blks, sizeof(tiler_block_info) * num_blocks);
#ifndef STUB_TILER
    dump_buf(&buf, "==(RBUF)=>");
    int ret = ioctl(td, TILIOC_RBUF, &buf);
    dump_buf(&buf, "<=(RBUF)==");
    if (NOT_I(ret,==,0)) return NULL;
    size = buf.length;
#else
    /* save buffer in stub */
    struct tiler_buf_info *buf_c = NEWN(struct tiler_buf_info,2);
    buf.offset = (uint32_t) buf_c;
#endif
    if (NOT_P(buf.offset,!=,0)) return NULL;

    /* map blocks to process space */
#ifndef STUB_TILER
    void *bufPtr = mmap(0, map_size(size, buf.offset),
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        td, buf.offset & ~(PAGE_SIZE - 1));
    if (bufPtr == MAP_FAILED){
        bufPtr = NULL;
    } else {
        bufPtr += buf.offset & (PAGE_SIZE - 1);
    }
    if(0) DP("ptr=%p", bufPtr);
#else
    void *bufPtr = malloc(size + PAGE_SIZE - 1);
    buf_c[1].blocks[0].ptr = bufPtr;
    bufPtr = ROUND_UP_TO2POW(bufPtr, PAGE_SIZE);
    /* P("<= [0x%x]", size); */

    /* fill out pointers - this is needed for caching 1D/2D type */
    for (size = ix = 0; ix < num_blocks; ix++)
    {
        buf.blocks[ix].ptr = bufPtr ? bufPtr + size : bufPtr;
        size += def_size(blks + ix);
    }

    memcpy(buf_c, &buf, sizeof(struct tiler_buf_info));
#endif

    if (bufPtr != NULL)
    {
        /* fill out pointers */
        for (size = ix = 0; ix < num_blocks; ix++)
        {
            buf.blocks[ix].ptr = bufPtr + size;
            /* P("   [0x%p]", buf.blocks[ix].ptr); */
            size += def_size(blks + ix);
#ifdef STUB_TILER
            buf.blocks[ix].ssptr = (uint32_t) buf.blocks[ix].ptr;
#else
            CHK_I((buf.blocks[ix].ssptr & (PAGE_SIZE - 1)),==,((uint32_t) buf.blocks[ix].ptr & (PAGE_SIZE - 1)));
#endif
        }
    }
    /* if failed to map: unregister buffer */
    if (NOT_P(bufPtr,!=,NULL) ||
        /* or failed to cache tiler buffer */
        NOT_I(buf_cache_add(&buf, buf_type),==,0))
    {
#ifndef STUB_TILER
        A_I(ioctl(td, TILIOC_URBUF, &buf),==,0);
#else
        FREE(buf_c);
        buf.offset = 0;
#endif
    } else {
        /* update original from local copy */
        memcpy(blks, buf.blocks, sizeof(tiler_block_info) * num_blocks);
    }

    return R_P(bufPtr);
}

/**
 * Checks whether the tiler_block_info is filled in correctly.
 * Verifies the pixel format, correct length, width and or
 * height, the length/stride relationship for 1D buffers, and
 * the correct stride for 2D buffers.  Also verifies block size
 * to be page sized if desired.
 *
 * @author a0194118 (9/4/2009)
 *
 * @param blk            Pointer to the tiler_block_info struct
 * @param is_page_sized  Whether the block needs to be page
 *                       sized (fit on whole pages).
 * @return 0 on success, non-0 error value on failure.
 */
static int check_block(tiler_block_info *blk, bool is_page_sized)
{
    /* check pixelformat */
    if (NOT_I(blk->fmt,>=,PIXEL_FMT_MIN) ||
        NOT_I(blk->fmt,<=,PIXEL_FMT_MAX)) return MEMMGR_ERR_GENERIC;

    /* check alignment */
    if (NOT_I(blk->align & (blk->align - 1),==,0) ||
        NOT_I(blk->align,<=,PAGE_SIZE) ||
        NOT_I(blk->offs,<,PAGE_SIZE))
        return MEMMGR_ERR_GENERIC;

    /* check the offset */
    if (blk->offs && blk->align && NOT_I(blk->offs,<,blk->align))
        return MEMMGR_ERR_GENERIC;

    if (blk->fmt == TILFMT_PAGE)
    {   /* check 1D buffers */

        /* length must be multiple of stride if stride > 0 */
        if (NOT_I(blk->dim.len,>,0) ||
            (blk->stride && NOT_I(blk->dim.len % blk->stride,==,0)))
            return MEMMGR_ERR_GENERIC;
    }
    else
    {   /* check 2D buffers */

        /* check width, height and stride (must be the default stride or 0) */
        bytes_t stride = def_stride(blk->dim.area.width * def_bpp(blk->fmt));
        if (NOT_I(blk->dim.area.width,>,0) ||
            NOT_I(blk->dim.area.height,>,0) ||
            (blk->stride && NOT_I(blk->stride,==,stride)))
            return MEMMGR_ERR_GENERIC;
    }

    if (is_page_sized && NOT_I(def_size(blk) & (PAGE_SIZE - 1),==,0))
        return MEMMGR_ERR_GENERIC;

    return MEMMGR_ERR_NONE;
}

/**
 * Checks whether the block information is correct for map and
 * alloc operations.  Checks the number of blocks, and validity
 * of each block.  Warns if reserved member is not 0.  Also
 * checks if the alignment/offset requirements are consistent
 * across the buffer
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blks                 Pointer to the block info array.
 * @param num_blocks           Number of blocks.
 * @param num_pagesize_blocks  Number of blocks that must be
 *                             page sized (these must be in
 *                             front)
 *
 * @return 0 on success, non-0 error value on failure.
 */
static int check_blocks(struct tiler_block_info *blks, int num_blocks,
                 int num_pagesize_blocks)
{
    uint32_t align = 0, offset = 0;

    /* check arguments */
    if (NOT_I(num_blocks,>,0) ||
        NOT_I(num_blocks,<=,TILER_MAX_NUM_BLOCKS)) return MEMMGR_ERR_GENERIC;

    /* check block allocation params */
    int ix;
    for (ix = 0; ix < num_blocks; ix++)
    {
        struct tiler_block_info *blk = blks + ix;
        CHK_I(blk->ssptr,==,0);
        CHK_I(blk->id,==,0);
        int ret = check_block(blk, ix < num_pagesize_blocks);

        /* check alignment */
        if (!ret)
        {
            /* set minimum alignment if none specified */
            if (!blk->align && blk->offs)
                while(align < blk->offs)
                    align <<= 1;

            /* offsets must be the same up to the smaller alignment */
            if ((offset || align) &&
                ((blk->offs ^ offset) & (align - 1) & (blk->align - 1)))
            {
                P("Incompatible offset: %x/%x & %x/%x\n", blk->offs, blk->align,
                  offset, align);
                ret = MEMMGR_ERR_GENERIC;
            }
            else
            {
                /* update offset if alignment increases (offset cannot
                   decrease) */
                if (blk->align > align)
                    align = blk->align;
                if (blk->offs > offset)
                    offset = blk->offs;
            }
        }
        if (ret)
        {
            DP("for block[%d]", ix);
            return ret;
        }


    }

    /* set alignment parameters */
    blks->align = align;
    blks->offs  = offset;
    return MEMMGR_ERR_NONE;
}

/**
 * Resets the ptr and reserved fields of the block info
 * structures.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param blks                 Pointer to the block info array.
 * @param num_blocks           Number of blocks.
 */
static void reset_blocks(struct tiler_block_info *blks, int num_blocks)
{
    int ix;
    for (ix = 0; ix < num_blocks; ix++)
    {
        blks[ix].ssptr = 0;
        blks[ix].id = 0;
        blks[ix].ptr = NULL;
    }

}

bytes_t MemMgr_PageSize()
{
    return PAGE_SIZE;
}

void *MemMgr_Alloc(MemAllocBlock blocks[], int num_blocks)
{
    IN;
    void *bufPtr = NULL;

    /* need to access ssptrs */
    struct tiler_block_info *blks = (tiler_block_info *) blocks;

    /* check block allocation params, and state */
    if (NOT_I(check_blocks(blks, num_blocks, num_blocks - 1),==,0) ||
        NOT_I(inc_ref(),==,0)) goto DONE;

    /* ----- begin recoverable portion ----- */
    int ix;

    /* allocate each buffer using tiler driver and initialize block info */
    for (ix = 0; ix < num_blocks; ix++)
    {
        if (ix)
        {
            /* continue offset between pages */
            blks[ix].align = PAGE_SIZE;
            blks[ix].offs = blks[ix - 1].offs;
        }
        CHK_I(blks[ix].ptr,==,NULL);
        if (NOT_I(tiler_alloc(blks + ix),==,0)) goto FAIL_ALLOC;
    }

    bufPtr = tiler_mmap(blks, num_blocks, BUF_ALLOCED);
    if (A_P(bufPtr,!=,0)) goto DONE;

    /* ------ error handling ------ */
FAIL_ALLOC:
    while (ix)
    {
        tiler_free(blks + --ix);
    }

    /* clear ssptr and ptr fields for all blocks */
    reset_blocks(blks, num_blocks);

    A_I(dec_ref(),==,0);
DONE:
    CHK_I(cache_check(),==,0);
    return R_P(bufPtr);
}

int MemMgr_Free(void *bufPtr)
{
    IN;

    int ret = MEMMGR_ERR_GENERIC;
    struct tiler_buf_info buf;
    ZERO(buf);

    /* retrieve registered buffers from vsptr */
    /* :NOTE: if this succeeds, Memory Allocator stops tracking this buffer */
    buf_cache_del(bufPtr, &buf, BUF_ALLOCED);

    if (A_L(buf.offset,!=,0))
    {
#ifndef STUB_TILER
        dump_buf(&buf, "==(URBUF)=>");
        ret = A_I(ioctl(td, TILIOC_URBUF, &buf),==,0);
        dump_buf(&buf, "<=(URBUF)==");

        /* free each block */
        int ix;
        for (ix = 0; ix < buf.num_blocks; ix++)
        {
            ERR_ADD(ret, tiler_free(buf.blocks + ix));
        }

        /* unmap buffer */
        bufPtr = (void *)((uint32_t)bufPtr & ~(PAGE_SIZE - 1));
        ERR_ADD(ret, munmap(bufPtr, map_size(buf.length, buf.offset)));
#else
        void *ptr = (void *) buf.offset;
        FREE(ptr);
        ret = MEMMGR_ERR_NONE;
#endif
        ERR_ADD(ret, dec_ref());
    }

    CHK_I(cache_check(),==,0);
    return R_I(ret);
}

void *MemMgr_Map(MemAllocBlock blocks[], int num_blocks)
{
    IN;
    void *bufPtr = NULL;

    /* need to access ssptrs */
    struct tiler_block_info *blks = (tiler_block_info *) blocks;

    /* check block params, and state */
    if (check_blocks(blks, num_blocks, num_blocks) ||
        NOT_I(inc_ref(),==,0)) goto DONE;

    /* we only map 1 page aligned 1D buffer for now */
    if (NOT_I(num_blocks,==,1) ||
        NOT_I(blocks[0].pixelFormat,==,PIXEL_FMT_PAGE) ||
        NOT_I(blocks[0].dim.len & (PAGE_SIZE - 1),==,0) ||
#ifdef STUB_TILER
        NOT_I(MemMgr_IsMapped(blocks[0].ptr),==,0) ||
#endif
        NOT_I((uint32_t)blocks[0].ptr & (PAGE_SIZE - 1),==,0))
        goto FAIL;

    /* ----- begin recoverable portion ----- */
    int ix;

    /* allocate each buffer using tiler driver */
    for (ix = 0; ix < num_blocks; ix++)
    {
        if (ix)
        {
            /* continue offset between pages */
            blks[ix].align = PAGE_SIZE;
            blks[ix].offs = blks[ix - 1].offs;
        }
        if (NOT_I(blks[ix].ptr,!=,NULL) ||
            NOT_I(tiler_map(blks + ix),==,0)) goto FAIL_MAP;
    }

    /* map bufer into tiler space and register with tiler manager */
    bufPtr = tiler_mmap(blks, num_blocks, BUF_MAPPED);
    if (A_P(bufPtr,!=,0)) goto DONE;

    /* ------ error handling ------ */
FAIL_MAP:
    while (ix)
    {
        tiler_unmap(blks + --ix);
    }

FAIL:
    /* clear ssptr and ptr fields for all blocks */
    reset_blocks(blks, num_blocks);

    A_I(dec_ref(),==,0);
DONE:
    CHK_I(cache_check(),==,0);
    return R_P(bufPtr);
}

int MemMgr_UnMap(void *bufPtr)
{
    IN;

    int ret = MEMMGR_ERR_GENERIC;
    struct tiler_buf_info buf;
    ZERO(buf);

    /* retrieve registered buffers from vsptr */
    /* :NOTE: if this succeeds, Memory Allocator stops tracking this buffer */
    buf_cache_del(bufPtr, &buf, BUF_MAPPED);

    if (A_L(buf.offset,!=,0))
    {
#ifndef STUB_TILER
        dump_buf(&buf, "==(URBUF)=>");
        ret = A_I(ioctl(td, TILIOC_URBUF, &buf),==,0);
        dump_buf(&buf, "<=(URBUF)==");

        /* unmap each block */
        int ix;
        for (ix = 0; ix < buf.num_blocks; ix++)
        {
            ERR_ADD(ret, tiler_unmap(buf.blocks + ix));
        }

        /* unmap buffer */
        bufPtr = (void *)((uint32_t)bufPtr & ~(PAGE_SIZE - 1));
        ERR_ADD(ret, munmap(bufPtr, map_size(buf.length, buf.offset)));
#else
        struct tiler_buf_info *ptr = (struct tiler_buf_info *) buf.offset;
        FREE(ptr[1].blocks[0].ptr);
        FREE(ptr);
        ret = MEMMGR_ERR_NONE;
#endif
        ERR_ADD(ret, dec_ref());
    }

    CHK_I(cache_check(),==,0);
    return R_I(ret);
}

bool MemMgr_Is1DBlock(void *ptr)
{
    IN;

    SSPtr ssptr = TilerMem_VirtToPhys(ptr);
    enum tiler_fmt fmt = tiler_get_fmt(ssptr);
    return R_I(fmt == TILFMT_PAGE);
}

bool MemMgr_Is2DBlock(void *ptr)
{
    IN;

    SSPtr ssptr = TilerMem_VirtToPhys(ptr);
    enum tiler_fmt fmt = tiler_get_fmt(ssptr);
    return R_I(fmt == TILFMT_8BIT || fmt == TILFMT_16BIT ||
               fmt == TILFMT_32BIT);
}

bool MemMgr_IsMapped(void *ptr)
{
    IN;
    SSPtr ssptr = TilerMem_VirtToPhys(ptr);
    enum tiler_fmt fmt = tiler_get_fmt(ssptr);
    return R_I(fmt == TILFMT_8BIT || fmt == TILFMT_16BIT ||
               fmt == TILFMT_32BIT || fmt == TILFMT_PAGE);
}

bytes_t MemMgr_GetStride(void *ptr)
{
    IN;
#ifndef STUB_TILER
    struct tiler_buf_info buf;
    ZERO(buf);

    /* find block that this buffer belongs to */
    buf_cache_query(ptr, &buf, BUF_ALLOCED | BUF_MAPPED);
    void *bufPtr = buf.blocks[0].ptr;

    A_I(inc_ref(),==,0);

    /* for tiler mapped buffers, get saved stride information */
    if (buf.offset)
    {
        /* walk through block to determine which stride we need */
        int ix;
        for (ix = 0; ix < buf.num_blocks; ix++)
        {
            bytes_t size = def_size(buf.blocks + ix);
            if (bufPtr <= ptr && ptr < bufPtr + size) {
                A_I(dec_ref(),==,0);
                return R_UP(buf.blocks[ix].stride);
            }
            bufPtr += size;
        }
        A_I(dec_ref(),==,0);
        DP("assert: should not ever get here");
        return R_UP(0);
    }
    /* see if pointer is valid */
    else if (TilerMem_VirtToPhys(ptr) == 0)
    {
        A_I(dec_ref(),==,0);
        return R_UP(0);
    }
    A_I(dec_ref(),==,0);
#else
    /* if emulating, we need to get through all allocated memory segments */
    pthread_mutex_lock(&che_mutex);
    init();

    _AllocData *ad;
    if (!ptr) return R_UP(0);
    DLIST_MLOOP(bufs, ad, link) {
        int ix;
        struct tiler_buf_info *buf = (struct tiler_buf_info *) ad->buf.offset;
        for (ix = 0; ix < buf->num_blocks; ix++)
        {
            if (ptr >= buf->blocks[ix].ptr &&
                ptr < buf->blocks[ix].ptr + def_size(buf->blocks + ix))
            {
                bytes_t stride = buf->blocks[ix].stride;
                pthread_mutex_unlock(&che_mutex);
                return R_UP(stride);
            }
        }
    }
    pthread_mutex_unlock(&che_mutex);
#endif
    return R_UP(PAGE_SIZE);
}

bytes_t TilerMem_GetStride(SSPtr ssptr)
{
    IN;
    switch(tiler_get_fmt(ssptr))
    {
    case TILFMT_8BIT:  return R_UP(TILER_STRIDE_8BIT);
    case TILFMT_16BIT: return R_UP(TILER_STRIDE_16BIT);
    case TILFMT_32BIT: return R_UP(TILER_STRIDE_32BIT);
    case TILFMT_PAGE:  return R_UP(PAGE_SIZE);
    default:           return R_UP(0);
    }
}

SSPtr TilerMem_VirtToPhys(void *ptr)
{
#ifndef STUB_TILER
    SSPtr ssptr = 0;
    if(!NOT_I(inc_ref(),==,0))
    {
        ssptr = ioctl(td, TILIOC_GSSP, (unsigned long) ptr);
        A_I(dec_ref(),==,0);
    }
    return (SSPtr)R_P(ssptr);
#else
    return (SSPtr)ptr;
#endif
}

/**
 * Internal Unit Test.  Tests the static methods of this
 * library.  Assumes an unitialized state as well.
 *
 * @author a0194118 (9/4/2009)
 *
 * @return 0 for success, non-0 error value for failure.
 */
int __test__MemMgr()
{
    int ret = 0;

    /* state check */
    ret |= NOT_I(TILER_PAGE_WIDTH * TILER_PAGE_HEIGHT,==,PAGE_SIZE);
    ret |= NOT_I(refCnt,==,0);
    ret |= NOT_I(inc_ref(),==,0);
    ret |= NOT_I(refCnt,==,1);
    ret |= NOT_I(dec_ref(),==,0);
    ret |= NOT_I(refCnt,==,0);

    /* enumeration check */
    ret |= NOT_I(PIXEL_FMT_8BIT,==,TILFMT_8BIT);
    ret |= NOT_I(PIXEL_FMT_16BIT,==,TILFMT_16BIT);
    ret |= NOT_I(PIXEL_FMT_32BIT,==,TILFMT_32BIT);
    ret |= NOT_I(PIXEL_FMT_PAGE,==,TILFMT_PAGE);
    ret |= NOT_I(sizeof(MemAllocBlock),==,sizeof(struct tiler_block_info));

    /* void * arithmetic */
    void *a = (void *)1000, *b = a + 2000, *c = (void *)3000;
    ret |= NOT_P(b,==,c);

    /* def_stride */
    ret |= NOT_I(def_stride(0),==,0);
    ret |= NOT_I(def_stride(1),==,PAGE_SIZE);
    ret |= NOT_I(def_stride(PAGE_SIZE),==,PAGE_SIZE);
    ret |= NOT_I(def_stride(PAGE_SIZE + 1),==,2 * PAGE_SIZE);

    /* def_bpp */
    ret |= NOT_I(def_bpp(PIXEL_FMT_32BIT),==,4);
    ret |= NOT_I(def_bpp(PIXEL_FMT_16BIT),==,2);
    ret |= NOT_I(def_bpp(PIXEL_FMT_8BIT),==,1);

    /* def_size */
    tiler_block_info blk = {0};
    blk.fmt = TILFMT_8BIT;
    blk.dim.area.width = PAGE_SIZE * 8 / 10;
    blk.dim.area.height = 10;
    ret |= NOT_I(def_size(&blk),==,10 * PAGE_SIZE);

    blk.fmt = TILFMT_16BIT;
    blk.dim.area.width = PAGE_SIZE * 7 / 10;
    ret |= NOT_I(def_size(&blk),==,20 * PAGE_SIZE);
    blk.dim.area.width = PAGE_SIZE * 4 / 10;
    ret |= NOT_I(def_size(&blk),==,10 * PAGE_SIZE);

    blk.fmt = TILFMT_32BIT;
    ret |= NOT_I(def_size(&blk),==,20 * PAGE_SIZE);
    blk.dim.area.width = PAGE_SIZE * 6 / 10;
    ret |= NOT_I(def_size(&blk),==,30 * PAGE_SIZE);

    return ret;
}

