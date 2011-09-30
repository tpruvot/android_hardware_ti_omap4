/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2008-2010, Texas Instruments Incorporated
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
/** ============================================================================
 *  @file   SysLinkMemUtils.h
 *
 *
 *
 *  ============================================================================
 */


#ifndef SYSLINKMEMUTILS_H_0xf2ba
#define SYSLINKMEMUTILS_H_0xf2ba


/* Standard headers */
#include <Std.h>
#include <ProcMgr.h>


#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining the MPU address to map to Remote Address
 */
typedef struct {
    UInt32 mpuAddr;
    /*!< Host Address to Map*/
    UInt32 size;
    /*!< Size of the Buffer to Map */
} SyslinkMemUtils_MpuAddrToMap;

/* =============================================================================
 *  APIs
 * =============================================================================
 */

Ptr
SysLinkMemUtils_DAtoVA (Ptr da);

Int32
SysLinkMemUtils_alloc (UInt32 dataSize, UInt32 *data);

Int32
SysLinkMemUtils_free (UInt32 dataSize, UInt32 *data);

Int
SysLinkMemUtils_map (SyslinkMemUtils_MpuAddrToMap mpuAddrList[],
                     UInt32                       numOfBuffers,
                     UInt32 *                     mappedAddr,
                     ProcMgr_MapType              memType,
                     ProcMgr_ProcId               procId);

Int
SysLinkMemUtils_unmap (UInt32 mappedAddr, ProcMgr_ProcId procId);

Int
SysLinkMemUtils_virtToPhysPages (UInt32 remoteAddr,
                                 UInt32 numOfPages,
                                 UInt32 physEntries[],
                                 ProcMgr_ProcId procId);

Int
SysLinkMemUtils_virtToPhys (UInt32          remoteAddr,
                            UInt32 *        physAddr,
                            ProcMgr_ProcId  procId);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* SYSLINKMEMUTILS_H_0xf2ba */
