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
 *  @file   hal_intgen.h
 *
 *  @desc   Hardware Abstraction Layer for OMAP.
 *          Declares necessary functions for Interrupt Handling.
 *  ============================================================================
 */


#if !defined (HAL_INTERRUPT_H)
#define HAL_INTERRUPT_H


/*  ----------------------------------- DSP/BIOS Link               */
#include <ipc.h>


#if defined (__cplusplus)
extern "C" {
#endif


/** ============================================================================
 *  @func   HAL_InterruptEnable
 *
 *  @desc   Enable the mailbox interrupt
 *
 *  @arg    intId
 *              ID of the interrupt to be enabled
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
Void
HAL_InterruptEnable (IN Uint32 intId) ;


/** ============================================================================
 *  @func   HAL_InterruptDisable
 *
 *  @desc   Disable the mailbox interrupt
 *
 *  @arg    intId
 *              ID of the interrupt to be disabled
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
Void
HAL_InterruptDisable (IN Uint32 intId) ;


/** ============================================================================
 *  @func   HAL_WaitClearInterrupt
 *
 *  @desc   Wait for interrupt to be cleared.
 *
 *  @arg    intId
 *              ID of the interrupt to wait to be cleared.
 *
 *  @ret    DSP_SOK
 *              On success
 *          DSP_ETIMEOUT
 *              Timeout while interrupting the DSP
 *          DSP_EFAIL
 *              General failure
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
HAL_WaitClearInterrupt (IN Uint32 intId) ;


/** ============================================================================
 *  @func   HAL_InterruptDsp
 *
 *  @desc   Sends a specified interrupt to the DSP.
 *
 *  @arg    intId
 *              ID of the interrupt to be sent.
 *  @arg    value
 *              Value to be sent with the interrupt.
 *
 *  @ret    DSP_SOK
 *              On success
 *          DSP_ETIMEOUT
 *              Timeout while interrupting the DSP
 *          DSP_EFAIL
 *              General failure
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
HAL_InterruptDsp (IN Uint32 intId, Uint32 value) ;


/** ============================================================================
 *  @func   HAL_ClearDspInterrupt
 *
 *  @desc   Clears the specified DSP interrupt.
 *
 *  @arg    intId
 *              ID of the interrupt to be cleared.
 *
 *  @ret    value
 *              Value returned with the interrupt.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
Uint32
HAL_ClearDspInterrupt (IN Uint32 intId) ;


#if defined (__cplusplus)
}
#endif


#endif  /* !defined (HAL_INTERRUPT_H) */
