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
 *  @file   print.h
 *
 *  @desc   Interface declaration of OS printf abstraction.
 *
 *  ============================================================================
 */


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/*  ----------------------------------- IPC headers */
#include <ipc.h>
#include <_ipc.h>


/** ============================================================================
 *  @func   PRINT_init
 *
 *  @desc   Initializes the PRINT sub-component.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
PRINT_init (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function PRINT_Initialize has been replaced
 *              with PRINT_init.
 *  ============================================================================
 */
#define PRINT_Initialize PRINT_init


/** ============================================================================
 *  @func   PRINT_exit
 *
 *  @desc   Releases resources used by this sub-component.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
PRINT_exit (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function PRINT_Finalize has been replaced
 *              with PRINT_exit.
 *  ============================================================================
 */
#define PRINT_Finalize PRINT_exit


/** ============================================================================
 *  @func   PRINT_Printf
 *
 *  @desc   Provides standard printf functionality abstraction.
 *
 *  @arg    format
 *              Format string.
 *  @arg    ...
 *              Variable argument list.
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
#if defined (TRACE_KERNEL)

Void
PRINT_Printf (Pstr format, ...) ;
#endif

#if defined (TRACE_USER)
/*  ----------------------------------------------------------------------------
 *  Extern declaration for printf to avoid compiler warning.
 *  ----------------------------------------------------------------------------
 */
extern int printf (const char * format, ...) ;

#define PRINT_Printf printf
#endif


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
