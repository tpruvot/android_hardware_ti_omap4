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
 *  @file   isr.h
 *
 *  @desc   Defines the interfaces and data structures for the
 *          sub-component ISR.
 *  ============================================================================
 */


#if !defined (ISR_H)
#define ISR_H


/*  ----------------------------------- IPC headers */
#include <ipc.h>
#include <_ipc.h>

/*  ----------------------------------- Trace & Debug */
#include <_trace.h>


#if defined (__cplusplus)
extern "C" {
#endif


/** ============================================================================
 *  @name   ISR_State
 *
 *  @desc   Enumerates the various states of ISR.
 *
 *  @field  ISR_installed
 *              Indicates that the ISR is installed.
 *  @field  ISR_uninstalled
 *              Indicates that the ISR is uninstalled.
 *  @field  ISR_disabled
 *              Indicates that the ISR is disabled.
 *  @field  ISR_enabled
 *              Indicates that the ISR is enabled.
 *  ============================================================================
 */
typedef enum {
    ISR_installed   = 0,
    ISR_uninstalled = 1,
    ISR_disabled    = 2,
    ISR_enabled     = 3
} ISR_State ;


/** ============================================================================
 *  @name   IsrObject
 *
 *  @desc   Forward declaration for IsrObject, actual definition is
 *          OS dependent.
 *  ============================================================================
 */
typedef struct IsrObject_tag IsrObject ;

/** ============================================================================
 *  @name   IsrProc
 *
 *  @desc   Function prototype for an ISR. The user defined function
 *          to be invoked as an ISR should conform to this signature.
 *
 *  @arg    refData
 *             Data to be passed to ISR when invoked.
 *
 *  @ret    None
 *  ============================================================================
 */
typedef Void (*IsrProc) (Pvoid refData) ;


/** ============================================================================
 *  @func   ISR_init
 *
 *  @desc   Initializes and allocates resources used by ISR subcomponent.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory.
 *
 *  @enter  None
 *
 *  @leave  ISR must be initialized.
 *
 *  @see    ISR_install, ISR_exit
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_init (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Initialize has been replaced
 *              with ISR_init.
 *  ============================================================================
 */
#define ISR_Initialize ISR_init


/** ============================================================================
 *  @func   ISR_exit
 *
 *  @desc   Releases resources reserved for ISR subcomponent.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory.
 *
 *  @enter  ISR must be initialized.
 *
 *  @leave  None
 *
 *  @see    ISR_init
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_exit (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Finalize has been replaced
 *              with ISR_exit.
 *  ============================================================================
 */
#define ISR_Finalize ISR_exit


/** ============================================================================
 *  @func   ISR_create
 *
 *  @desc   Creates an ISR object.
 *
 *  @arg    fnISR
 *              User defined interrupt service routine.
 *  @arg    refData
 *              Argument to be passed to ISR when it is invoked.
 *  @arg    intObj
 *              Interrupt information (OS and hardware dependent).
 *  @arg    isrObj
 *              Out argument for IsrObject.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EINVALIDARG
 *              Invalid arguments were passed to function.
 *          DSP_EMEMORY
 *              Out of memory error.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be valid pointer.
 *          intObj must be a valid pointer.
 *          fnISR must be a valid function pointer.
 *
 *  @leave  A valid IsrObject is returned on success.
 *
 *  @see    ISR_delete
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_create (IN  IsrProc           fnISR,
            IN  Pvoid             refData,
            IN  InterruptObject * intObj,
            OUT IsrObject **      isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Create has been replaced
 *              with ISR_create.
 *  ============================================================================
 */
#define ISR_Create ISR_create


/** ============================================================================
 *  @func   ISR_delete
 *
 *  @desc   Deletes the isrObject.
 *
 *  @arg    isrObj
 *              Object to be deleted.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EPOINTER
 *              Invalid isrObj object.
 *          DSP_EMEMORY
 *              Free memory error.
 *          DSP_EACCESSDENIED
 *              isrObj not uninstalled.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    ISR_create
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_delete (IN IsrObject * isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Delete has been replaced
 *              with ISR_delete.
 *  ============================================================================
 */
#define ISR_Delete ISR_delete


/** ============================================================================
 *  @func   ISR_install
 *
 *  @desc   Installs an interrupt service routine defined by isrObj.
 *
 *  @arg    hostConfig
 *              Void pointer containing installation information related
 *              to installation of an ISR.
 *  @arg    isrObj
 *              The isrObj object.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EPOINTER
 *              Invalid isrObj object.
 *          DSP_EACCESSDENIED
 *              An ISR is already installed.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be valid.
 *
 *  @leave  The isr is installed on success.
 *          isrObj contains a valid IsrObject.
 *
 *  @see    ISR_Func, ISR_uninstall
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_install (IN  Void *       hostConfig,
             IN  IsrObject *  isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Install has been replaced
 *              with ISR_install.
 *  ============================================================================
 */
#define ISR_Install ISR_install


/** ============================================================================
 *  @func   ISR_uninstall
 *
 *  @desc   Uninstalls the interrupt service routine defined by isrObj.
 *
 *  @arg    isrObj
 *             ISR object to uninstall.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EPOINTER
 *              Invalid isrObj object.
 *          DSP_EACCESSDENIED
 *              ISR is already uninstalled.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be a valid IsrObject.
 *
 *  @leave  None
 *
 *  @see    ISR_install
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_uninstall (IN IsrObject * isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Uninstall has been replaced
 *              with ISR_uninstall.
 *  ============================================================================
 */
#define ISR_Uninstall ISR_uninstall


/** ============================================================================
 *  @func   ISR_disable
 *
 *  @desc   Disables an ISR associated with interrupt Id of isrObject.
 *
 *  @arg    isrObj
 *              ISR object.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EACCESSDENIED
 *              ISR is not installed.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    ISR_enable, ISR_install
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_disable (IN  IsrObject * isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Disable has been replaced
 *              with ISR_disable.
 *  ============================================================================
 */
#define ISR_Disable ISR_disable


/** ============================================================================
 *  @func   ISR_enable
 *
 *  @desc   Reactivates ISRs based on the specified flags argument. The flags
 *          argument must be obtained with an earlier call to ISR_disable.
 *
 *  @arg    isrObj
 *              ISR object.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EACCESSDENIED
 *              ISR is not installed.
 *          DSP_EFAIL
 *              General error from GPP-OS.
 *
 *  @enter  ISR must be initialized.
 *          isrObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    ISR_disable
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_enable (IN  IsrObject * isrObj) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Enable has been replaced
 *              with ISR_enable.
 *  ============================================================================
 */
#define ISR_Enable ISR_enable


/** ============================================================================
 *  @func   ISR_getState
 *
 *  @desc   Gets the state of an ISR.
 *
 *  @arg    isrObj
 *              The ISR object.
 *  @arg    isrState
 *              Current status of the ISR.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EPOINTER
 *              Invalid isrObj object.
 *          DSP_EINVALIDARG
 *              Invalid isrStatus pointer.
 *
 *  @enter  isrObj must be a valid IsrObject.
 *          isrStatus must be a valid pointer.
 *
 *  @leave  None
 *
 *  @see    ISR_install, ISR_uninstall, ISR_enable, ISR_disable
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
ISR_getState (IN  IsrObject *  isrObj,
              OUT ISR_State *  isrState) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_GetState has been replaced
 *              with ISR_getState.
 *  ============================================================================
 */
#define ISR_GetState ISR_getState


#if defined (DDSP_DEBUG)
/** ============================================================================
 *  @func   ISR_debug
 *
 *  @desc   Prints the current status of ISR objects.
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
EXPORT_API
Void
ISR_debug (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function ISR_Debug has been replaced
 *              with ISR_debug.
 *  ============================================================================
 */
#define ISR_Debug ISR_debug
#endif /* defined (DDSP_DEBUG) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (ISR_H) */
