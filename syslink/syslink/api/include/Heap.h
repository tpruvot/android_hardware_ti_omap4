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
 *  @file   Heap.h
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
 *  ============================================================================
 */


#ifndef HEAP_H_0x7033
#define HEAP_H_0x7033


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    HEAP_MODULEID
 *  @brief  Unique module ID.
 */
#define HEAP_MODULEID               (0x7033)

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
 *  @def    HEAP_STATUSCODEBASE
 *  @brief  Error code base for Heap.
 */
#define HEAP_STATUSCODEBASE  (HEAP_MODULEID << 12u)

/*!
 *  @def    HEAP_MAKE_FAILURE
 *  @brief  Macro to make error code.
 */
#define HEAP_MAKE_FAILURE(x)    ((Int)  (  0x80000000                  \
                                         + (HEAP_STATUSCODEBASE  \
                                         + (x))))

/*!
 *  @def    HEAP_MAKE_SUCCESS
 *  @brief  Macro to make success code.
 */
#define HEAP_MAKE_SUCCESS(x)    (HEAP_STATUSCODEBASE + (x))

/*!
 *  @def    HEAP_E_INVALIDARG
 *  @brief  Argument passed to a function is invalid.
 */
#define HEAP_E_INVALIDARG       HEAP_MAKE_FAILURE(1)

/*!
 *  @def    HEAP_E_MEMORY
 *  @brief  Memory allocation failed.
 */
#define HEAP_E_MEMORY           HEAP_MAKE_FAILURE(2)

/*!
 *  @def    HEAP_E_BUSY
 *  @brief  The name is already registered or not.
 */
#define HEAP_E_BUSY             HEAP_MAKE_FAILURE(3)

/*!
 *  @def    HEAP_E_FAIL
 *  @brief  Generic failure.
 */
#define HEAP_E_FAIL             HEAP_MAKE_FAILURE(4)

/*!
 *  @def    HEAP_E_NOTFOUND
 *  @brief  Name not found in the nameserver.
 */
#define HEAP_E_NOTFOUND         HEAP_MAKE_FAILURE(5)

/*!
 *  @def    HEAP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define HEAP_E_INVALIDSTATE     HEAP_MAKE_FAILURE(6)

/*!
 *  @def    HEAP_E_NOTONWER
 *  @brief  Instance is not created on this processor.
 */
#define HEAP_E_NOTONWER         HEAP_MAKE_FAILURE(7)

/*!
 *  @def    HEAP_E_REMOTEACTIVE
 *  @brief  Remote opener of the instance has not closed the instance.
 */
#define HEAP_E_REMOTEACTIVE     HEAP_MAKE_FAILURE(8)

/*!
 *  @def    HEAP_E_INUSE
 *  @brief  Indicates that the instance is in use..
 */
#define HEAP_E_INUSE            HEAP_MAKE_FAILURE(9)

/*!
 *  @def    HEAP_E_INVALIDBUFALIGN
 *  @brief  Indicates that the buffer is not properly aligned.
 */
#define     HEAP_E_INVALIDBUFALIGN     HEAP_MAKE_FAILURE(10)

/*!
 *  @def    HEAP_E_INVALIDALIGN
 *  @brief  Indicates that the buffer alignment parameter is not a
 *          power of 2.
 */
#define HEAP_E_INVALIDALIGN     HEAP_MAKE_FAILURE(11)

/*!
 *  @def    HEAP_E_INVALIDBUFSIZE
 *  @brief  Indicates that the buffer size is invalid (too small)
 */
#define HEAP_E_INVALIDBUFSIZE   HEAP_MAKE_FAILURE(12)

/*!
 *  @def    HEAP_E_OSFAILURE
 *  @brief  Failure in OS call.
 */
#define HEAP_E_OSFAILURE        HEAP_MAKE_FAILURE(13)

/*!
 *  @def    HEAP_SUCCESS
 *  @brief  Operation successful.
 */
#define HEAP_SUCCESS            HEAP_MAKE_SUCCESS(0)

/*!
 *  @def    HEAP_S_ALREADYSETUP
 *  @brief  The HEAP module has already been setup in this process.
 */
#define HEAP_S_ALREADYSETUP     HEAP_MAKE_SUCCESS(1)


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining memory related statistics
 */
typedef struct Memory_Stats_tag {
    UInt32 totalSize;
    /*!< Total memory size */
    UInt32 totalFreeSize;
    /*!< Total free memory size */
    UInt32 largestFreeSize;
    /*!< Largest free memory size */
} Memory_Stats;


/* =============================================================================
 *  Forward declarations
 * =============================================================================
 */
/*! @brief Forward declaration of structure defining object for the
 *         Heap module
 */
typedef struct Heap_Object_tag Heap_Object;

/*!
 *  @brief  Handle for the Heap Buf.
 */
typedef struct Heap_Object * Heap_Handle;


/* =============================================================================
 *  Function pointer types for heap operations
 * =============================================================================
 */
/*! @brief Type for function pointer to allocate a memory block */
typedef Void* (*Heap_allocFxn) (Heap_Handle    handle,
                                UInt32         size,
                                UInt32         align);

/*! @brief Type for function pointer to free a memory block */
typedef Int (*Heap_freeFxn) (Heap_Handle handle,
                             Ptr         block,
                             UInt32      size);

/*! @brief Type for function pointer to get memory related statistics */
typedef Int (*Heap_getStatsFxn) (Heap_Handle     handle,
                                 Memory_Stats  * stats);

/*
 * ! @brief Type for function pointer to indicate whether the heap may block
 *          during an alloc or free call
 */
typedef Bool (*Heap_isBlockingFxn) (Heap_Handle handle);

/*! @brief Type for function pointer to get handle to kernel object */
typedef Void * (*Heap_getKnlHandleFxn) (Heap_Handle handle);


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure for the Handle for the Heap.
 */
struct Heap_Object_tag {
    Heap_allocFxn               alloc;
    /*!<  Allocate a block */
    Heap_freeFxn                free;
    /*!<  Free a block */
    Heap_getStatsFxn            getStats;
    /*!<  Get statistics */
    Heap_isBlockingFxn          isBlocking;
    /*!<  Does the Heap block during alloc/free? */
    Heap_getKnlHandleFxn        getKnlHandle;
    /*!<  Get kernel object handle */
    Ptr                         obj;
    /*!<  Actual Heap Handle */
};


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Allocate a block. */
Void * Heap_alloc (Heap_Handle handle,
                   UInt32       size,
                   UInt32       align);

/* Frees the block to this Heap. */
Int Heap_free (Heap_Handle handle,
               Ptr         block,
               UInt32      size);

/* Get memory statistics */
Int Heap_getStats (Heap_Handle       handle,
                   Memory_Stats    * stats);

/* Indicate whether the heap may block during an alloc or free call */
Bool Heap_isBlocking (Heap_Handle handle);

/* Function to return kernel object handle */
Void * Heap_getKnlHandle (Heap_Handle handle);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* HEAP_H_0x7033 */
