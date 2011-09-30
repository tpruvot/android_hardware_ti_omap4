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
 *  @file   HeapBufMPApp.c
 *
 *  @brief  Sample application for HeapBufMP module
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
#include <ti/ipc/HeapBufMP.h>
#include <omap4430proc.h>
#include <ConfigNonSysMgrSamples.h>
#include <HeapBufMPApp.h>
#include "HeapBufMPApp_config.h"

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

#define HEAPBUF_SYSM3_IMAGE_PATH "./HeapBufMP_MPUSYS_Test_Core0.xem3"
#define HEAPBUF_APPM3_IMAGE_PATH ""

ProcMgr_Handle       HeapBufMPApp_Prochandle   = NULL;
UInt32               curAddr   = 0;

typedef struct ListMP_Node_Tag
{
    ListMP_Elem elem;
    UInt32      id;
} ListMP_Node;


/*
 *  Function to execute the startup for HeapBufMPApp sample application
 */
Int
HeapBufMPApp_startup (Void)
{
    Int32                     status = 0 ;
    Ipc_Config config;
    /* String                    Gate_Name = "TESTGATE"; */
    UInt16                    Remote_procId;
    UInt16                    myProcId;
    ProcMgr_AttachParams      attachParams;
    ProcMgr_State             state;

    procId = 2;
    Osal_printf ("Entered HeapBufMP startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    myProcId = MultiProc_self();
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
    }
    procId = 2;

    ProcUtil_load (HEAPBUF_SYSM3_IMAGE_PATH);
    Osal_printf ("Done loading image to SYSM3\n");
    ProcUtil_start ();
    Osal_printf ("Started Ducati: SYSM3\n");

    Osal_printf ("Leaving HeapBufMPApp_startup\n");
    return 0;
}


/*
 *  Function to execute the HeapBufMPApp sample application
 */
Int
HeapBufMPApp_execute (Void)
{
    Int32                status = 0;
    Int                  i;
    ListMP_Params        listMPParamsLocal;
    ListMP_Node *        node;
    ListMP_Handle        listmpHandleRemote;
    ListMP_Handle        listmpHandleLocal;
    HeapBufMP_Handle     heapHandleLocal;
    HeapBufMP_Handle     heapHandleRemote;
    HeapBufMP_Params     heapParams;
    UInt32               localProcId;
    Char                 tempStr [LISTMPAPP_NAMELENGTH];
    SharedRegion_Entry   entry;
    Memory_Stats         stats;
    UInt32               sharedAddr;

    SharedRegion_getEntry(0, &entry);
    sharedAddr = (UInt32)entry.base;

    Osal_printf("\nEntered HeapBufMPApp_execute\n");
    localProcId = MultiProc_self();

    /* -------------------------------------------------------------------------
     * Create and open lists
     * -------------------------------------------------------------------------
     */
    ListMP_Params_init(&listMPParamsLocal);
    listMPParamsLocal.regionId = APP_SHAREDREGION_ENTRY_ID;/*
    memset (tempStr, 0, LISTMPAPP_NAMELENGTH);
    sprintf (tempStr,
            "%s_%d%d",
            LISTMPSLAVE_PREFIX,
            procId,
            localProcId);
    listMPParamsLocal.name = tempStr;
    */
    sprintf(tempStr, "MPU List");
    listMPParamsLocal.sharedAddr = (Void *)(sharedAddr + LOCAL_LIST_OFFSET);
    listmpHandleLocal = ListMP_create(&listMPParamsLocal);
    Osal_printf("created list\n");
    /* Open remote list. */
    /*
    memset (tempStr, 0, LISTMPAPP_NAMELENGTH);
    sprintf (tempStr,
            "%s_%d%d",
            LISTMPHOST_PREFIX,
            procId,
            localProcId);
    */
    sprintf(tempStr, "Ducati List");

    Osal_printf("Opening list\n");
    /* Open the remote list by name */
    do {
        //status = ListMP_open(tempStr,&listmpHandleRemote);
        status = ListMP_openByAddr((Void *)(sharedAddr + REMOTE_LIST_OFFSET),&listmpHandleRemote);
        if (status == ListMP_E_NOTFOUND) {
            Osal_printf("List %s not found, trying again.\n", tempStr);
            /* Sleep for a while before trying again. */
            usleep (1000);
        }
    }
    while (status != ListMP_S_SUCCESS);
    Osal_printf("List %s successfully opened.\n", tempStr);
    /* -------------------------------------------------------------------------
     * Create HeapBufMP
     * -------------------------------------------------------------------------
     */

    HeapBufMP_Params_init(&heapParams);
    heapParams.sharedAddr = (Ptr)(sharedAddr + LOCAL_HEAP_OFFSET);
    heapParams.blockSize = 0x80;    /* Make sure this can fit in HEAPBUFMP_SIZE */
    heapParams.numBlocks = 32;
    heapHandleLocal = HeapBufMP_create(&heapParams);
    if (heapHandleLocal == NULL) {
        Osal_printf("HeapBufMP creation failed!\n");
        exit(-1);
    }
    Osal_printf("Local HeapBufMP successfully created.\n");

    /* -------------------------------------------------------------------------
     * Open remote HeapBufMP
     * -------------------------------------------------------------------------
     */
    do {
        status = HeapBufMP_openByAddr((Ptr)(sharedAddr + REMOTE_HEAP_OFFSET),
            &heapHandleRemote);
        if (status == HeapBufMP_E_NOTFOUND) {
            /* Sleep for a while before trying again. */
            usleep(1000);
        }
    }
    while (status != HeapBufMP_S_SUCCESS);
    Osal_printf("Remote HeapBufMP successfully opened.\n");

    Memory_getStats((IHeap_Handle)heapHandleLocal, &stats);
    Osal_printf("Heap stats: 0x%x bytes free, "
        "0x%x bytes total\n", stats.totalFreeSize, stats.totalSize);


    /* -------------------------------------------------------------------------
     * Put nodes in remotely created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        node = (ListMP_Node *) Memory_alloc (
                                   (IHeap_Handle)heapHandleRemote,
                                   sizeof (ListMP_Node),
                                   0);
        if (node == NULL) {
            Osal_printf("Failed to allocate node for remote list.\n");
        }
        else {
            Osal_printf("Allocated node at 0x%x\n", (UInt32)node);
            node->id = 0x0 + i;
            ListMP_putTail (listmpHandleRemote, &(node->elem));

            Osal_printf("Node 0x%x with id 0x%x successfully put.\n", (UInt32)node, node->id);
        }
    }

    /* -------------------------------------------------------------------------
     * Get nodes from locally created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        do {
            node = (ListMP_Node *) ListMP_getHead (listmpHandleLocal);
            /* Sleep for a while if the element is not yet available. */
            if (node == NULL) {
                usleep (1000);
            }
        }
        while (node == NULL);
        Osal_printf("Obtained node 0x%x with id 0x%x.\n", (UInt32) node, node->id);

        Memory_free ((IHeap_Handle)heapHandleLocal,
                      node,
                      sizeof (ListMP_Node));
    }

    Osal_printf("Waiting for synchronization, press key to continue.\n");
    getchar();

    /* -------------------------------------------------------------------------
     * Put nodes in locally created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        node = (ListMP_Node *) Memory_alloc (
                                   (IHeap_Handle)heapHandleLocal,
                                    sizeof (ListMP_Node),
                                    0);
        if (node == NULL) {
            Osal_printf("Failed to allocate node for remote list.\n");
        }
        else {
            Osal_printf("Allocated node at 0x%x\n", (UInt32)node);
            node->id = 0x100 + i;
            ListMP_putTail (listmpHandleLocal, &(node->elem));

            Osal_printf("Node 0x%x with id 0x%x successfully put.\n", (UInt32)node, node->id);
        }
    }

    /* -------------------------------------------------------------------------
     * Get nodes from remotely created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        do {
            node = (ListMP_Node *) ListMP_getTail (listmpHandleRemote);
            /* Sleep for a while if the element is not yet available. */
            if (node == NULL) {
                usleep (1000);
            }
        }
        while (node == NULL);

        Osal_printf("Obtained node 0x%x with id 0x%x.\n", (UInt32) node, node->id);

        Memory_free ((IHeap_Handle)heapHandleRemote,
                      node,
                      sizeof (ListMP_Node));
    }

    /* -------------------------------------------------------------------------
     * Cleanup
     * -------------------------------------------------------------------------
     */
    Memory_getStats((IHeap_Handle)heapHandleLocal, &stats);
    Osal_printf("Heap stats: 0x%x bytes free, "
        "0x%x bytes total\n", stats.totalFreeSize, stats.totalSize);

    status = HeapBufMP_close (&heapHandleRemote);
    if (status != HeapBufMP_S_SUCCESS) {
        Osal_printf("ERROR: HeapBufMP_close failed. status [0x%x]\n", status);
    } else {
        Osal_printf("HeapBufMP_close status [0x%x]\n", status);
    }

    status = HeapBufMP_delete (&heapHandleLocal);
    if (status != HeapBufMP_S_SUCCESS) {
        Osal_printf("ERROR: HeapBufMP_delete failed. status [0x%x]\n", status);
    } else {
        Osal_printf("HeapBufMP_delete status [0x%x]\n", status);
    }

    status = ListMP_close (&listmpHandleRemote);
    if (status != ListMP_S_SUCCESS) {
        Osal_printf("ERROR: ListMP_close failed.status [0x%x]\n", status);
    } else {
        Osal_printf("ListMP_close status [0x%x]\n", status);
    }

    status = ListMP_delete (&listmpHandleLocal);
    if (status != ListMP_S_SUCCESS) {
        Osal_printf("ERROR: ListMP_delete failed.status [0x%x]\n", status);
    } else {
        Osal_printf("ListMP_delete status [0x%x]\n", status);
    }

    Osal_printf ("Leaving HeapBufMPApp_execute\n");
    return (0);
}


Int
HeapBufMPApp_shutdown (Void)
{
    Int32 status;

    Osal_printf ("Entered HeapBufMPApp_shutdown\n");

    ProcUtil_stop ();
    status = ProcMgr_detach (procHandle);
    Osal_printf ("ProcMgr_detach status: [0x%x]\n", status);
    status = ProcMgr_close (&procHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);
    Ipc_destroy ();

    Osal_printf ("Leaving HeapBufMPApp_shutdown\n");
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
