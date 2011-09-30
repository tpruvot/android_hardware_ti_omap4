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
 *  @file   IpcDrv.c
 *
 *  @brief      OS-specific implementation of Ipc driver for Linux
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <Gate.h>

/* Module specific header files */
#include <ti/ipc/Ipc.h>
#include <IpcDrvDefs.h>

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
 *  @brief  Driver name for Ipc.
 */
#define IPC_DRIVER_NAME     "/dev/syslinkipc/Ipc"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for Ipc in this process.
 */
static Int32 IpcDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 IpcDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the Ipc driver.
 *
 *  @sa     IpcDrv_close
 */
Int
IpcDrv_open (Void)
{
    Int status      = Ipc_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "IpcDrv_open");

    if (IpcDrv_refCount == 0) {

        IpcDrv_handle = open (IPC_DRIVER_NAME,
                                       O_SYNC | O_RDWR);
        if (IpcDrv_handle < 0) {
            perror (IPC_DRIVER_NAME);
            /*! @retval Ipc_E_OSFAILURE Failed to open Ipc driver with OS
             */
            status = Ipc_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "IpcDrv_open",
                                 status,
                                 "Failed to open Ipc driver with OS!");
        }
        else {
            osStatus = fcntl (IpcDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval Ipc_E_OSFAILURE Failed to set file descriptor flags
                 */
                status = Ipc_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "IpcDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else {
                /* TBD: Protection for refCount. */
                IpcDrv_refCount++;
            }
        }
    }
    else {
        IpcDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "IpcDrv_open", status);

    /*! @retval Ipc_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the Ipc driver.
 *
 *  @sa     IpcDrv_open
 */
Int
IpcDrv_close (Void)
{
    Int status      = Ipc_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "IpcDrv_close");

    /* TBD: Protection for refCount. */
    IpcDrv_refCount--;
    if (IpcDrv_refCount == 0) {
        osStatus = close (IpcDrv_handle);
        if (osStatus != 0) {
            perror ("Ipc driver close: ");
            /*! @retval Ipc_E_OSFAILURE Failed to open Ipc driver with OS */
            status = Ipc_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "IpcDrv_close",
                                 status,
                                 "Failed to close Ipc driver with OS!");
        }
        else {
            IpcDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "IpcDrv_close", status);

    /*! @retval Ipc_S_SUCCESS Operation successfully completed. */
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
IpcDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = Ipc_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "IpcDrv_ioctl", cmd, args);

    GT_assert (curTrace, (IpcDrv_refCount > 0));

    osStatus = ioctl (IpcDrv_handle, cmd, args);
    if (osStatus < 0) {
        /*! @retval Ipc_E_OSFAILURE Driver ioctl failed */
        status = Ipc_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "IpcDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((IpcDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "IpcDrv_ioctl", status);

    /*! @retval Ipc_S_SUCCESS Operation successfully completed. */
    return status;
}



#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
