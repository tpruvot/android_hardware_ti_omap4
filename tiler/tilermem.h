/*
 *  tilermem.h
 *
 *  Tiler Memory Interface functions for TI OMAP processors.
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

#ifndef _TILERMEM_H_
#define _TILERMEM_H_

/* retrieve type definitions */
#include "mem_types.h"

/**
 * Tiler Memory Allocator is responsible for:
 * <ol>
 * <li>Getting the stride information for containers(???) or
 * buffers
 * <li>Converting virtual addresses to physical addresses.
 * </ol>
 */

/**
 * Returns the tiler stride corresponding to the system space
 * address.  For 2D buffers it returns the container stride. For
 * 1D buffers it returns the page size.  For non-tiler buffers
 * it returns 0.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr    pointer to a virtual address
 *
 * @return The stride of the block that contains the address.
 */
bytes_t TilerMem_GetStride(SSPtr ssptr);

/**
 * Retrieves the physical system-space address that corresponds
 * to the virtual address.
 *
 * @author a0194118 (9/1/2009)
 *
 * @param ptr    pointer to a virtual address
 *
 * @return The physical system-space address that the virtual
 *         address refers to.  If the virtual address is invalid
 *         or unmapped, it returns 0.
 */
SSPtr TilerMem_VirtToPhys(void *ptr);

#endif
