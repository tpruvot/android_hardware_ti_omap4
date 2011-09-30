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
 *  @file       Notify.h
 *
 *  @brief      Notification manager for IPC
 *
 *  The Notify module manages the multiplexing/demultiplexing of software
 *  interrupts over hardware interrupts.  In order to receive notifications,
 *  a processor registers one or more callback functions to an eventId
 *  using #Notify_registerEvent.  The #Notify_registerEvent
 *  call (like most other Notify APIs) uses a MultiProc id and
 *  line id to target a specific interrupt line to/from a specific processor
 *  on a device.
 *
 *  Once an event has been registered, a remote processor may "send" an event
 *  using the #Notify_sendEvent call.  If the event and the interrupt line
 *  are both enabled, all callback functions registered to the event will
 *  be called sequentially.
 *
 *  A specific event may be disabled or enabled using the #Notify_disableEvent
 *  and #Notify_enableEvent calls.  An entire interrupt line may be disabled
 *  or restored using the #Notify_disable or #Notify_restore calls.  The
 *  Notify_disable call does not alter the state of individual events.
 *  Instead, it just disables the ability of the Notify module to receive
 *  events on the interrupt line.
 *
 *  Notify APIs should never be called within an Hwi context.  All API calls
 *  should be made within main(), a Task or a Swi.
 *
 *  "Loopback" functionality allows Notifications to be registered
 *  and sent locally.  This is accomplished by supplying our own MultiProc id
 *  to Notify APIs. Line id #0 is always used for local notifications.  It is
 *  important to be aware of some subtle (but important) differences between
 *  remote and local notifications:
 *
 *      - Loopback callback functions will execute in the same thread in which
 *        #Notify_sendEvent is called.  This is in contrast to callback
 *        functions that are called due to another processor's sent
 *        notification- these 'remote' callback functions will execute in an
 *        ISR context.
 *
 *      - Disabling the local interrupt line will cause all notifications that
 *        are sent to the local processor to be lost.  By contrast, a
 *        notification sent to an enabled event on a remote processor that has
 *        called #Notify_disable results in a pending notifications until the
 *        disabled processor has called #Notify_restore
 *
 *      - Local notifications do not support events of different priorities.
 *        By contrast, Notify driver implementations may correlate event ids
 *        with varying priorities.
 *
 *  In order to use any Notify APIs on DSP/BIOS, IPC/Syslink must first be
 *  started.  This will internally call #Notify_attach which sets up
 *  all necessary Notify drivers, shared memory and interprocessor interrupts.
 *  It is possible for a user application to call #Notify_attach directly
 *  if notification must be set up prior to runtime SharedRegion
 *  initialization. Refer to the documentation for #Notify_attach for more
 *  information.
 *
 *  ============================================================================
 */

#ifndef ti_ipc_Notify__include
#define ti_ipc_Notify__include

#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
 *  @def    Notify_S_BUSY
 *  @brief  The resource is still in use
 */
#define Notify_S_BUSY               2

/*!
 *  @def    Notify_S_ALREADYSETUP
 *  @brief  Module already set up
 */
#define Notify_S_ALREADYSETUP       1

/*!
 *  @def    Notify_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define Notify_S_SUCCESS            0

/*!
 *  @def    Notify_E_FAIL
 *  @brief  Generic failure.
 */
#define Notify_E_FAIL               -1

/*!
 *  @def    Notify_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define Notify_E_INVALIDARG          -2

/*!
 *  @def    Notify_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define Notify_E_MEMORY              -3

/*!
 *  @def    Notify_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define Notify_E_ALREADYEXISTS       -4

/*!
 *  @def    Notify_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define Notify_E_NOTFOUND            -5

/*!
 *  @def    Notify_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define Notify_E_TIMEOUT             -6

/*!
 *  @def    Notify_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define Notify_E_INVALIDSTATE        -7

/*!
 *  @def    Notify_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define Notify_E_OSFAILURE           -8

/*!
 *  @def    Notify_E_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define Notify_E_ALREADYSETUP        -9

/*!
 *  @def    Notify_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define Notify_E_RESOURCE            -10

/*!
 *  @def    Notify_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define Notify_E_RESTART             -11

/*!
 *  @def    Notify_E_BUSY
 *  @brief  The resource is still in use
 */
#define Notify_E_BUSY                -12

/*!
 *  @def    Notify_E_DRIVERNOTREGISTERED
 *  @brief  Driver corresponding to the specified eventId is not registered
 */
#define Notify_E_DRIVERNOTREGISTERED    -13

/*!
 *  @def    Notify_E_EVTNOTREGISTERED
 *  @brief  Event not registered
 */
#define Notify_E_EVTNOTREGISTERED       -14

/*!
 *  @def    Notify_E_EVTDISABLED
 *  @brief  Event is disabled
 */
#define Notify_E_EVTDISABLED            -15

/*!
 *  @def    Notify_E_NOTINITIALIZED
 *  @brief  Remote notification is not initialized
 */
#define Notify_E_NOTINITIALIZED         -16

/*!
 *  @def    Notify_E_EVTRESERVED
 *  @brief  Trying to illegally use a reserved event
 */
#define Notify_E_EVTRESERVED            -17

/*!
 *  @def    Notify_MAXEVENTS
 *  @brief  Maximum number of events supported by the Notify module
 */
#define Notify_MAXEVENTS  (UInt16)32

/*!
 *  @def    Notify_MAX_INTLINES
 *  @brief  Maximum number of IPC interrupt lines per processor.
 */
#define Notify_MAX_INTLINES   4u

/*!
 *  @def    Notify_SYSTEMKEY
 *  @brief  This key must be provided as the upper 16 bits of the eventId when
 *          registering for an event, if any reserved event numbers are to be
 *          used.
 */
#define Notify_SYSTEMKEY ((UInt16)0xC1D2)

/*!
 *  @def     Notify_FnNotifyCbck
 *  @brief   Signature of any callback function that can be registered with the
 *           Notify component.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  eventId     Event id (minus system key if reserved event)
 *  @param[in]  arg         Argument specified in the registerEvent
 *  @param[in]  payload     Payload specified in the sendEvent
 */
typedef Void (*Notify_FnNotifyCbck)(UInt16 , UInt16, UInt32, UArg, UInt32);

/* =============================================================================
 *  APIs
 * =============================================================================
 */

/*!
 *  @brief      Disable the NotifyDriver from receiving events
 *
 *  This function disables a NotifyDriver from processing received
 *  events. The key that is returned from this call must be used
 *  in the #Notify_restore function.
 *
 *  Notify supports nested disable/restore calls. The last restore call
 *  will re-enable Notifications.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *
 *  @return     Key that must be used in #Notify_restore
 *
 *  @sa         Notify_restore
 */
UInt Notify_disable(UInt16 procId, UInt16 lineId);

/*!
 *  @brief      Disable an event
 *
 *  This function allows the disabling of a single event number
 *  from the specified source processor.  Sending to a disabled event
 *  will return #Notify_E_EVTDISABLED on the sender if waitClear is false.
 *
 *  An event is, by default, enabled upon driver initialization.
 *  Calling #Notify_disableEvent upon an event that is already disabled
 *  results in no change in state.
 *
 *  Note that callbacks may be registered to an event or removed
 *  from the event even while the event has been disabled.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  eventId     Event id
 *
 *  @sa         Notify_enableEvent
 */
Void Notify_disableEvent(UInt16 procId, UInt16 lineId, UInt32 eventId);

/*!
 *  @brief      Enable an event
 *
 *  This function re-enables an event that has been previously disabled
 *  using disableEvent().  Calling Notify_enableEvent upon an event that is
 *  already enabled results in no change in state.  An event is,
 *  by default, enabled upon driver initialization.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  eventId     Event id
 *
 *  @sa         Notify_disableEvent
 */
Void Notify_enableEvent(UInt16 procId, UInt16 lineId, UInt32 eventId);

/*!
 *  @brief      Whether notification via interrupt line has been registered.
 *
 *  This function will return TRUE if and only if a notify driver has been
 *  registered for the interrupt line identified by the supplied procId and
 *  lineId.  The interrupt line corresponding to loopback functionality is
 *  always registered.  A value of FALSE indicates that either
 *  #Notify_attach has not yet been called or that notification to the
 *  remote processor is unsupported.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *
 *  @return     TRUE if registered, FALSE otherwise
 */
Bool Notify_isRegistered(UInt16 procId, UInt16 lineId);

/*! @cond */
/*!
 *  @brief  Returns the amount of shared memory used by one Notify instance.
 *
 *  This will typically be used internally by other IPC modules during system
 *  initialization.  The return value depends upon the base address because
 *  of cache alignment settings.
 *
 *  @param[in]  sharedAddr      Base address that will be passed to
 *                              #Notify_start
 *
 *  @return     Shared memory required (in MAUs)
 */
SizeT Notify_sharedMemReq(UInt16 procId, Ptr sharedAddr);
/*! @endcond */

/*!
 *  @brief      Register a callback for an event (supports multiple callbacks)
 *
 *  This function registers a callback to a specific event number,
 *  processor id and interrupt line. When the event is received by the
 *  specified processor, the callback is called.
 *
 *  The callback function prototype is of type #Notify_FnNotifyCbck.
 *  This function must be non-blocking.
 *
 *  It is important to note that multiple callbacks may be registered with
 *  a single event.  This is accomplished by simply calling
 *  Notify_registerEvent for each combination of callback functions and
 *  callback arguments as needed.
 *
 *  It is important to note that interrupts are disabled during the entire
 *  duration of this function's execution.
 *
 *  @param[in]  procId          Remote processor id
 *  @param[in]  lineId          Line id (0 for most systems)
 *  @param[in]  eventId         Event id
 *  @param[in]  fnNotifyCbck    Pointer to callback function
 *  @param[in]  cbckArg         Callback function argument
 *
 *  @return     Notify status:
 *              - #Notify_S_SUCCESS: Event successfully registered
 */
Int Notify_registerEvent(UInt16 procId,
                         UInt16 lineId,
                         UInt32 eventId,
                         Notify_FnNotifyCbck fnNotifyCbck,
                         UArg cbckArg);

/*!
 *  @brief      Register a single callback for an event
 *
 *  This function registers a callback to a specific event number,
 *  processor id and interrupt line. When the event is received by the
 *  specified processor, the callback is called.
 *
 *  The callback function prototype is of type #::Notify_FnNotifyCbck.
 *  The callback function must be non-blocking.
 *
 *  Only one callback may be registered with this API.
 *
 *  Use #Notify_registerEvent to register multiple callbacks for a single event.
 *
 *  @param[in]  procId          Remote processor id
 *  @param[in]  lineId          Line id (0 for most systems)
 *  @param[in]  eventId         Event id
 *  @param[in]  fnNotifyCbck    Pointer to callback function
 *  @param[in]  cbckArg         Callback function argument
 *
 *  @return     Notify status:
 *              - #Notify_E_ALREADYEXISTS: Event already registered
 *              - #Notify_S_SUCCESS: Event successfully registered
 */
Int Notify_registerEventSingle(UInt16 procId,
                               UInt16 lineId,
                               UInt32 eventId,
                               Notify_FnNotifyCbck fnNotifyCbck,
                               UArg cbckArg);

/*!
 *  @brief      Restore a NotifyDriver instance
 *
 *  This function restores a NotifyDriver's ability to process received
 *  events.
 *
 *  Notify supports nested disable/restore calls. The last restore call
 *  will re-enable Notifications.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  key         Key returned by #Notify_disable
 *
 *  @sa         Notify_disable
 */
Void Notify_restore(UInt16 procId, UInt16 lineId, UInt key);

/*!
 *  @brief      Send an event on an interrupt line
 *
 *  This function sends an event to a processor via an interrupt line
 *  identified by a processor id and line id. A payload may be optionally
 *  sent to the the remote processor if supported by the device.
 *
 *  On the destination processor, the callback functions registered
 *  with Notify with the associated eventId and source
 *  processor id are called.
 *
 *  The 'waitClear' flag indicates that, if an event was previously sent
 *  to the same eventId, sendEvent should wait until the previous event has
 *  been acknowledged by the remote processor.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  eventId     Event id
 *  @param[in]  payload     Payload to be sent along with the event.
 *  @param[in]  waitClear   Indicates whether Notify driver will wait for
 *                          previous event to be cleared. If a payload needs to
 *                          be sent across, this must be TRUE.
 *
 *  @return     Notify status:
 *              - #Notify_E_EVTNOTREGISTERED: event has no registered callback
 *                functions
 *              - #Notify_E_NOTINITIALIZED: remote driver has not yet been
 *                initialized
 *              - #Notify_E_EVTDISABLED: remote event is disabled
 *              - #Notify_E_TIMEOUT: timeout occured (when waitClear is TRUE)
 *              - #Notify_S_SUCCESS: event successfully sent
 */
Int Notify_sendEvent(UInt16 procId,
                     UInt16 lineId,
                     UInt32 eventId,
                     UInt32 payload,
                     Bool waitClear);

/*!
 *  @brief      Creates notify drivers and registers them with Notify
 *
 *  This function must be called before other Notify API calls are
 *  invoked.  Performing a attach invokes device-specific Notify
 *  initialization routine that creates required drivers and
 *  registers those drivers with the Notify module.  If the drivers
 *  require shared memory, a shared address must be supplied.
 *
 *  Notify_attach is typically called internally as part of the IPC
 *  initialization sequence.  Any memory required for Notify drivers
 *  is reserved by SharedRegion and passed to Notify_attach.  However, if it
 *  is necessary to pass a specific base address to Notify_attach (i.e. if
 *  the shared region is not valid yet), then Notify_attach should be called
 *  before Ipc_start with this address.  When Ipc_start is eventually
 *  called, Notify_attach will be internally bypassed since it was directly
 *  called by the user.
 *
 *  @param[in]  remoteProcId    Remote processor id
 *  @param[in]  sharedAddr      Shared address to use if any driver requires
 *                              shared memory
 *  @return     Notify status:
 *              - #Notify_E_FAIL: failed to attach to remote processor
 *              - #Notify_S_SUCCESS: successfully attach to remote processor
 */
Int Notify_attach(UInt16 remoteProcId, Ptr sharedAddr);

/*!
 *  @brief      Remove an event listener from an event
 *
 *  This function removes a previously registered callback
 *  with the driverHandle. The procId, lineId, eventId, fnNotifyCbck and
 *  cbckArg must exactly match with the registered one.
 *
 *  This API is used to unregister events that were registered with
 *  #Notify_registerEvent.  If this is the last event, then
 *  #Notify_unregisterEventSingle is called to completely remove the event.
 *
 *  It is important to note that interrupts are disabled during the entire
 *  duration of this function's execution.  This duration may be quite large
 *  especially because a list of callback functions must be traversed
 *  and several internal function calls need to be made.
 *
 *  @param[in]  procId          Remote processor id
 *  @param[in]  lineId          Line id
 *  @param[in]  eventId         Event id
 *  @param[in]  fnNotifyCbck    Pointer to callback function
 *  @param[in]  cbckArg         Callback function argument
 *
 *  @return     Notify status:
 *              - #Notify_E_NOTFOUND: event listener not found
 *              - #Notify_S_SUCCESS: event listener unregistered
 *
 *  @sa         Notify_registerEvent
 */
Int Notify_unregisterEvent(UInt16 procId, UInt16 lineId, UInt32 eventId,
                           Notify_FnNotifyCbck fnNotifyCbck, UArg cbckArg);

/*!
 *  @brief      Remove an event listener from an event
 *
 *  This function removes a previously registered callback
 *  with the driverHandle. The procId, lineId, and eventId must exactly match
 *  the registered one.
 *
 *  @param[in]  procId      Remote processor id
 *  @param[in]  lineId      Line id
 *  @param[in]  eventId     Event id that is being unregistered
 *
 *  @return     Notify status:
 *              - #Notify_E_EVTNOTREGISTERED: event was not registered.
 *              - #Notify_S_SUCCESS: event unregistered
 */
Int Notify_unregisterEventSingle(UInt16 procId, UInt16 lineId, UInt32 eventId);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_Notify__include */
