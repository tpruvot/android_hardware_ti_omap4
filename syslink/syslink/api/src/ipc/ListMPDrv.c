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
 *  @file   ListMPDrv.c
 *
 *  @brief  OS-specific implementation of ListMP driver
 *          for Linux
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>

/* Module specific header files */
#include <_ListMP.h>
#include <ti/ipc/ListMP.h>
#include <ListMPDrvDefs.h>

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
 *  @brief  Driver name for ListMP.
 */
#define LISTMP_DRIVER_NAME     "/dev/syslinkipc/ListMP"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for ListMP in this process.
 */
static Int32 ListMPDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 ListMPDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the ListMP driver.
 *
 *  @sa     ListMPDrv_close
 */
Int
ListMPDrv_open (Void)
{
    Int status      = ListMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "ListMPDrv_open");

    if (ListMPDrv_refCount == 0) {
        ListMPDrv_handle = open (LISTMP_DRIVER_NAME,
                                 O_SYNC | O_RDWR);
        if (ListMPDrv_handle < 0) {
            perror (LISTMP_DRIVER_NAME);
            /*! @retval ListMP_E_OSFAILURE
             *          Failed to open ListMP driver with OS
             */
            status = ListMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMPDrv_open",
                                 status,
                                 "Failed to open ListMP driver"
                                 " with OS!");
        }
        else {
            osStatus = fcntl (ListMPDrv_handle,
                              F_SETFD,
                              FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval ListMP_E_OSFAILURE
                 *          Failed to set file descriptor flags
                 */
                status = ListMP_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ListMPDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else{
                /* TBD: Protection for refCount. */
                ListMPDrv_refCount++;
            }
        }
    }
    else {
        ListMPDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "ListMPDrv_open", status);

    /*! @retval ListMP_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the ListMP driver.
 *
 *  @sa     ListMPDrv_open
 */
Int
ListMPDrv_close (Void)
{
    Int status      = ListMP_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "ListMPDrv_close");

    /* TBD: Protection for refCount. */
    ListMPDrv_refCount--;
    if (ListMPDrv_refCount == 0) {
        osStatus = close (ListMPDrv_handle);
        if (osStatus != 0) {
            perror ("ListMP driver close: ");
            /*! @retval ListMP_E_OSFAILURE Failed to open
             *          ListMP driver with OS
             */
            status = ListMP_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMPDrv_close",
                                 status,
                                 "Failed to close ListMP driver"
                                 " with OS!");
        }
        else {
            ListMPDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "ListMPDrv_close", status);

    /*! @retval ListMP_S_SUCCESS Operation successfully completed. */
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
ListMPDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = ListMP_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "ListMPDrv_ioctl", cmd, args);

    GT_assert (curTrace, (ListMPDrv_refCount > 0));

    osStatus = ioctl (ListMPDrv_handle, cmd, args);
    if (osStatus < 0) {
    /*! @retval ListMP_E_OSFAILURE Driver ioctl failed */
        status = ListMP_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMPDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((ListMPDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "ListMPDrv_ioctl", status);

    /*! @retval ListMP_S_SUCCESS Operation successfully completed. */
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
