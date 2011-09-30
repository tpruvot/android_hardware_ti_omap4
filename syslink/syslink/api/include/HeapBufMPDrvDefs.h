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
 *  @file   HeapBufMPDrvDefs.h
 *
 *  @brief  Definitions of HeapBufMPDrv types and structures.
 *  ============================================================================
 */


#ifndef HEAPBUFMP_DRVDEFS_H_0xb9a7
#define HEAPBUFMP_DRVDEFS_H_0xb9a7


/* Utilities headers */
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/SharedRegion.h>
#include <IpcCmdBase.h>
#include <_HeapBufMP.h>
#include <MemoryDefs.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for HeapBufMP
 *  ----------------------------------------------------------------------------
 */
#define   HEAPBUFMP_IOC_MAGIC    IPC_IOC_MAGIC
enum CMD_HEAPBUF {
    HEAPBUFMP_GETCONFIG = HEAPBUFMP_BASE_CMD,
    HEAPBUFMP_SETUP,
    HEAPBUFMP_DESTROY,
    HEAPBUFMP_PARAMS_INIT,
    HEAPBUFMP_CREATE,
    HEAPBUFMP_DELETE,
    HEAPBUFMP_OPEN,
    HEAPBUFMP_OPENBYADDR,
    HEAPBUFMP_CLOSE,
    HEAPBUFMP_ALLOC,
    HEAPBUFMP_FREE,
    HEAPBUFMP_SHAREDMEMREQ,
    HEAPBUFMP_GETSTATS,
    HEAPBUFMP_GETEXTENDEDSTATS
};

/*!
 *  @brief  Command for HeapBufMP_getConfig
 */
#define CMD_HEAPBUFMP_GETCONFIG         _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_GETCONFIG,                   \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_setup
 */
#define CMD_HEAPBUFMP_SETUP             _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_SETUP,                       \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_setup
 */
#define CMD_HEAPBUFMP_DESTROY           _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_DESTROY,                     \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_destroy
 */
#define CMD_HEAPBUFMP_PARAMS_INIT       _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_PARAMS_INIT,                 \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_create
 */
#define CMD_HEAPBUFMP_CREATE            _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_CREATE,                      \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_delete
 */
#define CMD_HEAPBUFMP_DELETE            _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_DELETE,                      \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_open
 */
#define CMD_HEAPBUFMP_OPEN              _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_OPEN,\
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_close
 */
#define CMD_HEAPBUFMP_CLOSE             _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_CLOSE,                       \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_alloc
 */
#define CMD_HEAPBUFMP_ALLOC             _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_ALLOC,                       \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_free
 */
#define CMD_HEAPBUFMP_FREE              _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_FREE,                        \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_sharedMemReq
 */
#define CMD_HEAPBUFMP_SHAREDMEMREQ      _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_SHAREDMEMREQ,                \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_getStats
 */
#define CMD_HEAPBUFMP_GETSTATS          _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_GETSTATS,                    \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_getExtendedStats
 */
#define CMD_HEAPBUFMP_GETEXTENDEDSTATS  _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_GETEXTENDEDSTATS,            \
                                        HeapBufMPDrv_CmdArgs)

/*!
 *  @brief  Command for HeapBufMP_openByAddr
 */
#define CMD_HEAPBUFMP_OPENBYADDR        _IOWR(HEAPBUFMP_IOC_MAGIC,             \
                                        HEAPBUFMP_OPENBYADDR,                  \
                                        HeapBufMPDrv_CmdArgs)


/*  ----------------------------------------------------------------------------
 *  Command arguments for HeapBufMP
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for HeapBufMP
 */
typedef struct HeapBufMPDrv_CmdArgs {
    union {
        struct {
            HeapBufMP_Params          * params;
        } ParamsInit;

        struct {
            HeapBufMP_Config          * config;
        } getConfig;

        struct {
            HeapBufMP_Config          * config;
        } setup;

        struct {
            Ptr                         handle;
            HeapBufMP_Params          * params;
            UInt32                      nameLen;
            SharedRegion_SRPtr          sharedAddrSrPtr;
            Ptr                         knlGate;
        } create;

        struct {
            Ptr                         handle;
        } deleteInstance;

        struct {
            Ptr                         handle;
            String                      name;
            UInt32                      nameLen;
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
            UInt32                      size;
            UInt32                      align;
            SharedRegion_SRPtr          blockSrPtr;
        } alloc;

        struct {
            Ptr                         handle;
            SharedRegion_SRPtr          blockSrPtr;
            UInt32                      size;
        } free;

        struct {
            Ptr                         handle;
            Memory_Stats              * stats;
        } getStats;

        struct {
            Ptr                         handle;
            HeapBufMP_ExtendedStats   * stats;
        } getExtendedStats;

        struct {
            Ptr                         handle;
            HeapBufMP_Params          * params;
            SharedRegion_SRPtr          sharedAddrSrPtr;
            UInt32                      bytes;
        } sharedMemReq;
    } args;

    Int32 apiStatus;
} HeapBufMPDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* HEAPBUFMP_DRVDEFS_H_0xb9a7 */
