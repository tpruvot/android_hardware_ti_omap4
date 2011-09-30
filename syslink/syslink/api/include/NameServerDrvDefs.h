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
 *  @file       NameServerDrvDefs.h
 *
 *  @brief      Definitions of NameServerDrv types and structures.
 *  ============================================================================
 */


#ifndef NAMESERVER_DRVDEFS_H_0xf2ba
#define NAMESERVER_DRVDEFS_H_0xf2ba


/* Standard headers */
#include <Std.h>

/* Utilities headers */
#include <ti/ipc/NameServer.h>
#include <_NameServer.h>
#include <IpcCmdBase.h>

#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
#define NAMESERVER_IOC_MAGIC        IPC_IOC_MAGIC
enum CMD_NAMESERVER {
    NAMESERVER_SETUP = NAMESERVER_BASE_CMD,
    NAMESERVER_DESTROY,
    NAMESERVER_PARAMS_INIT,
    NAMESERVER_CREATE,
    NAMESERVER_DELETE,
    NAMESERVER_ADD,
    NAMESERVER_ADDUINT32,
    NAMESERVER_GET,
    NAMESERVER_GETLOCAL,
    NAMESERVER_MATCH,
    NAMESERVER_REMOVE,
    NAMESERVER_REMOVEENTRY,
    NAMESERVER_GETHANDLE,
    NAMESERVER_ISREGISTERED,
    NAMESERVER_GETCONFIG
};

/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for NameServer
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command for NameServer_setup
 */
#define CMD_NAMESERVER_SETUP            _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_SETUP,                      \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_destroy
 */
#define CMD_NAMESERVER_DESTROY          _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_DESTROY,                    \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_Params_init
 */
#define CMD_NAMESERVER_PARAMS_INIT      _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_PARAMS_INIT,                \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_create
 */
#define CMD_NAMESERVER_CREATE           _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_CREATE,                     \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_delete
 */
#define CMD_NAMESERVER_DELETE           _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_DELETE,                     \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_add
 */
#define CMD_NAMESERVER_ADD              _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_ADD,                        \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_addUInt32
 */
#define CMD_NAMESERVER_ADDUINT32        _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_ADDUINT32,                  \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_get
 */
#define CMD_NAMESERVER_GET              _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_GET,                        \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_getLocal
 */
#define CMD_NAMESERVER_GETLOCAL         _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_GETLOCAL,                   \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_match
 */
#define CMD_NAMESERVER_MATCH            _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_MATCH,                      \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_remove
 */
#define CMD_NAMESERVER_REMOVE           _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_REMOVE,                     \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_removeEntry
 */
#define CMD_NAMESERVER_REMOVEENTRY     _IOWR(NAMESERVER_IOC_MAGIC,             \
                                        NAMESERVER_REMOVEENTRY,                \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_getHandle
 */
#define CMD_NAMESERVER_GETHANDLE        _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_GETHANDLE,                  \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_isRegistered
 */
#define CMD_NAMESERVER_ISREGISTERED     _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_ISREGISTERED,               \
                                        NameServerDrv_CmdArgs)

/*!
 *  @brief  Command for NameServer_getConfig
 */
#define CMD_NAMESERVER_GETCONFIG        _IOWR(NAMESERVER_IOC_MAGIC,            \
                                        NAMESERVER_GETCONFIG,                  \
                                        NameServerDrv_CmdArgs)


/*  ----------------------------------------------------------------------------
 *  Command arguments for NameServer
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for NameServer
 */
typedef struct NameServerDrv_CmdArgs_tag {
    union {
        struct {
            NameServer_Config * config;
        } getConfig;

        struct {
            NameServer_Config * config;
        } setup;

        struct {
            NameServer_Params * params;
        } ParamsInit;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            NameServer_Params * params;
        } create;

        struct {
            NameServer_Handle   handle;
        } delete;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            Ptr                 buf;
            UInt                len;
            Ptr                 entry;
            Ptr                 node;
        } add;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            UInt32              value;
            Ptr                 entry;
        } addUInt32;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            Ptr                 value;
            UInt32              len;
            UInt16            * procId;
            UInt32              procLen;
        } get;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            Ptr                 value;
            UInt32              len;
        } getLocal;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
            UInt32              value;
            UInt32              count;
        } match;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
        } remove;

        struct {
            NameServer_Handle   handle;
            Ptr                 entry;
        } removeEntry;

        struct {
            NameServer_Handle   handle;
            String              name;
            UInt32              nameLen;
        } getHandle;

        struct {
            UInt16              procId;
            Bool                check;
        } isRegistered;
    } args;

    Int32 apiStatus;
} NameServerDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* NameServer_DrvDefs_H_0xf2ba */
