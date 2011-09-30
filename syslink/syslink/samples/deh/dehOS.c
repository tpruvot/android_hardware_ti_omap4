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
 *  @file   dehOS.c
 *
 *  @brief  OS-specific sample application for testing DEH functionality
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
#include <UsrUtilsDrv.h>

/* Application header */
#include "dehApp_config.h"

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
 *  @brief  Function to execute the startup for DEH sample application
 */
extern Int deh_startup (Int testNo);

/*!
 *  @brief  Function to execute the execute for DEH sample application
 */
extern Int deh_execute (Int testNo, Int cmd, Int* arg);

/*!
 *  @brief  Function to execute the shutdown for DEH sample app
 */
extern Int deh_shutdown (Int testNo);


/** ============================================================================
 *  Globals
 *  ============================================================================
 */


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Void printUsage (Void)
{
    Osal_printf ("Usage:  ./dehtest.out <TestNo> <Cmd> [<Arg>]\n");
    Osal_printf ("                     or                    \n");
    Osal_printf ("\t./dehdaemontest.out <TestNo> <Cmd> [<Arg>]\n");
    Osal_printf ("\tUse dehdaemontest.out when running alongside SysLink "
                 "Daemon\n");
    Osal_printf ("\n\tValid Values:\n\t\tTestNo: 1 or 2 \n");
    Osal_printf ("\t\t\t 1 = SysM3 2 = AppM3\n");
    Osal_printf ("\t\tCmd: 1 to 6 \n");
    Osal_printf ("\t\t\t 1 = DivByZero  2 = WatchDog      3 = MMU Fault\n");
    Osal_printf ("\t\t\t 4 = AMMU Fault 5 = Read Inv.Addr 6 = Write Inv.Addr\n");
    Osal_printf ("\t\tArg: Required for Cmds 3 to 6\n");
    Osal_printf ("\t\t\t 1 or 2 for Cmds 3 & 4; Hex value for Cmds 5 & 6\n");
    Osal_printf ("\tExamples (all shown without daemon):\n");
    Osal_printf ("\t\t./dehtest.out 1 1: Div By Zero test with SysM3\n");
    Osal_printf ("\t\t./dehTest.out 1 2: Watchdog test with SysM3\n");
    Osal_printf ("\t\t./dehTest.out 2 3 1: MMU Fault read test with AppM3\n");
    Osal_printf ("\t\t./dehTest.out 2 3 2: MMU Fault write test with AppM3\n");
    Osal_printf ("\t\t./dehTest.out 1 4 1: AMMU Fault read test with SysM3\n");
    Osal_printf ("\t\t./dehTest.out 2 4 2: AMMU Fault read test with AppM3\n");
    Osal_printf ("\t\t./dehTest.out 2 5 <value>: Read address <value> test with"
                 "AppM3\n");
    Osal_printf ("\t\t./dehTest.out 1 6 <value>: Write address <value> test"
                 "with SysM3\n");

    return;
}


Int
main (Int argc, Char ** argv)
{
    Int     status                  = 0;
    Char  * trace                   = FALSE;
    Bool    deh_enableTrace         = FALSE;
    Char  * traceEnter              = FALSE;
    Bool    deh_enableTraceEnter    = FALSE;
    Char  * traceFailure            = FALSE;
    Bool    deh_enableTraceFailure  = FALSE;
    Char  * traceClass              = NULL;
    UInt32  deh_traceClass          = 0;
    Int     deh_testNo              = 0;
    Bool    validTest               = TRUE;
    Int32   arg [MAX_NUM_ARGS];
    UInt32  cmd                     = 0;

    Osal_printf ("DEH sample application\n");

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        deh_enableTrace = strtol (trace, NULL, 16);
        if ((deh_enableTrace != 0) && (deh_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (deh_enableTrace == TRUE) {
            Osal_printf ("Trace enabled\n");
            curTrace = GT_TraceState_Enable;
        }
        else if (deh_enableTrace == FALSE) {
            Osal_printf ("Trace disabled\n");
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        deh_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (deh_enableTraceEnter != 0)
            &&  (deh_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (deh_enableTraceEnter == TRUE) {
            Osal_printf ("Trace entry/leave prints enabled\n");
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        deh_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (deh_enableTraceFailure != 0)
            &&  (deh_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (deh_enableTraceFailure == TRUE) {
            Osal_printf ("Trace SetFailureReason enabled\n");
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        deh_traceClass = strtol (traceClass, NULL, 16);
        if (    (deh_traceClass != 1)
            &&  (deh_traceClass != 2)
            &&  (deh_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            Osal_printf ("Trace class %s\n", traceClass);
            deh_traceClass =
                         deh_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= deh_traceClass;
        }
    }

    switch (argc) {
    case 4:
    case 3:
        deh_testNo = atoi (argv [1]);
        if (deh_testNo != 1 && deh_testNo != 2) {
            validTest = FALSE;
        }
        else {
            Osal_printf ("Test with %s\n",
                ((deh_testNo == 1) ? "SysM3": "AppM3"));
        }
        break;
    case 2:
    case 1:
    default:
        validTest = FALSE;
        break;
    }

    if (validTest) {
        UInt32 inCmd = atoi (argv [2]);
        switch (inCmd) {
        case 1:
            Osal_printf ("Division by zero\n");
            cmd = DIV_BY_ZERO;
            break;
        case 2:
            Osal_printf ("Infinite loop \n");
            cmd = INFINITE_LOOP;
            arg[0] = 1;
            break;
        case 3:
            if (argc != 4) {
                validTest = FALSE;
            }
            else {
                UInt32 readWrite = atoi (argv [3]);

                if (readWrite == 1) {
                    Osal_printf ("Causing a MMU Fault by a Read operation\n");
                    cmd = IOMMU_FAULT_BY_READ_OP;
                }
                else if (readWrite == 2) {
                    Osal_printf ("Causing a MMU Fault by a Write operation\n");
                    cmd = IOMMU_FAULT_BY_WRITE_OP;
                }
                else {
                    validTest = FALSE;
                }
            }
            break;
        case 4:
            if (argc != 4) {
                validTest = FALSE;
            }
            else {
                UInt32 readWrite = atoi (argv [3]);

                if (readWrite == 1) {
                    Osal_printf ("Causing a AMMU Fault by a Read operation\n");
                    cmd = AMMU_FAULT_BY_READ_OP;
                }
                else if (readWrite == 2) {
                    Osal_printf ("Causing a AMMU Fault by a Write operation\n");
                    cmd = AMMU_FAULT_BY_WRITE_OP;
                }
                else {
                    validTest = FALSE;
                }
            }
            break;
        case 5:
            if (argc != 4) {
                validTest = FALSE;
            }
            else {
                UInt32 value;
                sscanf(argv[3], "%x", &value);
                arg[0] = value;
                Osal_printf ("Read 0x%x\n", arg[0]);
                cmd = READ;
            }
            break;
        case 6:
            if (argc != 4) {
                validTest = FALSE;
            }
            else {
                UInt32 value;
                sscanf(argv[3], "%x", &value);
                arg[0] = value;
                Osal_printf ("Write 0x%x\n", arg[0]);
                cmd = WRITE;
            }
            break;
        default:
            validTest = FALSE;
            break;
        }
    }

    if (validTest == FALSE) {
        status = -1;
        printUsage ();
    }
    else {
        status = deh_startup (deh_testNo);

        if (status >= 0) {
            status = deh_execute (deh_testNo, cmd, arg);
        }

        deh_shutdown (deh_testNo);
    }

    /* Trace for TITAN support */
    if (status) {
        Osal_printf ("test_case_status=%d\n", status);
    } else {
        Osal_printf ("test_case_status=0\n");
    }

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
