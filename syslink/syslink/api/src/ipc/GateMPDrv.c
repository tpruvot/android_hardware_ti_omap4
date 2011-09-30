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
/*============================================================================
 *  @file   GateMPDrv.c
 *
 *  @brief  OS-specific implementation of GateMP driver for Linux
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <Gate.h>

/* Module specific header files */
#include <ti/ipc/GateMP.h>
#include <GateMPDrvDefs.h>

/* Linux specific header files */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <IPCManager.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Driver name for GateMP.
 */
#define GATEMP_DRIVER_NAME     "/dev/syslinkipc/GateMP"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for GateMP in this process.
 */
static Int32 GateMPDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 GateMPDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the GateMP driver.
 *
 *  @sa     GateMPDrv_close
 */
Int
GateMPDrv_open (Void)
{
    Int status      = GateMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "GateMPDrv_open");

    if (GateMPDrv_refCount == 0) {
        GateMPDrv_handle = open (GATEMP_DRIVER_NAME,
                                 O_SYNC | O_RDWR);
        if (GateMPDrv_handle < 0) {
            perror (GATEMP_DRIVER_NAME);
            /*! @retval GateMP_E_OSFAILURE Failed to open GateMP
                            driver with OS */
            status = GateMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "GateMPDrv_open",
                                 status,
                                 "Failed to open GateMP driver with OS!");
        }
        else {
            osStatus = fcntl (GateMPDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval GateMP_E_OSFAILURE Failed to set file
                     descriptor flags */
                status = GateMP_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "GateMPDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else {
                /* TBD: Protection for refCount. */
                GateMPDrv_refCount++;
            }
        }
    }
    else {
        GateMPDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "GateMPDrv_open", status);

    /*! @retval GateMP_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the GateMP driver.
 *
 *  @sa     GateMPDrv_open
 */
Int
GateMPDrv_close (Void)
{
    Int status      = GateMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "GateMPDrv_close");

    /* TBD: Protection for refCount. */
    GateMPDrv_refCount--;
    if (GateMPDrv_refCount == 0) {
        osStatus = close (GateMPDrv_handle);
        if (osStatus != 0) {
            perror ("GateMP driver close: ");
            status = GateMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "GateMPDrv_close",
                                 status,
                                 "Failed to close GateMP driver with OS!");
        }
        else {
            GateMPDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "GateMPDrv_close", status);

    return status;
}


/*!
 *  @brief  Function to invoke the APIs through ioctl.
 *
 *  @param  cmd     Command for driver ioctl
 *  @param  args    Arguments for the ioctl command
 *
 *  @sa
 */
Int
GateMPDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = GateMP_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "GateMPDrv_ioctl", cmd, args);

    GT_assert (curTrace, (GateMPDrv_refCount > 0));

    osStatus = ioctl (GateMPDrv_handle, cmd, args);
    if (osStatus < 0) {
        status = GateMP_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMPDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((GateMPDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "GateMPDrv_ioctl", status);

    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
