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
 *  @file   slpmresourcesOS.c
 *
 *  @brief  OS-specific sample application driver module for MessageQ module
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
#include "slpmresourcesApp_config.h"

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
 *  @brief  Function to execute the startup for SlpmResources sample application
 */
extern Int SlpmResources_startup (Int testNo);

/*!
 *  @brief  Function to execute the execute for SlpmResources sample application
 */
extern Int SlpmResources_execute (Int testNo);

/*!
 *  @brief  Function to execute the shutdown for SlpmResources sample app
 */
extern Int SlpmResources_shutdown (Int testNo);


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  ProcMgr handle.
 */
//ProcMgr_Handle SlpmResources_procMgrHandle = NULL;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Void printUsage (Void)
{
    Osal_printf ("Usage: ./slpmresources.out [<TestNo>]\n");
    Osal_printf ("\tValid Values:\n\t\tTestNo: 1 or 2 (default = 1)\n");
    Osal_printf ("\tExamples:\n");
    Osal_printf ("\t\t./slpmresources.out 1: MessageQ sample with SysM3\n");
    Osal_printf ("\t\t./slpmresources.out 2: MessageQ sample with AppM3\n");

    return;
}
int
main (int argc, char ** argv)
{
    Int     status                          = 0;
    Char *  trace                           = FALSE;
    Bool    slpmResources_enableTrace         = FALSE;
    Char *  traceEnter                      = FALSE;
    Bool    slpmResources_enableTraceEnter    = FALSE;
    Char *  traceFailure                    = FALSE;
    Bool    slpmResources_enableTraceFailure  = FALSE;
    Char *  traceClass                      = NULL;
    UInt32  slpmResources_traceClass          = 0;
    Int     slpmResources_testNo              = 0;
    Bool    validTest                       = TRUE;

    Osal_printf ("PM Resources MPU - AppM3 sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        slpmResources_enableTrace = strtol (trace, NULL, 16);
        if ((slpmResources_enableTrace != 0) && (slpmResources_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (slpmResources_enableTrace == TRUE) {
            Osal_printf ("Trace enabled\n");
            curTrace = GT_TraceState_Enable;
        }
        else if (slpmResources_enableTrace == FALSE) {
            Osal_printf ("Trace disabled\n");
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        slpmResources_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (slpmResources_enableTraceEnter != 0)
            &&  (slpmResources_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (slpmResources_enableTraceEnter == TRUE) {
            Osal_printf ("Trace entry/leave prints enabled\n");
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        slpmResources_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (slpmResources_enableTraceFailure != 0)
            &&  (slpmResources_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (slpmResources_enableTraceFailure == TRUE) {
            Osal_printf ("Trace SetFailureReason enabled\n");
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        slpmResources_traceClass = strtol (traceClass, NULL, 16);
        if (    (slpmResources_traceClass != 1)
            &&  (slpmResources_traceClass != 2)
            &&  (slpmResources_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            Osal_printf ("Trace class %s\n", traceClass);
            slpmResources_traceClass =
                         slpmResources_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= slpmResources_traceClass;
        }
    }

    switch (argc) {
        case 2:
            slpmResources_testNo = atoi (argv[1]);
            if (slpmResources_testNo != 1 && slpmResources_testNo != 2) {
                validTest = FALSE;
            }
            break;

        case 1:
            slpmResources_testNo = 1;
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
        status = SlpmResources_startup (slpmResources_testNo);

    if (status >= 0) {
        status = SlpmResources_execute (slpmResources_testNo);
    }

    SlpmResources_shutdown (slpmResources_testNo);
    }

    /* Trace for TITAN support */
    if(status)
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");
    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
