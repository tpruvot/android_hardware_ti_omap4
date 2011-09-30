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
/** ============================================================================
 *  @file   notifyxfer.c
 *
 *  @desc   Sample for Notify.
 *  ============================================================================
 */


/*  ----------------------------------- IPC headers                 */
#include <stdio.h>

/* Standard headers */
#include <Std.h>
#include <errbase.h>

/*  ----------------------------------- Notify headers              */
#include <notify.h>
#include <notify_shmDriver.h>
#include <notifyxfer_os.h>
#include <notify_driverdefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */
#define NOTIFY_TESLA_EVENTNUMBER 0x00000001

/** ============================================================================
 *  @macro  EVENT_STARTNO
 *
 *  @desc   Number after resevred events.
 *  ============================================================================
 */
#define  EVENT_STARTNO  3


/** ============================================================================
 *  @macro  numIterations
 *
 *  @desc   Number of ietrations.
 *  ============================================================================
 */
Uint32 numIterations = 0 ;

/** ----------------------------------------------------------------------------
 *  @name   NOTIFYXFER_execute
 *
 *  @desc   Function to execute the NOTIFYXFER sample application
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
static

Void
NOTIFYXFER_execute (Void) ;


/** ----------------------------------------------------------------------------
 *  @name   NOTIFYXFER_callback
 *
 *  @desc   Callback
 *
 *  @modif  None.
 *  ----------------------------------------------------------------------------
 */
Void
NOTIFYXFER_callback (IN     Processor_Id procId,
                     IN     Uint32       eventNo,
                     IN OPT Void *       arg,
                     IN OPT Uint32       payload)
{
    (Void) eventNo ;
    (Void) procId  ;
    (Void) payload ;
	printf("Entering into NOTIFYXFER_callback\n ");

    NOTIFYXFER_PostSem (arg) ;
}


/** ----------------------------------------------------------------------------
 *  @name   NOTIFYXFER_execute
 *
 *  @desc   Function to execute the NOTIFYXFER sample application
 *
 *  @modif  None.
 *  ----------------------------------------------------------------------------
 */
static

Void
NOTIFYXFER_execute (Void) {
    Notify_Status        status = NOTIFY_SOK ;
    Notify_Config        config ;
    Notify_Handle        handle ;
    NotifyShmDrv_Attrs   driverAttrs ;
    Uint32               iter ;
    Uint32               i,j    ;
    PVOID                event [32] ;

    /* Initialize the Semaphore */
    for (i = 3 ; i < 32 ; i++) {
        NOTIFYXFER_CreateSem (&event [i]) ;
    }
#if 0
    driverAttrs.shmBaseAddr        = 0x87F00000 ;
    driverAttrs.shmSize            = 0x4000 ;
    driverAttrs.numEvents          = 32 ;
    driverAttrs.sendEventPollCount = (Uint32) -1 ;
#endif

    config.driverAttrs = (Void *) &driverAttrs ;
    status = Notify_driverInit (NOTIFYSHMDRV_DRIVERNAME,
                                &config,
                                &handle) ;
    if (NOTIFY_FAILED (status)) {
        NOTIFYXFER_1Print ("NOTIFYXFER_execute:Notify_driverInit failed! Status [0x%x]\n",
                           status) ;
    }

    if (NOTIFY_SUCCEEDED (status)) {
        /* register events */

		j = ((NOTIFY_SYSTEM_KEY<<16)|NOTIFY_TESLA_EVENTNUMBER);
		i= EVENT_STARTNO;
//        for (i = EVENT_STARTNO ; (i < 32) && (NOTIFY_SUCCEEDED (status)) ; i++)
			{
			printf("NOTIFYXFER_execute:Calling NotifyRegistereventin \n");
            status = Notify_registerEvent (handle,
                                           0,
                                           j,
                                           NOTIFYXFER_callback,
                                           event [i]) ;
        }

        if (NOTIFY_FAILED (status) && (i != EVENT_STARTNO)) {
            /* unregister events */
            for (; i >= 0 ; i--) {
                Notify_unregisterEvent (handle,
                                        0,
                                        j-1,
                                        NOTIFYXFER_callback,
                                        event [i]) ;
            }
        }
    }

    if (NOTIFY_FAILED (status)) {
       NOTIFYXFER_1Print ("Notify_registerEvent failed : %x\n", status) ;
       Notify_driverExit (handle) ;
    }


    for (iter = 0 ; (iter < numIterations) && (NOTIFY_SUCCEEDED (status)) ; iter++) {
 //       for (i = EVENT_STARTNO ; (i < 32) && (NOTIFY_SUCCEEDED (status)) ; i++)
		i = EVENT_STARTNO;
		{
			printf("calling send event\n");
            status = Notify_sendEvent (handle, 0, j, 0, TRUE) ;
        }

//        for (i = EVENT_STARTNO ; (i < 32) && (NOTIFY_SUCCEEDED (status)) ; i++)
		i = EVENT_STARTNO;

		{
            NOTIFYXFER_WaitSem (event [i]) ;
        }

        if ((iter % 100) == 0 && iter != 0) {
            NOTIFYXFER_1Print ("Transferred %d iterations\n", iter) ;
        }
    }

    if (NOTIFY_FAILED (status)) {
       NOTIFYXFER_1Print ("Notify_sendEvent failed : %x\n", status) ;
       Notify_driverExit (handle) ;
    }

    /* unregister the events */
//    for (i = EVENT_STARTNO ; i < 32 ; i++)
	i = EVENT_STARTNO;
		{
        status = Notify_unregisterEvent (handle,
                                         0,
                                         j,
                                         NOTIFYXFER_callback,
                                         event [i]) ;
    }

    if (handle != NULL) {
       Notify_driverExit (handle) ;
    }
}


/** ============================================================================
 *  @func   NOTIFYXFER_1Print
 *
 *  @desc   Print a message with one arguments.
 *
 *  @modif  None
 *  ============================================================================
 */
int
main (int argc, char * argv [])
{
    Notify_Attrs attrs ;

    if (argc != 2) {
        printf ("Usage %s <number of iterations>\n", argv [0]) ;
    }
    else {
        numIterations = NOTIFYXFER_Atoi (argv [1]) ;
		printf("Calling Notify_init");
		attrs.maxDrivers = 1;
        Notify_init (&attrs) ;
		printf("Calling NOTIFYXFER_execute");
    NOTIFYXFER_execute () ;
    }

    return 0 ;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
