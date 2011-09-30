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
 *  @file   ListMPApp.c
 *
 *  @brief  Sample application for ListMP module
 *  ============================================================================
 */

/* Linux specific header files */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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
#include <omap4430proc.h>
#include <ConfigNonSysMgrSamples.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/ListMP.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/* prefix for the host listmp. */
#define LISTMPHOST_PREFIX          "LISTMPHOST"

/* prefix for the slave listmp. */
#define LISTMPSLAVE_PREFIX         "LISTMPSLAVE"

/* Length of ListMP Names */
#define  LISTMPAPP_NAMELENGTH       80u

/* Shared Region ID */
#define APP_SHAREDREGION_ENTRY_ID   0u

/* shared memory size */
#define SHAREDMEM               0xA0000000

/* Memory for the Notify Module */
#define NOTIFYMEM               (SHAREDMEM)
#define NOTIFYMEMSIZE           0x4000

/* Memory a GatePeterson instance */
#define GATEMPMEM               (NOTIFYMEM + NOTIFYMEMSIZE)
#define GATEMPMEMSIZE           0x1000

/* Memory a HeapMultiBuf instance */
#define HEAPMBMEM_CTRL          (GATEMPMEM + GATEMPMEMSIZE)
#define HEAPMBMEMSIZE_CTRL      0x1000
#define HEAPMBMEM_BUFS          (HEAPMBMEM_CTRL + HEAPMBMEMSIZE_CTRL)
#define HEAPMBMEMSIZE_BUFS      0x3000

/* Memory for NameServerRemoteNotify */
#define NSRN_MEM                (HEAPMBMEM_BUFS + HEAPMBMEMSIZE_BUFS)
#define NSRN_MEMSIZE            0x1000

/* Memory a Transport instance */
#define TRANSPORTMEM            (NSRN_MEM + NSRN_MEMSIZE)
#define TRANSPORTMEMSIZE        0x2000

/* Memory for MessageQ's NameServer instance */
#define MESSAGEQ_NS_MEM         (TRANSPORTMEM + TRANSPORTMEMSIZE)
#define MESSAGEQ_NS_MEMSIZE     0x1000

/* Memory for HeapBuf's NameServer instance */
#define HEAPBUF_NS_MEM          (MESSAGEQ_NS_MEM + MESSAGEQ_NS_MEMSIZE)
#define HEAPBUF_NS_MEMSIZE      0x1000

#define GATEMPMEM1              (HEAPBUF_NS_MEM + HEAPBUF_NS_MEMSIZE)
#define GATEMPMEMSIZE1          0x1000

/* Memory for the Notify Module */
#define HEAPMEM                 (GATEMPMEM1 + GATEMPMEMSIZE1)
#define HEAPMEMSIZE             0x1000

#define HEAPMEM1                (HEAPMEM + HEAPMEMSIZE)
#define HEAPMEMSIZE1            0x1000

#define LOCAL_LIST              (HEAPMEM1 + HEAPMEMSIZE1)
#define LOCAL_LIST_SIZE         0x1000

#define REMOTE_LIST             (LOCAL_LIST + LOCAL_LIST_SIZE)
#define REMOTE_LIST_SIZE        0x1000

#define LOCAL_LIST_OFFSET       (LOCAL_LIST - SHAREDMEM)
#define REMOTE_LIST_OFFSET      (REMOTE_LIST - SHAREDMEM)

/* Base Image to be loaded */
#define LISTMP_SYSM3_IMAGE_PATH "./ListMP_MPUSYS_Test_Core0.xem3"
/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/* Handle to the ListMP instance used. */
ListMP_Handle             ListMPApp_handle;
GateMP_Handle             ListMPApp_gateHandle;
UInt32  ListMPApp_shAddrBase;

typedef struct ListMP_Node_Tag
{
    ListMP_Elem elem;
    Int32  id;
}ListMP_Node;


/*  ============================================================================
 *  Functions
 *  ============================================================================
 */


Int
ListMPApp_startup (UInt32 sharedAddr)
{
    Int                       status  = 0;
    Ipc_Config config;
    ProcMgr_AttachParams      attachParams;
    ProcMgr_State             state;

    Osal_printf ("\nEntered ListMPApp_startup\n");

    Ipc_getConfig (&config);
    Osal_printf ("Calling Ipc_setup \n");
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }
    status = ProcMgr_open (&procHandle,
                           MultiProc_getId("SysM3"));
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

    ProcUtil_load(LISTMP_SYSM3_IMAGE_PATH);
    ProcUtil_start();

    Osal_printf ("Leaving ListMPApp_startup\n");

    return (status);
}


Int
ListMPApp_execute (UInt32 sharedAddr)
{
    Int               status  = -1;
    ListMP_Params     listMPParamsLocal;
    ListMP_Handle     ListMPApp_handleRemote;
    ListMP_Handle     ListMPApp_handleLocal;
    ListMP_Node *     node;
    UInt32            localProcId;
    Char              tempStr [LISTMPAPP_NAMELENGTH];
    UInt              i;
    Ptr               ListMPApp_heapHandle;
    SharedRegion_Entry      entry;
    Memory_Stats            stats;

//    sharedAddr = ListMPApp_shAddrBase;
    SharedRegion_getEntry(0, &entry);
    sharedAddr = (UInt32)entry.base;

    Osal_printf("\nEntered ListMPApp_execute\n");
    localProcId = MultiProc_self();

    ListMPApp_heapHandle = SharedRegion_getHeap(APP_SHAREDREGION_ENTRY_ID);

    if (ListMPApp_heapHandle == NULL) {
        status = ListMP_E_FAIL;
        Osal_printf ("Error in SharedRegion_getHeap\n");
    }
    else {
        Osal_printf ("Heap in SharedRegion_getHeap : 0x%x\n",
                     ListMPApp_heapHandle);

        Memory_getStats(ListMPApp_heapHandle, &stats);
        Osal_printf("Heap stats: 0x%x bytes free, "
            "0x%x bytes total\n", stats.totalFreeSize, stats.totalSize);
    }

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
    ListMPApp_handleLocal = ListMP_create(&listMPParamsLocal);
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
        //status = ListMP_open(tempStr,&ListMPApp_handleRemote);
        status = ListMP_openByAddr((Void *)(sharedAddr + REMOTE_LIST_OFFSET),&ListMPApp_handleRemote);
        if (status == ListMP_E_NOTFOUND) {
            Osal_printf("List %s not found, trying again.\n", tempStr);
            /* Sleep for a while before trying again. */
            usleep (1000);
        }
    }
    while (status != ListMP_S_SUCCESS);
    Osal_printf("List %s successfully opened.\n", tempStr);

    /* -------------------------------------------------------------------------
     * Get nodes from locally created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        do {
            node = (ListMP_Node *) ListMP_getHead (ListMPApp_handleLocal);
            /* Sleep for a while if the element is not yet available. */
            if (node == NULL) {
                usleep (1000);
            }
        }
        while (node == NULL);
        Osal_printf("Obtained node 0x%x with id 0x%x.\n", (UInt32) node, node->id);

        Memory_free (ListMPApp_heapHandle,
                      node,
                      sizeof (ListMP_Node));
    }

    /* -------------------------------------------------------------------------
     * Put nodes in remotely created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        node = (ListMP_Node *) Memory_alloc (
                                   (IHeap_Handle)ListMPApp_heapHandle,
                                   sizeof (ListMP_Node),
                                   0);
        if (NULL == node) {
            status = ListMP_E_MEMORY;
            Osal_printf("ERROR: Failed to allocate nodes for remotely created list.\n");
            break;
        }
        Osal_printf("Allocated node at 0x%x\n", (UInt32)node);
        node->id = 0x0 + i;
        ListMP_putTail (ListMPApp_handleRemote, &(node->elem));

        Osal_printf("Node 0x%x with id 0x%x successfully put.\n", (UInt32)node, node->id);
    }
    if (status < 0)
        goto func_clean;
    Osal_printf("Waiting for synchronization, press key to continue.\n");
    getchar();

    /* -------------------------------------------------------------------------
     * Put nodes in locally created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        node = (ListMP_Node *) Memory_alloc (
                                   (IHeap_Handle)ListMPApp_heapHandle,
                                    sizeof (ListMP_Node),
                                    0);
        if (NULL == node) {
            status = ListMP_E_MEMORY;
            Osal_printf("ERROR: Failed to allocate nodes for locally created list.\n\n");
            break;
        }
        Osal_printf("Allocated node at 0x%x\n", (UInt32)node);
        node->id = 0x100 + i;
        ListMP_putTail (ListMPApp_handleLocal, &(node->elem));

        Osal_printf("Node 0x%x with id 0x%x successfully put.\n", (UInt32)node, node->id);
    }
    if (status < 0)
        goto func_clean;

    /* -------------------------------------------------------------------------
     * Get nodes from remotely created list.
     * -------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        do {
            node = (ListMP_Node *) ListMP_getTail (ListMPApp_handleRemote);
            /* Sleep for a while if the element is not yet available. */
            if (node == NULL) {
                usleep (1000);
            }
        }
        while (node == NULL);

        Osal_printf("Obtained node 0x%x with id 0x%x.\n", (UInt32) node, node->id);
        Memory_getStats(ListMPApp_heapHandle, &stats);

        Memory_free (ListMPApp_heapHandle,
                      node,
                      sizeof (ListMP_Node));
    }
func_clean:
    /* -------------------------------------------------------------------------
     * Cleanup
     * -------------------------------------------------------------------------
     */
    status = ListMP_close (&ListMPApp_handleRemote);
    if (status != ListMP_S_SUCCESS) {
        Osal_printf("ERROR: ListMP_close failed.status [0x%x]\n", status);
    } else {
        Osal_printf("ListMP_close status [0x%x]\n", status);
    }

    status = ListMP_delete (&ListMPApp_handleLocal);
    if (status != ListMP_S_SUCCESS) {
        Osal_printf("ERROR: ListMP_delete failed.status [0x%x]\n", status);
    } else {
        Osal_printf("ListMP_delete status [0x%x]\n", status);
    }

    Memory_getStats(ListMPApp_heapHandle, &stats);
    Osal_printf("Heap stats: 0x%x bytes free, "
        "0x%x bytes total\n", stats.totalFreeSize, stats.totalSize);
    Osal_printf ("Leaving ListMPApp_execute\n");

    getchar();
    return (0);
}


Int
ListMPApp_shutdown (Void)
{
    Int32 status = 0;

    Osal_printf ("\nEntered ListMPApp_shutdown\n");

    ProcUtil_stop ();
    status = ProcMgr_detach (procHandle);
    Osal_printf ("ProcMgr_detach status: [0x%x]\n", status);
    status = ProcMgr_close (&procHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);
    Ipc_destroy ();

    Osal_printf ("Leaving ListMPApp_shutdown\n");
    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
