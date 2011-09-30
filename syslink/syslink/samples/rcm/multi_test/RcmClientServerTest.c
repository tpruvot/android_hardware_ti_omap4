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
 *  @file   RcmClientServerTest.c
 *
 *  @brief  OS-specific sample application framework for RCM module
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
#include <pthread.h>
#include <semaphore.h>

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
#include <RcmServer.h>

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
#define RCM_CLNTSRVR_SYSM3ONLY_IMAGE  "./RCMSrvClnt_MPUSYS_Test_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_CLNTSRVR_SYSM3_IMAGE      "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_CLNTSRVR_APPM3_IMAGE      "./RCMSrvClnt_MPUAPP_Test_Core1.xem3"

/*!
 *  @brief  Name of the Tesla baseImage to be used for sample execution with
 *          Tesla
 */
#define RCM_CLNTSRVR_DSP_IMAGE        "./RCMSrvClnt_Dsp_Test.xe64T"


/* RCM client test definitions */
typedef struct {
    Int a;
} RCM_Remote_FxnArgs;

/* RCM server test definitions */
typedef struct {
    Int a;
} RCM_Remote_FxnDoubleArgs;

RcmClient_Handle        rcmClientHandle	    = NULL;
pthread_t               clientThread;       /* client thread object */
UInt                    fxnDoubleIdx;
UInt                    fxnExitIdx;
sem_t                   clientThreadWait;
RcmServer_Handle        rcmServerHandle;
pthread_t               serverThread;       /* server thread object */
sem_t                   serverThreadSync;
sem_t                   serverThreadWait;

/* Common definitions */
Char                  * remoteServerName;
#if !defined(SYSLINK_USE_DAEMON)
HeapBufMP_Handle        heapHandle          = NULL;
SizeT                   heapSize            = 0;
Ptr                     heapBufPtr          = NULL;
IHeap_Handle            srHeap              = NULL;
#endif
ProcMgr_Handle          procMgrHandle;
ProcMgr_Handle          procMgrHandle1;
UInt16                  remoteId;

/*
 *  ======== fxnDouble ========
 */
Int32 fxnDouble (UInt32 dataSize, UInt32 *data)
{
    RCM_Remote_FxnDoubleArgs * args;
    Int                        a;
    Int                        result;

    args = (RCM_Remote_FxnDoubleArgs *)data;
    a = args->a;
    result = a * 2;

    Osal_printf ("Executed Remote Function fxnDouble, result = %d\n", result);

    return result;
}

/*
 *  ======== fxnExit ========
 */
Int32 fxnExit (UInt32 dataSize, UInt32 *data)
{
    Int status = 0;

    Osal_printf ("Executing Remote Function fxnExit \n");

    sem_post (&serverThreadSync);

    return status;
}


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

    Osal_printf ("\nTesting exec API\n");

    for (loop = 1; loop <= LOOP_COUNT; loop++) {
        /* Allocate a remote command message */
        rcmMsgSize = sizeof(RCM_Remote_FxnArgs);

        Osal_printf ("TestExec: calling RcmClient_alloc \n");
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
            Osal_printf ("TestExec: RcmClient_exec error. \n");
            goto exit;
        }

        fxnDoubleArgs = (RCM_Remote_FxnArgs *)(&(returnMsg->data));
        Osal_printf ("TestExec: exec (fxnDouble(%d)), result = %d\n",
            fxnDoubleArgs->a, returnMsg->result);

        /* Return message to the heap */
        Osal_printf ("TestExec: calling RcmClient_free \n");
        RcmClient_free (rcmClientHandle, returnMsg);
    }

exit:
    Osal_printf ("TestExec: Leaving Testexec ()\n");
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
                     "(1-SysM3, 2-AppM3, 3-Tesla)\n\n");
        goto exit;
        break;
    }

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in Ipc_setup [0x%x]\n", status);
        goto exit;
    }
    Osal_printf("ipcSetup: Ipc_setup status [0x%x]\n", status);

    procId = ((testCase == 3) ? MultiProc_getId (DSP_PROC_NAME) : \
                                MultiProc_getId (SYSM3_PROC_NAME));
    remoteId = MultiProc_getId (procName);

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandle, procId);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
        goto exit;
    }
    if (status >= 0) {
        Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandle);
            Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", state);
        }
    }

    if ((status >= 0) && (testCase == 2)) {
        status = ProcMgr_open (&procMgrHandle1, remoteId);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
            goto exit;
        }
        if (status >= 0) {
            Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandle1, &attachParams);
            if (status < 0) {
                Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n",
                                status);
            }
            else {
                Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n",
                                status);
                state = ProcMgr_getState (procMgrHandle1);
                Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
#if defined(SYSLINK_USE_LOADER)
    if (testCase == 1)
        imageName = RCM_CLNTSRVR_SYSM3ONLY_IMAGE;
    else if (testCase == 2)
        imageName = RCM_CLNTSRVR_SYSM3_IMAGE;
    else if (testCase == 3)
        imageName = RCM_CLNTSRVR_DSP_IMAGE;

    status = ProcMgr_load (procMgrHandle, imageName, 2, &imageName,
                            &entryPoint, &fileId, procId);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_load %s image: [0x%x]\n",
                        procName, status);
        goto exit;
    }
    Osal_printf ("ipcSetup: ProcMgr_load %s image Status [0x%x]\n", procName,
                    status);
#endif /* defined(SYSLINK_USE_LOADER) */
    startParams.proc_id = procId;
    status = ProcMgr_start (procMgrHandle, entryPoint, &startParams);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_start %s [0x%x]\n", procName,
                        status);
        goto exit;
    }
    Osal_printf ("ipcSetup: ProcMgr_start %s Status [0x%x]\n", procName,
                    status);

    if (testCase == 2) {
#if defined(SYSLINK_USE_LOADER)
        imageName = RCM_CLNTSRVR_APPM3_IMAGE;
        uProcId = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_load (procMgrHandle1, imageName, 2, &imageName,
                                &entryPoint, &fileId, uProcId);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_load AppM3 image: "
                "[0x%x]\n", status);
            goto exit;
        }
        Osal_printf ("ipcSetup: AppM3: ProcMgr_load Status [0x%x]\n", status);
#endif /* defined(SYSLINK_USE_LOADER) */
        startParams.proc_id = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_start (procMgrHandle1, entryPoint, &startParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_start AppM3 [0x%x]\n",
                        status);
            goto exit;
        }
        Osal_printf ("ProcMgr_start AppM3 Status [0x%x]\n", status);
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
 *  ======== RcmServerThreadFxn ========
 *     RCM server test thread function
 */
Void RcmServerThreadFxn (Void *arg)
{
    RcmServer_Params    rcmServerParams;
    UInt                fxnIdx;
    Char *              rcmServerName       = RCMSERVER_NAME;
    Int                 status              = 0;

    /* Rcm server module setup*/
    Osal_printf ("RcmServerThreadFxn: RCM Server module setup.\n");
    RcmServer_init ();

    /* Rcm server module params init*/
    Osal_printf ("RcmServerThreadFxn: RCM Server module params init.\n");
    status = RcmServer_Params_init (&rcmServerParams);
    if (status < 0) {
        Osal_printf ("RcmServerThreadFxn: Error in RCM Server instance params "
                        "init \n");
        goto exit;
    }
    Osal_printf ("RcmServerThreadFxn: RCM Server instance params init "
                   "passed \n");

    /* Create the RcmServer instance */
    Osal_printf ("RcmServerThreadFxn: Creating RcmServer instance %s.\n",
        rcmServerName);
    status = RcmServer_create (rcmServerName, &rcmServerParams,
                                &rcmServerHandle);
    if (status < 0) {
        Osal_printf ("RcmServerThreadFxn: Error in RCM Server create.\n");
        goto exit;
    }
    Osal_printf ("RcmServerThreadFxn: RCM Server Create passed \n");

    sem_init (&serverThreadSync, 0, 0);

    /* Register the remote functions */
    Osal_printf ("RcmServerThreadFxn: Registering remote function - "
                "fxnDouble\n");
    status = RcmServer_addSymbol (rcmServerHandle, "fxnDouble", fxnDouble,
                                    &fxnIdx);
    if ((status < 0) || (fxnIdx == 0xFFFFFFFF)) {
        Osal_printf ("RcmServerThreadFxn: Add symbol failed.\n");
        goto exit;
    }

    Osal_printf ("RcmServerThreadFxn: Registering remote function - "
                "fxnExit\n");
    status = RcmServer_addSymbol (rcmServerHandle, "fxnExit", fxnExit, &fxnIdx);
    if ((status < 0) || (fxnIdx == 0xFFFFFFFF)) {
        Osal_printf ("RcmServerThreadFxn: Add symbol failed.\n");
        goto exit;
    }

    Osal_printf ("RcmServerThreadFxn: Start RCM server thread \n");
    RcmServer_start (rcmServerHandle);
    Osal_printf ("RcmServerThreadFxn: RCM Server start passed \n");

    sem_wait (&serverThreadSync);

    sem_post (&serverThreadWait);

exit:
    Osal_printf ("RcmServerThreadFxn: Leaving RCM server test thread "
                    "function \n");
    return;
}


/*
 *  ======== RcmClientThreadFxn ========
 *     RCM client test thread function
 */
Void RcmClientThreadFxn (Void *arg)
{
    RcmClient_Params    rcmClientParams;
    Int                 count           = 0;
    Int                 testCase;
    Int                 status          = 0;

    /* Size (in bytes) of RCM header including the messageQ header */
    /* RcmClient_Message member data[1] is the start of the payload */
    Osal_printf ("RcmClientThreadFxn: Size of RCM header in bytes = %d \n",
                            RcmClient_getHeaderSize());

    /* Rcm client module setup*/
    Osal_printf ("RcmClientThreadFxn: RCM Client module setup.\n");
    RcmClient_init ();

    /* Rcm client module params init*/
    Osal_printf ("RcmClientThreadFxn: RCM Client module params init.\n");
    status = RcmClient_Params_init (&rcmClientParams);
    if (status < 0) {
        Osal_printf ("RcmClientThreadFxn: Error in RCM Client instance params "
                        "init \n");
        goto exit;
    }
    Osal_printf ("RcmClientThreadFxn: RCM Client instance params init "
                    "passed \n");

    /* Create an rcm client instance */
    Osal_printf ("RcmClientThreadFxn: Creating RcmClient instance \n");
    rcmClientParams.callbackNotification = 0; /* disable asynchronous exec */
    testCase = (Int)arg;
    rcmClientParams.heapId = RCM_MSGQ_HEAPID;

    while ((rcmClientHandle == NULL) && (count++ < MAX_CREATE_ATTEMPTS)) {
        status = RcmClient_create (remoteServerName, &rcmClientParams,
                                    &rcmClientHandle);
        if (status < 0) {
            if (status == RcmClient_E_SERVERNOTFOUND) {
                Osal_printf ("RcmClientThreadFxn: Unable to open remote"
                                "server %d time \n", count);
            }
            else {
                Osal_printf ("RcmClientThreadFxn: Error in RCM Client "
                                "create \n");
                goto exit;
            }
        }
    }

    if (MAX_CREATE_ATTEMPTS <= count) {
        Osal_printf ("RcmClientThreadFxn: Timeout... could not connect with"
                     "remote server\n");
    }
    else {
        Osal_printf ("RcmClientThreadFxn: RCM Client create passed \n");
    }

    status = GetSymbolIndex ();
    if (status < 0) {
        Osal_printf ("RcmClientThreadFxn: Error in GetSymbolIndex \n");
        goto exit;
    }

    status = TestExec ();
    if (status < 0) {
        Osal_printf ("RcmClientThreadFxn: Error in TestExec \n");
        goto exit;
    }

    sem_post (&clientThreadWait);

exit:
    Osal_printf ("RcmClientThreadFxn: Leaving RCM client test thread "
                    "function \n");
    return;
}


/*
 *  ======== RcmTestCleanup ========
 */
Int RcmTestCleanup (Int testCase)
{
    Int                  status             = 0;
    RcmClient_Message  * rcmMsg             = NULL;
    RcmClient_Message  * returnMsg          = NULL;
    UInt                 rcmMsgSize;
    RCM_Remote_FxnArgs * fxnExitArgs;
#if !defined(SYSLINK_USE_DAEMON)
    ProcMgr_StopParams   stopParams;
#endif

    Osal_printf ("\nRcmTestCleanup: Entering RcmTestCleanup()\n");

    /* Send terminate message */
    /* Allocate a remote command message */
    rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
    status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
    if (status < 0) {
        Osal_printf ("RcmTestCleanup: Error allocating RCM message\n");
        goto exit;
    }

    /* Fill in the remote command message */
    rcmMsg->fxnIdx = fxnExitIdx;
    fxnExitArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
    fxnExitArgs->a = 0xFFFF;

    /* Execute the remote command message */
    Osal_printf ("RcmTestCleanup: calling RcmClient_execDpc \n");
    status = RcmClient_execDpc (rcmClientHandle, rcmMsg, &returnMsg);
    if (status < 0) {
        Osal_printf ("RcmTestCleanup: RcmClient_execDpc error [0x%x]\n",
                        status);
        goto exit;
    }

    /* Return message to the heap */
    Osal_printf ("RcmTestCleanup: Calling RcmClient_free \n");
    RcmClient_free (rcmClientHandle, returnMsg);

    /* Delete the rcm client */
    Osal_printf ("RcmTestCleanup: Delete RCM client instance \n");
    status = RcmClient_delete (&rcmClientHandle);
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in RCM Client instance delete"
                     " [0x%x]\n", status);
    else
        Osal_printf ("RcmTestCleanup: RcmClient_delete status: [0x%x]\n",
                        status);

    /* Rcm client module destroy */
    Osal_printf ("RcmTestCleanup: Clean up RCM client module \n");
    RcmClient_exit ();

    /* Delete the rcm server */
    Osal_printf ("RcmTestCleanup: Delete RCM server instance \n");
    status = RcmServer_delete (&rcmServerHandle);
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in RCM Server instance delete"
                    " [0x%x]\n", status);
    else
        Osal_printf ("RcmTestCleanup: RcmServer_delete status: [0x%x]\n",
                        status);

    /* Rcm server module destroy */
    Osal_printf ("RcmTestCleanup: Clean up RCM server module \n");
    RcmServer_exit ();

    /* Finalize modules */
#if !defined(SYSLINK_USE_DAEMON)    // Do not call ProcMgr_stop if using daemon
    status = MessageQ_unregisterHeap (RCM_MSGQ_HEAPID);
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in MessageQ_unregisterHeap"
                     " [0x%x]\n", status);
    else
        Osal_printf ("RcmTestCleanup: MessageQ_unregisterHeap status:"
                     " [0x%x]\n", status);

    if (heapHandle) {
        status = HeapBufMP_delete (&heapHandle);
        if (status < 0)
            Osal_printf ("RcmTestCleanup: Error in HeapBufMP_delete [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmTestCleanup: HeapBufMP_delete status: [0x%x]\n",
                            status);
    }

    if (heapBufPtr) {
        Memory_free (srHeap, heapBufPtr, heapSize);
    }

    stopParams.proc_id = remoteId;
    if (testCase == 2) {
        status = ProcMgr_stop (procMgrHandle1, &stopParams);
        if (status < 0)
            Osal_printf ("RcmTestCleanup: Error in ProcMgr_stop [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmTestCleanup: ProcMgr_stop status: [0x%x]\n",
                            status);
        stopParams.proc_id = MultiProc_getId (SYSM3_PROC_NAME);
    }

    status = ProcMgr_stop (procMgrHandle, &stopParams);
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in ProcMgr_stop [0x%x]\n",
                        status);
    else
        Osal_printf ("RcmTestCleanup: ProcMgr_stop status: [0x%x]\n",
                        status);
#endif

    if (testCase == 2) {
        status =  ProcMgr_detach (procMgrHandle1);
        Osal_printf ("RcmTestCleanup: ProcMgr_detach status [0x%x]\n", status);

        status = ProcMgr_close (&procMgrHandle1);
        if (status < 0)
            Osal_printf ("RcmTestCleanup: Error in ProcMgr_close [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmTestCleanup: ProcMgr_close status: [0x%x]\n",
                            status);
    }

    status =  ProcMgr_detach (procMgrHandle);
    Osal_printf ("RcmTestCleanup: ProcMgr_detach status [0x%x]\n", status);

    status = ProcMgr_close (&procMgrHandle);
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in ProcMgr_close [0x%x]\n", status);
    else
        Osal_printf ("RcmTestCleanup: ProcMgr_close status: [0x%x]\n", status);

    status = Ipc_destroy ();
    if (status < 0)
        Osal_printf ("RcmTestCleanup: Error in Ipc_destroy [0x%x]\n", status);
    else
        Osal_printf ("RcmTestCleanup: Ipc_destroy status: [0x%x]\n", status);

exit:
    Osal_printf ("RcmTestCleanup: Leaving RcmTestCleanup()\n");
    return status;
}


/*
 *  ======== RunServerTestThread ========
 *     RCM server test thread function
 */
Void StartRcmTestThreads (Int testCase)
{
    sem_init (&serverThreadWait, 0, 0);
    sem_init (&clientThreadWait, 0, 0);

    /* Create the server thread */
    Osal_printf ("StartRcmTestThreads: Create server thread.\n");
    pthread_create (&serverThread, NULL, (Void *)&RcmServerThreadFxn,
                    (Void *) testCase);

    /* Create the client thread */
    Osal_printf ("StartRcmTestThreads: Create client thread.\n");
    pthread_create (&clientThread, NULL, (Void *)&RcmClientThreadFxn,
                    (Void *) testCase);

    return;
}


/*
 *  ======== RunTest ========
 */
Int RunTest (Int testCase)
{
    Int status = 0;

    Osal_printf ("RunTest: Testing RCM Server on MPU\n");

    status = ipcSetup (testCase);
    if (status < 0) {
        Osal_printf ("RunTest: ipcSetup failed, status [0x%x]\n");
        goto exit;
    }

    StartRcmTestThreads (testCase);

    /* Wait until signaled to delete the rcm server */
    Osal_printf ("RunTest: Wait for server thread completion.\n");
    sem_wait (&serverThreadWait);

    Osal_printf ("RunTest: Wait for client thread completion.\n");
    sem_wait (&clientThreadWait);

    pthread_join (serverThread, NULL);
    pthread_join (clientThread, NULL);

    status = RcmTestCleanup (testCase);
    if (status < 0)
        Osal_printf ("RunTest: Error in RcmTestCleanup \n");

exit:
    Osal_printf ("RunTest: Leaving RunTest()\n");
    return status;
}


Void printUsage (Void)
{
    Osal_printf ("Usage: ./rcm_multitest.out <Test#>\n:\n");
    Osal_printf ("\t./rcm_multitest.out 1 : MPU <--> SysM3 Sample\n");
    Osal_printf ("\t./rcm_multitest.out 2 : MPU <--> AppM3 Sample\n");
    Osal_printf ("\t./rcm_multitest.out 2 : MPU <--> Tesla Sample\n");

    return;
}


/*
 *  ======== main ========
 */
Int main (Int argc, Char * argv [])
{
    Int status = -1;
    Int testNo;

    Osal_printf ("\nmain: == RCM Client and Server Sample ==\n");

    if (argc < 2) {
        printUsage ();
        goto exit;
    }

    testNo = atoi (argv[1]);
    if (testNo < 1 || testNo > 3) {
        printUsage ();
        goto exit;
    }

    /* Run RCM client and server test */
    Osal_printf ("main: RCM client and server sample invoked\n");
    status = RunTest (testNo);
    if (status < 0)
        Osal_printf ("main: Error in RCM Client & Server sample \n");

exit:
    Osal_printf ("\n== Sample End ==\n");

    /* Trace for TITAN support */
    if (status < 0)
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");

    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
