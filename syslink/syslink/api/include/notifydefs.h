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
 *  @file   notifydefs.h
 *
 *  @desc   Defines data types and structures used by Notify module
 *
 *  ============================================================================
 */


#if !defined (NOTIFYDEFS_H)
#define NOTIFYDEFS_H


/*  ----------------------------------- IPC */
#include <ipctypes.h>

/*  ----------------------------------- Notify */
#include <notifyerr.h>


#if defined (__cplusplus)
EXTERN "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  NOTIFY_MAX_DRIVERS
 *
 *  @desc   Maximum number of Notify drivers supported.
 *          NOTE: This can be modified by the user if data section size needs to
 *                be reduced, or if there is a need for more than the defined
 *                max drivers.
 *  ============================================================================
 */
#define NOTIFY_MAX_DRIVERS   16

/** ============================================================================
 *  @const  NOTIFY_MAX_NAMELEN
 *
 *  @desc   Maximum length of the name of Notify drivers, inclusive of NULL
 *          string terminator.
 *  ============================================================================
 */
#define NOTIFY_MAX_NAMELEN   32

/** ============================================================================
 *  @name   Notify_Handle
 *
 *  @desc   This typedef defines the type for the handle to the Notify driver.
 *  ============================================================================
 */
typedef Void * Notify_Handle ;


/** ============================================================================
 *  @name   FnNotifyCbck
 *
 *  @desc   Signature of the callback function to be registered with the NOTIFY
 *          component.
 *
 *  @arg    procId
 *              Processor ID from which the event is being received
 *  @arg    eventNo
 *              Event number for which the callback is being received
 *  @arg    arg
 *              Fixed argument registered with the IPS component along with
 *              the callback function.
 *  @arg    payload
 *              Run-time information provided to the upper layer by the Notify
 *              component. Depending on the Notify driver implementation, this
 *              may or may not pass on a user-specific value to the registered
 *              application
 *
 *  @ret    None.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None.
 *  ============================================================================
 */
typedef Void (*FnNotifyCbck) (IN     Processor_Id procId,
                              IN     Uint32       eventNo,
                              IN OPT Void *       arg,
                              IN OPT Uint32       payload) ;


/** ============================================================================
 *  @name   Notify_Attrs
 *
 *  @desc   This structure defines attributes for initialization of the Notify
 *          module.
 *
 *  @field  maxDrivers
 *              Maximum number of drivers that can be registered with the Notify
 *              module.
 *
 *  @see    Notify_init ()
 *  ============================================================================
 */
typedef struct Notify_Attrs_tag {
    Uint32   maxDrivers ;
} Notify_Attrs ;

/** ============================================================================
 *  @name   Notify_Config
 *
 *  @desc   This structure defines the configuration structure for
 *          initialization of the Notify driver.
 *
 *  @field  driverAttrs
 *              Notify driver-specific attributes
 *
 *  @see    Notify_driverInit ()
 *  ============================================================================
 */
typedef struct Notify_Config_tag {
    Void *   driverAttrs ;
} Notify_Config ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (NOTIFYDEFS_H) */
