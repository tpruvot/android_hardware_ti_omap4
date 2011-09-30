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
/*==============================================================================
 *  @file   Interm3.c
 *
 *  @brief  Utility to run InterM3 test cases from A9.
 *
 *  ============================================================================
 */

/* Linux headers */
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

/* Standard headers */
#include <Std.h>
#include <errbase.h>
#include <ipctypes.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <String.h>
#include <Memory.h>
#include <OsalPrint.h>

/* Module level headers */
#include <ti/ipc/Notify.h>
#include <IpcUsr.h>
#include <ti/ipc/MultiProc.h>

#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MessageQ.h>

/* Application header */
#include <notifyxfer_os.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/* Defines for the default HeapBufMP being configured in the System */
#define RCM_MSGQ_HEAPNAME       "Heap0"
#define RCM_MSGQ_HEAP_BLOCKS    4
#define RCM_MSGQ_HEAP_ALIGN     128
#define RCM_MSGQ_MSGSIZE        256
#define RCM_MSGQ_HEAP_SR        0
#define RCM_MSGQ_HEAPID         0

#define INTERM3_EVENT           5
#define INTERM3_EVENT_END       6

ProcMgr_Handle                  procMgrHandleSysM3;
ProcMgr_Handle                  procMgrHandleAppM3;
Bool                            appM3Client         = FALSE;
UInt16                          remoteIdSysM3;
UInt16                          remoteIdAppM3;
HeapBufMP_Handle                heapHandle          = NULL;
SizeT                           heapSize            = 0;
Ptr                             heapBufPtr          = NULL;
IHeap_Handle                    srHeap              = NULL;

UInt16                          testNo;
UInt16                          param1;
UInt16                          param2;
Int16                           testStatus;
UInt16                          sysReady;
UInt16                          appReady;
PVOID                           eventSys;
PVOID                           eventApp;

UInt16                          iloop               = 0;

struct eventArg {
    UInt32 arg1;
    UInt32 arg2;
};

typedef struct InterM3_semObj_tag {
    sem_t  sem;
} InterM3_semObj;

struct eventArg eventCbckArgSys;
struct eventArg eventCbckArgApp;


Int InterM3Test_createSem (OUT PVOID * semPtr)
{
    Int                 status   = 0;
    InterM3_semObj    * semObj;
    Int                 osStatus;

    semObj = malloc (sizeof (InterM3_semObj));
    if (semObj != NULL) {
        osStatus = sem_init (&(semObj->sem), 0, 0);
        if (osStatus < 0) {
            status = -1;
        }
        else {
            *semPtr = (PVOID) semObj;
        }
    }
    else {
        *semPtr = NULL;
        status = -1;
    }

    return status;
}

Void InterM3Test_deleteSem (IN PVOID * semHandle)
{
    Int                  status   = 0;
    InterM3_semObj     * semObj   = (InterM3_semObj *)semHandle;
    Int                  osStatus;

    osStatus = sem_destroy (&(semObj->sem));
    if (osStatus) {
        status = -1;
    }
    else {
       free (semObj);
    }
}

Int InterM3Test_postSem (IN PVOID semHandle)
{
    Int                     status   = 0;
    InterM3_semObj        * semObj   = semHandle;
    Int                     osStatus;

    osStatus = sem_post (&(semObj->sem));
    if (osStatus < 0) {
        status = -1;
    }

    return status;
}

Int InterM3Test_waitSem (IN PVOID semHandle)
{
    Int                  status = 0;
    InterM3_semObj     * semObj = semHandle;
    Int                  osStatus;

    osStatus = sem_wait (&(semObj->sem));
    if (osStatus < 0) {
        status = Notify_E_FAIL;
    }

    return status;
}

Void InterM3Test_callbackSys (IN     Processor_Id procId,
    IN     UInt16       lineId,
    IN     UInt32       eventNo,
    IN OPT Void *       arg,
    IN OPT UInt32       payload)
{
    (Void) eventNo;
    (Void) procId ;
    (Void) payload;
    InterM3Test_postSem (eventSys);
    testStatus = payload;
}

Void InterM3Test_callbackApp (IN     Processor_Id procId,
    IN     UInt16       lineId,
    IN     UInt32       eventNo,
    IN OPT Void *       arg,
    IN OPT UInt32       payload)
{
    (Void) eventNo;
    (Void) procId ;
    (Void) payload;

    InterM3Test_postSem (eventApp);
    testStatus = payload;
}

Int InterM3Test_sendEvent (Int ev_msg, Processor_Id proc, String print_msg)
{
    Int status;

    Osal_printf ("Txed Event:%s:Core:%d:%d", print_msg, proc, ev_msg);
    do {
        status = Notify_sendEvent (proc, 0, INTERM3_EVENT, ev_msg, FALSE);
    } while (status != 0);
    Osal_printf (":: SUCCESS\n");

    return (status);
}

Int InterM3Test_getEvent (Processor_Id proc)
{
    Int status;

    if (proc == MultiProc_getId ("SysM3"))
        status = InterM3Test_waitSem (eventSys);
    else
        status = InterM3Test_waitSem (eventApp);

    if (status != 0) {
        Osal_printf ("Error after send event, status %d\n", status);
        return status;
    }

    if (proc == MultiProc_getId ("SysM3"))
        Osal_printf ("Rxed from Sys :%d \n", testStatus);
    else
        Osal_printf ("Rxed from App :%d \n", testStatus);

    return (testStatus);
}

/*
 *  ======== InterM3Test ========
 */
Int InterM3Test (Void)
{
    Int             status;
    Processor_Id    procIdSys;
    Processor_Id    procIdApp;

    testStatus = 0;
    procIdSys = MultiProc_getId ("SysM3");
    procIdApp = MultiProc_getId ("AppM3");

    /* Register events for passing params to both cores */
    status = Notify_registerEvent (procIdSys,0, INTERM3_EVENT,
            (Notify_FnNotifyCbck) InterM3Test_callbackSys,
            (Void *)&eventCbckArgSys);
    if (status < 0) {
        Osal_printf ("Error in Notify_registerEvent %d [0x%x]\n", INTERM3_EVENT,
                status);
        return status;
    }
    status = Notify_registerEvent (procIdApp,0, INTERM3_EVENT,
            (Notify_FnNotifyCbck) InterM3Test_callbackApp,
            (Void *)&eventCbckArgApp);
    if (status < 0) {
        Osal_printf ("Error in Notify_registerEvent %d [0x%x]\n", INTERM3_EVENT,
                status);
        return status;
    }

    /* Create semaphore for callback functions */
    status = InterM3Test_createSem (&eventSys);
    if (status < 0) {
        Osal_printf ("Error in InterM3Test_createSem [0x%x]\n", status);
        return status;
    }
    status = InterM3Test_createSem (&eventApp);
    if (status < 0) {
        Osal_printf ("Error in InterM3Test_createSem [0x%x]\n", status);
        return status;
    }

    if (InterM3Test_sendEvent (testNo, procIdSys, "TestNo"))
        goto t_end;
    if (InterM3Test_getEvent (procIdSys))
        goto t_end;

    if (InterM3Test_sendEvent (testNo, procIdApp, "TestNo"))
        goto t_end;
    if (InterM3Test_getEvent (procIdApp))
        goto t_end;

    if (InterM3Test_sendEvent (param1, procIdSys, "Param1"))
        goto t_end;
    if (InterM3Test_getEvent (procIdSys))
        goto t_end;

    if (InterM3Test_sendEvent (param1, procIdApp, "Param1"))
        goto t_end;
    if (InterM3Test_getEvent (procIdApp))
        goto t_end;

    if (InterM3Test_sendEvent (param2, procIdSys, "Param2"))
        goto t_end;
    if (InterM3Test_getEvent (procIdSys))
        goto t_end;

    if (InterM3Test_sendEvent (param2, procIdApp, "Param2"))
        goto t_end;
    if (InterM3Test_getEvent (procIdApp))
        goto t_end;

    if (InterM3Test_sendEvent (0, procIdApp, "Start Test"))
        goto t_end;
    if (InterM3Test_sendEvent (0, procIdSys, "Start Test"))
        goto t_end;

    if (InterM3Test_getEvent (procIdSys))
        goto t_end;
    if (InterM3Test_getEvent (procIdApp))
        goto t_end;

t_end:
    if (InterM3Test_sendEvent (0xDEAD, procIdApp, "End Test"))
        goto t_end;
    if (InterM3Test_getEvent (procIdApp))
        goto t_end;

    if (InterM3Test_sendEvent (0xDEAD, procIdSys, "End Test"))
        goto t_end;
    if (InterM3Test_getEvent (procIdSys))
        goto t_end;

    /* Unregister events for passing params to both cores */
    status = Notify_unregisterEvent (procIdSys, 0, INTERM3_EVENT,
            (Notify_FnNotifyCbck) InterM3Test_callbackSys,
            (UArg)&eventCbckArgSys);

    if (status < 0) {
        Osal_printf ("Error in Notify_unregisterEvent %d [0x%x]\n",
                        INTERM3_EVENT, status);
        return status;
    }

    status = Notify_unregisterEvent (procIdApp, 0, INTERM3_EVENT,
            (Notify_FnNotifyCbck) InterM3Test_callbackApp,
            (UArg)&eventCbckArgApp);
    if (status < 0) {
        Osal_printf ("Error in Notify_unegisterEvent %d [0x%x]\n",
                        INTERM3_EVENT, status);
        return status;
    }

    InterM3Test_deleteSem (eventSys);
    InterM3Test_deleteSem (eventApp);

    return (testStatus);
}

/*
 *  ======== ipcCleanup ========
 */
Void ipcCleanup()
{
    ProcMgr_StopParams stopParams;
    Int                status = 0;

    /* Cleanup the default HeapBufMP registered with MessageQ */
    status = MessageQ_unregisterHeap (RCM_MSGQ_HEAPID);
    if (status < 0) {
        Osal_printf ("Error in MessageQ_unregisterHeap [0x%x]\n", status);
        return;
    }

    if (heapHandle) {
        status = HeapBufMP_delete (&heapHandle);
        if (status < 0) {
            Osal_printf ("Error in HeapBufMP_delete [0x%x]\n", status);
            return;
        }
    }

    if (heapBufPtr) {
        Memory_free (srHeap, heapBufPtr, heapSize);
    }

    if (appM3Client) {
        stopParams.proc_id = remoteIdAppM3;
        status = ProcMgr_stop (procMgrHandleAppM3, &stopParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                    stopParams.proc_id, status);
            return;
        }
    }

    stopParams.proc_id = remoteIdSysM3;
    status = ProcMgr_stop (procMgrHandleSysM3, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                stopParams.proc_id, status);
        return;
    }

    status = ProcMgr_detach (procMgrHandleAppM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach(AppM3): status = 0x%x\n", status);
        return;
    }

    status = ProcMgr_close (&procMgrHandleAppM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close(AppM3): status = 0x%x\n", status);
        return;
    }

    status = ProcMgr_detach (procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach(SysM3): status = 0x%x\n", status);
        return;
    }

    status = ProcMgr_close (&procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close(SysM3): status = 0x%x\n", status);
        return;
    }

    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Error in Ipc_destroy: status = 0x%x\n", status);
        return;
    }

    Osal_printf ("Done cleaning up ipc!\n");
}


/*
 *  ======== ipcSetup ========
 */
Int ipcSetup (Char * sysM3ImageName, Char * appM3ImageName)
{
    Ipc_Config                      config;
    ProcMgr_StopParams              stopParams;
    ProcMgr_StartParams             startParams;
    UInt32                          entryPoint = 0;
    UInt16                          procId;
    UInt32                          fileId;
    Int                             status = 0;
    ProcMgr_AttachParams            attachParams;
    ProcMgr_State                   state;
    HeapBufMP_Params                heapbufmpParams;
    Int                             i;
    UInt32                          srCount;
    SharedRegion_Entry              srEntry;

    if (appM3ImageName != NULL)
        appM3Client = TRUE;
    else
        appM3Client = FALSE;

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
        goto exit;
    }

    /* Get MultiProc IDs by name. */
    remoteIdSysM3 = MultiProc_getId ("SysM3");
    Osal_printf ("MultiProc_getId remoteId: [0x%x]\n", remoteIdSysM3);
    remoteIdAppM3 = MultiProc_getId ("AppM3");
    Osal_printf ("MultiProc_getId remoteId: [0x%x]\n", remoteIdAppM3);
    procId = remoteIdSysM3;
    Osal_printf ("MultiProc_getId procId: [0x%x]\n", procId);

    printf("RCM procId= %d\n", procId);
    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleSysM3, procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
        goto exit_ipc_destroy;
    }
    else {
        Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandleSysM3, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
            goto exit_procmgr_close_sysm3;
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandleSysM3);
            Osal_printf ("After attach: ProcMgr_getState\n"
                    "    state [0x%x]\n", status);
        }
    }

    if (status >= 0 && appM3Client) {
        procId = remoteIdAppM3;
        Osal_printf ("MultiProc_getId procId: [0x%x]\n", procId);

        /* Open a handle to the ProcMgr instance. */
        status = ProcMgr_open (&procMgrHandleAppM3, procId);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
            goto exit_procmgr_detach_sysm3;
        }
        else {
            Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandleAppM3, &attachParams);
            if (status < 0) {
                Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
                goto exit_procmgr_close_appm3;
            }
            else {
                Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
                state = ProcMgr_getState (procMgrHandleAppM3);
                Osal_printf ("After attach: ProcMgr_getState\n"
                        "    state [0x%x]\n", status);
            }
        }
    }

    Osal_printf ("SYSM3 Load: loading the SYSM3 image %s\n",
            sysM3ImageName);

    status = ProcMgr_load (procMgrHandleSysM3, sysM3ImageName, 2,
                           &sysM3ImageName, &entryPoint, &fileId,
                           remoteIdSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_load, status [0x%x]\n", status);
        goto exit_procmgr_detach_appm3;
    }
    startParams.proc_id = remoteIdSysM3;
    Osal_printf ("Starting ProcMgr for procID = %d\n", startParams.proc_id);
    status  = ProcMgr_start (procMgrHandleSysM3, entryPoint, &startParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_start, status [0x%x]\n", status);
        goto exit_procmgr_detach_appm3;
    }

    if (appM3Client) {
        Osal_printf ("APPM3 Load: loading the APPM3 image %s\n",
                appM3ImageName);
        status = ProcMgr_load (procMgrHandleAppM3, appM3ImageName, 2,
                               &appM3ImageName, &entryPoint, &fileId,
                               remoteIdAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_load, status [0x%x]\n", status);
            goto exit_procmgr_stop_sysm3;
        }
        startParams.proc_id = remoteIdAppM3;
        Osal_printf ("Starting ProcMgr for procID = %d\n", startParams.proc_id);
        status  = ProcMgr_start (procMgrHandleAppM3, entryPoint,
                &startParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_start, status [0x%x]\n", status);
            goto exit_procmgr_stop_sysm3;
        }
    }

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

    /* Create the heap to be used by RCM and register it with MessageQ */
    /* TODO: Do this dynamically by reading from the IPC config from the
     *       baseimage using Ipc_readConfig() */
    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = RCM_MSGQ_HEAP_ALIGN;
        heapbufmpParams.numBlocks  = RCM_MSGQ_HEAP_BLOCKS;
        heapbufmpParams.blockSize  = RCM_MSGQ_MSGSIZE;
        heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize = 0x%x\n", heapSize);

        srHeap = SharedRegion_getHeap (RCM_MSGQ_HEAP_SR);
        if (srHeap == NULL) {
            status = MEMORYOS_E_FAIL;
            Osal_printf ("SharedRegion_getHeap failed for srHeap:"
                    " [0x%x]\n", srHeap);
            goto exit_procmgr_stop_appm3;
        }
        else {
            Osal_printf ("Before Memory_alloc = 0x%x\n", srHeap);
            heapBufPtr = Memory_alloc (srHeap, heapSize, 0);
            if (heapBufPtr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("Memory_alloc failed for ptr: [0x%x]\n",
                        heapBufPtr);
                goto exit_procmgr_stop_appm3;
            }
            else {
                heapbufmpParams.name           = RCM_MSGQ_HEAPNAME;
                heapbufmpParams.sharedAddr     = heapBufPtr;
                Osal_printf ("Before HeapBufMP_Create: [0x%x]\n", heapBufPtr);
                heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("HeapBufMP_create failed for Handle:"
                            "[0x%x]\n", heapHandle);
                    goto exit_procmgr_stop_appm3;
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (heapHandle,
                            RCM_MSGQ_HEAPID);
                    if (status < 0) {
                        Osal_printf ("MessageQ_registerHeap failed!\n");
                        goto exit_procmgr_stop_appm3;
                    }
                }
            }
        }
    }

    Osal_printf ("=== SysLink-IPC setup completed successfully!===\n");
    return 0;

exit_procmgr_stop_appm3:
    if (appM3Client) {
        stopParams.proc_id = remoteIdAppM3;
        status = ProcMgr_stop (procMgrHandleAppM3, &stopParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                    stopParams.proc_id, status);
        }
    }

exit_procmgr_stop_sysm3:
    stopParams.proc_id = remoteIdSysM3;
    status = ProcMgr_stop (procMgrHandleSysM3, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                stopParams.proc_id, status);
    }

exit_procmgr_detach_appm3:
    if (appM3Client) {
        status = ProcMgr_detach (procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_detach(AppM3): status = 0x%x\n", status);
        }
    }

exit_procmgr_close_appm3:
    if (appM3Client) {
        status = ProcMgr_close (&procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_close: status = 0x%x\n", status);
        }
    }

exit_procmgr_detach_sysm3:
    status = ProcMgr_detach (procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach(SysM3): status = 0x%x\n", status);
    }

exit_procmgr_close_sysm3:
    status = ProcMgr_close (&procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close: status = 0x%x\n", status);
    }

exit_ipc_destroy:
    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Error in Ipc_destroy: status = 0x%x\n", status);
    }

exit:
    return (-1);
}


Int main (Int argc, Char * argv [])
{
    Int     status;
    FILE  * fp;
    Bool    calledIpcSetup = false;

    /* Determine args */
    switch(argc) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        default:
            status = -1;
            Osal_printf ("Invalid arguments.  Usage:\n");
            Osal_printf ("./interM3.out <SysM3-image> <AppM3-image> <Test-No> "
                         "<param1> <param2> \n");
            break;
        case 5:
            param2 = 1;
            Osal_printf ("Param2 defaults to 1\n");
        case 6:
            fp = fopen (argv[1], "rb");
            if (fp != NULL) {
                fclose (fp);
                fp = fopen (argv[2], "rb");
                if (fp != NULL) {
                    fclose (fp);
                    status = ipcSetup (argv[1], argv[2]);
                    calledIpcSetup = true;
                }
                else {
                    Osal_printf ("File %s could not be opened.\n", argv[2]);
                }
            }
            else {
                Osal_printf ("File %s could not be opened.\n", argv[1]);
            }
            break;
    }

    if (calledIpcSetup) {
        if (status < 0) {
            Osal_printf ("ipcSetup failed!\n");
            return (-1); /* Quit if there was a setup error */
        }
        else {
            Osal_printf ("ipcSetup succeeded!\n");

            testNo = param1 = param2 = 0;
            testNo = atoi (argv[3]);
            param1 = atoi (argv[4]);
            if (argc > 5)
                param2 = atoi (argv[5]);
            Osal_printf ("InterM3 test No:%d %d %d\n",testNo, param1, param2);

            status = InterM3Test ();

            /* IPC_Cleanup function*/
            ipcCleanup ();

            Osal_printf ("test_case_status=%d\n", status);
        }
    }

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
