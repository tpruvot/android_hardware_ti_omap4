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
 *  @file   HeapBufMPDrv.c
 *
 *  @brief  OS-specific implementation of HeapBufMP driver for Linux
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <IHeap.h>

/* Module specific header files */
#include <_HeapBufMP.h>
#include <ti/ipc/HeapBufMP.h>
#include <HeapBufMPDrvDefs.h>

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
 *  @brief  Driver name for HeapBufMP.
 */
#define HEAPBUF_DRIVER_NAME     "/dev/syslinkipc/HeapBufMP"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for HeapBufMP in this process.
 */
static Int32 HeapBufMPDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 HeapBufMPDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the HeapBufMP driver.
 *
 *  @sa     HeapBufMPDrv_close
 */
Int
HeapBufMPDrv_open (Void)
{
    Int status      = HeapBufMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "HeapBufMPDrv_open");

    if (HeapBufMPDrv_refCount == 0) {

        HeapBufMPDrv_handle = open (HEAPBUF_DRIVER_NAME,
                                  O_SYNC | O_RDWR);
        if (HeapBufMPDrv_handle < 0) {
            perror (HEAPBUF_DRIVER_NAME);
            /*! @retval HeapBufMP_E_OSFAILURE Failed to open
             *          HeapBufMP driver with OS
             */
            status = HeapBufMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMPDrv_open",
                                 status,
                                 "Failed to open HeapBufMP driver with OS!");
        }
        else {
            osStatus = fcntl (HeapBufMPDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval HeapBufMP_E_OSFAILURE
                 *          Failed to set file descriptor flags
                 */
                status = HeapBufMP_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapBufMPDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else {
                /* TBD: Protection for refCount. */
                HeapBufMPDrv_refCount++;
            }
        }
    }
    else {
        HeapBufMPDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMPDrv_open", status);

    /*! @retval HeapBufMP_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the HeapBufMP driver.
 *
 *  @sa     HeapBufMPDrv_open
 */
Int
HeapBufMPDrv_close (Void)
{
    Int status      = HeapBufMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "HeapBufMPDrv_close");

    /* TBD: Protection for refCount. */
    HeapBufMPDrv_refCount--;
    if (HeapBufMPDrv_refCount == 0) {
        osStatus = close (HeapBufMPDrv_handle);
        if (osStatus != 0) {
            perror ("HeapBufMP driver close: ");
            /*! @retval HeapBufMP_E_OSFAILURE
             *          Failed to open HeapBufMP driver with OS
             */
            status = HeapBufMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMPDrv_close",
                                 status,
                                 "Failed to close HeapBufMP driver with OS!");
        }
        else {
            HeapBufMPDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMPDrv_close", status);

    /*! @retval HeapBufMP_S_SUCCESS Operation successfully completed. */
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
HeapBufMPDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = HeapBufMP_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "HeapBufMPDrv_ioctl", cmd, args);

    GT_assert (curTrace, (HeapBufMPDrv_refCount > 0));

    do {
        osStatus = ioctl (HeapBufMPDrv_handle, cmd, args);
    } while( (osStatus < 0) && (errno == EINTR) );

    if (osStatus < 0) {
        /*! @retval HeapBufMP_E_OSFAILURE Driver ioctl failed */
        status = HeapBufMP_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMPDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((HeapBufMPDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMPDrv_ioctl", status);

    /*! @retval HeapBufMP_S_SUCCESS Operation successfully completed. */
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
