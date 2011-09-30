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
 *  @file   hal_cache.h
 *
 *
 *  @desc   This file declares cache functions of HAL (Hardware Abstraction
 *          Layer)
 *  ============================================================================
 */


#if !defined (HAL_CACHE_H)
#define HAL_CACHE_H


/*  ----------------------------------- CGTOOLS headers             */
#include <stddef.h>

/*  ----------------------------------- DSP/BIOS LINK headers       */
//#include <_hal_cache.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @func   HAL_cacheInv
 *
 *  @desc   Invalidate cache contents for specified memory region.
 *
 *  @arg    addr
 *              Address of memory region for which cache is to be invalidated.
 *  @arg    size
 *              Size of memory region for which cache is to be invalidated.
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
inline Void HAL_cacheInv (Ptr addr, size_t size)
{
    //HAL_CACHE_INV (addr, size) ;
}


/** ============================================================================
 *  @func   HAL_cacheWb
 *
 *  @desc   Write-back cache contents for specified memory region.
 *
 *  @arg    addr
 *              Address of memory region for which cache is to be invalidated.
 *  @arg    size
 *              Size of memory region for which cache is to be invalidated.
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
inline Void HAL_cacheWb (Ptr addr, size_t size)
{
    ;
}


/** ============================================================================
 *  @func   HAL_cacheWbInv
 *
 *  @desc   Write-back and invalidate cache contents for specified memory region
 *
 *  @arg    addr
 *              Address of memory region for which cache is to be invalidated.
 *  @arg    size
 *              Size of memory region for which cache is to be invalidated.
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
inline Void HAL_cacheWbInv (Ptr addr, size_t size)
{
    //HAL_CACHE_WBINV (addr, size) ;
}


/** ============================================================================
 *  @func   HAL_cacheWbAll
 *
 *  @desc   Write-back complete cache contents.
 *
 *  @arg    None
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
inline Void HAL_cacheWbAll ()
{
    //HAL_CACHE_WBALL ;
}


/** ============================================================================
 *  @func   HAL_cacheWbInvAll
 *
 *  @desc   Write-back and invaliate complete cache contents.
 *
 *  @arg    None
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
inline Void HAL_cacheWbInvAll ()
{
    //HAL_CACHE_WBINVALL ;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (HAL_CACHE_H) */
