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
 *  @file   Server.c
 *
 *  @brief  RCM Server Sample application for RCM module between MPU & Ducati
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
#define RCM_MPUSERVER_SYSM3ONLY_IMAGE  "./RCMClient_MPUSYS_Test_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_MPUSERVER_SYSM3_IMAGE      "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define RCM_MPUSERVER_APPM3_IMAGE      "./RCMClient_MPUAPP_Test_Core1.xem3"

/*!
 *  @brief  Name of the Tesla baseImage to be used for sample execution with
 *          Tesla
 */
#define RCM_MPUSERVER_DSP_IMAGE        "./RCMClient_Dsp_Test.xe64T"


/* RCM server test definitions */
typedef struct {
    Int a;
} RCM_Remote_FxnDoubleArgs;

RcmServer_Handle        rcmServerHandle;
pthread_t               thread;             /* server thread object */
sem_t                   serverThreadSync;
sem_t                   mainThreadWait;
#if !defined(SYSLINK_USE_DAEMON)
HeapBufMP_Handle        heapHandleServer    = NULL;
SizeT                   heapSizeServer      = 0;
Ptr                     heapBufPtrServer    = NULL;
IHeap_Handle            srHeapServer        = NULL;
#endif
ProcMgr_Handle          procMgrHandleServer;
ProcMgr_Handle          procMgrHandleServer1;
UInt16                  remoteIdServer;


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
 *  ======== ipcSetup1 ========
 */
Int ipcSetup1 (Int testCase)
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
        procName = SYSM3_PROC_NAME;
        break;
    case 2:
        Osal_printf ("ipcSetup: RCM test with RCM client and server on "
                     "App M3\n\n");
        procName = APPM3_PROC_NAME;
        break;
    case 3:
        Osal_printf ("ipcSetup: RCM test with RCM client and server on "
                     "Tesla\n\n");
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
    remoteIdServer = MultiProc_getId (procName);

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleServer, procId);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
        goto exit;
    }
    if (status >= 0) {
        Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandleServer, &attachParams);
        if (status < 0) {
            Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandleServer);
            Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", state);
        }
    }

    if ((status >= 0) && (testCase == 2)) {
        status = ProcMgr_open (&procMgrHandleServer1, remoteIdServer);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_open [0x%x]\n", status);
            goto exit;
        }
        if (status >= 0) {
            Osal_printf ("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandleServer1, &attachParams);
            if (status < 0) {
                Osal_printf ("ipcSetup: ProcMgr_attach failed [0x%x]\n",
                                status);
            }
            else {
                Osal_printf ("ipcSetup: ProcMgr_attach status: [0x%x]\n",
                                status);
                state = ProcMgr_getState (procMgrHandleServer1);
                Osal_printf ("ipcSetup: After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
#if defined(SYSLINK_USE_LOADER)
    if (testCase == 1)
        imageName = RCM_MPUSERVER_SYSM3ONLY_IMAGE;
    else if (testCase == 2)
        imageName = RCM_MPUSERVER_SYSM3_IMAGE;
    else if (testCase == 3)
        imageName = RCM_MPUSERVER_DSP_IMAGE;

    status = ProcMgr_load (procMgrHandleServer, imageName, 2, &imageName,
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
    status = ProcMgr_start (procMgrHandleServer, entryPoint, &startParams);
    if (status < 0) {
        Osal_printf ("ipcSetup: Error in ProcMgr_start %s [0x%x]\n", procName,
                        status);
        goto exit;
    }
    Osal_printf ("ipcSetup: ProcMgr_start %s Status [0x%x]\n", procName,
                    status);

    if (testCase == 2) {
#if defined(SYSLINK_USE_LOADER)
        imageName = RCM_MPUSERVER_APPM3_IMAGE;
        uProcId = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_load (procMgrHandleServer1, imageName, 2, &imageName,
                                &entryPoint, &fileId, uProcId);
        if (status < 0) {
            Osal_printf ("ipcSetup: Error in ProcMgr_load AppM3 image: "
                "[0x%x]\n", status);
            goto exit;
        }
        Osal_printf ("ipcSetup: AppM3: ProcMgr_load Status [0x%x]\n", status);
#endif /* defined(SYSLINK_USE_LOADER) */
        startParams.proc_id = MultiProc_getId (APPM3_PROC_NAME);
        status = ProcMgr_start (procMgrHandleServer1, entryPoint, &startParams);
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
        heapSizeServer = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("ipcSetup: heapSizeServer = 0x%x\n", heapSizeServer);

        srHeapServer = SharedRegion_getHeap (RCM_HEAP_SR);
        if (srHeapServer == NULL) {
            status = MEMORYOS_E_FAIL;
            Osal_printf ("ipcSetup: SharedRegion_getHeap failed for "
                         "srHeapServer: [0x%x]\n", srHeapServer);
        }
        else {
            Osal_printf ("ipcSetup: Before Memory_alloc = 0x%x\n", srHeapServer);
            heapBufPtrServer = Memory_alloc (srHeapServer, heapSizeServer, 0);
            if (heapBufPtrServer == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("ipcSetup: Memory_alloc failed for ptr: [0x%x]\n",
                             heapBufPtrServer);
            }
            else {
                heapbufmpParams.name           = RCM_MSGQ_HEAPNAME;
                heapbufmpParams.sharedAddr     = heapBufPtrServer;
                Osal_printf ("ipcSetup: Before HeapBufMP_Create: [0x%x]\n",
                                heapBufPtrServer);
                heapHandleServer = HeapBufMP_create (&heapbufmpParams);
                if (heapHandleServer == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("ipcSetup: HeapBufMP_create failed for Handle:"
                                 " [0x%x]\n", heapHandleServer);
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (heapHandleServer,
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

    /* Rcm server module init */
    Osal_printf ("RcmServerThreadFxn: RCM Server module init.\n");
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

    sem_post (&mainThreadWait);

exit:
    Osal_printf ("RcmServerThreadFxn: Leaving RCM server test thread "
                    "function \n");
    return;
}


/*
 *  ======== RcmServerCleanup ========
 */
Int RcmServerCleanup (Int testCase)
{
    Int                  status             = 0;
#if !defined(SYSLINK_USE_DAEMON)
    ProcMgr_StopParams   stopParams;
#endif

    Osal_printf ("\nRcmServerCleanup: Entering RcmServerCleanup()\n");
    /* Delete the rcm server */
    Osal_printf ("RcmServerCleanup: Delete RCM server instance \n");
    status = RcmServer_delete (&rcmServerHandle);
    if (status < 0)
        Osal_printf ("RcmServerCleanup: Error in RCM Server instance delete"
                    " [0x%x]\n", status);
    else
        Osal_printf ("RcmServerCleanup: RcmServer_delete status: [0x%x]\n",
                        status);

    /* Rcm server module destroy */
    Osal_printf ("RcmServerCleanup: Clean up RCM server module \n");
    RcmServer_exit ();

    /* Finalize modules */
#if !defined(SYSLINK_USE_DAEMON)    // Do not call ProcMgr_stop if using daemon
    status = MessageQ_unregisterHeap (RCM_MSGQ_HEAPID);
    if (status < 0)
        Osal_printf ("RcmServerCleanup: Error in MessageQ_unregisterHeap"
                     " [0x%x]\n", status);
    else
        Osal_printf ("RcmServerCleanup: MessageQ_unregisterHeap status:"
                     " [0x%x]\n", status);

    if (heapHandleServer) {
        status = HeapBufMP_delete (&heapHandleServer);
        if (status < 0)
            Osal_printf ("RcmServerCleanup: Error in HeapBufMP_delete [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmServerCleanup: HeapBufMP_delete status: [0x%x]\n",
                            status);
    }

    if (heapBufPtrServer) {
        Memory_free (srHeapServer, heapBufPtrServer, heapSizeServer);
    }

    stopParams.proc_id = remoteIdServer;
    if (testCase == 2) {
        status = ProcMgr_stop (procMgrHandleServer1, &stopParams);
        if (status < 0)
            Osal_printf ("RcmServerCleanup: Error in ProcMgr_stop [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmServerCleanup: ProcMgr_stop status: [0x%x]\n",
                            status);
        stopParams.proc_id = MultiProc_getId (SYSM3_PROC_NAME);
    }

    status = ProcMgr_stop (procMgrHandleServer, &stopParams);
    if (status < 0)
        Osal_printf ("RcmServerCleanup: Error in ProcMgr_stop [0x%x]\n",
                        status);
    else
        Osal_printf ("RcmServerCleanup: ProcMgr_stop status: [0x%x]\n",
                        status);
#endif

    if (testCase == 2) {
        status =  ProcMgr_detach (procMgrHandleServer1);
        Osal_printf ("RcmServerCleanup: ProcMgr_detach status [0x%x]\n",
                        status);

        status = ProcMgr_close (&procMgrHandleServer1);
        if (status < 0)
            Osal_printf ("RcmServerCleanup: Error in ProcMgr_close [0x%x]\n",
                            status);
        else
            Osal_printf ("RcmServerCleanup: ProcMgr_close status: [0x%x]\n",
                            status);
    }

    status =  ProcMgr_detach (procMgrHandleServer);
    Osal_printf ("RcmServerCleanup: ProcMgr_detach status [0x%x]\n", status);

    status = ProcMgr_close (&procMgrHandleServer);
    if (status < 0)
        Osal_printf ("RcmServerCleanup: Error in ProcMgr_close [0x%x]\n",
                        status);
    else
        Osal_printf ("RcmServerCleanup: ProcMgr_close status: [0x%x]\n",
                        status);

    status = Ipc_destroy ();
    if (status < 0)
        Osal_printf ("RcmServerCleanup: Error in Ipc_destroy [0x%x]\n", status);
    else
        Osal_printf ("RcmServerCleanup: Ipc_destroy status: [0x%x]\n", status);

    Osal_printf ("RcmServerCleanup: Leaving RcmServerCleanup()\n");
    return status;
}


/*
 *  ======== RunServerTestThread ========
 *     RCM server test thread function
 */
Void StartRcmServerTestThread (Int testCase)
{
    sem_init (&mainThreadWait, 0, 0);

    /* Create the server thread */
    Osal_printf ("StartRcmServerThread: Create server thread.\n");
    pthread_create (&thread, NULL, (Void *)&RcmServerThreadFxn,
                    (Void *) testCase);

    return;
}


/*
 *  ======== MpuRcmServerTest ========
 */
Int MpuRcmServerTest (Int testCase)
{
    Int status = 0;

    Osal_printf ("MpuRcmServerTest: Testing RCM server on MPU\n");

    status = ipcSetup1 (testCase);
    if (status < 0) {
        Osal_printf ("MpuRcmServerTest: ipcSetup failed, status [0x%x]\n");
        goto exit;
    }

    StartRcmServerTestThread (testCase);

    /* Wait until signaled to delete the rcm server */
    Osal_printf ("MpuRcmServerTest: Wait for server thread completion.\n");
    sem_wait (&mainThreadWait);

    pthread_join (thread, NULL);

    status = RcmServerCleanup (testCase);
    if (status < 0)
        Osal_printf ("MpuRcmServerTest: Error in RcmServerCleanup \n");

exit:
    Osal_printf ("MpuRcmServerTest: Leaving MpuRcmServerTest()\n");
    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
