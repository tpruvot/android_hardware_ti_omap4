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
/** ===========================================================================
 *  @file       ListMP.h
 *
 *  @brief      ListMP shared memory module.
 *
 *  ListMP is a doubly linked-list based module designed to be used
 *  in a multi-processor environment.  It provides a way for
 *  multiple processors to create, access, and manipulate the same link
 *  list in shared memory.
 *
 *  The ListMP module uses a NameServer instance
 *  to store information about an instance during create.  Each
 *  ListMP instance is created by specifying a name and region id.
 *  The name supplied must be unique for all ListMP instances in the system.
 *
 *  ListMP_create() is used to create an instance of a ListMP.
 *  Shared memory is modified during create.  Once created, an instance
 *  may be opened by calling ListMP_open().  Open does not
 *  modify any shared memory.  Open() should be called only when global
 *  interrupts are enabled.
 *
 *  To use a ListMP instance, a #ListMP_Elem must be embedded
 *  as the very first element of a struture.   ListMP does not provide
 *  cache coherency for the buffer put onto the link list.
 *  ListMP only provides cache coherency for the #ListMP_Elem
 *  fields.  The buffer should be written back before being placed
 *  on a ListMP, if cache coherency is required.
 *
 *  ============================================================================
 */

#ifndef ti_ipc_ListMP__include
#define ti_ipc_ListMP__include

#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/GateMP.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    ListMP_S_BUSY
 *  @brief  The resource is still in use
 */
#define ListMP_S_BUSY           2

/*!
 *  @def    ListMP_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define ListMP_S_ALREADYSETUP   1

/*!
 *  @def    ListMP_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define ListMP_S_SUCCESS        0

/*!
 *  @def    ListMP_E_FAIL
 *  @brief  Generic failure.
 */
#define ListMP_E_FAIL           -1

/*!
 *  @def    ListMP_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define ListMP_E_INVALIDARG     -2

/*!
 *  @def    ListMP_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define ListMP_E_MEMORY         -3

/*!
 *  @def    ListMP_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define ListMP_E_ALREADYEXISTS  -4

/*!
 *  @def    ListMP_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define ListMP_E_NOTFOUND       -5

/*!
 *  @def    ListMP_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define ListMP_E_TIMEOUT        -6

/*!
 *  @def    ListMP_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define ListMP_E_INVALIDSTATE   -7

/*!
 *  @def    ListMP_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define ListMP_E_OSFAILURE      -8

/*!
 *  @def    ListMP_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define ListMP_E_RESOURCE       -9

/*!
 *  @def    ListMP_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define ListMP_E_RESTART        -10

/*!
 *  @brief      ListMP_Handle type
 */
typedef struct ListMP_Object *ListMP_Handle;    /* opaque */

/*!
 *  @brief  Structure defining a ListMP element.
 */
typedef struct ListMP_Elem {
    /*! SharedRegion pointer to next element */
    volatile SharedRegion_SRPtr next;

    /*! SharedRegion pointer to previous element */
    volatile SharedRegion_SRPtr prev;

} ListMP_Elem;

/*!
 *  @brief  Structure defining parameter structure for ListMP_create().
 */
typedef struct ListMP_Params {
    /*!
     *  GateMP instance for critical region management of shared memory
     *
     *  Using the default value of NULL will result in use of the GateMP
     *  system gate for context protection.
     */
    GateMP_Handle gate;

    /*! @cond
     *  Physical address of the shared memory
     *
     *  The shared memory that will be used for maintaining shared state
     *  information.  This is an optional parameter to create.  If value
     *  is null, then the shared memory for the new instance will be
     *  allocated from the heap in the specified region Id.
     */
    Ptr sharedAddr;
    /*! @endcond */

    /*!
     *  Name of the instance
     *
     *  The name must be unique among all ListMP instances in the system.
     *  When creating using name and  region Id, the name must not be
     *  null.
     */
    String name;

    /*!
     *  SharedRegion ID.
     *
     *  The ID corresponding to the index of the shared region in which this
     *  shared instance is to be placed.  This is used in create() only when
     *  name is not null.
     */
    UInt16 regionId;
} ListMP_Params;

/* =============================================================================
 *  ListMP Module-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Initializes ListMP parameters.
 *
 *  @param      params  Instance param structure.
 */
Void ListMP_Params_init(ListMP_Params *params);

/*!
 *  @brief      Creates and initializes ListMP module.
 *
 *  @param      params  Instance param structure.
 *
 *  @return     a ListMP instance handle
 *
 *  @sa         ListMP_delete
 */
ListMP_Handle ListMP_create(const ListMP_Params *params);

/*!
 *  @brief      Close an opened ListMP instance
 *
 *  Closing an instance will free local memory consumed by the opened
 *  instance.  Instances that are opened should be closed before the
 *  instance is deleted.  If using NameServer,
 *  the instance name will be removed from the NameServer instance.
 *
 *  @param      handlePtr  Pointer to a ListMP instance
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  ListMP successfully closed
 *              - #ListMP_E_FAIL:  ListMP close failed
 *
 *  @sa         ListMP_open
 */
Int ListMP_close(ListMP_Handle *handlePtr);

/*!
 *  @brief      Deletes a ListMP instance.
 *
 *  @param      handlePtr  Pointer to ListMP instance
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  ListMP successfully deleted
 *              - #ListMP_E_FAIL:  ListMP delete failed
 *
 *  @sa         ListMP_create
 */
Int ListMP_delete(ListMP_Handle *handlePtr);

/*!
 *  @brief      Open a created ListMP instance
 *
 *  An open can be performed on a previously created instance.
 *  Open is used to gain access to the same ListMP instance.
 *  Generally an instance is created on one processor and opened on
 *  other processors but it can be opened on the same processor too.
 *  Open returns a ListMP instance handle like create, but it does
 *  not initialize any shared memory.
 *
 *  The open call searches the local ListMP NameServer table first
 *  for a matching name. If no local match is found, it will search all
 *  remote ListMP NameServer tables for a matching name.
 *
 *  A status value of #ListMP_S_SUCCESS is returned if a matching
 *  ListMP instance is found.  A #ListMP_E_FAIL is returned if no
 *  matching instance is found.  Generally this means the ListMP instance
 *  has not yet been created.  A more specific status error is returned if
 *  an error was raised.
 *
 *  Call close() when the opened instance is no longer needed.
 *
 *  @param      name        Name of created ListMP instance
 *  @param      handlePtr   pointer to the handle if a handle was found.
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  ListMP successfully opened
 *              - #ListMP_E_FAIL:  ListMP is not ready to be opened
 *
 *  @sa         ListMP_create
 */
Int ListMP_open(String name, ListMP_Handle *handlePtr);

/*! @cond
 *  @brief      Open a created ListMP instance by address
 *
 *  Just like ListMP_open(), openByAddr returns a handle to a
 *  created ListMP instance.  This function allows the ListMP to be
 *  opened using a shared address instead of a name.
 *  While ListMP_open() should generally be used to open ListMP
 *  instances that have been either locally or remotely created, openByAddr
 *  may be used to bypass a NameServer query that would typically be
 *  required of an ListMP_open() call.
 *
 *  Opening by address requires that the created instance was created
 *  by supplying a ListMP_Params#sharedAddr parameter rather than a
 *  ListMP_Params#regionId parameter.
 *
 *  A status value of #ListMP_S_SUCCESS is returned if the ListMP is
 *  successfully opened.  #ListMP_E_FAIL indicates that the ListMP is
 *  not yet ready to be opened.
 *
 *  Call ListMP_close() when the opened instance is not longer needed.
 *
 *  @param      sharedAddr  Shared address for the ListMP instance
 *  @param      handlePtr   Pointer to ListMP handle to be opened
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  ListMP successfully opened
 *              - #ListMP_E_FAIL:  ListMP is not ready to be opened
 */
Int ListMP_openByAddr(Ptr sharedAddr, ListMP_Handle *handlePtr);
/*! @endcond */

/*! @cond
 *  @brief      Amount of shared memory required for creation of each instance
 *
 *  The ListMP_Params#regionId or ListMP_Params#sharedAddr
 *  needs to be supplied because the cache alignment settings for the region
 *  may affect the total amount of shared memory required.
 *
 *  @param      params  Pointer to the parameters that will be used in
 *                      the create.
 *
 *  @return     Number of MAUs needed to create the instance.
 */
SizeT ListMP_sharedMemReq(const ListMP_Params *params);
/*! @endcond */

/* =============================================================================
 *  ListMP Per-instance functions
 * =============================================================================
 */

/*!
 *  @brief      Determines if a ListMP instance is empty
 *
 *  @param      handle  a ListMP handle.
 *
 *  @return     TRUE if 'next' element points to head, otherwise FALSE
 */
Bool ListMP_empty(ListMP_Handle handle);

/*!
 *  @brief      Retrieves the GateMP handle associated with the ListMP instance.
 *
 *  @param      handle  a ListMP handle.
 *
 *  @return     GateMP handle for ListMP instance.
 */
GateMP_Handle ListMP_getGate(ListMP_Handle handle);

/*!
 *  @brief      Get an element from front of a ListMP instance
 *
 *  Atomically removes the element from the front of a
 *  ListMP instance and returns a pointer to it.
 *  Uses #ListMP_Params#gate for critical region management.
 *
 *  @param      handle  a ListMP handle.
 *
 *  @return     pointer to former first element.
 */
Ptr ListMP_getHead(ListMP_Handle handle);

/*!
 *  @brief      Get an element from back of a ListMP instance
 *
 *  Atomically removes the element from the back of a
 *  ListMP instance and returns a pointer to it.
 *  Uses #ListMP_Params#gate for critical region management.
 *
 *  @param      handle  a ListMP handle.
 *
 *  @return     pointer to former last element
 */
Ptr ListMP_getTail(ListMP_Handle handle);

/*!
 *  @brief      Insert an element into a ListMP instance
 *
 *  Atomically inserts `newElem` in the instance in front of `curElem`.
 *  To place an element at the back of a ListMP instance, use
 *  ListMP_putTail().  To place an element at the front of a
 *  ListMP instance, use ListMP_putHead().
 *
 *  The following code shows an example.
 *
 *  @code
 *  ListMP_Elem elem, curElem;
 *
 *  ListMP_insert(listHandle, &elem, &curElem);  // insert before curElem
 *  @endcode
 *
 *  @param      handle  a ListMP handle.
 *  @param      newElem  new element to insert into the ListMP.
 *  @param      curElem  current element in the ListMP.
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  if operation was successful
 *              - #ListMP_E_FAIL:  if operation failed
 */
Int ListMP_insert(ListMP_Handle handle,
                  ListMP_Elem *newElem,
                  ListMP_Elem *curElem);


/*!
 *  @brief      Return the next element in a ListMP instance (non-atomic)
 *
 *  Is useful in searching a ListMP instance.
 *  It does not remove any items from the ListMP instance.
 *  The caller should protect the ListMP instance from being changed
 *  while using this call since it is non-atomic.
 *
 *  To look at the first `elem` on the ListMP, use NULL as the `elem`
 *  argument.
 *
 *  The following code shows an example.
 *  The scanning of a ListMP instance should be protected
 *  against other threads that modify the ListMP.
 *
 *  @code
 *  ListMP_Elem   *elem = NULL;
 *  GateMP_Handle gate;
 *  IArg          key;
 *
 *  // get the gate for the ListMP instance
 *  gate = ListMP_getGate(listHandle);
 *
 *  // Begin protection against modification of the ListMP.
 *  key = GateMP_enter(gate);
 *
 *  while ((elem = ListMP_next(ListMPHandle, elem)) != NULL) {
 *      //act on elem as needed. For example call ListMP_remove().
 *  }
 *
 *  // End protection against modification of the ListMP.
 *  GateMP_leave(gate, key);
 *  @endcode
 *
 *  @param      handle  a ListMP handle.
 *  @param      elem    element in ListMP or NULL to start at the head
 *
 *  @return     next element in ListMP instance or NULL to denote end.
 */
Ptr ListMP_next(ListMP_Handle handle, ListMP_Elem *elem);

/*!
 *  @brief      Return the previous element in ListMP instance (non-atomic)
 *
 *  Useful in searching a ListMP instance in reverse order.
 *  It does not remove any items from the ListMP instance.
 *  The caller should protect the ListMP instance from being changed
 *  while using this call since it is non-atomic.
 *
 *  To look at the last `elem` on the ListMP instance, use NULL as the
 *  `elem` argument.
 *
 *  The following code shows an example. The scanning of a ListMP instance
 *  should be protected against other threads that modify the instance.
 *
 *  @code
 *  ListMP_Elem  *elem = NULL;
 *  GateMP_Handle gate;
 *  IArg          key;
 *
 *  // get the gate for the ListMP instance
 *  gate = ListMP_getGate(listHandle);
 *
 *  // Begin protection against modification of the ListMP.
 *  key = GateMP_enter(gate);
 *
 *  while ((elem = ListMP_prev(listHandle, elem)) != NULL) {
 *      //act on elem as needed. For example call ListMP_remove().
 *  }
 *
 *  // End protection against modification of the ListMP.
 *  GateMP_leave(gate, key);
 *  @endcode
 *
 *  @param      handle  a ListMP handle.
 *  @param      elem    element in ListMP or NULL to start at the end
 *
 *  @return     previous element in ListMP or NULL if empty.
 */
Ptr ListMP_prev(ListMP_Handle handle, ListMP_Elem *elem);

/*!
 *  @brief      Put an element at head of a ListMP instance
 *
 *  Atomically places the element at the front of a ListMP instance.
 *  Uses #ListMP_Params#gate for critical region management.
 *
 *  @param      handle  a ListMP handle
 *  @param      elem    pointer to new ListMP element
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  if operation was successful
 *              - #ListMP_E_FAIL:  if operation failed
 */
Int ListMP_putHead(ListMP_Handle handle, ListMP_Elem *elem);

/*!
 *  @brief      Put an element at back of a ListMP instance
 *
 *  Atomically places the element at the back of a ListMP instance.
 *  Uses #ListMP_Params#gate for critical region management.
 *
 *  @param      handle  a ListMP handle
 *  @param      elem    pointer to new ListMP element
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  if operation was successful
 *              - #ListMP_E_FAIL:  if operation failed
 */
Int ListMP_putTail(ListMP_Handle handle, ListMP_Elem *elem);

/*!
 *  @brief      Remove an element from a ListMP instance
 *
 *  Atomically removes an element from a ListMP.
 *
 *  The `elem` parameter is a pointer to an existing element to be removed
 *  from a ListMP instance.
 *
 *  @param      handle  a ListMP handle
 *  @param      elem    element in ListMP
 *
 *  @return     Status
 *              - #ListMP_S_SUCCESS:  if operation was successful
 *              - #ListMP_E_FAIL:  if operation failed
 */
Int ListMP_remove(ListMP_Handle handle, ListMP_Elem *elem);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_ListMP__include */
