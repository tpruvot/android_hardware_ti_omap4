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
 *  @file   slpmtransportApp.c
 *
 *  @brief  Sample application for SlpmTransport module between MPU & AppM3
 *  using messageQ
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
#include "slpmtransportApp_config.h"


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
#define SLPMTRANSPORT_SYSM3ONLY_IMAGE "./Transport_MPUSYS_Test_PM_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define SLPMTRANSPORT_SYSM3_IMAGE     "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define SLPMTRANSPORT_APPM3_IMAGE     "./Transport_MPUAPP_Test_PM_Core1.xem3"

/*!
 *  @brief  Number of transfers to be tested.
 */
#define  SLPMTRANSPORT_NUM_TRANSFERS  4


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
MessageQ_Handle                slpmTransport_messageQ;
HeapBufMP_Handle               slpmTransport_heapHandle;
MessageQ_QueueId               slpmTransport_queueId = MessageQ_INVALIDMESSAGEQ;
UInt16                         slpmTransport_procId;
UInt16                         slpmTransport_procId1;
UInt32                         slpmTransport_curShAddr;
ProcMgr_Handle                 slpmTransport_procMgrHandle;
ProcMgr_Handle                 slpmTransport_procMgrHandle1;
SizeT                          slpmTransport_heapSize         = 0;
Ptr                            slpmTransport_ptr              = NULL;

/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for SlpmTransport sample application
 *
 *  @sa
 */
Int
SlpmTransport_startup (Int testNo)
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

    Osal_printf ("[A9]:Entered slpmTransport_startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("[A9]:Error in Ipc_setup [0x%x]\n", status);
    }

    /* Open a handle to the SysM3 ProcMgr instance. */
    slpmTransport_procId = MultiProc_getId ("SysM3");
    status = ProcMgr_open (&slpmTransport_procMgrHandle, slpmTransport_procId);
    if (status < 0) {
        Osal_printf ("[A9]:Error in ProcMgr_open (SysM3) [0x%x]\n", status);
    }
    else {
        Osal_printf ("[A9]:ProcMgr_open (SysM3) Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (slpmTransport_procMgrHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("[A9]:ProcMgr_attach (SysM3) failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("[A9]:ProcMgr_attach (SysM3) status:[0x%x]\n", status);
            state = ProcMgr_getState (slpmTransport_procMgrHandle);
            Osal_printf ("[A9]:After attach: ProcMgr_getState (SysM3)\n"
                         "    state [0x%x]\n", status);
        }
    }

    /* Open a handle to the AppM3 ProcMgr instance. */
    if ((status >= 0) && (testNo == 2)) {
        slpmTransport_procId1 = MultiProc_getId ("AppM3");
        status = ProcMgr_open (&slpmTransport_procMgrHandle1,
                                slpmTransport_procId1);
        if (status < 0) {
            Osal_printf ("[A9]:Error in ProcMgr_open (AppM3) [0x%x]\n", status);
        }
        else {
            Osal_printf ("[A9]:ProcMgr_open (AppM3) Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (slpmTransport_procMgrHandle1,
                                    &attachParams);
            if (status < 0) {
                Osal_printf ("[A9]:ProcMgr_attach (AppM3) failed [0x%x]\n",
                                status);
            }
            else {
                Osal_printf ("[A9]:ProcMgr_attach(AppM3) status: [0x%x]\n",
                                status);
                state = ProcMgr_getState (slpmTransport_procMgrHandle1);
                Osal_printf ("[A9]:After attach: ProcMgr_getState (AppM3)\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

#if !defined(SYSLINK_USE_DAEMON) /* Daemon sets this up */
#ifdef SYSLINK_USE_LOADER
    if (status >= 0) {
        if (testNo == 1)
            imageName = SLPMTRANSPORT_SYSM3ONLY_IMAGE;
        else if (testNo == 2)
            imageName = SLPMTRANSPORT_SYSM3_IMAGE;

        Osal_printf ("Loading image (%s) onto Ducati with ProcId %d\n",
                        imageName, slpmTransport_procId);
        status = ProcMgr_load (slpmTransport_procMgrHandle, imageName, 2,
                                (String *) &imageName, &entryPoint, &fileId,
                                slpmTransport_procId);
        if (status < 0) {
            Osal_printf ("[A9]:Error in ProcMgr_load (SysM3) image: [0x%x]\n",
                            status);
        }
        else {
            Osal_printf ("[A9]:ProcMgr_load (SysM3) Status [0x%x]\n", status);
        }
    }
#endif /* defined(SYSLINK_USE_LOADER) */
    if (status >= 0) {
        startParams.proc_id = slpmTransport_procId;
        status = ProcMgr_start (slpmTransport_procMgrHandle, entryPoint,
                                &startParams);
        if (status < 0) {
            Osal_printf ("[A9]:Error in ProcMgr_start (SysM3) [0x%x]\n",
                            status);
        }
        else {
           Osal_printf ("[A9]:ProcMgr_start (SysM3) Status [0x%x]\n",
                            status);
        }
    }

    if ((status >= 0) && (testNo == 2)) {
#if defined(SYSLINK_USE_LOADER)
        imageName = SLPMTRANSPORT_APPM3_IMAGE;
        status = ProcMgr_load (slpmTransport_procMgrHandle1, imageName, 2,
                                &imageName, &entryPoint, &fileId,
                                slpmTransport_procId1);
        if (status < 0) {
            Osal_printf ("[A9]:Error in ProcMgr_load (AppM3) image: 0x%x]\n",
                            status);
        }
        else {
            Osal_printf ("[A9]:ProcMgr_load (AppM3) Status [0x%x]\n",
                            status);
        }
#endif /* defined(SYSLINK_USE_LOADER) */
        startParams.proc_id = slpmTransport_procId1;
        status = ProcMgr_start (slpmTransport_procMgrHandle1, entryPoint,
                                    &startParams);
        if (status < 0) {
            Osal_printf ("[A9]:Error in ProcMgr_start (AppM3) [0x%x]\n",
                            status);
        }
        else {
            Osal_printf ("[A9]:ProcMgr_start (AppM3) Status [0x%x]\n",
                            status);
        }
    }
#endif /* !defined(SYSLINK_USE_DAEMON) */

    srCount = SharedRegion_getNumRegions();
    Osal_printf ("[A9]:SharedRegion_getNumRegions = %d\n", srCount);
    for (i = 0; i < srCount; i++) {
        status = SharedRegion_getEntry (i, &srEntry);
        Osal_printf ("[A9]:SharedRegion_entry #%d: base = 0x%x len = 0x%x "
                        "ownerProcId = %d isValid = %d \n cacheEnable = %d "
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
        slpmTransport_heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("[A9]:heapSize = 0x%x\n", slpmTransport_heapSize);

        srHeap = SharedRegion_getHeap (0);
        if (srHeap == NULL) {
            status = MessageQ_E_FAIL;
            Osal_printf ("[A9]:SharedRegion_getHeap failed for %d processor."
                         " srHeap: [0x%x]\n",
                         slpmTransport_procId,
                         srHeap);
        }
        else {
            Osal_printf ("[A9]:Before Memory_alloc = 0x%x\n", srHeap);
            slpmTransport_ptr = Memory_alloc (srHeap,
                                slpmTransport_heapSize,
                                0);
            if (slpmTransport_ptr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("[A9]:Memory_alloc failed for %d processor."
                             " ptr: [0x%x]\n",
                             slpmTransport_procId,
                             slpmTransport_ptr);
            }
            else {
                heapbufmpParams.name           = "Heap0";
                heapbufmpParams.sharedAddr     = slpmTransport_ptr;
                Osal_printf ("[A9]:Before HeapBufMP_Create: [0x%x]\n",
                                slpmTransport_ptr);
                slpmTransport_heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (slpmTransport_heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("[A9]:HeapBufMP_create failed for %d "
                                 " processor. Handle: [0x%x]\n",
                                 slpmTransport_procId,
                                 slpmTransport_heapHandle);
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (slpmTransport_heapHandle,
                                                    HEAPID);
                    if (status < 0) {
                        Osal_printf ("[A9]:MessageQ_registerHeap failed!\n");
                    }
                }
            }
        }
    }
#endif /* !defined(SYSLINK_USE_DAEMON) */

    Osal_printf ("[A9]:Leaving SlpmTransport_startup: status = 0x%x\n", status);

    return (status);
}


/*!
 *  @brief  Function to execute the SlpmTransport sample application
 *
 *  @sa
 */
Int
SlpmTransport_execute (Int testNo)
{
    Int32                    status = 0;
    MessageQ_Msg             msg    = NULL;
    MessageQ_Params          msgParams;
    UInt16                   i;
    Char                   * msgQName;

    Osal_printf ("[A9]:Entered SlpmTransport_execute\n");

    /* Create the Message Queue. */
    MessageQ_Params_init (&msgParams);
    slpmTransport_messageQ = MessageQ_create (ARM_MESSAGEQNAME, &msgParams);
    if (slpmTransport_messageQ == NULL) {
        Osal_printf ("[A9]:Error in MessageQ_create\n");
    }
    else {
        Osal_printf ("[A9]:MessageQ_create handle [0x%x]\n",
                     slpmTransport_messageQ);
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
            status = MessageQ_open (msgQName, &slpmTransport_queueId);
        } while (status == MessageQ_E_NOTFOUND);
        if (status < 0) {
            Osal_printf ("[A9]:Error in MessageQ_open [0x%x]\n", status);
        }
        else {
            Osal_printf ("[A9]:MessageQ_open Status [0x%x]\n", status);
            Osal_printf ("[A9]:slpmTransport_queueId  [0x%x]\n",
                            slpmTransport_queueId);
        }
    }

    if (status >= 0) {
        Osal_printf ("[A9]:\nExchanging messages with remote processor\n");
        for (i = 0 ; i < SLPMTRANSPORT_NUM_TRANSFERS ; i++) {
            /* Allocate message. */
            msg = MessageQ_alloc (HEAPID, MSGSIZE);
            if (msg == NULL) {
                Osal_printf ("[A9]:Error in MessageQ_alloc\n");
                break;
            }
            else {
                Osal_printf ("[A9]:MessageQ_alloc msg [0x%x]\n", msg);
            }

            MessageQ_setMsgId (msg, (i % 16));

            /* Have the DSP reply to this message queue */
            MessageQ_setReplyQueue (slpmTransport_messageQ, msg);

            status = MessageQ_put (slpmTransport_queueId, msg);
            if (status < 0) {
                Osal_printf ("[A9]:Error in MessageQ_put [0x%x]\n",
                             status);
                break;
            }

            status = MessageQ_get(slpmTransport_messageQ, &msg,
                                    MessageQ_FOREVER);
            if (status < 0) {
                Osal_printf ("[A9]:Error in MessageQ_get\n");
                break;
            }
            else {

                /* Validate the returned message. */
                if (msg != NULL) {
                    if (MessageQ_getMsgId (msg) != ((i % 16) + 1) ) {
                        Osal_printf ("[A9]:Data integrity failure!\n"
                                     "    Expected %d\n"
                                     "    Received %d\n",
                                     ((i % 16) + 1),
                                     MessageQ_getMsgId (msg));
                        break;
                    }
                }

                status = MessageQ_free (msg);
                Osal_printf ("[A9]:MessageQ_free status [0x%x]\n", status);
            }

            if ((i % 2) == 0) {
                Osal_printf ("[A9]:Exchanged %d messages with"
                            "remote processor\n", i);
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
        MessageQ_setReplyQueue (slpmTransport_messageQ, msg);

        /* Send the message off */
        status = MessageQ_put (slpmTransport_queueId, msg);
        if (status < 0) {
            Osal_printf ("Error in MessageQ_put (die message) [0x%x]\n",
                         status);
        }
        else {
            Osal_printf ("MessageQ_put (die message) Status [0x%x]\n", status);
        }

        /* Wait for the final message. */
        status = MessageQ_get(slpmTransport_messageQ, &msg, MessageQ_FOREVER);
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
    if (slpmTransport_messageQ != NULL) {
        status = MessageQ_delete (&slpmTransport_messageQ);
        if (status < 0) {
            Osal_printf ("Error in MessageQ_delete [0x%x]\n",
                         status);
        }
        else {
            Osal_printf ("MessageQ_delete Status [0x%x]\n", status);
        }
    }

    if (slpmTransport_queueId != MessageQ_INVALIDMESSAGEQ) {
        MessageQ_close (&slpmTransport_queueId);
    }

    Osal_printf ("Leaving SlpmTransport_execute\n");

    return (status);
}


/*!
 *  @brief Function to execute the shutdown for SlpmTransport sample app
 *
 *  @sa
 */

Int
SlpmTransport_shutdown (Int testNo)
{
    Int32               status = 0;
#if !defined (SYSLINK_USE_DAEMON)
    ProcMgr_StopParams  stopParams;
#endif /* !defined(SYSLINK_USE_DAEMON) */
    IHeap_Handle        srHeap = NULL;

    Osal_printf ("[A9]:Entered SlpmTransport_shutdown()\n");

    status = MessageQ_unregisterHeap (HEAPID);
    Osal_printf ("[A9]:MessageQ_unregisterHeap status: [0x%x]\n", status);

    if (slpmTransport_heapHandle) {
        status = HeapBufMP_delete (&slpmTransport_heapHandle);
        Osal_printf ("[A9]:HeapBufMP_delete status: [0x%x]\n", status);
    }

    if (slpmTransport_ptr) {
        srHeap = SharedRegion_getHeap (0);
        Memory_free (srHeap, slpmTransport_ptr, slpmTransport_heapSize);
    }

#if !defined (SYSLINK_USE_DAEMON)
    if (testNo == 2) {
        stopParams.proc_id = slpmTransport_procId1;
        status = ProcMgr_stop (slpmTransport_procMgrHandle1, &stopParams);
        Osal_printf ("[A9]:ProcMgr_stop status: [0x%x]\n", status);
    }

    stopParams.proc_id = slpmTransport_procId;
    status = ProcMgr_stop (slpmTransport_procMgrHandle, &stopParams);
    Osal_printf ("[A9]:ProcMgr_stop status: [0x%x]\n", status);
#endif /* !defined(SYSLINK_USE_DAEMON) */

    if (testNo == 2) {
        status =  ProcMgr_detach (slpmTransport_procMgrHandle1);
        Osal_printf ("[A9]:ProcMgr_detach status [0x%x]\n", status);

        status = ProcMgr_close (&slpmTransport_procMgrHandle1);
        Osal_printf ("[A9]:ProcMgr_close status: [0x%x]\n", status);
    }

    status =  ProcMgr_detach (slpmTransport_procMgrHandle);
    Osal_printf ("[A9]:ProcMgr_detach status [0x%x]\n", status);

    status = ProcMgr_close (&slpmTransport_procMgrHandle);
    Osal_printf ("[A9]:ProcMgr_close status: [0x%x]\n", status);

    status = Ipc_destroy ();
    Osal_printf ("[A9]:Ipc_destroy status: [0x%x]\n", status);

    Osal_printf ("[A9]:Leave SlpmTransport_shutdown()\n");

    return (status);
}
