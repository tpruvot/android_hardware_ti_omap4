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
 *  @file   ProcMgr.h
 *
 *              The Device Manager (ProcMgr module) on a master Processor
 *              provides control functionality for a slave device.
 *
 *  ============================================================================
 */


#ifndef ProcMgr_H_0xf2ba
#define ProcMgr_H_0xf2ba


/* Standard headers */
#include <Std.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    PROCMGR_MODULEID
 *  @brief  Module ID for ProcMgr.
 */
#define PROCMGR_MODULEID           (UInt16) 0xf2ba


/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
 *  @def    PROCMGR_STATUSCODEBASE
 *  @brief  Error code base for ProcMgr.
 */
#define PROCMGR_STATUSCODEBASE      (PROCMGR_MODULEID << 12u)

/*!
 *  @def    PROCMGR_MAKE_FAILURE
 *  @brief  Macro to make failure code.
 */
#define PROCMGR_MAKE_FAILURE(x)    ((Int)(  0x80000000                           \
                                      | (PROCMGR_STATUSCODEBASE + (x))))

/*!
 *  @def    PROCMGR_MAKE_SUCCESS
 *  @brief  Macro to make success code.
 */
#define PROCMGR_MAKE_SUCCESS(x)    (PROCMGR_STATUSCODEBASE + (x))

/*!
 *  @def    PROCMGR_E_INVALIDARG
 *  @brief  Argument passed to a function is invalid.
 */
#define PROCMGR_E_INVALIDARG       PROCMGR_MAKE_FAILURE(1)

/*!
 *  @def    PROCMGR_E_MEMORY
 *  @brief  Memory allocation failed.
 */
#define PROCMGR_E_MEMORY           PROCMGR_MAKE_FAILURE(2)

/*!
 *  @def    PROCMGR_E_FAIL
 *  @brief  Generic failure.
 */
#define PROCMGR_E_FAIL             PROCMGR_MAKE_FAILURE(3)

/*!
 *  @def    PROCMGR_E_INVALIDSTATE
 *  @brief  Module is in invalid state.
 */
#define PROCMGR_E_INVALIDSTATE     PROCMGR_MAKE_FAILURE(4)

/*!
 *  @def    PROCMGR_E_HANDLE
 *  @brief  Invalid object handle specified
 */
#define PROCMGR_E_HANDLE           PROCMGR_MAKE_FAILURE(5)

/*!
 *  @def    PROCMGR_E_OSFAILURE
 *  @brief  Failure in an OS-specific operation.
 */
#define PROCMGR_E_OSFAILURE        PROCMGR_MAKE_FAILURE(6)

/*!
 *  @def    PROCMGR_E_ACCESSDENIED
 *  @brief  The operation is not permitted in this process.
 */
#define PROCMGR_E_ACCESSDENIED     PROCMGR_MAKE_FAILURE(7)

/*!
 *  @def    PROCMGR_E_TRANSLATE
 *  @brief  An address translation error occurred.
 */
#define PROCMGR_E_TRANSLATE        PROCMGR_MAKE_FAILURE(8)

/*!
 *  @def    PROCMGR_E_TIMEOUT
 *  @brief  The operation has timed out.
 */
#define PROCMGR_E_TIMEOUT          PROCMGR_MAKE_FAILURE(9)

/*!
 *  @def    PROCMGR_SUCCESS
 *  @brief  Operation successful.
 */
#define PROCMGR_SUCCESS            PROCMGR_MAKE_SUCCESS(0)

/*!
 *  @def    PROCMGR_S_ALREADYSETUP
 *  @brief  The ProcMgr module has already been setup in this process.
 */
#define PROCMGR_S_ALREADYSETUP     PROCMGR_MAKE_SUCCESS(1)

/*!
 *  @def    PROCMGR_S_OPENHANDLE
 *  @brief  Other ProcMgr clients have still setup the ProcMgr module.
 */
#define PROCMGR_S_SETUP            PROCMGR_MAKE_SUCCESS(2)

/*!
 *  @def    PROCMGR_S_OPENHANDLE
 *  @brief  Other ProcMgr handles are still open in this process.
 */
#define PROCMGR_S_OPENHANDLE       PROCMGR_MAKE_SUCCESS(3)

/*!
 *  @def    PROCMGR_S_ALREADYEXISTS
 *  @brief  The ProcMgr instance has already been created/opened in this process
 */
#define PROCMGR_S_ALREADYEXISTS    PROCMGR_MAKE_SUCCESS(4)


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  Maximum name length for ProcMgr module strings.
 */
#define PROCMGR_MAX_STRLEN 32u

/*!
 *  @brief  Maximum number of memory regions supported by ProcMgr module.
 */
#define PROCMGR_MAX_MEMORY_REGIONS  32u

/*!
 *  @brief  Defines ProcMgr object handle
 */
typedef struct ProcMgr_Object * ProcMgr_Handle;


/*!
 *  @brief  Enumerations to indicate Processor states.
 */
typedef enum {
    ProcMgr_State_Unknown     = 0u,
    /*!< Unknown Processor state (e.g. at startup or error). */
    ProcMgr_State_Powered     = 1u,
    /*!< Indicates the Processor is powered up. */
    ProcMgr_State_Reset       = 2u,
    /*!< Indicates the Processor is reset. */
    ProcMgr_State_Loaded      = 3u,
    /*!< Indicates the Processor is loaded. */
    ProcMgr_State_Running     = 4u,
    /*!< Indicates the Processor is running. */
    ProcMgr_State_Unavailable = 5u,
    /*!< Indicates the Processor is running. */
    ProcMgr_State_Loading     = 6u,
    /*!< Indicates the Processor is unavailable to the physical transport. */
    ProcMgr_State_EndValue    = 7u
    /*!< End delimiter indicating start of invalid values for this enum */
} ProcMgr_State ;

/*!
 *  @brief  Enumerations to identify the Processor
 */
typedef enum {
    PROC_TESLA = 0,
    PROC_APPM3 = 1,
    PROC_SYSM3 = 2,
    PROC_MPU   = 3,
    PROC_END   = 4
} ProcMgr_ProcId;

/*!
 *  @brief  Enumerations to indicate different types of slave boot modes.
 */
typedef enum {
    ProcMgr_BootMode_Boot     = 0u,
    /*!< ProcMgr is responsible for loading the slave and its reset control. */
    ProcMgr_BootMode_NoLoad   = 1u,
    /*!< ProcMgr is not responsible for loading the slave. It is responsible
         for reset control of the slave. */
    ProcMgr_BootMode_NoBoot   = 2u,
    /*!< ProcMgr is not responsible for loading or reset control of the slave.
         The slave either self-boots, or this is done by some entity outside of
         the ProcMgr module. */
    ProcMgr_BootMode_EndValue = 3u
    /*!< End delimiter indicating start of invalid values for this enum */
} ProcMgr_BootMode ;

/*!
 *  @brief  Enumerations to indicate address types used for translation
 */
typedef enum {
    ProcMgr_AddrType_MasterKnlVirt = 0u,
    /*!< Kernel Virtual address on master processor */
    ProcMgr_AddrType_MasterUsrVirt = 1u,
    /*!< User Virtual address on master processor */
    ProcMgr_AddrType_SlaveVirt     = 2u,
    /*!< Virtual address on slave processor */
    ProcMgr_AddrType_EndValue      = 3u
    /*!< End delimiter indicating start of invalid values for this enum */
} ProcMgr_AddrType;

/*!
 *  @brief  Enumerations to indicate types of address mapping
 */
typedef enum {
    ProcMgr_MapType_Virt     = 0u,
     /*!< Map/unmap to virtual address space (user) */
    ProcMgr_MapType_Phys     = 1u,
    /*!< Map/unmap to Physical address space */
    ProcMgr_MapType_Tiler    = 2u,
    /*!< Map/unmap to TILER address space */
    ProcMgr_MapType_EndValue = 3u
    /*!< End delimiter indicating start of invalid values for this enum */
} ProcMgr_MapType;

/*!
 *  @brief  Event types
 */
typedef enum {
    PROC_MMU_FAULT  = 0u,
    /*!< MMU fault event */
    PROC_ERROR      = 1u,
    /*!< Proc Error event */
    PROC_STOP       = 2u,
    /*!< Proc Stop event */
    PROC_START      = 3u,
    /*!< Proc start event */
    PROC_WATCHDOG   = 4u,
    /*!< Proc WatchDog Timer event */
} ProcMgr_EventType;

/*!
 *  @brief  Enumerations to indicate the available device address memory pools
 */
typedef enum {
    ProcMgr_DMM_MemPool    = 0u,
     /*!< Map/unmap to virtual address space (user) */
    ProcMgr_TILER_MemPool  = 1u,
    /*!< Map/unmap to Tiler address space */
    ProcMgr_NONE_MemPool   = -1,
    /*!< Provide valid Device address as input*/
} ProcMgr_MemPoolType;

/*!
 *  @brief  Enumerations to indicate the Processor version
 */
typedef enum {
    OMAP4_REV_ES1_0 = 0x0,
    OMAP4_REV_ES2_0 = 0x10,
} ProcMgr_cpuRevision;

/*!
 *  @brief  Module configuration structure.
 */
typedef struct ProcMgr_Config {
    UInt32 reserved;
    /*!< Reserved value. */
} ProcMgr_Config;

/*!
 *  @brief  Configuration parameters specific to the slave ProcMgr instance.
 */
typedef struct ProcMgr_Params_tag {
    Ptr    procHandle;
    /*!< Handle to the Processor object associated with this ProcMgr. */
    Ptr    loaderHandle;
    /*!< Handle to the Loader object associated with this ProcMgr. */
    Ptr    pwrHandle;
    /*!< Handle to the PwrMgr object associated with this ProcMgr. */
} ProcMgr_Params ;

/*!
 *  @brief  Configuration parameters specific to the slave ProcMgr instance.
 */
typedef struct ProcMgr_AttachParams_tag {
    ProcMgr_BootMode bootMode;
    /*!< Boot mode for the slave processor. */
} ProcMgr_AttachParams ;

/*!
 *  @brief  Configuration parameters to be provided while starting the slave
 *          processor.
 */
typedef struct ProcMgr_StartParams_tag {
    UInt32 proc_id;
} ProcMgr_StartParams ;

/*!
 *  @brief  Configuration parameters to be provided while stopping the slave
 *          processor.
 */
typedef struct ProcMgr_StopParams_tag {
    UInt32 proc_id;
} ProcMgr_StopParams ;

/*!
 *  @brief  This structure defines information about memory regions mapped by
 *          the ProcMgr module.
 */
typedef struct ProcMgr_AddrInfo_tag {
    Bool    isInit;
    /*!< Is this memory region initialized? */
    UInt32  addr [ProcMgr_AddrType_EndValue];
    /*!< Addresses for each type of address space */
    UInt32  size;
    /*!< Size of the memory region in bytes */
} ProcMgr_AddrInfo;

/*!
 *  @brief  Characteristics of the slave processor
 */
typedef struct ProcMgr_ProcInfo_tag {
    ProcMgr_BootMode    bootMode;
    /*!< Boot mode of the processor. */
    UInt16              numMemEntries;
    /*!< Number of valid memory entries */
    ProcMgr_AddrInfo    memEntries [PROCMGR_MAX_MEMORY_REGIONS];
    /*!< Configuration of memory regions */
} ProcMgr_ProcInfo;

/*!
 *  @brief      Function pointer type that is passed to the
 *              ProcMgr_registerNotify function
 *
 *  @param      procId    Processor ID for the processor that is undergoing the
 *                        state change.
 *  @param      handle    Handle to the processor instance.
 *  @param      fromState Previous processor state
 *  @param      toState   New processor state
 *
 *  @sa         ProcMgr_registerNotify
 */
typedef Int (*ProcMgr_CallbackFxn) (UInt16         procId,
                                    ProcMgr_Handle handle,
                                    ProcMgr_State  fromState,
                                    ProcMgr_State  toState);

/* =============================================================================
 *  APIs
 * =============================================================================
 */

/* Function to get the default configuration for the ProcMgr module. */
Void ProcMgr_getConfig (ProcMgr_Config * cfg);

/* Function to setup the ProcMgr module. */
Int ProcMgr_setup (ProcMgr_Config * cfg);

/* Function to destroy the ProcMgr module. */
Int ProcMgr_destroy (Void);

/* Function to initialize the parameters for the ProcMgr instance. */
Void ProcMgr_Params_init (ProcMgr_Handle handle, ProcMgr_Params * params);

/* Function to create a ProcMgr object for a specific slave processor. */
ProcMgr_Handle ProcMgr_create (UInt16 procId, const ProcMgr_Params * params);

/* Function to delete a ProcMgr object for a specific slave processor. */
Int ProcMgr_delete (ProcMgr_Handle * handlePtr);

/* Function to open a handle to an existing ProcMgr object handling the
 * procId.
 */
Int ProcMgr_open (ProcMgr_Handle * handle, UInt16 procId);

/* Function to close this handle to the ProcMgr instance. */
Int ProcMgr_close (ProcMgr_Handle * handlePtr);

/* Function to initialize the parameters for the ProcMgr attach function. */
Void ProcMgr_getAttachParams (ProcMgr_Handle         handle,
                              ProcMgr_AttachParams * params);

/* Function to attach the client to the specified slave and also initialize the
 * slave (if required).
 */
Int ProcMgr_attach (ProcMgr_Handle handle, ProcMgr_AttachParams * params);

/* Function to detach the client from the specified slave and also finalze the
 * slave (if required).
 */
Int ProcMgr_detach (ProcMgr_Handle handle);

/* Function to load the specified slave executable on the slave Processor. */
Int
ProcMgr_load (ProcMgr_Handle    handle,
              String            imagePath,
              UInt32            argc,
              String *          argv,
              UInt32 *          entry_point,
              UInt32 *          fileId,
              ProcMgr_ProcId    procID);

/* Function to unload the previously loaded file on the slave processor.
 * The fileId received from the load function must be passed to this
 * function.
 */
Int ProcMgr_unload (ProcMgr_Handle handle,
                    UInt32         fileId);

/* Function to initialize the parameters for the ProcMgr start function. */
Void ProcMgr_getStartParams (ProcMgr_Handle        handle,
                             ProcMgr_StartParams * params);

/* Function to starts execution of the loaded code on the slave from the
 * starting point specified in the slave executable loaded earlier by call to
 * ProcMgr_load ().
 */
Int ProcMgr_start (ProcMgr_Handle           handle,
                   UInt32                   entry_point,
                   ProcMgr_StartParams *    params);

/* Function to stop execution of the slave Processor. */
Int ProcMgr_stop (ProcMgr_Handle        handle,
                  ProcMgr_StopParams *  params);

/* Function to get the current state of the slave Processor as maintained on
 * the master Processor state machine.
 */
ProcMgr_State ProcMgr_getState (ProcMgr_Handle handle);

/* Function to set the state of the slave Processor as maintained on
 * the master Processor state machine.
 */
Int ProcMgr_setState (ProcMgr_State   state);


/* Function to read from the slave Processor's memory space. */
Int ProcMgr_read (ProcMgr_Handle handle,
                  UInt32         procAddr,
                  UInt32 *       numBytes,
                  Ptr            buffer);

/* Function to read from the slave Processor's memory space. */
Int ProcMgr_write (ProcMgr_Handle handle,
                   UInt32         procAddr,
                   UInt32 *       numBytes,
                   Ptr            buffer);

/* Function that provides a hook for performing device dependent operations on
 * the slave Processor.
 */
Int ProcMgr_control (ProcMgr_Handle handle, Int32 cmd, Ptr arg);

/* Function to translate between two types of address spaces. */
Int ProcMgr_translateAddr (ProcMgr_Handle   handle,
                           Ptr *            dstAddr,
                           ProcMgr_AddrType dstAddrType,
                           Ptr              srcAddr,
                           ProcMgr_AddrType srcAddrType);

/* Function that gets the slave address corresponding to a symbol within an
 * executable currently loaded on the slave Processor.
 */
Int ProcMgr_getSymbolAddress (ProcMgr_Handle handle,
                              UInt32         fileId,
                              String         symbolName,
                              UInt32 *       symValue);

/* Function that maps the specified slave address to master address space. */
Int ProcMgr_map (ProcMgr_Handle  handle,
                 UInt32          procAddr,
                 UInt32          size,
                 UInt32 *        mappedAddr,
                 UInt32 *        mappedSize,
                 ProcMgr_MapType type,
                 ProcMgr_ProcId  procID);

/* Function that maps the specified slave address to master address space. */
Int ProcMgr_unmap (ProcMgr_Handle  handle,
                   UInt32          mappedAddr,
                   ProcMgr_ProcId procID);

/* Function that registers for notification when the slave processor
 * transitions to any of the states specified.
 */
Int ProcMgr_registerNotify (ProcMgr_Handle      handle,
                            ProcMgr_CallbackFxn fxn,
                            Ptr                 args,
                            ProcMgr_State       state []);

/* Function that returns information about the characteristics of the slave
 * processor.
 */
Int ProcMgr_getProcInfo (ProcMgr_Handle     handle,
                         ProcMgr_ProcInfo * procInfo);

/* Function that performs virtual address to physical address translations.
 */
Int ProcMgr_virtToPhysPages (ProcMgr_Handle handle,
                             UInt32         remoteAddr,
                             UInt32         numOfPages,
                             UInt32 *       physEntries,
                             ProcMgr_ProcId procId);

/* Function to flush cache */
Int ProcMgr_flushMemory (PVOID          bufAddr,
                         UInt32         bufSize,
                         ProcMgr_ProcId procID);

/* Function to invalidate cache */
Int ProcMgr_invalidateMemory (PVOID             bufAddr,
                              UInt32            bufSize,
                              ProcMgr_ProcId    procID);

/* Function to wait for an Event */
Int ProcMgr_waitForEvent (ProcMgr_ProcId    procId,
                          ProcMgr_EventType eventType,
                          Int               timeout);

/* Function to wait for multiple Events */
Int ProcMgr_waitForMultipleEvents (ProcMgr_ProcId      procId,
                                   ProcMgr_EventType * eventType,
                                   UInt32              size,
                                   Int                 timeout,
                                   UInt *              index);

/* Function to get the OMAP revision */
Int ProcMgr_getCpuRev (UInt32 *cpuRev);

/* Function to create DMM pool */
Int ProcMgr_createDMMPool (UInt32   poolId,
                           UInt32   daBegin,
                           UInt32   size,
                           Int      proc);

/* Function to delete DMM pool */
Int ProcMgr_deleteDMMPool (UInt32 poolId, Int proc);

/* =============================================================================
 *  Compatibility layer for SYSBIOS
 * =============================================================================
 */
#define ProcMgr_MODULEID           PROCMGR_MODULEID
#define ProcMgr_STATUSCODEBASE     PROCMGR_STATUSCODEBASE
#define ProcMgr_MAKE_FAILURE       PROCMGR_MAKE_FAILURE
#define ProcMgr_MAKE_SUCCESS       PROCMGR_MAKE_SUCCESS
#define ProcMgr_E_INVALIDARG       PROCMGR_E_INVALIDARG
#define ProcMgr_E_MEMORY           PROCMGR_E_MEMORY
#define ProcMgr_E_FAIL             PROCMGR_E_FAIL
#define ProcMgr_E_INVALIDSTATE     PROCMGR_E_INVALIDSTATE
#define ProcMgr_SUCCESS            PROCMGR_SUCCESS

/* Types of mapping attributes */

/* MPU address is virtual and needs to be translated to physical addr */
#define ProcMgr_MAPVIRTUALADDR          0x00000000
#define ProcMgr_MAPPHYSICALADDR         0x00000001

/* Mapped data is big endian */
#define ProcMgr_MAPBIGENDIAN            0x00000002
#define ProcMgr_MAPLITTLEENDIAN         0x00000000

/* Element size is based on DSP r/w access size */
#define ProcMgr_MAPMIXEDELEMSIZE        0x00000004

/*
 * Element size for MMU mapping (8, 16, 32, or 64 bit)
 * Ignored if DSP_MAPMIXEDELEMSIZE enabled
 */
#define ProcMgr_MAPELEMSIZE8            0x00000008
#define ProcMgr_MAPELEMSIZE16           0x00000010
#define ProcMgr_MAPELEMSIZE32           0x00000020
#define ProcMgr_MAPELEMSIZE64           0x00000040

#define ProcMgr_MAPVMALLOCADDR          0x00000080
#define ProcMgr_MAPTILERADDR            0x00000100


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ProcMgr_H_0xf2ba */
