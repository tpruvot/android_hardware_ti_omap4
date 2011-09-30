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
 *  @file   GateMPDrvDefs.h
 *
 *  @brief  Definitions of GateMPDrv types and structures.
 *  ============================================================================
 */


#ifndef GATEMP_DRVDEFS_H_0xF415
#define GATEMP_DRVDEFS_H_0xF415


/* Utilities headers */
#include <ti/ipc/GateMP.h>
#include <_GateMP.h>
#include <ti/ipc/SharedRegion.h>
#include <IpcCmdBase.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  IOCTL command ID definitions for GateMP
 *  ----------------------------------------------------------------------------
 */
#define GATEMP_IOC_MAGIC        IPC_IOC_MAGIC
enum GateMPDrvCmdId {
    GATEMP_GETCONFIG = GATEMP_BASE_CMD,
    GATEMP_SETUP,
    GATEMP_DESTROY,
    GATEMP_PARAMS_INIT,
    GATEMP_CREATE,
    GATEMP_DELETE,
    GATEMP_OPEN,
    GATEMP_CLOSE,
    GATEMP_ENTER,
    GATEMP_LEAVE,
    GATEMP_SHAREDMEMREQ,
    GATEMP_OPENBYADDR,
    GATEMP_GETDEFAULTREMOTE
};

/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for GateMP
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command for GateMP_getConfig
 */
#define CMD_GATEMP_GETCONFIG            _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_GETCONFIG,                      \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_setup
 */
#define CMD_GATEMP_SETUP                _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_SETUP,                          \
                                        GateMPDrv_CmdArgs)
/*!
 *  @brief  Command for GateMP_destroy
 */
#define CMD_GATEMP_DESTROY              _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_DESTROY,                        \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_Params_init
 */
#define CMD_GATEMP_PARAMS_INIT          _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_PARAMS_INIT,                    \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_create
 */
#define CMD_GATEMP_CREATE               _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_CREATE,                         \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_delete
 */
#define CMD_GATEMP_DELETE               _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_DELETE,                         \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_open
 */
#define CMD_GATEMP_OPEN                 _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_OPEN,                           \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_close
 */
#define CMD_GATEMP_CLOSE                _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_CLOSE,                          \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_enter
 */
#define CMD_GATEMP_ENTER                _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_ENTER,                          \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_leave
 */
#define CMD_GATEMP_LEAVE                _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_LEAVE,                          \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_sharedMemReq
 */
#define CMD_GATEMP_SHAREDMEMREQ         _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_SHAREDMEMREQ,                   \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_openByAddr
 */
#define CMD_GATEMP_OPENBYADDR           _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_OPENBYADDR,                     \
                                        GateMPDrv_CmdArgs)

/*!
 *  @brief  Command for GateMP_getDefaultRemote
 */
#define CMD_GATEMP_GETDEFAULTREMOTE     _IOWR(GATEMP_IOC_MAGIC,                \
                                        GATEMP_GETDEFAULTREMOTE,               \
                                        GateMPDrv_CmdArgs)


/*  ----------------------------------------------------------------------------
 *  Command arguments for GateMP
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for GateMP
 */
typedef struct GateMPDrv_CmdArgs_tag {
    union {
        struct {
            GateMP_Params       * params;
        } ParamsInit;

        struct {
            GateMP_Config       * config;
        } getConfig;

        struct {
            GateMP_Config       * config;
        } setup;

        struct {
            GateMP_Handle         handle;
            GateMP_Params       * params;
            UInt32                nameLen;
            SharedRegion_SRPtr    sharedAddrSrPtr;
        } create;

        struct {
            GateMP_Handle         handle;
        } deleteInstance;

        struct {
            GateMP_Handle         handle;
            String                name;
            UInt32                nameLen;
            SharedRegion_SRPtr    sharedAddrSrPtr;
        } open;

        struct {
            GateMP_Handle         handle;
            SharedRegion_SRPtr    sharedAddrSrPtr;
        } openByAddr;

        struct {
            GateMP_Handle         handle;
        } close;

        struct {
            GateMP_Handle         handle;
            IArg                  flags;
        } enter;

        struct {
            GateMP_Handle         handle;
            IArg                  flags;
        } leave;

        struct {
            GateMP_Params       * params;
            UInt32                retVal;
        } sharedMemReq;

        struct {
            Ptr                   handle;
        } getDefaultRemote;
    } args;

    Int32 apiStatus;
} GateMPDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* GATEMP_DRVDEFS_H_0xF415 */
