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
 *  @file   MessageQDrv.c
 *
 *  @brief  OS-specific implementation of MessageQ driver for Linux
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>

/* Module specific header files */
#include <ti/ipc/MessageQ.h>
#include <MessageQDrvDefs.h>

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
 *  @brief  Driver name for MessageQ.
 */
#define MESSAGEQ_DRIVER_NAME     "/dev/syslinkipc/MessageQ"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for MessageQ in this process.
 */
static Int32 MessageQDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 MessageQDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the MessageQ driver.
 *
 *  @sa     MessageQDrv_close
 */
Int
MessageQDrv_open (Void)
{
    Int status      = MessageQ_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "MessageQDrv_open");

    if (MessageQDrv_refCount == 0) {
        MessageQDrv_handle = open (MESSAGEQ_DRIVER_NAME,
                                  O_SYNC | O_RDWR);
        if (MessageQDrv_handle < 0) {
            perror (MESSAGEQ_DRIVER_NAME);
            /*! @retval MessageQ_E_OSFAILURE Failed to open
             *          MessageQ driver with OS
             */
            status = MessageQ_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQDrv_open",
                                 status,
                                 "Failed to open MessageQ driver with OS!");
        }
        else {
            osStatus = fcntl (MessageQDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval MessageQ_E_OSFAILURE
                 *          Failed to set file descriptor flags
                 */
                status = MessageQ_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MessageQDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else{
                /* TBD: Protection for refCount. */
                MessageQDrv_refCount++;
            }
        }
    }
    else {
        MessageQDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQDrv_open", status);

    /*! @retval MessageQ_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the MessageQ driver.
 *
 *  @sa     MessageQDrv_open
 */
Int
MessageQDrv_close (Void)
{
    Int status      = MessageQ_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "MessageQDrv_close");

    /* TBD: Protection for refCount. */
    MessageQDrv_refCount--;
    if (MessageQDrv_refCount == 0) {
        osStatus = close (MessageQDrv_handle);
        if (osStatus != 0) {
            perror ("MessageQ driver close: ");
            /*! @retval MessageQ_E_OSFAILURE
             *          Failed to open MessageQ driver with OS
             */
            status = MessageQ_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQDrv_close",
                                 status,
                                 "Failed to close MessageQ driver with OS!");
        }
        else {
            MessageQDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQDrv_close", status);

    /*! @retval MessageQ_S_SUCCESS Operation successfully completed. */
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
MessageQDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = MessageQ_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "MessageQDrv_ioctl", cmd, args);

    GT_assert (curTrace, (MessageQDrv_refCount > 0));

    do {
        osStatus = ioctl (MessageQDrv_handle, cmd, args);
    } while( (osStatus < 0) && (errno == EINTR) );

    if (osStatus < 0) {
        /*! @retval MessageQ_E_OSFAILURE Driver ioctl failed */
        status = MessageQ_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((MessageQDrv_CmdArgs *) args)->apiStatus;
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQDrv_ioctl", status);

    /*! @retval MessageQ_S_SUCCESS Operation successfully completed. */
    return status;
}



#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
