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
 *  @file   notify_shmDriver.h
 *
 *
 *  @desc   Defines the interface for the Notify driver using Shared Memory and
 *          interrupts to communicate with the remote processor.
 *
 *  ============================================================================
 */


#if !defined (_NOTIFY_SHMDRIVER_H_)
#define _NOTIFY_SHMDRIVER_H_


/*  ----------------------------------- IPC */
#include <ipctypes.h>

/*  ----------------------------------- Notify       */
#include <notifyerr.h>

/*  ----------------------------------- Notify driver */
#include <notify_driverdefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @name   NotifyShmDrv_EventEntry
 *
 *  @desc   Defines the structure of event entry within the event chart.
 *          Each entry contains occured event-specific information.
 *          This is shared between GPP and DSP.
 *
 *  @field  flag
 *              Indicating event is present or not.
 *  @field  payload
 *              Variable containing data associated with each occured event.
 *  @field  reserved
 *              Reserved field to contain additional information about the
 *              event entry.
 *  @field  padding
 *              Padding.
 *  ============================================================================
 */
typedef struct NotifyShmDrv_EventEntry_tag {
    volatile Uint32  flag     ;
    volatile Uint32  payload  ;
    volatile Uint32  reserved ;
    ADD_PADDING(padding, NOTIFYSHMDRV_EVENT_ENTRY_PADDING)
} NotifyShmDrv_EventEntry ;

/** ============================================================================
 *  @name   NotifyShmDrv_EventRegMask
 *
 *  @desc   Defines the mask indicating registered events on the processor.
 *          This is shared between GPP and DSP.
 *
 *  @field  mask
 *              Indicating event is registered.
 *  @field  enableMask
 *              Indicates event is enabled.
 *  @field  padding
 *              Padding.
 *  ============================================================================
 */
typedef struct NotifyShmDrv_EventRegMask_tag {
    volatile Uint32 mask ;
    volatile Uint32 enableMask ;
    ADD_PADDING (padding, IPC_64BIT_PADDING)
} NotifyShmDrv_EventRegMask ;

/** ============================================================================
 *  @name   NotifyShmDrv_EventRegEntry
 *
 *  @desc   Defines the structure of event registration entry within the Event
 *          Registration Chart.
 *          Each entry contains registered event-specific information.
 *
 *  @field  regEventNo
 *              Index into the event chart, indicating the registered event.
 *  @field  reserved
 *              Reserved field to contain additional information about the
 *              registered event.
 *  ============================================================================
 */
typedef struct NotifyShmDrv_EventRegEntry_tag {
    Uint32     regEventNo ;
    Uint32     reserved ;
} NotifyShmDrv_EventRegEntry ;

/** ============================================================================
 *  @name   NotifyShmDrv_ProcCtrl
 *
 *  @desc   Defines the NotifyShmDrv control structure, which contains all
 *          information for one processor.
 *          This structure is shared between the two processors.
 *
 *  @field  otherEventChart
 *              Address of event chart for the other processor.
 *  @field  selfEventChart
 *              Address of event chart for this processor.
 *  @field  recvInitStatus
 *              Indicates whether the driver has been initialized, and is ready
 *              to receive events on this processor. If the driver does not
 *              support events from other processor to this processor, this flag
 *              will always indicate not-initialized status.
 *  @field  sendInitStatus
 *              Indicates whether the driver has been initialized, and is ready
 *              to send events on this processor. If the driver does not
 *              support events from this processor to other processor, this flag
 *              will always indicate not-initialized status.
 *  @field  padding
 *              Padding for alignment.
 *  @field  selfRegMask
 *              Registration mask.
 *  ============================================================================
 */
typedef struct NotifyShmDrv_ProcCtrl_tag {
    NotifyShmDrv_EventEntry *     selfEventChart ;
    NotifyShmDrv_EventEntry *     otherEventChart ;
    Uint32                        recvInitStatus ;
    Uint32                        sendInitStatus ;
    ADD_PADDING(padding, NOTIFYSHMDRV_CTRL_PADDING)

    NotifyShmDrv_EventRegMask     regMask ;
} NotifyShmDrv_ProcCtrl ;

/** ============================================================================
 *  @name   NotifyShmDrv_Ctrl
 *
 *  @desc   Defines the NotifyShmDrv control structure, which contains all
 *          information shared between the two connected processors
 *          This structure is shared between the two processors.
 *
 *  @field  otherProcCtrl
 *              Control structure for other processor
 *  @field  selfProcCtrl
 *              Control structure for self processor
 *  ============================================================================
 */
typedef struct NotifyShmDrv_Ctrl_tag {
    NotifyShmDrv_ProcCtrl otherProcCtrl ;
    NotifyShmDrv_ProcCtrl selfProcCtrl ;
} NotifyShmDrv_Ctrl ;


/** ============================================================================
 *  @name   NotifyShmDrv_driverInit
 *
 *  @desc   Initialization function for the Notify shared memory mailbox driver.
 *
 *  @arg    driverName
 *              Name of the Notify driver to be initialized.
 *  @arg    config
 *              Configuration information for the Notify driver. This contains
 *              generic information as well as information specific to the type
 *              of Notify driver, as defined by the driver provider.
 *  @arg    driverObj
 *              Location to receive the pointer to the Notify-driver specific
 *              object.
 *
 *  @ret    NOTIFY_SOK
 *              The Notify driver has been successfully initialized
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_EPOINTER
 *              Invalid pointer passed
 *          NOTIFY_ECONFIG
 *              Invalid configuration information passed
 *          NOTIFY_EINVALIDARG
 *              Invalid arguments
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  Notify module must have been initialized before this call
 *          driverName must be a valid pointer
 *          config must be valid
 *          driverObj must be a valid pointer.
 *
 *  @leave  On success, the driver must be initialized.
 *
 *  @see    Notify_Interface, Notify_driverInit ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_driverInit (IN  Char8 *          driverName,
                         IN  Notify_Config *  config,
                         OUT Void **          driverObj) ;

/** ============================================================================
 *  @name   NotifyShmDrv_driverExit
 *
 *  @desc   Finalization function for the Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *
 *  @ret    NOTIFY_SOK
 *              The Notify driver has been successfully initialized
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EINVALIDARG
 *              Invalid arguments
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling
 *          this function.
 *          handle must be a valid Notify driver handle
 *
 *  @leave  On success, the driver must be initialized.
 *
 *  @see    Notify_Interface, Notify_driverExit ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_driverExit (IN Notify_DriverHandle handle) ;

/** ============================================================================
 *  @func   NotifyShmDrv_registerEvent
 *
 *  @desc   Register a callback for an event with the Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    procId
 *              ID of the processor from which notifications can be received on
 *              this event.
 *  @arg    eventNo
 *              Event number to be registered.
 *  @arg    fnNotifyCbck
 *              Callback function to be registered for the specified event.
 *  @arg    cbckArg
 *              Optional argument to the callback function to be registered for
 *              the specified event. This argument shall be passed to each
 *              invocation of the callback function.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling
 *          this function.
 *          handle must be a valid Notify driver handle
 *          fnNotifyCbck must be a valid pointer.
 *          The event must be supported by the Notify driver.
 *
 *  @leave  On success, the event must be registered with the Notify driver
 *
 *  @see    Notify_Interface, Notify_registerEvent ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_registerEvent (IN Notify_DriverHandle handle,
                            IN Processor_Id        procId,
                            IN Uint32              eventNo,
                            IN FnNotifyCbck        fnNotifyCbck,
                            IN Void *              cbckArg) ;

/** ============================================================================
 *  @func   NotifyShmDrv_unregisterEvent
 *
 *  @desc   Unregister a callback for an event with the Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    procId
 *              ID of the processor from which notifications can be received on
 *              this event.
 *  @arg    eventNo
 *              Event number to be un-registered.
 *  @arg    fnNotifyCbck
 *              Callback function to be un-registered for the specified event.
 *  @arg    cbckArg
 *              Optional argument to the callback function to be un-registered
 *              for the specified event.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_ENOTFOUND
 *              Specified event registration was not found
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *          fnNotifyCbck must be a valid pointer.
 *          The event must be supported by the Notify driver.
 *          The event must have been registered with the Notify driver earlier.
 *
 *  @leave  On success, the event must be unregistered with the Notify driver.
 *
 *  @see    Notify_Interface, Notify_unregisterEvent ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_unregisterEvent (IN Notify_DriverHandle handle,
                              IN Processor_Id        procId,
                              IN Uint32              eventNo,
                              IN FnNotifyCbck        fnNotifyCbck,
                              IN Void *              cbckArg) ;

/** ============================================================================
 *  @func   NotifyShmDrv_sendEvent
 *
 *  @desc   Send a notification event to the registered users for this
 *          notification on the specified processor.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    procId
 *              ID of the processor to which the notification is to be sent.
 *  @arg    eventNo
 *              Event number to be used.
 *  @arg    payload
 *              Data to be sent along with the event.
 *  @arg    waitClear
 *              Indicates whether the function should wait for previous event
 *              to be cleared, or sending single event is sufficient without
 *              payload.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EDRIVERINIT
 *              Remote Notify driver is not setup to receive events.
 *          NOTIFY_ENOTREADY
 *              Other side is not ready to receive an event. This can be due to
 *              one of two reasons:
 *              1. No client is registered for this event on the other side
 *              2. Other side has disabled the event
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *          The event must be supported by the Notify driver.
 *
 *  @leave  On success, the event must be sent to the specified processor.
 *
 *  @see    Notify_Interface, Notify_sendEvent ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_sendEvent (IN Notify_DriverHandle handle,
                        IN Processor_Id        procId,
                        IN Uint32              eventNo,
                        IN Uint32              payload,
                        IN Bool                waitClear) ;

/** ============================================================================
 *  @func   NotifyShmDrv_disable
 *
 *  @desc   Disable all events for this Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *
 *  @ret    flags
 *              Flags to be provided when NotifyShmDrv_restore is called.
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *
 *  @leave  On success, all events for the Notify driver must be disabled
 *
 *  @see    Notify_Interfacej, Notify_disable ()
 *  ============================================================================
 */
NORMAL_API
Void *
NotifyShmDrv_disable (IN Notify_DriverHandle handle) ;

/** ============================================================================
 *  @func   NotifyShmDrv_restore
 *
 *  @desc   Restore the Notify driver to the state before the last disable was
 *          called.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    flags
 *              Flags returned from the call to last NotifyShmDrv_disable in order
 *              to restore the Notify driver to the state before the last
 *              disable call.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EPOINTER
 *              Invalid pointer passed
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *          flags must be the same as what was returned from the previous
 *          NotifyShmDrv_disable call.
 *
 *  @leave  On success, all events for the Notify driver must be restored to
 *          the state as indicated by the provided flags.
 *
 *  @see    Notify_Interface, Notify_restore ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_restore (IN Notify_DriverHandle handle,
                      IN Void *              flags) ;

/** ============================================================================
 *  @func   NotifyShmDrv_disableEvent
 *
 *  @desc   Disable a specific event for this Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    procId
 *              ID of the processor.
 *  @arg    eventNo
 *              Event number to be disabled.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *          The event must be supported by the Notify driver.
 *
 *  @leave  On success, the event must be disabled.
 *
 *  @see    Notify_Interface, Notify_disableEvent ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_disableEvent (IN Notify_DriverHandle handle,
                           IN Processor_Id        procId,
                           IN Uint32              eventNo) ;

/** ============================================================================
 *  @func   NotifyShmDrv_enableEvent
 *
 *  @desc   Enable a specific event for this Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *  @arg    procId
 *              ID of the processor.
 *  @arg    eventNo
 *              Event number to be enabled.
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *          The event must be supported by the Notify driver.
 *
 *  @leave  On success, the event must be enabled.
 *
 *  @see    Notify_Interface, Notify_enableEvent ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_enableEvent (IN Notify_DriverHandle handle,
                          IN Processor_Id        procId,
                          IN Uint32              eventNo) ;

#if defined (NOTIFY_DEBUG)
/** ============================================================================
 *  @func   NotifyShmDrv_debug
 *
 *  @desc   Print debug information for the Notify driver.
 *
 *  @arg    handle
 *              Handle to the Notify driver
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module and driver must be initialized before calling this
 *          function.
 *          handle must be a valid Notify driver handle
 *
 *  @leave  None.
 *
 *  @see    Notify_Interface, Notify_debug ()
 *  ============================================================================
 */
NORMAL_API
Notify_Status
NotifyShmDrv_debug (IN Notify_DriverHandle handle) ;
#endif /* defined (NOTIFY_DEBUG) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (_NOTIFY_SHMDRIVER_H_) */

