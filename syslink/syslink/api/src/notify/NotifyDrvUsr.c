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
 *  @file       NotifyDrvUsr.c
 *
 *  @brief      User-side OS-specific implementation of Notify driver for Linux
 *
 *  ============================================================================
 */


/* Linux specific header files */
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <NotifyDrvUsr.h>

/* Module headers */
#include <NotifyDrvDefs.h>
#include <IPCManager.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Driver name for Notify.
 */
#define NOTIFY_DRIVER_NAME         "/dev/syslinkipc/Notify"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for Notify in this process.
 */
static Int32 NotifyDrvUsr_handle = -1;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 NotifyDrvUsr_refCount = 0;

/*!
 *  @brief  Thread handler for event receiver thread.
 */
static pthread_t  NotifyDrv_workerThread;


/** ============================================================================
 *  Forward declaration of internal functions
 *  ============================================================================
 */
/*!
 *  @brief      This is the worker thread for polling on events.
 *
 *  @param      arg module attributes
 *
 *  @sa
 */
Void _NotifyDrvUsr_eventWorker (Void * arg);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the Notify driver.
 *
 *  @param  createThread  Flag to indicate whether to create thread or not.
 *
 *  @sa     NotifyDrvUsr_close
 */
Int
NotifyDrvUsr_open (Bool createThread)
{
    Int    status   = Notify_S_SUCCESS;
    int    osStatus = 0;
    UInt32 pid;

    GT_1trace (curTrace, GT_ENTER, "NotifyDrvUsr_open", createThread);

    if (NotifyDrvUsr_refCount == 0) {
        /* TBD: Protection for refCount. */
        NotifyDrvUsr_refCount++;

        NotifyDrvUsr_handle = open (NOTIFY_DRIVER_NAME, O_SYNC | O_RDWR);
        if (NotifyDrvUsr_handle < 0) {
            perror ("Notify driver open: " NOTIFY_DRIVER_NAME);
            /*! @retval Notify_E_OSFAILURE Failed to open Notify driver with
                        OS */
            status = Notify_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NotifyDrvUsr_open",
                                 status,
                                 "Failed to open Notify driver with OS!");
        }
        else {
            osStatus = fcntl (NotifyDrvUsr_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval Notify_E_OSFAILURE Failed to set file descriptor
                                                flags */
                status = Notify_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NotifyDrvUsr_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else {
                if (createThread == TRUE) {
                    pid = getpid ();
                    status = NotifyDrvUsr_ioctl (CMD_NOTIFY_THREADATTACH, &pid);
                    if (status < 0) {
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "NotifyDrvUsr_close",
                                             status,
                                             "Notify attach failed on kernel "
                                             "side!");
                    }
                    else {
                        /* Create the pthread */
                        pthread_create (&NotifyDrv_workerThread,
                                        NULL,
                                        (Ptr) _NotifyDrvUsr_eventWorker,
                                        NULL);
                        if (NotifyDrv_workerThread == (UInt32) NULL) {
                            /*! @retval Notify_E_OSFAILURE Failed to create
                                                           Notify thread */
                            status = Notify_E_OSFAILURE;
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "NotifyDrvUsr_open",
                                                 status,
                                                 "Failed to create Notify "
                                                 "thread!");
                        }
                    }
                }
            }
        }
    }
    else {
        /* TBD: Protection for refCount. */
        NotifyDrvUsr_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "NotifyDrvUsr_open", status);

    /*! @retval Notify_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the Notify driver.
 *
 *  @param  deleteThread  Flag to indicate whether to delete thread or not.
 *
 *  @sa     NotifyDrvUsr_open
 */
Int
NotifyDrvUsr_close (Bool deleteThread)
{
    Int    status      = Notify_S_SUCCESS;
    int    osStatus    = 0;
    UInt32 pid;

    GT_1trace (curTrace, GT_ENTER, "NotifyDrvUsr_close", deleteThread);

    /* TBD: Protection for refCount. */
    if (NotifyDrvUsr_refCount == 1) {
        if (deleteThread == TRUE) {
            pid = getpid ();
            status = NotifyDrvUsr_ioctl (CMD_NOTIFY_THREADDETACH, &pid);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NotifyDrvUsr_close",
                                     status,
                                     "Notify detach failed on kernel side!");
            }

            pthread_join (NotifyDrv_workerThread, NULL);
        }
        NotifyDrvUsr_refCount--;

        osStatus = close (NotifyDrvUsr_handle);
        if (osStatus != 0) {
            perror ("Notify driver close: " NOTIFY_DRIVER_NAME);
            /*! @retval Notify_E_OSFAILURE Failed to open Notify driver with
                                            OS */
            status = Notify_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NotifyDrvUsr_close",
                                 status,
                                 "Failed to close Notify driver with OS!");
        }
        else {
            NotifyDrvUsr_handle = -1;
        }
    }
    else {
        NotifyDrvUsr_refCount--;
    }

    GT_1trace (curTrace, GT_LEAVE, "NotifyDrvUsr_close", status);

    /*! @retval Notify_S_SUCCESS Operation successfully completed. */
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
NotifyDrvUsr_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = Notify_S_SUCCESS;
    int osStatus    = 0;

    GT_2trace (curTrace, GT_ENTER, "NotifyDrvUsr_ioctl", cmd, args);

    GT_assert (curTrace, (NotifyDrvUsr_refCount > 0));

    osStatus = ioctl (NotifyDrvUsr_handle, cmd, args);
    if (osStatus < 0) {
        /*! @retval Notify_E_OSFAILURE Driver ioctl failed */
        status = Notify_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NotifyDrvUsr_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((Notify_CmdArgs *) args)->apiStatus;
    }
    GT_1trace (curTrace, GT_LEAVE, "NotifyDrvUsr_ioctl", status);

    /*! @retval Notify_S_SUCCESS Operation successfully completed. */
    return status;
}


/** ============================================================================
 *  Internal functions
 *  ============================================================================
 */
/*!
 *  @brief      This is the worker thread which polls for events.
 *
 *  @param      attrs module attributes
 *
 *  @sa
 */
Void
_NotifyDrvUsr_eventWorker (Void * arg)
{
    Int32                 status = Notify_S_SUCCESS;
    UInt32                nRead  = 0;
    NotifyDrv_EventPacket packet;
    sigset_t              blockSet;

    GT_1trace (curTrace, GT_ENTER, "_NotifyDrvUsr_eventWorker", arg);

#ifndef HAVE_ANDROID_OS
    if (sigfillset (&blockSet) != 0) {
        perror ("Event worker thread error in sigfillset");
        return;
        
    }

    if (pthread_sigmask (SIG_BLOCK, &blockSet, NULL) != 0) {
        perror ("Event worker thread error in setting sigmask");
        return;
    }
#endif /* #ifndef HAVE_ANDROID_OS */

    while (status >= 0) {
        memset (&packet, 0, sizeof (NotifyDrv_EventPacket));
        packet.pid = getpid ();
        nRead = read (NotifyDrvUsr_handle,
                      &packet,
                      sizeof (NotifyDrv_EventPacket));
        if (0 == nRead || nRead == sizeof (NotifyDrv_EventPacket)) {
            /* check for termination packet */
            if (packet.isExit  == TRUE) {
                return;
            }

            if (packet.func != NULL) {
                packet.func (packet.procId,
                             packet.lineId,
                             packet.eventId,
                             packet.param,
                             packet.data);
            }
        }
    }

    GT_0trace (curTrace, GT_LEAVE, "_NotifyDrvUsr_eventWorker");
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
