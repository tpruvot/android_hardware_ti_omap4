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
/* =============================================================================
 *  @file   RcmServer.h
 *
 *  @brief  Remote Command Message Server Module.
 *
 *          The RcmServer processes inbound messages received from an RcmClient.
 *          After processing a message, the server will return the message to
 *          the client.
 *
 *  ============================================================================
 */

#ifndef _RCMSERVER_H_
#define _RCMSERVER_H_

/* Standard headers */
#include <Std.h>
#include <List.h>

/* Module headers */
#include <ti/ipc/MessageQ.h>

#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*
 *  @def    RcmServer_S_SUCCESS
 *  @brief  Success return code
 */
#define RcmServer_S_SUCCESS             (0)

/*
 *  @def    RcmServer_S_ALREADYSETUP
 *  @brief  Module already setup
 */
#define RcmServer_S_ALREADYSETUP        (1)

/*
 *  @def    RcmServer_S_ALREADYCLEANEDUP
 *  @brief  Module already cleaned up
 */
#define RcmServer_S_ALREADYCLEANEDUP    (1)

/*!
 *  @def    RcmServer_E_FAIL
 *  @brief  General failure return code
 */
#define RcmServer_E_FAIL                (-1)

/*!
 *  @def    RcmServer_E_NOMEMORY
 *  @brief  The was insufficient memory left on the heap
 */
#define RcmServer_E_NOMEMORY            (-2)

/*!
 *  @def    RcmServer_E_SYMBOLNOTFOUND
 *  @brief  The given symbol was not found in the server's symbol table
 *
 *  This error could occur if the symbol spelling is incorrect or
 *  if the RcmServer is still loading its symbol table.
 */
#define RcmServer_E_SYMBOLNOTFOUND      (-3)

/*!
 *  @def    RcmServer_E_SYMBOLSTATIC
 *  @brief  The given symbols is in the static table, it cannot be removed
 *
 *  All symbols installed at instance create time are added to the
 *  static symbol table. They cannot be removed. The statis symbol
 *  table must remain intact for the lifespan of the server instance.
 */
#define RcmServer_E_SYMBOLSTATIC        (-4)

/*!
 *  @def    RcmServer_E_SYMBOLTABLEFULL
 *  @brief  The server's symbol table is full
 *
 *  The symbol table is full. You must remove some symbols before
 *  any new symbols can be added.
 */
#define RcmServer_E_SYMBOLTABLEFULL     (-5)

/*
 *  @def    RcmServer_E_INVALIDFXNIDX
 *  @brief  Invalid function index
 *
 *  An RcmClient_Message was received which contained a function
 *  index value that was not found in the server's function table.
 */
#define RcmServer_E_INVALIDFXNIDX       (-6)

/*
 *  @def    RcmServer_E_IPCERR
 *  @brief  An unknown error has been detected from the IPC layer
 *
 *  Check the error log for additional information.
 */
#define RcmServer_E_IPCERR              (-7)

/*
 *  @def    RcmServer_E_MSGQCREATEFAILED
 *  @brief  The client message queue could not be created
 *
 *  Each RcmClient instance must create its own message queue for
 *  receiving return messages from the RcmServer. The creation of
 *  this message queue failed, thus failing the RcmClient instance
 *  creation.
 */
#define RcmServer_E_MSGQCREATEFAILED    (-8)

/*
 *  @def    RcmServer_E_INVALIDARG
 *  @brief  Invalid argument
 */
#define RcmServer_E_INVALIDARG          (-9)

/*
 *  @def    RcmServer_E_INVALIDSTATE
 *  @brief  Module not initialized
 */
#define RcmServer_E_INVALIDSTATE        (-10)


/* =============================================================================
 * Macros
 * =============================================================================
 */
/*
 *  @def    RcmServer_INVALID_JOB_ID
 *  @brief  Invalid Job ID
 */
#define RcmServer_INVALID_JOB_ID        0xFFFF

/*
 *  @def    RcmServer_HIGH_PRIORITY
 *  @brief  High thread priority
 */
#define RCMSERVER_HIGH_PRIORITY         0xFF

/*
 *  @def    RcmServer_REGULAR_PRIORITY
 *  @brief  Regular thread priority
 */
#define RCMSERVER_REGULAR_PRIORITY      0x01

/*
 *  @def    RcmServer_INVALID_OS_PRIORITY
 *  @brief  Invalid OS thread priority
 */
#define RCMSERVER_INVALID_OS_PRIORITY   (-1)


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*
 *  @brief Structure defining config parameters for the RcmClient module.
 */
typedef struct RcmServer_Config_tag {
    UInt32 maxNameLen; /*!< Maximum length of name                           */
    UInt16 maxTables;  /*!< No of dynamic tables for remote function storage */
    UInt16 poolMapLen; /*!< No of pools                                      */
} RcmServer_Config;

/*
 *  @brief RCM server message structure.
 */
typedef struct RcmServer_Message_tag {
    UInt32    fxnIdx;   /*!<  remote function index                   */
    UInt32    result;   /*!<  function return value                   */
    UInt32    dataSize; /*!<  read-only size of data block (in chars) */
    UInt32    data[1];  /*!<  data buffer of dataSize chars           */
} RcmServer_Message;

/*
 *  @brief Remote function type
 *
 *  All functions executed by the RcmServer must be of this
 *  type. Typically, these functions are simply wrappers to the vendor
 *  function. The server invokes this remote function by passing in
 *  the RcmClient_Message.dataSize field and the address of the
 *  RcmClient_Message.data array.
 *
 *  RcmServer_MsgFxn fxn = ...;
 *  RcmClient_Message *msg = ...;
 *  msg->result = (*fxn)(msg->dataSize, msg->data);
 */
typedef Int32 (*RcmServer_MsgFxn)(UInt32, UInt32 *);

/*!
 *  @brief Function descriptor
 *
 *  This function descriptor is used internally in the server. Its
 *  values are defined either at config time by the server's
 *  RcmServer_Params.fxns create parameter or at runtime through a call to
 *  RcmClient_addSymbol().
 */
typedef struct {
    String              name;
    /*!< The name of the function
     *
     *  The name is used for table lookup, it does not search the
     *  actual symbol table. You can provide any string as long as it
     *  is unique and the client uses the same name for lookup.
     */

    RcmServer_MsgFxn    addr;
    /*!< The function address in the server's address space.
     *
     *  The server will ultimately branch to this address.
     */
} RcmServer_FxnDesc;

/*!
 *  @brief Function descriptor array
 */
typedef struct {
    Int                 length;     /*!< The length of the array */
    RcmServer_FxnDesc * elem;       /*!< Pointer to the array */
} RcmServer_FxnDescAry;

/*!
 *  @brief Worker pool descriptor
 *
 *  Use this data structure to define a worker pool to be created either
 *  at the server create time or dynamically at runtime.
 */
typedef struct {
    String  name;
    /*!<
     *   The name of the worker pool.
     */

    UInt    count;
    /*!<
     *   The number of worker threads in the pool.
     */

    Int     priority;
    /*!<
     *   The priority of all threads in the worker pool.
     *
     *  This value is Operating System independent. It determines the
     *  execution priority of all the worker threads in the pool.
     */

    Int     osPriority;
    /*!<
     *   The priority (OS-specific) of all threads in the worker pool
     *
     *  This value is Operating System specific. It determines the execution
     *  priority of all the worker threads in the pool. If this property is
     *  set, it takes precedence over the priority property above.
     */

    Int     stackSize;
    /*!<
     *   The stack size in bytes of a worker thread.
     */

    String  stackSeg;
    /*!<
     *   The worker thread stack placement.
     */
} RcmServer_ThreadPoolDesc;

/*!
 *  @brief  Worker pool descriptor array
 */
typedef struct {
    Int length;
    /*!<
     *   The length of the array
     */

    RcmServer_ThreadPoolDesc *elem;
    /*!<
     *   Pointer to the array
     */
} RcmServer_ThreadPoolDescAry;


/*!
 *  @brief  RcmServer instance object handle
 */
typedef struct RcmServer_Object_tag *RcmServer_Handle;

/*!
 *  @brief  RcmServer Instance create parameters
 */
typedef struct {
    Int                         priority;
    /*!<
     *   Server thread priority.
     *
     *  This value is Operating System independent. It determines the execution
     *  priority of the server thread. The server thread reads the incoming
     *  message and then either executes the message function (in-band
     *  execution) or dispatches the message to a thread pool (out-of-band
     *  execution). The server thread then wait on the next message.
     */

    Int                         osPriority;
    /*!<
     *   Server thread priority (OS-specific).
     *
     *  This value is Operating System specific. It determines the execution
     *  priority of the server thread. If this attribute is set, it takes
     *  precedence over the priority attribute below. The server thread reads
     *  the incoming message and then either executes the message function
     *  (in-band execution) or dispatches the message to a thread pool
     *  (out-of-band execution). The server thread then wait on the next
     *  message.
     */

    Int                         stackSize;
    /*!<
     *   The stack size in bytes of the server thread.
     */

    String                      stackSeg;
    /*!<
     *   The server thread stack placement.
     */

    RcmServer_ThreadPoolDesc    defaultPool;
    /*!<
     *   The default thread pool used for anonymous messages.
     */

    RcmServer_ThreadPoolDescAry workerPools;
    /*!<
     *   Array of thread pool descriptors
     *
     *  The worker pools declared with this instance parameter are statically
     *  bound to the server, they persist for the life of the server. They
     *  cannot be removed with a call to RcmServer_deletePool(). However,
     *  worker threads may be created or deleted at runtime.
     *
     */

    RcmServer_FxnDescAry        fxns;
    /*!<
     *   Array of function names to install into the server
     *
     *  The functions declared with this instance parameter are statically
     *  bound to the server, they persist for the life of the server. They
     *  cannot be removed with a call to RcmServer_removeSymbol().
     *
     *  To specify a function address, use the & character in the string
     *  as in the following example.
     *
     *  @code
     *  RcmServer_FxnDesc serverFxnAry[] = {
     *      { "LED_on",         LED_on  },
     *      { "LED_off",        LED_off }
     *  };
     *
     *  #define serverFxnAryLen (sizeof serverFxnAry / sizeof serverFxnAry[0])
     *
     *  RcmServer_FxnDescAry Server_fxnTab = {
     *      serverFxnAryLen,
     *      serverFxnAry
     *  };
     *
     *  RcmServer_Params rcmServerP;
     *  RcmServer_Params_init(&rcmServerP);
     *  rcmServerP.fxns.length = Server_fxnTab.length;
     *  rcmServerP.fxns.elem = Server_fxnTab.elem;
     *
     *  RcmServer_Handle rcmServerH;
     *  rval = RcmServer_create("ServerA", &rcmServerP, &(rcmServerH));
     *  @endcode
     */
} RcmServer_Params;


/* =============================================================================
 *  APIs
 * =============================================================================
 */

/*!
 *  @brief  Add a symbol to the server's function table<br>
 *
 *          This function adds a new symbol to the server's function table.
 *          This is useful for supporting Dynamic Load Libraries (DLLs).
 *
 *  @param  handle      Handle to an instance object.
 *  @param  name        The function's name.
 *  @param  addr        The function's address in the server's address space.
 *  @param  index       The function's index value to be used in the
 *                      RcmClient_Message.fxnIdx field.
 *
 *  @return Status of the call
 *          - #RcmClient_S_SUCCESS
 *          - #RcmServer_E_NOMEMORY
 *          - #RcmServer_E_SYMBOLTABLEFULL
 */
Int RcmServer_addSymbol (RcmServer_Handle   handle,
                         String             name,
                         RcmServer_MsgFxn   addr,
                         UInt32 *           index);

/*!
 *  @brief  Create an RcmServer instance
 *
 *          A server instance is used to execute functions on behalf of an
 *          RcmClient instance. There can be multiple server instances
 *          on any given CPU. The servers typically reside on a remote CPU
 *          from the RcmClient instance.
 *
 *  @param  name    The name of the server. The RcmClient will
 *                  locate a server instance using this name. It must be
 *                  unique to the system.
 *  @param  params  The create params used to customize the instance object.
 *  @param  handle  An opaque handle to the created instance object.
 *
 *  @return Status of the call
 *          - #RcmClient_S_SUCCESS
 *          - #RcmServer_E_FAIL
 *          - #RcmServer_E_NOMEMORY
 */
Int RcmServer_create (String            name,
                      RcmServer_Params *params,
                      RcmServer_Handle *handle);

/*!
 *  @brief  Function will delete a RCM Server instance
 *
 *  @param  handlePtr   Pointer to handle to delete.
 *
 *  @return Status of the call
 */
Int RcmServer_delete (RcmServer_Handle * handlePtr);

/*!
 *  @brief  Finalize the RcmServer module
 *
 *          This function is used to finalize the RcmServer module. Any
 *          resources acquired by RcmServer_init() will be released. Do not call
 *          any RcmServer functions after calling RcmServer_exit().
 *
 *          This function must be serialized by the caller.
 */
Void RcmServer_exit (Void);

/*!
 *  @brief  Initialize the RcmServer module.
            This function must be serialized by the caller
 */
Void RcmServer_init (Void);

/*!
 *  @brief  Initialize the instance create params structure.
 *
 *  @params Pointer to a RcmServer_Params structure
 *
 *  @return Status of the call.
 */
Int RcmServer_Params_init (RcmServer_Params *params);

/*!
 *  @brief  Remove a symbol and from the server's function table
 *          Useful when unloading a DLL from the server.
 *
 *  @param  handle  Handle to an instance object.
 *  @param  name    The function's name.
 *
 *  @return Status of the call
 *          -#RcmClient_S_SUCCESS
 *          -#RcmServer_E_SYMBOLNOTFOUND
 *          -#RcmServer_E_SYMBOLSTATIC
 */
Int RcmServer_removeSymbol (RcmServer_Handle handle, String name);

/*!
 *  @brief  Start the server
 *
 *          The server is created in stop mode. It will not start
 *          processing messages until it has been started.
 *
 *  @param  handle  Handle to the RcmServer object
 *
 *  @return Status of the call
 *          -#RcmClient_S_SUCCESS
 *          -#RcmServer_E_FAIL
 */
Int RcmServer_start (RcmServer_Handle handle);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _RCMSERVER_H_ */
