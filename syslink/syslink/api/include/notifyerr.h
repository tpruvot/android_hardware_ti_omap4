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
 *  @file   notifyerr.h
 *
 *  @desc   Central repository for error and status code bases and ranges for
 *          Notify module and any Algorithm Framework built on top of it.
 *  ============================================================================
 */


#if !defined (NOTIFYERR_H)
#define NOTIFYERR_H


/*  ----------------------------------- IPC */
#include <ipctypes.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @name   Notify_Status
 *
 *  @desc   Defines the type for the return status codes from the Notify module
 *  ============================================================================
 */
typedef Int32  Notify_Status ;


/** ============================================================================
 *  @name   NOTIFY_SUCCEEDED
 *
 *  @desc   Check if the provided status code indicates a success code.
 *
 *  @arg    status
 *              Status code to be checked
 *
 *  @ret    TRUE
 *              If status code indicates success
 *          FALSE
 *              If status code indicates failure
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    NOTIFY_FAILED
 *  ============================================================================
 */
#define NOTIFY_SUCCEEDED(status)                  \
              (    ((Notify_Status) (status) >= (NOTIFY_SBASE))   \
               &&  ((Notify_Status) (status) <= (NOTIFY_SLAST)))


/** ============================================================================
 *  @name   NOTIFY_FAILED
 *
 *  @desc   Check if the provided status code indicates a failure code.
 *
 *  @arg    status
 *              Status code to be checked
 *
 *  @ret    TRUE
 *              If status code indicates failure
 *          FALSE
 *              If status code indicates success
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    NOTIFY_FAILED
 *  ============================================================================
 */
#define NOTIFY_FAILED(status)      (!NOTIFY_SUCCEEDED (status))



/** ============================================================================
 *  @name   NOTIFY_SBASE, NOTIFY_SLAST
 *
 *  @desc   Defines the base and range for the success codes used by the
 *          Notify module
 *  ============================================================================
 */
#define NOTIFY_SBASE               (Notify_Status)0x00002000l
#define NOTIFY_SLAST               (Notify_Status)0x000020FFl

/** ============================================================================
 *  @name   NOTIFY_EBASE, NOTIFY_ELAST
 *
 *  @desc   Defines the base and range for the failure codes used by the
 *          Notify module
 *  ============================================================================
 */
#define NOTIFY_EBASE               (Notify_Status)0x80002000l
#define NOTIFY_ELAST               (Notify_Status)0x800020FFl


/*  ============================================================================
 *  SUCCESS Codes
 *  ============================================================================
 */

/* Generic success code for Notify module */
#define NOTIFY_SOK                (NOTIFY_SBASE + 0x01l)

/* Indicates that the Notify module (or driver) has already been initialized
 * by another client, and this process has now successfully acquired the right
 * to use the Notify module.
 */
#define NOTIFY_SALREADYINIT       (NOTIFY_SBASE + 0x02l)

/* Indicates that the Notify module (or driver) is now being finalized, since
 * the calling client is the last one finalizing the module, and all open
 * handles to it have been closed.
 */
#define NOTIFY_SEXIT              (NOTIFY_SBASE + 0x03l)


/*  ============================================================================
 *  FAILURE Codes
 *  ============================================================================
 */

/* Generic failure code for Notify module */
#define NOTIFY_EFAIL              (NOTIFY_EBASE + 0x01l)

/* This failure code indicates that an operation has timed out. */
#define NOTIFY_ETIMEOUT           (NOTIFY_EBASE + 0x02l)

/* This failure code indicates a configuration error */
#define NOTIFY_ECONFIG            (NOTIFY_EBASE + 0x03l)

/* This failure code indicates that the Notify module has already been
 * initialized from the calling client (process).
 */
#define NOTIFY_EALREADYINIT       (NOTIFY_EBASE + 0x04l)

/* This failure code indicates that the specified entity was not found
 * The interpretation of this error code depends on the function from which it
 * was returned.
 */
#define NOTIFY_ENOTFOUND          (NOTIFY_EBASE + 0x05l)

/* This failure code indicates that the specified feature is not supported
 * The interpretation of this error code depends on the function from which it
 * was returned.
 */
#define NOTIFY_ENOTSUPPORTED      (NOTIFY_EBASE + 0x06l)

/* This failure code indicates that the specified event number is not supported
 */
#define NOTIFY_EINVALIDEVENT      (NOTIFY_EBASE + 0x07l)

/* This failure code indicates that the specified pointer is invalid */
#define NOTIFY_EPOINTER           (NOTIFY_EBASE + 0x08l)

/* This failure code indicates that a provided parameter was outside its valid
 * range.
 * The interpretation of this error code depends on the function from which it
 * was returned.
 */
#define NOTIFY_ERANGE             (NOTIFY_EBASE + 0x09l)

/* This failure code indicates that the specified handle is invalid */
#define NOTIFY_EHANDLE            (NOTIFY_EBASE + 0x0Al)

/* This failure code indicates that an invalid argument was specified */
#define NOTIFY_EINVALIDARG        (NOTIFY_EBASE + 0x0Bl)

/* This failure code indicates a memory related failure */
#define NOTIFY_EMEMORY            (NOTIFY_EBASE + 0x0Cl)

/* This failure code indicates that the Notify module has not been initialized*/
#define NOTIFY_EINIT              (NOTIFY_EBASE + 0x0Dl)

/* This failure code indicates that a resource was not available.
 * The interpretation of this error code depends on the function from which it
 * was returned.
 */
#define NOTIFY_ERESOURCE          (NOTIFY_EBASE + 0x0El)

/* This failure code indicates that there was an attempt to register for a
 * reserved event.
 */
#define NOTIFY_ERESERVEDEVENT     (NOTIFY_EBASE + 0x0Fl)

/* This failure code indicates that the specified entity already exists.
 * The interpretation of this error code depends on the function from which it
 * was returned.
 */
#define NOTIFY_EALREADYEXISTS     (NOTIFY_EBASE + 0x10l)

/* This failure code indicates that the Notify driver has not been fully
 * initialized
 */
#define NOTIFY_EDRIVERINIT        (NOTIFY_EBASE + 0x11l)

/* This failure code indicates that the other side is not ready to receive
 * notifications.
 */
#define NOTIFY_ENOTREADY          (NOTIFY_EBASE + 0x12l)


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (NOTIFYERR_H) */
