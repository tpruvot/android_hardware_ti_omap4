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
 *  @file   ProcDefs.h
 *
 *  @brief      Definitions for the Processor interface.
 *
 *  ============================================================================
 */


#ifndef ProcDefs_H_0x6a85
#define ProcDefs_H_0x6a85


/* Standard headers */
#include <Std.h>

/* Module level headers */
#include <ProcMgr.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief   Enumerates the types of Endianism of slave processor.
 */
typedef enum {
    Processor_Endian_Default  = 0u,
    /*!< Default endianism (no conversion required) */
    Processor_Endian_Big      = 1u,
    /*!< Big endian */
    Processor_Endian_Little   = 2u,
    /*!< Little endian */
    Processor_Endian_EndValue = 3u
    /*!< End delimiter indicating start of invalid values for this enum */
} Processor_Endian;


/*!
 *  @brief  Configuration parameters for attaching to the slave Processor
 */
typedef struct Processor_AttachParams_tag {
    ProcMgr_AttachParams * params;
    /*!< Common attach parameters for ProcMgr */
    UInt16                 numMemEntries;
    /*!< Number of valid memory entries */
    ProcMgr_AddrInfo       memEntries [PROCMGR_MAX_MEMORY_REGIONS];
    /*!< Configuration of memory regions */
} Processor_AttachParams ;

/*!
 *  @brief  Configuration parameters for starting the slave Processor
 */
typedef struct Processor_StartParams_tag {
    ProcMgr_StartParams * params;
    /*!< Common start parameters for ProcMgr */
} Processor_StartParams ;


/* =============================================================================
 *  Function pointer types
 * =============================================================================
 */
/*!
 *  @brief  Function pointer type for the function to attach to the processor.
 */
typedef Int (*Processor_AttachFxn) (Handle                   handle,
                                    Processor_AttachParams * params);

/*!
 *  @brief  Function pointer type for the function to detach from the
 *          procssor
 */
typedef Int (*Processor_DetachFxn) (Handle handle);

/*!
 *  @brief  Function pointer type for the function to start the processor.
 */
typedef Int (*Processor_StartFxn) (Handle                  handle,
                                   UInt32                  entryPt,
                                   Processor_StartParams * params);

/*!
 *  @brief  Function pointer type for the function to stop the processor.
 */
typedef Int (*Processor_StopFxn) (Handle handle);

/*!
 *  @brief  Function pointer type for the function to read from the slave
 *          processor's memory.
 */
typedef Int (*Processor_ReadFxn) (Handle   handle,
                                  UInt32   procAddr,
                                  UInt32 * numBytes,
                                  Ptr      buffer);

/*!
 *  @brief  Function pointer type for the function to write into the slave
 *          processor's memory.
 */
typedef Int (*Processor_WriteFxn) (Handle   handle,
                                   UInt32   procAddr,
                                   UInt32 * numBytes,
                                   Ptr      buffer);

/*!
 *  @brief  Function pointer type for the function to perform device-dependent
 *          operations.
 */
typedef Int (*Processor_ControlFxn) (Handle handle, Int32 cmd, Ptr arg);

/*!
 *  @brief  Function pointer type for the function to translate between
 *          two types of address spaces.
 */
typedef Int (*Processor_TranslateAddrFxn) (Handle           handle,
                                           Ptr *            dstAddr,
                                           ProcMgr_AddrType dstAddrType,
                                           Ptr              srcAddr,
                                           ProcMgr_AddrType srcAddrType);

/*!
 *  @brief  Function pointer type for the function to map address to slave
 *          address space
 */
typedef Int (*Processor_MapFxn) (Handle             handle,
                                 UInt32             procAddr,
                                 UInt32             size,
                                 UInt32 *           mappedAddr,
                                 UInt32 *           mappedSize,
								 UInt32				map_attribs);

typedef Int (*Processor_UnMapFxn) (Handle  handle, UInt32 mapped_addr);

/* =============================================================================
 *  Function table interface
 * =============================================================================
 */
/*!
 *  @brief  Function table interface for Processor.
 */
typedef struct Processor_FxnTable_tag {
    Processor_AttachFxn        attach;
    /*!< Function to attach to the slave processor */
    Processor_DetachFxn        detach;
    /*!< Function to detach from the slave processor */
    Processor_StartFxn         start;
    /*!< Function to start the slave processor */
    Processor_StopFxn          stop;
    /*!< Function to stop the slave processor */
    Processor_ReadFxn          read;
    /*!< Function to read from the slave processor's memory */
    Processor_WriteFxn         write;
    /*!< Function to write into the slave processor's memory */
    Processor_ControlFxn       control;
    /*!< Function to perform device-dependent control function */
    Processor_TranslateAddrFxn translateAddr;
    /*!< Function to translate between address ranges */
    Processor_MapFxn           map;
    /*!< Function to map slave addresses to master address space */
	Processor_UnMapFxn           unmap;
    /*!< Function to unmap slave addresses  */
} Processor_FxnTable;


/* =============================================================================
 * Processor structure
 * =============================================================================
 */
/*
 *  Generic Processor object. This object defines the handle type for all
 *  Processor operations.
 */
typedef struct Processor_Object_tag {
    Processor_FxnTable      procFxnTable;
    /*!< Interface function table to plug into the generic Processor. */
    ProcMgr_State           state;
    /*!< State of the slave processor */
    ProcMgr_BootMode        bootMode;
    /*!< Boot mode for the slave processor. */
    Ptr                     object;
    /*!< Pointer to Processor-specific object. */
    UInt16                  procId;
    /*!< Processor ID addressed by this Processor instance. */
} Processor_Object;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ProcDefs_H_0x6a85 */
