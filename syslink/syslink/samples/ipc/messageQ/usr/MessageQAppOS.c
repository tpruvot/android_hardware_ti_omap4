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
 *  @file   MessageQAppOS.c
 *
 *  @brief  OS-specific sample application driver module for MessageQ module
 *
 *  ============================================================================
 */

/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>

/* Linux OS-specific headers */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>

/* Module headers */
#include <IpcUsr.h>
#include <ti/ipc/MultiProc.h>
//#include <ProcMgr.h>
#include <UsrUtilsDrv.h>

/* Application header */
#include "MessageQApp_config.h"

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define MESSAGEQ_DRIVER_NAME         "/dev/syslinkipc/MessageQ"


/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for MessageQApp sample application
 */
extern Int MessageQApp_startup (Int testNo);

/*!
 *  @brief  Function to execute the startup for MessageQApp sample application
 */
extern Int MessageQApp_execute (Int testNo);

/*!
 *  @brief  Function to execute the shutdown for MessageQApp sample application
 */
extern Int MessageQApp_shutdown (Int testNo);


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  ProcMgr handle.
 */
//ProcMgr_Handle MessageQApp_procMgrHandle = NULL;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Void printUsage (Void)
{
    Osal_printf ("Usage: ./messageQApp.out [<TestNo>]\n");
    Osal_printf ("\tValid Values:\n\t\tTestNo: 1 or 2 or 3 (default = 1)\n");
    Osal_printf ("\tExamples:\n");
    Osal_printf ("\t\t./messageQApp.out 1: MessageQ sample with SysM3\n");
    Osal_printf ("\t\t./messageQApp.out 2: MessageQ sample with AppM3\n");
    Osal_printf ("\t\t./messageQApp.out 3: MessageQ sample with Tesla\n");

    return;
}


int
main (int argc, char ** argv)
{
    Int     status                          = 0;
    Char *  trace                           = FALSE;
    Bool    MessageQApp_enableTrace         = FALSE;
    Char *  traceEnter                      = FALSE;
    Bool    MessageQApp_enableTraceEnter    = FALSE;
    Char *  traceFailure                    = FALSE;
    Bool    MessageQApp_enableTraceFailure  = FALSE;
    Char *  traceClass                      = NULL;
    UInt32  MessageQApp_traceClass          = 0;
    Int     MessageQApp_testNo              = 0;
    Bool    validTest                       = TRUE;

    Osal_printf ("MessageQApp MPU - Ducati sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        MessageQApp_enableTrace = strtol (trace, NULL, 16);
        if ((MessageQApp_enableTrace != 0) && (MessageQApp_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (MessageQApp_enableTrace == TRUE) {
            Osal_printf ("Trace enabled\n");
            curTrace = GT_TraceState_Enable;
        }
        else if (MessageQApp_enableTrace == FALSE) {
            Osal_printf ("Trace disabled\n");
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        MessageQApp_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (MessageQApp_enableTraceEnter != 0)
            &&  (MessageQApp_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (MessageQApp_enableTraceEnter == TRUE) {
            Osal_printf ("Trace entry/leave prints enabled\n");
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        MessageQApp_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (MessageQApp_enableTraceFailure != 0)
            &&  (MessageQApp_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (MessageQApp_enableTraceFailure == TRUE) {
            Osal_printf ("Trace SetFailureReason enabled\n");
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        MessageQApp_traceClass = strtol (traceClass, NULL, 16);
        if (    (MessageQApp_traceClass != 1)
            &&  (MessageQApp_traceClass != 2)
            &&  (MessageQApp_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            Osal_printf ("Trace class %s\n", traceClass);
            MessageQApp_traceClass =
                            MessageQApp_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= MessageQApp_traceClass;
        }
    }

    switch (argc) {
        case 2:
            MessageQApp_testNo = atoi (argv[1]);
            if (MessageQApp_testNo < 1 || MessageQApp_testNo > 3) {
                validTest = FALSE;
            }
            break;

        case 1:
            MessageQApp_testNo = 1;
            break;

        default:
            validTest = FALSE;
            break;
    }

    if (validTest == FALSE) {
        status = -1;
        printUsage ();
    }
    else {
        status = MessageQApp_startup (MessageQApp_testNo);

        if (status >= 0) {
            MessageQApp_execute (MessageQApp_testNo);
        }

        MessageQApp_shutdown (MessageQApp_testNo);
    }

    /* Trace for TITAN support */
    if(status < 0)
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
