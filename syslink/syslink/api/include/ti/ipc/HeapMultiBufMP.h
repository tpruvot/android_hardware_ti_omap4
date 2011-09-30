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
 *  @file       HeapMultiBufMP.h
 *
 *  @brief      Multiple fixed size buffer heap implementation.
 *
 *  The HeapMultiBufMP manager provides functions to allocate and free storage
 *  from a shared memory heap of type HeapMultiBufMP which inherits from IHeap.
 *  HeapMultiBufMP manages multiple fixed-size memory buffers. Each buffer
 *  contains a fixed number of allocatable memory 'blocks' of the same size.
 *  HeapMultiBufMP is intended as a fast and deterministic memory manager which
 *  can service requests for blocks of arbitrary size.
 *
 *  An example HeapMultiBufMP instance might have sixteen 32-byte blocks in one
 *  buffer, and four 128-byte blocks in another buffer. A request for memory
 *  will be serviced by the smallest possible block, so a request for 100
 *  bytes would receive a 128-byte block in our example.
 *
 *  Allocating from HeapMultiBufMP will try to return a block from the first
 *  buffer which has:
 *
 *    1. A block size that is >= to the requested size
 *
 *    AND
 *
 *    2. An alignment that is >= to the requested alignment
 *
 *  Buffer configuration for a new instance is primarily supplied via the
 *  #HeapMultiBufMP_Params::bucketEntries instance configuration parameter.
 *  Once buckets are adjusted for size and alignment, buffers with equal sizes
 *  and alignments are combined.
 *
 *  Block Sizes and Alignment
 *      - A buffer with a requested alignment of 0 will receive the target-
 *        specific minimum alignment
 *      - If cache is enabled for the particular shared region in which the
 *        shared buffer is to be placed, then the minimum alignment for each
 *        buffer is equal to the cache line size of the shared region.
 *      - The actual block sizes will be a multiple of the alignment. For
 *        example, if a buffer is configured to have 56-byte blocks with an
 *        alignment of 32, HeapMultiBufMP will actually create 64-byte blocks.
 *        When providing the buffer for a dynamice create, make sure it is
 *        large enough to take this into account.
 *      - Multiple buffers with the same block size ARE allowed. This may
 *        occur, for example, if sizeof is used to specify the block sizes.
 *      - If any buffers have both the same block size and alignment, they
 *        will be merged. If this is a problem, consider managing these buffers
 *        directly with HeapBufMP objects.
 *
 *  In addition to the buffer configuration, a #HeapMultiBufMP_Params::name
 *  and a #HeapMultiBufMP_Params::regionId (from which shared memory is
 *  allocated) must be supplied when creating an instance.
 *
 *  Once an instance is created, an #HeapMultiBufMP_open can be performed using
 *  the name that was supplied in the #HeapMultiBufMP_create. The open is used
 *  to gain access to the same HeapMultiBufMP instance.  Generally an instance
 *  is created on one processor and opened on the other processor(s). The open
 *  returns (by reference) a HeapMultiBufMP instance handle like the create,
 *  however the open does not modify the shared memory.
 *
 *  #HeapMultiBufMP_open will return HeapMultiBufMP_E_FAIL if called before
 *  the instance is created.
 *
 *  ============================================================================
 */


#ifndef ti_ipc_HeapMultiBufMP__include
#define ti_ipc_HeapMultiBufMP__include

#if defined (__cplusplus)
extern "C" {
#endif

#include <ti/ipc/GateMP.h>

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
 *  @def    HeapMultiBufMP_S_BUSY
 *  @brief  The resource is still in use
 */
#define HeapMultiBufMP_S_BUSY               2

/*!
 *  @def    HeapMultiBufMP_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define HeapMultiBufMP_S_ALREADYSETUP       1

/*!
 *  @def    Notify_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define HeapMultiBufMP_S_SUCCESS            0

/*!
 *  @def    Notify_E_FAIL
 *  @brief  Generic failure.
 */
#define HeapMultiBufMP_E_FAIL              -1

/*!
 *  @def    HeapMultiBufMP_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define HeapMultiBufMP_E_INVALIDARG          -2

/*!
 *  @def    HeapMultiBufMP_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define HeapMultiBufMP_E_MEMORY              -3

/*!
 *  @def    HeapMultiBufMP_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define HeapMultiBufMP_E_ALREADYEXISTS       -4

/*!
 *  @def    HeapMultiBufMP_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define HeapMultiBufMP_E_NOTFOUND            -5

/*!
 *  @def    HeapMultiBufMP_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define HeapMultiBufMP_E_TIMEOUT             -6

/*!
 *  @def    HeapMultiBufMP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define HeapMultiBufMP_E_INVALIDSTATE        -7

/*!
 *  @def    HeapMultiBufMP_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call  */
#define HeapMultiBufMP_E_OSFAILURE           -8

/*!
 *  @def    HeapMultiBufMP_E_RESOURCE
 *  @brief  Specified resource is not available  */
#define HeapMultiBufMP_E_RESOURCE            -9

/*!
 *  @def    HeapMultiBufMP_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation  */
#define HeapMultiBufMP_E_RESTART             -10



/* =============================================================================
 * Macros
 * =============================================================================
 */


/*!
 *  @def    HeapMultiBufMP_MAXBUCKETS
 *  @brief  Maximum number of buffer buckets supported.
 */
#define HeapMultiBufMP_MAXBUCKETS           ((UInt)8)

/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/*!
 *  @brief  HeapMultiBufMP_Handle type
 */
typedef struct HeapMultiBufMP_Object *HeapMultiBufMP_Handle;

/*!
 *  @brief   Structure for bucket configuration
 *
 *  An array of buckets is a required parameter to create any
 *  HeapMultiBufMP instance.  The fields of each bucket correspond
 *  to the attributes of each buffer in the HeapMultiBufMP.  The actual
 *  block sizes and alignments may be adjusted per the process described
 *  at #HeapMultiBufMP_Params::bucketEntries.
 */
typedef struct HeapMultiBufMP_Bucket {
    SizeT blockSize;    /*!< Size of each block in this bucket (in MADUs)   */
    UInt numBlocks;     /*!< Number of blocks of this size                  */
    SizeT align;        /*!< Alignment of each block (in MADUs)             */
} HeapMultiBufMP_Bucket;

/*!
 *  @brief  Structure defining parameters for the HeapMultiBufMP module.
 */
typedef struct HeapMultiBufMP_Params {
    GateMP_Handle gate;
    /*!< GateMP used for critical region management of the shared memory
     *
     *  Using the default value of NULL will result in use of the GateMP
     *  system gate for context protection.
     */

    Bool exact;
    /*!< Use exact matching
     *
     *  Setting this flag will allow allocation only if the requested size
     *  is equal to (rather than less than or equal to) a buffer's block size.
     */

    String name;
    /*!< Name of this instance.
     *
     *  The name (if not NULL) must be unique among all HeapMultiBufMP
     *  instances in the entire system.  When creating a new
     *  heap, it is necessary to supply an instance name.
     */

    Int numBuckets;
    /*!< Number of buckets in #HeapMultiBufMP_Params::bucketEntries
     *
     *  This parameter is required to create any instance.
     */

    HeapMultiBufMP_Bucket *bucketEntries;
    /*!< Bucket Entries
     *
     *  The bucket entries are an array of #HeapMultiBufMP_Bucket whose values
     *  correspond to the desired alignment, block size and length for each
     *  buffer.  It is important to note that the alignments and sizes for each
     *  buffer may be adjusted due to cache and alignment related constraints.
     *  Buffer sizes are rounded up by their corresponding alignments.  Buffer
     *  alignments themselves will assume the value of region cache alignment
     *  size when the cache size is greater than the requested buffer alignment.
     *
     *  For example, specifying a bucket with {blockSize: 192, align: 256} will
     *  result in a buffer of blockSize = 256 and alignment = 256.  If cache
     *  alignment is required, then a bucket of {blockSize: 96, align: 64} will
     *  result in a buffer of blockSize = 128 and alignment = 128 (assuming
     *  cacheSize = 128).
     */

    UInt16 regionId;
    /*!<Shared region ID
     *
     *  The index corresponding to the shared region from which shared memory
     *  will be allocated.
     */

    /*! @cond */
    Ptr sharedAddr;
    /*!<Physical address of the shared memory
     *
     *  This value can be left as 'null' unless it is required to place the
     *  heap at a specific location in shared memory.  If sharedAddr is null,
     *  then shared memory for a new instance will be allocated from the
     *  heap belonging to the region identified by #HeapMultiBufMP_Params::regionId.
     */
    /*! @endcond */

} HeapMultiBufMP_Params;

/*!
 *  @brief  Stats structure for the #HeapMultiBufMP_getExtendedStats API.
 */
typedef struct HeapMultiBufMP_ExtendedStats {
    UInt numBuckets;
    /*!<Number of buffers after optimization */

    UInt numBlocks          [HeapMultiBufMP_MAXBUCKETS];
    /*!<Number of blocks in each buffer */

    UInt blockSize          [HeapMultiBufMP_MAXBUCKETS];
    /*!<Block size of each buffer */

    UInt align              [HeapMultiBufMP_MAXBUCKETS];
    /*!<Alignment of each buffer */

    UInt maxAllocatedBlocks [HeapMultiBufMP_MAXBUCKETS];
    /*!<The maximum number of blocks allocated from this heap at any single
     * point in time during the lifetime of this HeapMultiBufMP instance
     */

    UInt numAllocatedBlocks [HeapMultiBufMP_MAXBUCKETS];
    /*!<The total number of blocks currently allocated in this HeapMultiBufMP
     * instance
     */

} HeapMultiBufMP_ExtendedStats;

/* =============================================================================
 *  HeapMultiBufMP Module-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Close a HeapMultiBufMP instance
 *
 *  Closing an instance will free local memory consumed by the opened
 *  instance. All opened instances should be closed before the instance
 *  is deleted.
 *
 *  @param      handlePtr      Pointer to handle returned from
 *                              #HeapMultiBufMP_open
 *
 *  @sa         HeapMultiBufMP_open
 */
Int HeapMultiBufMP_close(HeapMultiBufMP_Handle *handlePtr);

/*!
 *  @brief      Create a HeapMultiBufMP instance
 *
 *  @param      params      HeapMultiBufMP parameters
 *
 *  @return     HeapMultiBufMP Handle
 */
HeapMultiBufMP_Handle HeapMultiBufMP_create(const HeapMultiBufMP_Params *params);

/*!
 *  @brief      Delete a created HeapMultiBufMP instance
 *
 *  @param      handlePtr   Pointer to handle to delete.
 */
Int HeapMultiBufMP_delete(HeapMultiBufMP_Handle *handlePtr);

/*!
 *  @brief      Open a created HeapMultiBufMP instance
 *
 *  Once an instance is created, an open can be performed. The
 *  open is used to gain access to the same HeapMultiBufMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor.
 *
 *  The open returns a HeapMultiBufMP instance handle like the create,
 *  however the open does not initialize the shared memory. The supplied
 *  name is used to identify the created instance.
 *
 *  Call #HeapMultiBufMP_close when the opened instance is not longer needed.
 *
 *  @param      name        Name of created HeapMultiBufMP instance
 *  @param      handlePtr   Pointer to HeapMultiBufMP handle to be opened
 *
 *  @return     HeapMultiBufMP status
 *              - #HeapMultiBufMP_S_SUCCESS: Heap successfully opened
 *              - #HeapMultiBufMP_E_FAIL: Heap is not yet ready to be opened.
 *
 *  @sa         HeapMultiBufMP_close
 */
Int HeapMultiBufMP_open(String name, HeapMultiBufMP_Handle *handlePtr);

/*! @cond */
Int HeapMultiBufMP_openByAddr(Ptr sharedAddr, HeapMultiBufMP_Handle *handlePtr);
/*! @endcond */

/*!
 *  @brief      Initialize a HeapMultiBufMP parameters struct
 *
 *  @param[out] params      Pointer to GateMP parameters
 *
 */
Void HeapMultiBufMP_Params_init(HeapMultiBufMP_Params *params);

/*! @cond */
/*!
 *  @brief      Amount of shared memory required for creation of each instance
 *
 *  @param[in]  params      Pointer to the parameters that will be used in
 *                          the create.
 *
 *  @return     Number of MAUs needed to create the instance.
 */
SizeT HeapMultiBufMP_sharedMemReq(const HeapMultiBufMP_Params *params);
/*! @endcond */


/* =============================================================================
 *  HeapMultiBufMP Per-instance functions
 * =============================================================================
 */

/*!
 *  @brief      Allocate a block of memory of specified size and alignment
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @sa         HeapMultiBufMP_free
 */
Void *HeapMultiBufMP_alloc(HeapMultiBufMP_Handle handle, SizeT size,
                           SizeT align);

/*!
 *  @brief      Frees a block of memory.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         HeapMultiBufMP_alloc
 */
Void HeapMultiBufMP_free(HeapMultiBufMP_Handle handle, Ptr block, SizeT size);

/*!
 *  @brief      Get extended memory statistics
 *
 *  This function retrieves the extended statistics for a HeapMultiBufMP
 *  instance.  It does not retrieve the standard Memory_Stats
 *  information.  Refer to #HeapMultiBufMP_ExtendedStats for more information
 *  regarding what information is returned.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param[out] stats     ExtendedStats structure
 *
 *  @sa
 */
Void HeapMultiBufMP_getExtendedStats(HeapMultiBufMP_Handle handle,
    HeapMultiBufMP_ExtendedStats *stats);

/*!
 *  @brief      Get memory statistics
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param[out] stats     Memory statistics structure
 *
 *  @sa
 */
Void HeapMultiBufMP_getStats(HeapMultiBufMP_Handle handle, Ptr stats);

/* Module functions */

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_HeapMultiBufMP__include */
