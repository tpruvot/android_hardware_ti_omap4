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
 *  @file   sync.h
 *
 *  @desc   Defines the interfaces and data structures for the sub-component
 *          SYNC.
 *  ============================================================================
 */


#if !defined (SYNC_H)
#define SYNC_H


/*  ----------------------------------- IPC headers */
#include <ipc.h>
#include <_ipc.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  SYNC_WAITFOREVER
 *
 *  @desc   Argument used to wait forever in function SYNC_WaitOnEvent
 *          function.
 *  ============================================================================
 */
#define SYNC_WAITFOREVER           WAIT_FOREVER

/** ============================================================================
 *  @const  SYNC_NOWAIT
 *
 *  @desc   Argument used for no waiting option in function SYNC_WaitOnEvent
 *          function.
 *  ============================================================================
 */
#define SYNC_NOWAIT                WAIT_NONE


/** ============================================================================
 *  @Macro  SYNC_SpinLockStart
 *
 *  @desc   Begin protection of code through spin lock with all ISRs disabled.
 *          Calling this API protects critical regions of code from preemption
 *          by tasks, DPCs and all interrupts.
 *          This API can be called from DPC context.
 *
 *  @arg    spinLockHandle
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
#define SYNC_SpinLockStart() SYNC_SpinLockStartEx (NULL)


/** ============================================================================
 *  @Macro  SYNC_SpinLockEnd
 *
 *  @desc   End protection of code through spin lock with all ISRs disabled.
 *          This API can be called from DPC context.
 *
 *  @arg    irqFlags
 *              ISR flags value returned from SYNC_SpinLockStart ().
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
#define SYNC_SpinLockEnd(irqFlags) SYNC_SpinLockEndEx (NULL, irqFlags)


/** ============================================================================
 *  @name   SyncAttrs
 *
 *  @desc   This object contains various attributes of SYNC object.
 *
 *  @field  flag
 *              This flag is used by the various SYNC functions and its usage is
 *              dependent on the function using it.
 *
 *  @see    None
 *  ============================================================================
 */
typedef struct SyncAttrs_tag {
    Uint32   flag ;
} SyncAttrs ;


/*  ============================================================================
 *  @name   SyncEvObject
 *
 *  @desc   Forward declaration. See corresponding C file for actual definition.
 *  ============================================================================
 */
typedef struct SyncEvObject_tag SyncEvObject ;

/*  ============================================================================
 *  @name   SyncCsObject
 *
 *  @desc   Forward declaration. See correponding C file for actual definition.
 *  ============================================================================
 */
typedef struct SyncCsObject_tag SyncCsObject ;

/*  ============================================================================
 *  @name   SyncSemObject
 *
 *  @desc   Forward declaration. See correponding C file for actual definition.
 *  ============================================================================
 */
typedef struct SyncSemObject_tag SyncSemObject ;


/** ============================================================================
 *  @name   SyncSemType
 *
 *  @desc   This enumeration defines the possible types of semaphores that
 *          can be created.
 *
 *  @field  SyncSemType_Binary
 *              Indicates that the semaphore is a binary semaphore.
 *  @field  SyncSemType_Counting
 *              Indicates that the semaphore is a counting semaphore.
 *
 *  @see    None
 *  ============================================================================
 */
typedef enum {
    SyncSemType_Binary   = 0,
    SyncSemType_Counting = 1
} SyncSemType ;


/** ============================================================================
 *  @func   SYNC_init
 *
 *  @desc   Initializes the SYNC subcomponent.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_init (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function SYNC_Initialize has been replaced
 *              with SYNC_init.
 *  ============================================================================
 */
#define SYNC_Initialize SYNC_init


/** ============================================================================
 *  @func   SYNC_exit
 *
 *  @desc   This function frees up all resources used by the SYNC subcomponent.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_exit (Void) ;

/** ============================================================================
 *  @deprecated The deprecated function SYNC_Finalize has been replaced
 *              with SYNC_exit.
 *  ============================================================================
 */
#define SYNC_Finalize SYNC_exit


/** ============================================================================
 *  @func   SYNC_OpenEvent
 *
 *  @desc   Creates and initializes an event object for thread
 *          synchronization. The event is initialized to a non-signaled state.
 *
 *  @arg    event
 *              OUT argument to store the newly created event object.
 *  @arg    attr
 *              Reserved for future use.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EMEMORY
 *              Operation failed due to insufficient memory.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  Pointer to event must be valid.
 *          Pointer to attributes must be valid.
 *
 *  @leave  Valid object is returned in case of success.
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_OpenEvent (OUT SyncEvObject ** event, IN SyncAttrs * attr) ;


/** ============================================================================
 *  @func   SYNC_CloseEvent
 *
 *  @desc   Closes the handle corresponding to an event. It also frees the
 *          resources allocated, if any, during call to SYNC_OpenEvent ().
 *
 *  @arg    event
 *              Event to be closed.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *               General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  event must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_CloseEvent (IN SyncEvObject * event) ;


/** ============================================================================
 *  @func   SYNC_ResetEvent
 *
 *  @desc   Resets the synchronization object to non-signaled state.
 *
 *  @arg    event
 *              Event to be reset.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  event must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_ResetEvent (IN SyncEvObject * event) ;


/** ============================================================================
 *  @func   SYNC_SetEvent
 *
 *  @desc   Sets the state of sychronization object to signaled and unblocks all
 *          threads waiting for it.
 *
 *  @arg    event
 *              Event to be set.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  event must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_SetEvent (IN SyncEvObject * event) ;


/** ============================================================================
 *  @func   SYNC_WaitOnEvent
 *
 *  @desc   Waits for an event to be signaled for a specified amount of time.
 *          It is also possible to wait infinitely. This function must 'block'
 *          and not 'spin'.
 *
 *  @arg    event
 *              Event to be waited on.
 *  @arg    timeout
 *              Timeout value.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_ETIMEOUT
 *              Timeout occured while performing operation.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  event must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_WaitOnEvent (IN SyncEvObject * event, IN Uint32 timeout) ;


/** ============================================================================
 *  @func   SYNC_WaitOnMultipleEvents
 *
 *  @desc   Waits on multiple events. Returns when any of the event is set.
 *
 *  @arg    syncEvents
 *              Array of events to be wait on.
 *  @arg    count
 *              Number of events.
 *  @arg    timeout
 *              Timeout for wait.
 *  @arg    index
 *              OUT argument to store the index of event that was set.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_ETIMEOUT
 *              Timeout occured while performing operation.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  syncEvents must be a valid object array.
 *          index must be a valid pointer.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_WaitOnMultipleEvents (IN  SyncEvObject ** syncEvents,
                           IN  Uint32          count,
                           IN  Uint32          timeout,
                           OUT Uint32 *        index) ;


/** ============================================================================
 *  @func   SYNC_CreateCS
 *
 *  @desc   Initializes the Critical section structure.
 *
 *  @arg    cSObj
 *              Structure to be intialized
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EMEMORY
 *              Operation failed due to insufficient memory.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  cSObj must not be NULL.
 *
 *  @leave  In case of success cSObj is valid.
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_CreateCS (OUT SyncCsObject ** cSObj) ;


/** ============================================================================
 *  @func   SYNC_DeleteCS
 *
 *  @desc   Deletes the critical section object.
 *
 *  @arg    cSObj
 *              Critical section to be deleted.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  cSObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_DeleteCS (IN SyncCsObject * cSObj) ;


/** ============================================================================
 *  @func   SYNC_EnterCS
 *
 *  @desc   This function enters the critical section that is passed as an
 *          argument to it. After successful return of this function no other
 *          thread can enter until this thread exits the CS.
 *
 *  @arg    cSObj
 *              Critical section to enter.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  cSObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_EnterCS (IN SyncCsObject * cSObj) ;


/** ============================================================================
 *  @func   SYNC_LeaveCS
 *
 *  @desc   This function makes the critical section available for other threads
 *          to enter.
 *
 *  @arg    cSObj
 *              Critical section to leave.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EMEMORY
 *              Operation failed due to insufficient memory.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  cSObj should be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_LeaveCS (IN SyncCsObject * cSObj) ;


/** ============================================================================
 *  @func   SYNC_CreateSEM
 *
 *  @desc   Creates the semaphore object.
 *
 *  @arg    semObj
 *              Location to receive the pointer to the created semaphore object.
 *  @arg    attr
 *              Attributes to specify the kind of semaphore required to be
 *              created.
 *                  For binary semaphores flag field in the attr should be set
 *                  to SyncSemType_Binary.
 *
 *                  For counting semaphores flag field in the attr should be set
 *                  to SyncSemType_Counting.
 *
 *  @ret    DSP_SOK
 *              Semaphore object successfully created.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EINVALIDARG
 *              Invalid arguments passed.
 *          DSP_EMEMORY
 *              Operation failed due to insufficient memory.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  semObj must not be NULL.
 *          attr must not be NULL.
 *
 *  @leave  In case of success semObj is valid.
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_CreateSEM (OUT SyncSemObject ** semObj, IN SyncAttrs * attr) ;


/** ============================================================================
 *  @func   SYNC_DeleteSEM
 *
 *  @desc   Deletes the semaphore object.
 *
 *  @arg    semObj
 *              Pointer to semaphore object to be deleted.
 *
 *  @ret    DSP_SOK
 *              Semaphore object successfully deleted.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  semObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_DeleteSEM (IN SyncSemObject * semObj) ;


/** ============================================================================
 *  @func   SYNC_WaitSEM
 *
 *  @desc   This function waits on the semaphore.
 *
 *  @arg    semObj
 *              Pointer to semaphore object on which function will wait.
 *  @arg    timeout
 *              Timeout value.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_ETIMEOUT
 *              Timeout occured while performing operation.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *
 *  @enter  semObj must be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_WaitSEM (IN SyncSemObject * semObj, IN Uint32  timeout) ;


/** ============================================================================
 *  @func   SYNC_SignalSEM
 *
 *  @desc   This function signals the semaphore and makes it available for other
 *          threads.
 *
 *  @arg    semObj
 *              Pointer to semaphore object to be signalled.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          SYNC_E_FAIL
 *              General error from GPP-OS.
 *          DSP_EPOINTER
 *              Invalid pointer passed.
 *          DSP_EMEMORY
 *              Operation failed due to memory error.
 *
 *  @enter  semObj should be a valid object.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
DSP_STATUS
SYNC_SignalSEM (IN SyncSemObject * semObj) ;


/** ============================================================================
 *  @func   SYNC_ProtectionStart
 *
 *  @desc   Marks the start of protected code execution.
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
SYNC_ProtectionStart (Void) ;


/** ============================================================================
 *  @func   SYNC_ProtectionEnd
 *
 *  @desc   Marks the end of protected code execution.
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
SYNC_ProtectionEnd (Void) ;


/** ============================================================================
 *  @func   SYNC_SpinLockCreate
 *
 *  @desc   Creates the Spinlock Object.
 *
 *  @arg    None
 *
 *  @ret    Handle to the spin lock object, otherwise NULL.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
Pvoid
SYNC_SpinLockCreate (Void) ;


/** ============================================================================
 *  @func   SYNC_SpinLockEnd
 *
 *  @desc   Deletes the Spinlock Object.
 *
 *  @arg    spinLockhandle
 *              handle to spinlock object to be freed.
 *
 *  @ret    None
 *
 *  @enter  spinLockhandle must be valid.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
Void
SYNC_SpinLockDelete (IN Pvoid spinLockhandle) ;


/** ============================================================================
 *  @func   SYNC_SpinLockStartEx
 *
 *  @desc   Begin protection of code through spin lock with all ISRs disabled.
 *          Calling this API protects critical regions of code from preemption
 *          by tasks, DPCs and all interrupts.
 *          This API can be called from DPC context.
 *
 *  @arg    spinLockHandle
 *              Handle to the spinlock object to be used.
 *              Used if valid, otherwise global lock object will be used.
 *
 *  @ret    Current irq Flags.
 *
 *  @enter  spinLockHandle must be valid if global spinlock is not to be used.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
Uint32
SYNC_SpinLockStartEx (IN OPT Pvoid spinLockHandle) ;


/** ============================================================================
 *  @func   SYNC_SpinLockEndEx
 *
 *  @desc   End protection of code through spin lock with all ISRs disabled.
 *          This API can be called from DPC context.
 *
 *  @arg    spinLockHandle
 *              Handle to the spinlock object to be used. Used only if valid.
 *          irqFlags
 *              ISR flags value returned from SYNC_SpinLockStartEx ().
 *
 *  @ret    None
 *
 *  @enter  spinLockHandle must be valid if global spinlock is not to be used.
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
EXPORT_API
Void
SYNC_SpinLockEndEx (IN OPT Pvoid spinLockHandle, IN OPT Uint32 irqFlags) ;


/** ============================================================================
 *  @func   SYNC_UDelay
 *
 *  @desc   Pause for number microseconds.
 *
 *  @arg    numUSec
 *              number of usec to delay.
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
SYNC_UDelay (IN Uint32 numUSec) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !define (SYNC_H) */
