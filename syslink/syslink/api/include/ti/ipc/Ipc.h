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
/** ===========================================================================
 *  @file       Ipc.h
 *
 *  @brief      Ipc Manager.
 *
 *  This modules is primarily used to configure IPC-wide settings and
 *  initialize IPC at runtime.  SharedRegion 0 must be valid before
 *  Ipc_start() can be called.  Ipc_start() must be called before other
 *  IPC APIs are made.
 *
 *  ============================================================================
 */

#ifndef ti_ipc_Ipc__include
#define ti_ipc_Ipc__include

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    Ipc_S_BUSY
 *  @brief  The resource is still in use
 */
#define Ipc_S_BUSY              2

/*!
 *  @def    Ipc_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define Ipc_S_ALREADYSETUP      1

/*!
 *  @def    Ipc_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define Ipc_S_SUCCESS        0

/*!
 *  @def    Ipc_E_FAIL
 *  @brief  Generic failure.
 */
#define Ipc_E_FAIL           -1

/*!
 *  @def    Ipc_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define Ipc_E_INVALIDARG     -2

/*!
 *  @def    Ipc_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define Ipc_E_MEMORY         -3

/*!
 *  @def    Ipc_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define Ipc_E_ALREADYEXISTS  -4

/*!
 *  @def    Ipc_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define Ipc_E_NOTFOUND       -5

/*!
 *  @def    Ipc_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define Ipc_E_TIMEOUT        -6

/*!
 *  @def    Ipc_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define Ipc_E_INVALIDSTATE      -7

/*!
 *  @def    Ipc_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define Ipc_E_OSFAILURE -8

/*!
 *  @def    Ipc_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define Ipc_E_RESOURCE  -9

/*!
 *  @def    Ipc_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define Ipc_E_RESTART   -10


/* =============================================================================
 *  Ipc Module-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Attach to remote processor
 *
 *  Sets the default GateMP if 'NULL'. Sets the SharedRegion 0 heap if 'NULL'.
 *  Creates the Notify, NameServerRemoteNotify, and MessageQ transport
 *  instances, in SharedRegion 0 heap, for communicating with the
 *  specified remote processor. Synchronizes self with the remote processor.
 *  Calls the user's attach function if configured. Ipc_start() must be called
 *  before calling Ipc_attach().
 *
 *  @param      remoteProcId  remote processor's MultiProc id
 *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 *
 *  @sa         Ipc_detach
 */
Int Ipc_attach(UInt16 remoteProcId);

/*!
 *  @brief      Detach from the remote processor
 *
 *  Deletes the Notify, NameServerRemoteNotify, and MessageQ transport
 *  instances, in SharedRegion 0 heap, for communicating with the
 *  specified remote processor.
 *
 *  @param      remoteProcId  remote processor's MultiProc id
 *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 *
 *  @sa         Ipc_attach
 */
Int Ipc_detach(UInt16 remoteProcId);

/*!
 *  @brief      Reads the config entry from the config area.
 *
 *  @param      remoteProcId  remote processor's MultiProc id
 *  @param      tag           tag to identify a config entry
 *  @param      cfg           address where the entry will be copied
 *  @param      size          size of config entry
 *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 */
Int readConfig(UInt16 remoteProcId, UInt32 tag, Ptr cfg, SizeT size);

/*!
 *  @brief      Reserves memory, creates default GateMP and HeapMemMP
 *
 *  Function needs to be called before Ipc_attach(). Ipc reserves some
 *  shared memory in SharedRegion 0 for synchronization. GateMP reserves
 *  some shared memory for managing the gates and for the default GateMP.
 *  The same amount of memory must be reserved by each processor, but only
 *  the owner of SharedRegion 0 clears the reserved memory and creates the
 *  default GateMP. The default heap for each SharedRegion is created by
 *  the owner of each SharedRegion.
 *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 */
Int Ipc_start(Void);

/*!
 *  @brief      Writes the config entry to the config area.
 *
 *  @param      remoteProcId  remote processor's MultiProc id
 *  @param      tag           tag to identify a config entry
 *  @param      cfg           address where the entry will be copied
 *  @param      size          size of config entry
 *
 *  @return     Status
 *              - #Ipc_S_SUCCESS: if operation was successful
 *              - #Ipc_E_FAIL: if operation failed
 */
Int writeConfig(UInt16 remoteProcId, UInt32 tag, Ptr cfg, SizeT size);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_Ipc__include */
