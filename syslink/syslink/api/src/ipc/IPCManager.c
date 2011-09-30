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
/** ============================================================================
 *  @file   IPCManager.c
 *
 *  @brief  Source file for the IPC driver wrapper for Syslink modules.
 *  ============================================================================
*/

/*  ----------------------------------- Host OS */
#include <host_os.h>
#include <errno.h>
#include <Std.h>
#include <Trace.h>

#include <ti/ipc/MultiProc.h>
#include <MultiProcDrvDefs.h>
#include <ti/ipc/NameServer.h>
#include <NameServerDrvDefs.h>
#include <ti/ipc/SharedRegion.h>
#include <SharedRegionDrvDefs.h>
#include <ti/ipc/GateMP.h>
#include <GateMPDrvDefs.h>
#include <ti/ipc/HeapBufMP.h>
#include <HeapBufMPDrvDefs.h>
#include <ti/ipc/HeapMemMP.h>
#include <HeapMemMPDrvDefs.h>
#include <ti/ipc/MessageQ.h>
#include <MessageQDrvDefs.h>
#include <ti/ipc/ListMP.h>
#include <ListMPDrvDefs.h>
#include <ti/ipc/Notify.h>
#include <NotifyDrvDefs.h>
#include <ti/ipc/Ipc.h>
#include <IpcUsr.h>
#include <IpcDrvDefs.h>
/*
#include <SysMemMgr.h>
#include <SysMemMgrDrvDefs.h>
*/

#define IPC_SOK   0
#define IPC_EFAIL -1

/*
 * Driver name for complete IPC module
 */
#define IPC_DRIVER_NAME  "/dev/syslink_ipc"

enum IPC_MODULES_ID {
    SHAREDREGION = 0,
    HEAPMEMMP,
    GATEMP,
    HEAPBUFMP,
    NAMESERVER,
    MESSAGEQ,
    LISTMP,
    MULTIPROC,
    IPC,
    SYSMEMMGR,
    NOTIFY,
    MAX_IPC_MODULES
};
struct name_id_table {
    const char *module_name;
    enum IPC_MODULES_ID module_id;
    unsigned long ioctl_count;
};

struct name_id_table name_id_table[MAX_IPC_MODULES] = {
    { "/dev/syslinkipc/SharedRegion", SHAREDREGION, 0 },
    { "/dev/syslinkipc/HeapMemMP", HEAPMEMMP, 0},
    { "/dev/syslinkipc/GateMP", GATEMP, 0},
    { "/dev/syslinkipc/HeapBufMP", HEAPBUFMP, 0},
    { "/dev/syslinkipc/NameServer", NAMESERVER, 0},
    { "/dev/syslinkipc/MessageQ", MESSAGEQ, 0},
    { "/dev/syslinkipc/ListMP", LISTMP, 0},
    { "/dev/syslinkipc/MultiProc", MULTIPROC, 0},
    { "/dev/syslinkipc/Ipc", IPC, 0},
    { "/dev/syslinkipc/SysMemMgr", SYSMEMMGR, 0},
    { "/dev/syslinkipc/Notify", NOTIFY, 0},
};

/*  ----------------------------------- Globals */
Int                     ipc_handle = -1;         /* class driver handle */
static unsigned long    ipc_usage_count;
static sem_t            semOpenClose;
static bool             ipc_sem_initialized = false;


Int getHeapBufMPStatus(Int apiStatus)
{
    Int status = HeapBufMP_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getHeapBufMPStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
        case 0:
            status = HeapBufMP_S_SUCCESS;
            break;

        case -EFAULT:
            status = HeapBufMP_E_FAIL;
            break;

        case -EINVAL:
            status = HeapBufMP_E_INVALIDARG;
            break;

        case -ENOMEM:
            status = HeapBufMP_E_MEMORY;
            break;

        case -ENOENT:
            status = HeapBufMP_E_NOTFOUND;
            break;

        case -ENODEV:
            status = HeapBufMP_E_INVALIDSTATE;
            break;

        case -EEXIST:
        case 1:
            status = HeapBufMP_S_ALREADYSETUP;
            break;

        case 2:
            status = HeapBufMP_S_BUSY;
            break;

        default:
            status = HeapBufMP_E_FAIL;
            break;
        }

        GT_1trace (curTrace, GT_2CLASS,
            "IPCMGR-getHeapBufMPStatus: status=0x%x\n", status);
        return status;
}

Int getHeapMemMPStatus(Int apiStatus)
{
    Int status = HeapMemMP_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getHeapMemMPStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
        case 0:
            status = HeapMemMP_S_SUCCESS;
            break;

        case -EFAULT:
            status = HeapMemMP_E_FAIL;
            break;

        case -EINVAL:
            status = HeapMemMP_E_INVALIDARG;
            break;

        case -ENOMEM:
            status = HeapMemMP_E_MEMORY;
            break;

        case -ENOENT:
            status = HeapMemMP_E_NOTFOUND;
            break;

        case -ENODEV:
            status = HeapMemMP_E_INVALIDSTATE;
            break;

        case -EEXIST:
        case 1:
            status = HeapMemMP_S_ALREADYSETUP;
            break;

        case 2:
            status = HeapMemMP_S_BUSY;
            break;

        default:
            status = HeapMemMP_E_FAIL;
            break;
        }

        GT_1trace (curTrace, GT_2CLASS,
            "IPCMGR-getHeapMemMPStatus: status=0x%x\n", status);
        return status;
}


Int getGateMPStatus(Int apiStatus)
{
    Int status = GateMP_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getGateMPStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = GateMP_S_SUCCESS;
        break;

    case -EINVAL:
        status = GateMP_E_INVALIDARG;
        break;

    case -ENOMEM:
        status = GateMP_E_MEMORY;
        break;

    case -ENOENT:
        status = GateMP_E_NOTFOUND;
        break;

    case -EEXIST:
        status = GateMP_E_ALREADYEXISTS;
        break;

    case 1:
        status = GateMP_S_ALREADYSETUP;
        break;

    case -ENODEV:
        status = GateMP_E_INVALIDSTATE;
        break;

    default:
        status = GateMP_E_FAIL;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getGateMPStatus: status=0x%x\n", status);
    return status;
}


Int getSharedRegionStatus(Int apiStatus)
{
    Int status = SharedRegion_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getSharedRegionStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = SharedRegion_S_SUCCESS;
        break;

    case -EINVAL:
        status = SharedRegion_E_INVALIDARG;
        break;

    case -ENOMEM:
        status = SharedRegion_E_MEMORY;
        break;

    case -ENOENT:
        status = SharedRegion_E_NOTFOUND;
        break;

    case -EEXIST:
        status = SharedRegion_E_ALREADYEXISTS;
        break;

    case -ENODEV:
        status = SharedRegion_E_INVALIDSTATE;
        break;

    case 1:
        status = SharedRegion_S_ALREADYSETUP;
        break;

    case -1:
    default:
        status = SharedRegion_E_FAIL;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getSharedRegionStatus: status=0x%x\n", status);
    return status;
}


Int getNameServerStatus(Int apiStatus)
{
    Int status = NameServer_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getNameServerStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = NameServer_S_SUCCESS;
        break;

    case -EINVAL:
        status = NameServer_E_INVALIDARG;
        break;

    case 1:
        status = NameServer_S_ALREADYSETUP;
        break;

    case -EEXIST:
        status = NameServer_E_ALREADYEXISTS;
        break;

    case -ENOMEM:
        status = NameServer_E_MEMORY;
        break;

    case -ENOENT:
        status = NameServer_E_NOTFOUND;
        break;

    case -ENODEV:
        status = NameServer_E_INVALIDSTATE;
        break;

    case -EBUSY: /* NameServer instances are still present */
    default:
        if(apiStatus < 0) {
            status = NameServer_E_FAIL;
        }
        else {
            status = apiStatus;
        }
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getNameServerStatus: status=0x%x\n", status);
    return status;
}


Int getMessageQStatus(Int apiStatus)
{
    Int status = MessageQ_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getMessageQ: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = MessageQ_S_SUCCESS;
        break;

    case -EINVAL:
        status = MessageQ_E_INVALIDARG;
        break;

    case -ENOMEM:
        status = MessageQ_E_MEMORY;
        break;

    case -ENODEV:
        status = MessageQ_E_INVALIDSTATE;
        break;

    case -ENOENT:
        status = MessageQ_E_NOTFOUND;
        break;

    case -ETIME:
        status = MessageQ_E_TIMEOUT;
        break;

    case 1:
        status = MessageQ_S_ALREADYSETUP;
        break;

    /* All error codes are not converted to kernel error codes */
    default:
        status = apiStatus;
        /* status = MessageQ_E_FAIL;
        status = MessageQ_E_OSFAILURE; */
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getMessageQStatus: status=0x%x\n", status);
    return status;
}

Int getListMPStatus(Int apiStatus)
{
    Int status = ListMP_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getListMPStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = ListMP_S_SUCCESS;
        break;

    case -EINVAL:
        status = ListMP_E_INVALIDARG;
        break;

    case -ENOMEM:
        status = ListMP_E_MEMORY;
        break;

    case -ENODEV:
        status = ListMP_E_INVALIDSTATE;
        break;

    case 1:
        status = ListMP_S_ALREADYSETUP;
        break;

    /* All error codes are not converted to kernel error codes */
    default:
        status = apiStatus;
        /* status = ListMP_E_FAIL;
        status = ListMP_E_OSFAILURE; */
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getListMP: status=0x%x\n", status);
    return status;
}


Int getMultiProcStatus(Int apiStatus)
{
    Int status = MultiProc_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getMultiProcStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = MultiProc_S_SUCCESS;
        break;

    case -ENOMEM:
        status = MultiProc_E_MEMORY;
        break;

    case -EEXIST:
    case 1:
        status = MultiProc_S_ALREADYSETUP;
        break;

    case -ENODEV:
        status = MultiProc_E_INVALIDSTATE;
        break;

    default:
        status = MultiProc_E_FAIL;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getMultiProcStatus: status=0x%x\n", status);
    return status;
}


Int getIpcStatus(Int apiStatus)
{
    Int status = Ipc_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getSysMgrStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = Ipc_S_SUCCESS;
        break;

    case -ENOMEM:
        status = Ipc_E_MEMORY;
        break;

    case 1:
        status = Ipc_S_ALREADYSETUP;
        break;

    case -ENODEV:
        status = Ipc_E_INVALIDSTATE;
        break;

    default:
        status = Ipc_E_FAIL;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getIpcStatus: status=0x%x\n", status);
    return status;
}


Int getNotifyStatus(Int apiStatus)
{
    Int status = Notify_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getNotifyStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    default:
        status = apiStatus;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getIpcStatus: status=0x%x\n", status);
    return status;
}

#if 0 /* TBD:Temporarily comment. */
Int getSysMemMgrStatus(Int apiStatus)
{
    Int status = SYSMEMMGR_E_OSFAILURE;

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getSysMemMgrStatus: apiStatus=0x%x\n", apiStatus);

    switch(apiStatus) {
    case 0:
        status = SYSMEMMGR_SUCCESS;
        break;

    case -ENOMEM:
        status = SYSMEMMGR_E_MEMORY;
        break;

    case -EEXIST:
    case SYSMEMMGR_S_ALREADYSETUP:
        status = SYSMEMMGR_S_ALREADYSETUP;
        break;

    case -ENODEV:
        status = SYSMEMMGR_E_INVALIDSTATE;
        break;

    default:
        status = SYSMEMMGR_E_FAIL;
        break;
    }

    GT_1trace (curTrace, GT_2CLASS,
        "IPCMGR-getSysMemMgrStatus: status=0x%x\n", status);
    return status;
}
#endif /* TBD:Temporarily comment. */

/* This function converts the os specific (Linux) error code to
 *  module specific error code
 */
void IPCManager_getModuleStatus(Int module_id, Ptr args)
{
    NameServerDrv_CmdArgs *ns_cmdargs = NULL;
    SharedRegionDrv_CmdArgs *shrn_cmdargs = NULL;
    GateMPDrv_CmdArgs *gp_cmdargs = NULL;
    HeapBufMPDrv_CmdArgs *hb_cmdargs = NULL;
    HeapMemMPDrv_CmdArgs *hm_cmdargs = NULL;
    MessageQDrv_CmdArgs *mq_cmdargs = NULL;
    ListMPDrv_CmdArgs *lstmp_cmdargs = NULL;
    MultiProcDrv_CmdArgs *multiproc_cmdargs = NULL;
    IpcDrv_CmdArgs *ipc_cmdargs = NULL;
#if 0 /* TBD:Temporarily comment. */
    SysMemMgrDrv_CmdArgs *sysmemmgr_cmdargs = NULL;
#endif /* TBD:Temporarily comment. */
    Notify_CmdArgs *notify_cmdargs = NULL;

    switch(module_id) {
    case MULTIPROC:
        multiproc_cmdargs = (MultiProcDrv_CmdArgs *)args;
        multiproc_cmdargs->apiStatus = getMultiProcStatus(multiproc_cmdargs->apiStatus);
        break;

    case NAMESERVER:
        ns_cmdargs = (NameServerDrv_CmdArgs *)args;
        ns_cmdargs->apiStatus = getNameServerStatus(ns_cmdargs->apiStatus);
        break;

    case SHAREDREGION:
        shrn_cmdargs = (SharedRegionDrv_CmdArgs *)args;
        shrn_cmdargs->apiStatus = getSharedRegionStatus(shrn_cmdargs->apiStatus);
        break;

    case GATEMP: /* Above id and this has same value */
        gp_cmdargs = (GateMPDrv_CmdArgs *)args;
        gp_cmdargs->apiStatus = getGateMPStatus(gp_cmdargs->apiStatus);
        break;

    case HEAPBUFMP:
        hb_cmdargs = (HeapBufMPDrv_CmdArgs *)args;
        hb_cmdargs->apiStatus = getHeapBufMPStatus(hb_cmdargs->apiStatus);
        break;

    case HEAPMEMMP:
        hm_cmdargs = (HeapMemMPDrv_CmdArgs *)args;
        hm_cmdargs->apiStatus = getHeapMemMPStatus(hm_cmdargs->apiStatus);
        break;

    case MESSAGEQ:
        mq_cmdargs = (MessageQDrv_CmdArgs *)args;
        mq_cmdargs->apiStatus = getMessageQStatus(mq_cmdargs->apiStatus);
        break;

    case LISTMP:
        lstmp_cmdargs = (ListMPDrv_CmdArgs *)args;
        lstmp_cmdargs->apiStatus = \
                        getListMPStatus(lstmp_cmdargs->apiStatus);
        break;

    case IPC:
        ipc_cmdargs = (IpcDrv_CmdArgs *)args;
        ipc_cmdargs->apiStatus = getIpcStatus(ipc_cmdargs->apiStatus);
        break;

#if 0 /* TBD:Temporarily comment. */
    case SYSMEMMGR:
        sysmemmgr_cmdargs = (SysMemMgrDrv_CmdArgs *)args;
        sysmemmgr_cmdargs->apiStatus = \
                getSysMemMgrStatus(sysmemmgr_cmdargs->apiStatus);
        break;
#endif /* TBD:Temporarily comment. */

    case NOTIFY:
        notify_cmdargs = (Notify_CmdArgs *)args;
        notify_cmdargs->apiStatus = getNotifyStatus(notify_cmdargs->apiStatus);
        break;

    default:
        GT_0trace (curTrace, GT_2CLASS, "IPCMGR-Unknown module\n");
        break;
    }
}


/* This will increment individual module usage count */
Int createIpcModuleHandle(const char *name)
{
    Int module_found = 0;
    Int module_handle = 0;
    Int i;

    for (i = 0; i < MAX_IPC_MODULES; i++) {
        if (strcmp(name, name_id_table[i].module_name) == 0) {
            module_found = 1;
            break;
        }
    }

    if (module_found) {
        switch (name_id_table[i].module_id) {
        case MULTIPROC:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case SHAREDREGION:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case GATEMP:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case HEAPBUFMP:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case HEAPMEMMP:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case NAMESERVER:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case MESSAGEQ:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case LISTMP:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case IPC:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case SYSMEMMGR:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        case NOTIFY:
            module_handle = (name_id_table[i].module_id << 16) | ipc_handle;
            break;

        default:
            break;
        }
    }

    if (module_found) {
        return module_handle;
    }
    else
        return -1;
}


/* This will track ioctl trafic from different ipc modules */
void trackIoctlCommandFlow(Int fd)
{
    switch (fd >> 16) {
    case MULTIPROC:
        name_id_table[MULTIPROC].ioctl_count += 1;
        break;

    case SHAREDREGION:
        name_id_table[SHAREDREGION].ioctl_count += 1;
        break;

    case GATEMP:
        name_id_table[GATEMP].ioctl_count += 1;
        break;

    case HEAPBUFMP:
        name_id_table[HEAPBUFMP].ioctl_count += 1;
        break;

    case HEAPMEMMP:
        name_id_table[HEAPMEMMP].ioctl_count += 1;
        break;

    case NAMESERVER:
        name_id_table[NAMESERVER].ioctl_count += 1;
        break;

    case MESSAGEQ:
        name_id_table[MESSAGEQ].ioctl_count += 1;
        break;

    case LISTMP:
        name_id_table[LISTMP].ioctl_count += 1;
        break;

    case IPC:
        name_id_table[IPC].ioctl_count += 1;
        break;

#if 0 /* TBD:Temporarily comment. */
    case SYSMEMMGR:
        name_id_table[SYSMEMMGR].ioctl_count += 1;
        break;
#endif /* TBD: Temporarily comment. */

    case NOTIFY:
        name_id_table[NOTIFY].ioctl_count += 1;
        break;

    default:
        break;
    }
}


Void displayIoctlInfo(Int fd, UInt32 cmd, Ptr args)
{
    switch (fd >> 16) {
    case MULTIPROC:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from MULTIPROC module\n");
        break;

    case SHAREDREGION:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from SHAREDREGION module\n");
        break;

    case GATEMP:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from GATEMP module\n");
        break;

    case HEAPBUFMP:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from HEAPBUFMP module\n");
        break;

    case HEAPMEMMP:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from HEAPMEMMP module\n");
        break;

    case NAMESERVER:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from NAMESERVER module\n");
        break;

    case MESSAGEQ:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from MESSAGEQ module\n");
        break;

    case LISTMP:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from LISTMP module\n");
        break;

    case IPC:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from IPC module\n");
        break;

    case SYSMEMMGR:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from SYSMEMMGR module\n");
        break;

    case NOTIFY:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from NOTIFY module\n");
        break;

    default:
        GT_0trace (curTrace, GT_2CLASS,
            "IPCMGR: IOCTL command from UNKNOWN module\n");
        break;
    }

    GT_4trace (curTrace, GT_2CLASS,
        "IOCTL cmd : %x, IOCTL ioc_nr: %x(%d) args: %p\n",
            cmd, _IOC_NR(cmd), _IOC_NR(cmd), args);
}


/*
 *  ======== IPCManager_open ========
 *  Purpose:
 *      Open handle to the IPC driver
 */
Int IPCManager_open(const char *name, Int flags)
{
    Int module_handle = 0;
    Int retval = -1;

    GT_2trace (curTrace, GT_1CLASS,
        "IPCManager_open: Module Name:%s Flags:0x%x\n", name, flags);

    if (!ipc_sem_initialized) {
        if (sem_init(&semOpenClose, 0, 1) == -1) {
            GT_0trace (curTrace, GT_4CLASS,
                "MGR: Failed to Initialize"
                "the ipc semaphore\n");
            goto exit;
        } else
            ipc_sem_initialized = true;
    }

    sem_wait(&semOpenClose);
    if (ipc_usage_count == 0) {
        retval = open(IPC_DRIVER_NAME, flags);
        if (retval < 0) {
            GT_0trace(curTrace, GT_4CLASS,
                "IPCManager_open: Failed to open the ipc driver\n");
            goto error;
        }
        ipc_handle = retval;
    }

    /* Success in opening handle to ipc driver */
    module_handle = createIpcModuleHandle(name);
    if (module_handle < 0) {
        GT_0trace(curTrace, GT_4CLASS,
            "IPCManager_open: Failed to creat module handle\n");
        retval = -1;
        goto error;
    }

    ipc_usage_count++;
    sem_post(&semOpenClose);
    return module_handle;

error:
    GT_1trace (curTrace, GT_LEAVE, "IPCManager_open", retval);
    sem_post(&semOpenClose);

exit:
    return retval;
}


/*
 *  ======== IPCManager_Close ========
 *  Purpose:   Close handle to the IPC driver
 */
Int IPCManager_close(Int fd)
{
    Int retval = 0;

    sem_wait(&semOpenClose);
    if (ipc_handle < 0)
        goto exit;

    ipc_usage_count--;
    if (ipc_usage_count == 0) {
        retval = close(ipc_handle);
        if (retval < 0)
            goto exit;

        ipc_handle = -1;
    }

exit:
    sem_post(&semOpenClose);
    return retval;
}


/*
 * ======== IPCTRAP_Trap ========
 */
Int IPCManager_ioctl(Int fd, UInt32 cmd, Ptr args)
{
    Int status = -1;

    displayIoctlInfo(fd, cmd, args);
    if (ipc_handle >= 0)
        status = ioctl(ipc_handle, cmd, args);

    IPCManager_getModuleStatus(fd >> 16, args);
    /* This is to track the command flow */
    trackIoctlCommandFlow(fd);

    GT_1trace (curTrace, GT_LEAVE, "IPCManager_ioctl", status);
    return status;
}


/*
 * ======== IPCTRAP_Read ========
 */
Int IPCManager_read(Int fd, Ptr packet, UInt32 size)
{
    Int status = -1;

    if (ipc_handle >= 0)
        status = read(ipc_handle, packet, size);

    GT_1trace (curTrace, GT_LEAVE, "IPCManager_read", status);
    return status;
}


/*
 * ======== IPCTRAP_fcntl========
 */
Int IPCManager_fcntl(Int fd, Int cmd, long arg)
{
    Int retval = -1;

    retval = fcntl(ipc_handle, cmd, arg);

    return retval;
}
