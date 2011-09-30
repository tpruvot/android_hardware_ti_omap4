/*
 *  memmgr.h
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

#ifndef _MEMMGR_H_
#define _MEMMGR_H_

/* retrieve type definitions */
#include "mem_types.h"

/**
 * Memory Allocator is responsible for:
 * <ol>
 * <li>Allocate 1D and 2D blocks and pack them into a buffer.
 * <li>Free such allocated blocks
 * <li>Map 1D buffers into tiler space
 * <li>Unmap 1D buffers from tiler space
 * <li>Verify if an address lies in 1D or 2D space, whether it
 * is mapped (to tiler space)
 * <li>Mapping Ducati memory blocks to host processor and vice
 * versa.
 * </ol>
 *
 * The allocator distinguishes between:
 * <ul>
 * <li>1D and 2D blocks
 * <li>2D blocks allocated by MemAlloc are non-cacheable.  All
 * other blocks are cacheable (e.g. 1D blocks).  Preallocated
 * may or may not be cacheable, depending on how they've been
 * allocated, but are assumed to be cacheable.
 * <li>buffers (an ordered collection of one or more blocks
 * mapped consecutively)
 * </ul>
 *
 * The allocator tracks each buffer based on (addr, size).
 *
 * Also, via the tiler manager, it tracks each block.  The tiler
 * manager itself also tracks each buffer.
 *
 */

/**
 * Memory Allocator block specification
 *
 * Size of a 2D buffer is calculated as height * stride.  stride
 * is the smallest multiple of page size that is at least
 * the width, and is set by MemMgr_Alloc.
 *
 * Size of a 1D buffer is calculated as length.  stride is not
 * set by MemMgr_Alloc, but it can be set by the user.  length
 * must be a multiple of stride unless stride is 0.
 *
 * @author a0194118 (9/1/2009)
 */
struct MemAllocBlock {
    pixel_fmt_t pixelFormat; /* pixel format */
    union {
        struct {
            pixels_t width;  /* width of 2D buffer */
            pixels_t height; /* height of 2D buffer */
        } area;
        bytes_t  len;        /* length of 1D buffer.  Must be multiple of
                                stride if stride is not 0. */
    } dim;
    uint32_t stride;    /* must be multiple of page size.  Can be 0 only
                           if pixelFormat is PIXEL_FMT_PAGE. */
    void    *ptr;       /* pointer to beginning of buffer */
    uint32_t id;        /* buffer ID - received at allocation */
    uint32_t key;       /* buffer key - given at allocation */
    uint32_t group_id;  /* group ID */
    /* alignment requirements for ssptr: ssptr & (align - 1) == offs */
    uint32_t align;     /* block alignment */
    uint32_t offs;      /* block offset */
    uint32_t reserved;  /* system space address (used internally) */
};

typedef struct MemAllocBlock MemAllocBlock;

/**
 * Returns the page size.  This is required for allocating 1D
 * blocks that stack under any other blocks.
 *
 * @author a0194118 (9/3/2009)
 *
 * @return Page size.
 */
bytes_t MemMgr_PageSize();

/**
 * Allocates a buffer as a list of blocks (1D or 2D), and maps
 * them so that they are packaged consecutively. Returns the
 * pointer to the first block, or NULL on failure.
 * <p>
 * The size of each block other than the last must be a multiple
 * of the page size.  This ensures that the blocks stack
 * correctly.  Set stride to 0 to avoid stride/length alignment
 * constraints.  Stride of 2D blocks will be updated by this
 * method.
 * <p>
 * 2D blocks will be non-cacheable, while 1D blocks will be
 * cacheable.
 * <p>
 * On success, the buffer is registered with the memory
 * allocator.
 * <p>
 * As a side effect, if the operation was successful, the ssptr
 * fields of the block specification will be filled with the
 * system-space addresses, while the ptr fields will be set to
 * the individual blocks.  The stride information is set for 2D
 * blocks.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param blocks     Block specification information.  This
 *                   should be an array of at least num_blocks
 *                   elements.
 * @param num_blocks Number of blocks to be included in the
 *                   allocated memory segment
 *
 * @return Pointer to the buffer, which is also the pointer to
 *         the first allocated block. NULL if allocation failed.
 */
void *MemMgr_Alloc(MemAllocBlock blocks[], int num_blocks);

/**
 * Frees a buffer allocated by MemMgr_Alloc(). It fails for
 * any buffer not allocated by MemMgr_Alloc() or one that has
 * been already freed.
 * <p>
 * It also unregisters the buffer with the memory allocator.
 * <p>
 * This function unmaps the processor's virtual address to the
 * tiler address for all blocks allocated, unregisters the
 * buffer, and frees all of its tiler blocks.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param bufPtr     Pointer to the buffer allocated (returned)
 *                   by MemMgr_Alloc()
 *
 * @return 0 on success.  Non-0 error value on failure.
 */
int MemMgr_Free(void *bufPtr);

/**
 * This function maps the user provided data buffer to the tiler
 * space as blocks, and maps that area into the process space
 * consecutively. You can map a data buffer multiple times,
 * resulting in multiple mapping for the same buffer. However,
 * you cannot map a buffer that is already mapped to tiler, e.g.
 * a buffer pointer returned by this method.
 *
 * In phase 1 and 2, the supported configurations are:
 * <ol>
 * <li> Mapping exactly one 1D block to tiler space (e.g.
 * MapIn1DMode).
 * </ol>
 *
 * @author a0194118 (9/3/2009)
 *
 * @param blocks     Block specification information.  This
 *                   should be an array of at least num_blocks
 *                   elements.  The ptr fields must contain the
 *                   user allocated buffers for the block.
 *                   These will be updated with the mapped
 *                   addresses of these blocks on success.
 *
 *                   Each block must be page aligned.  Length of
 *                   each block also must be page aligned.
 *
 * @param num_blocks Number of blocks to be included in the
 *                   mapped memory segment
 *
 * @return Pointer to the buffer, which is also the pointer to
 *         the first mapped block. NULL if allocation failed.
 */
void *MemMgr_Map(MemAllocBlock blocks[], int num_blocks);

/**
 * This function unmaps the user provided data buffer from tiler
 * space that was mapped to the tiler space in paged mode using
 * MemMgr_Map().  It also unmaps the buffer itself from the
 * process space.  Trying to unmap a previously unmapped buffer
 * will fail.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param bufPtr    Pointer to the buffer as returned by a
 *                  previous call to MemMgr_MapIn1DMode()
 *
 * @return 0 on success.  Non-0 error value on failure.
 */
int MemMgr_UnMap(void *bufPtr);

/**
 * Checks if a given virtual address is mapped by tiler manager
 * to tiler space.
 * <p>
 * This function is equivalent to MemMgr_Is1DBuffer(ptr) ||
 * MemMgr_Is2DBuffer(ptr).  It retrieves the system space
 * address that the virtual address maps to. If this system
 * space address lies within the tiler area, the function
 * returns TRUE.
 *
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr   Pointer to a virtual address
 *
 * @return TRUE (non-0) if the virtual address is within a
 *         buffer that was mapped into tiler space, e.g. by
 *         calling MemMgr_MapIn1DMode() or MemMgr_MapIn2DMode()
 */
bool MemMgr_IsMapped(void *ptr);

/**
 * Checks if a given virtual address lies in a tiler 1D buffer.
 * <p>
 * This function retrieves the system space address that the
 * virtual address maps to.  If this system space address is
 * within the 1D tiler area, it is considered lying within a 1D
 * buffer.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr   Pointer to a virtual address
 *
 * @return TRUE (non-0) if the virtual address is within a
 *         mapped 1D tiler buffer.  FALSE (0) if the virtual
 *         address is not mapped, invalid, or is mapped to an
 *         area other than a 1D tiler buffer.  In phase 1,
 *         however, it is TRUE it the virtual address is mapped
 *         to the page-mode area of the tiler space.
 */
bool MemMgr_Is1DBlock(void *ptr);

/**
 * Checks if a given virtual address lies in a 2D buffer.
 * <p>
 * This function retrieves the system space address that the
 * virtual address maps to.  If this system space address is
 * within the 2D tiler area, it is considered lying within a 2D
 * buffer.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr   Pointer to a virtual address
 *
 * @return TRUE (non-0) if the virtual address is within a
 *         mapped 2D buffer.  FALSE (0) if the virtual address
 *         is not mapped, invalid, or is mapped to an area other
 *         than a 2D buffer.  In phase 1, however, it is TRUE it
 *         the virtual address is mapped to any area of the
 *         tiler space other than page mode.
 */
bool MemMgr_Is2DBlock(void *ptr);

/**
 * Returns the stride corresponding to a virtual address.  For
 * 1D and 2D buffers it returns the stride supplied
 * with/acquired during the allocation/mapping.  For non-tiler
 * buffers it returns the page size.
 * <p>
 * NOTE: on Ducati phase 1, stride should return 16K for 8-bit
 * 2D buffers, 32K for 16-bit and 32-bit 2D buffers, the stride
 * used for alloc/map for 1D buffers, and the page size for
 * non-tiler buffers.
 *
 * For unmapped addresses it returns 0.  However, this cannot be
 * used to determine if an address is unmapped as 1D buffers
 * could also have 0 stride (e.g. compressed buffers).
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr    pointer to a virtual address
 *
 * @return The virtual stride of the block that contains the
 *         address.
 */
bytes_t MemMgr_GetStride(void *ptr);

#endif
