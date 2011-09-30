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
 *  @file   osal.h
 *
 *  @desc   Defines the interfaces for initializing and finalizing OSAL.
 *  ============================================================================
 */


#if !defined (OSAL_H)
#define OSAL_H


/*  ----------------------------------- IPC headers */
#include <ipc.h>
#include <_ipc.h>

/*  ----------------------------------- OSAL Headers                */
#include <isr.h>
#include <mem_os.h>
#include <mem.h>
#include <dpc.h>
#include <sync.h>


#if defined (__cplusplus)
extern "C" {
#endif


/** ============================================================================
 *  @func   OSAL_init
 *
 *  @desc   Initializes the OS Adaptation layer.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory error.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    OSAL_exit
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
OSAL_init (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function OSAL_Initialize has been replaced
 *              with OSAL_init.
 *  ============================================================================
 */
#define OSAL_Initialize OSAL_init


/** ============================================================================
 *  @func   OSAL_exit
 *
 *  @desc   Releases OS adaptation layer resources indicating that they would
 *          no longer be used.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory error.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  Subcomponent must be initialized.
 *
 *  @leave  None
 *
 *  @see    OSAL_init
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
OSAL_exit (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function OSAL_Finalize has been replaced
 *              with OSAL_exit.
 *  ============================================================================
 */
#define OSAL_Finalize OSAL_exit


#if defined (__cplusplus)
}
#endif


#endif /* !defined (OSAL_H) */
