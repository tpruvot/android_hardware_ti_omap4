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

/*  ----------------------------------- Linux headers                 */
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdarg.h>
/* Standard headers */
#include <Std.h>
#include <errbase.h>
#include <ipctypes.h>

/*  ----------------------------------- Notify headers              */
#include <notifyxfer_os.h>
#include <ti/ipc/Notify.h>
#include <Trace.h>
#include <stdlib.h>
#include <String.h>
#include <OsalPrint.h>
#include <IpcUsr.h>
#include <ti/ipc/MultiProc.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @macro  EVENT_STARTNO
 *
 *  @desc   Number after resevred events.
 *  ============================================================================
 */
#define EVENT_STARTNO           5
#define MAX_ITERATIONS          100
#define MIN_ITERATIONS          1

/*!
 *  @brief  Interrupt ID of physical interrupt handled by the Notify driver to
 *          receive events.
 */
#define NOTIFYAPP_RECV_INT_ID   26

/*!
 *  @brief  Interrupt ID of physical interrupt handled by the Notify driver to
 *          send events.
 */
#define NOTIFYAPP_SEND_INT_ID   55

/*!
 *  @brief  Interrupt line ID to be used
 */
#define  NOTIFYAPP_LINEID       0

#define NOTIFYAPP_NUMITERATIONS 5

#define NUM_MEM_ENTRIES         9

#define RESET_VECTOR_ENTRY_ID   0

#define TERMINATE_MESSAGE       0xDEAD

#define NOTIFY_SYSM3ONLY_IMAGE  "./Notify_MPUSYS_Test_Core0.xem3"
#define NOTIFY_SYSM3_IMAGE      "./Notify_MPUSYS_reroute_Test_Core0.xem3"
#define NOTIFY_APPM3_IMAGE      "./Notify_MPUAPP_reroute_Test_Core1.xem3"
#define NOTIFY_TESLA_IMAGE      "./Notify_Test_Dsp.xe64T"

/** ============================================================================
 *  @macro  numIterations
 *
 *  @desc   Number of ietrations.
 *  ============================================================================
 */
UInt16 numIterations = NOTIFYAPP_NUMITERATIONS;

UInt16 NotifyPing_recvEventCount [Notify_MAXEVENTS];

Processor_Id procId;

PVOID           event [Notify_MAXEVENTS];
UInt16          eventNo = Notify_MAXEVENTS;
ProcMgr_Handle  procHandle;
ProcMgr_Handle  procHandle1;
Handle          proc4430Handle;


typedef struct NotifyPing_semObj_tag {
    sem_t  sem;
} NotifyPing_semObj;

struct pingArg {
    UInt32 arg1;
    UInt32 arg2;
};

struct pingArg eventCbckArg [Notify_MAXEVENTS];


Int NotifyPing_createSem (OUT PVOID * semPtr)
{
    Int                 status   = Notify_S_SUCCESS;
    NotifyPing_semObj * semObj;
    Int                 osStatus;

    semObj = malloc (sizeof (NotifyPing_semObj));
    if (semObj != NULL) {
        osStatus = sem_init (&(semObj->sem), 0, 0);
        if (osStatus < 0) {
            status = DSP_EFAIL;
        }
        else {
            *semPtr = (PVOID) semObj;
        }
    }
    else {
        *semPtr = NULL;
        status = DSP_EFAIL;
    }

    return status;
}


Void NotifyPing_deleteSem (IN PVOID * semHandle)
{
    Int                  status   = Notify_S_SUCCESS;
    NotifyPing_semObj  * semObj   = (NotifyPing_semObj *)semHandle;
    Int                  osStatus;

    osStatus = sem_destroy (&(semObj->sem));
    if (osStatus) {
        status = DSP_EFAIL;
    }
    else {
       free (semObj);
    }

    Osal_printf ("Notify_DestroySem status[%x]\n",status);
}


Int NotifyPing_postSem (IN PVOID semHandle)
{
    Int                     status   = Notify_S_SUCCESS;
    NotifyPing_semObj     * semObj   = semHandle;
    Int                     osStatus;

    osStatus = sem_post (&(semObj->sem));
    if (osStatus < 0) {
        status = DSP_EFAIL;
    }

    return status;
}


Int NotifyPing_waitSem (IN PVOID semHandle)
{
    Int                  status = Notify_S_SUCCESS;
    NotifyPing_semObj  * semObj = semHandle;
    Int                  osStatus;

    Osal_printf ("NotifyPing_waitSem\n");
    osStatus = sem_wait (&(semObj->sem));
    if (osStatus < 0) {
        status = Notify_E_FAIL;
    }

    return status;
}


/** ----------------------------------------------------------------------------
 *  @name   NotifyPing_callback
 *
 *  @desc   Callback
 *
 *  @modif  None.
 *  ----------------------------------------------------------------------------
 */
Void NotifyPing_callback (IN     Processor_Id procId,
    IN     UInt16       lineId,
    IN     UInt32       eventNo,
    IN OPT Void *       arg,
    IN OPT UInt32       payload)
{
    (Void) eventNo;
    (Void) procId ;
    (Void) payload;
    struct pingArg eventClbkArg;

    eventClbkArg.arg1 = ((struct pingArg *)arg)->arg1;
    eventClbkArg.arg2 = ((struct pingArg *)arg)->arg2;

    Osal_printf ("------Called callbackfunction------" );
    Osal_printf (" arg1: 0x%x arg2 = 0x%x\n", eventClbkArg.arg1,
                    eventClbkArg.arg2);

    NotifyPing_postSem ((PVOID)eventClbkArg.arg1);
}


static Int NotifyPing_startup (Int testNo)
{
    Int                     status      = Notify_S_SUCCESS;
    UInt16                  eventId;
    UInt32                  fileId;
    UInt32                  entryPoint  = 0;
    ProcMgr_StartParams     startParams;
    char *                  imagePath;
    Ipc_Config              sysCfg;
    ProcMgr_AttachParams    attachParams;
    ProcMgr_State           state;
    Int                     i;

    Osal_printf ("Entered NotifyPing_startup: testNo = %d\n", testNo);

    Ipc_getConfig (&sysCfg);
    status = Ipc_setup (&sysCfg);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
        return status;
    }
    else {
        Osal_printf ("Ipc_setup successful!");
    }

    eventId = eventNo;
    procId = ((testNo == 1) ? MultiProc_getId ("SysM3") : \
                ((testNo == 2) ? MultiProc_getId ("AppM3") : \
                    MultiProc_getId("Tesla")));

    status = ProcMgr_open (&procHandle, (testNo == 2 ? \
                                MultiProc_getId ("SysM3") : procId));
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
        return status;
    }
    if (status >= 0) {
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
                         "    state [0x%x]\n", state);
        }
    }

    if ((status >= 0) && (procId == MultiProc_getId ("AppM3"))) {
        status = ProcMgr_open (&procHandle1, procId);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
            return status;
        }
        if (status >= 0) {
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procHandle1, &attachParams);
            if (status < 0) {
                Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
            }
            else {
                Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
                state = ProcMgr_getState (procHandle1);
                Osal_printf ("After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", state);
            }
        }
    }

    if (status >= 0) {
        startParams.proc_id = procId;
        if (procId == MultiProc_getId("Tesla")) {
            imagePath = NOTIFY_TESLA_IMAGE;
            Osal_printf ("Testing Tesla only! Image = %s\n", imagePath);
        }
        else if (procId == MultiProc_getId("SysM3")) {
            imagePath = NOTIFY_SYSM3ONLY_IMAGE;
            Osal_printf ("Testing SysM3 only! Image = %s\n", imagePath);
        }
        else if (procId == MultiProc_getId("AppM3")) {
            startParams.proc_id = MultiProc_getId("SysM3");
            imagePath = NOTIFY_SYSM3_IMAGE;
            Osal_printf ("Testing AppM3 only! Image = %s\n", imagePath);
        }

        status = ProcMgr_load (procHandle, imagePath,
                2, &imagePath, &entryPoint,
                &fileId, startParams.proc_id);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_load [0x%x]:SYSM3\n", status);
            return status;
        }
        else {
            Osal_printf ("ProcMgr_load (SysM3) successful!\n");
        }
        status = ProcMgr_start (procHandle, 0, &startParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_start [0x%x]:SYSM3\n", status);
            return status;
        }
    }

    if ((status >= 0) && (procId == MultiProc_getId ("AppM3"))) {
        startParams.proc_id = procId;
        imagePath = NOTIFY_APPM3_IMAGE;
        status = ProcMgr_load (procHandle1, NOTIFY_APPM3_IMAGE,
                               2, &imagePath, &entryPoint,
                               &fileId, startParams.proc_id);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_load [0x%x]:APPM3\n", status);
            return status;
        }

        status = ProcMgr_start (procHandle1, 0, &startParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_start [0x%x]:APPM3\n", status);
            return status;
        }
    }

    /* Register for all events */
    if ((status >= 0) && (eventNo == Notify_MAXEVENTS)) {
        Osal_printf ("Registering for all events\n");

        for(i = EVENT_STARTNO; i < Notify_MAXEVENTS; i++) {
            /* Create the semaphore */
            status = NotifyPing_createSem (&event[i]);
            if (status < 0) {
                Osal_printf ("Error in NotifyPing_createSem [0x%x]\n", status);
                return status;
            }
            /*Register for the event */
            NotifyPing_recvEventCount [i] = numIterations;
            eventCbckArg[i].arg1 = (UInt32)event[i];
            eventCbckArg[i].arg2 = i;
            Osal_printf("eventCbckArg[%d].arg1 = 0x%x arg2 = 0x%x\n", i,
                        (UInt32)event[i], i);

            status = Notify_registerEvent (procId, NOTIFYAPP_LINEID,
                            i,
                            (Notify_FnNotifyCbck) NotifyPing_callback,
                            (Void *)&(eventCbckArg[i]));
            if (status < 0) {
                Osal_printf ("Error in Notify_registerEvent status [%d] "
                             "Event[0x%x]\n", status, i);
                return status;
            }
            Osal_printf ("Registered event number %d with Notify module 0x%x\n",
                i, (UInt32)&(eventCbckArg[i]));
        }
    }
    else if (status >=0 ){
        /* Create Semaphore for the event */
        Osal_printf ("Create sem for event number %d\n", eventNo);
        status = NotifyPing_createSem (&event [eventNo]);
        if (status < 0) {
            Osal_printf ("Error in NotifyPing_createSem [0x%x]\n", status);
            return status;
        }
        NotifyPing_recvEventCount [eventNo] = numIterations;

        /* Fill the callback args */
        eventCbckArg[eventNo].arg1 = (UInt32)event[eventNo];
        eventCbckArg[eventNo].arg2 = eventNo;

        /* Register Event */
        Osal_printf ("Registering for event number %d\n",eventNo);
        status = Notify_registerEvent (procId,
                                    NOTIFYAPP_LINEID,
                                    eventNo,
                                    (Notify_FnNotifyCbck) NotifyPing_callback,
                                    (Void *)&eventCbckArg[eventNo]);

        if (status < 0) {
            Osal_printf ("Error in Notify_registerEvent %d [0x%x]\n", eventNo,
                        status);
            return status;
        }
        Osal_printf ("Registered event number %d with Notify module\n",
                     eventNo);
    }

    Osal_printf ("Leaving NotifyPing_startup: status = 0x%x\n", status);

    return status;
}


/** ----------------------------------------------------------------------------
 *  @name   NotifyPing_execute
 *
 *  @desc   Function to execute the NotifyPing sample application
 *
 *  @arg    None
 *
 *  @ret    None
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None.
 *  ----------------------------------------------------------------------------
 */
Int NotifyPing_execute (Void)
{
    Int                     status  = Notify_E_FAIL;
    UInt32                  payload;
    UInt32                  count   = 0;
    Int                     i;
    Int                     j;
    UInt16                  eventId;

    Osal_printf ("Entered NotifyPing_execute\n");

    eventId = eventNo;
    if (eventNo == Notify_MAXEVENTS) {
        eventId = EVENT_STARTNO;
    }
    payload = 0xDEED1;

    /* Keep sending events until ducati side is up and ready to receive
     events */
    do {
        status = Notify_sendEvent (procId, NOTIFYAPP_LINEID, eventId,
                                   payload, FALSE);
    } while (status == Notify_E_FAIL || status == Notify_E_EVTNOTREGISTERED);

    Osal_printf ("---------------------------------------------------------\n");
    Osal_printf ("-----------------[NotifyPingExecute]---------------------\n");
    Osal_printf ("---------------------------------------------------------\n");
    Osal_printf ("------------Iteration\t %d-------------\n", numIterations);

    Osal_printf ("[NotifyPingExecute]>>>>>>>> Sent Event [%d]\n", eventId);

    if (status != Notify_S_SUCCESS) {
        Osal_printf ("Error after send event, status %d\n", status);
        return status;
    }

    /* Start sending and receiving events */
    if (eventNo == Notify_MAXEVENTS) {
        /* Send all events, from 4 - 31, to Ducati for each
           number of transfer */
        for(j = numIterations; j > 0; j--) {
            /* Send for events from 5 - 31 */
            for(i = EVENT_STARTNO + 1; i < Notify_MAXEVENTS; i++) {
                payload++;
                if ((j == 1) && (i == Notify_MAXEVENTS - 1))
                    payload = TERMINATE_MESSAGE;
                status = Notify_sendEvent (procId,
                                           NOTIFYAPP_LINEID,
                                           i,
                                           payload,
                                           FALSE);
                if (status != Notify_S_SUCCESS) {
                    Osal_printf ("Error after send event, status %d\n", status);
                    return status;
                }
                Osal_printf ("Notify_sendEvent status[%d] for Event[%d] payload"
                             "[0x%x]\n", status, i, payload);
                Osal_printf ("[NotifyPingExecute]>>>>>>>> Sent Event[%d]\n", i);
            }

            /* Wait for events from 4 - 31 */
            for(i = EVENT_STARTNO; i < Notify_MAXEVENTS; i++) {
                Osal_printf ("[NotifyPingExecute] Waiting on event %d\n", i);
                status = NotifyPing_waitSem (event [i]);
                Osal_printf ("NotifyPing_waitSem status[%d] for Event[%d]\n",status, i);
                NotifyPing_recvEventCount [i] = numIterations - j;
                Osal_printf ("[NotifyPingExecute]<<<<<<<< Received Event[%d] count %d\n",
                             i, NotifyPing_recvEventCount [i]);
            }

            /* Stop iterations here */
            if (j == 1)
                break;

            Osal_printf ("------------Iteration\t %d-------------\n", j - 1);

            /* Start the next iteration here */
            payload++;
            /* Send event 4 for the next iteration */
            status = Notify_sendEvent (procId,
                                       NOTIFYAPP_LINEID,
                                       EVENT_STARTNO,
                                       payload,
                                       FALSE);

            if (status != Notify_S_SUCCESS) {
                Osal_printf ("Error after send event, status %d\n", status);
                return status;
            }
            Osal_printf ("Notify_sendEvent status[%d] for Event[4] \n", status);
            Osal_printf ("[NotifyPingExecute]>>>>>>>> Sent Event[4]\n");
        }
    }
    else {
        Osal_printf ("[NotifyPingExecute] Waiting on event %d\n", eventNo);
        status = NotifyPing_waitSem (event [eventNo]);

        if (status != Notify_S_SUCCESS) {
            Osal_printf ("Error after send event, status %d\n", status);
            return status;
        }
        Osal_printf ("NotifyPing_waitSem status[%d] for Event[%d]\n", status,
                eventNo);
        Osal_printf ("[NotifyPingExecute]<<<<<<<< Received Event [%d] count 1\n",
                eventNo);

        NotifyPing_recvEventCount [eventNo] = count;
        count = numIterations - 1;

        while (count) {
            payload++;
            if (count == 1)
                payload = TERMINATE_MESSAGE;
            Osal_printf ("------------Iteration\t %d ------------\n",
                            (UInt32)count);
            status = Notify_sendEvent(procId,
                                      NOTIFYAPP_LINEID,
                                      eventNo,
                                      payload,
                                      FALSE);

            if (status != Notify_S_SUCCESS) {
                Osal_printf ("Error after send event, status %d\n", status);
                return status;
            }

            Osal_printf ("Notify_sendEvent status [%d]\n",status);
            Osal_printf ("[NotifyPingExecute]>>>>>>>> Sent Event [%d]\n",
                eventNo);
            Osal_printf ("[NotifyPingExecute] Waiting on event %d\n", eventNo);
            status = NotifyPing_waitSem (event [eventNo]);

            if (status != Notify_S_SUCCESS) {
                Osal_printf ("Error after send event, status %d\n", status);
                return status;
            }
            Osal_printf ("NotifyPing_waitSem status[%d] for Event[%d]\n",
                                                            status, eventNo);
            NotifyPing_recvEventCount [eventNo] = count;
            Osal_printf ("[NotifyPingExecute]<<<<<<<< Received Event [%d] count %d\n",
                                                                    eventNo, count);
            count--;
        }
    }

    if (status < 0) {
        Osal_printf ("Error in Notify_sendEvent [0x%x]\n", status);
    }
    else {
        Osal_printf ("Sent %i events to event ID to remote processor\n",
                    numIterations);
    }

    Osal_printf ("Leaving NotifyPing_execute: status = 0x%x\n", status);

    return status;
}


Int NotifyPing_shutdown (Void)
{
    Int32               status      = 0;
    UInt32              i;
    ProcMgr_StopParams  stopParams;

    Osal_printf ("Entered NotifyPing_shutdown\n");

    /* Unregister event. */
    if (eventNo == Notify_MAXEVENTS) {
        for(i = EVENT_STARTNO; i < Notify_MAXEVENTS; i++) {
            status = Notify_unregisterEvent (procId, NOTIFYAPP_LINEID, i,
                                    (Notify_FnNotifyCbck) NotifyPing_callback,
                                    (UArg) &eventCbckArg[i]);

            if (status != Notify_S_SUCCESS) {
                Osal_printf ("Error after unregister event, status %d event no %d\n",
                        status, i);
                return status;
            }
           /*status = Notify_unregisterEventSingle (procId,
                                         NOTIFYAPP_LINEID,
                                         eventNo);*/
            Osal_printf ("Notify_unregisterEvent status: [0x%x] Event[%d]\n",
                                                                  status, i);
            NotifyPing_deleteSem (event[i]);
        }
    }
    else {
        status = Notify_unregisterEvent (procId,
                                     NOTIFYAPP_LINEID,
                                     eventNo,
                                     (Notify_FnNotifyCbck)NotifyPing_callback,
                                     (UArg) &eventCbckArg[eventNo]);

        if (status != Notify_S_SUCCESS) {
            Osal_printf ("Error after unregister event, status %d\n", status);
            return status;
        }
       /*status = Notify_unregisterEventSingle (procId,
                                     NOTIFYAPP_LINEID,
                                     eventNo);*/

        Osal_printf ("Notify_unregisterEvent status: [0x%x]\n", status);
        NotifyPing_deleteSem (event[eventNo]);
    }

    /* Uninitialize ProcMgr setup */
    stopParams.proc_id = procId;
    if (procId == MultiProc_getId ("AppM3")) {
        status = ProcMgr_stop (procHandle1, &stopParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_stop [0x%d]:APPM3\n", status);
            return status;
        } else {
            stopParams.proc_id = MultiProc_getId ("SysM3");
            Osal_printf ("ProcMgr_stop [0x%x]:APPM3\n", status);
        }
    }

    status = ProcMgr_stop (procHandle, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop [0x%d]:SYSM3\n", status);
        return status;
    } else {
        Osal_printf ("ProcMgr_stop [0x%x]:SYSM3\n", status);
    }

    if (procId == MultiProc_getId ("AppM3")) {
        status =  ProcMgr_detach (procHandle1);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_detach(AppM3) [0x%d]\n", status);
            return status;
        }

        status = ProcMgr_close (&procHandle1);
        if (status < 0) {
            Osal_printf ("ProcMgr_close(SysM3)  status: [0x%x]\n", status);
        }
    }

    status =  ProcMgr_detach (procHandle);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach [0x%d]\n", status);
        return status;
    }

    status = ProcMgr_close (&procHandle);
    if (status < 0) {
        Osal_printf ("ProcMgr_close  status: [0x%x]\n", status);
    }

    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Ipc_destroy  status: [0x%x]\n", status);
    }

    Osal_printf ("Leaving NotifyPing_shutdown\n");

    return 0;
}


Void printUsage (Void)
{
    Osal_printf ("Usage: ./notifyping.out <TestNo> [[<EventNo>] "
        "[<numIterations>]]\n");
    Osal_printf ("\tValid Values:\n\t\tTestNo: 1 or 2 or 3\n"
                "\t\tEventNo: 4 to 31\n"
                "\t\tnumIterations: 1 to 100, default: 5\n");
    Osal_printf ("\tExamples:\n");
    Osal_printf ("\t./notifyping.out 1      : "
                    "NotifyPing all events on SysM3\n");
    Osal_printf ("\t./notifyping.out 2      : "
                    "NotifyPing all events on AppM3\n");
    Osal_printf ("\t./notifyping.out 3      : "
                    "NotifyPing all events on Tesla\n");
    Osal_printf ("\t./notifyping.out 1 17   : "
                    "NotifyPing event#17 on SysM3 for 5 iterations\n");
    Osal_printf ("\t./notifyping.out 2 5    : "
                    "NotifyPing event#5 on AppM3 for 5 iterations\n");
    Osal_printf ("\t./notifyping.out 3 7    : "
                    "NotifyPing event#7 on Tesla for 5 iterations\n");
    Osal_printf ("\t./notifyping.out 1 8 10 : "
                    "NotifyPing event#8 on SysM3 for 10 iterations\n");
    Osal_printf ("\t./notifyping.out 2 30 15: "
                    "NotifyPing event#30 on AppM3 for 15 iterations\n");
    Osal_printf ("\t./notifyping.out 3 24 50: "
                    "NotifyPing event#24 on Tesla for 50 iterations\n");

    return;
}


/** ============================================================================
 *  @func   main()
 *
 *  ============================================================================
 */
Int main (Int argc, Char * argv [])
{
    Int     status         = Notify_S_SUCCESS;
    Int     shutdownStatus = Notify_S_SUCCESS;
    Int     testNo;
    Bool    validTest      = TRUE;

    Osal_printf ("\n== NotifyPing Sample ==\n");

    switch (argc) {
        case 2:
            testNo = atoi (argv[1]);
            break;

        case 3:
            testNo = atoi (argv[1]);
            eventNo = atoi (argv[2]);
            break;

        case 4:
            testNo = atoi (argv[1]);
            eventNo = atoi (argv[2]);
            numIterations  = atoi (argv[3]);
            break;

        default:
            validTest = FALSE;
            break;
    }

    if ((validTest == TRUE) && ((testNo < 1 || testNo > 3) ||
        (eventNo < EVENT_STARTNO || eventNo > Notify_MAXEVENTS) ||
        (numIterations < MIN_ITERATIONS|| numIterations > MAX_ITERATIONS))) {
        validTest = FALSE;
    }

    if (validTest == FALSE) {
        status = Notify_E_FAIL;
        printUsage ();
    }
    else {
        status = NotifyPing_startup (testNo);
        if (status >= 0) {
            status = NotifyPing_execute ();
            if (status < 0) {
                Osal_printf("Error in NotifyPing_execute %d \n",status);
            }

            shutdownStatus = NotifyPing_shutdown ();
        }
    }

    Osal_printf ("\nNotifyPing_execute status = 0x%x\n", status);
    Osal_printf ("\nNotifyPing_shutdown status = 0x%x\n", shutdownStatus);
    Osal_printf ("\n== End NotifyPing Sample ==\n");

    /* Trace for TITAN support */
    if((status < 0) || (shutdownStatus < 0))
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
