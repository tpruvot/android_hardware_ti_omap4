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
 *  @file   omap4430procDrvUsr.c
 *
 *  @brief      User-side OS-specific implementation of OMAP4430PROC driver for
 *              Linux
 *
 *  ============================================================================
 */


/* Linux specific header files */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <omap4430procDrvUsr.h>

/* Module headers */
#include <omap4430procDrvDefs.h>
#include <omap4430proc.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Driver name for OMAP4430PROC.
 */
#define OMAP4430PROC_DRIVER_NAME         "/dev/syslink-proc4430"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for OMAP4430PROC in this process.
 */
static Int32 OMAP4430PROC_drvHandle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 OMAP4430PROCDrvUsr_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the OMAP4430PROC driver.
 *
 *  @sa     OMAP4430PROCDrvUsr_close
 */
Int
OMAP4430PROCDrvUsr_open (Void)
{
    Int status      = OMAP4430PROC_SUCCESS;
    int osStatus    = 0;

	Osal_printf ("-> OMAP4430PROCDrvUsr_open\n");
    GT_0trace (curTrace, GT_ENTER, "OMAP4430PROCDrvUsr_open");

    if (OMAP4430PROCDrvUsr_refCount == 0) {
        /* TBD: Protection for refCount. */
        OMAP4430PROCDrvUsr_refCount++;

        OMAP4430PROC_drvHandle = open (OMAP4430PROC_DRIVER_NAME, O_SYNC | O_RDWR);
        if (OMAP4430PROC_drvHandle < 0) {
            perror ("OMAP4430PROC driver open: " OMAP4430PROC_DRIVER_NAME);
            /*! @retval OMAP4430PROC_E_OSFAILURE Failed to open OMAP4430PROC
                                                 driver with OS */
            status = OMAP4430PROC_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROCDrvUsr_open",
                                 status,
                                 "Failed to open OMAP4430PROC driver with OS!");
        }
        else {
            osStatus = fcntl (OMAP4430PROC_drvHandle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval OMAP4430PROC_E_OSFAILURE Failed to set file
                                                     descriptor flags */
                status = OMAP4430PROC_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "OMAP4430PROCDrvUsr_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
        }
    }
    else {
        OMAP4430PROCDrvUsr_refCount++;
    }

	Osal_printf ("<- OMAP4430PROCDrvUsr_open\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROCDrvUsr_open", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the OMAP4430PROC driver.
 *
 *  @sa     OMAP4430PROCDrvUsr_open
 */
Int
OMAP4430PROCDrvUsr_close (Void)
{
    Int status      = OMAP4430PROC_SUCCESS;
    int osStatus    = 0;

	Osal_printf ("-> OMAP4430PROCDrvUsr_close\n");
    GT_0trace (curTrace, GT_ENTER, "OMAP4430PROCDrvUsr_close");

    /* TBD: Protection for refCount. */
    OMAP4430PROCDrvUsr_refCount--;
    if (OMAP4430PROCDrvUsr_refCount == 0) {
        osStatus = close (OMAP4430PROC_drvHandle);
        if (osStatus != 0) {
            perror ("OMAP4430PROC driver close: " OMAP4430PROC_DRIVER_NAME);
            /*! @retval OMAP4430PROC_E_OSFAILURE Failed to open OMAP4430PROC
                                                 driver with OS */
            status = OMAP4430PROC_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                GT_4CLASS,
                                "OMAP4430PROCDrvUsr_close",
                                status,
                                "Failed to close OMAP4430PROC driver with OS!");
        }
        else {
            OMAP4430PROC_drvHandle = 0;
        }
    }

	Osal_printf ("<- OMAP4430PROCDrvUsr_close\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROCDrvUsr_close", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successfully completed. */
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
OMAP4430PROCDrvUsr_ioctl (UInt32 cmd, Ptr args)
{
    Int status      = OMAP4430PROC_SUCCESS;
    int osStatus    = 0;

	Osal_printf ("-> OMAP4430PROCDrvUsr_ioctl\n");
    GT_2trace (curTrace, GT_ENTER, "OMAP4430PROCDrvUsr_ioctl", cmd, args);

    osStatus = ioctl (OMAP4430PROC_drvHandle, cmd, args);
    if (osStatus < 0) {
        /*! @retval OMAP4430PROC_E_OSFAILURE Driver ioctl failed */
        status = OMAP4430PROC_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROCDrvUsr_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((ProcMgr_CmdArgs *) args)->apiStatus;
    }

	Osal_printf ("<- OMAP4430PROCDrvUsr_ioctl\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROCDrvUsr_ioctl", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successfully completed. */
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
