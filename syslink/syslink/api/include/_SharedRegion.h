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
 *  @file   _SharedRegion.h
 *
 *  @brief  Shared Region Manager internal functions
 *  ============================================================================
 */


#ifndef _SharedRegion_H_0X5D8A
#define _SharedRegion_H_0X5D8A


/* Standard headers */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/HeapMemMP.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    SharedRegion_MODULEID
 *  @brief  Module ID for Shared region manager.
 */
#define SharedRegion_MODULEID      (0x5D8A)

/*!
 *  @brief  Name of the reserved name server used for application.
 */
#define SharedRegion_NAMESERVER    "SHAREDREGION"

/*!
 *  @def        SharedRegion_INVALIDSRPTR
 *  @brief      Invalid SharedRegion pointer. Default value for when translate
 *              is true.
 */
#define SharedRegion_INVALIDSRPTR  (~0)

/*!
 *  @brief  Module configuration structure.
 */
typedef struct SharedRegion_Config {
    SizeT      cacheLineSize;
    /*!<
     *  Worst-case cache line size
     *
     *  This is the default system cache line size for all modules.
     *  When a module puts structures in shared memory, this value is
     *  used to make sure items are aligned on a cache line boundary.
     *  If no cacheLineSize is specified for a region, it will use this
     *  value.
     */

    UInt16     numEntries;
    /*!<
     *  The number of shared region table entries.
     *
     *  This value is used for calculating the number of bits for the offset.
     *  Note: This value must be the same across all processors in the system.
     *        Increasing this parameter will increase the footprint and
     *        the time for translating a pointer to a SRPtr.
     */

    Bool       translate;
    /*!< This configuration parameter should be set to 'true'
     *   if and only if all shared memory regions are the same
     *   for all processors. If 'true', it results in a fast
     *   getPtr and getSRPtr.
     */
} SharedRegion_Config;

/*!
 *  @brief  Information stored on a per region basis
 */
typedef struct SharedRegion_Region_tag {
    SharedRegion_Entry entry;
    SizeT              reservedSize;
    HeapMemMP_Handle   heap;
} SharedRegion_Region;

/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief      Function to setup the SharedRegion module.
 *
 *  @param      cfgParams   Configuration values.
 */
Void SharedRegion_getConfig (SharedRegion_Config * config);

/*!
 *  @brief      Function to setup the SharedRegion module.
 *
 *              Here if user passes both numEntries and indexBits, than
 *              numEntries takes precedence over indexBits.
 *
 *  @param      cfg   Configuration values.
 *
 *  @sa         SharedRegion_destroy
 */
Int SharedRegion_setup (SharedRegion_Config * config);

/*!
 *  @brief      Function to destroy the SharedRegion module.
 *
 *  @sa         SharedRegion_setup
 */
Int SharedRegion_destroy (Void);

/*!
 *  @brief      Creates a heap by owner of region for each SharedRegion.
 *              Function is called by Ipc_start(). Requires that SharedRegion 0
 *              be valid before calling start().
 *
 *  @param      None
 *
 *  @sa         Ipc_start
 */
Int SharedRegion_start (Void);

/*!
 *  @brief      Function to stop Shared Region 0
 *
 *  @param      None
 *
 *  @sa         Ipc_stop
 */
Int SharedRegion_stop (Void);

/*!
 *  @brief      Opens a heap, for non-owner processors, for each SharedRegion.
 *
 *  @param      None
 *
 *  @sa         SharedRegion_detach
 */
Int SharedRegion_attach (UInt16 remoteProcId);

/*!
 *  @brief      Closes a heap, for non-owner processors, for each SharedRegion.
 *
 *  @param      None
 *
 *  @sa         SharedRegion_attach
 */
Int SharedRegion_detach (UInt16 remoteProcId);

/*!
 *  @brief      Reserve shared region memory
 *
 *  @param      id      Shared region id
 *  @param      size    Shared region size
 *
 *  @sa         None
 */
Ptr SharedRegion_reserveMemory (UInt16 id, UInt32 size);


/*!
 *  @brief      Reserve shared region memory
 *
 *  @param      id      Shared region id
 *  @param      size    Shared region size
 *
 *  @sa         None
 */
Void SharedRegion_unreserveMemory (UInt16 id, UInt32 size);


/*!
 *  @brief      Sets the entry at the specified region id (doesn't create heap)
 *
 *  @param      regionId  region id
 *  @param      entry     pointer to set region information.
 *
 *  @return     Status
 *              - #SharedRegion_S_SUCCESS:  Operation was successful
 *              - #SharedRegion_E_FAIL:  Region already exists or overlaps with
 *                                       with another region
 *              - #SharedRegion_E_MEMORY: Unable to create Heap
 */
Int _SharedRegion_setEntry(UInt16 regionId, SharedRegion_Entry *entry);


/*! @brief      Function to clear the reserved memory */
Void SharedRegion_clearReservedMemory (void);

/*! @brief      Return the region info
 *
 *  @param      i       Shared region id
 *  @param      region  Pointer to region structure
 */
Void SharedRegion_getRegionInfo (UInt16                i,
                                 SharedRegion_Region * region);

/* User space internal API */
/*! Sets the regions in user space that are created in knl space and
 * not on user space
 */
Int
_SharedRegion_setRegions (Void);

/*! Clears the regions in user space that are created in knl space and
 * not on user space.
 */
Int
_SharedRegion_clearRegions (Void);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _SharedRegion_H_0X5D8A */
