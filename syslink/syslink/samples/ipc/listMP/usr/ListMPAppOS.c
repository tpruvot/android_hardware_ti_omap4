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
 *  @file   ListMPAppOS.c
 *
 *  @brief  OS-specific sample application driver module for Notify module
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

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define NOTIFY_DRIVER_NAME         "/dev/syslinkipc/Notify"

/*!
 *  @brief  shared memory base address
 */
#define SHAREDMEM               0xA0000000



/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for ListMPApp sample application
 */
extern Int ListMPApp_startup (UInt32 sharedAddr);

/*!
 *  @brief  Function to execute the startup for ListMPApp sample application
 */
extern Int ListMPApp_execute (UInt32 sharedAddr);

/*!
 *  @brief  Function to execute the shutdown for ListMPApp sample application
 */
extern Int ListMPApp_shutdown (Void);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
int
main (int argc, char ** argv)
{
    Char *  trace                          = FALSE;
    Bool    ListMPApp_enableTrace          = FALSE;
    Char *  traceEnter                     = FALSE;
    Bool    ListMPApp_enableTraceEnter     = FALSE;
    Char *  traceFailure                   = FALSE;
    Bool    ListMPApp_enableTraceFailure   = FALSE;
    Char *  traceClass                     = NULL;
    UInt32  ListMPApp_traceClass           = 0;
    UInt32  ListMPApp_shAddrBase           = 0;
    Int32   status;


    printf ("ListMPApp sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        Osal_printf ("Trace enable %s\n", trace) ;
        ListMPApp_enableTrace = strtol (trace, NULL, 16);
        if ((ListMPApp_enableTrace != 0) && (ListMPApp_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (ListMPApp_enableTrace == TRUE) {
            curTrace = GT_TraceState_Enable;
        }
        else if (ListMPApp_enableTrace == FALSE) {
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        Osal_printf ("Trace entry/leave prints enable %s\n", traceEnter) ;
        ListMPApp_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (ListMPApp_enableTraceEnter != 0)
            &&  (ListMPApp_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (ListMPApp_enableTraceEnter == TRUE) {
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        Osal_printf ("Trace SetFailureReason enable %s\n", traceFailure) ;
        ListMPApp_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (ListMPApp_enableTraceFailure != 0)
            &&  (ListMPApp_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (ListMPApp_enableTraceFailure == TRUE) {
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        Osal_printf ("Trace class %s\n", traceClass);
        ListMPApp_traceClass = strtol (traceClass, NULL, 16);
        if (    (ListMPApp_traceClass != 1)
            &&  (ListMPApp_traceClass != 2)
            &&  (ListMPApp_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            ListMPApp_traceClass =
                            ListMPApp_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= ListMPApp_traceClass;
        }
    }

    status = ListMPApp_startup (ListMPApp_shAddrBase);

    if (status >= 0) {
        status = ListMPApp_execute (ListMPApp_shAddrBase);
    }

    status = ListMPApp_shutdown ();

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
