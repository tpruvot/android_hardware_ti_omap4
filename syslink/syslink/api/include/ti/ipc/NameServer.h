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
 *  @file       NameServer.h
 *
 *  @brief      NameServer Manager
 *
 *  The NameServer module manages local name/value pairs that
 *  enables an application and other modules to store and
 *  retrieve values based on a name. The module supports different
 *  lengths of values. The NameServer_add()/{@link NameServer_get()}
 *  functions are for variable length values. The NameServer_addUInt32()
 *  /{@link NameServer_getUInt32()} functions are optimized for UInt32
 *  variables and constants.
 *
 *  The NameServer module maintains thread-safety for the APIs. However,
 *  the NameServer APIs cannot be called from interrupt (i.e. Hwi context).
 *  The can be called from Swi's and Tasks.
 *
 *  Each NameServer instance manages a different name/value table.
 *  This allows each table to be customized to meet the requirements
 *  of user:
 *  @li <b>Size differences:</b> one table could allow long values
 *  (e.g. > 32 bits) while another table could be used to store integers.
 *  This customization enables better memory usage.
 *  @li <b>Performance:</b> improves search time when retrieving
 *  a name/value pair.
 *  @li <b>Relax name uniqueness:</b> names in a specific table must
 *  be unique, but the same name can be used in different tables.
 *
 *  When adding a name/value pair, the name and value are copied into
 *  internal buffers in NameServer. To minimize runtime memory allocation
 *  these buffers can be allocated at creation time.
 *
 *  NameServer maintains the name/values table in local memory (e.g.
 *  not shared memory). However the NameServer module can be
 *  used in a multiprocessor system.
 *  The module communicates to other processors via NameServer Remote drivers.
 *  Remote drivers inherit from the INameServerRemote interface.
 *  The communication to the other processors is dependent on the
 *  Remote drivers implementation.
 *
 *  When a remote driver is created, it registers with NameServer. There
 *  can be multiple types of NameServer Remote drivers in a system.
 *  The IPC package contains the ti.sdo.ipc.NameServerRemoteNotify
 *  remote driver.
 *
 *  The NameServer module uses the MultiProc module for
 *  identifying the different processors. Which remote processors and
 *  the order they are queried is determined by the procId array in the
 *  {@link NameServer_get()} function.
 *
 *  Currently there is no endian or wordsize conversion performed by the
 *  NameServer module. Also there is no asynchronous support at this time.
 */

#ifndef ti_ipc_NameServer__include
#define ti_ipc_NameServer__include

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    NameServer_S_BUSY
 *  @brief  The resource is still in use
 */
#define NameServer_S_BUSY               2

/*!
 *  @def    NameServer_S_ALREADYSETUP
 *  @brief  The module has been already setup
 */
#define NameServer_S_ALREADYSETUP       1

/*!
 *  @def    NameServer_S_SUCCESS
 *  @brief  Operation is successful.
 */
#define NameServer_S_SUCCESS            0

/*!
 *  @def    NameServer_E_FAIL
 *  @brief  Generic failure.
 */
#define NameServer_E_FAIL               -1

/*!
 *  @def    NameServer_E_INVALIDARG
 *  @brief  Argument passed to function is invalid.
 */
#define NameServer_E_INVALIDARG         -2

/*!
 *  @def    NameServer_E_MEMORY
 *  @brief  Operation resulted in memory failure.
 */
#define NameServer_E_MEMORY             -3

/*!
 *  @def    NameServer_E_ALREADYEXISTS
 *  @brief  The specified entity already exists.
 */
#define NameServer_E_ALREADYEXISTS      -4

/*!
 *  @def    NameServer_E_NOTFOUND
 *  @brief  Unable to find the specified entity.
 */
#define NameServer_E_NOTFOUND           -5

/*!
 *  @def    NameServer_E_TIMEOUT
 *  @brief  Operation timed out.
 */
#define NameServer_E_TIMEOUT            -6

/*!
 *  @def    NameServer_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define NameServer_E_INVALIDSTATE       -7

/*!
 *  @def    NameServer_E_OSFAILURE
 *  @brief  A failure occurred in an OS-specific call
 */
#define NameServer_E_OSFAILURE          -8

/*!
 *  @def    NameServer_E_RESOURCE
 *  @brief  Specified resource is not available
 */
#define NameServer_E_RESOURCE           -9

/*!
 *  @def    NameServer_E_RESTART
 *  @brief  Operation was interrupted. Please restart the operation
 */
#define NameServer_E_RESTART            -10

/*!
 *  @def    NameServer_ALLOWGROWTH
 *  @brief  Allow dynamic growth of the NameServer instance table
 */
#define NameServer_ALLOWGROWTH          (~0)

/*!
 *  @def    NameServer_Params_MAXNAMELEN
 *  @brief  The default maximum length of the name for the name/value pair
 */
#define NameServer_Params_MAXNAMELEN    16

/*!
 *  @brief      NameServer handle type
 */
typedef struct NameServer_Object *NameServer_Handle; /* opaque */

/*!
 *  @brief      NameServer_Handle type
 */
typedef struct NameServer_Params {
    /*!
     *  Maximum number of name/value pairs that can be dynamically created.
     *
     *  This parameter allows NameServer to pre-allocate memory.
     *  When NameServer_add() or NameServer_addUInt32() is
     *  called, no memory allocation occurs.
     *
     *  If the number of pairs is not known at configuration time, set this
     *  value to #NameServer_ALLOWGROWTH. This instructs NameServer
     *  to grow the table as needed. NameServer will allocate memory from the
     *  #NameServer_Params#tableHeap when a name/value pair is added.
     *
     *  The default is #NameServer_ALLOWGROWTH.
     */
    UInt maxRuntimeEntries;

    /*!
     *  Name/value table is allocated from this heap.
     *
     *  The instance table and related buffers are allocated out of this heap
     *  during the dynamic create. This heap is also used to allocate new
     *  name/value pairs when #NameServer_ALLOWGROWTH for
     *  #NameServer_Params#maxRuntimeEntries
     *
     *  The default is to use the same heap that instances are allocated
     *  from which can be configured via the
     *  NameServer.common$.instanceHeap configuration parameter.
     */
    Ptr  tableHeap;

    /*!
     *  Check if a name already exists in the name/value table.
     *
     *  When a name/value pair is added during runtime, if this boolean is
     *  true, the table is searched to see if the name already exists. If
     *  it does, the name is not added and the
     *  #NameServer_E_ALREADYEXISTS error is returned.
     *
     *  If this flag is false, the table will not be checked to see if the
     *  name already exists. It will simply be added. This mode has better
     *  performance at the expense of potentially having non-unique names
     *  in the table.
     *
     *  This flag is used for runtime adds only. Adding non-unique names during
     *  configuration results in a build error.
     */
    Bool checkExisting;

    /*!
     *  Length, in MAUs, of the value field in the table.
     *
     *  Any value less than sizeof(UInt32) will be rounded up to sizeof(UInt32)
     */
    UInt maxValueLen;

    /*!
     *  Length, in MAUs, of the name field in the table.
     *
     *  The maximum length of the name portion of the name/value
     *  pair.  The length includes the null terminator ('\\0').
     */
    UInt maxNameLen;
} NameServer_Params;


/* =============================================================================
 *  NameServer Module-wide Functions
 * =============================================================================
 */

/*!
 *  @brief      Initializes parameter structure
 *
 *  @param      params  Instance param structure
 *
 *  @sa         NameServer_create
 */
Void NameServer_Params_init(NameServer_Params *params);

/*!
 *  @brief      Creates a NameServer instance
 *
 *  @param      name    Instance name
 *  @param      params  Instance param structure
 *
 *  @return     NameServer handle
 *
 *  @sa         NameServer_delete
 */
NameServer_Handle NameServer_create(String name,
                                    const NameServer_Params *params);

/*!
 *  @brief      Deletes a NameServer instance
 *
 *  @param      handlePtr  Pointer to a NameServer handle
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Instance successfully deleted
 *              - #NameServer_E_FAIL:  Instance delete failed
 *
 *  @sa         NameServer_create
 */
Int NameServer_delete(NameServer_Handle *handlePtr);

/*!
 *  @brief      Gets the NameServer handle given the name
 *
 *  Each NameServer instance has a name. The name and this function can be
 *  used by the remote driver module to aid in queries to remote
 *  processors. This function allows the caller to get the local handle
 *  based on the instance name.
 *
 *  For example, when a remote driver sends a request to the remote
 *  processor, it probably contains the name of the instance to query.
 *  The receiving remote driver uses that name to obtain the handle for
 *  the local NameServer instance in question. Then that instance
 *  can be queried for the name/value pair.
 *
 *  This function does not query remote processors.
 *
 *  @param      name  Name of instance
 *
 *  @return     NameServer handle
 */
NameServer_Handle NameServer_getHandle(String name);

/* =============================================================================
 *  NameServer Per-instance Functions
 * =============================================================================
 */

/*!
 *  @brief      Adds a variable length value into the local NameServer table
 *
 *  This function adds a variable length value into the local table.
 *  If the #NameServer_Params#checkExisting flag was true on
 *  creation, this function searches the table to make sure the name
 *  does not already exist.  If it does, the name/value pair is not
 *  added and the #NameServer_E_ALREADYEXISTS error is returned.
 *
 *  There is memory allocation during this function if
 *  #NameServer_Params#maxRuntimeEntries is set to
 *  #NameServer_ALLOWGROWTH.
 *
 *  This function copies the name and buffer into the name/value table,
 *  so they do not need to be persistent after the call.
 *
 *  The function does not query remote processors to make sure the
 *  name is unique.
 *
 *  @param      handle  Instance handle
 *  @param      name    Name for the name/value pair
 *  @param      buf     Pointer to value for the name/value pair
 *  @param      len     length of the value
 *
 *  @return     Unique entry identifier
 *
 *  @sa         NameServer_addUInt32,
 *              NameServer_get,
 */
Ptr NameServer_add(NameServer_Handle handle, String name, Ptr buf, UInt32 len);

/*!
 *  @brief      Adds a 32-bit value into the local NameServer table
 *
 *  This function adds a 32-bit value into the local table. This
 *  function is simply a specialized NameServer_add() function
 *  that allows a convenient way to add UInt32 bit values without needing
 *  to specify the address of a UInt32 local variable.
 *
 *  If the #NameServer_Params#checkExisting flag was true on
 *  creation, this function searches the table to make sure the name does
 *  not already exist.  If it does, the name/value pair is not added and the
 *  #NameServer_E_ALREADYEXISTS error is returned.
 *
 *  This function copies the name into the name/value table.
 *
 *  There is memory allocation during this function if
 *  #NameServer_Params#maxRuntimeEntries is set to
 *  #NameServer_ALLOWGROWTH.
 *
 *  The function does not query remote processors to make sure the
 *  name is unique.
 *
 *  @param      handle  Instance handle
 *  @param      name    Name for the name/value pair
 *  @param      value   Value for the name/value pair
 *
 *  @return     Unique entry identifier
 *
 *  @sa         NameServer_add,
 *              NameServer_getUInt32
 */
Ptr NameServer_addUInt32(NameServer_Handle handle, String name, UInt32 value);

/*!
 *  @brief      Gets the variable value length by name
 *
 *  If the name is found, the value is copied into the value
 *  argument, the number of MAUs copied is returned in len, and
 *  a success status is returned.
 *
 *  If the name is not found, zero is returned in len, the contents
 *  of value are not modified.  Not finding a name is not considered
 *  an error.
 *
 *  For NameServer to work across processors, each processor must
 *  must be assigned a unique id.
 *
 *  The processors to query is determined by the procId array.
 *  The array is used to specify which processors to query and the order.
 *  The processor ids are determined via the MultiProc module.
 *  The following are the valid settings for the procId array:
 *  @li <b>NULL:</b> denotes that the local table is searched first, then
 *  all remote processors in ascending order by MultiProc id.
 *  @li <b>Filled in array:</b> The NameServer will search the processors
 *  in the order given in the array. The end of the list must be denoted
 *  by the #MultiProc_INVALIDID value.
 *
 *  @code
 *  UInt16 queryList[4] = {3, 2, 4, MultiProc_INVALIDID};
 *  count = NameServer_get(handle, "foo", &value, &len, queryList);
 *  @endcode
 *
 *  The NameServer_getLocal() call can be used for searching
 *  the local table only.
 *
 *  @param      handle          Instance handle
 *  @param      name            Name to query
 *  @param      buf             Pointer where the value is returned
 *  @param      len             Pointer for input/output length.
 *  @param      procId          Array of processor(s) to query
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully found entry, len
 *                                        holds amount of data retrieved.
 *              - #NameServer_E_FAIL:  Entry was not found, len remains
 *                                     unchanged.
 *              - #NameServer_E_NOTFOUND:  Error searching for entry, len
 *                                         remains unchanged.
 *
 *  @sa         NameServer_add,
 *              NameServer_getLocal
 */
Int NameServer_get(NameServer_Handle handle,
                   String name,
                   Ptr buf,
                   UInt32 *len,
                   UInt16 procId[]);

/*!
 *  @brief      Gets a 32-bit value by name
 *
 *  If the name is found, the value is copied into the value
 *  argument, and a success status is returned.
 *
 *  If the name is not found, the contents of value are not modified.
 *  Not finding a name is not considered an error.
 *
 *  For NameServer to work across processors, each processor must
 *  must be assigned a unique id.
 *
 *  The processors to query is determined by the procId array.
 *  The array is used to specify which processors to query and the order.
 *  The processor ids are determined via the MultiProc module.
 *  The following are the valid settings for the procId array:
 *  @li <b>NULL:</b> denotes that the local table is searched first, then
 *  all remote processors in ascending order by MultiProc id.
 *  @li <b>Filled in array:</b> The NameServer will search the processors
 *  in the order given in the array. The end of the list must be denoted
 *  by the #MultiProc_INVALIDID value.
 *
 *  @code
 *  UInt16 queryList[4] = {3, 2, 4, MultiProc_INVALIDID};
 *  count = NameServer_getUInt32(handle, "foo", &value, queryList);
 *  @endcode
 *
 *  The NameServer_getLocal() call can be used for searching
 *  the local table only.
 *  @param      handle          Instance handle
 *  @param      name            Name to query
 *  @param      buf             Pointer where the value is returned
 *  @param      procId          Array of processor(s) to query
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully found entry
 *              - #NameServer_E_FAIL:  Entry was not found
 *              - #NameServer_E_NOTFOUND:  Error searching for entry
 *
 *  @sa         NameServer_addUInt32,
 *              NameServer_getLocalUInt32
 */
Int NameServer_getUInt32(NameServer_Handle handle,
                         String name,
                         Ptr buf,
                         UInt16 procId[]);

/*!
 *  @brief      Gets the variable value length by name from the local table
 *
 *  If the name is found, the value is copied into the value
 *  argument, the number of MAUs copied is returned in len, and
 *  a success status is returned.
 *
 *  If the name is not found, zero is returned in len, a fail status is
 *  returned and the contents of value are not modified.
 *  Not finding a name is not considered an error.
 *
 *  This function only searches the local name/value table.
 *
 *  @param      handle  Instance handle
 *  @param      name    Name to query
 *  @param      buf     Pointer where the value is returned
 *  @param      len     Length of the value
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully found entry, len
 *                                        holds amount of data retrieved.
 *              - #NameServer_E_FAIL:  Entry was not found, len remains
 *                                     unchanged.
 *
 *  @sa         NameServer_add,
 *              NameServer_get
 */
Int NameServer_getLocal(NameServer_Handle handle,
                        String name,
                        Ptr buf,
                        UInt32 *len);

/*!
 *  @brief      Gets a 32-bit value by name from the local table
 *
 *  If the name is found, the 32-bit value is copied into the value
 *  argument and a success status is returned.
 *
 *  If the name is not found, zero is returned in len and the contents
 *  of value are not modified. Not finding a name is not considered
 *  an error.
 *
 *  This function only searches the local name/value table.
 *
 *  @param      handle  Instance handle
 *  @param      name    Name to query
 *  @param      buf     Pointer where the value is returned
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully found entry
 *              - #NameServer_E_FAIL:  Entry was not found
 *
 *  @sa         NameServer_addUInt32,
 *              NameServer_getUInt32
 */
Int NameServer_getLocalUInt32(NameServer_Handle handle,
                              String name,
                              Ptr buf);

/*!  @cond
  *  @brief      Match the name with the longest entry
  *
  *  Returns the number of characters that matched with an entry.
  *  So if "abc" and "ab" was an entry and you called match with "abcd",
  *  this function will match the "abc" entry. The return would be 3 since
  *  three characters matched.
  *
  *  Currently only 32-bit values are supported.
  *
  *  @param     handle  Instance handle
  *  @param     name    Name in question
  *  @param     value   Pointer in which the value is returned
  *
  *  @return    Number of matching characters
  */
Int NameServer_match(NameServer_Handle handle, String name, UInt32 *value);
/*! @endcond */

/*!
 *  @brief        Remove a name/value pair from the table
 *
 *  This function removes a name/value pair from the table.
 *
 *  If #NameServer_Params#maxRuntimeEntries is set to
 *  #NameServer_ALLOWGROWTH,
 *  memory will be freed which was allocated in NameServer_add().
 *  Otherwise, no memory is freed during this call.
 *  The entry in the table is simply emptied.
 *  When another NameServer_add() occurs, it will reuse the empty
 *  entry.
 *
 *  @param      handle  Instance handle
 *  @param      name    Name to remove
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully removed entry or
 *                                        entry does not exists
 *              - #NameServer_E_FAIL:  Operation failed
 *
 *  @sa         NameServer_add
 */
Int NameServer_remove(NameServer_Handle handle, String name);

/*!
 *  @brief        Remove a name/value pair from the table
 *
 *  This function removes an entry from the table based on the
 *  unique identifier returned from NameServer_add() or
 *  NameServer_addUInt32().
 *
 *  If #NameServer_Params#maxRuntimeEntries is set to
 *  #NameServer_ALLOWGROWTH,
 *  memory will be freed which was allocated in NameServer_add().
 *  Otherwise, no memory is freed during this call.
 *  The entry in the table is simply emptied.
 *  When another NameServer_add() occurs, it will reuse the
 *  empty entry.
 *
 *  Once an Entry is removed from the NameServer table, it cannot be
 *  removed again (just like you cannot free the same block of memory
 *  twice).
 *
 *  @param      handle  Instance handle
 *  @param      entry   Pointer to entry to be removed
 *
 *  @return     Status
 *              - #NameServer_S_SUCCESS:  Successfully removed entry or
 *                                        entry does not exists
 *              - #NameServer_E_FAIL:  Operation failed
 *
 *  @sa         NameServer_add
 */
Int NameServer_removeEntry(NameServer_Handle handle, Ptr entry);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ti_ipc_NameServer__include */
