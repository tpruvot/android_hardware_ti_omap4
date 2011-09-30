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
 *  @file   Cache.h
 *
 *  @brief      Defines Cache API interface.
 *  ============================================================================
 */


#ifndef CACHE_H
#define CACHE_H


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/*! Lists of bitmask cache types */
enum Cache_Type {
    Cache_Type_L1P = 0x1,         /*! Level 1 Program cache */
    Cache_Type_L1D = 0x2,         /*! Level 1 Data cache */
    Cache_Type_L1  = 0x3,         /*! Level 1 caches */
    Cache_Type_L2P = 0x4,         /*! Level 2 Program cache */
    Cache_Type_L2D = 0x8,         /*! Level 2 Data cache */
    Cache_Type_L2  = 0xC,         /*! Level 2 caches */
    Cache_Type_ALL = 0xffff       /*! All caches */
};

/*
 *  ======== Cache_inv ========
 */
Void Cache_inv(Ptr blockPtr, UInt32 byteCnt, Bits16 type, Bool wait);

/*
 *  ======== Cache_wb ========
 */
Void Cache_wb(Ptr blockPtr, UInt32 byteCnt, Bits16 type, Bool wait);

/*
 *  ======== Cache_wbInv ========
 */
Void Cache_wbInv(Ptr blockPtr, UInt32 byteCnt, Bits16 type, Bool wait);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* CACHE_H */
