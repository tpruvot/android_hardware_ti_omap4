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
 *  @file   MessageQ.c
 *
 *  @brief  MessageQ module implementation
 *
 *  The MessageQ module supports the structured sending and receiving of
 *  variable length messages. This module can be used for homogeneous or
 *  heterogeneous multi-processor messaging.
 *
 *  MessageQ provides more sophisticated messaging than other modules. It is
 *  typically used for complex situations such as multi-processor messaging.
 *
 *  The following are key features of the MessageQ module:
 *  -Writers and readers can be relocated to another processor with no
 *   runtime code changes.
 *  -Timeouts are allowed when receiving messages.
 *  -Readers can determine the writer and reply back.
 *  -Receiving a message is deterministic when the timeout is zero.
 *  -Messages can reside on any message queue.
 *  -Supports zero-copy transfers.
 *  -Can send and receive from any type of thread.
 *  -Notification mechanism is specified by application.
 *  -Allows QoS (quality of service) on message buffer pools. For example,
 *   using specific buffer pools for specific message queues.
 *
 *  Messages are sent and received via a message queue. A reader is a thread
 *  that gets (reads) messages from a message queue. A writer is a thread that
 *  puts (writes) a message to a message queue. Each message queue has one
 *  reader and can have many writers. A thread may read from or write to multiple
 *  message queues.
 *
 *  Conceptually, the reader thread owns a message queue. The reader thread
 *  creates a message queue. Writer threads  a created message queues to
 *  get access to them.
 *
 *  Message queues are identified by a system-wide unique name. Internally,
 *  MessageQ uses the NameServermodule for managing
 *  these names. The names are used for opening a message queue. Using
 *  names is not required.
 *
 *  Messages must be allocated from the MessageQ module. Once a message is
 *  allocated, it can be sent on any message queue. Once a message is sent, the
 *  writer loses ownership of the message and should not attempt to modify the
 *  message. Once the reader receives the message, it owns the message. It
 *  may either free the message or re-use the message.
 *
 *  Messages in a message queue can be of variable length. The only
 *  requirement is that the first field in the definition of a message must be a
 *  MsgHeader structure. For example:
 *  typedef struct MyMsg {
 *      MessageQ_MsgHeader header;
 *      ...
 *  } MyMsg;
 *
 *  The MessageQ API uses the MessageQ_MsgHeader internally. Your application
 *  should not modify or directly access the fields in the MessageQ_MsgHeader.
 *
 *  All messages sent via the MessageQ module must be allocated from a
 *  Heap implementation. The heap can be used for
 *  other memory allocation not related to MessageQ.
 *
 *  An application can use multiple heaps. The purpose of having multiple
 *  heaps is to allow an application to regulate its message usage. For
 *  example, an application can allocate critical messages from one heap of fast
 *  on-chip memory and non-critical messages from another heap of slower
 *  external memory
 *
 *  MessageQ does support the usage of messages that allocated via the
 *  alloc function. Please refer to the staticMsgInit
 *  function description for more details.
 *
 *  In a multiple processor system, MessageQ communications to other
 *  processors via MessageQTransport instances. There must be one and
 *  only one MessageQTransport instance for each processor where communication
 *  is desired.
 *  So on a four processor system, each processor must have three
 *  MessageQTransport instance.
 *
 *  The user only needs to create the MessageQTransport instances. The instances
 *  are responsible for registering themselves with MessageQ.
 *  This is accomplished via the registerTransport function.
 *
 *  ============================================================================
 */



/* Standard headers */
#include <Std.h>

/* Osal And Utils  headers */
#include <String.h>
#include <List.h>
#include <Trace.h>
#include <Memory.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/MessageQ.h>
#include <_MessageQ.h>
#include <MessageQDrvDefs.h>
#include <MessageQDrv.h>
#include <ti/ipc/SharedRegion.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/* Structure defining object for the Gate Peterson */
typedef struct MessageQ_Object_tag {
    Ptr              knlObject;
    /*!< Pointer to the kernel-side MessageQ object. */
    MessageQ_QueueId queueId;
    /* Unique id */
} MessageQ_Object;

/*!
 *  @brief  MessageQ Module state object
 */
typedef struct MessageQ_ModuleObject_tag {
    UInt32          setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
} MessageQ_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    MessageQ_state
 *
 *  @brief  MessageQ state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
MessageQ_ModuleObject MessageQ_state =
{
    .setupRefCount = 0
};

/*!
 *  @var    MessageQ_module
 *
 *  @brief  Pointer to the MessageQ module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
MessageQ_ModuleObject * MessageQ_module = &MessageQ_state;


/* =============================================================================
 * APIS
 * =============================================================================
 */
/* Function to get default configuration for the MessageQ module. */
Void
MessageQ_getConfig (MessageQ_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int status = MessageQ_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_getConfig",
                             MessageQ_E_INVALIDARG,
                             "Argument of type (MessageQ_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        MessageQDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.getConfig.config = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            MessageQDrv_ioctl (CMD_MESSAGEQ_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "MessageQ_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        MessageQDrv_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_ENTER, "MessageQ_getConfig");
}


/* Function to setup the MessageQ module. */
Int
MessageQ_setup (const MessageQ_Config * config)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_setup", config);

    /* TBD: Protect from multiple threads. */
    MessageQ_module->setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (MessageQ_module->setupRefCount > 1) {
        status = MessageQ_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "MessageQ module has been already setup in this process.\n"
                   "    RefCount: [%d]\n",
                   MessageQ_module->setupRefCount);
    }
    else {
        /* Open the driver handle. */
        status = MessageQDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.setup.config = (MessageQ_Config *) config;
            status = MessageQDrv_ioctl (CMD_MESSAGEQ_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MessageQ_setup",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_setup", status);

    return status;
}


/* Function to destroy the MessageQ module. */
Int
MessageQ_destroy (void)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "MessageQ_destroy");

    /* TBD: Protect from multiple threads. */
    MessageQ_module->setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (MessageQ_module->setupRefCount >= 1) {
        status = MessageQ_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "MessageQ module has been already setup in this process.\n"
                   "    RefCount: [%d]\n",
                   MessageQ_module->setupRefCount);
    }
    else {
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        MessageQDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_destroy", status);

    return status;
}


/* Function to initialize the parameters for the MessageQ instance. */
Void
MessageQ_Params_init (MessageQ_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = MessageQ_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    MessageQDrv_CmdArgs   cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_Params_init",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_Params_init",
                             MessageQ_E_INVALIDARG,
                             "Argument of type (MessageQ_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.ParamsInit.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        MessageQDrv_ioctl (CMD_MESSAGEQ_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MessageQ_Params_init");

    return;
}


/* Function to create a MessageQ object. */
MessageQ_Handle
MessageQ_create (      String            name,
                 const MessageQ_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = MessageQ_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    MessageQ_Object *     handle = NULL;
    MessageQDrv_CmdArgs   cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_create", name, params);

    /* NULL name is allowed for unnamed (anonymous queues) */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_create",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if ((params != NULL) && (params->synchronizer != NULL)) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_create",
                             MessageQ_E_INVALIDARG,
                             "Cannot provide non-NULL synchronizer on HLOS!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.create.params = (MessageQ_Params *) params;
        cmdArgs.args.create.name = name;
        if (name != NULL) {
            cmdArgs.args.create.nameLen = (String_len (name) + 1);
        }
        else {
            cmdArgs.args.create.nameLen = 0;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        MessageQDrv_ioctl (CMD_MESSAGEQ_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_create",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Allocate memory for the handle */
            handle = (MessageQ_Object *) Memory_calloc (NULL,
                                                       sizeof (MessageQ_Object),
                                                       0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (handle == NULL) {
                status = MessageQ_E_MEMORY;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MessageQ_create",
                                     status,
                                     "Memory allocation failed for handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Set pointer to kernel object into the user handle. */
                handle->knlObject = cmdArgs.args.create.handle;
                handle->queueId = cmdArgs.args.create.queueId;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
             }
         }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_create", handle);

    return (MessageQ_Handle) handle;
}


/* Function to delete a MessageQ object for a specific slave processor. */
Int
MessageQ_delete (MessageQ_Handle * handlePtr)
{
    Int                    status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_delete",
                             status,
                             "Module is in an invalid state!");
    }
    else if (handlePtr == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_delete",
                             status,
                             "handlePtr pointer passed is NULL!");
    }
    else if (*handlePtr == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_delete",
                             status,
                             "*handlePtr passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.deleteMessageQ.handle =
                            ((MessageQ_Object *)(*handlePtr))->knlObject;
        GT_assert (curTrace,
                   (((MessageQ_Object *)(handlePtr))->knlObject != NULL));
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_DELETE, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_delete",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            Memory_free (NULL, *handlePtr, sizeof (MessageQ_Object));
            *handlePtr = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_delete", status);

    return status;
}


/* Opens a created instance of MessageQ module. */
Int
MessageQ_open (String name, MessageQ_QueueId * queueId)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_open", name, queueId);

    GT_assert (curTrace, (name != NULL));
    GT_assert (curTrace, (queueId != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_open",
                             status,
                             "Module is in an invalid state!");
    }
    else if (name == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_open",
                             status,
                             "name passed is NULL!");
    }
    else if (queueId == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_open",
                             status,
                             "name passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.open.name = name;
        if (name != NULL) {
            cmdArgs.args.open.nameLen = (String_len (name) + 1);
        }
        else {
            cmdArgs.args.open.nameLen = 0;
        }

        /* Initialize return queue ID to invalid. */
        *queueId = MessageQ_INVALIDMESSAGEQ;

        status = MessageQDrv_ioctl (CMD_MESSAGEQ_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        /* MessageQ_E_NOTFOUND is a valid runtime failure. */
        if ((status < 0) && (status != MessageQ_E_NOTFOUND)) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_open",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            *queueId = cmdArgs.args.open.queueId;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_open", status);

    return status;
}


/* Closes previously opened/created instance of MessageQ module. */
Int
MessageQ_close (MessageQ_QueueId * queueId)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_close", queueId);

    GT_assert (curTrace, (queueId != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_close",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (queueId == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_close",
                             MessageQ_E_INVALIDARG,
                             "queueId passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.close.queueId = *queueId;
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_CLOSE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_close",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            *queueId = MessageQ_INVALIDMESSAGEQ;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_close", status);

    return status;
}


/* Place a message onto a message queue. */
Int
MessageQ_put (MessageQ_QueueId queueId,
              MessageQ_Msg     msg)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;
    UInt16              index;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_put", queueId, msg);

    GT_assert (curTrace, (queueId != MessageQ_INVALIDMESSAGEQ));
    GT_assert (curTrace, (msg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_put",
                             status,
                             "Module is not initialized!");
    }
    else if (queueId == MessageQ_INVALIDMESSAGEQ) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_put",
                             status,
                             "queueId is MessageQ_INVALIDMESSAGEQ!");
    }
    else if (msg  == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_put",
                             status,
                             "msg is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.put.queueId  = queueId;
        index = SharedRegion_getId (msg);
        cmdArgs.args.put.msgSrPtr = SharedRegion_getSRPtr (msg, index);

        status = MessageQDrv_ioctl (CMD_MESSAGEQ_PUT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_put",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_put", status);

    return (status);
}


/* Gets a message for a message queue and blocks if the queue is empty.
 * If a message is present, it returns it.  Otherwise it blocks
 * waiting for a message to arrive.
 * When a message is returned, it is owned by the caller.
 */
Int
MessageQ_get (MessageQ_Handle handle, MessageQ_Msg * msg ,UInt timeout)
{
    Int                 status   = MessageQ_S_SUCCESS;
    SharedRegion_SRPtr  msgSrPtr = SharedRegion_INVALIDSRPTR;
    MessageQDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_get", handle, timeout);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (msg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_put",
                             status,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_count",
                             MessageQ_E_INVALIDMSG,
                             "handle pointer passed is null!");
    }
    else if (msg == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_get",
                             status,
                             "msg pointer passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.get.handle = ((MessageQ_Object *)(handle))->knlObject;
        GT_assert (curTrace,
                   (((MessageQ_Object *)(handle))->knlObject != NULL));
        cmdArgs.args.get.timeout = timeout;

        /* Initialize return message pointer to NULL. */
        *msg = NULL;
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_GET, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (    (status < 0)
            &&  (status != MessageQ_E_TIMEOUT)
            &&  (status != MessageQ_E_UNBLOCKED)) {
            /* Timeout and unblock are valid runtime errors. */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_get",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            msgSrPtr = cmdArgs.args.get.msgSrPtr;
            if (msgSrPtr != SharedRegion_INVALIDSRPTR) {
                *msg = (MessageQ_Msg) SharedRegion_getPtr (msgSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_get", status);

    return (status);
}


/* Return a count of the number of messages in the queue */
Int
MessageQ_count (MessageQ_Handle handle)
{
    Int                 status = MessageQ_S_SUCCESS;
    Int                 count  = 0;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_count", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_count",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_count",
                             MessageQ_E_INVALIDMSG,
                             "Invalid NULL obj pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.count.handle = ((MessageQ_Object *)(handle))->knlObject;
        GT_assert (curTrace,
                   (((MessageQ_Object *)(handle))->knlObject != NULL));
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_COUNT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_get",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            count = cmdArgs.args.count.count;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_count", count);

    return (count);
}


/* Initializes a message not obtained from MessageQ_alloc. */
Void
MessageQ_staticMsgInit (MessageQ_Msg msg, UInt32 size)
{
    GT_2trace (curTrace, GT_ENTER, "MessageQ_staticMsgInit", msg, size);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_staticMsgInit",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (msg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_staticMsgInit",
                             MessageQ_E_INVALIDMSG,
                             "Msg is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Fill in the fields of the message */
        msg->heapId  = MessageQ_STATICMSG;
        msg->msgSize = size;
        msg->replyId = (UInt16) MessageQ_INVALIDMESSAGEQ;
        msg->msgId   = MessageQ_INVALIDMSGID;
        msg->dstId   = (UInt16) MessageQ_INVALIDMESSAGEQ;
        msg->flags   =   MessageQ_HEADERVERSION
                       | MessageQ_NORMALPRI;
        msg->srcProc = MultiProc_self ();

        /* TBD: May need to drop down to kernel-side for this. */
        /* msg->seqNum  = MessageQ_module->seqNum++;

        if (MessageQ_module->cfg.traceFlag == TRUE) {
            GT_3trace (curTrace,
                       GT_1CLASS,
                       "MessageQ_staticMsgInit",
                       (msg),
                       ((msg)->seqNum),
                       ((msg)->srcProc));
        } */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MessageQ_staticMsgInit");
}


/* Allocate a message and initialize the needed fields (note some
 * of the fields in the header are set via other APIs or in the
 * MessageQ_put function.)
 */
MessageQ_Msg
MessageQ_alloc (UInt16 heapId, UInt32 size)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                 status   = MessageQ_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    SharedRegion_SRPtr  msgSrPtr = SharedRegion_INVALIDSRPTR;
    MessageQ_Msg        msg      = NULL;
    MessageQDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_alloc", heapId, size);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_alloc",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.alloc.heapId = heapId;
        cmdArgs.args.alloc.size   = size;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        MessageQDrv_ioctl (CMD_MESSAGEQ_ALLOC, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_alloc",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            msgSrPtr = cmdArgs.args.alloc.msgSrPtr;
            if (msgSrPtr != SharedRegion_INVALIDSRPTR) {
                msg = SharedRegion_getPtr (msgSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    GT_1trace (curTrace, GT_LEAVE, "MessageQ_alloc", msg);

    return msg;
}


/* Frees the message back to the heap that was used to allocate it. */
Int
MessageQ_free (MessageQ_Msg msg)
{
    UInt32              status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;
    UInt16              index;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_free", msg);

    GT_assert (curTrace, (msg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_free",
                             status,
                             "Module is not initialized!");
    }
    else if (msg == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_free",
                             status,
                             "msg passed is null!");
    }
    else if (msg->heapId ==  MessageQ_STATICMSG) {
        status = MessageQ_E_CANNOTFREESTATICMSG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_free",
                             MessageQ_E_CANNOTFREESTATICMSG,
                             "Static message has been passed!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        index = SharedRegion_getId (msg);
        cmdArgs.args.free.msgSrPtr = SharedRegion_getSRPtr (msg, index);
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_FREE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_free",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_free", status);

    return status;
}


/* Register a heap with MessageQ. */
Int
MessageQ_registerHeap (Ptr heap, UInt16 heapId)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_registerHeap", heap, heapId);

    GT_assert (curTrace, (heap != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_registerHeap",
                             status,
                             "Module is not initialized!");
    }
    else if (heap == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_registerHeap",
                             status,
                             "Invalid NULL heap provided!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Translate Heap handle to kernel-side gate handle. */
        cmdArgs.args.registerHeap.heap = IHeap_getKnlHandle (heap);
        GT_assert (curTrace, (cmdArgs.args.registerHeap.heap));

        cmdArgs.args.registerHeap.heapId = heapId;
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_REGISTERHEAP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "MessageQ_registerHeap",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_registerHeap", status);

    return status;
}


/* Unregister a heap with MessageQ. */
Int
MessageQ_unregisterHeap (UInt16 heapId)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_unregisterHeap", heapId);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_unregisterHeap",
                             status,
                             "Module is not initialized!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.unregisterHeap.heapId = heapId;
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_UNREGISTERHEAP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "MessageQ_unregisterHeap",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_unregisterHeap", status);

    return status;
}


/* Unblocks a MessageQ */
Int
MessageQ_unblock (MessageQ_Handle handle)
{
    Int                 status;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_unblock", handle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        status = MessageQ_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_unblock",
                             status,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        status = MessageQ_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_unblock",
                             status,
                             "handle passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.unblock.handle = ((MessageQ_Object *)(handle))->knlObject;
        GT_assert (curTrace,
                   (((MessageQ_Object *)(handle))->knlObject != NULL));
        status = MessageQDrv_ioctl (CMD_MESSAGEQ_UNBLOCK, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_unblock",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_unblock", status);

    return status;
}


/* Embeds a source message queue into a message. */
Void
MessageQ_setReplyQueue (MessageQ_Handle   handle,
                        MessageQ_Msg      msg)
{
    MessageQ_Object * obj = (MessageQ_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_setReplyQueue", handle, msg);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (msg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_setReplyQueue",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_setReplyQueue",
                             MessageQ_E_INVALIDARG,
                             "handle passed is null!");
    }
    else if (msg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_setReplyQueue",
                             MessageQ_E_INVALIDARG,
                             "msg passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        msg->replyId   = (UInt16)(obj->queueId);
        msg->replyProc = (UInt16)(obj->queueId >> 16);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MessageQ_setReplyQueue");
}


/* Returns the QueueId associated with the handle. */
MessageQ_QueueId
MessageQ_getQueueId (MessageQ_Handle handle)
{
    MessageQ_Object * obj     = (MessageQ_Object *) handle;
    UInt32            queueId = MessageQ_INVALIDMESSAGEQ;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_getQueueId", obj);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_unregisterTransport",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_getQueueId",
                             MessageQ_E_INVALIDARG,
                             "handle passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        queueId = (obj->queueId);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_getQueueId", queueId);

    return queueId;
}


/* Returns the MultiProc processor id on which the queue resides. */
UInt16
MessageQ_getProcId (MessageQ_Handle handle)
{
    MessageQ_Object * obj    = (MessageQ_Object *) handle;
    UInt16            procId = MultiProc_INVALIDID;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_getProcId", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MessageQ_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_unregisterTransport",
                             MessageQ_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_getProcId",
                             MessageQ_E_INVALIDARG,
                             "handle passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        procId = (UInt16)(obj->queueId >> 16);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_getProcId", procId);

    return procId;
}


/* Sets the tracing of a message */
Void
MessageQ_setMsgTrace (MessageQ_Msg msg, Bool traceFlag)
{
    GT_2trace (curTrace, GT_ENTER, "MessageQ_setMsgTrace", msg, traceFlag);

    GT_assert (curTrace, (msg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (msg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MessageQ_setMsgTrace",
                             MessageQ_E_INVALIDARG,
                             "msg passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        msg->flags =    (msg->flags & ~MessageQ_TRACEMASK)
                     |  (traceFlag << MessageQ_TRACESHIFT);
        GT_4trace (curTrace,
                   GT_1CLASS,
                   "MessageQ_setMsgTrace",
                    msg,
                    msg->seqNum,
                    msg->srcProc,
                    traceFlag);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MessageQ_setMsgTrace");
}


/* Returns the amount of shared memory used by one transport instance.
 *
 *  The MessageQ module itself does not use any shared memory but the
 *  underlying transport may use some shared memory.
 */
SizeT
MessageQ_sharedMemReq (Ptr sharedAddr)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                 status = MessageQ_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    SizeT               memReq = 0u;
    UInt16              index;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_sharedMemReq", sharedAddr);

    if (MultiProc_getNumProcessors ()  > 1) {
        index = SharedRegion_getId (sharedAddr);
        cmdArgs.args.sharedMemReq.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            sharedAddr, index);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        MessageQDrv_ioctl (CMD_MESSAGEQ_SHAREDMEMREQ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_sharedMemReq",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            memReq = cmdArgs.args.sharedMemReq.memReq;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        /* Only 1 processor: no shared memory needed */
        memReq = 0;
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_sharedMemReq", memReq);

    return (memReq);
}


/* Calls the SetupProxy to setup the MessageQ transports. */
Int
MessageQ_attach (UInt16 remoteProcId, Ptr sharedAddr)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;
    UInt16              index;

    GT_2trace (curTrace, GT_ENTER, "MessageQ_attach", remoteProcId, sharedAddr);

    if (MultiProc_getNumProcessors ()  > 1) {
        cmdArgs.args.attach.remoteProcId = remoteProcId;
        index = SharedRegion_getId (sharedAddr);
        cmdArgs.args.attach.sharedAddrSrPtr = SharedRegion_getSRPtr (sharedAddr,
                                                                     index);

        status = MessageQDrv_ioctl (CMD_MESSAGEQ_ATTACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_attach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_attach", status);

    return (status);
}


/* Calls the SetupProxy to detach the MessageQ transports. */
Int
MessageQ_detach (UInt16 remoteProcId)
{
    Int                 status = MessageQ_S_SUCCESS;
    MessageQDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MessageQ_detach", remoteProcId);

    if (MultiProc_getNumProcessors ()  > 1) {
        cmdArgs.args.detach.remoteProcId = remoteProcId;

        status = MessageQDrv_ioctl (CMD_MESSAGEQ_DETACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MessageQ_detach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "MessageQ_detach", status);

    return (status);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
