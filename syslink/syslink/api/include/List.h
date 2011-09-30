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
 *  @file   List.h
 *
 *  @brief      Creates a doubly linked list. It works as utils for other
 *              modules
 *
 *  ============================================================================
 */


#ifndef LIST_H_0XB734
#define LIST_H_0XB734


/* Standard headers */
#include <IGateProvider.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @def    List_traverse
 *  @brief  Traverse the full list till the last element.
 */
#define List_traverse(x,y) for(x = (List_Elem *)((List_Object *)y)->elem.next; \
                               (UInt32) x != (UInt32)&((List_Object *)y)->elem;\
                                x = x->next)

/*!
 *  @brief  Structure defining object for the list element.
 */
typedef struct List_Elem_tag {
    struct List_Elem_tag *  next; /*!< Pointer to the next element */
    struct List_Elem_tag *  prev; /*!< Pointer to the previous element */
} List_Elem;

/*!
 *  @brief  Structure defining object for the list.
 */
typedef struct List_Object_tag {
    List_Elem            elem;
    /*!< Head pointing to next element */
    IGateProvider_Handle gateHandle;
    /*!< Handle to lock for protecting objList */
} List_Object;

/*! @brief Defines List handle type */
typedef List_Object * List_Handle;

/*!
 *  @brief  Structure defining params for the list.
 */
typedef struct List_Params_tag {
    IGateProvider_Handle gateHandle;
    /*!< Handle to lock for protecting objList. If not to be provided, set to
     *   IGateProvider_NULL. In that case, internal protection shall be done
     *   with #Gate_enterSystem/#Gate_leaveSystem for protected APIs.
     */
} List_Params;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Initialize this config-params structure with supplier-specified
 *              defaults before instance creation.
 *
 *  @param      params  Instance config-params structure.
 *
 *  @sa         List_create
 */
Void List_Params_init (List_Params * params);

/*!
 *  @brief      Function to create a list object.
 *
 *  @param      params  Pointer to list creation parameters. If NULL is passed,
 *                      default parameters are used.
 *
 *  @retval     List-handle Handle to the list object
 *
 *  @sa         List_delete
 */
List_Handle List_create (List_Params * params);

/*!
 *  @brief      Function to delete a list object.
 *
 *  @param      handlePtr  Pointer to the list handle
 *
 *  @sa         List_delete
 */
Void List_delete (List_Handle * handlePtr);

/*!
 *  @brief      Function to construct a list object. This function is used when
 *              memory for the list object is not to be allocated by the List
 *              API.
 *
 *  @param      obj     Pointer to the list object to be constructed
 *  @param      params  Pointer to list construction parameters. If NULL is
 *                      passed, default parameters are used.
 *
 *  @sa         List_destruct
 */
Void List_construct (List_Object * obj, List_Params * params);

/*!
 *  @brief      Function to destruct a list object.
 *
 *  @param      obj  Pointer to the list object to be destructed
 *
 *  @sa         List_construct
 */
Void List_destruct (List_Object * obj);

/*!
 *  @brief      Function to clear element contents.
 *
 *  @param      elem Element to be cleared
 *
 *  @sa
 */
Void List_elemClear (List_Elem * elem);

/*!
 *  @brief      Function to check if list is empty.
 *
 *  @param      handle  Pointer to the list
 *
 *  @retval     TRUE    List is empty
 *  @retval     FALSE   List is not empty
 *
 *  @sa
 */
Bool List_empty (List_Handle handle);

/*!
 *  @brief      Function to get first element of List.
 *
 *  @param      handle  Pointer to the list
 *
 *  @retval     NULL          Operation failed
 *  @retval     Valid-pointer Pointer to first element
 *
 *  @sa         List_put
 */
Ptr List_get (List_Handle handle);

/*!
 *  @brief      Function to insert element at the end of List.
 *
 *  @param      handle  Pointer to the list
 *  @param      elem    Element to be inserted
 *
 *  @sa         List_get
 */
Void List_put (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Function to traverse to the next element in the list (non
 *              atomic)
 *
 *  @param      handle  Pointer to the list
 *  @param      elem    Pointer to the current element
 *
 *  @retval     NULL          Operation failed
 *  @retval     Valid-pointer Pointer to next element
 *
 *  @sa         List_prev
 */
Ptr List_next (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Function to traverse to the previous element in the list (non
 *              atomic)
 *
 *  @param      handle  Pointer to the list
 *  @param      elem    Pointer to the current element
 *
 *  @retval     NULL          Operation failed
 *  @retval     Valid-pointer Pointer to previous element
 *
 *  @sa         List_next
 */
Ptr List_prev (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Function to insert element before the existing element
 *              in the list.
 *
 *  @param      handle  Pointer to the list
 *  @param      newElem Element to be inserted
 *  @param      curElem Existing element before which new one is to be inserted
 *
 *  @sa         List_remove
 */
Void List_insert (List_Handle handle, List_Elem * newElem, List_Elem * curElem);

/*!
 *  @brief      Function to removes element from the List.
 *
 *  @param      handle    Pointer to the list
 *  @param      elem      Element to be removed
 *
 *  @sa         List_insert
 */
Void List_remove (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Function to put the element before head.
 *
 *  @param      handle    Pointer to the list
 *  @param      elem      Element to be added at the head
 *
 *  @sa         List_put
 */
Void List_putHead (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Get element from front of List (non-atomic)
 *
 *  @param      handle  Pointer to the list
 *
 *  @retval     NULL          Operation failed
 *  @retval     Valid-pointer Pointer to removed element
 *
 *  @sa         List_enqueue, List_enqueueHead
 */
Ptr List_dequeue (List_Handle handle);

/*!
 *  @brief      Put element at end of List (non-atomic)
 *
 *  @param      handle  Pointer to the list
 *  @param      elem    Element to be put
 *
 *  @sa         List_dequeue
 */
Void List_enqueue (List_Handle handle, List_Elem * elem);

/*!
 *  @brief      Put element at head of List (non-atomic)
 *
 *  @param      handle   Pointer to the list
 *  @param      elem     Element to be added
 *
 *  @sa         List_dequeue
 */
Void List_enqueueHead (List_Handle handle, List_Elem * elem);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* LIST_H_0XB734 */
