/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2010, Texas Instruments Incorporated
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
/*============================================================================
 *  @file   ProcDEH.c
 *
 *  @brief   Processor Device Error Handler module
 *
 *  ============================================================================
 */

/* OS-specific headers */
#include <host_os.h>
#include <sys/types.h>
#include <fcntl.h>

/* OSAL & Utils headers */
#include <Std.h>
#include <Trace.h>
#include <ti/ipc/MultiProc.h>

/* Module level headers */
#include <ProcDEH.h>

#if defined (__cplusplus)
extern "C" {
#endif

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define PROC_DEH_SYSM3_DRIVER_NAME  "/dev/omap-devh1"
#define PROC_DEH_APPM3_DRIVER_NAME  "/dev/omap-devh2"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for ProcDEH in this process.
 */
static Int32    ProcDEH_SysM3Handle = -1;
static Int32    ProcDEH_AppM3Handle = -1;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32   ProcDEH_SysM3RefCount = 0;
static UInt32   ProcDEH_AppM3RefCount = 0;
static sem_t    semRefCount;


static Void start(Void) __attribute__((constructor));
Void start (Void)
{
    sem_init (&semRefCount, 0, 1);
}


/* Function to close the ProcDEH driver */
Int
ProcDEH_close (UInt16 procId)
{
    Int                 status      = ProcDEH_S_SUCCESS;
    Int                 osStatus    = 0;

    GT_1trace (curTrace, GT_ENTER, "ProcDEH_close", procId);

    sem_wait (&semRefCount);
    if (procId == MultiProc_getId ("SysM3") && --ProcDEH_SysM3RefCount) {
        sem_post (&semRefCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcDEH_close", status);
        return status;
    }

    if (procId == MultiProc_getId ("AppM3") && --ProcDEH_AppM3RefCount) {
        sem_post (&semRefCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcDEH_close", status);
        return status;
    }

    if (procId == MultiProc_getId ("SysM3")) {
        osStatus = close (ProcDEH_SysM3Handle);
        if (osStatus != 0) {
            status = ProcDEH_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcDEH_close",
                                 status,
                                 "Failed to close ProcDEH driver with OS!");
        }
        else {
            ProcDEH_SysM3Handle = -1;
        }
    }
    else if (procId == MultiProc_getId ("AppM3")) {
        osStatus = close (ProcDEH_AppM3Handle);
        if (osStatus != 0) {
            status = ProcDEH_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcDEH_close",
                                 status,
                                 "Failed to close ProcDEH driver with OS!");
        }
        else {
            ProcDEH_AppM3Handle = -1;
        }
    }
    else {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcDEH_close",
                             status,
                             "Devh device not supported!");
        status = ProcDEH_E_FAIL;
    }
    sem_post (&semRefCount);

    GT_1trace (curTrace, GT_LEAVE, "ProcDEH_close", status);

    return status;
}


/* Function to open the ProcDEH driver */
Int
ProcDEH_open (UInt16 procId)
{
    Int32               status          = ProcDEH_S_SUCCESS;
    Int32               osStatus        = 0;

    GT_1trace (curTrace, GT_ENTER, "ProcDEH_open", procId);

    sem_wait (&semRefCount);
    if (procId == MultiProc_getId ("SysM3") && ProcDEH_SysM3RefCount++) {
        sem_post (&semRefCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcDEH_open", status);
        return status;
    }

    if (procId == MultiProc_getId ("AppM3") && ProcDEH_AppM3RefCount++) {
        sem_post (&semRefCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcDEH_open", status);
        return status;
    }

    if (procId == MultiProc_getId ("SysM3")) {
        ProcDEH_SysM3Handle = open (PROC_DEH_SYSM3_DRIVER_NAME,
                                    O_SYNC | O_RDONLY);
        if (ProcDEH_SysM3Handle < 0) {
            status = ProcDEH_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcDEH_open",
                                 status,
                                 "Failed to open ProcDEH driver with OS!");
        }
        else {
            osStatus = fcntl (ProcDEH_SysM3Handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                status = ProcDEH_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcDEH_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
        }
    }
    else if (procId == MultiProc_getId ("AppM3")) {
        ProcDEH_AppM3Handle = open (PROC_DEH_APPM3_DRIVER_NAME,
                                    O_SYNC | O_RDONLY);
        if (ProcDEH_AppM3Handle < 0) {
            status = ProcDEH_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcDEH_open",
                                 status,
                                 "Failed to open ProcDEH driver with OS!");
        }
        else {
            osStatus = fcntl (ProcDEH_AppM3Handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                status = ProcDEH_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcDEH_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
        }
    }
    else {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcDEH_open",
                             status,
                             "Devh device not supported!");
        status = ProcDEH_E_FAIL;
    }
    sem_post (&semRefCount);

    GT_1trace (curTrace, GT_LEAVE, "ProcDEH_open", status);

    return status;
}


/* Function to Register for DEH faults */
Int32 ProcDEH_registerEvent (UInt16     procId,
                             UInt32     eventType,
                             Int32      eventfd,
                             Bool       reg)
{
    Int32                           status      = ProcDEH_S_SUCCESS;
    Int                             osStatus    = 0;
    UInt32                          cmd;
    ProcDEH_CmdArgsRegisterEvent    args;

    GT_4trace (curTrace, GT_ENTER, "ProcDEH_registerEvent", procId, eventType,
                eventfd, reg);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if ((procId == MultiProc_self ()) || (procId == MultiProc_getId ("Tesla"))) {
        status = ProcDEH_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcDEH_open",
                             status,
                             "Devh device not supported!");
    }
    else if (eventType != ProcDEH_SYSERROR &&
                eventType != ProcDEH_WATCHDOGERROR) {
        status = ProcDEH_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcDEH_open",
                             status,
                             "Invalid Device Error type passed!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmd = (reg ? CMD_DEVH_IOCEVENTREG : CMD_DEVH_IOCEVENTUNREG);
        args.fd = eventfd;
        args.eventType = eventType;
        if (procId == MultiProc_getId ("SysM3")) {
            osStatus = ioctl (ProcDEH_SysM3Handle, cmd, &args);
        }
        else if (procId == MultiProc_getId ("AppM3")) {
            osStatus = ioctl (ProcDEH_AppM3Handle, cmd, &args);
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (osStatus < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcDEH_registerEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
            status = ProcDEH_E_OSFAILURE;
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcDEH_registerEvent", status);

    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
