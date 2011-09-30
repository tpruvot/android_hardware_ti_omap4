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
 *  @file   IpcDrvDefs.h
 *
 *  @brief      Definitions of IpcDrv types and structures.
 *
 *  ============================================================================
 */


#ifndef IPC_DRVDEFS_H_0xF414
#define IPC_DRVDEFS_H_0xF414


/* Utilities headers */
#include <ti/ipc/Ipc.h>
#include <IpcUsr.h>
#include <IpcCmdBase.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for Ipc
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  IOC Magic Number for Ipc
 */
#define SYSIPC_IOC_MAGIC        IPC_IOC_MAGIC

/*!
 *  @brief  IOCTL command numbers for Ipc
 */
enum IpcDrvCmd {
    IPC_SETUP = IPC_BASE_CMD,
    IPC_DESTROY,
    IPC_CONTROL,
    IPC_READCONFIG,
    IPC_WRITECONFIG
};

/*!
 *  @brief  Command for Ipc_setup
 */
#define CMD_IPC_SETUP \
                _IOWR(SYSIPC_IOC_MAGIC, IPC_SETUP, \
                IpcDrv_CmdArgs)
/*!
 *  @brief  Command for Ipc_destroy
 */
#define CMD_IPC_DESTROY \
                _IOWR(SYSIPC_IOC_MAGIC, IPC_DESTROY, \
                IpcDrv_CmdArgs)
/*!
 *  @brief  Command for Ipc_control
 */
#define CMD_IPC_CONTROL \
                _IOWR(SYSIPC_IOC_MAGIC, IPC_CONTROL, \
                IpcDrv_CmdArgs)
/*!
 *  @brief  Command for Ipc_readConfig
 */
#define CMD_IPC_READCONFIG \
                _IOWR(SYSIPC_IOC_MAGIC, IPC_READCONFIG, \
                IpcDrv_CmdArgs)
/*!
 *  @brief  Command for Ipc_writeConfig
 */
#define CMD_IPC_WRITECONFIG \
                _IOWR(SYSIPC_IOC_MAGIC, IPC_WRITECONFIG, \
                IpcDrv_CmdArgs)

/*  ----------------------------------------------------------------------------
 *  Command arguments for Ipc
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for Ipc
 */
typedef struct IpcDrv_CmdArgs {
    union {
        struct {
            UInt16                procId;
            Int32                 cmdId;
            Ptr                   arg;
        } control;

        struct {
            UInt16                remoteProcId;
            UInt32                tag;
            Ptr                   cfg;
            SizeT                 size;
        } readConfig;

        struct {
            UInt16                remoteProcId;
            UInt32                tag;
            Ptr                   cfg;
            SizeT                 size;
        } writeConfig;

        struct {
            IpcKnl_Config       * config;
        } setup;
    } args;

    Int32 apiStatus;
} IpcDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* IPC_DRVDEFS_H_0xF414 */
