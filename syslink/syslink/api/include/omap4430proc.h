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
 *  @file   omap4430proc.h
 *
 *  @brief      Processor interface for OMAP4430SLAVE.
 *  ============================================================================
 */


#ifndef OMAP4430PROC_H_0xbbec
#define OMAP4430PROC_H_0xbbec


/* Standard headers */
#include <Std.h>

/* Module headers */
#include <ProcMgr.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    OMAP4430PROC_MODULEID
 *  @brief  Module ID for OMAP4430SLAVE.
 */
#define OMAP4430PROC_MODULEID           (UInt16) 0xbbec

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
 *  @def    OMAP4430PROC_STATUSCODEBASE
 *  @brief  Error code base for ProcMgr.
 */
#define OMAP4430PROC_STATUSCODEBASE      (OMAP4430PROC_MODULEID << 12u)

/*!
 *  @def    OMAP4430PROC_MAKE_FAILURE
 *  @brief  Macro to make failure code.
 */
#define OMAP4430PROC_MAKE_FAILURE(x)    ((Int)(  0x80000000                    \
                                         | (OMAP4430PROC_STATUSCODEBASE + (x))))

/*!
 *  @def    OMAP4430PROC_MAKE_SUCCESS
 *  @brief  Macro to make success code.
 */
#define OMAP4430PROC_MAKE_SUCCESS(x)    (OMAP4430PROC_STATUSCODEBASE + (x))

/*!
 *  @def    OMAP4430PROC_E_MMUENTRYEXISTS
 *  @brief  Specified MMU entry already exists.
 */
#define OMAP4430PROC_E_MMUENTRYEXISTS   OMAP4430PROC_MAKE_FAILURE(1)

/*!
 *  @def    OMAP4430PROC_E_ISR
 *  @brief  Error occurred during ISR operation.
 */
#define OMAP4430PROC_E_ISR              OMAP4430PROC_MAKE_FAILURE(2)

/*!
 *  @def    OMAP4430PROC_E_MMUCONFIG
 *  @brief  Error occurred during MMU configuration
 */
#define OMAP4430PROC_E_MMUCONFIG        OMAP4430PROC_MAKE_FAILURE(3)

/*!
 *  @def    OMAP4430PROC_E_OSFAILURE
 *  @brief  Failure in an OS-specific operation.
 */
#define OMAP4430PROC_E_OSFAILURE        OMAP4430PROC_MAKE_FAILURE(4)

/*!
 *  @def    OMAP4430PROC_E_INVALIDARG
 *  @brief  Argument passed to a function is invalid.
 */
#define OMAP4430PROC_E_INVALIDARG       OMAP4430PROC_MAKE_FAILURE(5)

/*!
 *  @def    OMAP4430PROC_E_MEMORY
 *  @brief  Memory allocation failed.
 */
#define OMAP4430PROC_E_MEMORY           OMAP4430PROC_MAKE_FAILURE(6)

/*!
 *  @def    OMAP4430PROC_E_HANDLE
 *  @brief  Invalid object handle specified
 */
#define OMAP4430PROC_E_HANDLE           OMAP4430PROC_MAKE_FAILURE(7)

/*!
 *  @def    OMAP4430PROC_E_ACCESSDENIED
 *  @brief  The operation is not permitted in this process.
 */
#define OMAP4430PROC_E_ACCESSDENIED     OMAP4430PROC_MAKE_FAILURE(8)

/*!
 *  @def    OMAP4430PROC_E_FAIL
 *  @brief  Generic failure.
 */
#define OMAP4430PROC_E_FAIL             OMAP4430PROC_MAKE_FAILURE(9)

/*!
 *  @def    OMAP4430PROC_SUCCESS
 *  @brief  Operation successful.
 */
#define OMAP4430PROC_SUCCESS           OMAP4430PROC_MAKE_SUCCESS(0)

/*!
 *  @def    OMAP4430PROC_S_ALREADYSETUP
 *  @brief  The OMAP4430PROC module has already been setup in this process.
 */
#define OMAP4430PROC_S_ALREADYSETUP     OMAP4430PROC_MAKE_SUCCESS(1)

/*!
 *  @def    OMAP4430PROC_S_OPENHANDLE
 *  @brief  Other OMAP4430PROC clients have still setup the OMAP4430PROC module.
 */
#define OMAP4430PROC_S_SETUP            OMAP4430PROC_MAKE_SUCCESS(2)

/*!
 *  @def    OMAP4430PROC_S_OPENHANDLE
 *  @brief  Other OMAP4430PROC handles are still open in this process.
 */
#define OMAP4430PROC_S_OPENHANDLE       OMAP4430PROC_MAKE_SUCCESS(3)

/*!
 *  @def    OMAP4430PROC_S_ALREADYEXISTS
 *  @brief  The OMAP4430PROC instance has already been created/opened in this
 *          process
 */
#define OMAP4430PROC_S_ALREADYEXISTS    OMAP4430PROC_MAKE_SUCCESS(4)


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  Module configuration structure.
 */
typedef struct OMAP4430PROC_Config {
    Handle gateHandle;
    /*!< Handle of gate to be used for local thread safety */
} OMAP4430PROC_Config;

/*!
 *  @brief  Memory entry for slave memory map configuration
 */
typedef struct OMAP4430PROC_MemEntry_tag {
    Char                     name [PROCMGR_MAX_STRLEN];
    /*!< Name identifying the memory region. */
    UInt32                   physAddr;
    /*!< Physical address of the memory region. */
    UInt32                   slaveVirtAddr;
    /*!< Slave virtual address of the memory region. */
    UInt32                   masterVirtAddr;
    /*!< Master virtual address of the memory region. If specified as -1,
         the master virtual address is assumed to be invalid, and shall be
         set internally within the Processor module. */
    UInt32                   size;
    /*!< Size (in bytes) of the memory region. */
    Bool                     shared;
    /*!< Flag indicating whether the memory region is shared between master
         and slave. */
} OMAP4430PROC_MemEntry;

/*!
 *  @brief  Configuration parameters specific to this processor.
 */
typedef struct OMAP4430PROC_Params_tag {
    UInt32                  numMemEntries;
    /*!< Number of memory regions to be configured. */
    OMAP4430PROC_MemEntry * memEntries;
    /*!< Array of information structures for memory regions to be configured. */
    UInt32                  resetVectorMemEntry;
    /*!< Index of the memory entry within the memEntries array, which is the
         resetVector memory region. */
} OMAP4430PROC_Params;

typedef struct OMAP4430PROC_Object * OMAP4430PROC_Handle;

/* =============================================================================
 *  APIs
 * =============================================================================
 */

/* Function to get the default configuration for the OMAP4430PROC module. */
Void OMAP4430PROC_getConfig (OMAP4430PROC_Config * cfg);

/* Function to setup the OMAP4430PROC module. */
Int OMAP4430PROC_setup (OMAP4430PROC_Config * cfg);

/* Function to destroy the OMAP4430PROC module. */
Int OMAP4430PROC_destroy (Void);

/* Function to initialize the parameters for this processor instance. */
Void OMAP4430PROC_Params_init (Handle                 handle,
                               OMAP4430PROC_Params *  params);

/* Function to create an instance of this processor. */
Handle OMAP4430PROC_create (      UInt16                procId,
                            const OMAP4430PROC_Params * params);

/* Function to delete an instance of this processor. */
Int OMAP4430PROC_delete (Handle * handlePtr);

/* Function to open an instance of this processor. */
Int OMAP4430PROC_open (Handle * handlePtr, UInt16 procId);

/* Function to close an instance of this processor. */
Int OMAP4430PROC_close (OMAP4430PROC_Handle *handle);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* OMAP4430PROC_H_0xbbec */
