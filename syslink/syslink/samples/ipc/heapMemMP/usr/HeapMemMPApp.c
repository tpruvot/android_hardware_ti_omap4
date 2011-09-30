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
 *  @file   HeapMemMPApp.c
 *
 *  @brief  Sample application for HeapMemMP module
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
#include <OsalSemaphore.h>
#include <Memory.h>
#include <String.h>

/* Module level headers */
#include <ProcMgr.h>
#include <_ProcMgrDefs.h>

//#include <ti/ipc/Ipc.h>
#include <IpcUsr.h>
#include <Heap.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/HeapMemMP.h>
#include <omap4430proc.h>
#include <ConfigNonSysMgrSamples.h>
#include <HeapMemMPApp.h>
#include "HeapMemMPApp_config.h"

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

#define GATEPETERSONMEMOFFSET    (GATEPETERSONMEM - SHAREDMEM)
#define HHEAPMEMMEMOFFSET_1       (HEAPMBMEM_CTRL - SHAREDMEM)
#define HHEAPMEMMEMOFFSET_2       (HEAPMBMEM1_CTRL - SHAREDMEM)
#define HHEAPMEM_SYSM3_IMAGE_PATH "/binaries/HeapMemMPTestApp_SYSM3_MPU_SYSMGR.xem3"
#define HHEAPMEM_APPM3_IMAGE_PATH ""

ProcMgr_Handle       HeapMemMPApp_Prochandle   = NULL;
UInt32               curAddr   = 0;
HeapMemMP_Handle       heapHandle;
HeapMemMP_Handle       heapHandle1;

typedef struct Heap_Block_Tag
{
    Char   name[MAX_NAME_LENGTH];
    Int32  id;
}Heap_Block;

/*!
 *  @brief  Function to execute the startup for HeapMemMPApp sample application
 *
 *  @sa
 */
Int
HeapMemMPApp_startup (Void)
{
    Int32                     status = 0 ;
    Ipc_Config config;
    /* String                    Gate_Name = "TESTGATE"; */
    UInt16                    Remote_procId;
    UInt16                    myProcId;
    SharedRegion_Entry        entry;
    ProcMgr_AttachParams      attachParams;
    ProcMgr_State             state;

    procId = 2;
    Osal_printf ("Entered HeapMemMP startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    myProcId = MultiProc_getId("MPU");
    Osal_printf ("MultiProc_getId(MPU) = 0x%x]\n", myProcId);

    /* Get MultiProc ID by name for the remote processor. */
    Remote_procId = MultiProc_getId ("SysM3");
    Osal_printf ("MultiProc_getId(SysM3) = 0x%x]\n", Remote_procId);

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procHandle, Remote_procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }
    else {
        Osal_printf ("ProcMgr_open Status [0x%x]\n", status);

        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procHandle);
            Osal_printf ("After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", status);
        }

        if (status >= 0) {
            /* Get the address of the shared region in kernel space. */
            status = ProcMgr_translateAddr (procHandle,
                                            (Ptr) &curAddr,
                                            ProcMgr_AddrType_MasterUsrVirt,
                                            (Ptr) SHAREDMEM,
                                            ProcMgr_AddrType_SlaveVirt);
            if (status < 0) {
                Osal_printf ("Error in ProcMgr_translateAddr [0x%x]\n",
                             status);
            } else {
                Osal_printf ("Virt address of shared address base:"
                             " [0x%x]\n",
                             curAddr);
            }
        }
    }

    ProcUtil_load (HHEAPMEM_SYSM3_IMAGE_PATH);
    Osal_printf ("Done loading image to SYSM3\n");

    entry.base = (Ptr) curAddr;
    entry.len = SHAREDMEMSIZE;
    SharedRegion_setEntry(0, &entry);

    procId = 2;
    ProcUtil_start ();
    Osal_printf ("Started Ducati:SYSM3\n");

    return 0;
}


/*!
 *  @brief  Function to execute the HeapMemMPApp sample application
 *
 *  @sa     HeapMemMPApp_callback
 */
Int
HeapMemMPApp_execute (Void)
{
    int size = 0;
    Int32          status     = -1;
    HeapMemMP_Params heapmemmpParams;
    HeapMemMP_Params heapmemmpParams1;
    Heap_Block *   heapBlock;
    HeapMemMP_ExtendedStats stats;

    HeapMemMP_Params_init(&heapmemmpParams);

    heapmemmpParams.name              = NULL;
    heapmemmpParams.sharedAddr        = (Ptr)(curAddr + HHEAPMEMMEMOFFSET_1);
    heapmemmpParams.sharedBufSize     = HEAPMEMSIZE;

    HeapMemMP_Params_init(&heapmemmpParams1);

    heapmemmpParams1.name              = NULL;
    heapmemmpParams1.sharedAddr        = (Ptr)(curAddr + HHEAPMEMMEMOFFSET_2);
    heapmemmpParams1.sharedBufSize     = HEAPMEMSIZE;

    size = HeapMemMP_sharedMemReq(&heapmemmpParams1);
    /* Clear the shared area for the new heap instance */
    memset(heapmemmpParams1.sharedAddr, 0, HeapMemMP_sharedMemReq(&heapmemmpParams1));

    do {
        heapmemmpParams.sharedAddr = (Ptr)(curAddr + HHEAPMEMMEMOFFSET_1);
        status =  HeapMemMP_openByAddr(heapmemmpParams.sharedAddr, &heapHandle);
    } while(status < 0);

    if (heapHandle == NULL) {
        Osal_printf ("Error in HeapMemMP_open [0x%x]\n",status);
    }
    else {
        Osal_printf ("HeapMemMP_open heapHandle:%p\n", heapHandle);
        heapBlock = (Heap_Block *)HeapMemMP_alloc(
                                heapHandle,
                                sizeof(Heap_Block),
                                0u);

        HeapMemMP_getExtendedStats(heapHandle,
                                  &stats);

        Osal_printf ("HeapMemMP_getExtendedStats size = %d\n",
                                                    stats.size);

        HeapMemMP_free(heapHandle,
                            heapBlock,
                            sizeof(Heap_Block));
    }

    heapHandle1 = HeapMemMP_create(&heapmemmpParams1);
    Osal_printf ("HeapMemMP_create heapHandle1:%p\n", heapHandle1);
    return status;
}


/*!
 *  @brief  Function to execute the shutdown for HeapMemMPApp sample application
 *
 *  @sa     HeapMemMPApp_callback
 */
Int
HeapMemMPApp_shutdown (Void)
{
    Int32 status;

    Osal_printf ("Entered HeapMemMPApp_shutdown\n");

    status = HeapMemMP_close (&heapHandle);
    Osal_printf ("HeapMemMP_close status: [0x%x]\n", status);

    status = HeapMemMP_delete (&heapHandle1);
    Osal_printf ("HeapMemMP_delete status: [0x%x]\n", status);

    ProcUtil_stop ();
    status = ProcMgr_close (&procHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);
    Ipc_destroy ();

    Osal_printf ("Leaving HeapMemMPApp_shutdown\n");
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
