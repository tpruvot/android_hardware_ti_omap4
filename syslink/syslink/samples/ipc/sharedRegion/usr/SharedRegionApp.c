/*
 *  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*============================================================================
 *  @file   SharedRegionApp.c
 *
 *  @brief  Sample application for SharedRegion module
 *
 *  ============================================================================
 */

 /* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>

 /* Linux OS-specific headers */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

 /* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>
#include <Memory.h>
#include <String.h>

/* Module level headers */
//#include <ti/ipc/Ipc.h>
#include <IpcUsr.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/SharedRegion.h>

#include <ProcMgr.h>
#include <ProcDefs.h>
#include <_ProcMgrDefs.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

#define PROCMGR_DRIVER_NAME         "/dev/syslink-procmgr"

/*!
 *  @brief  Maximum name length
 */
#define MAX_NAME_LENGTH 32u
/*!
 *  @brief  Maximum name length
 */
#define MAX_VALUE_LENGTH 32u
/*!
 *  @brief  Maximum name length
 */
#define MAX_RUNTIMEENTRIES 10u


/*!
 *  @brief  Shared memory base
 */
#define SHAREDREGION_ID         1

/*!
 *  @brief  Number of shared regions to be configured
 */
#define NUM_SHAREDREGIONS       3

/*!
 *  @brief  Shared memory size
 */
#define SHAREDREGION_SIZE       0x54000

#define SHAREDMEM               0xA0000000
#define SHAREDMEMSIZE           0x54000

/*
#define SHAREDMEM_PHY           0x83f00000
#define SHAREDMEMSIZE           0xF000
*/
ProcMgr_Handle sharedRegionApp_procMgrHandle = NULL;


UInt32 sharedRegionApp_shAddrBase;

UInt32 curAddr;

void * ProcMgrApp_startup ();

#define NOTIFY_SYSM3_IMAGE_PATH "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Int
sharedRegionApp_startup (Void)
{
    Int                  status     = 0;
    SharedRegion_Entry   entry;
    UInt32               fileId;
    UInt32               entryPoint = 0;
    ProcMgr_StartParams  startParams;
    char *               image_path;
    Ipc_Config config;
    ProcMgr_AttachParams attachParams;
    ProcMgr_State        state;
    Int                  i;
    Char                 tmpStr[80];

    Osal_printf ("Entered sharedRegionApp_startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    status = ProcMgr_open (&sharedRegionApp_procMgrHandle,
                           MultiProc_getId("SysM3"));
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }

    if (status >= 0) {
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (sharedRegionApp_procMgrHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (sharedRegionApp_procMgrHandle);
            Osal_printf ("After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", state);
        }
    }

    startParams.proc_id = MultiProc_getId("SysM3");
    image_path = NOTIFY_SYSM3_IMAGE_PATH;
    status = ProcMgr_load (sharedRegionApp_procMgrHandle, NOTIFY_SYSM3_IMAGE_PATH,
                           2, &image_path, &entryPoint,
                           &fileId, startParams.proc_id);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_load [0x%x]:SYSM3\n", status);
        return status;
    }
    else {
        Osal_printf ("ProcMgr_load successful!\n");
    }

    status = ProcMgr_start (sharedRegionApp_procMgrHandle, 0, &startParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_start [0x%x]:SYSM3\n", status);
        return status;
    }
    status = ProcMgr_translateAddr (sharedRegionApp_procMgrHandle,
                                    (Ptr) &sharedRegionApp_shAddrBase,
                                    ProcMgr_AddrType_MasterUsrVirt,
                                    (Ptr) SHAREDMEM,
                                    ProcMgr_AddrType_SlaveVirt);

    if (status < 0) {
        Osal_printf ("Error in ProcMgr_translateAddr [0x%x]\n", status);
    } else {
        Osal_printf ("\nProcMgr_translateAddr: Slave Virtual Addr:%x\n"
                    "Master Usr Virtual Addr:%x\n",
                    SHAREDMEM, sharedRegionApp_shAddrBase);
    }

    for (i = 0 ; i < NUM_SHAREDREGIONS ; i++) {
        /*  Create a shared region with an id  identified by i */
        /* base address of the region */
        entry.base = (Ptr) (sharedRegionApp_shAddrBase
                                       + (i * SHAREDREGION_SIZE));

        /*Length of the region */
        entry.len = SHAREDREGION_SIZE;

        /*MultiProc id of the owner of the region */
        entry.ownerProcId = MultiProc_self();

        /*Whether the region is valid */
        entry.isValid = TRUE;

        /*Whether to perform cache operations for the region */
        entry.cacheEnable = TRUE;

        /*The cache line size of the region */
        entry.cacheLineSize = 128;
        entry.createHeap = TRUE;
        /*The name of the region */
        sprintf(tmpStr,"AppSharedRegion%d" ,i);
        entry.name = tmpStr;

        status = SharedRegion_setEntry ((SHAREDREGION_ID + i), &entry);

        if (status < 0) {
            Osal_printf ("Error in SharedRegion_setEntry. Status [0x%x]"
                         " for:\n"
                         "    ID     : [%d]\n"
                         "    Address: [0x%x]\n"
                         "    Size   : [0x%x]\n",
                         status,
                         (SHAREDREGION_ID + i),
                         (Ptr) (sharedRegionApp_shAddrBase
                                        + (i * SHAREDREGION_SIZE)),
                         SHAREDREGION_SIZE);
            break;
        }
        else {
            Osal_printf ("SharedRegion_setEntry Status [0x%x]\n", status);
        }
    }
    return 0;
}

Int
sharedRegionApp_execute (Void)
{
    UInt32              usrVirtAddress = sharedRegionApp_shAddrBase;
    SharedRegion_SRPtr  srPtr;
    UInt32              Index = (~0);
    SharedRegion_Entry  info_set;
    SharedRegion_Entry  info_get;

    usrVirtAddress += 0x100;

    Osal_printf ("User virtual address  =  [0x%x]\n", usrVirtAddress);
    srPtr = SharedRegion_getSRPtr((Ptr)usrVirtAddress, 1);
    Osal_printf ("SharedRegion pointer  =  [0x%x]\n", srPtr);
    usrVirtAddress = (UInt32)SharedRegion_getPtr(srPtr);
    Osal_printf ("User virtual pointer  =  [0x%x]\n", usrVirtAddress);

    usrVirtAddress += 0x200;

    Osal_printf ("User virtual address  =  [0x%x]\n", usrVirtAddress);
    srPtr = SharedRegion_getSRPtr((Ptr)usrVirtAddress, 1);
    Osal_printf ("SharedRegion pointer  =  [0x%x]\n", srPtr);
    usrVirtAddress = (UInt32)SharedRegion_getPtr(srPtr);
    Osal_printf ("User virtual pointer  =  [0x%x]\n", usrVirtAddress);

    Index = SharedRegion_getId((Ptr)usrVirtAddress);
    Osal_printf ("User virtual address [0x%x] is in"
        " table entry with index = [0x%x]\n", usrVirtAddress, Index);


    info_set.isValid = 1;
    info_set.base = (Ptr)(usrVirtAddress + 4 * SHAREDMEMSIZE);
    info_set.len = SHAREDMEMSIZE;

    SharedRegion_clearEntry (2);
    SharedRegion_setEntry (2, &info_set);
    SharedRegion_getEntry (2, &info_get);
    Osal_printf ("User virtual pointer in table with index 2 is [0x%x]\n"
                 " with size [0x%x]\n", info_get.base, info_get.len);


    usrVirtAddress = usrVirtAddress + 4 * SHAREDMEMSIZE;
    usrVirtAddress += 0x6000;

    Osal_printf ("User virtual address  =  [0x%x]\n", usrVirtAddress);
    srPtr = SharedRegion_getSRPtr((Ptr)usrVirtAddress, 2);
    Osal_printf ("User region pointer   =  [0x%x]\n", srPtr);
    usrVirtAddress = (UInt32)SharedRegion_getPtr(srPtr);
    Osal_printf ("User virtual pointer  =  [0x%x]\n", usrVirtAddress);


    Osal_printf ("Passing an address which is not in any of the SharedRegion\n"
        "areas registered -- this should fail!\n");
    usrVirtAddress += 0x200000;
    Osal_printf ("User virtual address  =  [0x%x]\n", usrVirtAddress);
    srPtr = SharedRegion_getSRPtr((Ptr)usrVirtAddress, 0);
    Osal_printf ("User region pointer   =  [0x%x]\n", srPtr);
    usrVirtAddress = (UInt32)SharedRegion_getPtr(srPtr);
    Osal_printf ("User virtual pointer  =  [0x%x]\n", usrVirtAddress);

    return (0);
}

Int
sharedRegionApp_shutdown (Void)
{
    Int32 status = 0;
    ProcMgr_StopParams  stopParams;

    stopParams.proc_id = MultiProc_getId ("SysM3");
    status = ProcMgr_stop (sharedRegionApp_procMgrHandle, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop [0x%d]:SYSM3\n", status);
        return status;
    }
    Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);

    status =  ProcMgr_detach (sharedRegionApp_procMgrHandle);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach [0x%d]\n", status);
        return status;
    }
    Osal_printf ("ProcMgr_detach status: [0x%x]\n", status);

    status = ProcMgr_close (&sharedRegionApp_procMgrHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);

    Ipc_destroy ();

    Osal_printf ("Leaving sharedRegionApp_shutdown\n");

    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
