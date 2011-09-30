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
 *  @file   IHeap.h
 *
 *  @brief      Defines Heap based memory allocator.
 *
 *              Heap implementation that manages fixed size buffers that can be
 *              used in a multiprocessor system with shared memory.
 *
 *              The Heap manager provides functions to allocate and free storage
 *              from a heap of type Heap which inherits from IHeap. Heap manages
 *              a single fixed-size buffer, split into equally sized allocable
 *              blocks.
 *
 *              The Heap manager is intended as a very fast memory
 *              manager which can only allocate blocks of a single size. It is
 *              ideal for managing a heap that is only used for allocating a
 *              single type of object, or for objects that have very similar
 *              sizes.
 *
 *              This module is instance based. Each instance requires shared
 *              memory (for the buffers and other internal state).  This is
 *              specified via the sharedAddr parameter to the create. The proper
 *              sharedAddrSize parameter can be determined via the
 *              sharedMemReq call. Note: the parameters to this
 *              function must be the same that will used to create the instance.
 *
 *              The Heap module uses a NameServer instance to
 *              store instance information when an instance is created and the
 *              name parameter is non-NULL. If a name is supplied, it must be
 *              unique for all Heap instances.
 *
 *              The create initializes the shared memory as needed. The shared
 *              memory must be initialized to 0 before the Heap instance is
 *              created or opened.
 *
 *              Once an instance is created, an open can be performed. The
 *              open is used to gain access to the same Heap instance.
 *              Generally an instance is created on one processor and opened
 *              on the other processor(s).
 *
 *              The open returns a Heap instance handle like the create,
 *              however the open does not modify the shared memory.
 *
 *              There are two options when opening the instance:
 *              -Supply the same name as specified in the create. The Heap
 *              module queries the NameServer to get the needed information.
 *              -Supply the same sharedAddr value as specified in the create.
 *
 *              If the open is called before the instance is created, open
 *              returns NULL.
 *
 *              Constraints:
 *              -Align parameter must be a power of 2.
 *              -The buffer passed to dynamically create a Heap must be aligned
 *               according to the alignment parameter, and must be large enough
 *               to account for the actual block size after it has been rounded
 *               up to a multiple of the alignment.
 *
 *  ============================================================================
 */


#ifndef HEAP_H_0x7033
#define HEAP_H_0x7033


/* OSAL and utils */
#include <MemoryDefs.h>
#include <Trace.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Forward declarations
 * =============================================================================
 */
/*! @brief Forward declaration of structure defining object for the
 *         Heap module
 */
typedef struct IHeap_Object_tag IHeap_Object;

/*!
 *  @brief  Handle for the Heap Buf.
 */
typedef struct IHeap_Object * IHeap_Handle;


/* =============================================================================
 *  Function pointer types for heap operations
 * =============================================================================
 */
/*! @brief Type for function pointer to allocate a memory block */
typedef Ptr (*IHeap_allocFxn) (IHeap_Handle    handle,
                               SizeT           size,
                               SizeT           align);

/*! @brief Type for function pointer to free a memory block */
typedef Void (*IHeap_freeFxn) (IHeap_Handle handle,
                               Ptr          block,
                               SizeT        size);

/*! @brief Type for function pointer to get memory related statistics */
typedef Void (*IHeap_getStatsFxn) (IHeap_Handle    handle,
                                   Memory_Stats  * stats);

/*
 * ! @brief Type for function pointer to indicate whether the heap may block
 *          during an alloc or free call
 */
typedef Bool (*IHeap_isBlockingFxn) (IHeap_Handle handle);

/*! @brief Type for function pointer to get handle to kernel object */
typedef Ptr (*IHeap_getKnlHandleFxn) (IHeap_Handle handle);


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure for the Handle for the Heap.
 */
struct IHeap_Object_tag {
    IHeap_allocFxn               alloc;
    /*!<  Allocate a block */
    IHeap_freeFxn                free;
    /*!<  Free a block */
    IHeap_getStatsFxn            getStats;
    /*!<  Get statistics */
    IHeap_isBlockingFxn          isBlocking;
    /*!<  Does the Heap block during alloc/free? */
    IHeap_getKnlHandleFxn        getKnlHandle;
    /*!<  Get kernel object handle */
    Ptr                          obj;
    /*!<  Actual Heap Handle */
};


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Allocate a block of memory of specified size.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @retval     buffer    Allocated buffer
 *
 *  @sa         IHeap_free
 */
static inline Ptr IHeap_alloc (IHeap_Handle    handle,
                               SizeT           size,
                               SizeT           align)
{
    Ptr buffer;

    GT_assert (curTrace, (((IHeap_Object *) handle)->alloc != NULL));
    buffer = ((IHeap_Object *) handle)->alloc (handle, size, align);
    return (buffer);
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
static inline Void IHeap_free (IHeap_Handle handle,
                               Ptr          block,
                               SizeT        size)
{
    GT_assert (curTrace, (((IHeap_Object *) handle)->free != NULL));
    ((IHeap_Object *) handle)->free (handle, block, size);
}


/*!
 *  @brief      Get memory statistics
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @params     stats     Memory statistics structure
 *
 *  @sa
 */
static inline Void IHeap_getStats (IHeap_Handle    handle,
                                   Memory_Stats  * stats)
{
    GT_assert (curTrace, (((IHeap_Object *) handle)->getStats != NULL));
    ((IHeap_Object *) handle)->getStats (handle, stats);
}


/*!
 *  @brief      Indicate whether the heap may block during an alloc or free call
 *
 *  @param      handle    Handle to previously created/opened instance.
 *
 *  @retval     TRUE      Heap is blocking
 *  @retval     FALSE     Heap is non-blocking
 *
 *  @sa
 */
static inline Bool IHeap_isBlocking (IHeap_Handle    handle)
{
    Bool isBlocking;
    GT_assert (curTrace, (((IHeap_Object *) handle)->isBlocking != NULL));
    isBlocking = ((IHeap_Object *) handle)->isBlocking (handle);
    return (isBlocking);
}


/*!
 *  @brief Function to get the kernel object pointer embedded in userspace heap.
 *         Some Heap implementations return the kernel object handle.
 *         Heaps which do not have kernel object pointer embedded return NULL.
 *
 *  @params handle handle to a heap instance
 *
 *  @retval     handle    Handle to kernel object.
 *
 *  @sa
 */
static inline Ptr IHeap_getKnlHandle (IHeap_Handle    handle)
{
    Ptr knlHandle;
    GT_assert (curTrace, (((IHeap_Object *) handle)->getKnlHandle != NULL));
    knlHandle = ((IHeap_Object *) handle)->getKnlHandle (handle);
    return (knlHandle);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* HEAP_H_0x7033 */
