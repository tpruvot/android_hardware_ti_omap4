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
 *  @desc   Defines the direct user interface for the Notify driver using
 *          Shared Memory and interrupts to communicate with the remote
 *          processor.
 *  ============================================================================
 */


#if !defined (NOTIFY_SHMDRIVER_H_)
#define NOTIFY_SHMDRIVER_H_


/*  ----------------------------------- IPC */
#include <ipctypes.h>

/*  ----------------------------------- Notify       */
#include <notifyerr.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  NOTIFYSHMDRV_DRIVERNAME
 *
 *  @desc   Name of the Notify Shared Memory Mailbox driver.
 *  ============================================================================
 */
#define NOTIFYSHMDRV_DRIVERNAME   "NOTIFDUCATIDRV"

/** ============================================================================
 *  @const  NOTIFYSHMDRV_RESERVED_EVENTS
 *
 *  @desc   Maximum number of events marked as reserved events by the
 *          NotiyShmDrv driver.
 *          If required, this value can be changed by the system integrator.
 *  ============================================================================
 */
#define NOTIFYSHMDRV_RESERVED_EVENTS  3


/** ============================================================================
 *  @name   NotifyShmDrv_Attrs
 *
 *  @desc   This structure defines the attributes for Notify Shared Memory
 *          Mailbox driver.
 *          These attributes are passed to the driver when Notify_driverInit ()
 *          is called for this driver.
 *
 *  @field  shmBaseAddr
 *              Shared memory address base for the NotifyShmDrv driver. This
 *              must be the start of shared memory as used by both connected
 *              processors, and the same must be specified on both sides when
 *              initializing the NotifyShmDrv driver.
 *  @field  shmSize
 *              Size of shared memory provided to the NotifyShmDrv driver. This
 *              must be the start of shared memory as used by both connected
 *              processors, and the same must be specified on both sides when
 *              initializing the NotifyShmDrv driver.
 *  @field  numEvents
 *              Number of events required to be supported. Must be greater than
 *              or equal to reserved events supported by the driver.
 *  @field  sendEventPollCount
 *              Poll count to be used when sending event. If the count is
 *              specified as -1, the wait will be infinite. NOTIFY_sendEvent
 *              will return with timeout error if the poll count expires before
 *              the other processor acknowledges the received event.
 *
 *  @see    None.
 *  ============================================================================
 */
typedef struct NotifyShmDrv_Attrs_tag {
    Uint32    shmBaseAddr ;
    Uint32    shmSize ;
    Uint32    numEvents ;
    Uint32    sendEventPollCount ;
} NotifyShmDrv_Attrs ;


/** ============================================================================
 *  @name   NotifyShmDrv_init
 *
 *  @desc   Top-level initialization function for the Notify shared memory
 *          mailbox driver.
 *          This can be plugged in as the user init function.
 *
 *  @arg    None.
 *
 *  @ret    None.
 *
 *  @enter  Notify module must have been initialized before this call
 *
 *  @leave  On success, the driver is registered with the Notify module.
 *
 *  @see    NotifyShmDrv_exit ()
 *  ============================================================================
 */
EXPORT_API
Void
NotifyShmDrv_init (Void) ;

/** ============================================================================
 *  @name   NotifyShmDrv_exit
 *
 *  @desc   Top-level finalization function for the Notify shared memory
 *          mailbox driver.
 *
 *  @arg    None.
 *
 *  @ret    None.
 *
 *  @enter  Notify module must have been initialized before this call
 *
 *  @leave  On success, the driver is unregistered with the Notify module.
 *
 *  @see    NotifyShmDrv_init ()
 *  ============================================================================
 */
EXPORT_API
Void
NotifyShmDrv_exit (Void) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (NOTIFY_SHMDRIVER_H_) */

