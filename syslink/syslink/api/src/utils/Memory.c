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
 *  @file   Memory.c
 *
 *  @brief      Linux kernel Memory interface implementation.
 *
 *              This abstracts the Memory management interface in the kernel
 *              code. Allocation, Freeing-up, copy and address translate are
 *              supported for the kernel memory management.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <MemoryOS.h>
#include <IHeap.h>
#include <Trace.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    Memory_MODULEID
 *  @brief  Module ID for Memory OSAL module.
 */
#define Memory_MODULEID                 (UInt16) 0xC97E

/* =============================================================================
 *  All success and failure codes for the module. These are used only in the
 *  implementation, but are not part of the interface.
 * =============================================================================
 */
/*!
* @def   Memory_STATUSCODEBASE
* @brief Stauts code base for MEMORY module.
*/
#define Memory_STATUSCODEBASE           (Memory_MODULEID << 12u)

/*!
* @def   Memory_MAKE_FAILURE
* @brief Convert to failure code.
*/
#define Memory_MAKE_FAILURE(x)          ((Int) (0x80000000  \
                                         + (Memory_STATUSCODEBASE + (x))))
/*!
* @def   Memory_MAKE_SUCCESS
* @brief Convert to success code.
*/
#define Memory_MAKE_SUCCESS(x)          (Memory_STATUSCODEBASE + (x))

/*!
* @def   Memory_E_MEMORY
* @brief Indicates Memory alloc/free failure.
*/
#define Memory_E_MEMORY                 Memory_MAKE_FAILURE(1)

/*!
* @def   Memory_E_INVALIDARG
* @brief Invalid argument provided
*/
#define Memory_E_INVALIDARG             Memory_MAKE_FAILURE(2)

/*!
* @def   Memory_S_SUCCESS
* @brief Operation successfully completed
*/
#define Memory_S_SUCCESS                Memory_MAKE_SUCCESS(0)


/* Allocates the specified number of bytes. */
Ptr
Memory_alloc (IHeap_Handle heap, SizeT size, SizeT align)
{
    Ptr buffer = NULL;

    GT_3trace (curTrace, GT_ENTER, "Memory_alloc", heap, size, align);

    /* check whether the right paramaters are passed or not.*/
    GT_assert (curTrace, (size > 0));

    if (heap == NULL) {
        /* Call the OS API for memory allocation */
        buffer = MemoryOS_alloc (size, align, 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_alloc",
                                 Memory_E_MEMORY,
                                 "Failed to allocate memory!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        /* if align == 0, use default alignment */
        if (align == 0) {
            align = Memory_getMaxDefaultTypeAlign ();
        }

        buffer = IHeap_alloc (heap, size, align);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Heap_alloc failed */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_alloc",
                                 Memory_E_MEMORY,
                                 "IHeap_alloc failed!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "Memory_alloc", buffer);

    return buffer;
}


/* Allocates the specified number of bytes and memory is set to zero. */
Ptr
Memory_calloc (IHeap_Handle heap, SizeT size, SizeT align)
{
    Ptr buffer = NULL;

    GT_3trace (curTrace, GT_ENTER, "Memory_calloc", heap, size, align);

    /* Check whether the right paramaters are passed or not.*/
    GT_assert (curTrace, (size > 0));

    if (heap == NULL) {
        buffer = MemoryOS_calloc (size, align, 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Failed to allocate memory */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_calloc",
                                 Memory_E_MEMORY,
                                 "Failed to allocate memory!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        buffer = Memory_valloc (heap, size, align, 0u);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Memory_valloc failed */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_calloc",
                                 Memory_E_MEMORY,
                                 "Memory_valloc failed!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_0trace (curTrace, GT_LEAVE, "Memory_calloc");

    /*! @retval Pointer Success: Pointer to allocated buffer */
    return buffer;
}


/* Frees up the specified chunk of memory. */
Void
Memory_free (IHeap_Handle heap, Ptr block, SizeT size)
{
    GT_3trace (curTrace, GT_ENTER, "Memory_free", heap, block, size);

    GT_assert (curTrace, (block != NULL));
    GT_assert (curTrace, (size > 0));

    if (heap == NULL) {
        MemoryOS_free (block, size, 0);
    }
    else {
        IHeap_free (heap, block, size);
    }

    GT_0trace (curTrace, GT_LEAVE, "Memory_free");
}


/* Function to obtain statistics from a heap. */
Void
Memory_getStats (IHeap_Handle heap, Memory_Stats * stats)
{
    GT_2trace (curTrace, GT_ENTER, "Memory_getStats", heap, stats);

    GT_assert (curTrace, (heap != NULL));
    GT_assert (curTrace, (stats!= 0));

    /* Nothing to be done if heap is NULL. */
    if (heap != NULL) {
        IHeap_getStats (heap, stats);
    }

    GT_0trace (curTrace, GT_LEAVE, "Memory_getStats");
}


/* Function to test for a particular IHeap quality */
Bool
Memory_query (IHeap_Handle heap, Int qual)
{
    Bool flag = FALSE;

    GT_2trace (curTrace, GT_ENTER, "Memory_query", heap, qual);

    GT_assert (curTrace, (heap != NULL));

    switch (qual) {
        case Memory_Q_BLOCKING:
            if (heap != NULL) {
                flag = IHeap_isBlocking (heap);
            }
            else {
                /* Assume blocking alloc for OS-specific alloc. */
                flag = TRUE;
            }
            break;

        default:
            break;
    }

    GT_1trace (curTrace, GT_LEAVE, "Memory_query", flag);

    return flag;
}


/* Function to return the largest alignment required by the target */
UInt32
Memory_getMaxDefaultTypeAlign (Void)
{

    /*
     * Alignment has been hard coded to 4
     * This has been done to ensure that if
     * shared region is used for structures that
     * it aligns for bus width for integers
     */

    return 4;
}


/* Function to allocate the specified number of bytes and memory is set to
 * the specified value.
 */
Ptr
Memory_valloc (IHeap_Handle heap, SizeT size, SizeT align, Char value)
{
    Ptr buffer = NULL;

    GT_3trace (curTrace, GT_ENTER, "Memory_valloc", heap, size, align);

    /* Check whether the right paramaters are passed or not.*/
    GT_assert (curTrace, (size > 0));

    if (heap == NULL) {
        buffer = MemoryOS_alloc (size, align, 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Failed to allocate memory */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_valloc",
                                 Memory_E_MEMORY,
                                 "Failed to allocate memory!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        buffer = IHeap_alloc (heap, size, align);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Heap_alloc failed */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_valloc",
                                 Memory_E_MEMORY,
                                 "IHeap_alloc failed!");
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (buffer != NULL) {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        buffer = Memory_set (buffer, value, size);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (buffer == NULL) {
            /*! @retval NULL Memory_set to 0 failed */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Memory_valloc",
                                 Memory_E_MEMORY,
                                 "Memory_set to 0 failed!");
        }
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "Memory_valloc");

    /*! @retval Pointer Success: Pointer to allocated buffer */
    return buffer;
}


/* Function to translate an address. */
Ptr
Memory_translate (Ptr srcAddr, Memory_XltFlags flags)
{
    Ptr buf = NULL;

    GT_2trace (curTrace, GT_ENTER, "Memory_tranlate", srcAddr, flags);

    buf = MemoryOS_translate (srcAddr, flags);

    GT_1trace (curTrace, GT_LEAVE, "Memory_tranlate", buf);

    return buf;
}


#if defined (__cplusplus)
}
#endif /* defined (_cplusplus)*/
