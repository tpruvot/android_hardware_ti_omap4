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
 *  @file   SharedRegionDrv.c
 *
 *  @brief  OS-specific implementation of SharedRegion driver for Linux
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <Memory.h>
#include <ti/ipc/MultiProc.h>

/* Module specific header files */
#include <Gate.h>
#include <ti/ipc/SharedRegion.h>
#include <_SharedRegion.h>
#include <SharedRegionDrvDefs.h>

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
 *  @brief  Driver name for SharedRegion.
 */
#define SHAREDREGION_DRIVER_NAME     "/dev/syslinkipc/SharedRegion"


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for SharedRegion in this process.
 */
static Int32 SharedRegionDrv_handle = 0;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 SharedRegionDrv_refCount = 0;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the SharedRegion driver.
 *
 *  @sa     SharedRegionDrv_close
 */
Int
SharedRegionDrv_open (Void)
{
    Int status      = SharedRegion_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "SharedRegionDrv_open");

    if (SharedRegionDrv_refCount == 0) {
        SharedRegionDrv_handle = open (SHAREDREGION_DRIVER_NAME,
                                       O_SYNC | O_RDWR);
        if (SharedRegionDrv_handle < 0) {
            perror ("SharedRegion driver open: ");
            status = SharedRegion_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegionDrv_open",
                                 status,
                                 "Failed to open SharedRegion driver with OS!");
        }
        else {
            osStatus = fcntl (SharedRegionDrv_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                status = SharedRegion_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegionDrv_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else{
                /* TBD: Protection for refCount. */
                SharedRegionDrv_refCount++;
            }
        }
    }
    else {
        SharedRegionDrv_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegionDrv_open", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the SharedRegion driver.
 *
 *  @sa     SharedRegionDrv_open
 */
Int
SharedRegionDrv_close (Void)
{
    Int status      = SharedRegion_S_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "SharedRegionDrv_close");

    /* TBD: Protection for refCount. */
    SharedRegionDrv_refCount--;
    if (SharedRegionDrv_refCount == 0) {
        osStatus = close (SharedRegionDrv_handle);
        if (osStatus != 0) {
            perror ("SharedRegion driver close: ");
            status = SharedRegion_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegionDrv_close",
                                 status,
                                 "Failed to close SharedRegion driver with OS!");
        }
        else {
            SharedRegionDrv_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegionDrv_close", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successfully completed. */
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
SharedRegionDrv_ioctl (UInt32 cmd, Ptr args)
{
    Int                       status   = SharedRegion_S_SUCCESS;
    int                       osStatus = 0;
    SharedRegionDrv_CmdArgs * cargs    = (SharedRegionDrv_CmdArgs *) args;
    SharedRegion_Region     * regions  = NULL;
    SharedRegion_Config     * config;
    Memory_MapInfo            mapInfo;
    Memory_UnmapInfo          unmapInfo;
    UInt16                    i;

    GT_2trace (curTrace, GT_ENTER, "SharedRegionDrv_ioctl", cmd, args);

    GT_assert (curTrace, (SharedRegionDrv_refCount > 0));

    do {
        osStatus = ioctl (SharedRegionDrv_handle, cmd, args);
    } while( (osStatus < 0) && (errno == EINTR) );

    if (osStatus < 0) {
        /*! @retval SharedRegion_E_OSFAILURE Driver ioctl failed */
        status = SharedRegion_E_OSFAILURE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegionDrv_ioctl",
                             status,
                             "Driver ioctl failed!");
    }
    else {
        /* First field in the structure is the API status. */
        status = ((SharedRegionDrv_CmdArgs *) args)->apiStatus;

        /* Convert the base address to user virtual address */
        if (cmd == CMD_SHAREDREGION_SETUP) {
            config = cargs->args.setup.config;
            for (i = 0u; (   (i < config->numEntries) && (status >= 0)); i++) {
                regions = &(cargs->args.setup.regions [i]);
                if (regions->entry.isValid == TRUE) {
                    mapInfo.src  = (UInt32) regions->entry.base;
                    mapInfo.size = regions->entry.len;
                    status = Memory_map (&mapInfo);
                    if (status < 0) {
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "SharedRegionDrv_ioctl",
                                             status,
                                             "Memory_map failed!");
                    }
                    else {
                        regions->entry.base = (Ptr) mapInfo.dst;
                    }
                }
            }
        }
        else if (cmd == CMD_SHAREDREGION_DESTROY) {
            config = cargs->args.setup.config;
            for (i = 0u; (   (i < config->numEntries) && (status >= 0)); i++) {
                regions = &(cargs->args.setup.regions [i]);
                if (regions->entry.isValid == TRUE) {
                    unmapInfo.addr  = (UInt32) regions->entry.base;
                    unmapInfo.size = regions->entry.len;
                    status = Memory_unmap (&unmapInfo);
                    if (status < 0) {
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "SharedRegionDrv_ioctl",
                                             status,
                                             "Memory_unmap failed!");
                    }
                }
            }
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegionDrv_ioctl", status);

    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
