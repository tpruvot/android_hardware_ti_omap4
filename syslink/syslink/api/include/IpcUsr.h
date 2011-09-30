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
 *  @file   IpcUsr.h
 *
 *  @brief   This module is primarily used to configure IPC-wide settings and
 *           initialize IPC at runtime.
 *
 *  ============================================================================
 */


#ifndef _IPCUSR_H_
#define _IPCUSR_H_


#include <ti/ipc/Ipc.h>

/* Module headers */
#include <_MultiProc.h>
#include <_SharedRegion.h>
#include <_ListMP.h>
#include <_MessageQ.h>
#include <_Notify.h>
#include <_NameServer.h>
#include <ProcMgr.h>
#include <_HeapBufMP.h>
#include <_HeapMemMP.h>
#include <_GateMP.h>

#if 0 /* TBD: Temporarily commented. */
#include <_HeapMultiBuf.h>
#include <_ClientNotifyMgr.h>
#include <_FrameQBufMgr.h>
#include <_FrameQ.h>
#endif /* TBD: Temporarily commented. */


#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    IPC_MODULEID
 *  @brief  Unique module ID.
 */
#define IPC_MODULEID       (0xF086)


/*!
 *  @def    IPC_CMD_SCALABILITY
 *  @brief  Command ID for scalability info.
 */
#define IPC_CMD_SCALABILITY                (0x00000000)

/*!
 *  @def    IPC_CMD_SHAREDREGION_ENTRY_BASE
 *  @brief  Base of command IDs for entries used by Shared region.
 */
#define IPC_CMD_SHAREDREGION_ENTRY_START (0x00000001)
#define IPC_CMD_SHAREDREGION_ENTRY_END   (0x00001000)


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  The different options for processor synchronization
 */
typedef enum IpcProcSync_Config {
    Ipc_ProcSync_NONE,          /* don't do any processor sync            */
    Ipc_ProcSync_PAIR,          /* sync pair of processors in Ipc_attach  */
    Ipc_ProcSync_ALL            /* sync all processors, done in Ipc_start */
} IpcProcSync_Config;

/*!
 *  @brief  Structure defining config parameters for overall System.
 */
typedef struct IpcKnl_Config {
    IpcProcSync_Config              ipcSyncConfig;
    /*!< Ipc Proc Sync config parameter */
} IpcKnl_Config;

/*!
 *  @brief  Structure defining config parameters for overall System.
 */
typedef struct Ipc_Config {
    MultiProc_Config                multiProcConfig;
    /*!< Multiproc config parameter */

    SharedRegion_Config             sharedRegionConfig;
    /*!< SharedRegion config parameter */

    GateMP_Config                   gateMPConfig;
    /*!< GateMP config parameter */

    MessageQ_Config                 messageQConfig;
    /*!< MessageQ config parameter */

    Notify_Config                   notifyConfig;
    /*!< Notify config parameter */

    ProcMgr_Config                  procMgrConfig;
    /*!< Processor manager config parameter */

    HeapBufMP_Config                heapBufMPConfig;
    /*!< HeapBufMP config parameter */

    HeapMemMP_Config                heapMemMPConfig;
    /*!< HeapMemMP config parameter */

    ListMP_Config                   listMPConfig;
    /*!< ListMP config parameter */

#if 0 /* TBD: Temporarily commented. */
    HeapMultiBuf_Config             heapMultiBufConfig;
    /*!< HeapMultiBuf config parameter */

    ClientNotifyMgr_Config          cliNotifyMgrCfgParams;
    /*!< ClientNotifyMgr config parameter */

    FrameQBufMgr_Config             frameQBufMgrCfgParams;
    /*!< FrameQBufMgr config parameter */

    FrameQ_Config                   frameQCfgParams;
    /*!< FrameQ config parameter */
#endif /* TBD: Temporarily commented. */
} Ipc_Config;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief Returns default configuration values for Ipc.
 *
 *  @param cfgParams Pointer to configuration structure
 */
Void Ipc_getConfig (Ipc_Config * cfgParams);

/*!
 *  @brief Sets up Ipc for this processor.
 *
 *  @param cfgParams Pointer to configuration structure
 */
Int Ipc_setup (const Ipc_Config * cfgParams);

/*!
 *  @brief Destroys Ipc for this processor.
 */
Int Ipc_destroy (void);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* _IPCUSR_H__ */
