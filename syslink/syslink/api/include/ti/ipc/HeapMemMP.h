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
 *  @file       HeapMemMP.h
 *
 *  @brief      Multi-processor variable size buffer heap implementation
 *
 *  HeapMemMP is a heap implementation that manages variable size buffers that
 *  can be used in a multiprocessor system with shared memory. HeapMemMP
 *  manages a single buffer in shared memory from which blocks of user-
 *  specified length are allocated and freed.
 *
 *  The HeapMemMP module uses a NameServer instance to
 *  store instance information when an instance is created.  The name supplied
 *  must be unique for all HeapMemMP instances.
 *
 *  The #HeapMemMP_create call initializes the shared memory as needed. Once an
 *  instance is created, an #HeapMemMP_open can be performed. The
 *  open is used to gain access to the same HeapMemMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor(s).
 *
 *  The open returns a HeapMemMP instance handle like the create,
 *  however the open does not modify the shared memory.
 *
 *  Because HeapMemMP is a variable-size heap implementation, it is also used
 *  by the SharedRegion module to manage shared memory in each shared
 *  region.  When any shared memory IPC instance is created in a
 *  particular shared region, the HeapMemMP that manages shared memory in the
 *  shared region allocates shared memory for the instance.
 *
 *  ============================================================================
 */


#ifndef ti_ipc_HeapMemMP__include
#define ti_ipc_HeapMemMP__include

#if defined (__cplusplus)
extern "C" {
#endif

#include <ti/ipc/GateMP.h>

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
 *  @def    HeapMemMP_S_BUSY
 *  @brief  The resource is still in use
 */
#define HeapMemMP_S_BUSY               2

/*!
 *  @def    HeapMemMP_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define HeapMemMP_S_ALREADYSETUP       1

/*!
 *  @def    HeapMemMP_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define HeapMemMP_S_SUCCESS            0

/*!
 *  @def    HeapMemMP_E_FAIL
 *  @brief  Generic failure.
 */
#define HeapMemMP_E_FAIL              -1

/*!
 *  @def    HeapMemMP_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define HeapMemMP_E_INVALIDARG          -2

/*!
 *  @def    HeapMemMP_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define HeapMemMP_E_MEMORY              -3

/*!
 *  @def    HeapMemMP_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define HeapMemMP_E_ALREADYEXISTS       -4

/*!
 *  @def    HeapMemMP_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define HeapMemMP_E_NOTFOUND            -5

/*!
 *  @def    HeapMemMP_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define HeapMemMP_E_TIMEOUT             -6

/*!
 *  @def    HeapMemMP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define HeapMemMP_E_INVALIDSTATE        -7

/*!
 *  @def    HeapMemMP_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call  */
#define HeapMemMP_E_OSFAILURE           -8

/*!
 *  @def    HeapMemMP_E_RESOURCE
 *  @brief  Specified resource is not available  */
#define HeapMemMP_E_RESOURCE            -9

/*!
 *  @def    HeapMemMP_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation  */
#define HeapMemMP_E_RESTART             -10


/* =============================================================================
 * Macros
 * =============================================================================
 */


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/*!
 *  @brief  HeapMemMP_Handle type
 */
typedef struct HeapMemMP_Object *HeapMemMP_Handle;

/*!
 *  @brief  Structure defining parameters for the HeapMemMP module.
 */
typedef struct HeapMemMP_Params {
    String name;
    /*!< Name of this instance.
     *
     *  The name (if not NULL) must be unique among all HeapMemMP
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
     *  heap belonging to the region identified by
     *  #HeapMemMP_Params::regionId.
     */
    /*! @endcond */

    SizeT sharedBufSize;
    /*!< Size of shared buffer
     *
     *  This is the size of the buffer to be used in the HeapMemMP instance.
     *  The actual buffer size in the created instance might actually be less
     *  than the value supplied in 'sharedBufSize' because of alignment
     *  constraints.
     *
     *  It is important to note that the total amount of shared memory required
     *  for a HeapMemMP instance will be greater than the size supplied here.
     *  Additional space will be consumed by shared instance attributes and
     *  alignment-related padding.
     */

    GateMP_Handle gate;
    /*!< GateMP used for critical region management of the shared memory
     *
     *  Using the default value of NULL will result in use of the GateMP
     *  system gate for context protection.
     */

} HeapMemMP_Params;

/*!
 *  @brief  Stats structure for the HeapMemMP_getExtendedStats API.
 */
typedef struct HeapMemMP_ExtendedStats {
    Ptr   buf;
    /*!< Local address of the shared buffer */

    SizeT size;
    /*!< Size of the shared buffer */
} HeapMemMP_ExtendedStats;

/* =============================================================================
 *  HeapMemMP Module-wide Functions
 * =============================================================================
 */
/*!
 *  @brief      Close a HeapMemMP instance
 *
 *  Closing an instance will free local memory consumed by the opened
 *  instance. All opened instances should be closed before the instance
 *  is deleted.
 *
 *  @param[in,out]  handlePtr   Pointer to handle returned from #HeapMemMP_open
 */
Int HeapMemMP_close(HeapMemMP_Handle *handlePtr);

/*!
 *  @brief      Create a HeapMemMP instance
 *
 *  @param[in]  params      HeapMemMP parameters
 *
 *  @return     HeapMemMP Handle
 */
HeapMemMP_Handle HeapMemMP_create(const HeapMemMP_Params *params);

/*!
 *  @brief      Delete a created HeapMemMP instance
 *
 *  @param[in,out]      handlePtr   Pointer to handle to delete.
 */
Int HeapMemMP_delete(HeapMemMP_Handle *handlePtr);

/*!
 *  @brief      Open a created HeapMemMP instance
 *
 *  Once an instance is created, an open can be performed. The
 *  open is used to gain access to the same HeapMemMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor.
 *
 *  The open returns a HeapMemMP instance handle like the create,
 *  however the open does not initialize the shared memory. The supplied
 *  name is used to identify the created instance.
 *
 *  Call #HeapMemMP_close when the opened instance is not longer needed.
 *
 *  @param[in]  name        Name of created HeapMemMP instance
 *  @param[out] handlePtr   Pointer to HeapMemMP handle to be opened
 *
 *  @return     HeapMemMP status:
 *              - #HeapMemMP_S_SUCCESS: Heap successfully opened
 *              - #HeapMemMP_E_FAIL: Heap is not yet ready to be opened.
 *
 *  @sa         HeapMemMP_close
 */
Int HeapMemMP_open(String name, HeapMemMP_Handle *handlePtr);

/*! @cond */
Int HeapMemMP_openByAddr(Ptr sharedAddr, HeapMemMP_Handle *handlePtr);
/*! @endcond */

/*!
 *  @brief      Initialize a HeapMemMP parameters struct
 *
 *  @param[out] params      Pointer to GateMP parameters
 *
 */
Void HeapMemMP_Params_init(HeapMemMP_Params *params);

/*! @cond */
/*!
 *  @brief      Amount of shared memory required for creation of each instance
 *
 *  @param[in]  params      Pointer to the parameters that will be used in
 *                          the create.
 *
 *  @return     Number of MAUs needed to create the instance.
 */
SizeT HeapMemMP_sharedMemReq(const HeapMemMP_Params *params);
/*! @endcond */

/* =============================================================================
 *  HeapMemMP Per-instance functions
 * =============================================================================
 */

/*!
 *  @brief      Allocate a block of memory of specified size and alignment
 *
 *  The actual block returned may be larger than requested to satisfy
 *  alignment requirements.
 *
 *  HeapMemMP_alloc will lock the heap using the HeapMemMP gate
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
 *  @param[in]  handle    Handle to previously created/opened instance.
 *  @param[in]  size      Size to be allocated (in bytes)
 *  @param[in]  align     Alignment for allocation (power of 2)
 *
 *  @sa         HeapMemMP_free
 */
Void *HeapMemMP_alloc(HeapMemMP_Handle handle, SizeT size,
                           SizeT align);

/*!
 *  @brief      Frees a block of memory.
 *
 *  free() places the memory block specified by addr and size back into the
 *  free pool of the heap specified. The newly freed block is combined with
 *  any adjacent free blocks. The space is then available for further
 *  allocation by alloc().
 *
 *  #HeapMemMP_free will lock the heap using the HeapMemMP gate if one is
 *  specified or the system GateMP if not.
 *
 *  @param[in]  handle    Handle to previously created/opened instance.
 *  @param[in]  block     Block of memory to be freed.
 *  @param[in]  size      Size to be freed (in bytes)
 *
 *  @sa         HeapMemMP_alloc
 */
Void HeapMemMP_free(HeapMemMP_Handle handle, Ptr block, SizeT size);

/*!
 *  @brief      Get extended memory statistics
 *
 *  This function retrieves extended statistics for a HeapMemMP
 *  instance. Refer to #::ExtendedStats for more information
 *  regarding what information is returned.
 *
 *  @param[in]  handle    Handle to previously created/opened instance.
 *  @param[out] stats     ExtendedStats structure
 *
 *  @sa
 */
Void HeapMemMP_getExtendedStats(HeapMemMP_Handle handle,
    HeapMemMP_ExtendedStats *stats);

/*!
 *  @brief      Get memory statistics
 *
 *  @param[in]  handle    Handle to previously created/opened instance.
 *  @param[out] stats     Memory statistics structure
 *
 *  @sa
 */
Void HeapMemMP_getStats(HeapMemMP_Handle handle, Ptr stats);

/*!
 *  @brief      Restore an instance to it's original created state.
 *
 *  This function restores an instance to
 *  its original created state. Any memory previously allocated from the
 *  heap is no longer valid after this API is called. This function
 *  does not check whether there is allocated memory or not.
 *
 *  @param[in]  handle    Handle to previously created/opened instance.
 */
Void HeapMemMP_restore(HeapMemMP_Handle handle);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_HeapMemMP__include */
