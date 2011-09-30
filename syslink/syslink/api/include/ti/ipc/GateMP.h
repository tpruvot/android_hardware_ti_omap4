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
 *  @file       GateMP.h
 *
 *  @brief      Multiple processor gate that provides local and remote context
 *              protection.
 *
 *  A GateMP instance can be used to enforce both local and remote context
 *  context protection.  That is, entering a GateMP can prevent preemption by
 *  another thread running on the same processor and simultaneously prevent a
 *  remote processor from entering the same gate.  GateMP's are typically used
 *  to protect reads/writes to a shared resource, such as shared memory.
 *
 *  Creating a GateMP requires supplying the following configuration
 *      - Instance name (see #GateMP_Params::name)
 *      - Region id (see #GateMP_Params::regionId)
 *  In addition, the following parameters should be configured as necessary:
 *      - The level of local protection (interrupt, thread, tasklet, process
 *        or none) can be configured using the #GateMP_Params::localProtect
 *        config parameter.
 *      - The type of remote system gate that can be used.  Most devices will
 *        typically have a single type of system gate so this configuration
 *        should typically be left alone.  See #GateMP_Params::remoteProtect for
 *        more information.

 *  Once created GateMP allows the gate to be opened on another processor
 *  using #GateMP_open and the name that was used in #GateMP_create.
 *
 *  A GateMP can be entered and left using #GateMP_enter} and #GateMP_leave
 *  like any other gate that implements the IGateProvider interface.
 *
 *  GateMP has the following proxies - RemoteSystemProxy, RemoteCustom1Proxy
 *  and RemoteCustom2Proxy which are automatically plugged with device-specific
 *  delegates that implement multiple processor mutexes using a variety of
 *  hardware mechanisms.
 *
 *  GateMP creates a default system gate whose handle may be obtained
 *  using #GateMP_getDefaultRemote.  Most IPC modules typically use this gate
 *  by default if they require gates and no instance gate is configured by the
 *  user.a
 *
 *  ============================================================================
 */

#ifndef ti_ipc_GateMP__include
#define ti_ipc_GateMP__include

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    GateMP_S_BUSY
 *  @brief  The resource is still in use
 */
#define GateMP_S_BUSY               2

/*!
 *  @def    GateMP_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define GateMP_S_ALREADYSETUP       1

/*!
 *  @def    GateMP_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define GateMP_S_SUCCESS            0

/*!
 *  @def    GateMP_E_FAIL
 *  @brief  Generic failure.
 */
#define GateMP_E_FAIL              -1

/*!
 *  @def    GateMP_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define GateMP_E_INVALIDARG          -2

/*!
 *  @def    GateMP_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define GateMP_E_MEMORY              -3

/*!
 *  @def    GateMP_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define GateMP_E_ALREADYEXISTS       -4

/*!
 *  @def    GateMP_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define GateMP_E_NOTFOUND            -5

/*!
 *  @def    GateMP_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define GateMP_E_TIMEOUT             -6

/*!
 *  @def    GateMP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define GateMP_E_INVALIDSTATE        -7

/*!
 *  @def    GateMP_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call  */
#define GateMP_E_OSFAILURE           -8

/*!
 *  @def    GateMP_E_RESOURCE
 *  @brief  Specified resource is not available  */
#define GateMP_E_RESOURCE            -9

/*!
 *  @def    GateMP_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation  */
#define GateMP_E_RESTART             -10


/*!
 *  @brief  A set of local context protection levels
 *
 *  Each member corresponds to a specific local processor gates used for
 *  local protection.
 *
 *  In linux user mode, the following are the mapping for the constants
 *      - INTERRUPT -> [N/A]
 *      - TASKLET   -> [N/A]
 *      - THREAD    -> GateMutex
 *      - PROCESS   -> GateMutex
 *
 *  In linux kernel mode, the following are the mapping for the constants
 *      - INTERRUPT -> [Interrupts disabled]
 *      - TASKLET   -> GateMutex
 *      - THREAD    -> GateMutex
 *      - PROCESS   -> GateMutex
 *
 *  For SYS/BIOS users, the following are the mappings for the constants
 *      - INTERRUPT -> GateHwi: disables interrupts
 *      - TASKLET   -> GateSwi: disables Swi's (software interrupts)
 *      - THREAD    -> GateMutexPri: based on Semaphores
 *      - PROCESS   -> GateMutexPri: based on Semaphores
 */
typedef enum GateMP_LocalProtect {
    GateMP_LocalProtect_NONE      = 0,
    /*!< Use no local protection */

    GateMP_LocalProtect_INTERRUPT = 1,
    /*!< Use the INTERRUPT local protection level */

    GateMP_LocalProtect_TASKLET   = 2,
    /*!< Use the TASKLET local protection level */

    GateMP_LocalProtect_THREAD    = 3,
    /*!< Use the THREAD local protection level */

    GateMP_LocalProtect_PROCESS   = 4
    /*!< Use the PROCESS local protection level */

} GateMP_LocalProtect;


/*!
 *  @brief  Type of remote Gate
 *
 *  Each member corresponds to a specific type of remote gate.
 *  Each enum value corresponds to the following remote protection levels:
 *      - NONE      -> No remote protection (the GateMP instance will
 *                     exclusively offer local protection configured in
 *                     #GateMP_Params::localProtect
 *      - SYSTEM    -> Use the SYSTEM remote protection level (default for
 *                     remote protection
 *      - CUSTOM1   -> Use the CUSTOM1 remote protection level
 *      - CUSTOM2   -> Use the CUSTOM2 remote protection level
 */
typedef enum GateMP_RemoteProtect {
    GateMP_RemoteProtect_NONE     = 0,
    /*!< No remote protection (the GateMP instance will exclusively
     *  offer local protection configured in #GateMP_Params::localProtect)
     */

    GateMP_RemoteProtect_SYSTEM   = 1,
    /*!< Use the SYSTEM remote protection level (default remote protection) */

    GateMP_RemoteProtect_CUSTOM1  = 2,
    /*!< Use the CUSTOM1 remote protection level */

    GateMP_RemoteProtect_CUSTOM2  = 3
    /*!< Use the CUSTOM2 remote protection level */

} GateMP_RemoteProtect;

/*!
 *  @brief  GateMP_Handle type
 */
typedef struct GateMP_Object *GateMP_Handle;

/*!
 *  @brief  Structure defining parameters for the GateMP module.
 */
typedef struct GateMP_Params {
    String name;
    /*!< Name of this instance.
     *
     *  The name (if not NULL) must be unique among all GateMP
     *  instances in the entire system.  When creating a new
     *  heap, it is necessary to supply an instance name.
     */

    UInt32 regionId;
    /*!< Shared region ID
     *
     *  The index corresponding to the shared region from which shared memory
     *  will be allocated.
     *
     *  If not specified, the default of '0' will be used.
     */

    /*! @cond */
    Ptr sharedAddr;
    /*!< Physical address of the shared memory
     *
     *  This value can be left as 'null' unless it is required to place the
     *  heap at a specific location in shared memory.  If sharedAddr is null,
     *  then shared memory for a new instance will be allocated from the
     *  heap belonging to the region identified by #GateMP_Params::regionId.
     */
    /*! @endcond */

    GateMP_LocalProtect localProtect;
    /*!< Local protection level
     *
     *   The default value is #GateMP_LocalProtect_THREAD
     */

    GateMP_RemoteProtect remoteProtect;
    /*!< Remote protection level
     *
     *   The default value is #GateMP_RemoteProtect_SYSTEM
     */
} GateMP_Params;

/* =============================================================================
 *  GateMP Module-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Close an opened gate
 *
 *  @param[in,out]  handlePtr   Pointer to handle to opened GateMP instance
 *
 *  @return     GateMP status
 */
Int GateMP_close(GateMP_Handle *handlePtr);

/*!
 *  @brief      Create a GateMP instance
 *
 *  The params structure should be initialized using GateMP_Params_init.
 *
 *  @param[in]  params      GateMP parameters
 *
 *  @return     GateMP Handle
 */
GateMP_Handle GateMP_create(const GateMP_Params *params);

/*!
 *  @brief      Delete a created GateMP instance
 *
 *  @param[in,out]  handlePtr       Pointer to GateMP handle
 *
 *  @return     GateMP Status
 */
Int GateMP_delete(GateMP_Handle *handlePtr);

/*!
 *  @brief      Get the default remote gate
 *
 *  @return     GateMP handle
 */
GateMP_Handle GateMP_getDefaultRemote(Void);

/*!
 *  @brief      Open a created GateMP by name
 *
 *  @param[in]  name        Name of the GateMP instance
 *  @param[out] handlePtr   Pointer to GateMP handle to be opened
 *
 *  @return     GateMP status
 */
Int GateMP_open(String name, GateMP_Handle *handlePtr);

/*! @cond */
Int GateMP_openByAddr(Ptr sharedAddr, GateMP_Handle *handlePtr);
/*! @endcond */

/*!
 *  @brief      Initialize a GateMP parameters struct
 *
 *  @param[out] params      Pointer to GateMP parameters
 *
 */
Void GateMP_Params_init(GateMP_Params *params);

/*! @cond */
/*!
 *  @brief      Amount of shared memory required for creation of each instance
 *
 *  @param[in]  params      Pointer to the parameters that will be used in
 *                          the create.
 *
 *  @return     Number of MAUs needed to create the instance.
 */
SizeT GateMP_sharedMemReq(const GateMP_Params *params);
/*! @endcond */


/* =============================================================================
 *  HeapMemMP Per-instance functions
 * =============================================================================
 */

/*!
 *  @brief      Enter the GateMP
 *
 *  @param[in]  handle      GateMP handle
 *
 *  @return     key that must be used to leave the gate
 */
IArg GateMP_enter(GateMP_Handle handle);

/*!
 *  @brief      Leave the GateMP
 *
 *  @param[in]  handle      GateMP handle
 *  @param[in]  key         key returned from GateMP_enter
 */
Void GateMP_leave(GateMP_Handle handle, IArg key);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_GateMP__include */
