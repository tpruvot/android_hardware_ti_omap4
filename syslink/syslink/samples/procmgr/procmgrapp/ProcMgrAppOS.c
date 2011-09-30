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

/*!
 *  @file       ProcMgrAppOS.c
 *
 *  @brief      OS-specific sample application driver module for ProcMgr module
 *
 *  ============================================================================
 *
 */

/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for ProcMgrApp sample application
 */
extern Int ProcMgrApp_startup ();

/*!
 *  @brief  Function to execute the shutdown for ProcMgrApp sample application
 */
extern Int ProcMgrApp_shutdown (Void);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
int
main (int argc, char ** argv)
{
    Char *  trace                           = FALSE;
    Bool    ProcMgrApp_enableTrace          = FALSE;
    Char *  traceEnter                      = FALSE;
    Bool    ProcMgrApp_enableTraceEnter     = FALSE;
    Char *  traceFailure                    = FALSE;
    Bool    ProcMgrApp_enableTraceFailure   = FALSE;
    Char *  traceClass                      = NULL;
    UInt32  ProcMgrApp_traceClass           = 0;
    Char  * filePath                        = NULL;

    printf ("ProcMgrApp sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        Osal_printf ("Trace enable %s\n", trace) ;
        ProcMgrApp_enableTrace = strtol (trace, NULL, 16);
        if ((ProcMgrApp_enableTrace != 0) && (ProcMgrApp_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (ProcMgrApp_enableTrace == TRUE) {
        }
        else if (ProcMgrApp_enableTrace == FALSE) {
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        Osal_printf ("Trace entry/leave prints enable %s\n", traceEnter) ;
        ProcMgrApp_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (ProcMgrApp_enableTraceEnter != 0)
            &&  (ProcMgrApp_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (ProcMgrApp_enableTraceEnter == TRUE) {
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        Osal_printf ("Trace SetFailureReason enable %s\n", traceFailure) ;
        ProcMgrApp_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (ProcMgrApp_enableTraceFailure != 0)
            &&  (ProcMgrApp_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (ProcMgrApp_enableTraceFailure == TRUE) {
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        Osal_printf ("Trace class %s\n", traceClass) ;
        ProcMgrApp_traceClass = strtol (traceClass, NULL, 16);
        if (    (ProcMgrApp_enableTraceFailure != 1)
            &&  (ProcMgrApp_enableTraceFailure != 2)
            &&  (ProcMgrApp_enableTraceFailure != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n") ;
        }
        else {
            ProcMgrApp_traceClass =
                            ProcMgrApp_traceClass << (32 - GT_TRACECLASS_SHIFT);
        }
    }

        filePath = argv [1];
        ProcMgrApp_startup ();
        ProcMgrApp_shutdown ();


    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
