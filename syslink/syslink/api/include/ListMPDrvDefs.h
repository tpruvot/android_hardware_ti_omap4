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
 *  @file   ListMPDrvDefs.h
 *
 *  @brief  Definitions of ListMPDrv types and structures.
 *  ============================================================================
 */


#ifndef LISTMP_DRVDEFS_H_0x42d8
#define LISTMP_DRVDEFS_H_0x42d8


/* Standard headers */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/ListMP.h>
#include <_ListMP.h>
#include <IpcCmdBase.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for ListMP
 *  ----------------------------------------------------------------------------
 */
/* Base command ID for ListMP */
#define LISTMP_IOC_MAGIC        IPC_IOC_MAGIC
enum CMD_LISTMP {
    LISTMP_GETCONFIG = LISTMP_BASE_CMD,
    LISTMP_SETUP,
    LISTMP_DESTROY,
    LISTMP_PARAMS_INIT,
    LISTMP_CREATE,
    LISTMP_DELETE,
    LISTMP_OPEN,
    LISTMP_CLOSE,
    LISTMP_ISEMPTY,
    LISTMP_GETHEAD,
    LISTMP_GETTAIL,
    LISTMP_PUTHEAD,
    LISTMP_PUTTAIL,
    LISTMP_INSERT,
    LISTMP_REMOVE,
    LISTMP_NEXT,
    LISTMP_PREV,
    LISTMP_SHAREDMEMREQ,
    LISTMP_OPENBYADDR
};
/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for ListMP
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command for ListMP_getConfig
 */
#define CMD_LISTMP_GETCONFIG            _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_GETCONFIG,                      \
                                        ListMPDrv_CmdArgs)

/*!
 *  @brief  Command for ListMP_setup
 */
#define CMD_LISTMP_SETUP                _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_SETUP,                          \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_setup
 */
#define CMD_LISTMP_DESTROY              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_DESTROY,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_destroy
 */
#define CMD_LISTMP_PARAMS_INIT          _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_PARAMS_INIT,                    \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_create
 */
#define CMD_LISTMP_CREATE               _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_CREATE ,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_delete
 */
#define CMD_LISTMP_DELETE               _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_DELETE,                         \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_open
 */
#define CMD_LISTMP_OPEN                 _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_OPEN,                           \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_close
 */
#define CMD_LISTMP_CLOSE                _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_CLOSE ,                           \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_isEmpty
 */
#define CMD_LISTMP_ISEMPTY              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_ISEMPTY,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_getHead
 */
#define CMD_LISTMP_GETHEAD              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_GETHEAD,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_getTail
 */
#define CMD_LISTMP_GETTAIL              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_GETTAIL,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_putHead
 */
#define CMD_LISTMP_PUTHEAD              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_PUTHEAD,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_putTail
 */
#define CMD_LISTMP_PUTTAIL              _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_PUTTAIL,                        \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_insert
 */
#define CMD_LISTMP_INSERT               _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_INSERT,                         \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_remove
 */
#define CMD_LISTMP_REMOVE               _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_REMOVE,                         \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_next
 */
#define CMD_LISTMP_NEXT                 _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_NEXT,                           \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_prev
 */
#define CMD_LISTMP_PREV                 _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_PREV,                           \
                                        ListMPDrv_CmdArgs)
/*!
 *  @brief  Command for ListMP_sharedMemReq
 */
#define CMD_LISTMP_SHAREDMEMREQ         _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_SHAREDMEMREQ,                   \
                                        ListMPDrv_CmdArgs)

/*!
 *  @brief  Command for ListMP_openByAddr
 */
#define CMD_LISTMP_OPENBYADDR           _IOWR(LISTMP_IOC_MAGIC,                \
                                        LISTMP_OPENBYADDR,                     \
                                        ListMPDrv_CmdArgs)

/*  ----------------------------------------------------------------------------
 *  Command arguments for ListMP
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for ListMP
 */
typedef struct ListMPDrv_CmdArgs {
    union {
        struct {
            ListMP_Params             * params;
        } ParamsInit;

        struct {
            ListMP_Config             * config;
        } getConfig;

        struct {
            ListMP_Config             * config;
        } setup;

        struct {
            Ptr                         handle;
            ListMP_Params             * params;
            UInt32                      nameLen;
            SharedRegion_SRPtr          sharedAddrSrPtr;
            Ptr                         knlGate;
        } create;

        struct {
            Ptr                         handle;
        } deleteInstance;

        struct {
            Ptr                         handle;
            UInt32                      nameLen;
            String                      name;
        } open;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          sharedAddrSrPtr;
        } openByAddr;

        struct {
            Ptr                         handle;
        } close;

        struct {
            Ptr                         handle;
            Bool                        isEmpty;
        } isEmpty;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr;
        } getHead;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr;
        } getTail;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr ;
        } putHead;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr ;
        } putTail;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          newElemSrPtr;
            SharedRegion_SRPtr          curElemSrPtr;
        } insert;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr ;
        } remove;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr ;
            SharedRegion_SRPtr          nextElemSrPtr ;
        } next;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          elemSrPtr ;
            SharedRegion_SRPtr          prevElemSrPtr ;
        } prev;

        struct {
            ListMP_Params             * params;
            UInt32                      bytes;
            Ptr                         knlGate;
            SharedRegion_SRPtr          sharedAddrSrPtr;
            UInt32                      nameLen;
            Ptr                         handle;
        } sharedMemReq;
    } args;

    Int32 apiStatus;
} ListMPDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* LISTMP_DRVDEFS_H_0x42d8 */
