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
 *  @file       SharedRegion.h
 *
 *  @brief      SharedRegion is a shared memory manager and address translator.
 *
 *  The SharedRegion module is designed to be used in a multi-processor
 *  environment in which memory regions are shared and accessed
 *  across different processors. The module itself does not use any shared
 *  memory, because all module state is stored locally.  SharedRegion
 *  APIs use the system gate for thread protection.
 *
 *  This module creates and stores a local shared memory region table.  The
 *  table contains the processor's view for every shared region in the system.
 *  The table must not contain any overlapping regions.  Each processor's
 *  view of a particular shared memory region is determined by the region id.
 *  In cases where a processor cannot access a certain shared memory region,
 *  that shared memory region should be left invalid for that processor.
 *  Note:  The number of entries must be the same on all processors.
 *
 *  Each shared region contains the following:
 *  @li @b base - The base address
 *  @li @b len - The length
 *  @li @b name - The name of the region
 *  @li @b isValid - Whether the region is valid
 *  @li @b ownerProcId - The id of the processor which owns the region
 *  @li @b cacheEnable - Whether the region is cacheable
 *  @li @b cacheLineSize - The cache line size
 *
 *  ============================================================================
 */

#ifndef ti_ipc_SharedRegion__include
#define ti_ipc_SharedRegion__include


#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    SharedRegion_S_BUSY
 *  @brief  The resource is still in use
 */
#define SharedRegion_S_BUSY             2

/*!
 *  @def    SharedRegion_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define SharedRegion_S_ALREADYSETUP     1

/*!
 *  @def    SharedRegion_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define SharedRegion_S_SUCCESS          0

/*!
 *  @def    SharedRegion_E_FAIL
 *  @brief  Generic failure.
 */
#define SharedRegion_E_FAIL             -1

/*!
 *  @def    SharedRegion_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define SharedRegion_E_INVALIDARG       -2

/*!
 *  @def    SharedRegion_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define SharedRegion_E_MEMORY           -3

/*!
 *  @def    SharedRegion_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define SharedRegion_E_ALREADYEXISTS    -4

/*!
 *  @def    SharedRegion_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define SharedRegion_E_NOTFOUND         -5

/*!
 *  @def    SharedRegion_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define SharedRegion_E_TIMEOUT          -6

/*!
 *  @def    SharedRegion_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define SharedRegion_E_INVALIDSTATE     -7

/*!
 *  @def    SharedRegion_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define SharedRegion_E_OSFAILURE        -8

/*!
 *  @def    SharedRegion_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define SharedRegion_E_RESOURCE         -9

/*!
 *  @def    SharedRegion_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define SharedRegion_E_RESTART          -10

/*!
 *  @def        SharedRegion_INVALIDREGIONID
 *  @brief      Invalid region id
 */
#define SharedRegion_INVALIDREGIONID (0xFFFF)

/*!
 *  @def        SharedRegion_DEFAULTOWNERID
 *  @brief      Default owner processor id
 */
#define SharedRegion_DEFAULTOWNERID (UInt16)(~0)

/*!
 *  @def        SharedRegion_INVALIDSRPTR
 *  @brief      Invalid SharedRegion pointer
 */
#define SharedRegion_INVALIDSRPTR (~0)

/*!
 *  @brief      SharedRegion pointer type
 */
typedef Bits32 SharedRegion_SRPtr;

/*!
 *  @brief      Structure defining a region
 */
typedef struct SharedRegion_Entry {
    /*! The base address of the region */
    Ptr    base;

    /*! The length of the region */
    SizeT  len;

    /*! The MultiProc id of the owner of the region */
    UInt16  ownerProcId;

    /*! Whether the region is valid */
    Bool   isValid;

    /*! Whether to perform cache operations for the region */
    Bool   cacheEnable;

    /*! The cache line size of the region */
    SizeT  cacheLineSize;

    /*! Whether a heap is created for the region */
    Bool  createHeap;

    /*! The name of the region */
    String name;
} SharedRegion_Entry;


/* =============================================================================
 *  SharedRegion Moduel-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Clears the entry at the specified region id
 *
 *  @param      regionId  the region id
 *
 *  @return     Status
 *              - #SharedRegion_S_SUCCESS:  Operation was successful
 *              - #SharedRegion_E_FAIL:  Delete or close of heap created
 *                                       in region failed.
 *
 *  @sa         SharedRegion_setEntry
 */
Int SharedRegion_clearEntry(UInt16 regionId);

/*!
 *  @brief      Initializes the entry fields
 *
 *  @param      entry  pointer to a SharedRegion entry
 *
 *  @sa         SharedRegion_setEntry
 */
Void SharedRegion_entryInit(SharedRegion_Entry *entry);

/*!
 *  @brief      Gets the cache line size for the specified region id
 *
 *  @param      regionId  the region id
 *
 *  @return     the specified cache line size
 *
 *  @sa         SharedRegion_isCacheEnabled
 */
SizeT SharedRegion_getCacheLineSize(UInt16 regionId);

/*!
 *  @brief      Gets the entry information for the specified region id
 *
 *  @param      regionId  the region id
 *  @param      entry     pointer to return region information
 *
 *  @return     Status
 *              - #SharedRegion_S_SUCCESS:  Operation was successful
 *              - #SharedRegion_E_FAIL:  Operation failed
 *
 *  @sa         SharedRegion_setEntry
 */
Int SharedRegion_getEntry(UInt16 regionId, SharedRegion_Entry *entry);

/*!
 *  @brief      Gets the heap associated with the specified region id
 *
 *  @param      regionId  the region id
 *
 *  @return     Handle of the heap
 */
Ptr SharedRegion_getHeap(UInt16 regionId);

/*!
 *  @brief      Gets the region id for the specified address
 *
 *  @param      addr  address
 *
 *  @return     region id
 */
UInt16 SharedRegion_getId(Ptr addr);

/*!
 *  @brief      Gets the id of a region, given the name
 *
 *  @param      name  name of the region
 *
 *  @return     region id
 */
UInt16 SharedRegion_getIdByName(String name);

/*!
 *  @brief      Gets the number of regions
 *
 *  @return     number of regions
 */
UInt16 SharedRegion_getNumRegions(Void);

/*!
 *  @brief      Calculate the local pointer from the shared region pointer
 *
 *  @param      srptr  SharedRegion pointer
 *
 *  @return     local pointer
 *
 *  @sa         SharedRegion_getPtr
 */
Ptr SharedRegion_getPtr(SharedRegion_SRPtr srptr);

/*!
 *  @brief      Calculate the shared region pointer given local address and id
 *
 *  @param      addr      the local address
 *  @param      regionId  region id
 *
 *  @return     SharedRegion pointer
 *
 *  @sa         SharedRegion_getPtr
 */
SharedRegion_SRPtr SharedRegion_getSRPtr(Ptr addr, UInt16 regionId);

/*!
 *  @brief      whether cache enable was specified
 *
 *  @param      regionId  region id
 *
 *  @return     'TRUE' if cache enable specified, otherwise 'FALSE'
 */
Bool SharedRegion_isCacheEnabled(UInt16 regionId);

/*!
 *  @brief      Sets the entry at the specified region id
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
Int SharedRegion_setEntry(UInt16 regionId, SharedRegion_Entry *entry);

/*!
 *  @brief      Whether address translation is enabled
 *
 *  @return     'TRUE' if translate is enabled otherwise 'FALSE'
 */
Bool SharedRegion_translateEnabled(Void);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_SharedRegion__include */
