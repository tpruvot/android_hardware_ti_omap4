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
 *  @file   NameServerApp.c
 *
 *  @brief  Sample application for NameServer module
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
#include <Memory.h>
#include <String.h>

/* Module level headers */
#if defined (SYSLINK_USE_SYSMGR)
#include <IpcUsr.h>
#else /* if defined (SYSLINK_USE_SYSMGR) */
#include <UsrUtilsDrv.h>
#include <_MultiProc.h>
#include <ti/ipc/MultiProc.h>
#include <_NameServer.h>
#include <ti/ipc/NameServer.h>
#endif /* if defined(SYSLINK_USE_SYSMGR) */

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Maximum name length
 */
#define MAX_NAME_LENGTH 32u
/*!
 *  @brief  Maximum name length
 */
#define MAX_VALUE_LENGTH 32u
/*!
 *  @brief  Maximum name length
 */
#define MAX_RUNTIMEENTRIES 10u


UInt32 curAddr;

void NameServerApp(void);
NameServer_Handle NameServerApp_setup();
int NameServerApp_execute(NameServer_Handle handle);
int NameServerApp_delete(NameServer_Handle* pNameServerHandle);


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
int
main (int argc, char ** argv)
{
    Char *  trace                           = (Char * )TRUE;
    Bool    sharedRegionApp_enableTrace          = TRUE;
    Char *  traceEnter                      = (Char * )TRUE;
    Bool    sharedRegionApp_enableTraceEnter     = TRUE;
    Char *  traceFailure                    = (Char * )TRUE;
    Bool    sharedRegionApp_enableTraceFailure   = TRUE;
    Char *  traceClass                      = NULL;
    UInt32  sharedRegionApp_traceClass           = 0;

    /* Display the version info and created date/time */
    Osal_printf ("NameServer sample application created on Date:%s Time:%s\n",
            __DATE__,
            __TIME__);

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

    NameServerApp();
    return 0;
}

void NameServerApp(void)
{
    NameServer_Handle handle = NULL;

    printf("Entered NameServerApp \n");
    handle = NameServerApp_setup();
    if(handle == NULL) {
        printf("NameServerApp_setup failed\n");
        goto exit;
    }

    NameServerApp_execute(handle);
    NameServerApp_delete(&handle);
exit:
    return;
}


/*
 *  Setup and create a nameserver instance and add, remove,
 *  and get local entries
 */
NameServer_Handle NameServerApp_setup()
{
    int status;
    NameServer_Handle      nameServerHandle = NULL;
    NameServer_Params      nameServerParams;
    char *name = "NMSVR_TEST";
    MultiProc_Config multiProcConfig;

    printf("Entered NameServerAPP_setup \n");
    nameServerParams.maxNameLen        = MAX_NAME_LENGTH;
    nameServerParams.maxRuntimeEntries = MAX_VALUE_LENGTH;
    nameServerParams.maxValueLen       = MAX_RUNTIMEENTRIES;
    nameServerParams.checkExisting     = TRUE; /* Checks if exists */

    UsrUtilsDrv_setup ();

    multiProcConfig.numProcessors = 4;
    multiProcConfig.id = 3;
    String_cpy (multiProcConfig.nameList [0], "Tesla");
    String_cpy (multiProcConfig.nameList [1], "AppM3");
    String_cpy (multiProcConfig.nameList [2], "SysM3");
    String_cpy (multiProcConfig.nameList [3], "MPU");
    status = MultiProc_setup(&multiProcConfig);
    if (status < 0) {
        Osal_printf ("Error in MultiProc_setup [0x%x]\n", status);
    }

    status = NameServer_setup();
    printf("NameServer_setup status:%x\n", status);

    /* Get the default params for  the Name server */
    NameServer_Params_init(&nameServerParams);
    printf("nameServerParams maxNameLen:%x, maxValueLen: %x,"
        "maxRuntimeEntries:%x\n",
        nameServerParams.maxNameLen,
        nameServerParams.maxValueLen,
        nameServerParams.maxRuntimeEntries);

    nameServerHandle = NameServer_create(name, &nameServerParams);
    printf("NameServer_create nameServerHandle:%x\n", (unsigned int)nameServerHandle);
    {
        NameServer_Handle  Handle = NULL;
        Handle = NameServer_getHandle(name);
        printf("NameServer_getHandle Handle:%x\n", (unsigned int) Handle);
    }

    return nameServerHandle;
}

struct name_value_table {
    char *name;
    int value;
};

struct name_value_table nstable[2] = {
        { "name1", 0xab},
        { "name2", 0x55}
};

int NameServerApp_execute(NameServer_Handle handle)
{
    UInt16 proc_id[4] = { 0, 1, 2, 0xFFFF };
    int buf[2] = { 0 };
    void *entry[2] = { NULL, NULL };
    UInt32 size = sizeof(int);

    proc_id[0] = MultiProc_getId(NULL);
    entry[0] =NameServer_add(handle, nstable[0].name, &nstable[0].value, sizeof(int));
    entry[1] =  NameServer_addUInt32(handle, nstable[1].name, nstable[1].value);


    NameServer_getLocal(handle, nstable[0].name, &buf[0], &size);
    if(buf[0] != nstable[0].value) {
        printf("NameServer_getLocal returned wrong value %x\n", buf[0]);
    } else {
        printf("NameServer_getLocal returned correct value %x\n", buf[0]);
    }

    NameServer_getLocal(handle, nstable[1].name, &buf[1], &size);
    if(buf[1] != nstable[1].value) {
        printf("NameServer_get returned wrong value from local table %x\n",
                buf[1]);
    } else {
        printf("NameServer_get returned correct value %x\n", buf[1]);
    }

    printf("First NameServer_removeEntry\n");
    NameServer_removeEntry(handle, entry[1]);
    printf("Second NameServer_remove\n");
    NameServer_remove(handle, nstable[0].name);
    return 0;
}


/*!
 *  @brief      API to delete the name server instance.
 *
 *  @param      pNameServerHandle Pointer to the NameServer Handle.
 */
int NameServerApp_delete(NameServer_Handle* pNameServerHandle)
{
    Int32 status = -1;

    status = NameServer_delete(pNameServerHandle);
    if (status < 0) {
        printf("NameServer_delete failed status:%x\n", status);
    }

    status = NameServer_destroy();
    printf("NameServer_destroy status:%x\n", status);

    status = MultiProc_destroy ();
    Osal_printf ("Multiproc_destroy status: [0x%x]\n", status);
    return status;
}

