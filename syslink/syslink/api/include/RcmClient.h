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
 *  @file     RcmClient.h
 *
 *  @brief    The RCM client module manages the allocation, sending,
 *            receiving of RCM messages to/ from the RCM server.
 *
 *  ============================================================================
 */

#ifndef _RCMCLIENT_H_
#define _RCMCLIENT_H_

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
 *  @def    RcmClient_S_SUCCESS
 *  @brief  Success return code
 */
#define RcmClient_S_SUCCESS             (0)

/*
 *  @def    RcmClient_S_ALREADYSETUP
 *  @brief  Success, already set up
 */
#define RcmClient_S_ALREADYSETUP        (1)

/*
 *  @def    RcmClient_S_ALREADYCLEANEDUP
 *  @brief  Success, already cleaned up
 */
#define RcmClient_S_ALREADYCLEANEDUP    (2)

/*
 *  @def    RcmClient_E_FAIL
 *  @brief  General failure return code
 */
#define RcmClient_E_FAIL                (-1)

/*
 *  @def    RcmClient_E_EXECASYNCNOTENABLED
 *  @brief  The client has not been configured for asynchronous notification
 *
 *  In order to use the RcmClient_execAsync() function, the RcmClient
 *  must be configured with callbackNotification set to true in the
 *  instance create parameters.
 */
#define RcmClient_E_EXECASYNCNOTENABLED (-2)

/*
 *  @def    RcmClient_E_EXECFAILED
 *  @brief  The client was unable to send the command message to the server
 *
 *  An IPC transport error occurred. The message was never sent to the server.
 */
#define RcmClient_E_EXECFAILED          (-3)

/*
 *  @def    RcmClient_E_INVALIDHEAPID
 *  @brief  A heap id must be provided in the create params
 *
 *  When an RcmClient instance is created, a heap id must be given
 *  in the create params. This heap id must be registered with MessageQ
 *  before calling RcmClient_create().
 */
#define RcmClient_E_INVALIDHEAPID       (-4)

/*
 *  @def    RcmClient_E_INVALIDFXNIDX
 *  @brief  Invalid function index
 *
 *  An RcmClient_Message was sent to the server which contained a
 *  function index value (in the fxnIdx field) that was not found
 *  in the server's function table.
 */
#define RcmClient_E_INVALIDFXNIDX       (-5)

/*
 *  @def    RcmClient_E_MSGFXNERROR
 *  @brief  Message function error
 *
 *  There was an error encountered in either the message function or
 *  the library function invoked by the message function. The semantics
 *  of the error code are implementation dependent.
 */
#define RcmClient_E_MSGFXNERROR         (-6)

/*
 *  @def    RcmClient_E_IPCERROR
 *  @brief  An unknown error has been detected from the IPC layer
 *
 *  Check the error log for additional information.
 */
#define RcmClient_E_IPCERROR            (-7)

/*
 *  @def    RcmClient_E_LISTCREATEFAILED
 *  @brief  Failed to create the list object
 */
#define RcmClient_E_LISTCREATEFAILED    (-8)

/*
 *  @def    RcmClient_E_LOSTMSG
 *  @brief  The expected reply message from the server was lost
 *
 *  A command message was sent to the RcmServer but the reply
 *  message was not received. This is an internal error.
 */
#define RcmClient_E_LOSTMSG             (-9)

/*
 *  @def    RcmClient_E_MSGALLOCFAILED
 *  @brief  Insufficient memory to allocate a message
 *
 *  The message heap cannot allocate a buffer of the requested size.
 *  The reported size it the requested data size and the underlying
 *  message header size.
 */
#define RcmClient_E_MSGALLOCFAILED      (-10)

/*
 *  @def    RcmClient_E_MSGQCREATEFAILED
 *  @brief  The client message queue could not be created
 *
 *  Each RcmClient instance must create its own message queue for
 *  receiving return messages from the RcmServer. The creation of
 *  this message queue failed, thus failing the RcmClient instance
 *  creation.
 */
#define RcmClient_E_MSGQCREATEFAILED    (-11)

/*
 *  @def    RcmClient_E_MSGQOPENFAILED
 *  @brief  The server message queue could not be opened
 *
 *  Each RcmClient instance must open the server's message queue.
 *  This error is raised when an internal error occurred while trying
 *  to open the server's message queue.
 */
#define RcmClient_E_MSGQOPENFAILED      (-12)

/*
 *  @def    RcmClient_E_SERVERERROR
 *  @brief  The server returned an unknown error code
 *
 *  The server encountered an error with the given message but
 *  the error code is not recognized by the client.
 */
#define RcmClient_E_SERVERERROR         (-13)

/*
 *  @def    RcmClient_E_SERVERNOTFOUND
 *  @brief  The server specified in the create params was not found
 *
 *  When creating an RcmClient instance, the specified server could not
 *  be found. This could occur if the server name is incorrect, or
 *  if the RcmClient instance is created before the RcmServer. In such an
 *  instance, the client can retry when the RcmServer is expected to
 *  have been created.
 */
#define RcmClient_E_SERVERNOTFOUND      (-14)

/*
 *  @def    RcmClient_E_SYMBOLNOTFOUND
 *  @brief  The given symbol was not found in the server symbol table
 *
 *  This error could occur if the symbol spelling is incorrect or
 *  if the RcmServer is still loading its symbol table.
 */
#define RcmClient_E_SYMBOLNOTFOUND      (-15)

/*
 *  @def    RcmClient_E_NOMEMORY
 *  @brief  There is insufficient memory left in the heap
 */
#define RcmClient_E_NOMEMORY            (-16)

/*!
 *  @def    RcmClient_E_JOBIDNOTFOUND
 *  @brief  The given job id was not found on the server
 *
 *  When releasing a job id with a call to RcmClient_releaseJobId(),
 *  this error return value indicates that the given job id was not
 *  previously allocated with a call to RcmClient_acquireJobId().
 */
#define RcmClient_E_JOBIDNOTFOUND       (-17)

/*
 *  @def    RcmClient_E_INVALIDSTATE
 *  @brief  Invalid module state
 */
#define RcmClient_E_INVALIDSTATE        (-18)

/*
 *  @def    RcmClient_E_NOTSUPPORTED
 *  @brief  Function not supported
 */
#define RcmClient_E_NOTSUPPORTED        (-19)

/*
 *  @def    RcmClient_E_INVALIDARG
 *  @brief  Invalid argument
 */
#define RcmClient_E_INVALIDARG          (-20)


/* =============================================================================
 *  constants and types
 * =============================================================================
 */
/*
 *  @def    RcmClient_INVALIDFXNIDX
 *  @brief  Invalid function index
 */
#define RcmClient_INVALIDFXNIDX     ((UInt32)(0xFFFFFFFF))

/*
 *  @def    RcmClient_INVALIDHEAPID
 *  @brief  Invalid heap id
 */
#define RcmClient_INVALIDHEAPID     ((UInt16)(0xFFFF))

/*
 *  @def    RcmClient_INVALIDMSGID
 *  @brief  Invalid message id
 */
#define RcmClient_INVALIDMSGID      ((UInt16)(0))

/*!
 *  @def    RcmClient_DEFAULTPOOLID
 *  @brief  Default worker pool id
 *
 *  The default worker pool is used to process all anonymous messages.
 *  When a new message is allocated, the pool id property is
 *  initialized to this value.
 */
#define RcmClient_DEFAULTPOOLID     ((UInt16)(0x8000))

/*!
 *  @def    RcmClient_DISCRETEJOBID
 *  @brief  Invalid job stream id
 *
 *  All discrete messages must have their jobId property set to this value.
 *  When a new message is allocated, the jobId property is initialized tothis value.
 */
#define RcmClient_DISCRETEJOBID     (0)

/*!
 *  @def    RcmClient_DEFAULT_HEAPID
 *  @brief  RCM client default heap ID
 */
#define RCMCLIENT_DEFAULT_HEAPID    0xFFFF


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining config parameters for the RcmClient module.
 */
typedef struct RcmClient_Config_tag {
    UInt32 maxNameLen;               /*!< Maximum length of name */
    UInt16 defaultHeapIdArrayLength; /*!< Entries in default heapID array */
    UInt16 defaultHeapBlockSize;     /*!< Blocksize of the entries in the
                                          default heapID array */
} RcmClient_Config;

/*
 *  @brief  Instance config-params object.
 */
typedef struct RcmClient_Params_tag {
    UInt16 heapId;                /*!< heap ID for msg alloc */
    Bool   callbackNotification;  /*!< enable/ disable asynchronous exec */
} RcmClient_Params;

/*
 *  @brief RCM message structure.
 */
typedef struct RcmClient_Message_tag {
    UInt16   poolId;    /*!<  worker thread pool to be used */
    UInt16   jobId;     /*!<  worker thread job-ID */
    UInt32   fxnIdx;    /*!<  remote function index */
    Int32    result;    /*!<  function return value */
    UInt32   dataSize;  /*!<  read-only size of data block (in chars) */
    UInt32   data[1];   /*!<  data buffer of dataSize chars */
} RcmClient_Message;

/*
 * @brief RCM remote function pointer.
 */
typedef Int32 (*RcmClient_RemoteFuncPtr)(RcmClient_Message *, Ptr);

/*
 * @brief RCM callback function pointer
 */
typedef Void (*RcmClient_CallbackFxn)(RcmClient_Message *, Ptr);


/* =============================================================================
 *  Forward declarations
 * =============================================================================
 */
/*!
 *   @brief Handle for the RcmClient.
 */
typedef struct RcmClient_Object_tag *RcmClient_Handle;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief  Function to setup RCM client
 */
Void RcmClient_init (Void);

/*!
 * @brief   Function to clean up RCM client
 */
Void RcmClient_exit (Void);

/*!
 *  @brief  Function to create a RCM client instance
 *
 *  @param  server      Name of the Server to connect to
 *  @param  params      Pointer to RcmClient_Params structure
 *  @param  handle      Pointer to return the created handle
 *
 *  @return Status of the call
 */
Int RcmClient_create (String                server,
                      RcmClient_Params *    params,
                      RcmClient_Handle *    handle);

/*!
 *  @brief  Function to delete RCM Client instance
 *
 *  @param  handlePtr   Pointer to handle to delete.
 *
 *  @return Status of the call
 */
Int RcmClient_delete (RcmClient_Handle * handlePtr);

/*!
 *  @brief  Initialize this config-params structure with supplier-specified
 *          defaults
 *
 *  @param  params      Pointer to the parameter structure to be initialized
 *
 *  @return Status of the call
 */
Int RcmClient_Params_init (RcmClient_Params *    params);

/*!
 *  @brief  Acquire unique job-id for a particular RCM client
 *
 *  @param  handle      RcmClient Handle
 *  @param  jobId       Pointer to return the acquired job Id
 *
 *  @return Status of the call
 */
Int RcmClient_acquireJobId (RcmClient_Handle handle,  UInt16 * jobId);

/*!
 *  @brief  Function adds symbol to server, return the function index
 *
 *  @param  handle      RcmClient Handle
 *  @param  name        Name of the symbol to be added
 *  @param  addr        Address of the remote function
 *  @param  index       Pointer to return the index of the function
 *
 *  @return Status of the call
 */
Int RcmClient_addSymbol (RcmClient_Handle           handle,
                         String                     name,
                         RcmClient_RemoteFuncPtr    addr,
                         UInt32 *                   index);

/*!
 *  @brief  Function returns size (in bytes) of RCM header.
 *
 *  @return Size of the RcmClient header
 */
Int RcmClient_getHeaderSize (Void);

/*!
 *  @brief  Function allocates memory for RCM message on heap,
 *          populates MessageQ and RCM message.
 *
 *  @param  handle      RcmClient Handle
 *  @param  dataSize    Size of the message to be allocated
 *  @param  message     Pointer to return the allocated RcmClient message
 *
 *  @return Status of the call
 */
Int RcmClient_alloc (RcmClient_Handle       handle,
                     UInt32                 dataSize,
                     RcmClient_Message **   message);

/*!
 *  @brief  Function requests RCM server to execute remote function
 *
 *  @param  handle      RcmClient Handle
 *  @param  cmdMsg      RcmClient message to be sent to the server
 *  @param  returnMsg   Pointer to return the received RcmClient message
 *
 *  @return Status of the call
 */
Int RcmClient_exec (RcmClient_Handle        handle,
                    RcmClient_Message *     cmdMsg,
                    RcmClient_Message **    returnMsg);

/*!
 *  @brief  Function requests RCM  server to execute remote function, it is
 *          asynchronous
 *
 *  @param  handle      RcmClient Handle
 *  @param  cmdMsg      RcmClient message to be sent to the server
 *  @param  callback    Callback function to be called upon receiving the
 *                      message
 *  @param  appData     Argument to be passed to the callback function
 *
 *  @return Status of the call
 */
Int RcmClient_execAsync (RcmClient_Handle       handle,
                         RcmClient_Message *    cmdMsg,
                         RcmClient_CallbackFxn  callback,
                         Ptr                    appData);

/*!
 *  @brief  Function requests RCM server to execute remote function,
 *          does not wait for completion of remote function for reply
 *
 *  @param  handle      RcmClient Handle
 *  @param  cmdMsg      RcmClient message to be sent to the server
 *  @param  returnMsg   Pointer to return the received RcmClient message
 *
 *  @return Status of the call
 */
Int RcmClient_execDpc (RcmClient_Handle         handle,
                       RcmClient_Message *      cmdMsg,
                       RcmClient_Message **     returnMsg);

/*!
 *  @brief  Function requests RCM server to execute remote function,
 *          provides a msgId to wait on later
 *
 *  @param  handle      RcmClient Handle
 *  @param  cmdMsg      RcmClient message to be sent to the server
 *  @param  msgId       Pointer to return the msgId that can be used to wait on
 *
 *  @return Status of the call
 */
Int RcmClient_execNoWait (RcmClient_Handle      handle,
                          RcmClient_Message *   cmdMsg,
                          UInt16 *              msgId);

/*!
 *  @brief  Function requests RCM server to execute remote function,
 *          without waiting for the reply. The function is same as
 *          previous function RcmClient-execNoReply
 *
 *  @param  handle      RcmClient Handle
 *  @param  cmdMsg      RcmClient message to be sent to the server
 *
 *  @return Status of the call
 */
Int RcmClient_execCmd (RcmClient_Handle handle, RcmClient_Message *cmdMsg);

/*!
 *  @brief  Function frees the RCM message and allocated memory
 *
 *  @param  handle      RcmClient Handle
 *  @param  msg         RcmClient message to be freed
 *
 *  @return Status of the call
 */
Int RcmClient_free (RcmClient_Handle handle, RcmClient_Message *msg);

/*!
 *  @brief  Function gets symbol index
 *
 *  @param  handle      RcmClient Handle
 *  @param  name        Name of the remote function
 *  @param  index       Pointer to return the index of the function
 *
 *  @return Status of the call
 */
Int RcmClient_getSymbolIndex (RcmClient_Handle  handle,
                              String            name,
                              UInt32 *          index);

/*!
 *  @brief  Function removes symbol (remote function) from registry
 *
 *  @param  handle      RcmClient Handle
 *  @param  name        Name of the remote function
 *
 *  @return Status of the call
 */
Int RcmClient_removeSymbol (RcmClient_Handle handle, String name);

/*!
 *  @brief  Function waits till invoked remote function completes
 *          return message will contain result and context
 *
 *  @param  handle      RcmClient Handle
 *  @param  msgId       Message Id to be waited on
 *  @param  returnMsg   Pointer to return the received RcmClient message
 *
 *  @return Status of the call
 */
Int RcmClient_waitUntilDone (RcmClient_Handle       handle,
                             UInt16                 msgId,
                             RcmClient_Message **   returnMsg);

/*!
 *  @brief  Used along with RcmClient_execcmd to check for errors after
 *          the message has already been processed. A return value of
 *          RcmClient_S_SUccESS indicates there is no error and no
 *          return message
 *
 *  @param  handle      RcmClient Handle
 *  @param  returnMsg   Pointer to return the received RcmClient error message
 *
 *  @return Status of the call
 */
Int RcmClient_checkForError (RcmClient_Handle       handle,
                             RcmClient_Message **   returnMsg);

/*
 *  @brief  Release acquired job id
 *
 *  @param  handle      RcmClient Handle
 *  @param  jobId       Jod id to be released
 *
 *  @return Status of the call
 */
Int RcmClient_releaseJobId (RcmClient_Handle handle, UInt16 jobId);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _RCMCLIENT_H_ */
