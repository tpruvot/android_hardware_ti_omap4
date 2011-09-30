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
 *  @file   List.c
 *
 *  @brief      Creates a doubly linked list.
 *
 *  ============================================================================
 */

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <List.h>
#include <Memory.h>
#include <Trace.h>

/* Module level headers */

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  All success and failure codes for the module. Defined here because they are
 *  only used internally.
 * =============================================================================
 */

/*!
 *  @def    List_S_BUSY
 *  @brief  The resource is still in use
 */
#define List_S_BUSY           2

/*!
 *  @def    List_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define List_S_ALREADYSETUP   1

/*!
 *  @def    List_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define List_S_SUCCESS        0

/*!
 *  @def    List_E_FAIL
 *  @brief  Generic failure.
 */
#define List_E_FAIL           -1

/*!
 *  @def    List_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define List_E_INVALIDARG     -2

/*!
 *  @def    List_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define List_E_MEMORY         -3

/*!
 *  @def    List_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define List_E_ALREADYEXISTS  -4

/*!
 *  @def    List_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define List_E_NOTFOUND       -5

/*!
 *  @def    List_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define List_E_TIMEOUT        -6

/*!
 *  @def    List_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define List_E_INVALIDSTATE   -7

/*!
 *  @def    List_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define List_E_OSFAILURE      -8

/*!
 *  @def    List_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define List_E_RESOURCE       -9

/*!
 *  @def    List_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define List_E_RESTART        -10


/*!
 *  @brief      Initialize this config-params structure with supplier-specified
 *              defaults before instance creation.
 *
 *  @param      params  Instance config-params structure.
 *
 *  @sa         List_create
 */
Void
List_Params_init (List_Params * params)
{
    GT_1trace (curTrace, GT_ENTER, "List_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (params == NULL) {
        /* No retVal since this is a Void function. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_Params_init",
                             List_E_INVALIDARG,
                             "Argument of type (List_Params *) is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        params->gateHandle = IGateProvider_NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_Params_init");
}


/*!
 *  @brief      Function to create a list object.
 *
 *  @param      params  Pointer to list creation parameters. If NULL is passed,
 *                      default parameters are used.
 *
 *  @sa         List_delete
 */
List_Handle
List_create (List_Params * params)
{
    List_Object * obj = NULL;

    GT_1trace (curTrace, GT_ENTER, "List_create", params);

    (void) params;

    /* heapHandle can be NULL if created from default OS memory. */
    obj = (List_Object *) Memory_alloc (NULL,
                                        sizeof (List_Object),
                                        0);


    GT_assert (curTrace, (obj != NULL));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (obj == NULL) {
        /*! @retval NULL Allocate memory for handle failed */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_create",
                             List_E_MEMORY,
                             "Allocating memory for handle failed");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj->elem.next  = obj->elem.prev = &(obj->elem);
        obj->gateHandle = params->gateHandle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_create", obj);

    /*! @retval valid-handle Operation successful */
    return (List_Handle) obj;
}


/*!
 *  @brief      Function to delete a list object.
 *
 *  @param      handle  Pointer to the list handle
 *
 *  @sa         List_delete
 */
Void
List_delete (List_Handle * handlePtr)
{
    List_Object * obj;

    GT_1trace (curTrace, GT_ENTER, "List_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_delete",
                             List_E_INVALIDARG,
                             "handlePtr passed is invalid!");
    }
    else if (*handlePtr == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_delete",
                             List_E_INVALIDARG,
                             "*handlePtr passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (List_Object *) (*handlePtr);
        Memory_free (NULL, obj, sizeof (List_Object));
        *handlePtr = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_delete");
}


/*!
 *  @brief      Function to construct a list object.
 *
 *  @param      obj     Pointer to the list object to be constructed
 *  @param      params  Pointer to list construction parameters. If NULL is
 *                      passed, default parameters are used.
 *
 *  @sa         List_destruct
 */
Void
List_construct (List_Object * obj, List_Params * params)
{
    GT_1trace (curTrace, GT_ENTER, "List_construct", obj);

    GT_assert (curTrace, (obj != NULL));
    /* params may be provided as NULL. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (obj == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_construct",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for obj parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj->elem.next = obj->elem.prev = &(obj->elem);
        if (params != NULL) {
            obj->gateHandle = params->gateHandle;
            GT_assert (curTrace, (obj->gateHandle != NULL));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (obj->gateHandle == NULL) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "List_construct",
                                     List_E_INVALIDARG,
                                     "Invalid NULL passed for "
                                     "obj->gateHandle parameter");
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
        else {
            obj->gateHandle = IGateProvider_NULL;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_construct");
}


/*!
 *  @brief      Function to destruct a list object.
 *
 *  @param      obj  Pointer to the list object to be destructed
 */
Void
List_destruct (List_Object * obj)
{
    IArg key;

    GT_1trace (curTrace, GT_ENTER, "List_destruct", obj);

    GT_assert (curTrace, (obj != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (obj == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_destruct",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for obj parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = IGateProvider_enter (obj->gateHandle);
        obj->elem.next = NULL;
        obj->elem.prev = NULL;
        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_destruct");
}


/*!
 *  @brief     Function to clear element contents.
 *
 *  @param     elem Element to be cleared
 */
Void
List_elemClear (List_Elem * elem)
{
    GT_1trace (curTrace, GT_ENTER, "List_elemClear", elem);

    GT_assert (curTrace, (elem != NULL));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_elemClear",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
         elem->next = elem->prev = elem;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_elemClear");
}


/*!
 *  @brief     Function to check if list is empty.
 *
 *  @param     handle  Pointer to the list
 */
Bool
List_empty (List_Handle handle)
{
    Bool          isEmpty = FALSE;
    List_Object * obj     = (List_Object *) handle;
    IArg          key     = 0;

    GT_1trace (curTrace, GT_ENTER, "List_empty", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_empty",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));
        key = IGateProvider_enter (obj->gateHandle);

        if (obj->elem.next == &(obj->elem)) {
            /*! @retval TRUE List is empty */
            isEmpty = TRUE;
        }

        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_empty", isEmpty);

    /*! @retval FALSE List is not empty */
    return isEmpty;
}


/*!
 *  @brief     Function to get first element of List.
 *
 *  @param     handle  Pointer to the list
 */
Ptr
List_get (List_Handle handle)
{
    List_Elem *   elem = NULL;
    List_Object * obj  = (List_Object *) handle;
    IArg          key  = 0;

    GT_1trace (curTrace, GT_ENTER, "List_get", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Invalid NULL passed for handle parameter */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_get",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));
        key = IGateProvider_enter (obj->gateHandle);

        elem = List_dequeue(obj);

        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_get", elem);

    /*! @retval Valid-pointer Pointer to first element */
    return elem ;
}


/*!
 *  @brief     Function to insert element at the end of List.
 *
 *  @param     handle  Pointer to the list
 *  @param     element Element to be inserted
 */
Void
List_put (List_Handle handle, List_Elem * elem)
{
    List_Object * obj    = (List_Object *) handle;
    IArg          key    = 0;

    GT_2trace (curTrace, GT_ENTER, "List_put", handle, elem);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_put",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_put",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));
        key = IGateProvider_enter (obj->gateHandle);

        List_enqueue (obj, elem);

        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_put");
}


/*!
 *  @brief     Function to traverse to the next element in the list (non
 *             atomic)
 *
 *  @param     handle  Pointer to the list
 *  @param     elem    Pointer to the current element
 */
Ptr
List_next (List_Handle handle, List_Elem * elem)
{
    List_Elem *   retElem = NULL;
    List_Object * obj     = (List_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "List_Next", handle, elem);

    GT_assert (curTrace, (handle != NULL)) ;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Invalid NULL passed for handle parameter */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_next",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* elem == NULL -> start at the head */
        if (elem == NULL) {
            retElem = obj->elem.next;
        }
        else {
            retElem = elem->next;
        }

        if (retElem == (List_Elem *) obj) {
            /*! @retval NULL List reached end or list is empty */
            retElem = NULL;
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_Next", retElem);

    /*! @retval Valid-pointer Pointer to the next element */
    return retElem;
}


/*!
 *  @brief     Function to traverse to the previous element in the list (non
 *             atomic)
 *
 *  @param     handle  Pointer to the list
 *  @param     elem    Pointer to the current element
 *
 *  @sa
 */
Ptr
List_prev (List_Handle handle, List_Elem * elem)
{
    List_Elem *   retElem = NULL;
    List_Object * obj     = (List_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "List_prev", handle, elem);

    GT_assert (curTrace, (handle != NULL)) ;
    GT_assert (curTrace, (elem != NULL)) ;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Invalid NULL passed for handle parameter */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_prev",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        /*! @retval NULL Invalid NULL passed for elem parameter */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_prev",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* elem == NULL -> start at the head */
        if (elem == NULL) {
            retElem = obj->elem.prev;
        }
        else {
            retElem = elem->prev;
        }

        if (retElem == (List_Elem *)obj) {
            /*! @retval NULL List reached end or list is empty */
            retElem = NULL;
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_prev", retElem);

    /*! @retval Valid-pointer Pointer to the prev element */
    return retElem;
}


/*!
 *  @brief      Function to insert element before the existing element
 *              in the list.
 *
 *  @param     handle  Pointer to the list
 *  @param     newElem Element to be inserted
 *  @param     curElem Existing element before which new one is to be inserted
 */
Void
List_insert  (List_Handle handle, List_Elem * newElem, List_Elem * curElem)
{
    List_Object * obj = (List_Object *) handle;
    IArg          key;

    GT_3trace (curTrace, GT_ENTER, "List_insert", handle, newElem, curElem);

    GT_assert (curTrace, (handle     != NULL));
    GT_assert (curTrace, (newElem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (obj == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_insert",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (newElem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_insert",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for newElem parameter");
    }
    else if (curElem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_insert",
                             List_E_INVALIDARG,
                  "Invalid NULL passed for curElem parameter use List_putHead");
    }else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            GT_assert (curTrace, (obj->gateHandle != NULL));

            /* Protect with gate if provided. */
            key = IGateProvider_enter (obj->gateHandle);

            /* Cannot directly call enqueue since the object has other fields */
            newElem->next       = curElem;
            newElem->prev       = curElem->prev;
            newElem->prev->next = newElem;
            curElem->prev       = newElem;

            IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_insert");
}


/*!
 *  @brief      Function to removes element from the List.
 *
 *  @param     handle    Pointer to the list
 *  @param     element   Element to be removed
 */
Void
List_remove (List_Handle handle, List_Elem * elem)
{
    List_Object * obj    = (List_Object *) handle;
    IArg          key;

    GT_2trace (curTrace, GT_ENTER, "List_remove", handle, elem);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_remove",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_remove",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));

        /* Protect with gate if provided. */
        key = IGateProvider_enter (obj->gateHandle);

        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;

        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_remove");
}


/*!
 *  @brief     Function to put the element before head.
 *
 *  @param     handle    Pointer to the list
 *  @param     element   Element to be removed
 */
Void
List_putHead (List_Handle handle, List_Elem *elem)
{
    List_Object * obj = (List_Object *) handle;
    IArg          key = 0;

    GT_2trace (curTrace, GT_ENTER, "List_putHead", handle, elem);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_putHead",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_putHead",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));
        key = IGateProvider_enter (obj->gateHandle);

        List_enqueueHead (handle, elem);

        IGateProvider_leave (obj->gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_putHead");
}


/*!
 *  @brief     Get element from front of List (non-atomic)
 *
 *  @param     handle  Pointer to the list
 */
Ptr
List_dequeue (List_Handle handle)
{
    List_Elem *   elem = NULL;
    List_Object * obj  = (List_Object *) handle;

    GT_1trace (curTrace, GT_ENTER, "List_dequeue", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval NULL Invalid NULL passed for handle parameter */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_get",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));

        elem = obj->elem.next;
        /* See if the List was empty */
        if (elem == (List_Elem *)obj) {
            /*! @retval NULL List is empty */
            elem = NULL;
        }
        else {
            obj->elem.next   = elem->next;
            elem->next->prev = &(obj->elem);
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "List_dequeue", elem);

    /*! @retval Valid-pointer Pointer to first element */
    return elem ;
}


/*!
 *  @brief     Put element at end of List (non-atomic)
 *
 *  @param     handle  Pointer to the list
 *  @param     elem    Element to be put
 */
Void
List_enqueue (List_Handle handle, List_Elem * elem)
{
    List_Object * obj    = (List_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "List_enqueue", handle, elem);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_enqueue",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_enqueue",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));

        elem->next           = &(obj->elem);
        elem->prev           = obj->elem.prev;
        obj->elem.prev->next = elem;
        obj->elem.prev       = elem;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_enqueue");
}


/*!
 *  @brief     Put element at head of List (non-atomic)
 *
 *  @param     handle   Pointer to the list
 *  @param     elem     Element to be added
 */
Void
List_enqueueHead (List_Handle handle, List_Elem * elem)
{
    List_Object * obj    = (List_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "List_enqueueHead", handle, elem);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_enqueueHead",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for handle parameter");
    }
    else if (elem == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "List_enqueueHead",
                             List_E_INVALIDARG,
                             "Invalid NULL passed for elem parameter");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        GT_assert (curTrace, (obj->gateHandle != NULL));
        /* Protect with gate if provided. */

        elem->next           = obj->elem.next;
        elem->prev           = &(obj->elem);
        obj->elem.next->prev = elem;
        obj->elem.next       = elem;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "List_enqueueHead");
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
