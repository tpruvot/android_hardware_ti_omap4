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
 *  @file       HeapBufMP.h
 *
 *  @brief      Multi-processor fixed-size buffer heap implementation
 *
 *  Heap implementation that manages fixed size buffers that can be used
 *  in a multiprocessor system with shared memory.
 *
 *  The HeapBufMP manager provides functions to allocate and free storage from a
 *  heap of type HeapBufMP which inherits from IHeap. HeapBufMP manages a single
 *  fixed-size buffer, split into equally sized allocatable blocks.
 *
 *  The HeapBufMP manager is intended as a very fast memory
 *  manager which can only allocate blocks of a single size. It is ideal for
 *  managing a heap that is only used for allocating a single type of object,
 *  or for objects that have very similar sizes.
 *
 *  The HeapBufMP module uses a NameServer instance to
 *  store instance information when an instance is created.  The name supplied
 *  must be unique for all HeapBufMP instances.
 *
 *  The #HeapBufMP_create call initializes the shared memory as needed. Once an
 *  instance is created, a #HeapBufMP_open can be performed. The
 *  open is used to gain access to the same HeapBufMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor(s).
 *
 *  The open returns a HeapBufMP instance handle like the create,
 *  however the open does not modify the shared memory.
 *
 */


#ifndef ti_ipc_HeapBufMP__include
#define ti_ipc_HeapBufMP__include

#if defined (__cplusplus)
extern "C" {
#endif

#include <ti/ipc/GateMP.h>

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
 *  @def    HeapBufMP_S_BUSY
 *  @brief  The resource is still in use
 */
#define HeapBufMP_S_BUSY               2

/*!
 *  @def    HeapBufMP_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define HeapBufMP_S_ALREADYSETUP       1

/*!
 *  @def    HeapBufMP_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define HeapBufMP_S_SUCCESS            0

/*!
 *  @def    HeapBufMP_E_FAIL
 *  @brief  Generic failure.
 */
#define HeapBufMP_E_FAIL              -1

/*!
 *  @def    HeapBufMP_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define HeapBufMP_E_INVALIDARG          -2

/*!
 *  @def    HeapBufMP_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define HeapBufMP_E_MEMORY              -3

/*!
 *  @def    HeapBufMP_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define HeapBufMP_E_ALREADYEXISTS       -4

/*!
 *  @def    HeapBufMP_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define HeapBufMP_E_NOTFOUND            -5

/*!
 *  @def    HeapBufMP_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define HeapBufMP_E_TIMEOUT             -6

/*!
 *  @def    HeapBufMP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define HeapBufMP_E_INVALIDSTATE        -7

/*!
 *  @def    HeapBufMP_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call  */
#define HeapBufMP_E_OSFAILURE           -8

/*!
 *  @def    HeapBufMP_E_RESOURCE
 *  @brief  Specified resource is not available  */
#define HeapBufMP_E_RESOURCE            -10

/*!
 *  @def    HeapBufMP_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation  */
#define HeapBufMP_E_RESTART             -11


/* =============================================================================
 * Macros
 * =============================================================================
 */


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/*!
 *  @brief  HeapBufMP_Handle type
 */
typedef struct HeapBufMP_Object *HeapBufMP_Handle;

/*!
 *  @brief  Structure defining parameters for the HeapBufMP module.
 */
typedef struct HeapBufMP_Params {

    String name;
    /*!< Name of this instance.
     *
     *  The name (if not NULL) must be unique among all HeapBufMP
     *  instances in the entire system.  When creating a new
     *  heap, it is necessary to supply an instance name.
     */

    UInt16 regionId;
    /*!< Shared region ID
     *
     *  The index corresponding to the shared region from which shared memory
     *  will be allocated.
     */

    /*! @cond */
    Ptr sharedAddr;
    /*!< Physical address of the shared memory
     *
     *  This value can be left as 'null' unless it is required to place the
     *  heap at a specific location in shared memory.  If sharedAddr is null,
     *  then shared memory for a new instance will be allocated from the
     *  heap belonging to the region identified by #HeapBufMP_Params::regionId.
     */
    /*! @endcond */

    SizeT blockSize;
    /*!< Size (in MAUs) of each block.
     *
     *  HeapBufMP will round the blockSize up to the nearest multiple of the
     *  alignment, so the actual blockSize may be larger. When creating a
     *  HeapBufMP dynamically, this needs to be taken into account to determine
     *  the proper buffer size to pass in.
     *
     *  Required parameter.
     *
     *  The default size of the blocks is 0 MAUs.
     */

    UInt numBlocks;
    /*!<Number of fixed-size blocks.
     *
     *  This is a required parameter for all new HeapBufMP instances.
     */

    SizeT align;
    /*!< Alignment (in MAUs) of each block.
     *
     *  The alignment must be a power of 2. If the value 0 is specified,
     *  the value will be changed to meet minimum structure alignment
     *  requirements and the cache alignment size of the region in which the
     *  heap will be placed.  Therefore, the actual alignment may be larger.
     *
     *  The default alignment is 0.
     */

    Bool exact;
    /*!< Use exact matching
     *
     *  Setting this flag will allow allocation only if the requested size
     *  is equal to (rather than less than or equal to) the buffer's block
     *  size.
     */

    GateMP_Handle gate;
    /*!< GateMP used for critical region management of the shared memory
     *
     *  Using the default value of NULL will result in use of the GateMP
     *  system gate for context protection.
     */

} HeapBufMP_Params;

/*!
 *  @brief  Stats structure for the HeapBufMP_getExtendedStats API.
 */
typedef struct HeapBufMP_ExtendedStats {
    UInt maxAllocatedBlocks;
    /*< The maximum number of blocks allocated from this heap at any point in
     *  time during the lifetime of this HeapBufMP instance.
     */

    UInt numAllocatedBlocks;
    /*< The total number of blocks currently allocated in this HeapBufMP
     *  instance.
     */
} HeapBufMP_ExtendedStats;

/* =============================================================================
 *  APIs
 * =============================================================================
 */

/*!
 *  @brief      Close a HeapBufMP instance
 *
 *  Closing an instance will free local memory consumed by the opened
 *  instance. All opened instances should be closed before the instance
 *  is deleted.
 *
 *  @param      handlePtr   Pointer to handle returned from #HeapBufMP_open
 *
 *  @sa         HeapBufMP_open
 */
Int HeapBufMP_close(HeapBufMP_Handle *handlePtr);

/*!
 *  @brief      Create a HeapBufMP instance
 *
 *  @param      params      HeapBufMP parameters
 *
 *  @return     HeapBufMP Handle
 */
HeapBufMP_Handle HeapBufMP_create(const HeapBufMP_Params *params);

/*!
 *  @brief      Delete a created HeapBufMP instance
 *
 *  @param      handlePtr   Pointer to handle to delete.
 */
Int HeapBufMP_delete(HeapBufMP_Handle *handlePtr);

/*!
 *  @brief      Open a created HeapBufMP instance
 *
 *  Once an instance is created, an open can be performed. The
 *  open is used to gain access to the same HeapBufMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor.
 *
 *  The open returns a HeapBufMP instance handle like the create,
 *  however the open does not initialize the shared memory. The supplied
 *  name is used to identify the created instance.
 *
 *  Call #HeapBufMP_close when the opened instance is not longer needed.
 *
 *  @param      name        Name of created HeapBufMP instance
 *  @param      handlePtr   Pointer to HeapBufMP handle to be opened
 *
 *  @return     HeapBufMP status:
 *              - #HeapBufMP_S_SUCCESS: Heap successfully opened
 *              - #HeapBufMP_E_FAIL: Heap is not yet ready to be opened.
 *
 *  @sa         HeapBufMP_close
 */
Int HeapBufMP_open(String name, HeapBufMP_Handle *handlePtr);

/*! @cond */
Int HeapBufMP_openByAddr(Ptr sharedAddr, HeapBufMP_Handle *handlePtr);
/*! @endcond */

/*!
 *  @brief      Initialize a HeapBufMP parameters struct
 *
 *  @param[out] params      Pointer to GateMP parameters
 *
 */
Void HeapBufMP_Params_init(HeapBufMP_Params *params);

/*! @cond */
/*!
 *  @brief      Amount of shared memory required for creation of each instance
 *
 *  @param[in]  params      Pointer to the parameters that will be used in
 *                          the create.
 *
 *  @return     Number of MAUs needed to create the instance.
 */
SizeT HeapBufMP_sharedMemReq(const HeapBufMP_Params *params);
/*! @endcond */

/* =============================================================================
 *  HeapBufMP Per-instance functions
 * =============================================================================
 */

/*!
 *  @brief      Allocate a block of memory of specified size and alignment
 *
 *  The actual block returned may be larger than requested to satisfy
 *  alignment requirements.
 *
 *  HeapBufMP_alloc will lock the heap using the HeapBufMP gate
 *  while it traverses the list of free blocks to find a large enough block
 *  for the request.
 *
 *  Guidelines for using large heaps and multiple alloc() calls.
 *      - If possible, allocate larger blocks first. Previous allocations
 *        of small memory blocks can reduce the size of the blocks
 *        available for larger memory allocations.
 *      - Realize that alloc() can fail even if the heap contains a
 *        sufficient absolute amount of unalloccated space. This is
 *        because the largest free memory block may be smaller than
 *        total amount of unallocated memory.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @sa         HeapBufMP_free
 */
Void *HeapBufMP_alloc(HeapBufMP_Handle handle, SizeT size,
                           SizeT align);

/*!
 *  @brief      Frees a block of memory.
 *
 *  free() places the memory block specified by addr and size back into the
 *  free pool of the heap specified. The newly freed block is combined with
 *  any adjacent free blocks. The space is then available for further
 *  allocation by alloc().
 *
 *  #HeapBufMP_free will lock the heap using the HeapBufMP gate if one is
 *  specified or the system GateMP if not.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         HeapBufMP_alloc
 */
Void HeapBufMP_free(HeapBufMP_Handle handle, Ptr block, SizeT size);

/*!
 *  @brief      Get extended memory statistics
 *
 *  This function retrieves the extended statistics for a HeapBufMP
 *  instance.  It does not retrieve the standard Memory_Stats
 *  information.  Refer to #::ExtendedStats for more information
 *  regarding what information is returned.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param[out] stats     ExtendedStats structure
 *
 *  @sa
 */
Void HeapBufMP_getExtendedStats(HeapBufMP_Handle handle,
    HeapBufMP_ExtendedStats *stats);

/*!
 *  @brief      Get memory statistics
 *
 *  @param[in]  handle    Handle to previously created/opened instance.
 *  @param[out] stats     Memory statistics structure
 *
 *  @sa
 */
Void HeapBufMP_getStats(HeapBufMP_Handle handle, Ptr stats);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_HeapBufMP__include */
