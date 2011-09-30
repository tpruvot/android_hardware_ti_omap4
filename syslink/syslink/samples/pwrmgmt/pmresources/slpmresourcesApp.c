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
 *  @file   slpmresourcesApp.c
 *
 *  @brief  Sample application for SlpmResources module between MPU & AppM3
 *  using slpm
 *
 *  ============================================================================
 */


/* Standard headers */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>
#include <Memory.h>
#include <String.h>

/* Module level headers */
#include <IpcUsr.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/Notify.h>

/* Application header */
#include "slpmresourcesApp_config.h"


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
#define SLPMRESOURCES_SYSM3ONLY_IMAGE "./Resources_MPUSYS_Test_PM_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define SLPMRESOURCES_SYSM3_IMAGE     "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define SLPMRESOURCES_APPM3_IMAGE     "./Resources_MPUAPP_Test_PM_Core1.xem3"

/*!
 *  @brief  Number of transfers to be tested.
 */
#define  MESSAGEQAPP_NUM_TRANSFERS  23
#define  PM_SUSPEND                 0
#define  PM_RESUME                  1


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
MessageQ_Handle                slpmResources_messageQ;
HeapBufMP_Handle               slpmResources_heapHandle;
MessageQ_QueueId               slpmResources_queueId = MessageQ_INVALIDMESSAGEQ;
UInt16                         slpmResources_procId;
UInt16                         slpmResources_procId1;
UInt32                         slpmResources_curShAddr;
ProcMgr_Handle                 slpmResources_procMgrHandle;
ProcMgr_Handle                 slpmResources_procMgrHandle1;
SizeT                          slpmResources_heapSize         = 0;
Ptr                            slpmResources_ptr              = NULL;

typedef struct MsgInfo {
    MessageQ_MsgHeader  header;
    Int32               slpmError;
} MsgInfo;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for SlpmResources sample application
 *
 *  @sa
 */
Int
SlpmResources_startup (Int testNo)
{
    Int32                          status           = 0;
    Ipc_Config                     config;
    HeapBufMP_Params               heapbufmpParams;
    UInt32                         srCount;
    SharedRegion_Entry             srEntry;
    Int                            i;
    IHeap_Handle                   srHeap           = NULL;
#if !defined (SYSLINK_USE_DAEMON)
    UInt32                         entryPoint       = 0;
    ProcMgr_StartParams            startParams;
#if defined(SYSLINK_USE_LOADER)
    Char                         * imageName;
    UInt32                         fileId;
#endif /* if defined(SYSLINK_USE_LOADER) */
#endif /* if !defined(SYSLINK_USE_DAEMON) */
    ProcMgr_AttachParams           attachParams;
    ProcMgr_State                  state;

    Osal_printf ("Entered SlpmResources_startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    /* Open a handle to the SysM3 ProcMgr instance. */
    slpmResources_procId = MultiProc_getId ("SysM3");
    status = ProcMgr_open (&slpmResources_procMgrHandle, slpmResources_procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open (SysM3) [0x%x]\n", status);
    }
    else {
        Osal_printf ("ProcMgr_open (SysM3) Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (slpmResources_procMgrHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach (SysM3) failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach (SysM3) status: [0x%x]\n", status);
            state = ProcMgr_getState (slpmResources_procMgrHandle);
            Osal_printf ("After attach: ProcMgr_getState (SysM3)\n"
                         "    state [0x%x]\n", status);
        }
    }

    /* Open a handle to the AppM3 ProcMgr instance. */
    if ((status >= 0) && (testNo == 2)) {
        slpmResources_procId1 = MultiProc_getId ("AppM3");
        status = ProcMgr_open (&slpmResources_procMgrHandle1, slpmResources_procId1);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_open (AppM3) [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_open (AppM3) Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (slpmResources_procMgrHandle1, &attachParams);
            if (status < 0) {
                Osal_printf ("ProcMgr_attach (AppM3) failed [0x%x]\n", status);
            }
            else {
                Osal_printf ("ProcMgr_attach(AppM3) status: [0x%x]\n", status);
                state = ProcMgr_getState (slpmResources_procMgrHandle1);
                Osal_printf ("After attach: ProcMgr_getState (AppM3)\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
#ifdef SYSLINK_USE_LOADER
    if (status >= 0) {
        if (testNo == 1)
            imageName = SLPMRESOURCES_SYSM3ONLY_IMAGE;
        else if (testNo == 2)
            imageName = SLPMRESOURCES_SYSM3_IMAGE;

        Osal_printf ("Loading image (%s) onto Ducati with ProcId %d\n",
                        imageName, slpmResources_procId);
        status = ProcMgr_load (slpmResources_procMgrHandle, imageName, 2,
                                (String *) &imageName, &entryPoint, &fileId,
                                slpmResources_procId);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_load (SysM3) image: [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_load (SysM3) Status [0x%x]\n", status);
        }
    }
#endif /* defined(SYSLINK_USE_LOADER) */
    if (status >= 0) {
        startParams.proc_id = slpmResources_procId;
        status = ProcMgr_start (slpmResources_procMgrHandle, entryPoint,
                                &startParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_start (SysM3) [0x%x]\n", status);
        }
        else {
           Osal_printf ("ProcMgr_start (SysM3) Status [0x%x]\n", status);
        }
    }

    if ((status >= 0) && (testNo == 2)) {
#if defined(SYSLINK_USE_LOADER)
        imageName = SLPMRESOURCES_APPM3_IMAGE;
        status = ProcMgr_load (slpmResources_procMgrHandle1, imageName, 2,
                                &imageName, &entryPoint, &fileId,
                                slpmResources_procId1);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_load (AppM3) image: 0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_load (AppM3) Status [0x%x]\n", status);
        }
#endif /* defined(SYSLINK_USE_LOADER) */
        startParams.proc_id = slpmResources_procId1;
        status = ProcMgr_start (slpmResources_procMgrHandle1, entryPoint,
                                    &startParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_start (AppM3) [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_start (AppM3) Status [0x%x]\n", status);
        }
    }
#endif /* !defined(SYSLINK_USE_DAEMON) */

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
        slpmResources_heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize = 0x%x\n", slpmResources_heapSize);

        srHeap = SharedRegion_getHeap (0);
        if (srHeap == NULL) {
            status = MessageQ_E_FAIL;
            Osal_printf ("SharedRegion_getHeap failed for %d processor."
                         " srHeap: [0x%x]\n",
                         slpmResources_procId,
                         srHeap);
        }
        else {
            Osal_printf ("Before Memory_alloc = 0x%x\n", srHeap);
            slpmResources_ptr = Memory_alloc (srHeap,
                                slpmResources_heapSize,
                                0);
            if (slpmResources_ptr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("Memory_alloc failed for %d processor."
                             " ptr: [0x%x]\n",
                             slpmResources_procId,
                             slpmResources_ptr);
            }
            else {
                heapbufmpParams.name           = "Heap0";
                heapbufmpParams.sharedAddr     = slpmResources_ptr;
                Osal_printf ("Before HeapBufMP_Create: [0x%x]\n",
                                slpmResources_ptr);
                slpmResources_heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (slpmResources_heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("HeapBufMP_create failed for %d processor."
                                 " Handle: [0x%x]\n",
                                 slpmResources_procId,
                                 slpmResources_heapHandle);
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (slpmResources_heapHandle,
                                                    HEAPID);
                    if (status < 0) {
                        Osal_printf ("MessageQ_registerHeap failed!\n");
                    }
                }
            }
        }
    }
#endif /* !defined(SYSLINK_USE_DAEMON) */

    Osal_printf ("Leaving slpmResources_startup: status = 0x%x\n", status);

    return (status);
}


/*!
 *  @brief  Function to execute the SlpmResources sample application
 *
 *  @sa
 */
Int
SlpmResources_execute (Int testNo)
{
    Int32                    status             = 0;
    MessageQ_Msg             msg                = NULL;
    MessageQ_Params          msgParams;
    UInt16                   i;
    Char                   * msgQName;
    Int32                    slpmErrorStatus    = 0;
    MsgInfo*                 SlpmMsg;

    Osal_printf ("Entered SlpmResources_execute\n");

    /* Create the Message Queue. */
    MessageQ_Params_init (&msgParams);
    slpmResources_messageQ = MessageQ_create (ARM_MESSAGEQNAME, &msgParams);
    if (slpmResources_messageQ == NULL) {
        Osal_printf ("Error in MessageQ_create\n");
    }
    else {
        Osal_printf ("MessageQ_create handle [0x%x]\n",
                     slpmResources_messageQ);
    }

    /* Assign the MessageQ Name being opened */
    switch (testNo) {
        case 2:
            msgQName = DUCATI_CORE1_MESSAGEQNAME;
            break;

        case 1:
        default:
            msgQName = DUCATI_CORE0_MESSAGEQNAME;
            break;
    }

    sleep (1); /* Adding a small delay to resolve infinite nameserver issue */
    if (status >=0) {
        do {
            status = MessageQ_open (msgQName, &slpmResources_queueId);
        } while (status == MessageQ_E_NOTFOUND);
        if (status < 0) {
            Osal_printf ("Error in MessageQ_open [0x%x]\n", status);
        }
        else {
            Osal_printf ("MessageQ_open Status [0x%x]\n", status);
            Osal_printf ("slpmResources_queueId  [0x%x]\n", slpmResources_queueId);
        }
    }

    if (status >= 0) {
        Osal_printf ("\nExchanging messages with remote processor\n");
        for (i = 0 ; i < MESSAGEQAPP_NUM_TRANSFERS ; i++) {
            /* Allocate message. */
            msg = MessageQ_alloc (HEAPID, MSGSIZE);
            if (msg == NULL) {
                Osal_printf ("Error in MessageQ_alloc\n");
                break;
            }
            else {
                Osal_printf ("MessageQ_alloc msg [0x%x]\n", msg);
            }

            MessageQ_setMsgId (msg, (i % MESSAGEQAPP_NUM_TRANSFERS));
            /* In the middle of the requests we send the suspend */

            if (i==5) {
                status = ProcMgr_control (slpmResources_procMgrHandle,
                                          PM_SUSPEND, NULL);
                /* Delay is needed when hibernation is enabled to simulate*/
                /* the behavior in the System Suspend-Resume path*/
                sleep (WAIT_DUCATI_SAVE_CONTEXT);
                status = ProcMgr_control (slpmResources_procMgrHandle,
                                          PM_RESUME, NULL);
            }

            /* Have the DSP reply to this message queue */
            MessageQ_setReplyQueue (slpmResources_messageQ, msg);

            status = MessageQ_put (slpmResources_queueId, msg);
            if (status < 0) {
                Osal_printf ("Error in MessageQ_put [0x%x]\n",
                             status);
                break;
            }

            status = MessageQ_get(slpmResources_messageQ, &msg, MessageQ_FOREVER);
            if (status < 0) {
                Osal_printf ("Error in MessageQ_get\n");
                break;
            }
            else {

                /* Validate the returned message. */
                if (msg != NULL) {
                    if (MessageQ_getMsgId (msg) !=
                                    ((i % MESSAGEQAPP_NUM_TRANSFERS) + 1)) {
                        Osal_printf ("Data integrity failure!\n"
                                     "    Expected %d\n"
                                     "    Received %d\n",
                                     ((i % MESSAGEQAPP_NUM_TRANSFERS) + 1),
                                     MessageQ_getMsgId (msg));
                        break;
                    }
                    SlpmMsg = (MsgInfo*)msg;
                    if (SlpmMsg->slpmError) {
                        slpmErrorStatus = SlpmMsg->slpmError;
                        break;
                    }
                }

                status = MessageQ_free (msg);
                Osal_printf ("MessageQ_free status [0x%x]\n", status);
            }

            if ((i % 2) == 0) {
                Osal_printf ("Exchanged %d messages with remote processor\n",
                             i);
            }
        }
    }

    /* Keep the Ducati application running. */
#if !defined (SYSLINK_USE_DAEMON)
    /* Send die message */
    msg = MessageQ_alloc (HEAPID, MSGSIZE);
    if (msg == NULL) {
        Osal_printf ("MessageQ_alloc (die message) failed\n");
    }
    else {
        Osal_printf ("MessageQ_alloc (die message) msg = [0x%x]\n", msg);

        /* Send a die message */
        MessageQ_setMsgId(msg, DIEMESSAGE);

        /* Have the DSP reply to this message queue */
        MessageQ_setReplyQueue (slpmResources_messageQ, msg);

        /* Send the message off */
        status = MessageQ_put (slpmResources_queueId, msg);
        if (status < 0) {
            Osal_printf ("Error in MessageQ_put (die message) [0x%x]\n",
                         status);
        }
        else {
            Osal_printf ("MessageQ_put (die message) Status [0x%x]\n", status);
        }

        /* Wait for the final message. */
        status = MessageQ_get(slpmResources_messageQ, &msg, MessageQ_FOREVER);
        if (status < 0) {
            Osal_printf ("\nError in MessageQ_get (die message)!\n");
        }
        else {
            /* Validate the returned message. */
            if (msg != NULL) {
                if (MessageQ_getMsgId (msg) == DIEMESSAGE) {
                    Osal_printf ("\nSuccessfully received die response from the "
                                 "remote processor\n");
                    Osal_printf ("Sample application successfully completed!\n");
                }
                else {
                    Osal_printf("\nUnsuccessful run of the sample "
                                "application!\n");
                }
            }
            else {
                Osal_printf("\nUnsuccessful run of the sample application msg "
                          "is NULL!\n");
            }
        }
        MessageQ_free(msg);
    }
#else
    Osal_printf ("Sample application successfully completed!\n");
#endif /* !SYSLINK_USE_DAEMON */

    /* Clean-up */
    if (slpmResources_messageQ != NULL) {
        status = MessageQ_delete (&slpmResources_messageQ);
        if (status < 0) {
            Osal_printf ("Error in MessageQ_delete [0x%x]\n",
                         status);
        }
        else {
            Osal_printf ("MessageQ_delete Status [0x%x]\n", status);
        }
    }

    if (slpmResources_queueId != MessageQ_INVALIDMESSAGEQ) {
        MessageQ_close (&slpmResources_queueId);
    }

    if (slpmErrorStatus)
        status = slpmErrorStatus;

    Osal_printf ("Leaving SlpmResources_execute\n");

    return (status);
}


/*!
 *  @brief Function to execute the shutdown for SlpmResources sample app
 *
 *  @sa
 */

Int
SlpmResources_shutdown (Int testNo)
{
    Int32               status = 0;
#if !defined (SYSLINK_USE_DAEMON)
    ProcMgr_StopParams  stopParams;
#endif /* !defined(SYSLINK_USE_DAEMON) */
    IHeap_Handle        srHeap = NULL;

    Osal_printf ("Entered SlpmResources_shutdown()\n");

    status = MessageQ_unregisterHeap (HEAPID);
    Osal_printf ("MessageQ_unregisterHeap status: [0x%x]\n", status);

    if (slpmResources_heapHandle) {
        status = HeapBufMP_delete (&slpmResources_heapHandle);
        Osal_printf ("HeapBufMP_delete status: [0x%x]\n", status);
    }

    if (slpmResources_ptr) {
        srHeap = SharedRegion_getHeap (0);
        Memory_free (srHeap, slpmResources_ptr, slpmResources_heapSize);
    }

#if !defined (SYSLINK_USE_DAEMON)
    if (testNo == 2) {
        stopParams.proc_id = slpmResources_procId1;
        status = ProcMgr_stop (slpmResources_procMgrHandle1, &stopParams);
        Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);
    }

    stopParams.proc_id = slpmResources_procId;
    status = ProcMgr_stop (slpmResources_procMgrHandle, &stopParams);
    Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);
#endif /* !defined(SYSLINK_USE_DAEMON) */

    if (testNo == 2) {
        status =  ProcMgr_detach (slpmResources_procMgrHandle1);
        Osal_printf ("ProcMgr_detach status [0x%x]\n", status);

        status = ProcMgr_close (&slpmResources_procMgrHandle1);
        Osal_printf ("ProcMgr_close status: [0x%x]\n", status);
    }

    status =  ProcMgr_detach (slpmResources_procMgrHandle);
    Osal_printf ("ProcMgr_detach status [0x%x]\n", status);

    status = ProcMgr_close (&slpmResources_procMgrHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);

    status = Ipc_destroy ();
    Osal_printf ("Ipc_destroy status: [0x%x]\n", status);

    Osal_printf ("Leave slpmResources_shutdown()\n");

    return (status);
}
