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
 *  @file   slpmtransportOS.c
 *
 *  @brief  OS-specific sample application driver module for slpm module
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
#include "slpmtransportApp_config.h"

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for SlpmTransport sample application
 */
extern Int SlpmTransport_startup (Int testNo);

/*!
 *  @brief  Function to execute the execute for SlpmTransport sample application
 */
extern Int SlpmTransport_execute (Int testNo);

/*!
 *  @brief  Function to execute the shutdown for SlpmTransport sample app
 */
extern Int SlpmTransport_shutdown (Int testNo);


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  ProcMgr handle.
 */
//ProcMgr_Handle SlpmTransport_procMgrHandle = NULL;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Void printUsage (Void)
{
    Osal_printf ("Usage: ./slpmtransport.out [<TestNo>]\n");
    Osal_printf ("\tValid Values:\n\t\tTestNo: 1 or 2 (default = 1)\n");
    Osal_printf ("\tExamples:\n");
    Osal_printf ("\t\t./slpmtransport.out 1: slpmtransport sample with SysM3\n");
    Osal_printf ("\t\t./slpmtransport.out 2: slpmtransport sample with AppM3\n");

    return;
}
int
main (int argc, char ** argv)
{
    Int     status                          = 0;
    Char *  trace                           = FALSE;
    Bool    slpmTransport_enableTrace         = FALSE;
    Char *  traceEnter                      = FALSE;
    Bool    slpmTransport_enableTraceEnter    = FALSE;
    Char *  traceFailure                    = FALSE;
    Bool    slpmTransport_enableTraceFailure  = FALSE;
    Char *  traceClass                      = NULL;
    UInt32  slpmTransport_traceClass          = 0;
    Int     slpmTransport_testNo              = 0;
    Bool    validTest                       = TRUE;

    Osal_printf ("PM Transport MPU - AppM3 sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        slpmTransport_enableTrace = strtol (trace, NULL, 16);
        if ((slpmTransport_enableTrace != 0) && (slpmTransport_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (slpmTransport_enableTrace == TRUE) {
            Osal_printf ("Trace enabled\n");
            curTrace = GT_TraceState_Enable;
        }
        else if (slpmTransport_enableTrace == FALSE) {
            Osal_printf ("Trace disabled\n");
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        slpmTransport_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (slpmTransport_enableTraceEnter != 0)
            &&  (slpmTransport_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (slpmTransport_enableTraceEnter == TRUE) {
            Osal_printf ("Trace entry/leave prints enabled\n");
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        slpmTransport_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (slpmTransport_enableTraceFailure != 0)
            &&  (slpmTransport_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (slpmTransport_enableTraceFailure == TRUE) {
            Osal_printf ("Trace SetFailureReason enabled\n");
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        slpmTransport_traceClass = strtol (traceClass, NULL, 16);
        if (    (slpmTransport_traceClass != 1)
            &&  (slpmTransport_traceClass != 2)
            &&  (slpmTransport_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            Osal_printf ("Trace class %s\n", traceClass);
            slpmTransport_traceClass =
                            slpmTransport_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= slpmTransport_traceClass;
        }
    }

    switch (argc) {
        case 2:
            slpmTransport_testNo = atoi (argv[1]);
            if (slpmTransport_testNo != 1 && slpmTransport_testNo != 2) {
                validTest = FALSE;
            }
            break;

        case 1:
            slpmTransport_testNo = 1;
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
        status = SlpmTransport_startup (slpmTransport_testNo);

        if (status >= 0) {
        SlpmTransport_execute (slpmTransport_testNo);
        }
    SlpmTransport_shutdown (slpmTransport_testNo);
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
