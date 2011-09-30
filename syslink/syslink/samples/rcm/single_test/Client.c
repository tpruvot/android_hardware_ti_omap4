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

/*==============================================================================
 *  @file   Client.c
 *
 *  @brief  RCM Client Sample application for RCM module between MPU & Ducati
 *
 *  ============================================================================
 */

 /* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>
#include <Memory.h>
#include <String.h>

/* IPC headers */
#include <IpcUsr.h>
#include <ProcMgr.h>

/* RCM headers */
#include <RcmClient.h>

/* Sample headers */
#include <RcmTest_Config.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          SysM3
 */
#define RCM_MPUCLIENT_SYSM3ONLY_IMAGE  "./RCMServer_MPUSYS_Test_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_MPUCLIENT_SYSM3_IMAGE      "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_MPUCLIENT_APPM3_IMAGE      "./RCMServer_MPUAPP_Test_Core1.xem3"

/*!
 *  @brief  Name of the Tesla baseImage to be used for sample execution with
 *          Tesla
 */
#define RCM_MPUCLIENT_DSP_IMAGE         "./RCMServer_Dsp_Test.xe64T"

typedef struct {
    Int a;
} RCM_Remote_FxnArgs;


RcmClient_Handle        rcmClientHandle	    = NULL;
UInt                    fxnDoubleIdx;
UInt                    fxnExitIdx;

/* Common definitions */
Char                  * remoteServerName;
#if !defined(SYSLINK_USE_DAEMON)
HeapBufMP_Handle        heapHandle          = NULL;
SizeT                   heapSize            = 0;
Ptr                     heapBufPtr          = NULL;
IHeap_Handle            srHeap              = NULL;
#endif
ProcMgr_Handle          procMgrHandleClient;
ProcMgr_Handle          procMgrHandleClient1;
UInt16                  remoteIdClient;


/*
 *  ======== GetSymbolIndex ========
 */
Int GetSymbolIndex (Void)
{
    Int status = 0;

    /* Get remote function index */
    Osal_printf ("\nGetSymbolIndex: Querying server for fxnDouble() function "
                 "index \n");
    status = RcmClient_getSymbolIndex (rcmClientHandle, "fxnDouble",
                                        &fxnDoubleIdx);
    if (status < 0)
        Osal_printf ("GetSymbolIndex: Error getting fxnDouble symbol index"
                     "[0x%x]\n", status);
    else
        Osal_printf ("GetSymbolIndex: fxnDouble() symbol index [0x%x]\n",
                        fxnDoubleIdx);

    Osal_printf ("GetSymbolIndex: Querying server for fxnExit() function "
                 "index \n");
    status = RcmClient_getSymbolIndex (rcmClientHandle, "fxnExit", &fxnExitIdx);
    if (status < 0)
        Osal_printf ("GetSymbolIndex: Error getting fxnExit symbol index"
                     " [0x%x]\n", status);
    else
        Osal_printf ("GetSymbolIndex: fxnExit() symbol index [0x%x]\n",
                        fxnExitIdx);

    return status;
}


/*
 *  ======== TestExec ========
 */
Int TestExec (Void)
{
    Int                  status             = 0;
    Int                  loop;
    RcmClient_Message  * rcmMsg             = NULL;
    RcmClient_Message  * returnMsg          = NULL;
    UInt                 rcmMsgSize;
    RCM_Remote_FxnArgs * fxnDoubleArgs;

    Osal_printf ("\nTestExec: Testing exec API\n");

    for (loop = 1; loop <= LOOP_COUNT; loop++) {
        /* Allocate a remote command message */
        rcmMsgSize = sizeof(RCM_Remote_FxnArgs);

        Osal_printf ("TestExec: calling RcmClient_alloc \n");
        rcmMsg = NULL;
        status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
        if (status < 0) {
            Osal_printf ("TestExec: Error allocating RCM message\n");
            goto exit;
        }

        /* Fill in the remote command message */
        rcmMsg->fxnIdx = fxnDoubleIdx;
        fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
        fxnDoubleArgs->a = loop;

        /* Execute the remote command message */
        Osal_printf ("TestExec: calling RcmClient_exec \n");
        status = RcmClient_exec (rcmClientHandle, rcmMsg, &returnMsg);
        if (status < 0) {
            Osal_printf ("TestExec: RcmClient_exec error, status = "
                         "[0x%x].\n", status);
            goto exit;
        }

        fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&(returnMsg->data));
        Osal_printf ("TestExec: called fxnDouble(%d)), result = %d\n",
            fxnDoubleArgs->a, returnMsg->result);

        /* Return message to the heap */
        Osal_printf ("TestExec: calling RcmClient_free \n");
        RcmClient_free (rcmClientHandle, returnMsg);
        returnMsg = NULL;
    }

exit:
    Osal_printf ("TestExec: Leaving TestExec()\n");
    return status;
}


/*
 *  ======== TestExecDpc ========
 */
Int TestExecDpc (Void)
{
    Int                  status             = 0;
    Int                  loop;
    RcmClient_Message  * rcmMsg             = NULL;
    RcmClient_Message  * returnMsg          = NULL;
    UInt                 rcmMsgSize;
    RCM_Remote_FxnArgs * fxnDoubleArgs;

    Osal_printf ("TestExecDpc: Testing execDpc API\n");

    for (loop = 1; loop <= LOOP_COUNT; loop++) {
        /* Allocate a remote command message */
        rcmMsgSize = sizeof(RCM_Remote_FxnArgs);

        Osal_printf ("TestExecDpc: calling RcmClient_alloc \n");
        status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
        if (status < 0) {
            Osal_printf ("TestExecDpc: Error allocating RCM message\n");
            goto exit;
        }

        /* Fill in the remote command message */
        rcmMsg->fxnIdx = fxnDoubleIdx;
        fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
        fxnDoubleArgs->a = loop;

        /* Execute the remote command message */
        Osal_printf ("TestExecDpc: calling RcmClient_execDpc \n");
        status = RcmClient_execDpc (rcmClientHandle, rcmMsg, &returnMsg);
        if (status < 0) {
            Osal_printf ("TestExecDpc: RcmClient_execDpc error. \n");
            goto exit;
        }

        fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&(returnMsg->data));
        Osal_printf ("TestExecDpc: exec (fxnDouble(%d)), result = %d",
            fxnDoubleArgs->a, returnMsg->result);

        /* Return message to the heap */
        Osal_printf ("TestExecDpc: calling RcmClient_free \n");
        RcmClient_free (rcmClientHandle, returnMsg);
    }

exit:
    Osal_printf ("TestExecDpc: Leaving TestExecDpc ()\n");
    return status;
}


/*
 *  ======== TestExecNoWait ========
 */
Int TestExecNoWait(void)
{
    Int                     status                  = 0;
    Int                     loop, job;
    UInt16                  msgIdAry[JOB_COUNT];
    RcmClient_Message     * rcmMsg                  = NULL;
    RcmClient_Message     * returnMsg               = NULL;
    UInt                    rcmMsgSize;
    RCM_Remote_FxnArgs    * fxnDoubleArgs;

    Osal_printf ("\nTestExecNoWait: Testing TestExecNoWait API\n");

    for (loop = 1; loop <= LOOP_COUNT; loop++) {
        /* Issue process jobs */
        for (job = 1; job <= JOB_COUNT; job++) {
            /* Allocate a remote command message */
            rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
            Osal_printf ("TestExecNoWait: calling RcmClient_alloc \n");
            status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
            if (status < 0) {
                Osal_printf ("TestExecNoWait: Error allocating RCM message\n");
                goto exit;
            }
            /* Fill in the remote command message */
            rcmMsg->fxnIdx = fxnDoubleIdx;
            fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
            fxnDoubleArgs->a = job;

            /* Execute the remote command message */
            Osal_printf ("TestExecNoWait: calling RcmClient_execNoWait \n");
            status = RcmClient_execNoWait (rcmClientHandle, rcmMsg,
                                            &msgIdAry[job-1]);
            if (status < 0) {
                Osal_printf ("TestExecNoWait: RcmClient_execNoWait error. \n");
                goto exit;
            }
        }

        /* Reclaim process jobs */
        for (job = 1; job <= JOB_COUNT; job++) {
            Osal_printf ("TestExecNoWait: calling RcmClient_waitUntilDone \n");
            status = RcmClient_waitUntilDone (rcmClientHandle, msgIdAry[job-1],
                                                &returnMsg);
            if (status < 0) {
                Osal_printf ("TestExecNoWait: RcmClient_waitUntilDone error\n");
                goto exit;
            }

            Osal_printf ("TestExecNoWait: msgId: %d, result = %d",
                            msgIdAry[job-1], returnMsg->result);

            /* Return message to the heap */
            Osal_printf ("TestExecNoWait: calling RcmClient_free \n");
            RcmClient_free (rcmClientHandle, returnMsg);
        }
    }

exit:
    Osal_printf ("TestExecNoWait: Leaving TestExecNoWait()\n");
    return status;
}


/*
 *  ======== ipcSetup ========
 */
Int ipcSetup (Int testCase)
{
    Int                             status          = 0;
    Char *                          procName;
    UInt16                          procId;
    ProcMgr_AttachParams            attachParams;
    ProcMgr_State                   state;
#if !defined(SYSLINK_USE_DAEMON)
    UInt32                          entryPoint      = 0;
    ProcMgr_StartParams             startParams;
    Char                            uProcId;
    HeapBufMP_Params                heapbufmpParams;
#if defined(SYSLINK_USE_LOADER)
    Char                          * imageName;
    UInt32                          fileId;
#endif
#endif
    Ipc_Config                      config;
    Int                             i;
    UInt32                          srCount;
    SharedRegion_Entry              srEntry;

    Osal_printf ("ipcSetup: Setup IPC componnets \n");

    switch(testCase) {
    case 0:
        Osal_printf ("ipcSetup: Local RCM test\n");
        remoteServerName = RCMSERVER_NAME;
        procName = MPU_PROC_NAME;
        break;
    case 1:
        Osal_printf ("ipcSetup: RCM test with RCM client and server on "
                     "Sys M3\n\n");
        remoteServerName = SYSM3_SERVER_NAME;
        procName = SYSM3_PROC_NAME;
        break;
    case 2:
        Osal_printf ("ipcSetup: RCM test with RCM client and server on "
                     "App M3\n\n");
        remoteServerName = APPM3_SERVER_NAME;
        procName = APPM3_PROC_NAME;
        break;
    case 3:
        Osal_printf ("ipcSetup: RCM test with RCM client and server on "
                     "Tesla\n\n");
        remoteServerName = DSP_SERVER_NAME;
        procName = DSP_PROC_NAME;
        break;
    default:
        Osal_printf ("ipcSetup: Please pass valid arg "
                     "(0-local, 1-Sys M3, 2-App M3, 3-Tesla) \n");
        goto exit;
        break;
    }

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in Ipc_setup [0x%x]\n", status);
        goto exit;
    }
    Osal_printf("Ipc_setup status [0x%x]\n", status);

    procId = ((testCase == 3) ? MultiProc_getId (DSP_PROC_NAME) : \
                                MultiProc_getId (SYSM3_PROC_NAME));
    remoteIdClient = MultiProc_getId (procName);

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleClient, procId);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
        goto exit;
    }
    if (status >= 0) {
        Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandleClient, &attachParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandleClient);
            Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", state);
        }
    }

    if ((status >= 0) && (testCase == 2)) {
        status = ProcMgr_open (&procMgrHandleClient1, remoteIdClient);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
            goto exit;
        }
        if (status >= 0) {
            Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandleClient1, &attachParams);
            if (status < 0) {
                Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n",
                                status);
            }
            else {
                Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n",
                                status);
                state = ProcMgr_getState (procMgrHandleClient1);
                Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
#if defined(SYSLINK_USE_LOADER)
    if (testCase == 1)
        imageName = RCM_MPUCLIENT_SYSM3ONLY_IMAGE;
    else if (testCase == 2)
        imageName = RCM_MPUCLIENT_SYSM3_IMAGE;
    else if (testCase == 3)
        imageName = RCM_MPUCLIENT_DSP_IMAGE;

    if (testCase != 0) {
        status = ProcMgr_load (procMgrHandleClient, imageName, 2, &imageName,
                                &entryPoint, &fileId, procId);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_load %s image [0x%x]\n",
                            procName, status);
            goto exit;
        }
        Osal_printf ("ipcSetup: ProcMgr_load %s image Status [0x%x]\n",
                        procName, status);
    }
#endif /* defined(SYSLINK_USE_LOADER) */
    if (testCase != 0) {
        startParams.proc_id = procId;
        status = ProcMgr_start (procMgrHandleClient, entryPoint, &startParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_start %s [0x%x]\n",
                            procName, status);
            goto exit;
        }
        Osal_printf ("ipcSetup: ProcMgr_start %s Status [0x%x]\n", procName,
                        status);
    }

    if (testCase == 2) {
#if defined(SYSLINK_USE_LOADER)
        imageName = RCM_MPUCLIENT_APPM3_IMAGE;
        uProcId = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_load (procMgrHandleClient1, imageName, 2, &imageName,
                                &entryPoint, &fileId, uProcId);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_load AppM3 image: "
                "[0x%x]\n", status);
            goto exit;
        }
        Osal_printf ("ipcSetup: AppM3: ProcMgr_load Status [0x%x]\n", status);
#endif /* defined(SYSLINK_USE_LOADER) */
        startParams.proc_id = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_start (procMgrHandleClient1, entryPoint, &startParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_start AppM3 [0x%x]\n",
                        status);
            goto exit;
        }
        Osal_printf ("ipcSetup: ProcMgr_start AppM3 Status [0x%x]\n", status);
    }
#endif /* defined(SYSLINK_USE_DAEMON) */

    srCount = SharedRegion_getNumRegions();
    Osal_printf ("SharedRegion_getNumRegions = %d\n", srCount);
    for (i = 0; i < srCount; i++) {
        status = SharedRegion_getEntry (i, &srEntry);
        Osal_printf ("SharedRegion_entry #%d: base = 0x%x len = 0x%x "
                        "ownerProcId = %d isValid = %d cacheEnable = %d "
                        "cacheLineSize = 0x%x createHeap = %d name = %s\n",
                        i, srEntry.base, srEntry.len, srEntry.ownerProcId,
                        (Int)srEntry.isValid, (Int)srEntry.cacheEnable,
                        srEntry.cacheLineSize, (Int)srEntry.createHeap,
                        srEntry.name);
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
    /* Create Heap and register it with MessageQ */
    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = 128;
        heapbufmpParams.numBlocks  = 4;
        heapbufmpParams.blockSize  = MSGSIZE;
        heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("ipcSetup: heapSize = 0x%x\n", heapSize);

        srHeap = SharedRegion_getHeap (RCM_HEAP_SR);
        if (srHeap == NULL) {
            status = MEMORYOS_E_FAIL;
            Osal_printf ("ipcSetup: SharedRegion_getHeap failed for srHeap:"
                         " [0x%x]\n", srHeap);
        }
        else {
            Osal_printf ("ipcSetup: Before Memory_alloc = 0x%x\n", srHeap);
            heapBufPtr = Memory_alloc (srHeap, heapSize, 0);
            if (heapBufPtr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("ipcSetup: Memory_alloc failed for ptr: [0x%x]\n",
                             heapBufPtr);
            }
            else {
                heapbufmpParams.name           = RCM_MSGQ_HEAPNAME;
                heapbufmpParams.sharedAddr     = heapBufPtr;
                Osal_printf ("ipcSetup: Before HeapBufMP_Create: [0x%x]\n",
                                heapBufPtr);
                heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("ipcSetup: HeapBufMP_create failed for Handle:"
                                 "[0x%x]\n", heapHandle);
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (heapHandle,
                                                    RCM_MSGQ_HEAPID);
                    if (status < 0) {
                        Osal_printf ("ipcSetup: MessageQ_registerHeap "
                                     "failed!\n");
                    }
                }
            }
        }
    }
#endif /* defined(SYSLINK_USE_DAEMON) */

exit:
    Osal_printf ("ipcSetup: Leaving ipcSetup()\n");
    return status;
}


/*
 *  ======== CreateRcmClient ========
 */
Int CreateRcmClient(Int testCase)
{
    RcmClient_Params    rcmClientParams;
    Int                 count           = 0;
    Int                 status          = 0;

    /* Size (in bytes) of RCM header including the messageQ header */
    /* RcmClient_Message member data[1] is the start of the payload */
    Osal_printf ("CreateRcmClient: Size of RCM header in bytes = %d \n",
                            RcmClient_getHeaderSize());

    /* Rcm client module setup*/
    Osal_printf ("CreateRcmClient: RCM Client module setup.\n");
    RcmClient_init ();

    /* Rcm client module params init*/
    Osal_printf ("CreateRcmClient: RCM Client module params init.\n");
    status = RcmClient_Params_init (&rcmClientParams);
    if (status < 0) {
        Osal_printf ("CreateRcmClient: Error in RCM Client instance params "
                        "init \n");
        goto exit;
    }
    Osal_printf ("CreateRcmClient: RCM Client instance params init "
                    "passed \n");

    /* Create an rcm client instance */
    Osal_printf ("CreateRcmClient: Creating RcmClient instance \n");
    rcmClientParams.callbackNotification = 0; /* disable asynchronous exec */
    rcmClientParams.heapId = RCM_MSGQ_HEAPID;

    while ((rcmClientHandle == NULL) && (count++ < MAX_CREATE_ATTEMPTS)) {
        status = RcmClient_create (remoteServerName, &rcmClientParams,
                                    &rcmClientHandle);
        if (status < 0) {
            if (status == RcmClient_E_SERVERNOTFOUND) {
                Osal_printf ("CreateRcmClient: Unable to open remote"
                                "server %d time \n", count);
            }
            else {
                Osal_printf ("CreateRcmClient: Error in RCM Client "
                                "create \n");
                goto exit;
            }
        }
    }

    if (MAX_CREATE_ATTEMPTS <= count) {
        Osal_printf ("CreateRcmClient: Timeout... could not connect with"
                     "remote server\n");
    }
    else {
        Osal_printf ("CreateRcmClient: RCM Client create passed \n");
    }

exit:
    Osal_printf ("CreateRcmClient: Leaving CreateRcmClient() \n");
    return status;
}


/*
 *  ======== RcmClientCleanup ========
 */
Int RcmClientCleanup (Int testCase)
{
    Int                  status             = 0;
    RcmClient_Message  * rcmMsg             = NULL;
    RcmClient_Message  * returnMsg          = NULL;
    UInt                 rcmMsgSize;
    RCM_Remote_FxnArgs * fxnExitArgs;
#if !defined(SYSLINK_USE_DAEMON)
    ProcMgr_StopParams   stopParams;
#endif

    Osal_printf ("\nRcmClientCleanup: Entering RcmClientCleanup()\n");

    /* Send terminate message */
    /* Allocate a remote command message */
    rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
    status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
    if (status < 0) {
        Osal_printf ("RcmClientCleanup: Error allocating RCM message\n");
        goto exit;
    }

    /* Fill in the remote command message */
    rcmMsg->fxnIdx = fxnExitIdx;
    fxnExitArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
    fxnExitArgs->a = 0xFFFF;

#if !defined(SYSLINK_USE_DAEMON)
    /* Execute the remote command message */
    Osal_printf ("RcmClientCleanup: calling RcmClient_execDpc \n");
    status = RcmClient_execDpc (rcmClientHandle, rcmMsg, &returnMsg);
    if (status < 0) {
        Osal_printf ("RcmClientCleanup: RcmClient_execDpc error [0x%x]\n",
                        status);
        goto exit;
    }
#endif /* !defined(SYSLINK_USE_DAEMON) */

    /* Return message to the heap */
    Osal_printf ("RcmClientCleanup: Calling RcmClient_free \n");
    RcmClient_free (rcmClientHandle, returnMsg);

    /* Delete the rcm client */
    Osal_printf ("RcmClientCleanup: Delete RCM client instance \n");
    status = RcmClient_delete (&rcmClientHandle);
    if (status < 0)
        Osal_printf ("RcmClientCleanup: Error in RCM Client instance delete"
                     " [0x%x]\n", status);
    else
        Osal_printf ("RcmClientCleanup: RcmClient_delete status: [0x%x]\n",
                        status);

    /* Rcm client module destroy */
    Osal_printf ("RcmClientCleanup: Destroy RCM client module \n");
    RcmClient_exit ();

    /* Finalize modules */
#if !defined(SYSLINK_USE_DAEMON)    // Do not call ProcMgr_stop if using daemon
    status = MessageQ_unregisterHeap (RCM_MSGQ_HEAPID);
    if (status < 0)
        Osal_printf ("RcmClientCleanup: Error in MessageQ_unregisterHeap"
                     " [0x%x]\n", status);
    else
        Osal_printf ("RcmClientCleanup: MessageQ_unregisterHeap status:"
                     " [0x%x]\n", status);

    if (heapHandle) {
        status = HeapBufMP_delete (&heapHandle);
        if (status < 0)
            Osal_printf ("RcmClientCleanup: Error in HeapBufMP_delete [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmClientCleanup: HeapBufMP_delete status: [0x%x]\n",
                            status);
    }

    if (heapBufPtr) {
        Memory_free (srHeap, heapBufPtr, heapSize);
    }

    stopParams.proc_id = remoteIdClient;
    if (testCase == 2) {
        status = ProcMgr_stop(procMgrHandleClient1, &stopParams);
        if (status < 0)
            Osal_printf ("RcmClientCleanup: Error in ProcMgr_stop [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmClientCleanup: ProcMgr_stop status: [0x%x]\n",
                            status);
        stopParams.proc_id = MultiProc_getId (SYSM3_PROC_NAME);
    }

    status = ProcMgr_stop(procMgrHandleClient, &stopParams);
    if (status < 0)
        Osal_printf ("RcmClientCleanup: Error in ProcMgr_stop [0x%x]\n",
                        status);
    else
        Osal_printf ("RcmClientCleanup: ProcMgr_stop status: [0x%x]\n",
                        status);
#endif

    if (testCase == 2) {
        status =  ProcMgr_detach (procMgrHandleClient1);
        Osal_printf ("RcmClientCleanup: ProcMgr_detach status [0x%x]\n",
                        status);

        status = ProcMgr_close (&procMgrHandleClient1);
        if (status < 0)
            Osal_printf ("RcmClientCleanup: Error in ProcMgr_close [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmClientCleanup: ProcMgr_close status: [0x%x]\n",
                            status);
    }

    status =  ProcMgr_detach (procMgrHandleClient);
    Osal_printf ("RcmClientCleanup: ProcMgr_detach status [0x%x]\n", status);

    status = ProcMgr_close (&procMgrHandleClient);
    if (status < 0)
        Osal_printf ("RcmClientCleanup: Error in ProcMgr_close [0x%x]\n",
                        status);
    else
        Osal_printf ("RcmClientCleanup: ProcMgr_close status: [0x%x]\n",
                        status);

    status = Ipc_destroy ();
    if (status < 0)
        Osal_printf("RcmClientCleanup: Error in Ipc_destroy [0x%x]\n", status);
    else
        Osal_printf("RcmClientCleanup: Ipc_destroy status: [0x%x]\n", status);

exit:
    Osal_printf ("RcmClientCleanup: Leaving RcmClientCleanup()\n");
    return status;
}


/*
 *  ======== MpuRcmClientTest ========
 */
Int MpuRcmClientTest(Int testCase)
{
    Int status = 0;

    Osal_printf ("MpuRcmClientTest: Testing RCM Client on MPU\n");

    status = ipcSetup (testCase);
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: ipcSetup failed, status [0x%x]\n");
        goto exit;
    }

    status = CreateRcmClient (testCase);
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: Error in creating RcmClient \n");
        goto exit;
    }

    status = GetSymbolIndex ();
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: Error in GetSymbolIndex \n");
        goto exit;
    }

    status = TestExec ();
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: Error in TestExec \n");
        goto exit;
    }

    /* status = TestExecDpc ();
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: Error in TestExecDpc \n");
        goto exit;
    }

    status = TestExecNoWait ();
    if (status < 0) {
        Osal_printf ("MpuRcmClientTest: Error in TestExecNoWait \n");
        goto exit;
    } */

    status = RcmClientCleanup (testCase);
    if (status < 0)
        Osal_printf ("MpuRcmClientTest: Error in RcmClientCleanup \n");

exit:
    Osal_printf ("MpuRcmClientTest: Leaving MpuRcmClientTest()\n");
    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
