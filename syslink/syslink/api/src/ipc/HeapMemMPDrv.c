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
 *  @file   HeapMemMPDrv.c
 *
 *  @brief      OS-specific implementation of HeapMemMP driver for Linux
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>

/* Module specific header files */
#include <IHeap.h>
#include <_HeapMemMP.h>
#include <ti/ipc/HeapMemMP.h>
#include <HeapMemMPDrvDefs.h>

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
 *  @brief  Driver name for HeapMemMP.
 */
#define HEAPMEM_DRIVER_NAME     "/dev/syslinkipc/HeapMemMP"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for HeapMemMP in this process.
 */
static Int32 HeapMemMPDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 HeapMemMPDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the HeapMemMP driver.
 *
 *  @sa     HeapMemMPDrv_close
 */
Int
HeapMemMPDrv_open (Void)
{
    Int status      = HeapMemMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "HeapMemMPDrv_open");

    if (HeapMemMPDrv_refCount == 0) {

        HeapMemMPDrv_handle = open (HEAPMEM_DRIVER_NAME,
                                  O_SYNC | O_RDWR);
        if (HeapMemMPDrv_handle < 0) {
            perror (HEAPMEM_DRIVER_NAME);
            /*! @retval HeapMemMP_E_OSFAILURE Failed to open
             *          HeapMemMP driver with OS
             */
            status = HeapMemMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMPDrv_open",
                                 status,
                                 "Failed to open HeapMemMP driver with OS!");
        }
        else {
            osStatus = fcntl (HeapMemMPDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval HeapMemMP_E_OSFAILURE
                 *          Failed to set file descriptor flags
                 */
                status = HeapMemMP_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMPDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else {
                /* TBD: Protection for refCount. */
                HeapMemMPDrv_refCount++;
            }
        }
    }
    else {
        HeapMemMPDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMPDrv_open", status);

    /*! @retval HeapMemMP_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the HeapMemMP driver.
 *
 *  @sa     HeapMemMPDrv_open
 */
Int
HeapMemMPDrv_close (Void)
{
    Int status      = HeapMemMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "HeapMemMPDrv_close");

    /* TBD: Protection for refCount. */
    HeapMemMPDrv_refCount--;
    if (HeapMemMPDrv_refCount == 0) {
        osStatus = close (HeapMemMPDrv_handle);
        if (osStatus != 0) {
            perror ("HeapMemMP driver close: ");
            /*! @retval HeapMemMP_E_OSFAILURE
             *          Failed to open HeapMemMP driver with OS
             */
            status = HeapMemMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMPDrv_close",
                                 status,
                                 "Failed to close HeapMemMP driver with OS!");
        }
        else {
            HeapMemMPDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMPDrv_close", status);

    /*! @retval HeapMemMP_S_SUCCESS Operation successfully completed. */
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
HeapMemMPDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = HeapMemMP_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "HeapMemMPDrv_ioctl", cmd, args);

    GT_assert (curTrace, (HeapMemMPDrv_refCount > 0));

    osStatus = ioctl (HeapMemMPDrv_handle, cmd, args);
    if (osStatus < 0) {
        /*! @retval HeapMemMP_E_OSFAILURE Driver ioctl failed */
        status = HeapMemMP_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMPDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((HeapMemMPDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMPDrv_ioctl", status);

    /*! @retval HeapMemMP_S_SUCCESS Operation successfully completed. */
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
