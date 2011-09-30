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
 *  @file   MultiProcDrvDefs.h
 *
 *  @brief  Definitions of MultiProcDrv types and structures.
 *  ============================================================================
 */


#ifndef MULTIPROC_DRVDEFS_H_0xf2ba
#define MULTIPROC_DRVDEFS_H_0xf2ba

/* Standard headers */
#include <Std.h>
#include <IpcCmdBase.h>

/* Utilities headers */
#include <_MultiProc.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  IOCTL command IDs for MultiProc
 *  ----------------------------------------------------------------------------
 */
#define     MULTIPROC_IOC_MAGIC      IPC_IOC_MAGIC
enum CMD_MULTIPROC {
    MULTIPROC_SETUP = MULTIPROC_BASE_CMD,
    MULTIPROC_DESTROY,
    MULTIPROC_GETCONFIG,
    MULTIPROC_SETLOCALID
};

/*!
 *  @brief  Command for MultiProc_setup
 */
#define CMD_MULTIPROC_SETUP                 _IOWR(MULTIPROC_IOC_MAGIC,         \
                                            MULTIPROC_SETUP,                   \
                                            MultiProcDrv_CmdArgs)
/*!
 *  @brief  Command for MultiProc_setup
 */
#define CMD_MULTIPROC_DESTROY               _IOWR(MULTIPROC_IOC_MAGIC,         \
                                            MULTIPROC_DESTROY,                 \
                                            MultiProcDrv_CmdArgs)
/*!
 *  @brief  Command for MultiProc_destroy
 */
#define CMD_MULTIPROC_GETCONFIG             _IOWR(MULTIPROC_IOC_MAGIC,         \
                                            MULTIPROC_GETCONFIG,               \
                                            MultiProcDrv_CmdArgs)
/*!
 *  @brief  Command for MultiProc_delete
 */
#define CMD_MULTIPROC_SETLOCALID            _IOWR(MULTIPROC_IOC_MAGIC,         \
                                            MULTIPROC_SETLOCALID,              \
                                            MultiProcDrv_CmdArgs)


/*  ----------------------------------------------------------------------------
 *  Command arguments for MultiProc
 *  ----------------------------------------------------------------------------
 */
/*!
 *  @brief  Command arguments for MultiProc
 */
typedef struct MultiProcDrv_CmdArgs_tag {
    union {
        struct {
            MultiProc_Config  * config;
        } getConfig;

        struct {
            MultiProc_Config  * config;
        } setup;

        struct {
            UInt16              id;
        } setLocalId;
    } args;

    Int32 apiStatus;
} MultiProcDrv_CmdArgs;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* MultiProc_DrvDefs_H_0xf2ba */
