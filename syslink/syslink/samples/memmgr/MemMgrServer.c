/*
 *  Copyright 2008-2011 Texas Instruments - http://www.ti.com/
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
 *  @file   MemMgr.c
 *
 *  @brief  TILER Client Sample application for TILER module between MPU & Ducati
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>
#include <String.h>
#include <Trace.h>
#include <SysLinkMemUtils.h>

/* IPC headers */
#include <IpcUsr.h>
#include <ProcMgr.h>

/* RCM headers */
#include <RcmServer.h>

/* TILER headers */
#include <tilermgr.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

#define RCMSERVER_NAME              "MEMALLOCMGRSERVER"

RcmServer_Handle rcmServerHandle;
Int              status;
sem_t            semMemMgrWait;
pthread_t        mmuFaultHandle = 0;

/*
 *  ======== signalHandler ========
 */
static Void signalHandler (Int sig)
{
    Osal_printf ("\nexiting from the memmgr application\n ");
    sem_post (&semMemMgrWait);
}

/*
 *  ======== mmuFaultHandler ========
 */
static Void mmuFaultHandler (Void)
{
    Int               status;
    UInt              i;
    ProcMgr_EventType events[] = {PROC_STOP, PROC_MMU_FAULT};

    status = ProcMgr_waitForMultipleEvents (PROC_SYSM3, events, 2, -1, &i);
    if (status < 0) {
        Osal_printf ("\nProcMgr_waitForEvent failed status %d\n ", status);
    }
    else {
        Osal_printf ("\nFatal error received!\n ");
    }

    Osal_printf ("\nexiting from the memmgr application\n ");
    sem_post (&semMemMgrWait);
}

struct MemMgr_funcInfo {
    RcmServer_MsgFxn fxnPtr;
    String           name;
};

struct MemMgr_funcInfo memMgrFxns [] =
{
    { SysLinkMemUtils_alloc,                  "MemMgr_Alloc"},
    { SysLinkMemUtils_free,                   "MemMgr_Free"},
};

/*
 *  ======== MemMgrThreadFxn ========
 *     TILER server thread function
*/
Void MemMgrThreadFxn (Char * rcmServerName)
{
    RcmServer_Params    rcmServerParams;
    UInt                fxnIdx;
    Int                 numFxns;
    Int                 i;

    /* RCM Server module init */
    Osal_printf ("RCM Server module init.\n");
    RcmServer_init ();

    /* rcm client module params init*/
    Osal_printf ("RCM Server module params init.\n");
    status = RcmServer_Params_init (&rcmServerParams);
    if (status < 0) {
        Osal_printf ("Error in RCM Server instance params init \n");
        goto exit_rcmserver_destroy;
    } else {
        Osal_printf ("RCM Server instance params init passed \n");
    }

    /* create the RcmServer instance */
    Osal_printf ("Creating RcmServer instance with name %s.\n", rcmServerName);
    status = RcmServer_create (rcmServerName, &rcmServerParams,
                                &rcmServerHandle);
    if (status < 0) {
        Osal_printf ("Error in RCM Server create.\n");
        goto exit_rcmserver_destroy;
    } else {
        Osal_printf ("RCM Server Create passed \n");
    }

    numFxns = sizeof (memMgrFxns) / sizeof (struct MemMgr_funcInfo);
    for (i = 0; i < numFxns; i++) {
        status = RcmServer_addSymbol (rcmServerHandle, memMgrFxns [i].name,
                            memMgrFxns [i].fxnPtr, &fxnIdx);
        /* Register the remote functions */
        Osal_printf ("Registering remote function %s with index %d\n",
                        memMgrFxns [i].name, fxnIdx);
        if (status < 0) {
            Osal_printf ("Add symbol failed with status 0x%08x.\n", status);
        }
    }

    Osal_printf ("Start RCM server thread \n");

    RcmServer_start (rcmServerHandle);
    Osal_printf ("RCM Server start passed \n");

    Osal_printf ("\nDone initializing RCM server.  Ready to receive requests "
                    "from Ducati.\n");

    /* wait for commands */
    sem_init (&semMemMgrWait, 0, 0);
    sem_wait (&semMemMgrWait);

    sem_destroy (&semMemMgrWait);

    for (i = 0; i < numFxns; i++) {
        /* Unregister the remote functions */
        status = RcmServer_removeSymbol (rcmServerHandle, memMgrFxns [i].name);
        if (status < 0) {
            Osal_printf ("Remove symbol %s failed.\n", memMgrFxns [i].name);
        }
    }

    status = RcmServer_delete (&rcmServerHandle);
    if (status < 0) {
        Osal_printf ("Error in RcmServer_delete: status = 0x%x\n", status);
        goto exit;
    }

exit_rcmserver_destroy:
    RcmServer_exit ();

exit:
    Osal_printf ("Leaving RCM server test thread function \n");
    return;
}

Int main (Int argc, Char * argv [])
{
    Int         status      = 0;
    Ipc_Config  config;

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
        goto exit;
    }

    /* Setup the signal handlers*/
    signal (SIGINT, signalHandler);
    signal (SIGKILL, signalHandler);
    signal (SIGTERM, signalHandler);

    /* Create the MMU fault handler thread */
    Osal_printf ("Create MMU fault handler thread.\n");
    pthread_create (&mmuFaultHandle, NULL,
                        (Void *)&mmuFaultHandler, NULL);

    MemMgrThreadFxn (RCMSERVER_NAME);

    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Error in Ipc_destroy: status = 0x%x\n", status);
    }

exit:
    Osal_printf ("Exiting memmgr application.\n");
    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
