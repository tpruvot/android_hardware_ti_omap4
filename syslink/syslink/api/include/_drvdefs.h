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
 *  @file   _drvdefs.h
 *
 *
 *  @desc   Defines internal data types and structures used by Linux Druver
 *  ============================================================================
 */


#if !defined (_DRVDEFS_H)
#define _DRVDEFS_H

/*  ----------------------------------- IPC Headers                 */
#include <gpptypes.h>
#include <ipctypes.h>
#include <dbc.h>

/*  ----------------------------------- IPC */
#include <ipctypes.h>

/*  ----------------------------------- NOTIFY Headers*/
#include <notify.h>

#if defined (__cplusplus)
EXTERN "C" {
#endif /* defined (__cplusplus) */


/*  ============================================================================
 *  @macro  CMD_NOTIFY_XXXX
 *
 *  @desc   Command ids for NOTIFY functions.
 *  ============================================================================
 */
#define NOTIFY_BASE_CMD                      (0x100)
#define NOTIFY_DRV_CMD_DRIVERINIT            (NOTIFY_BASE_CMD + 1)
#define NOTIFY_DRV_CMD_DRIVEREXIT            (NOTIFY_BASE_CMD + 2)
#define NOTIFY_DRV_CMD_REGISTEREVENT         (NOTIFY_BASE_CMD + 3)
#define NOTIFY_DRV_CMD_UNREGISTEREVENT       (NOTIFY_BASE_CMD + 4)
#define NOTIFY_DRV_CMD_SENDEVENT             (NOTIFY_BASE_CMD + 5)
#define NOTIFY_DRV_CMD_DISABLE               (NOTIFY_BASE_CMD + 6)
#define NOTIFY_DRV_CMD_RESTORE               (NOTIFY_BASE_CMD + 7)
#define NOTIFY_DRV_CMD_DISABLEEVENT          (NOTIFY_BASE_CMD + 8)
#define NOTIFY_DRV_CMD_ENABLEEVENT           (NOTIFY_BASE_CMD + 9)


/** ============================================================================
 *  @name   CMD_Args
 *
 *  @desc   Union defining arguments to be passed to ioctl calls. For the
 *          explanation of individual field please see the corresponding APIs.

 *  @field  apiStatus
 *              Status returned by this API.
 *          apiArgs
 *              Union representing arguments for different APIs.
 *  ============================================================================
 */
typedef struct Notify_CmdArgs_tag {
    Notify_Status apiStatus ;
    union {
        struct {
            Notify_Handle   handle ;
            Notify_Config * config ;
            Char8 *         driverName ;
        } driverInitArgs ;

        struct {
            Notify_Handle   handle ;
        } driverExitArgs ;

        struct {
            Notify_Handle   handle ;
            Uint32          eventNo ;
            Processor_Id     procId ;
            FnNotifyCbck    fnNotifyCbck ;
            Void *          cbckArg ;
        } unregisterEventArgs ;

        struct {
            Notify_Handle   handle ;
            Uint32          eventNo ;
            Processor_Id     procId ;
            FnNotifyCbck    fnNotifyCbck ;
            Void *          cbckArg ;
        } registerEventArgs ;

        struct {
            Notify_Handle   handle ;
            Uint32          eventNo ;
            Processor_Id    procId ;
            Uint32          payload;
            Bool            waitClear;
        } sendEventArgs ;

        struct {
            Void *          disableFlags ;
        } disableArgs ;

        struct {
            Void *          restoreFlags ;
        } restoreArgs ;

        struct {
            Notify_Handle   handle ;
            Uint32          eventNo ;
            Processor_Id     procId ;
        } disableEventArgs ;

        struct {
            Notify_Handle   handle ;
            Uint32          eventNo ;
            Processor_Id     procId ;
        } enableEventArgs ;
    } apiArgs ;
} Notify_CmdArgs ;

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (_DRVDEFS_H) */
