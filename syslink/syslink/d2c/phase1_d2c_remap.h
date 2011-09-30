/*
 *  phase1_d2c_remap.h
 *
 *  Ducati to Chiron Tiler block remap Interface for TI OMAP processors.
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

#ifndef _PHASE1_D2C_REMAP_H_
#define _PHASE1_D2C_REMAP_H_

#define REMAP_ERR_NONE    0
#define REMAP_ERR_GENERIC 1

/**
 * During Phase 1, tiler info is needed to remap tiler blocks
 * from Ducati to Chiron.  However, this is no longer needed in
 * phase 2, as the virtual stride will be the same on Ducati and
 * Chiron.  This API is provided here temporarily for phase 1.
 *
 * This method remaps a list of tiler blocks (specified by the
 * Ducati virtual address and the block size) in a consecutive
 * fashion into one virtual buffer on Chiron.
 *
 * :NOTE: this function does not work correctly for 2D buffers
 * of the following geometries:
 *
 *  16-bit, width > 73.148 * height
 *  32-bit, width > 36.574 * height
 *
 * This is due to limitations of the API definition, as tiler
 * driver does not keep track of exact widths and heights of its
 * allocated blocks, and the remapping algorithm is trying to
 * determine the page-width of each block.  For buffers
 * mentioned above there are multiple solutions, and the
 *
 * This limitation will not apply in phase 2.
 *
 * @author a0194118 (9/9/2009)
 *
 * @param num_blocks   Number of blocks to remap
 * @param dsptrs       Array of Ducati addresses (one for each
 *                     block).  Each address must point to the
 *                     beginning of a tiler allocated 1D or 2D
 *                     block.
 * @param lengths      Array of block lenghts (one for each
 *                     block).  This is the desired length of
 *                     the blocks on the Chiron, and must be
 *                     multiples of the page size. These are
 *                     also used to infer the height of 2D
 *                     buffers, and the length of 1D buffers, as
 *                     the tiler driver does not keep track of
 *                     exact block sizes, only allocated block
 *                     sizes.
 *
 * @return Pointer to the remapped combined buffer.  NULL on
 *         failure.
 */
void *tiler_assisted_phase1_D2CReMap(int num_blocks, DSPtr dsptrs[],
                                     bytes_t lengths[]);

/**
 * During Phase 1, tiler info is needed to remap tiler blocks
 * from Ducati to Chiron.  However, this is no longer needed in
 * phase 2, as the virtual stride will be the same on Ducati and
 * Chiron.  This API is provided here temporarily for phase 1.
 *
 * This method unmaps a previously remapped buffer from the
 * Chiron side.
 *
 * @author a0194118 (9/9/2009)
 *
 * @param bufPtr   Pointer to the buffer as returned by a
 *                 tiler_assisted_phase1_D2CReMap call.
 *
 * @return 0 on success, non-0 error value on failure.
 */
int tiler_assisted_phase1_DeMap(void *bufPtr);

#endif
