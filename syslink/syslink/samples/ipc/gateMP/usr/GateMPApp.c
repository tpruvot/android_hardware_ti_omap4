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
 *  @file   GateMPApp.c
 *
 *  @brief  Sample application for GateMP module
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
#include <ti/ipc/Ipc.h>
#include <IpcUsr.h>

#include <ti/ipc/GateMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/SharedRegion.h>

#include <ProcMgr.h>
#include <_ProcMgrDefs.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */

#define PROCMGR_DRIVER_NAME         "/dev/syslink-procmgr"

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
/*
 *  The shared memory is going to split between
 *  Notify:       0xA0000000 - 0xA0004000
 *  HeapBuf:      0xA0004000 - 0xA0008000
 *  Transport:    0xA0008000 - 0xA000A000
 *  GateMP: 0xA000A000 - 0xA000B000
 *  MessageQ NS:  0xA000B000 - 0xA000C000
 *  HeapBuf NS:   0xA000C000 - 0xA000D000
 *  NameServer:   0xA000D000 - 0xA000E000
 *  Gatepeterson1:0xA000E000 - 0xA000F000
 */
#define SHAREDMEM               0xA0000000
#define SHAREDMEMSIZE           0xF000

ProcMgr_Handle gateMPApp_procMgrHandle = NULL;
UInt32 gateMPApp_Handle = 0;
UInt32 gateMPApp_shAddr_usr_Base = 0;


UInt32 curAddr;

void GateMP_app();
Handle GateMP_app_startup();
void GateMP_app_execute(Handle gateHandle);
void GateMP_app_shutdown(Handle *gateHandle);
void * ProcMgrApp_startup ();

/** ============================================================================
 *  Functions
 *  ============================================================================
 */
int
main (int argc, char ** argv)
{
    Char *trace    = (Char *)TRUE;
    Bool sharedRegionApp_enableTrace = TRUE;
    Char * traceEnter = (Char *)TRUE;
    Bool sharedRegionApp_enableTraceEnter = TRUE;
    Char * traceFailure    = (Char *)TRUE;
    Bool sharedRegionApp_enableTraceFailure = TRUE;
    Char *traceClass = (Char *)TRUE;
    UInt32 sharedRegionApp_traceClass = 0;


    /* Display the version info and created date/time */
    Osal_printf ("GateMP sample application created on Date:%s Time:%s\n",
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

    GateMP_app();
    return 0;
}

void GateMP_app()
{
    Handle gatepeterson_handle = NULL;
    gatepeterson_handle = GateMP_app_startup();
    GateMP_app_execute(gatepeterson_handle);
    GateMP_app_shutdown(&gatepeterson_handle);

}

Handle GateMP_app_startup()
{
    Int32 status = 0 ;
    GateMP_Params gateParams;
    GateMP_Handle gateHandle = NULL;
    Ipc_Config config;
    ProcMgr_AttachParams      attachParams;
    ProcMgr_State             state;
    SharedRegion_Entry        entry;

    Osal_printf ("Entering GateMP Application Startup\n");

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    status = ProcMgr_open (&gateMPApp_procMgrHandle,
                           MultiProc_getId("SysM3"));
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }

    if (status >= 0) {
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (gateMPApp_procMgrHandle, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (gateMPApp_procMgrHandle);
            Osal_printf ("After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", status);
        }
    }

    status = ProcMgr_translateAddr (gateMPApp_procMgrHandle,
                                (Ptr) &gateMPApp_shAddr_usr_Base,
                                ProcMgr_AddrType_MasterUsrVirt,
                                (Ptr) SHAREDMEM,
                                ProcMgr_AddrType_SlaveVirt);
    entry.base = (Ptr) gateMPApp_shAddr_usr_Base;
    entry.len = SHAREDMEMSIZE;
    SharedRegion_setEntry(0, &entry);

    GateMP_Params_init(&gateParams);
    gateParams.sharedAddr      = (Ptr)gateMPApp_shAddr_usr_Base;
    gateParams.name          = Memory_alloc(NULL,
                                      sizeof(10),
                                          0u);
    String_ncpy(gateParams.name,"TESTGATE",String_len ("TESTGATE"));

    gateHandle = GateMP_create(&gateParams);
    if (gateHandle == NULL) {
         Osal_printf ("Error in GateMP_create \n");
    }

    Osal_printf ("Leaving GateMP Application Startup\n");
    return gateHandle;
}

void GateMP_app_execute(Handle gateHandle)
{
    Int *key = 0;
    Osal_printf ("Entering GateMP Application Execute\n");
    key = GateMP_enter(gateHandle);
    GateMP_leave(gateHandle, key);
    Osal_printf ("Leaving GateMP Application Execute\n");
}

void GateMP_app_shutdown(Handle *gateHandle)
{
    Int32 status = 0 ;

    Osal_printf ("Entering GateMP Application Shutdown\n");

    status = GateMP_delete((GateMP_Handle *)gateHandle);
    if (gateHandle < 0) {
        Osal_printf ("Error in GateMP_delete [0x%x]\n", status);
    }

    status = ProcMgr_close (&gateMPApp_procMgrHandle);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);

    Ipc_destroy ();

    Osal_printf ("Leaving GateMP Application Execute\n");
}
