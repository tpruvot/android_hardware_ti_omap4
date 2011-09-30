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
 *  @file   SharedRegionAppOS.c
 *
 *  @brief      OS-specific sample application driver module for Notify module
 *
 *  ============================================================================
 */

/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>
#include <SharedRegionApp.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define NOTIFY_DRIVER_NAME         "/dev/syslinkipc/Notify"


/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for sharedRegionApp sample application
 */
extern Int sharedRegionApp_startup (Void);
/*!
 *  @brief  Function to execute the startup for sharedRegionApp sample application
 */
extern Int sharedRegionApp_execute (Void);

/*!
 *  @brief  Function to execute the shutdown for sharedRegionApp sample application
 */
extern Int sharedRegionApp_shutdown (Void);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
int
main (int argc, char ** argv)
{
    Char *  trace                           = FALSE;
    Bool    sharedRegionApp_enableTrace          = FALSE;
    Char *  traceEnter                      = FALSE;
    Bool    sharedRegionApp_enableTraceEnter     = FALSE;
    Char *  traceFailure                    = FALSE;
    Bool    sharedRegionApp_enableTraceFailure   = FALSE;
    Char *  traceClass                      = NULL;
    UInt32  sharedRegionApp_traceClass           = 0;

    printf ("sharedRegionApp sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        Osal_printf ("Trace enable %s\n", trace) ;
        sharedRegionApp_enableTrace = strtol (trace, NULL, 16);
        if ((sharedRegionApp_enableTrace != 0) && (sharedRegionApp_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (sharedRegionApp_enableTrace == TRUE) {
            curTrace = GT_TraceState_Enable;
        }
        else if (sharedRegionApp_enableTrace == FALSE) {
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        Osal_printf ("Trace entry/leave prints enable %s\n", traceEnter) ;
        sharedRegionApp_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (sharedRegionApp_enableTraceEnter != 0)
            &&  (sharedRegionApp_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (sharedRegionApp_enableTraceEnter == TRUE) {
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        Osal_printf ("Trace SetFailureReason enable %s\n", traceFailure) ;
        sharedRegionApp_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (sharedRegionApp_enableTraceFailure != 0)
            &&  (sharedRegionApp_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (sharedRegionApp_enableTraceFailure == TRUE) {
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        Osal_printf ("Trace class %s\n", traceClass);
        sharedRegionApp_traceClass = strtol (traceClass, NULL, 16);
        if (    (sharedRegionApp_enableTraceFailure != 1)
            &&  (sharedRegionApp_enableTraceFailure != 2)
            &&  (sharedRegionApp_enableTraceFailure != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            sharedRegionApp_traceClass =
                            sharedRegionApp_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= sharedRegionApp_traceClass;
        }
    }

    sharedRegionApp_startup ();
    sharedRegionApp_execute ();
    sharedRegionApp_shutdown ();

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
