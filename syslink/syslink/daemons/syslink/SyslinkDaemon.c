/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2008-2010, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*==============================================================================
 *  @file   SyslinkDaemon.c
 *
 *  @brief  Daemon for Syslink functions
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>
#include <Memory.h>
#include <String.h>

/* IPC headers */
#include <IpcUsr.h>
#include <ProcMgr.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MessageQ.h>

/* Sample headers */
#include <CrashInfo.h>

#include <syslink_ipc_listener.h>

#ifdef HAVE_ANDROID_OS
#undef LOG_TAG
#define LOG_TAG "SYSLINKD"
#endif

/* Defines for the default HeapBufMP being configured in the System */
#define RCM_MSGQ_TILER_HEAPNAME         "Heap0"
#define RCM_MSGQ_TILER_HEAP_BLOCKS      128
#define RCM_MSGQ_TILER_HEAP_ALIGN       128
#define RCM_MSGQ_TILER_MSGSIZE          256
#define RCM_MSGQ_TILER_HEAPID           0
#define RCM_MSGQ_DOMX_HEAPNAME          "Heap1"
#define RCM_MSGQ_DOMX_HEAP_BLOCKS       256
#define RCM_MSGQ_DOMX_HEAP_ALIGN        128
#define RCM_MSGQ_DOMX_MSGSIZE           384
#define RCM_MSGQ_DOMX_HEAPID            1
#define RCM_MSGQ_HEAP_SR                1

#define DUCATI_DMM_POOL_0_ID            0
#define DUCATI_DMM_POOL_0_START         0x90000000
#define DUCATI_DMM_POOL_0_SIZE          0x10000000

#define FAULT_RECOVERY_DELAY            500000

#define CONTEXTBUFFERADD                0x9E0FC000
#define CONTEXTBUFFERWDTSYSM3           CONTEXTBUFFERADD
#define CONTEXTBUFFERWDTAPPM3           (CONTEXTBUFFERADD + 0X0080)
#define STACKBUFFERADD                  0x9E0FD000
#define STACKBUFFERSZE                  0x3000

#define SYSM3_PROC_NAME                 "SysM3"
#define APPM3_PROC_NAME                 "AppM3"
#define READ_BUF_SIZE                   50

ProcMgr_Handle                  procMgrHandleSysM3;
ProcMgr_Handle                  procMgrHandleAppM3;
Bool                            appM3Client         = FALSE;
UInt16                          remoteIdSysM3;
UInt16                          remoteIdAppM3;
sem_t                           semDaemonWait;
HeapBufMP_Handle                heapHandle          = NULL;
SizeT                           heapSize            = 0;
Ptr                             heapBufPtr          = NULL;
HeapBufMP_Handle                heapHandle1         = NULL;
SizeT                           heapSize1           = 0;
Ptr                             heapBufPtr1         = NULL;
IHeap_Handle                    srHeap              = NULL;
pthread_t                       sysM3EvtHandlerThrd = 0;
pthread_t                       appM3EvtHandlerThrd = 0;
static Bool                     restart             = TRUE;
static Bool                     isSysM3Event        = FALSE;
static Bool                     isAppM3Event        = FALSE;
#if defined (SYSLINK_USE_LOADER)
UInt32                          fileIdSysM3;
UInt32                          fileIdAppM3;
#endif

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

static Bool isDaemonRunning (Char * pidName)
{
    DIR           * dir;
    pid_t           pid;
    Int             dirNum;
    FILE          * fp;
    struct dirent * next;
    Bool            isRunning                   = FALSE;
    Char            filename [READ_BUF_SIZE];
    Char            buffer [READ_BUF_SIZE];
    Char          * bptr                        = buffer;
    Char          * name;

    pid = getpid ();
    dir = opendir ("/proc");
    if (!dir) {
        Osal_printf ("Warning: Cannot open /proc filesystem\n");
        return isRunning;
    }

    name = strrchr (pidName, '/');
    if (name) {
        pidName = (name + 1);
    }

    while ((next = readdir (dir)) != NULL) {
        /* If it isn't a number, we don't want it */
        if (!isdigit (*next->d_name)) {
            continue;
        }

        dirNum = strtol (next->d_name, NULL, 10);
        if (dirNum == pid) {
            continue;
        }

        snprintf (filename, READ_BUF_SIZE, "/proc/%s/cmdline", next->d_name);
        if (!(fp = fopen (filename, "r"))) {
            continue;
        }
        if (fgets (buffer, READ_BUF_SIZE, fp) == NULL) {
            fclose (fp);
            continue;
        }
        fclose (fp);

        name = strrchr (buffer, '/');
        if (name && (name + 1)) {
            bptr = (name + 1);
        }
        else {
            bptr = buffer;
        }

        /* Buffer should contain the enitre command line */
        if (String_cmp (bptr, pidName) == 0) {
            isRunning = TRUE;
            break;
        }
    }
    closedir (dir);

    return isRunning;
}

/*
 *========exceptionDumpWdtRegisters=========
 */
static Void exceptionDumpWdtRegisters (Int ProcId)
{
    Int                     status;
    volatile ExcContext   * excContext;
    Memory_MapInfo          traceinfo;
    Char                  * ttype;
    Char                  * core;

    if (ProcId == PROC_SYSM3) {
        traceinfo.src  = CONTEXTBUFFERWDTSYSM3;
        core = "SYSM3";
    }
    else {
        traceinfo.src  = CONTEXTBUFFERWDTAPPM3;
        core = "APPM3";
    }

    Osal_printf ("========================================================\n"
                 "=============== %s WDT CONTEXT DUMP =================\n"
                 "========================================================\n",
                 core );

    traceinfo.size = 0x80;
    status = Memory_map (&traceinfo);
    if (status!= MEMORYOS_SUCCESS) {
        Osal_printf ("\nCrash-dump Memory_map failed\n\n");
    }
    else {
        /* Fill the Structure with data from memory */
        excContext = (volatile ExcContext *) traceinfo.dst;

        switch (excContext->threadType) {
        case BIOS_ThreadType_Task:
            ttype = "Task";
            break;
        case BIOS_ThreadType_Swi:
            ttype = "Swi";
            break;
        default:
            ttype = "Hwi/Main";
        }

        Osal_printf ("%s Exception occurred in ThreadType_%s.\n", core, ttype);
        Osal_printf ("%s %s handle: 0x%x.\n", core, ttype,
                     excContext->threadHandle);
        Osal_printf ("%s %s stack base: 0x%x.\n", core, ttype,
                     excContext->threadStack);
        Osal_printf ("%s %s stack size: 0x%x.\n", core, ttype,
                     excContext->threadStackSize);
        Osal_printf ("%s R0 = %08x  R1 = %08x\n", core, excContext->r0,
                     excContext->r1);
        Osal_printf ("%s R2 = %08x  R3 = %08x\n", core, excContext->r2,
                     excContext->r3);
        Osal_printf ("%s R12= %08x  SP(R13) = %08x\n", core, excContext->r12,
                     excContext->sp);
        Osal_printf ("%s PC = %08x  LR(R14) = %08x\n", core, excContext->pc,
                     excContext->lr);
        Osal_printf ("%s PSR = %08x\n\n", core, excContext->psr);
    }
}

/*
 *========exceptionDumpRegisters=========
 */
static Void exceptionDumpRegisters (Void)
{
    Int                     status;
    UInt32                  i;
    volatile ExcContext   * excContext;
    Memory_MapInfo          traceinfo;
    Char                  * ttype;

    traceinfo.src  = CONTEXTBUFFERADD;
    traceinfo.size = 0x80;
    status = Memory_map (&traceinfo);
    if (status!= MEMORYOS_SUCCESS) {
        Osal_printf ("Memory_map failed\n");
    }
    else {
        Osal_printf ("\nContext traceinfo.dst = 0x%x traceinfo.size = 0x%x\n",
                        traceinfo.dst, traceinfo.size);
    }

    /* Fill the Structure with data from memory */
    excContext = (volatile ExcContext *) traceinfo.dst;

    Osal_printf ("========================================================\n");
    Osal_printf ("===================== CONTEXT DUMP =====================\n");
    Osal_printf ("========================================================\n");

    switch (excContext->threadType) {
    case BIOS_ThreadType_Task:
        ttype = "Task";
        break;
    case BIOS_ThreadType_Swi:
        ttype = "Swi";
        break;
    case BIOS_ThreadType_Hwi:
        ttype = "Hwi";
        break;
    case BIOS_ThreadType_Main:
        ttype = "Main";
        break;
    default:
        ttype = "Unknown_thread";
    }

    Osal_printf ("Exception occurred in ThreadType_%s.\n", ttype);
    Osal_printf ("%s handle: 0x%x.\n", ttype, excContext->threadHandle);
    Osal_printf ("%s stack base: 0x%x.\n", ttype, excContext->threadStack);
    Osal_printf ("%s stack size: 0x%x.\n", ttype, excContext->threadStackSize);
    Osal_printf ("R0 = %08x  R8      = %08x\n", excContext->r0, excContext->r8);
    Osal_printf ("R1 = %08x  R9      = %08x\n", excContext->r1, excContext->r9);
    Osal_printf ("R2 = %08x  R10     = %08x\n", excContext->r2, excContext->r10);
    Osal_printf ("R3 = %08x  R11     = %08x\n", excContext->r3, excContext->r11);
    Osal_printf ("R4 = %08x  R12     = %08x\n", excContext->r4, excContext->r12);
    Osal_printf ("R5 = %08x  SP(R13) = %08x\n", excContext->r5, excContext->sp);
    Osal_printf ("R6 = %08x  LR(R14) = %08x\n", excContext->r6, excContext->lr);
    Osal_printf ("R7 = %08x  PC(R15) = %08x\n", excContext->r7, excContext->pc);
    Osal_printf ("PSR = %08x\n", excContext->psr);
    Osal_printf ("ICSR = %08x\n", excContext->ICSR);
    Osal_printf ("MMFSR = %02x\n", excContext->MMFSR);
    Osal_printf ("BFSR = %02x\n", excContext->BFSR);
    Osal_printf ("UFSR = %04x\n", excContext->UFSR);
    Osal_printf ("HFSR = %08x\n", excContext->HFSR);
    Osal_printf ("DFSR = %08x\n", excContext->DFSR);
    Osal_printf ("MMAR = %08x\n", excContext->MMAR);
    Osal_printf ("BFAR = %08x\n", excContext->BFAR);
    Osal_printf ("AFSR = %08x\n", excContext->AFSR);
    Osal_printf ("\n");

    traceinfo.src = STACKBUFFERADD;
    traceinfo.size = excContext->threadStackSize + 4;
    if (traceinfo.size > STACKBUFFERSZE - 4) {
        Osal_printf ("ERROR: Stack size larger than allocated space. Limiting "
                        "to 12KB\n");
        traceinfo.size = STACKBUFFERSZE;
    }

    status = Memory_map (&traceinfo);
    if (status!= MEMORYOS_SUCCESS) {
        Osal_printf ("Memory_map failed\n");
    }
    else {
        Osal_printf ("Stack traceinfo.dst = 0x%x traceinfo.size = 0x%x\n",
                        traceinfo.dst, traceinfo.size);
    }

    Osal_printf ("========================================================\n");
    Osal_printf ("====================== STACK DUMP ======================\n");
    Osal_printf ("========================================================\n");

    for (i = traceinfo.dst; i < traceinfo.dst + traceinfo.size ; i=i + 4) {
        Osal_printf ("[%04d]:%08x\n", (i - traceinfo.dst)/4 ,
                        *((volatile UInt32 *) i));
    }
}


/*
 *  ======== sysM3EventHandler ========
 */
static Void sysM3EventHandler (Void)
{
    Int                 status  = PROCMGR_E_FAIL;
    UInt                index;
    Int                 size;
    ProcMgr_EventType   eventList [] = {PROC_MMU_FAULT, PROC_ERROR,
                                                    PROC_WATCHDOG};

    size = (sizeof (eventList)) / (sizeof (ProcMgr_EventType));
    status = ProcMgr_waitForMultipleEvents (PROC_SYSM3, eventList, size, -1,
                                            &index);
    if (status == PROCMGR_SUCCESS) {
        if (eventList [index] == PROC_MMU_FAULT) {
            Osal_printf ("\nMMU Fault occured on the M3 subsystem. See crash "
                         "dump for more details...\n");
        }
        else if (eventList [index] == PROC_ERROR) {
            Osal_printf ("\nSysError occured on SysM3. See crash dump for more "
                         "details...\n");
        }
        else {
            Osal_printf ("\nWatchDog fired on the M3 subsystem.\n");
        }

        if ((eventList [index] == PROC_MMU_FAULT) ||
            (eventList [index] == PROC_ERROR)) {
            /* Dump Crash Info */
            exceptionDumpRegisters ();
        }
        else if (eventList [index] == PROC_WATCHDOG) {
            /* Dump Crash Info */
            exceptionDumpWdtRegisters (PROC_SYSM3);
        }

        /* Initiate cleanup */
        isSysM3Event = TRUE;
        restart = TRUE;
        sem_post (&semDaemonWait);
    }
    else {
        if (errno == EINTR) {
            Osal_printf ("SysM3 Event Handler Thread %s\n", strerror(errno));
        }
        else {
            Osal_printf ("SysM3 Event Handler Thread block failed 0x%x\n",
                status);
            isSysM3Event = TRUE;
            /* Post the semaphore to terminate the parent process */
            sem_post (&semDaemonWait);
        }
    }
}

/*
 *  ======== appM3EventHandler ========
 */
static Void appM3EventHandler (Void)
{
    Int                 status  = PROCMGR_E_FAIL;
    UInt                index;
    Int                 size;
    ProcMgr_EventType   eventList [] = {PROC_ERROR, PROC_WATCHDOG};

    size = (sizeof (eventList)) / (sizeof (ProcMgr_EventType));
    status = ProcMgr_waitForMultipleEvents (PROC_APPM3, eventList, size, -1,
                                            &index);
    if (status == PROCMGR_SUCCESS) {
        if (eventList [index] == PROC_WATCHDOG) {
            Osal_printf ("\nWatchDog fired on the M3 subsystem.\n");
            /* Dump Crash Info */
            exceptionDumpWdtRegisters (PROC_APPM3);
        }
        else {
            Osal_printf ("\nSysError occured on AppM3. See crash dump for more "
                         "details...\n");

            /* Dump Crash Info */
            exceptionDumpRegisters ();
        }

        /* Initiate cleanup */
        isAppM3Event = TRUE;
        restart = TRUE;
        sem_post (&semDaemonWait);
    }
    else {
        if (errno == EINTR) {
            Osal_printf ("AppM3 Event Handler Thread %s\n", strerror(errno));
        }
        else {
            Osal_printf ("AppM3 Event Handler Thread block failed 0x%x\n",
                status);
            isAppM3Event = TRUE;
            /* Post the semaphore to terminate the parent process */
            sem_post (&semDaemonWait);
        }
    }

}

/*
 *  ======== signal_handler ========
 */
static Void signal_handler (Int sig)
{
    pthread_t self = pthread_self ();
    if (pthread_equal (self, sysM3EvtHandlerThrd)) {
        Osal_printf ("\n** SysLink Daemon: SysM3 thread exiting...\n ");
    }
    else if (pthread_equal (self, appM3EvtHandlerThrd)) {
        Osal_printf ("\n** SysLink Daemon: AppM3 thread exiting...\n ");
    }
    else {
        Osal_printf ("\n** SysLink Daemon: Exiting due to KILL command...\n");
        sem_post (&semDaemonWait);
    }
}


/*
 *  ======== ipc_cleanup ========
 */
static Void ipcCleanup (Void)
{
    ProcMgr_StopParams stopParams;
    Int                status = 0;

    /* Cleanup the default HeapBufMP registered with MessageQ */
    status = MessageQ_unregisterHeap (RCM_MSGQ_DOMX_HEAPID);
    if (status < 0) {
        Osal_printf ("Error in MessageQ_unregisterHeap [0x%x]\n", status);
    }

    if (heapHandle1) {
        status = HeapBufMP_delete (&heapHandle1);
        if (status < 0) {
            Osal_printf ("Error in HeapBufMP_delete [0x%x]\n", status);
        }
    }

    if (heapBufPtr1) {
        Memory_free (srHeap, heapBufPtr1, heapSize1);
    }

    status = MessageQ_unregisterHeap (RCM_MSGQ_TILER_HEAPID);
    if (status < 0) {
        Osal_printf ("Error in MessageQ_unregisterHeap [0x%x]\n", status);
    }

    if (heapHandle) {
        status = HeapBufMP_delete (&heapHandle);
        if (status < 0) {
            Osal_printf ("Error in HeapBufMP_delete [0x%x]\n", status);
        }
    }

    if (heapBufPtr) {
        Memory_free (srHeap, heapBufPtr, heapSize);
    }

    status = ProcMgr_deleteDMMPool (DUCATI_DMM_POOL_0_ID, remoteIdSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_deleteDMMPool:status = 0x%x\n", status);
    }

    if(appM3Client) {
        stopParams.proc_id = remoteIdAppM3;
        status = ProcMgr_stop (procMgrHandleAppM3, &stopParams);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                            stopParams.proc_id, status);
        }

#if defined(SYSLINK_USE_LOADER)
        status = ProcMgr_unload (procMgrHandleAppM3, fileIdAppM3);
        if(status < 0) {
            Osal_printf ("Error in ProcMgr_unload, status [0x%x]\n", status);
        }
#endif
    }

    stopParams.proc_id = remoteIdSysM3;
    status = ProcMgr_stop (procMgrHandleSysM3, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
                        stopParams.proc_id, status);
    }

#if defined(SYSLINK_USE_LOADER)
    status = ProcMgr_unload (procMgrHandleSysM3, fileIdSysM3);
    if(status < 0) {
        Osal_printf ("Error in ProcMgr_unload, status [0x%x]\n", status);
    }
#endif

    if(appM3Client) {
        status = ProcMgr_detach (procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_detach(AppM3): status = 0x%x\n", status);
        }

        status = ProcMgr_close (&procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_close(AppM3): status = 0x%x\n", status);
        }
    }

    status = ProcMgr_detach (procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach(SysM3): status = 0x%x\n", status);
    }

    status = ProcMgr_close (&procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close(SysM3): status = 0x%x\n", status);
    }

    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Error in Ipc_destroy: status = 0x%x\n", status);
    }

    Osal_printf ("Done cleaning up ipc!\n\n");
}


/*
 *  ======== ipcSetup ========
 */
static Int ipcSetup (Char * sysM3ImageName, Char * appM3ImageName)
{
    Ipc_Config                      config;
    ProcMgr_StopParams              stopParams;
    ProcMgr_StartParams             startParams;
    UInt32                          entryPoint = 0;
    UInt16                          procId;
    Int                             status = 0;
    ProcMgr_AttachParams            attachParams;
    ProcMgr_State                   state;
    HeapBufMP_Params                heapbufmpParams;
    Int                             i;
    UInt32                          srCount;
    SharedRegion_Entry              srEntry;

    if(appM3ImageName != NULL)
        appM3Client = TRUE;
    else
        appM3Client = FALSE;

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
        goto exit;
    }

    /* Get MultiProc IDs by name. */
    remoteIdSysM3 = MultiProc_getId (SYSM3_PROC_NAME);
    Osal_printf ("MultiProc_getId remoteId: [0x%x]\n", remoteIdSysM3);
    remoteIdAppM3 = MultiProc_getId (APPM3_PROC_NAME);
    Osal_printf ("MultiProc_getId remoteId: [0x%x]\n", remoteIdAppM3);
    procId = remoteIdSysM3;
    Osal_printf ("MultiProc_getId procId: [0x%x]\n", procId);

    /* Temporary fix to account for a timing issue during recovery. */
    usleep(FAULT_RECOVERY_DELAY);

    printf("RCM procId= %d\n", procId);
    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleSysM3, procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
        goto exit_ipc_destroy;
    }
    else {
        Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandleSysM3, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandleSysM3);
            Osal_printf ("After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", status);
        }
    }

    if (status >= 0 && appM3Client) {
        procId = remoteIdAppM3;
        Osal_printf ("MultiProc_getId procId: [0x%x]\n", procId);

        /* Open a handle to the ProcMgr instance. */
        status = ProcMgr_open (&procMgrHandleAppM3, procId);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
            goto exit_ipc_destroy;
        }
        else {
            Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandleAppM3, &attachParams);
            if (status < 0) {
                Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
            }
            else {
                Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
                state = ProcMgr_getState (procMgrHandleAppM3);
                Osal_printf ("After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", status);
            }
        }
    }

#if defined(SYSLINK_USE_LOADER)
    Osal_printf ("SysM3 Load: loading the SysM3 image %s\n",
                sysM3ImageName);

    status = ProcMgr_load (procMgrHandleSysM3, sysM3ImageName, 2,
                            &sysM3ImageName, &entryPoint, &fileIdSysM3,
                            remoteIdSysM3);
    if(status < 0) {
        Osal_printf ("Error in ProcMgr_load, status [0x%x]\n", status);
        goto exit_procmgr_close_sysm3;
    }
#endif
    startParams.proc_id = remoteIdSysM3;
    Osal_printf ("Starting ProcMgr for procID = %d\n", startParams.proc_id);
    status  = ProcMgr_start(procMgrHandleSysM3, entryPoint, &startParams);
    if(status < 0) {
        Osal_printf ("Error in ProcMgr_start, status [0x%x]\n", status);
        goto exit_procmgr_close_sysm3;
    }

    if(appM3Client) {
#if defined(SYSLINK_USE_LOADER)
        Osal_printf ("AppM3 Load: loading the AppM3 image %s\n",
                    appM3ImageName);
        status = ProcMgr_load (procMgrHandleAppM3, appM3ImageName, 2,
                              &appM3ImageName, &entryPoint, &fileIdAppM3,
                              remoteIdAppM3);
        if(status < 0) {
            Osal_printf ("Error in ProcMgr_load, status [0x%x]\n", status);
            goto exit_procmgr_stop_sysm3;
        }
#endif
        startParams.proc_id = remoteIdAppM3;
        Osal_printf ("Starting ProcMgr for procID = %d\n", startParams.proc_id);
        status  = ProcMgr_start(procMgrHandleAppM3, entryPoint,
                                &startParams);
        if(status < 0) {
            Osal_printf ("Error in ProcMgr_start, status [0x%x]\n", status);
            goto exit_procmgr_stop_sysm3;
        }
    }

    Osal_printf ("SysM3: Creating Ducati DMM pool of size 0x%x\n",
                DUCATI_DMM_POOL_0_SIZE);
    status = ProcMgr_createDMMPool (DUCATI_DMM_POOL_0_ID,
                                    DUCATI_DMM_POOL_0_START,
                                    DUCATI_DMM_POOL_0_SIZE,
                                    remoteIdSysM3);
    if(status < 0) {
        Osal_printf ("Error in ProcMgr_createDMMPool, status [0x%x]\n", status);
        goto exit_procmgr_stop_sysm3;
    }

    srCount = SharedRegion_getNumRegions();
    Osal_printf ("SharedRegion_getNumRegions = %d\n", srCount);
    for (i = 0; i < srCount; i++) {
        status = SharedRegion_getEntry (i, &srEntry);
        Osal_printf ("SharedRegion_entry #%d: base = 0x%x len = 0x%x "
                        "ownerProcId = %d isValid = %d cacheEnable = %d "
                        "cacheLineSize = 0x%x createHeap = %d name = %s\n",
                        i, srEntry.base, srEntry.len, srEntry.ownerProcId,
                        (Int)srEntry.isValid, (Int)srEntry.cacheEnable,
                        srEntry.cacheLineSize, (Int)srEntry.createHeap,
                        srEntry.name);
    }

    /* Create the heap to be used by RCM and register it with MessageQ */
    /* TODO: Do this dynamically by reading from the IPC config from the
     *       baseimage using Ipc_readConfig() */
    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = RCM_MSGQ_TILER_HEAP_ALIGN;
        heapbufmpParams.numBlocks  = RCM_MSGQ_TILER_HEAP_BLOCKS;
        heapbufmpParams.blockSize  = RCM_MSGQ_TILER_MSGSIZE;
        heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize = 0x%x\n", heapSize);

        srHeap = SharedRegion_getHeap (RCM_MSGQ_HEAP_SR);
        if (srHeap == NULL) {
            status = MEMORYOS_E_FAIL;
            Osal_printf ("SharedRegion_getHeap failed for srHeap:"
                         " [0x%x]\n", srHeap);
            goto exit_procmgr_stop_sysm3;
        }
        else {
            Osal_printf ("Before Memory_alloc = 0x%x\n", srHeap);
            heapBufPtr = Memory_alloc (srHeap, heapSize, 0);
            if (heapBufPtr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("Memory_alloc failed for ptr: [0x%x]\n",
                             heapBufPtr);
                goto exit_procmgr_stop_sysm3;
            }
            else {
                heapbufmpParams.name           = RCM_MSGQ_TILER_HEAPNAME;
                heapbufmpParams.sharedAddr     = heapBufPtr;
                Osal_printf ("Before HeapBufMP_Create: [0x%x]\n", heapBufPtr);
                heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("HeapBufMP_create failed for Handle:"
                                 "[0x%x]\n", heapHandle);
                    goto exit_procmgr_stop_sysm3;
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (heapHandle,
                                                    RCM_MSGQ_TILER_HEAPID);
                    if (status < 0) {
                        Osal_printf ("MessageQ_registerHeap failed!\n");
                        goto exit_procmgr_stop_sysm3;
                    }
                }
            }
        }
    }

    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = RCM_MSGQ_DOMX_HEAP_ALIGN;
        heapbufmpParams.numBlocks  = RCM_MSGQ_DOMX_HEAP_BLOCKS;
        heapbufmpParams.blockSize  = RCM_MSGQ_DOMX_MSGSIZE;
        heapSize1 = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize1 = 0x%x\n", heapSize1);

        heapBufPtr1 = Memory_alloc (srHeap, heapSize1, 0);
        if (heapBufPtr1 == NULL) {
            status = MEMORYOS_E_MEMORY;
            Osal_printf ("Memory_alloc failed for ptr: [0x%x]\n",
                         heapBufPtr1);
            goto exit_procmgr_stop_sysm3;
        }
        else {
            heapbufmpParams.name           = RCM_MSGQ_DOMX_HEAPNAME;
            heapbufmpParams.sharedAddr     = heapBufPtr1;
            Osal_printf ("Before HeapBufMP_Create: [0x%x]\n", heapBufPtr1);
            heapHandle1 = HeapBufMP_create (&heapbufmpParams);
            if (heapHandle1 == NULL) {
                status = HeapBufMP_E_FAIL;
                Osal_printf ("HeapBufMP_create failed for Handle:"
                             "[0x%x]\n", heapHandle1);
                goto exit_procmgr_stop_sysm3;
            }
            else {
                /* Register this heap with MessageQ */
                status = MessageQ_registerHeap (heapHandle1,
                                                RCM_MSGQ_DOMX_HEAPID);
                if (status < 0) {
                    Osal_printf ("MessageQ_registerHeap failed!\n");
                    goto exit_procmgr_stop_sysm3;
                }
            }
        }
    }

    Osal_printf ("=== SysLink-IPC setup completed successfully!===\n");
    return 0;

exit_procmgr_stop_sysm3:
    stopParams.proc_id = remoteIdSysM3;
    status = ProcMgr_stop (procMgrHandleSysM3, &stopParams);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_stop(%d): status = 0x%x\n",
            stopParams.proc_id, status);
    }

exit_procmgr_close_sysm3:
    status = ProcMgr_close (&procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close: status = 0x%x\n", status);
    }
exit_ipc_destroy:
    status = Ipc_destroy ();
    if (status < 0) {
        Osal_printf ("Error in Ipc_destroy: status = 0x%x\n", status);
    }

exit:
    return (-1);
}

static Void printUsage (Void)
{
    Osal_printf ("\nInvalid arguments!\n"
                 "Usage: ./syslink_daemon.out [-f] <[-s] <SysM3 image file>> "
                 "[<[-a] <AppM3 image file>>]\n"
                 "Rules: - Full paths must be provided for image files.\n"
                 "       - Use '-f' option to run as a regular process.\n"
                 "       - Images can be specified in any order as long as\n"
                 "         the corresponding option is specified.\n"
                 "       - All images not preceded by an option are applied\n"
                 "         to the cores whose images are not already\n"
                 "         specified in the order of SysM3, AppM3\n\n");
    exit (EXIT_FAILURE);
}

Int main (Int argc, Char * argv [])
{
    pid_t   child_pid;
    pid_t   child_sid;
    Int     status      = 0;
    Bool    daemon      = TRUE;
    Int     o;
    Int     i;
    Char  * images []   = {NULL, NULL};
    Int     numImages   = sizeof (images) / sizeof (images [0]);

    if (isDaemonRunning (argv [0])) {
        Osal_printf ("Multiple instances of syslink_daemon.out are not "
                     "supported!\n");
        return (-1);
    }

    /* Determine args */
    while ((o = getopt (argc, argv, ":fs:a:")) != -1) {
        switch (o) {
        case 's':
            images [0] = optarg;
            break;
        case 'a':
            images [1] = optarg;
            break;
        case 'f':
            daemon = FALSE;
            break;
        case ':':
            status = -1;
            Osal_printf ("Option -%c requires an operand\n", optopt);
            break;
        case '?':
            status = -1;
            Osal_printf ("Unrecognized option: -%c\n", optopt);
            break;
        }
    }

    for (i = 0; optind < argc; optind++) {
        while (i < numImages && images [i]) i++;
        if (i == numImages) {
            printUsage ();
        }
        images [i++] = argv [optind];
    }

    for (i = 0; i < numImages; i++) {
        if (images [i] && access (images [i], R_OK)) {
            Osal_printf ("Error opening %s\n", images [i]);
            printUsage ();
        }
    }

    if (status || optind < argc || !images [0]) {
        printUsage ();
    }

    // Start listner that will listen for incoming messages
    startSyslinkListenerThread();

    /* Process will be daemonised if daemon flag is true */
    if (daemon) {
        Osal_printf ("Spawning SysLink Daemon...\n");
        /* Fork off the parent process */
        child_pid = fork ();
        if (child_pid < 0) {
            Osal_printf ("Spawn daemon failed!\n");
            exit (EXIT_FAILURE);     /* Failure */
        }
        /* If we got a good PID, then we can exit the parent process. */
        if (child_pid > 0) {
            exit (EXIT_SUCCESS);    /* Success */
        }

        /* Change file mode mask */
        umask (0);

        /* Create a new SID for the child process */
        child_sid = setsid ();
        if (child_sid < 0) {
            exit (EXIT_FAILURE);     /* Failure */
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            exit (EXIT_FAILURE);     /* Failure */
        }
    }

    /* Setup the signal handlers */
    if (signal (SIGINT, signal_handler) == SIG_ERR) {
        Osal_printf ("SIGINT registration failed!");
    }
    if (signal (SIGTERM, signal_handler) == SIG_ERR) {
        Osal_printf ("SIGTERM registration failed!");
    }

    while (restart) {
        restart = FALSE;
        isSysM3Event = FALSE;
        isAppM3Event = FALSE;

        sem_init (&semDaemonWait, 0, 0);

        status = ipcSetup (images [0], images [1]);
        if (status < 0) {
            Osal_printf ("ipcSetup failed!\n");
            sem_destroy (&semDaemonWait);
            return (-1);
        }
        Osal_printf ("ipcSetup succeeded!\n");

        /* Create the SysM3 fault handler thread */
        Osal_printf ("Create SysM3 event handler thread\n");
        status = pthread_create (&sysM3EvtHandlerThrd, NULL,
                                    (Void *)&sysM3EventHandler, NULL);

        if (status) {
            Osal_printf ("Error Creating SysM3 event handler thread:%d\n",
                            status);
            ipcCleanup ();
            sem_destroy (&semDaemonWait);
            exit (EXIT_FAILURE);
        }
        /* Only if AppM3 image is specified */
        if (images [1]) {
            /* Create an AppM3 fault handler thread */
            Osal_printf ("Create AppM3 event handler thread\n");
            status = pthread_create (&appM3EvtHandlerThrd, NULL,
                                        (Void *)&appM3EventHandler, NULL);
            if (status) {
                Osal_printf ("Error Creating AppM3 event handler thread:%d\n",
                                status);
                pthread_kill (sysM3EvtHandlerThrd, SIGTERM);
                sysM3EvtHandlerThrd = 0;
                ipcCleanup ();
                sem_destroy (&semDaemonWait);
                exit (EXIT_FAILURE);
            }
        }

        /* Wait for any event handler thread to be unblocked */
        sem_wait (&semDaemonWait);

        /* Clean up event handler threads */
        if (sysM3EvtHandlerThrd) {
            if (isSysM3Event) {
                status = pthread_join (sysM3EvtHandlerThrd, NULL);
                Osal_printf ("pthread_join SysM3 Thread = %d\n", status);
            }
            else {
                if (pthread_kill (sysM3EvtHandlerThrd, SIGTERM) == 0) {
                    status = pthread_join (sysM3EvtHandlerThrd, NULL);
                    Osal_printf ("pthread_kill & join SysM3 Thread = %d\n",
                                    status);
                }
            }
            sysM3EvtHandlerThrd = 0;
        }
        if (appM3EvtHandlerThrd) {
            if (isAppM3Event) {
                status = pthread_join (appM3EvtHandlerThrd, NULL);
                Osal_printf ("pthread_join AppM3 Thread = %d\n", status);
            }
            else {
                if (pthread_kill (appM3EvtHandlerThrd, SIGTERM) == 0) {
                    status = pthread_join (appM3EvtHandlerThrd, NULL);
                    Osal_printf ("pthread_kill & join AppM3 Thread = %d\n",
                                    status);
                }
            }
            appM3EvtHandlerThrd = 0;
        }

        /* IPC_Cleanup function */
        ipcCleanup ();

        sem_destroy (&semDaemonWait);
    }

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
