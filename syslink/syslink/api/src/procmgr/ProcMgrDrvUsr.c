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
 *  @file   ProcMgrDrvUsr.c
 *
 *  @brief      User-side OS-specific implementation of ProcMgr driver for Linux
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
#include <Memory.h>
#include <ProcMgrDrvUsr.h>

/* Module headers */
#include <ProcMgrDrvDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Driver name for ProcMgr.
 */
#define PROCMGR_DRIVER_NAME         "/dev/syslink-procmgr"

#define PROCTESLA_DRIVER_NAME         "/dev/omap-rproc0"
#define PROCSYSM3_DRIVER_NAME         "/dev/omap-rproc1"
#define PROCAPPM3_DRIVER_NAME         "/dev/omap-rproc2"

/* TEMPORARY DEFINES, REPLACE WITH THE MULTIPROC FRAMEWORK */
#define TEMP_PROC_TESLA_ID               0
#define TEMP_PROC_APPM3_ID               1
#define TEMP_PROC_SYSM3_ID               2
/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for ProcMgr in this process.
 */
static Int32 ProcMgrDrvUsr_handle = -1;

static Int32 ProcDrvTesla_handle = -1;

static Int32 ProcDrvSysM3_handle = -1;

static Int32 ProcDrvAppM3_handle = -1;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 ProcMgrDrvUsr_refCount = 0;


/** ============================================================================
 *  Forward declarations of internal functions
 *  ============================================================================
 */
/* Function to map the processor's memory regions to user space. */
static Int _ProcMgrDrvUsr_mapMemoryRegion (ProcMgr_ProcInfo * procInfo);

/* Function to unmap the processor's memory regions to user space. */
static Int _ProcMgrDrvUsr_unmapMemoryRegion (ProcMgr_ProcInfo * procInfo);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to open the ProcMgr driver.
 *
 *  @sa     ProcMgrDrvUsr_close
 */
Int
ProcMgrDrvUsr_open (Void)
{
    Int status      = PROCMGR_SUCCESS;
    int osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "ProcMgrDrvUsr_open");

    if (ProcMgrDrvUsr_refCount == 0) {
        ProcMgrDrvUsr_handle = open (PROCMGR_DRIVER_NAME, O_SYNC | O_RDWR);
        if (ProcMgrDrvUsr_handle < 0)
            perror ("ProcMgr driver open: " PROCMGR_DRIVER_NAME);

        ProcDrvTesla_handle = open (PROCTESLA_DRIVER_NAME, O_RDONLY);
        if (ProcDrvTesla_handle < 0)
            perror ("ProcMgr driver open: " PROCTESLA_DRIVER_NAME);

        ProcDrvSysM3_handle = open (PROCSYSM3_DRIVER_NAME, O_RDONLY);
        if (ProcDrvSysM3_handle < 0)
            perror ("ProcMgr driver open: " PROCSYSM3_DRIVER_NAME);

        ProcDrvAppM3_handle = open (PROCAPPM3_DRIVER_NAME, O_RDONLY);
        if (ProcDrvAppM3_handle < 0)
            perror ("ProcMgr driver open: " PROCAPPM3_DRIVER_NAME);

        if (ProcMgrDrvUsr_handle < 0 || ProcDrvSysM3_handle < 0
                   || ProcDrvAppM3_handle < 0 || ProcDrvTesla_handle < 0) {
            /*! @retval PROCMGR_E_OSFAILURE Failed to open ProcMgr driver with
                        OS */
            status = PROCMGR_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgrDrvUsr_open",
                                 status,
                                 "Failed to open ProcMgr driver with OS!");
        }
        else {
            osStatus = fcntl (ProcMgrDrvUsr_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval PROCMGR_E_OSFAILURE Failed to set file descriptor
                                                flags */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
            else{
                /* TBD: Protection for refCount. */
                ProcMgrDrvUsr_refCount++;
            }
        }
    }
    else {
        ProcMgrDrvUsr_refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "ProcMgrDrvUsr_open", status);

    /*! @retval PROCMGR_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to close the ProcMgr driver.
 *
 *  @sa     ProcMgrDrvUsr_open
 */
Int
ProcMgrDrvUsr_close (Void)
{
    Int status      = PROCMGR_SUCCESS;
    int errCount = 0;

    GT_0trace (curTrace, GT_ENTER, "ProcMgrDrvUsr_close");

    /* TBD: Protection for refCount. */
    ProcMgrDrvUsr_refCount--;
    if (ProcMgrDrvUsr_refCount == 0) {
        if (close (ProcMgrDrvUsr_handle)) {
            perror ("ProcMgr driver close: " PROCMGR_DRIVER_NAME);
            errCount++;
        }
        if (close (ProcDrvSysM3_handle)) {
            perror ("ProcMgr driver close: " PROCSYSM3_DRIVER_NAME);
            errCount++;
        }
        if (close (ProcDrvAppM3_handle)) {
            perror ("ProcMgr driver close: " PROCAPPM3_DRIVER_NAME);
            errCount++;
        }
        if (close (ProcDrvTesla_handle)) {
            perror ("ProcMgr driver close: " PROCTESLA_DRIVER_NAME);
            errCount++;
        }
        if (errCount != 0) {
            /*! @retval PROCMGR_E_OSFAILURE Failed to open ProcMgr driver
                                            with OS */
            status = PROCMGR_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgrDrvUsr_close",
                                 status,
                                 "Failed to close ProcMgr driver with OS!");
        }
        else {
            ProcMgrDrvUsr_handle = 0;
            ProcDrvSysM3_handle = 0;
            ProcDrvAppM3_handle = 0;
            ProcDrvTesla_handle = 0;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "ProcMgrDrvUsr_close", status);

    /*! @retval PROCMGR_SUCCESS Operation successfully completed. */
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
ProcMgrDrvUsr_ioctl (UInt32 cmd, Ptr args)
{
    Int     status          = PROCMGR_SUCCESS;
    Int     tmpStatus       = PROCMGR_SUCCESS;
    int     osStatus        = 0;
    Bool    driverOpened    = FALSE;

    GT_2trace (curTrace, GT_ENTER, "ProcMgrDrvUsr_ioctl", cmd, args);

    if (ProcMgrDrvUsr_handle < 0) {
        /* Need to open the driver handle. It was not opened from this process.
         */
        driverOpened = TRUE;
        status = ProcMgrDrvUsr_open ();
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgrDrvUsr_ioctl",
                                 status,
                                 "Failed to open OS driver handle!");
        }
    }

    GT_assert (curTrace, (ProcMgrDrvUsr_refCount > 0));

    switch (cmd) {
        case CMD_PROCMGR_ATTACH:
        {
            ProcMgr_CmdArgsAttach * srcArgs = (ProcMgr_CmdArgsAttach *) args;

            osStatus = ioctl (ProcMgrDrvUsr_handle, cmd, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
            else {
                status = _ProcMgrDrvUsr_mapMemoryRegion (&(srcArgs->procInfo));
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "ProcMgrDrvUsr_ioctl",
                                         status,
                                         "Failed to map memory regions!");
                }
            }
        }
        break;

        case CMD_PROCMGR_OPEN:
        {
            ProcMgr_CmdArgsOpen * srcArgs = (ProcMgr_CmdArgsOpen *) args;

            osStatus = ioctl (ProcMgrDrvUsr_handle, cmd, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
            else {
                /* If the driver was not opened earlier in this process, need to
                 * map the memory region. This can be done only if the Proc has
                 * been attached-to already, which will create the mappings on
                 * kernel-side.
                 */
                if (srcArgs->procInfo.numMemEntries != 0) {
                    status = _ProcMgrDrvUsr_mapMemoryRegion (
                                                        &(srcArgs->procInfo));
                    if (status < 0) {
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "ProcMgrDrvUsr_ioctl",
                                             status,
                                             "Failed to map memory regions!");
                    }
                }
            }
        }
        break;

        case CMD_PROCMGR_DETACH:
        {
            ProcMgr_CmdArgsDetach * srcArgs = (ProcMgr_CmdArgsDetach *) args;

            osStatus = ioctl (ProcMgrDrvUsr_handle, cmd, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
            else {
                status = _ProcMgrDrvUsr_unmapMemoryRegion(&(srcArgs->procInfo));
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "ProcMgrDrvUsr_ioctl",
                                         status,
                                         "Failed to unmap memory regions!");
                }
            }
        }
        break;

        case CMD_PROCMGR_CLOSE:
        {
            ProcMgr_CmdArgsClose * srcArgs = (ProcMgr_CmdArgsClose *) args;

            osStatus = ioctl (ProcMgrDrvUsr_handle, cmd, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
            else {
                status = _ProcMgrDrvUsr_unmapMemoryRegion(&(srcArgs->procInfo));
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "ProcMgrDrvUsr_ioctl",
                                         status,
                                         "Failed to unmap memory regions!");
                }
            }
        }
        break;

        case CMD_PROCMGR_START:
        {
            ProcMgr_CmdArgsStart * srcArgs = (ProcMgr_CmdArgsStart *) args;

            Osal_printf("%s %d proc id = %d\n", __func__, __LINE__,
                                            srcArgs->params->proc_id);
            if (srcArgs->params->proc_id == TEMP_PROC_SYSM3_ID)
                osStatus = ioctl (ProcDrvSysM3_handle, RPROC_IOCSTART, NULL);
            else if (srcArgs->params->proc_id == TEMP_PROC_APPM3_ID)
                osStatus = ioctl (ProcDrvAppM3_handle, RPROC_IOCSTART, NULL);
            else
                osStatus = ioctl (ProcDrvTesla_handle, RPROC_IOCSTART, NULL);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }


        }
        break;

        case CMD_PROCMGR_STOP:
        {
            ProcMgr_CmdArgsStop* srcArgs = (ProcMgr_CmdArgsStop *) args;

            if (srcArgs->params->proc_id == TEMP_PROC_SYSM3_ID)
                osStatus = ioctl (ProcDrvSysM3_handle, RPROC_IOCSTOP, NULL);
            else if (srcArgs->params->proc_id == TEMP_PROC_APPM3_ID)
                osStatus = ioctl (ProcDrvAppM3_handle, RPROC_IOCSTOP, NULL);
            else
                osStatus = ioctl (ProcDrvTesla_handle, RPROC_IOCSTOP, NULL);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }

        }
        break;

        case CMD_PROCMGR_REGEVENT:
        {
            ProcMgr_CmdArgsRegEvent *srcArgs = (ProcMgr_CmdArgsRegEvent *) args;

            if (srcArgs->procId == TEMP_PROC_SYSM3_ID)
                osStatus = ioctl (ProcDrvSysM3_handle, RPROC_IOCREGEVENT, args);
            else if (srcArgs->procId == TEMP_PROC_APPM3_ID)
                osStatus = ioctl (ProcDrvAppM3_handle, RPROC_IOCREGEVENT, args);
            else
                osStatus = ioctl (ProcDrvTesla_handle, RPROC_IOCREGEVENT, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
        }
        break;

        case CMD_PROCMGR_UNREGEVENT:
        {
            ProcMgr_CmdArgsUnRegEvent *srcArgs = (ProcMgr_CmdArgsUnRegEvent *) args;

            if (srcArgs->procId == TEMP_PROC_SYSM3_ID)
                osStatus = ioctl (ProcDrvSysM3_handle, RPROC_IOCUNREGEVENT, args);
            else if (srcArgs->procId == TEMP_PROC_APPM3_ID)
                osStatus = ioctl (ProcDrvAppM3_handle, RPROC_IOCUNREGEVENT, args);
            else
                osStatus = ioctl (ProcDrvTesla_handle, RPROC_IOCUNREGEVENT, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
        }
        break;

        default:
        {
            osStatus = ioctl (ProcMgrDrvUsr_handle, cmd, args);
            if (osStatus < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Driver ioctl failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Driver ioctl failed!");
            }
        }
        break ;

        if (osStatus >= 0) {
            /* First field in the structure is the API status. */
            status = ((ProcMgr_CmdArgs *) args)->apiStatus;
        }

        GT_1trace (curTrace,
                   GT_1CLASS,
                   "    ProcMgrDrvUsr_ioctl: API Status [0x%x]",
                   status);
    }

    if (driverOpened == TRUE) {
        /* If the driver was temporarily opened here, close it. */
        tmpStatus = ProcMgrDrvUsr_close ();
        if ((status > 0) && (tmpStatus < 0)) {
            status = tmpStatus;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgrDrvUsr_ioctl",
                                 status,
                                 "Failed to close OS driver handle!");
        }
        ProcMgrDrvUsr_handle = -1;
    }

    GT_1trace (curTrace, GT_LEAVE, "ProcMgrDrvUsr_ioctl", status);

    /*! @retval PROCMGR_SUCCESS Operation successfully completed. */
    return status;
}


/** ============================================================================
 *  Internal functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to map the processor's memory regions to user space.
 *
 *  @param  procInfo    Processor information structure
 *
 *  @sa     _ProcMgrDrvUsr_unmapMemoryRegion
 */
static
Int
_ProcMgrDrvUsr_mapMemoryRegion (ProcMgr_ProcInfo * procInfo)
{
    Int                status = PROCMGR_SUCCESS;
    ProcMgr_AddrInfo * memEntry;
    Memory_MapInfo     mapInfo;
    UInt16             i;

    GT_1trace (curTrace, GT_ENTER, "_ProcMgrDrvUsr_mapMemoryRegion", procInfo);

    GT_1trace (curTrace,
               GT_2CLASS,
               "    Number of memory entries: %d\n",
               procInfo->numMemEntries);

    for (i = 0 ; i < procInfo->numMemEntries ; i++) {
        GT_assert (curTrace, (i < PROCMGR_MAX_MEMORY_REGIONS));
        /* Map all memory regions to user-space. */
        memEntry = &(procInfo->memEntries [i]);

        /* Get virtual address for shared memory. */
        /* TBD: For now, assume that slave virt is same as physical
         * address.
         */
        mapInfo.src  = memEntry->addr [ProcMgr_AddrType_MasterUsrVirt];
        mapInfo.size = memEntry->size;
        mapInfo.isCached = FALSE;
        mapInfo.drvHandle = (Ptr) ProcMgrDrvUsr_handle;

        status = Memory_map (&mapInfo);
        if (status < 0) {
            /*! @retval PROCMGR_E_OSFAILURE Memory map to user space failed */
            status = PROCMGR_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgrDrvUsr_ioctl",
                                 status,
                                 "Memory map to user space failed!");
        }
        else {
            GT_5trace (curTrace,
                       GT_2CLASS,
                       "    Memory region %d mapped:\n"
                       "        Region slave   base [0x%x]\n"
                       "        Region knlVirt base [0x%x]\n"
                       "        Region usrVirt base [0x%x]\n"
                       "        Region size         [0x%x]\n",
                       i,
                       memEntry->addr [ProcMgr_AddrType_SlaveVirt],
                       memEntry->addr [ProcMgr_AddrType_MasterKnlVirt],
                       mapInfo.dst,
                       mapInfo.size);
            memEntry->addr [ProcMgr_AddrType_MasterUsrVirt] = mapInfo.dst;
            memEntry->isInit = TRUE;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "_ProcMgrDrvUsr_mapMemoryRegion", status);

    /*! @retval PROCMGR_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief  Function to unmap the processor's memory regions to user space.
 *
 *  @param  procInfo    Processor information structure
 *
 *  @sa     _ProcMgrDrvUsr_mapMemoryRegion
 */
static
Int
_ProcMgrDrvUsr_unmapMemoryRegion (ProcMgr_ProcInfo * procInfo)
{
    Int                status = PROCMGR_SUCCESS;
    ProcMgr_AddrInfo * memEntry;
    Memory_UnmapInfo   unmapInfo;
    UInt16             i;

    GT_1trace (curTrace, GT_ENTER, "_ProcMgrDrvUsr_unmapMemoryRegion",
               procInfo);

    for (i = 0 ; i < procInfo->numMemEntries ; i++) {
        GT_assert (curTrace, (i < PROCMGR_MAX_MEMORY_REGIONS));
        /* Unmap all memory regions from user-space. */
        memEntry = &(procInfo->memEntries [i]);

        if (memEntry->isInit == TRUE) {
            /* Unmap memory region from user-space. */
            unmapInfo.addr = memEntry->addr [ProcMgr_AddrType_MasterUsrVirt];
            unmapInfo.size = memEntry->size;
            status = Memory_unmap (&unmapInfo);
            if (status < 0) {
                /*! @retval PROCMGR_E_OSFAILURE Memory unmap from user
                                                space failed */
                status = PROCMGR_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgrDrvUsr_ioctl",
                                     status,
                                     "Memory unmap from user space failed!");
            }
            else {
                memEntry->isInit = FALSE;
            }
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "_ProcMgrDrvUsr_unmapMemoryRegion", status);

    /*! @retval PROCMGR_SUCCESS Operation successfully completed. */
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
