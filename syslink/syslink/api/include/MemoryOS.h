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
 *  @file   MemoryOS.h
 *
 *  @brief      Memory abstraction APIs for local memory allocation.
 *
 *              This provides a direct access to local memory allocation, which
 *              does not require creation of a Heap.
 *  ============================================================================
 */


#ifndef MEMORYOS_H_0x97D2
#define MEMORYOS_H_0x97D2

/* OSAL and utils */
#include <MemoryDefs.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    MEMORYOS_MODULEID
 *  @brief  Module ID for Memory OSAL module.
 */
#define MEMORYOS_MODULEID                 (UInt16) 0x97D2

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
 * @def   MEMORYOS_STATUSCODEBASE
 * @brief Stauts code base for MEMORY module.
 */
#define MEMORYOS_STATUSCODEBASE            (MEMORYOS_MODULEID << 12u)

/*!
 * @def   MEMORYOS_MAKE_FAILURE
 * @brief Convert to failure code.
 */
#define MEMORYOS_MAKE_FAILURE(x)          ((Int) (0x80000000  \
                                           + (MEMORYOS_STATUSCODEBASE + (x))))
/*!
 * @def   MEMORYOS_MAKE_SUCCESS
 * @brief Convert to success code.
 */
#define MEMORYOS_MAKE_SUCCESS(x)            (MEMORYOS_STATUSCODEBASE + (x))

/*!
 * @def   MEMORYOS_E_MEMORY
 * @brief Indicates Memory alloc/free failure.
 */
#define MEMORYOS_E_MEMORY                   MEMORYOS_MAKE_FAILURE(1)

/*!
 * @def   MEMORYOS_E_INVALIDARG
 * @brief Invalid argument provided.
 */
#define MEMORYOS_E_INVALIDARG               MEMORYOS_MAKE_FAILURE(2)

/*!
 * @def   MEMORYOS_E_MAP
 * @brief Failed to map to host address space.
 */
#define MEMORYOS_E_MAP                      MEMORYOS_MAKE_FAILURE(3)

/*!
 * @def   MEMORYOS_E_UNMAP
 * @brief Failed to unmap from host address space.
 */
#define MEMORYOS_E_UNMAP                    MEMORYOS_MAKE_FAILURE(4)

/*!
 * @def   MEMORYOS_E_INVALIDSTATE
 * @brief Module is in invalidstate.
 */
#define MEMORYOS_E_INVALIDSTATE             MEMORYOS_MAKE_FAILURE(5)

/*!
 * @def   MEMORYOS_E_FAIL
 * @brief Genral failure.
 */
#define MEMORYOS_E_FAIL                     MEMORYOS_MAKE_FAILURE(6)

/*!
 * @def   MEMORYOS_SUCCESS
 * @brief Operation successfully completed
 */
#define MEMORYOS_SUCCESS                    MEMORYOS_MAKE_SUCCESS(0)

/*!
 * @def   MEMORYOS_S_ALREADYSETUP
 * @brief Module already initialized
 */
#define MEMORYOS_S_ALREADYSETUP             MEMORYOS_MAKE_SUCCESS(1)


/* =============================================================================
 *  Macros and types
 *  See MemoryDefs.h
 * =============================================================================
 */
/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Initialize the memory os module. */
Int32 MemoryOS_setup (void);

/* Finalize the memory os module. */
Int32 MemoryOS_destroy (void);

/* Allocates the specified number of bytes of type specified through flags. */
Ptr MemoryOS_alloc (UInt32 size, UInt32 align, UInt32 flags);

/* Allocates the specified number of bytes and memory is set to zero. */
Ptr MemoryOS_calloc (UInt32 size, UInt32 align, UInt32 flags);

/* Frees local memory */
Void MemoryOS_free (Ptr ptr, UInt32 size, UInt32 flags);

/* Maps a memory area into virtual space. */
Int MemoryOS_map (Memory_MapInfo * mapInfo);

/* UnMaps a memory area into virtual space. */
Int MemoryOS_unmap (Memory_UnmapInfo * unmapInfo);

/* Copies the data between memory areas. Returns the number of bytes copied. */
Ptr MemoryOS_copy (Ptr dst, Ptr src, UInt32 len);

/* Set the specified values to the allocated  memory area */
Ptr MemoryOS_set (Ptr buf, Int value, UInt32 len);

/* Translate API */
Ptr MemoryOS_translate (Ptr srcAddr, Memory_XltFlags flags);

/* TBD: Add APIs for Memory_move, Scatter-Gather & translateAddr. */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ifndef MEMORYOS_H_0x97D2 */
