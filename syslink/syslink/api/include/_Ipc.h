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
 *  @file   _Ipc.h
 *
 *
 *  @brief   This module is primarily used to configure IPC-wide settings and
 *           initialize IPC at runtime.
 *
 *  ============================================================================
 */


#ifndef __IPC_H__
#define __IPC_H__


#include <ti/ipc/Ipc.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Macros
 * =============================================================================
 */
/*!
 *  @def    Ipc_CONTROLCMD_LOADCALLBACK
 *  @brief  Control command id for platform load callback.
 */
#define Ipc_CONTROLCMD_LOADCALLBACK  (0xBABE0000)

/*!
 *  @def    Ipc_CONTROLCMD_STARTCALLBACK
 *  @brief  Control command id for platform start callback.
 */
#define Ipc_CONTROLCMD_STARTCALLBACK (0xBABE0001)

/*!
 *  @def    Ipc_CONTROLCMD_STOPCALLBACK
 *  @brief  Control command id for platform stop callback.
 */
#define Ipc_CONTROLCMD_STOPCALLBACK  (0xBABE0002)


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*! @brief Function to control a Ipc instance for a slave
 *
 *  @param      procId  Remote processor ID
 *  @param      cmdId   Command ID
 *  @param      arg     Argument
 */
Int Ipc_control (UInt16 procId, Int32 cmdId, Ptr arg);

/*! @brief Function to read configuration information from Ipc module
 *
 *  @param      remoteProcId  Remote processor ID
 *  @param      tag           Tag associated with the information to be read
 *  @param      cfg           Pointer to configuration information
 *  @param      size          Size of configuration information to be read
 */
Int Ipc_readConfig (UInt16 remoteProcId, UInt32 tag, Ptr cfg, SizeT size);

/*! @brief Function to write configuration information to Ipc module
 *
 *  @param      remoteProcId  Remote processor ID
 *  @param      tag           Tag associated with the information to be written
 *  @param      cfg           Pointer to configuration information
 *  @param      size          Size of configuration information to be written
 */
Int Ipc_writeConfig (UInt16 remoteProcId, UInt32 tag, Ptr cfg, SizeT size);

/*!
 *  @brief      Clears memory, deletes  default GateMP and HeapMemMP
 *
 *  Function needs to be called after Ipc_detach().  *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 */
Int Ipc_stop (Void);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* __IPC_H__ */
