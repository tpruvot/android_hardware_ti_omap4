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
 *  @file   shmdefs.h
 *
 *  @desc   Shared definitions for DSP/BIOS
 *
 *  ============================================================================
 */


#if !defined (SHMDEFS_H)
#define SHMDEFS_H

/*  ----------------------------------- IPC */
#include <platform.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  IPC_BUF_ALIGN
 *
 *  @desc   Value of Align parameter to alloc/create calls.
 *  ============================================================================
 */
#define IPC_BUF_ALIGN     128

/** ============================================================================
 *  @const  DSP_MAUSIZE
 *
 *  @desc   Size of the DSP MAU (in bytes).
 *  ============================================================================
 */
#define DSP_MAUSIZE           1

/** ============================================================================
 *  @const  CACHE_L2_LINESIZE
 *
 *  @desc   Line size of DSP L2 cache (in bytes).
 *  ============================================================================
 */
#define CACHE_L2_LINESIZE     128

/** ============================================================================
 *  @const  ADD_PADDING
 *
 *  @desc   Macro to add padding to a structure.
 *  ============================================================================
 */
#define ADD_PADDING(padVar, count)  Uint16 padVar [count] ;

/*  ============================================================================
 *  @const  IPC_ALIGN
 *
 *  @desc   Macro to align a number to a specified value.
 *          x: The number to be aligned
 *          y: The value that the number should be aligned to.
 *  ============================================================================
 */
#define IPC_ALIGN(x, y) (Uint32)((Uint32)((x + y - 1) / y) * y)

/** ============================================================================
 *  @const  IPC_16BIT_PADDING
 *
 *  @desc   Padding required for alignment of a 16-bit value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define IPC_16BIT_PADDING  ((CACHE_L2_LINESIZE - sizeof (Uint16)) / 2)

/** ============================================================================
 *  @const  IPC_32BIT_PADDING
 *
 *  @desc   Padding required for alignment of a 32-bit value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define IPC_32BIT_PADDING  ((CACHE_L2_LINESIZE - sizeof (Uint32)) / 2)

/** ============================================================================
 *  @const  IPC_64BIT_PADDING
 *
 *  @desc   Padding required for alignment of a 64-bit value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define IPC_64BIT_PADDING  ((CACHE_L2_LINESIZE - (sizeof (Uint32) * 2)) / 2)

/** ============================================================================
 *  @const  IPC_BOOL_PADDING
 *
 *  @desc   Padding required for alignment of a Boolean value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define IPC_BOOL_PADDING  ((CACHE_L2_LINESIZE - sizeof (Bool)) / 2)

/** ============================================================================
 *  @const  IPC_PTR_PADDING
 *
 *  @desc   Padding required for alignment of a pointer value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define IPC_PTR_PADDING  ((CACHE_L2_LINESIZE - sizeof (Void *)) / 2)

/** ============================================================================
 *  @const  NOTIFYSHMDRV_EVENT_ENTRY_PADDING
 *
 *  @desc   Padding length for NotifyShmDrv event entry.
 *  ============================================================================
 */
#define NOTIFYSHMDRV_EVENT_ENTRY_PADDING (  (CACHE_L2_LINESIZE     \
                                          - ((sizeof (Uint32)) * 3)) / 2)

/** ============================================================================
 *  @const  NOTIFYSHMDRV_CTRL_PADDING
 *
 *  @desc   Padding length for NotifyShmDrv control structure.
 *  ============================================================================
 */
#define NOTIFYSHMDRV_CTRL_PADDING (  (CACHE_L2_LINESIZE            \
                                    - (   (sizeof (Void *) * 2)    \
                                       +  (sizeof (Uint32) * 2))) / 2)


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (SHMDEFS_H) */
