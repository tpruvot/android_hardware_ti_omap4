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
 *  @file   SlpmTestOS.c
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
#include <sys/select.h>

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
#include "slpmtestApp_config.h"


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


typedef union Payload {
    struct {
        unsigned pmMasterCore :8;
        unsigned rendezvousResume:8;
        unsigned force_suspend:8;
        unsigned MaintCmd:8;
    };
    UInt32 whole;
} Payload;

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define MESSAGEQ_DRIVER_NAME         "/dev/syslinkipc/MessageQ"
#define TEST_ON_SYSM3                1
#define TEST_ON_APPM3                2

#define TEST_ERROR                  -1
#define TEST_SUCCESS                 0

#define MENU_TEST                    0
#define AUTOMATED_TEST               1


#define LASTVALIDCMD                 34

#define FORCED_EXIT                  100

Int Cmd2Mask[LASTVALIDCMD] ={
    NOCMD,                     /* 0 */
    REQUEST,                   /* 1.-Request Resource */
    REGISTER,                  /* 2.-Register Resource Notification Callback */
    VALIDATE,                  /* 3.-Validate Resource */
    UNREGISTER,                /* 4.-Unregister Resource Notification Callback*/
    RELEASE,                   /* 5.-Release Resource */
    SUSPEND,                   /* 6.-Send Suspend Notification */
    RESUME,                    /* 7.-Send Resume Notification */
    DUMPRCB,                   /* 8.-Dump RCB Content */
    LIST,                      /* 9.-List Resources */
    REQUEST_REGISTER,          /* 10.-Request-Register Resource */
    REQUEST_VALIDATE,          /* 11.-Request-Validate Resource */
    REGISTER_VALIDATE,         /* 12.-Register-Validate Resource */
    REQUEST_REGISTER_VALIDATE, /* 13.-Request-Register-Validate Resource */
    UNREGISTER_RELEASE,        /* 14.-Unregister-Release Resource */
    MULTITHREADS,              /* 15.-Run MultiThread Test */
    ENTER_I2C_SPINLOCK,        /* 16.-Enter I2C Spinlock */
    LEAVING_I2C_SPINLOCK,      /* 17.-Leaving I2C Spinlock */
    MULTITHREADS_I2C,          /* 18.-3 Threads Entering Gate */
    MULTI_TASK_I2C,            /* 19.-3 Tasks Entering Gate */
    EXIT,                      /* 20.-Exit */
    NEWFEATURE,                /* 21 */
    IDLEWFION,                 /* 22 */
    IDLEWFIOFF,                /* 23 */
    COUNTIDLES,                /* 24 */
    PAUL_01,                   /* 25 */
    PAUL_02,                   /* 26 */
    PAUL_03,                   /* 27 */
    PAUL_04,                   /* 28 */
    TIMER,                     /* 29 */
    PWR_SUSPEND,               /* 30 */
    NOTIFY_SYSM3,              /* 31 */
    ENABLE_HIBERNATION,        /* 32 */
    SET_CONSTRAINTS            /* 33 */
};



/** ============================================================================
 *  Extern declarations
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for SlpmTest sample application
 */
extern Int SlpmTest_startup (Int testNo);

/*!
 *  @brief  Function to execute the execute for SlpmTest sample application
 */
extern Int SlpmTest_execute (Int testNo, Int cmd, Int resource, Int* resArg);

/*!
 *  @brief  Function to execute the shutdown for SlpmTest sample app
 */
extern Int SlpmTest_shutdown (Int testNo);


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  ProcMgr handle.
 */

/** ============================================================================
 *  Functions
 *  ============================================================================
 */
Void printUsage (Void)
{
    Osal_printf ("\nUsage: ./slpmtest.out <RemoteProc> <cmd> <Resource> <arg0> "
                 "<arg1> .. <arg9>\n");
    Osal_printf ("Valid Values:\n\tRemoteProc: 1 or 2 (default = 1)\n");
    Osal_printf ("\n\t\tCmd = Type of test. Check TestSpec");
    Osal_printf ("\n\t\tResource:\n");
    Osal_printf ("  0.-  FDIF\n");
    Osal_printf ("  1.-  IPU\n");
    Osal_printf ("  2.-  SYSM3\n");
    Osal_printf ("  3.-  APPM3\n");
    Osal_printf ("  4.-  ISS\n");
    Osal_printf ("  5.-  IVAHD\n");
    Osal_printf ("  6.-  IVASEQ0\n");
    Osal_printf ("  7.-  IVASEQ1\n");
    Osal_printf ("  8.-  L3 Bus\n");
    Osal_printf ("  9.-  MPU\n");
    Osal_printf ("  10.- Sdma\n");
    Osal_printf ("  11.- Gptimer\n");
    Osal_printf ("  12.- Gpio\n");
    Osal_printf ("  13.- I2C\n");
    Osal_printf ("  14.- Regulator\n");
    Osal_printf ("  15.- Auxiliar Clock\n");
    Osal_printf ("\n\tExamples:");
    Osal_printf ("\n\t\t./SlpmTest.out 1: SLPM Menu Test for SysM3\n");
    Osal_printf ("\n\t\t./SlpmTest.out 2: SLPM Menu Test for AppM3\n");
    Osal_printf ("\n\t\t./SlpmTest.out 1 7 11: Request-Register-Validate "
                 "Gptimer on SysM3\n");

    return;
}

Void setTracingEnv (Void)
{
    Char *  trace                        = FALSE;
    Bool    SlpmTest_enableTrace         = FALSE;
    Char *  traceEnter                   = FALSE;
    Bool    SlpmTest_enableTraceEnter    = FALSE;
    Char *  traceFailure                 = FALSE;
    Bool    SlpmTest_enableTraceFailure  = FALSE;
    Char *  traceClass                   = NULL;
    UInt32  SlpmTest_traceClass          = 0;

    trace = getenv ("TRACE");
    /* Enable/disable levels of tracing. */
    if (trace != NULL) {
        SlpmTest_enableTrace = strtol (trace, NULL, 16);
        if ((SlpmTest_enableTrace != 0) && (SlpmTest_enableTrace != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACE\n") ;
        }
        else if (SlpmTest_enableTrace == TRUE) {
            Osal_printf ("Trace enabled\n");
            curTrace = GT_TraceState_Enable;
        }
        else if (SlpmTest_enableTrace == FALSE) {
            Osal_printf ("Trace disabled\n");
            curTrace = GT_TraceState_Disable;
        }
    }

    traceEnter = getenv ("TRACEENTER");
    if (traceEnter != NULL) {
        SlpmTest_enableTraceEnter = strtol (traceEnter, NULL, 16);
        if (    (SlpmTest_enableTraceEnter != 0)
            &&  (SlpmTest_enableTraceEnter != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n") ;
        }
        else if (SlpmTest_enableTraceEnter == TRUE) {
            Osal_printf ("Trace entry/leave prints enabled\n");
            curTrace |= GT_TraceEnter_Enable;
        }
    }

    traceFailure = getenv ("TRACEFAILURE");
    if (traceFailure != NULL) {
        SlpmTest_enableTraceFailure = strtol (traceFailure, NULL, 16);
        if (    (SlpmTest_enableTraceFailure != 0)
            &&  (SlpmTest_enableTraceFailure != 1)) {
            Osal_printf ("Error! Only 0/1 supported for TRACEENTER\n");
        }
        else if (SlpmTest_enableTraceFailure == TRUE) {
            Osal_printf ("Trace SetFailureReason enabled\n");
            curTrace |= GT_TraceSetFailure_Enable;
        }
    }

    traceClass = getenv ("TRACECLASS");
    if (traceClass != NULL) {
        SlpmTest_traceClass = strtol (traceClass, NULL, 16);
        if (    (SlpmTest_traceClass != 1)
            &&  (SlpmTest_traceClass != 2)
            &&  (SlpmTest_traceClass != 3)) {
            Osal_printf ("Error! Only 1/2/3 supported for TRACECLASS\n");
        }
        else {
            Osal_printf ("Trace class %s\n", traceClass);
            SlpmTest_traceClass =
                         SlpmTest_traceClass << (32 - GT_TRACECLASS_SHIFT);
            curTrace |= SlpmTest_traceClass;
        }
    }
}

Int displayMainMenu (Void)
{
    Char str [80];

    Osal_printf ("\n\nPress Enter to continue...\n");
    fgets(str, 80, stdin);
    system("clear");
    Osal_printf ("\n\n============ Main Menu ============\n\n");
    Osal_printf ("  1.- Request Resource\n");
    Osal_printf ("  2.- Register Resource Notification Callback\n");
    Osal_printf ("  3.- Validate Resource\n");
    Osal_printf ("  4.- Unregister Resource Notification Callback\n");
    Osal_printf ("  5.- Release Resource\n");
    Osal_printf ("  6.- Send Suspend Notification\n");
    Osal_printf ("  7.- Send Resume Notification\n");
    Osal_printf ("  8.- Dump RCB Content\n");
    Osal_printf ("  9.- List Resources\n");
    Osal_printf ("\n==== Combinations ====\n");
    Osal_printf ("  10.- Request-Register Resource\n");
    Osal_printf ("  11.- Request-Validate Resource\n");
    Osal_printf ("  12.- Register-Validate Resource\n");
    Osal_printf ("  13.- Request-Register-Validate Resource\n");
    Osal_printf ("\n");
    Osal_printf ("  14.- Unregister-Release Resource\n");
    Osal_printf ("\n==== Other Options ====\n");
    Osal_printf ("  15.- Run MultiThread Test\n");
    Osal_printf ("  16.- Enter I2C Spinlock\n");
    Osal_printf ("  17.- Leaving I2C Spinlock\n");
    Osal_printf ("  18.- 3 Threads Entering Gate\n");
    Osal_printf ("  19.- 3 Tasks Entering Gate\n");
    Osal_printf ("\n");
    Osal_printf ("  20.- Exit\n");
    Osal_printf ("\n");
    Osal_printf ("  22.- WFI ON during Idle\n");
    Osal_printf ("  23.- WFI OFF during Idle\n");
    Osal_printf ("  24.- Count Idles in 10 Ticks\n");
    Osal_printf ("  25.- PAUL_01\n");
    Osal_printf ("  26.- PAUL_02\n");
    Osal_printf ("  27.- PAUL_03\n");
    Osal_printf ("  28.- PAUL_04\n");
    Osal_printf ("  29.- TIMER\n");
    Osal_printf ("  30.- PWR_SUSPEND\n");
    Osal_printf ("  31.- Notify_SYSM3\n");
    Osal_printf ("  32.- Enable Hiernation\n");
    Osal_printf ("  33.- Set Constraints\n");
    Osal_printf ("  100.- Forced Exit\n");
    Osal_printf ("\n");
    Osal_printf ("  Select one option:   ");
    fgets(str, 80, stdin);

    return atoi(str);
}


Int displayResMenu (Void)
{
    Char str [80];

    Osal_printf ("\n\n============ Request Resource Menu ============\n");

    Osal_printf ("  0.-  FDIF\n");
    Osal_printf ("  1.-  IPU\n");
    Osal_printf ("  2.-  SYSM3\n");
    Osal_printf ("  3.-  APPM3\n");
    Osal_printf ("  4.-  ISS\n");
    Osal_printf ("  5.-  IVAHD\n");
    Osal_printf ("  6.-  IVASEQ0\n");
    Osal_printf ("  7.-  IVASEQ1\n");
    Osal_printf ("  8.-  L3 Bus\n");
    Osal_printf ("  9.-  MPU\n");
    Osal_printf ("  10.- Sdma\n");
    Osal_printf ("  11.- Gptimer\n");
    Osal_printf ("  12.- Gpio\n");
    Osal_printf ("  13.- I2C\n");
    Osal_printf ("  14.- Regulator\n");
    Osal_printf ("  15.- Auxiliar Clock\n");
    fgets(str, 80, stdin);

    return atoi(str);
}

Int displayRemoteResourceList (Int SlpmTestOn, Int resource, Int* resArg)
{
    Char str [80];

    Osal_printf ("\n\n============ Release Menu ============\n");
    SlpmTest_execute (SlpmTestOn, LIST, resource, resArg);
    Osal_printf ("\n Make your choice (-1 = All):\n");
    fgets(str, 80, stdin);

    return atoi(str);
}

Int kbhit (Void)
{
    struct timeval tv;

    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

    return FD_ISSET(STDIN_FILENO, &fds);
}

Void PrintMessageUntilPressEnter (Int SlpmTestOn, Int* resArg)
{
    while(!kbhit()) {
        sleep(1);
        system("clear");
        SlpmTest_execute (SlpmTestOn, NEWFEATURE, -1, resArg);
        Osal_printf ("Load Monitor: \n");
    }
}

Int getConstrMask (Int resource, Int* resArg)
{
    Char    str [80];

    Osal_printf ("\n\n Request_constr =1 Release_constr =0 \n");
    fgets(str, 80, stdin);
    resArg[2] = atoi(str);
    Osal_printf ("\n\n Constr Mask \n");
    Osal_printf ("MHZ                       0\n");
    Osal_printf ("USEC                      1\n");
    Osal_printf ("BANDWITH                  2\n");
    Osal_printf ("MHZ  | USEC               3\n");
    Osal_printf ("MHZ  | BANDWITH           4\n");
    Osal_printf ("USEC | BANDWITH           5\n");
    Osal_printf ("MHZ  | USEC | BANDWITH    6\n");
    fgets(str, 80, stdin);
    resArg[3] = atoi(str);

    return 0;

}


Int getResourceParam (Int resource, Int* resArg)
{
    Char    str [80];
    Payload payload;

    switch (resource) {

        case FDIF:
        case IPU:
        case ISS:
        case IVA_HD:
        case SYSM3:
        case APPM3:
        case MPU:
            Osal_printf ("\n\n Performance (KHz)?\n");
            fgets(str, 80, stdin);
            resArg[0] = atoi(str);
            Osal_printf ("\n\n Latency (uSec)\n");
            fgets(str, 80, stdin);
            resArg[1] = atoi(str);
            break;

        case L3_BUS:
            Osal_printf ("\n\n Bandwidth\n");
            fgets(str, 80, stdin);
            resArg[0] = atoi(str);
            Osal_printf ("\n\n Latency (uSec)\n");
            fgets(str, 80, stdin);
            resArg[1] = atoi(str);
        break;

        case REGULATOR:
            Osal_printf ("\n\n Provide Resource ID?\n");
            fgets(str, 80, stdin);
            resArg[0] = atoi(str);
            Osal_printf ("\n\n Minimum Voltage?\n");
            fgets(str, 80, stdin);
            resArg[1] = atoi(str);
            Osal_printf ("\n\n Maximum Voltage?\n");
            fgets(str, 80, stdin);
            resArg[2] = atoi(str);
            break;


        case GP_IO:
        case I2C:
            Osal_printf ("\n\n Provide Resource ID?\n");
            goto OnlyOneArg;
        case SDMA:
            Osal_printf ("\n\n Number of Channels?\n");
            goto OnlyOneArg;
        case AUX_CLK:
            Osal_printf ("\n\n Number of Clk?\n");
            goto OnlyOneArg;

        case TIMER:
            Osal_printf ("\n\n Tick Period (uSec)?\n");
            goto OnlyOneArg;

        case PWR_SUSPEND:
            Osal_printf ("\n\n Is MasterCore (SYSM3=1, APPM3=0)?\n");
            fgets(str, 80, stdin);
            resArg[0] = atoi(str);
            Osal_printf ("\n\n rendezvousResume? (TRUE=1, FALSE = 0)\n");
            fgets(str, 80, stdin);
            resArg[1] = atoi(str);
            Osal_printf ("\n\n MaintCmd? (Clean =0, G_FLUSH = 1, "
                         "INV_CLEAN =2)\n");
            fgets(str, 80, stdin);
            resArg[2] = atoi(str);
            Osal_printf ("\n\n force Suspend? 0-Nothing, 1-Idle 2-Swi "
                         "3-Task Context\n");
            fgets(str, 80, stdin);
            resArg[3] = atoi(str);
            break;

        case NOTIFY_SYSM3:
            Osal_printf ("\n\n Is MasterCore (SYSM3=1, APPM3=0)?\n");
            fgets(str, 80, stdin);
            payload.pmMasterCore = atoi(str);
            Osal_printf ("\n\n rendezvousResume? (TRUE=1, FALSE = 0)\n");
            fgets(str, 80, stdin);
            payload.rendezvousResume = atoi(str);
            Osal_printf ("\n\n MaintCmd? (Clean =0, G_FLUSH = 1, "
                         "INV_CLEAN =2)\n");
            fgets(str, 80, stdin);
            payload.MaintCmd=atoi(str);
            Osal_printf ("\n\n force Suspend? 0-Nothing, 1-Idle 2-Swi "
                         "3-Task Context\n");
            fgets(str, 80, stdin);
            payload.force_suspend = atoi(str);
            resArg[0] = payload.whole;
            break;

        case ENABLE_HIBERNATION:
            Osal_printf ("\n\n Enable=1 Disable=0 \n");
            goto OnlyOneArg;
            break;

        case IVASEQ0:
        case IVASEQ1:
        case GP_TIMER:
            break;
        default:
            Osal_printf ("  Resource not supported yet\n");
            break;
        }
    return 0;

OnlyOneArg:
    fgets(str, 80, stdin);
    resArg[0] = atoi(str);

    return 0;
}



Int main (Int argc, Char ** argv)
{
    Int     status                      = 0;
    Int     slpmStatus                  = 0;
    Bool    validTest                   = TRUE;
    Int     SlpmTestType;
    Int     SlpmTestOn                  = TEST_ON_APPM3;
    Int     i;
    Int     cmd                         = NOCMD;
    Int     resource                    = NORESOURCE;
    Int     resArg[MAX_NUM_ARGS];

    setTracingEnv ();

    SlpmTestType = MENU_TEST;

    if (argc > 1) {
        SlpmTestOn = atoi (argv[1]);
        if (SlpmTestOn != TEST_ON_SYSM3 && SlpmTestOn != TEST_ON_APPM3) {
            validTest = FALSE;
        }
    }
    else {
        SlpmTestOn = TEST_ON_SYSM3;
    }
    if (argc == 3) {
        validTest = FALSE;
    }
    if (argc > 3) {
        cmd = atoi (argv[2]);
        resource = atoi (argv[3]);
        SlpmTestType = AUTOMATED_TEST;
    }
    if (argc > 4) {
        for (i = 4; i < argc; i++) {
            resArg[i-4] = atoi(argv[i]);
        }
    }

    if (validTest == FALSE) {
        status = TEST_ERROR;
        printUsage ();
    }
    else {
        status = SlpmTest_startup (SlpmTestOn);
        if (status == TEST_ERROR) {
            goto shutdown;
        }

        if (SlpmTestType == AUTOMATED_TEST) {
            if (cmd == MULTITHREADS) {
                for (i=0; i< 500; i++) {
                    status = SlpmTest_execute (SlpmTestOn, cmd, resource,
                                                resArg);
                    if (status) {
                        slpmStatus = status;
                        break;
                    }
                }
            } else {
                status = SlpmTest_execute (SlpmTestOn, cmd, resource, resArg);
                if (status)
                    slpmStatus = status;
                sleep(1);
            }
            status = SlpmTest_execute (SlpmTestOn, EXIT, resource, resArg);
            if (slpmStatus)
                status = slpmStatus;
            goto shutdown;
        }

        do {
            resource = NORESOURCE;
            cmd      = NOCMD;
            resArg[0]   = NOVALID;

            cmd = displayMainMenu ();
            if ((cmd >= NOCMD) && (cmd < LASTVALIDCMD)) {
                cmd = Cmd2Mask[cmd];
            }
            else if(cmd == FORCED_EXIT){
                goto shutdown;
            }
            else {
                cmd = NOCMD;
            }

            switch(cmd) {
                case REQUEST:
                case REQUEST_REGISTER:
                case REQUEST_VALIDATE:
                case REQUEST_REGISTER_VALIDATE:
                    resource = displayResMenu();
                    getResourceParam (resource,resArg);
                    break;
                case REGISTER:
                case VALIDATE:
                case UNREGISTER:
                case RELEASE:
                case UNREGISTER_RELEASE:
                case ENTER_I2C_SPINLOCK:
                case LEAVING_I2C_SPINLOCK:
                case MULTITHREADS_I2C:
                case MULTI_TASK_I2C:
                    resource = displayRemoteResourceList (SlpmTestOn, resource,
                                                            resArg);
                    break;
                case SET_CONSTRAINTS:
                    resource = displayResMenu();
                    if(resource <= LAST_CONSTR_RES) {
                        getResourceParam (resource,resArg);
                        getConstrMask(resource,resArg);
                        resource = displayRemoteResourceList (SlpmTestOn,
                                                        NORESOURCE,resArg);
                    }
                    else {
                        /* Abort the Cmd Submision */
                        cmd = NOCMD;
                    }
                    break;
                case NEWFEATURE:
                     PrintMessageUntilPressEnter (SlpmTestOn, resArg);
                     continue;
                     break;
                case TIMER:
                     resource = TIMER;
                     getResourceParam (resource, resArg);
                     resource = NORESOURCE;
                    break;
                case PWR_SUSPEND:
                    resource = PWR_SUSPEND;
                    getResourceParam (resource, resArg);
                    resource = NORESOURCE;
                    break;
                case NOTIFY_SYSM3:
                    resource = NOTIFY_SYSM3;
                    getResourceParam (resource, resArg);
                    resource = NORESOURCE;
                    break;
                case ENABLE_HIBERNATION:
                    resource = ENABLE_HIBERNATION;
                    getResourceParam (resource, resArg);
                    resource = NORESOURCE;
                default:
                    break;
             }
             if (cmd != NOCMD) {
                Osal_printf ("\n SlpmTest_execute SlpmTest On ProcId(%d) "
                             "cmd(%d) Res(%d) arg(%x) &arg(%x)\n", SlpmTestOn,
                             cmd, resource, resArg, &resArg);
                status = SlpmTest_execute (SlpmTestOn, cmd, resource, resArg);
                if (status)
                        Osal_printf ("status=%d\n", status);
                if (cmd == MULTITHREADS) {
                    for (i=0; i< 500; i++) {
                        status = SlpmTest_execute (SlpmTestOn, cmd, resource,
                                                    resArg);
                        if (status)
                                Osal_printf ("status=%d for i=%d\n", status, i);
                    }
                }
             }
        } while (cmd != EXIT);

shutdown:
        SlpmTest_shutdown (SlpmTestOn);
    }

    /* Trace for TITAN support */
    if (status)
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");
    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
