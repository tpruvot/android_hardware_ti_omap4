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
/*============================================================================
 *  @file   Heap.c
 *
 *  @brief      Defines Heap based memory allocator.
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
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @sa         Heap_free
 */
Void *
Heap_alloc (Heap_Handle   hpHandle,
            UInt32        size,
            UInt32        align)
{
    Char           * block  = NULL;
    Heap_Object    * obj    = NULL;

    GT_3trace (curTrace, GT_ENTER, "Heap_alloc", hpHandle, size, align);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (hpHandle == NULL) {
        /*! @retval NULL Invalid NULL hpHandle pointer specified */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_alloc",
                             HEAP_E_INVALIDARG,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) hpHandle;

        GT_assert (curTrace, (obj->alloc != NULL));
        block = obj->alloc (hpHandle,
                            size,
                            align);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_alloc", block);

    /*! @retval Valid heap block Operation Successful */
    return block;
}


/*!
 *  @brief      Frees a block of memory.
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         Heap_alloc
 */
Int
Heap_free (Heap_Handle   hpHandle,
           Ptr           block,
           UInt32        size)
{
    Int              status = HEAP_SUCCESS;
    Heap_Object    * obj    = NULL;

    GT_3trace (curTrace, GT_ENTER, "Heap_free", hpHandle, block, size);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (hpHandle == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL hpHandle pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_free",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) hpHandle;

        GT_assert (curTrace, (obj->free != NULL));
        status = obj->free (hpHandle,
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
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @params     stats     Memory statistics structure
 *
 *  @sa         Heap_getExtendedStats
 */
Int
Heap_getStats (Heap_Handle     hpHandle,
               Memory_Stats  * stats)
{
    Int              status = HEAP_SUCCESS;
    Heap_Object    * obj    = NULL;

    GT_2trace (curTrace, GT_ENTER, "Heap_getStats", hpHandle, stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (hpHandle == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL hpHandle pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getStats",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
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

        obj   = (Heap_Object *) hpHandle;

        GT_assert (curTrace, (obj->getStats != NULL));
        status = obj->getStats (hpHandle,
                                stats);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_getStats", status);

    /*! @retval HEAP_SUCCESS Operation Successful */
    return status;
}


/*!
 *  @brief      Get extended statistics
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @params     stats     Heap statistics Structure
 *
 *  @sa         Heap_getStats
 *
 */
Int
Heap_getExtendedStats (Heap_Handle          hpHandle,
                       Heap_ExtendedStats * stats)
{
    Int              status = HEAP_SUCCESS;
    Heap_Object    * obj    = NULL;

    GT_2trace (curTrace, GT_ENTER, "Heap_getExtendedStats", hpHandle, stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (hpHandle == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL hpHandle pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getExtendedStats",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (stats == NULL) {
        /*! @retval HEAP_E_INVALIDARG Invalid NULL stats pointer specified */
        status = HEAP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getExtendedStats",
                             status,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj   = (Heap_Object *) hpHandle;

        GT_assert (curTrace, (obj->getExtendedStats != NULL));
        status =  obj->getExtendedStats (hpHandle,
                                         stats);


#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Heap_getExtendedStats", status);

    /*! @retval HEAP_SUCCESS Operation Successful */
    return status;
}


/*!
 *  @brief Function to get the kernel object pointer embedded in userspace heap.
 *         Some Heap implementations return the kernel object handle.
 *         Heaps which do not have kernel object pointer embedded return NULL.
 *
 *  @params hpHandle handle to a heap instance
 *
 *  @sa
 */
Void *
Heap_getKnlHandle (Heap_Handle hpHandle)
{
    Heap_Object * heapObject = (Heap_Object *) hpHandle;
    Ptr           knlHandle = NULL;

    GT_1trace (curTrace, GT_ENTER, "Heap_getKnlHandle", hpHandle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Heap_getKnlHandle",
                             HEAP_E_INVALIDARG,
                             "Handle passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
       if (heapObject->getKnlHandle != NULL) {
           knlHandle = heapObject->getKnlHandle (hpHandle);
       }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "Heap_getKnlHandle");

    return (knlHandle);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
