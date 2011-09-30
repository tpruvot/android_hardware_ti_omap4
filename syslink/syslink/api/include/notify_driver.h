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
 *  @file   notify_driver.h
 *
 *  @desc   Defines data types and structures used by Notify driver writers.
 *  ============================================================================
 */


#if !defined (NOTIFYDRIVER_H)
#define NOTIFYDRIVER_H


/*  ----------------------------------- IPC */
#include <ipctypes.h>

/*  ----------------------------------- Notify */
#include <notifyerr.h>

/*  ----------------------------------- Notify driver */
#include <notify_driverdefs.h>


#if defined (__cplusplus)
EXTERN "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @func   Notify_registerDriver
 *
 *  @desc   This function registers a Notify driver with the Notify module.
 *          Each Notify driver must register itself with the Notify module. Once
 *          the registration is done, the user can start using the Notify APIs
 *          to interact with the Notify driver for sending and receiving
 *          notifications.
 *
 *  @arg    driverName
 *              Name of the Notify driver
 *  @arg    fnTable
 *              Pointer to the function table for this Notify driver
 *  @arg    drvAttrs
 *              Attributes of the Notify driver relevant to the generic Notify
 *              module
 *  @arg    driverHandle
 *              Location to receive the pointer to the Notify driver handle
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINIT
 *              The Notify module or driver was not initialized
 *          NOTIFY_ERESOURCE
 *              The maximum number of drivers have already been registered with
 *              the Notify module
 *          NOTIFY_EALREADYEXISTS
 *              The specified Notify driver is already registered.
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EPOINTER
 *              An invalid pointer was specified
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module must be initialized before calling this function.
 *          driverName must be a valid string.
 *          fnTable must be valid.
 *          driverAttrs must be valid.
 *          driverHandle must be a valid pointer.
 *
 *  @leave  On success, the Notify driver must be registered with the Notify
 *          module
 *
 *  @see    Notify_Interface, Notify_DriverAttrs, Notify_unregisterDriver ()
 *  ============================================================================
 */
EXPORT_API
Notify_Status
Notify_registerDriver (IN  Char8 *               driverName,
                       IN  Notify_Interface *    fnTable,
                       IN  Notify_DriverAttrs *  drvAttrs,
                       OUT Notify_DriverHandle * driverHandle) ;


/** ============================================================================
 *  @func   Notify_unregisterDriver
 *
 *  @desc   This function un-registers a Notify driver with the Notify module.
 *          When the Notify driver is no longer required, it can un-register
 *          itself with the Notify module. This facility is also useful if a
 *          different type of Notify driver needs to be plugged in for the same
 *          physical interrupt to meet different needs.
 *
 *  @arg    drvHandle
 *              Handle to the Notify driver object
 *
 *  @ret    NOTIFY_SOK
 *              Operation successfully completed
 *          NOTIFY_EINIT
 *              The Notify module or driver was not initialized
 *          NOTIFY_ENOTFOUND
 *              The specified driver is not registered with the Notify module
 *          NOTIFY_EHANDLE
 *              Invalid Notify handle specified
 *          NOTIFY_EMEMORY
 *              Operation failed due to a memory error
 *          NOTIFY_EINVALIDARG
 *              Invalid argument
 *          NOTIFY_EFAIL
 *              General failure
 *
 *  @enter  The Notify module must be initialized before calling this function.
 *          handle must be a valid Notify driver handle.
 *
 *  @leave  On success, the Notify driver must be unregistered with the Notify
 *          module
 *
 *  @see    Notify_DriverHandle, Notify_registerDriver ()
 *  ============================================================================
 */
EXPORT_API
Notify_Status
Notify_unregisterDriver (IN  Notify_DriverHandle drvHandle) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (NOTIFYDRIVER_H) */
