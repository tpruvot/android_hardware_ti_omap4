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
/*==============================================================================
 *  @file   Heap.c
 *
 *  @brief      Defines Heap based memory allocator.
 *
 *  Heap implementation that manages fixed size buffers that can be used
 *  in a multiprocessor system with shared memory.
 *
 *  The Heap manager provides functions to allocate and free storage from a
 *  heap of type Heap which inherits from IHeap. Heap manages a single
 *  fixed-size buffer, split into equally sized allocable blocks.
 *
 *  The Heap manager is intended as a very fast memory
 *  manager which can only allocate blocks of a single size. It is ideal for
 *  managing a heap that is only used for allocating a single type of object,
 *  or for objects that have very similar sizes.
 *
 *  This module is instance based. Each instance requires shared memory
 *  (for the buffers and other internal state).  This is specified via the
 *  #sharedAddr parameter to the create. The proper
 *  #sharedAddrSize parameter can be determined via the
 *  #sharedMemReq call. Note: the parameters to this
 *  function must be the same that will used to create the instance.
 *
 *  The Heap module uses a ti.sdo.utils.NameServer instance to
 *  store instance information when an instance is created and the name
 *  parameter is non-NULL. If a name is supplied, it must be unique for all
 *  Heap instances.
 *
 *  The #create initializes the shared memory as needed. The shared
 *  memory must be initialized to 0 before the Heap instance is created or
 *  opened.
 *
 *  Once an instance is created, an #open can be performed. The
 *  open is used to gain access to the same Heap instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor(s).
 *
 *  The open returns a Heap instance handle like the create,
 *  however the open does not modify the shared memory.
 *
 *  There are two options when opening the instance:
 *  @p(blist)
 *  -Supply the same name as specified in the create. The Heap module
 *  queries the NameServer to get the needed information.
 *  -Supply the same #sharedAddr value as specified in the create.
 *  @p
 *
 *  If the open is called before the instance is created, open returns NULL.
 *
 *  Constraints:
 *  @p(blist)
 *  -Align parameter must be a power of 2.
 *  -The buffer passed to dynamically create a Heap must be aligned
 *   according to the alignment parameter, and must be large enough to account
 *   for the actual block size after it has been rounded up to a multiple of
 *   the alignment.
 *
 *  ============================================================================
 */



/* Standard headers */
#include <Std.h>

/* Osal And Utils  headers */
#include <Trace.h>

/* Module level headers */
#include <Heap.h>


#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @brief      Allocate a block of memory of specified size.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @sa         Heap_free
 */
Void *
Heap_alloc (Heap_Handle   handle,
            UInt32        size,
            UInt32        align)
{
    Char           * block  = NULL;
    Heap_Object    * obj    = NULL;

    GT_3trace (curTrace, GT_ENTER, "Heap_alloc", handle, size, align);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Invalid NULL handle pointer specified */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_alloc",
                             HEAP_E_INVALIDARG,
                             "Invalid NULL handle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *)handle;

        GT_assert (curTrace, (obj->alloc != NULL));
        block = obj->alloc (handle,
                            size,
                            align);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_alloc", block);

    /*! @retval Valid-heap-block Operation Successful */
    return block;
}


/*!
 *  @brief      Frees a block of memory.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         Heap_alloc
 */
Int
Heap_free (Heap_Handle   handle,
           Ptr           block,
           UInt32        size)
{
    Int              status = HEAP_SUCCESS;
    Heap_Object    * obj    = NULL;

    GT_3trace (curTrace, GT_ENTER, "Heap_free", handle, block, size);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL handle pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_free",
                             status,
                             "Invalid NULL handle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) handle;

        GT_assert (curTrace, (obj->free != NULL));
        status = obj->free (handle,
                            block,
                            size);


#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_free", status);

    /*! @retval HEAP_SUCCESS Operation Successful */
    return status;
}


/*!
 *  @brief      Get memory statistics
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @params     stats     Memory statistics structure
 *
 *  @sa
 */
Int
Heap_getStats (Heap_Handle     handle,
               Memory_Stats  * stats)
{
    Int              status = HEAP_SUCCESS;
    Heap_Object    * obj    = NULL;

    GT_2trace (curTrace, GT_ENTER, "Heap_getStats", handle, stats);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL handle pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getStats",
                             status,
                             "Invalid NULL handle pointer specified!");
    }
    else if (stats == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL stats pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getStats",
                             status,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) handle;

        GT_assert (curTrace, (obj->getStats != NULL));
        status = obj->getStats (handle, stats);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_getStats", status);

    /*! @retval HEAP_SUCCESS Operation Successful */
    return status;
}


/*!
 *  @brief      Indicate whether the heap may block during an alloc or free call
 *
 *  @param      handle    Handle to previously created/opened instance.
 */
/*  */
Bool
Heap_isBlocking (Heap_Handle handle)
{
    Bool          isBlocking = FALSE;
    Heap_Object * obj        = NULL;

    GT_1trace (curTrace, GT_ENTER, "Heap_isBlocking", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_isBlocking",
                             HEAP_E_INVALIDARG,
                             "Invalid NULL handle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) handle;

        GT_assert (curTrace, (obj->isBlocking != NULL));
        isBlocking = obj->isBlocking (handle);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_isBlocking", isBlocking);

    /*! @retval TRUE  Heap blocks during alloc/free calls */
    /*! @retval FALSE Heap does not block during alloc/free calls */
    return isBlocking;
}


/*!
 *  @brief Function to get the kernel object pointer embedded in userspace heap.
 *         Some Heap implementations return the kernel object handle.
 *         Heaps which do not have kernel object pointer embedded return NULL.
 *
 *  @params handle handle to a heap instance
 *
 *  @sa
 */
Void *
Heap_getKnlHandle (Heap_Handle handle)
{
    Heap_Object * heapObject = (Heap_Object *) handle;
    Ptr           knlHandle = NULL;

    GT_1trace (curTrace, GT_ENTER, "Heap_getKnlHandle", handle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Handle passed is invalid */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getKnlHandle",
                             HEAP_E_INVALIDARG,
                             "Handle passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
       if (heapObject->getKnlHandle != NULL) {
           knlHandle = heapObject->getKnlHandle (handle);
       }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "Heap_getKnlHandle");

    /*! @retval Kernel-object-handle Operation successfully completed. */
    return (knlHandle);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
