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
 *  @file     RcmClient.c
 *
 *  @brief    The RCM client module manages the allocation, sending,
 *            receiving of RCM messages to/ from the RCM server.
 *
 *  ============================================================================
 */

/* Standard headers */
#include <host_os.h>
#include <pthread.h>

/* OSAL, Utility and IPC headers */
#include <Std.h>
#include <Trace.h>
#include <ti/ipc/MessageQ.h>
#include <GateMutex.h>
#include <IGateProvider.h>
#include <OsalSemaphore.h>
#include <Memory.h>
#include <List.h>
#include <String.h>

/* Module level headers */
#include <RcmTypes.h>
#include <RcmClient.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * RCM client defines
 * =============================================================================
 */
/*!
 *  @brief  RCM module defines
 */
#define RCMCLIENT_HEAPID_ARRAY_LENGTH      4   /*!< Default heap array len  */
#define RCMCLIENT_HEAPID_ARRAY_BLOCKSIZE 256   /*!< Default heap block size */
#define MAX_NAME_LEN                      32   /*!< Max RCM client name len */
#define WAIT_NONE                        0x0   /*!< 0 wait time for msg Que */

/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/*!
 *  @brief  RCM recipient used for managing messages from server
 *          (mailman algorithm)
 */
typedef struct Recipient_tag {
    List_Elem            elem;       /*!< Pointer to list of messages       */
    UInt16               msgId;      /*!< Msg ID received from server       */
    RcmClient_Message  * msg;        /*!< Ptr to msg received from server   */
    OsalSemaphore_Handle event;      /*!< Semaphore to unblock client task  */
} Recipient;

/*!
 *  @brief RCM Client instance object structure
 */
typedef struct RcmClient_Object_tag {
    IGateProvider_Handle gate;        /*!< Instance gate                    */
    MessageQ_Handle      msgQue;      /*!< Inbound message queue            */
    MessageQ_Handle      errorMsgQue; /*!< Error message queue              */
    UInt16               heapId;      /*!< Heap used for message allocation */
    Ptr                  sync;        /*!< Synchronizer for message queue   */
    MessageQ_QueueId     serverMsgQ;  /*!< Server message queue id          */
    Bool                 cbNotify;    /*!< Callback notification            */
    UInt16               msgId;       /*!< Last used message id             */
    OsalSemaphore_Handle mbxLock;     /*!< Mailbox lock                     */
    OsalSemaphore_Handle queueLock;   /*!< Message queue lock               */
    List_Handle          recipients;  /*!< List of waiting recipients       */
    List_Handle          newMail;     /*!< List of undelivered messages     */
} RcmClient_Object;

/*!
 *  @brief RCM Client defualt state initialization
 */
typedef struct RcmClient_ModuleObject_tag {
    Int32            setupRefCount;      /*!< # of times setup/detroy called */
    RcmClient_Config defaultCfg;         /*!< Default config values          */
    RcmClient_Params defaultInst_params; /*!< Default creation params        */
    UInt16 heapIdAry[RCMCLIENT_HEAPID_ARRAY_LENGTH];
                                         /*!< RCM Client default heap array  */
} RcmClient_ModuleObject;

/* =============================================================================
 *  Private functions
 * =============================================================================
 */
/*!
 *  @brief      Generate unique id for Messages
 */
static UInt16 _RcmClient_genMsgId (RcmClient_Object *obj);

/*!
 *  @brief      Get packet addres from RCM message
 */
static inline RcmClient_Packet * _RcmClient_getPacketAddr (
                                                        RcmClient_Message *msg);

/*!
 *  @brief      Get packet addres from MessageQ message
 */
static inline RcmClient_Packet * _getPacketAddrMsgqMsg (MessageQ_Msg msg);

/*!
 *  @brief      Get packet address from List element
 */
static inline RcmClient_Packet * _getPacketAddrElem (List_Elem *elem);

/*!
 *  @brief      Get a specific return message from the server
 */
static Int _RcmClient_getReturnMsg (RcmClient_Object      * obj,
                                    const UInt16            msgId,
                                    RcmClient_Message    ** returnMsg);

/*!
 *  @brief      Initialize RCM client module
 */
static Int _RcmClient_Instance_init (RcmClient_Object         * obj,
                                     String                     server,
                                     const RcmClient_Params   * params);

/*!
 *  @brief      Finalize RCM client for deletion
 */
static Int _RcmClient_Instance_finalize (RcmClient_Object *obj);


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*
 *  @var    RcmClient_state
 *
 *  @brief  RcmClient state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
RcmClient_ModuleObject RcmClient_module_obj =
{
    .defaultCfg.maxNameLen                   = MAX_NAME_LEN,
    .defaultCfg.defaultHeapIdArrayLength     = RCMCLIENT_HEAPID_ARRAY_LENGTH,
    .defaultCfg.defaultHeapBlockSize         = RCMCLIENT_HEAPID_ARRAY_BLOCKSIZE,
    .setupRefCount                           = 0,
    .defaultInst_params.heapId               = RCMCLIENT_DEFAULT_HEAPID,
    .defaultInst_params.callbackNotification = false
};

/*
 *  @var    RcmClient_module
 *
 *  @brief  Pointer to RcmClient_module_obj .
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
RcmClient_ModuleObject* RcmClient_module = &RcmClient_module_obj;


/*!
 *  @brief      Initialize RCM client module
 *
 *  @sa         RcmClient_exit
 */
Void RcmClient_init (Void)
{
    Int status = RcmClient_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmClient_init");

    /* TBD: Protect from multiple threads. */
    RcmClient_module->setupRefCount++;

    /* This is needed at runtime so should not be in
     * SYSLINK_BUILD_OPTIMIZE.
     */
    if (RcmClient_module->setupRefCount > 1) {
        status = RcmClient_S_ALREADYSETUP;
        GT_1trace (curTrace,
                GT_1CLASS,
                "RcmClient module has been already setup in this process.\n"
                " RefCount: [%d]\n",
                RcmClient_module->setupRefCount);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_init", status);
}


/*!
 *  @brief      This function is called after deleting RCM client
 *
 *  @sa         RcmClient_exit
 */
Void RcmClient_exit (Void)
{
    Int status = RcmClient_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmClient_exit");

    /* TBD: Protect from multiple threads. */
    RcmClient_module->setupRefCount--;

    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (RcmClient_module->setupRefCount < 0) {
        status = RcmClient_S_ALREADYCLEANEDUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "RcmClient module has been already been cleaned up in this "
                   "process.\nRefCount: [%d]\n",
                   RcmClient_module->setupRefCount);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_exit", status);
}


/*!
 *  @brief      Create RCM client instance
 *
 *  @param      server     name of RCM server
 *  @param      params     initialization params
 *  @param      handle     RCM instance handle
 *
 *  @sa         RcmClient_delete
 */
Int RcmClient_create (String                server,
                      RcmClient_Params    * params,
                      RcmClient_Handle    * handle)
{
    RcmClient_Object * obj    = NULL;
    Int                status = RcmClient_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmClient_create");

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Handle pointer passed is NULL!");
        goto leave;
    }
    if (params == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Argument of type (RcmClient_Params *) passed "
                             "is NULL!");
        goto leave;
    }
    if (server == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Server name passed is NULL!");
        goto leave;
    }
    if ((String_len (server)) > RcmClient_module->defaultCfg.maxNameLen) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Server name is too long!");
        goto leave;
    }

    obj = (RcmClient_Object *) Memory_calloc (NULL, sizeof (RcmClient_Object),
                                                0);
    if (obj == NULL) {
        status = RcmClient_E_NOMEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_create",
                             status,
                             "Object memory allocation failed!");
        goto leave;
    }

    /* Object-specific initialization */
    status = _RcmClient_Instance_init (obj, server, params);
    if (status < 0) {
        _RcmClient_Instance_finalize (obj);
        Memory_free (NULL, (RcmClient_Object *)obj, sizeof (RcmClient_Object));
        goto leave;
    }

    /* Success, return opaque pointer */
    *handle = (RcmClient_Handle)obj;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_create", status);

    return status;
}


/*!
 *  @brief      Initialize RCM client instance and allocate memory
 *
 *  @param      handle     RCM instance handle
 *  @param      server     name of RCM server
 *  @param      params     initialization params
 *
 *  @sa         _RcmClient_Instance_finalize
 */
static
Int _RcmClient_Instance_init (RcmClient_Object        * obj,
                              String                    server,
                              const RcmClient_Params  * params)
{
    MessageQ_Params mqParams;
    Int             rval;
    UInt16          procId;
    List_Params     listP;
    Int             status = RcmClient_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "_RcmClient_Instance_init");

    /* Initialize instance data */
    obj->heapId      = 0xFFFF;
    obj->msgId       = 0xFFFF;
    obj->sync        = NULL;
    obj->serverMsgQ  = MessageQ_INVALIDMESSAGEQ;
    obj->msgQue      = NULL;
    obj->errorMsgQue = NULL;
    obj->mbxLock     = NULL;
    obj->queueLock   = NULL;
    obj->recipients  = NULL;
    obj->newMail     = NULL;

    /* Create a gate instance */
    obj->gate = (IGateProvider_Handle) GateMutex_create ();
    if (obj->gate == NULL) {
        status = RcmClient_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create mutex");
        goto leave;
    }

    /* Create the message queue for inbound messages */
    MessageQ_Params_init (&mqParams);
    obj->msgQue = MessageQ_create (NULL, &mqParams);
    if (obj->msgQue == NULL) {
        status = RcmClient_E_MSGQCREATEFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create MessageQ");
        goto leave;
    }

    /* Create the message queue for error messages */
    MessageQ_Params_init (&mqParams);
    obj->errorMsgQue = MessageQ_create (NULL, &mqParams);
    if (obj->errorMsgQue == NULL) {
        status = RcmClient_E_MSGQCREATEFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create error MessageQ");
        goto leave;
    }

    /* Locate server message queue */
    rval = MessageQ_open (server, (MessageQ_QueueId *)(&obj->serverMsgQ));
    if (rval == MessageQ_E_NOTFOUND) {
        status = RcmClient_E_SERVERNOTFOUND;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "MessageQ not found");
        goto leave;
    }
    else if (rval < 0) {
        status = RcmClient_E_MSGQOPENFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             rval,
                             "Error in opening MessageQ");
        goto leave;
    }

    obj->cbNotify = params->callbackNotification;
    /* Create callback server */
    if (obj->cbNotify == true) {
        /* TODO Create callback server thread */
        /* Make sure to free resources acquired by thread */
        GT_0trace (curTrace,
                   GT_4CLASS,
                   "RcmClient asynchronous transfers not supported \n");
        goto leave;
    }

    /* Register the heapId used for message allocation */
    obj->heapId = params->heapId;
    if (obj->heapId == RCMCLIENT_DEFAULT_HEAPID ) {
        GT_0trace (curTrace,
                   GT_4CLASS,
                   "Rcm default heaps not supported \n");
        /* Extract procId from the server queue id */
        /* TODO: procId = MessageQ_getProcId (&(obj->serverMsgQ)); */
        procId = (UInt16) ((UInt32)obj->serverMsgQ) >> 16;
        /* MessageQ_getProcId needs to be fixed!*/
        if (procId >= RCMCLIENT_HEAPID_ARRAY_LENGTH) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmClient_Instance_init",
                                 status,
                                 "procId >="
                                  "RCMCLIENT_HEAPID_ARRAY_LENGTH");
            status = RcmClient_E_FAIL;
            goto leave;
        }
        obj->heapId = RcmClient_module->heapIdAry[procId];
    }

    /* Create the mailbox lock */
    obj->mbxLock = OsalSemaphore_create (OsalSemaphore_Type_Counting, 1);
    GT_assert (curTrace, (obj->mbxLock != NULL));
    if (obj->mbxLock ==  NULL) {
        status = RcmClient_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create mbx lock");
        goto leave;
    }

    /* Create the message queue lock */
    obj->queueLock = OsalSemaphore_create (OsalSemaphore_Type_Counting, 1);
    GT_assert (curTrace, (obj->queueLock != NULL));
    if (obj->queueLock ==  NULL) {
        status = RcmClient_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create queue lock");
        goto leave;
    }

    /* Create the return message recipient list */
    List_Params_init (&listP);
    listP.gateHandle = obj->gate;
    obj->recipients = List_create (&listP);
    GT_assert (curTrace, (obj->recipients != NULL));
    if (obj->recipients == NULL) {
        status = RcmClient_E_LISTCREATEFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Unable to create recipients list");
        goto leave;
    }

    /* Create list of undelivered messages (new mail) */
    List_Params_init (&listP);
    listP.gateHandle = obj->gate;
    obj->newMail = List_create (&listP);
    if (obj->newMail == NULL) {
        status = RcmClient_E_LISTCREATEFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_Instance_init",
                             status,
                             "Could not create new mail list object");
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmClient_Instance_init", status);

    return status;
}


/*!
 *  @brief      Delete RCM client instance
 *
 *  @param      handlePtr     RCM instance handle pointer
 *
 *  @sa         RcmClient_create
 */
Int RcmClient_delete (RcmClient_Handle * handlePtr)
{
    RcmClient_Object  * obj    = NULL;
    Int                 status = RcmClient_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmClient_delete");

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_delete",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handlePtr == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_delete",
                             status,
                             "handlePtr pointer passed is NULL!");
        goto leave;
    }
    if (*handlePtr == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_delete",
                             status,
                             "*handlePtr passed is NULL!");
        goto leave;
    }

    /* Finalize the object */
    obj    = (RcmClient_Object *)(*handlePtr);
    status = _RcmClient_Instance_finalize (obj);

    Memory_free (NULL, (RcmClient_Object *)*handlePtr,
                    sizeof (RcmClient_Object));
    *handlePtr = NULL;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_delete", status);

    return status;
}


/*!
 *  @brief      Deallocate memory of RCM instance and reset instance
 *
 *  @param      obj     RCM instance handle pointer
 *
 *  @sa         _RcmClient_Instance_init
 */
Int _RcmClient_Instance_finalize (RcmClient_Object * obj)
{
    Int status = RcmClient_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "RcmClient_instance_finalize", obj);

    if (obj->newMail != NULL) {
        while (!(List_empty(obj->newMail))) {
            List_remove(obj->newMail, List_get(obj->newMail));
        }
        List_delete (&obj->newMail);
    }

    if (obj->recipients != NULL) {
        while (!(List_empty(obj->recipients))) {
            List_remove(obj->recipients, List_get(obj->recipients));
        }
        List_delete (&obj->recipients);
    }

    if (obj->queueLock != NULL) {
        OsalSemaphore_delete (&(obj->queueLock));
        obj->queueLock = NULL;
    }

    if (obj->mbxLock != NULL) {
        OsalSemaphore_delete (&(obj->mbxLock));
        obj->mbxLock = NULL;
    }

    if (obj->serverMsgQ != MessageQ_INVALIDMESSAGEQ) {
        MessageQ_close ((MessageQ_QueueId *)(&obj->serverMsgQ));
    }

    if (obj->errorMsgQue != NULL) {
        MessageQ_delete (&obj->errorMsgQue);
    }

    if (obj->msgQue != NULL) {
        MessageQ_delete (&obj->msgQue);
    }

    /* Destruct the instance gate */
    GateMutex_delete ((GateMutex_Handle *)&(obj->gate));

    GT_1trace (curTrace, GT_LEAVE, "_RcmClient_Instance_finalize", status);

    return status;
}


/*!
 *  @brief      Initialize this config-params structure with supplier-specified
 *              defaults before instance creation.
 *
 *  @param      params  Instance config-params structure.
 *
 *  @sa         RcmClient_create
 */
Int RcmClient_Params_init (RcmClient_Params * params)
{
    Int status = RcmClient_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "RcmClient_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_Params_init",
                             status,
                             "Module is in an invalid state!");
    }
    else if (params == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_Params_init",
                             status,
                             "Argument of type (RcmClient_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        params->heapId = RcmClient_module->defaultInst_params.heapId;
        params->callbackNotification = \
                    RcmClient_module->defaultInst_params.callbackNotification;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_Params_init", status);

    return status;
}


/*!
 *  @brief      Request a unique Id for a job from RCM server
 *
 *  @param      handle    Instance pointer
 *  @param      jobIdPtr  Pointer to the jobId
 *
 *  @sa         RcmClient_releaseJobId
 */
Int RcmClient_acquireJobId (RcmClient_Handle handle, UInt16 * jobIdPtr)
{
    RcmClient_Message * msg             = NULL;
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    UInt16              msgId;
    Int                 rval;
    UInt16              serverStatus;
    Int                 status          = RcmClient_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_acquireJobId", handle, jobIdPtr);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (jobIdPtr == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             status,
                             "jobIdPtr pointer passed is NULL!");
        goto leave;
    }

    /* Allocate a message */
    status = RcmClient_alloc (handle, sizeof(UInt16), &msg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             status,
                             "Error allocating RCM message");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (msg);
    packet->desc |= RcmClient_Desc_JOB_ACQ << RcmClient_Desc_TYPE_SHIFT;
    msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_FAIL;
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg (handle, msgId, &msg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             status,
                             "Get return message failed");
        goto leave;
    }

    /* Check message status for error */
    packet = _RcmClient_getPacketAddr (msg);
    serverStatus = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                        RcmClient_Desc_TYPE_SHIFT);
    switch (serverStatus) {
    case RcmServer_Status_SUCCESS:
        break;

    default:
        status = RcmClient_E_SERVERERROR;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_acquireJobId",
                             serverStatus,
                             "Server error");
        goto leave;
    }

    /* Extract return value */
    *jobIdPtr = (UInt16)(msg->data[0]);

leave:
    if (msg != NULL) {
        RcmClient_free (handle, msg);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_acquireJobId", status);

    return status;
}


/*!
 *  @brief      Adds symbol to server, return the function
 *              (Not supported at this time)
 *
 *  @param      handle  Instance handle
 *  @param      name    Function name
 *  @param      addr    Function address
 *  @param     *index   Function index
 */
Int RcmClient_addSymbol (RcmClient_Handle           handle,
                         String                     name,
                         RcmClient_RemoteFuncPtr    addr,
                         UInt32                   * index)
{
    Int status = RcmClient_E_NOTSUPPORTED;

    GT_4trace (curTrace, GT_ENTER, "RcmClient_addSymbol", handle, name, addr,
                index);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_addSymbol",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_addSymbol",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (name == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_addSymbol",
                             status,
                             "Remote function name passed is NULL!");
        goto leave;
    }
    if (addr == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_addSymbol",
                             status,
                             "Remote function address is NULL!");
        goto leave;
    }
    if (index == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_addSymbol",
                             status,
                             "Index pointer passed is NULL!");
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_addSymbol", status);

    return status;
}


/*!
 *  @brief      Returns size (in chars) of RCM header.
 */
Int RcmClient_getHeaderSize (Void)
{
    Int headerSize;

    GT_0trace (curTrace, GT_ENTER, "RcmClient_getHeaderSize");

    /* Size (in bytes) of RCM header including the messageQ header */
    /* We deduct sizeof(UInt32) as "data[1]" is the start of the payload */
    headerSize = sizeof (RcmClient_Packet) - sizeof (UInt32);

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_getHeaderSize", headerSize);

    return headerSize;
}


/*!
 *  @brief      Allocates memory for RCM message on heap, populates MessageQ
 *              and RCM message
 *
 *  @param      handle   Instance handle
 *  @param      dataSize Function name
 *  @param    **message  pointer to message ptr which is updated here
 *
 *  @sa         RcmClient_free
 */
Int RcmClient_alloc (RcmClient_Handle       handle,
                     UInt32                 dataSize,
                     RcmClient_Message   ** message)
{
    Int                 totalSize;
    RcmClient_Packet  * packet;
    Int                 status      = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_alloc", handle, dataSize,
                message);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_alloc",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_alloc",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (message == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_alloc",
                             status,
                             "Message pointer passed is NULL!");
        goto leave;
    }

    /* Ensure minimum size of UInt32[1] */
    dataSize  = (dataSize < sizeof(UInt32) ? sizeof(UInt32) : dataSize);

    /* Total memory size (in chars) needed for headers and payload */
    /* We deduct sizeof(UInt32) as "data[1]" is the start of the payload */
    totalSize = sizeof(RcmClient_Packet) - sizeof(UInt32) + dataSize;

    /* Allocate the message */
    packet = (RcmClient_Packet *)MessageQ_alloc (handle->heapId, totalSize);
    if (NULL == packet) {
        *message = NULL;
        status = RcmClient_E_MSGALLOCFAILED;
        goto leave;
    }

    /* Initialize the packet structure */
    packet->desc             = 0;
    packet->msgId            = _RcmClient_genMsgId (handle);
    packet->message.poolId   = RcmClient_DEFAULTPOOLID;
    packet->message.jobId    = RcmClient_DISCRETEJOBID;
    packet->message.fxnIdx   = RcmClient_INVALIDFXNIDX;
    packet->message.result   = 0;
    packet->message.dataSize = dataSize;

    /* Set cmdMsg pointer to start of the message struct */
    *message = (RcmClient_Message *)(&(packet->message));

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_alloc", status);

    return status;
}


/*!
 *  @brief      If success, no message will be returned. If error message
 *              returned, then function return value will always be < 0.
 *
 *  @param      handle  RCM client instance
*   @param      rtnMsg  Pointer for returning error message
 *
 *  @sa         RcmClient_execCmd
 */
Int RcmClient_checkForError (RcmClient_Handle       handle,
                             RcmClient_Message   ** rtnMsg)
{
    RcmClient_Message * rcmMsg;
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    UInt16              serverStatus;
    Int                 rval;
    Int                 status          = RcmClient_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_checkForError", handle, rtnMsg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             status,
                             "Invalid handle passed");
        goto leave;
    }
    if (rtnMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             status,
                             "Return message pointer passed is NULL!");
        goto leave;
    }

    *rtnMsg = NULL;

    /* Get error message if available (non-blocking) */
    rval = MessageQ_get (handle->errorMsgQue, &msgqMsg, 0);
    if ((rval != MessageQ_E_TIMEOUT) && (rval < 0)) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             rval,
                             "MessageQ get fails");
        status = RcmClient_E_IPCERROR;
        goto leave;
    }
    if (msgqMsg == NULL) {
        goto leave;
    }

    /* Received an error message */
    packet = _getPacketAddrMsgqMsg (msgqMsg);
    rcmMsg = &packet->message;
    *rtnMsg = rcmMsg;

    /* Check the server status stored in the packet header */
    serverStatus = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                        RcmClient_Desc_TYPE_SHIFT);
    switch (serverStatus) {
    case RcmServer_Status_JobNotFound:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             rcmMsg->jobId,
                             "Job id not found");
        break;

    case RcmServer_Status_PoolNotFound:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             rcmMsg->poolId,
                             "Pool id not found");
        break;

    case RcmServer_Status_INVALID_FXN:
        status = RcmClient_E_INVALIDFXNIDX;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             rcmMsg->fxnIdx,
                             "Invalid function index");
        break;

    case RcmServer_Status_MSG_FXN_ERR:
        status = RcmClient_E_MSGFXNERROR;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             rcmMsg->result,
                             "Message function error");
        break;

    default:
        status = RcmClient_E_SERVERERROR;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_checkForError",
                             serverStatus,
                             "Server returned error");
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_checkForError", status);

    return status;
}


/*!
 *  @brief      Requests RCM server to execute remote function
 *
 *  @param      handle    Instance handle
 *  @param      cmdMsg    message for the RCM server
 *  @param    **returnMsg pointer to message ptr returned by server
 */
Int RcmClient_exec (RcmClient_Handle         handle,
                    RcmClient_Message      * cmdMsg,
                    RcmClient_Message     ** returnMsg)
{
    RcmClient_Packet  * packet;
    RcmClient_Message * rtnMsg;
    MessageQ_Msg        msgqMsg;
    UInt16              msgId;
    UInt16              serverStatus;
    Int                 rval;
    Int                 status          = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_exec", handle, cmdMsg, returnMsg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (cmdMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Command message passed is NULL!");
        goto leave;
    }
    if (returnMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Return message pointer passed is NULL!");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (cmdMsg);
    packet->desc |= (RcmClient_Desc_RCM_MSG << RcmClient_Desc_TYPE_SHIFT);
    msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_EXECFAILED;
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg (handle, msgId, &rtnMsg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Get return message failed");
        *returnMsg = NULL;
        goto leave;
    }
    *returnMsg = rtnMsg;

    /* Check the server's status stored in the packet header */
    packet = _RcmClient_getPacketAddr (rtnMsg);
    serverStatus = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                       RcmClient_Desc_TYPE_SHIFT);

    switch (serverStatus) {
    case RcmServer_Status_SUCCESS:
        break;

    case RcmServer_Status_INVALID_FXN:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             rtnMsg->fxnIdx,
                             "Invalid function index");
        status = RcmClient_E_INVALIDFXNIDX;
        goto leave;

    case RcmServer_Status_MSG_FXN_ERR:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             rtnMsg->result,
                             "Message function error");
        status = RcmClient_E_MSGFXNERROR;
        goto leave;

    default:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             serverStatus,
                             "Server returned error");
        status = RcmClient_E_SERVERERROR;
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_exec", status);

    return status;
}


/*!
 *  @brief      Requests RCM  server to execute remote function, it is
 *              asynchronous
 *
 *  @param      handle    Instance handle
 *  @param      cmdMsg    Message for the RCM server
 *  @param      callback  Callback function for return callback
 *  @param      appData   Not used at this time
 */
Int RcmClient_execAsync (RcmClient_Handle       handle,
                         RcmClient_Message    * cmdMsg,
                         RcmClient_CallbackFxn  callback,
                         Ptr                    appData)
{
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    Int                 rval;
    Int                 status = RcmClient_S_SUCCESS;

    GT_4trace (curTrace, GT_ENTER, "RcmClient_execAsync", handle, cmdMsg,
                callback, appData);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execAsync",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execAsync",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (cmdMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execAsync",
                             status,
                             "Command message passed is NULL!");
        goto leave;
    }

    /* Cannot use this function if callback notification is false */
    if (!handle->cbNotify) {
        status = RcmClient_E_EXECASYNCNOTENABLED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execAsync",
                             status,
                             "Asynchronous notification is not enabled!");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (cmdMsg);
    packet->desc |= (RcmClient_Desc_RCM_MSG << RcmClient_Desc_TYPE_SHIFT);

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execAsync",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_EXECFAILED;
        goto leave;
    }

    /* TODO finish this function */

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_execAsync", status);

    return status;
}


/*!
 *  @brief      Requests RCM  server to execute remote function, with no
 *              reply with call to another function to check for errors
 *
 *  @param      handle    Instance handle
 *  @param      msg       Message for the RCM server
 */
Int RcmClient_execCmd (RcmClient_Handle handle, RcmClient_Message * msg)
{
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    Int                 rval;
    Int                 status = RcmClient_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_execCmd", handle, msg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execCmd",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execCmd",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (msg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execCmd",
                             status,
                             "Command message passed is NULL!");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (msg);
    packet->desc |= (RcmClient_Desc_CMD << RcmClient_Desc_TYPE_SHIFT);

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->errorMsgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execCmd",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_IPCERROR;
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_execCmd", status);

    return status;
}


/*!
 *  @brief      Requests RCM server to execute remote function,does not
 *              wait for completion of remote function for reply
 *
 *  @param      handle    Instance handle
 *  @param      cmdMsg    Message for the RCM server
 *  @param    **returnMsg pointer to message ptr returned by server
 */
Int RcmClient_execDpc (RcmClient_Handle     handle,
                       RcmClient_Message  * cmdMsg,
                       RcmClient_Message ** returnMsg)
{
    RcmClient_Packet  * packet;
    RcmClient_Message * rtnMsg;
    MessageQ_Msg        msgqMsg;
    UInt16              msgId;
    Int                 rval;
    Int                 status = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_execDpc", handle, cmdMsg,
                returnMsg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execDpc",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execDpc",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (cmdMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execDpc",
                             status,
                             "Command message passed is NULL!");
        goto leave;
    }
    if (returnMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execDpc",
                             status,
                             "Return message pointer passed is NULL!");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (cmdMsg);
    packet->desc |= (RcmClient_Desc_DPC << RcmClient_Desc_TYPE_SHIFT);
    msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execDpc",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_EXECFAILED;
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg (handle, msgId, &rtnMsg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_exec",
                             status,
                             "Get return message failed");
        *returnMsg = NULL;
        goto leave;
    }
    *returnMsg = rtnMsg;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_execDpc", status);

    return status;
}


/*!
 *  @brief      Requests RCM server to execute remote function, provides
 *              a msgId to wait on later
 *
 *  @param      handle    Instance handle
 *  @param     *cmdMsg    Message for the RCM server
 *  @param     *msgId     Provides a message Id to check for errors
 */
Int RcmClient_execNoWait (RcmClient_Handle       handle,
                          RcmClient_Message    * cmdMsg,
                          UInt16               * msgId)
{
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    Int                 rval;
    Int                 status = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_execNoWait", handle, cmdMsg,
                msgId);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execNoWait",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execNoWait",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (cmdMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execNoWait",
                             status,
                             "Command message passed is NULL!");
        goto leave;
    }
    if (msgId == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execNoWait",
                             status,
                             "Message Id pointer passed is NULL!");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (cmdMsg);
    packet->desc |= (RcmClient_Desc_RCM_MSG << RcmClient_Desc_TYPE_SHIFT);
    *msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        *msgId = RcmClient_INVALIDMSGID;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_execNoWait",
                             status,
                             "Unable to send message to the server");
        status = RcmClient_E_EXECFAILED;
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_execNoWait", status);

    return status;
}


/*!
 *  @brief      Allocates memory for RCM message on heap, populates MessageQ
 *              and RCM message
 *
 *  @param      handle      Instance handle
 *  @param      msg      Free this message
 *
 *  @sa         RcmClient_free
 */
Int RcmClient_free (RcmClient_Handle handle, RcmClient_Message * msg)
{
    Int          rval;
    MessageQ_Msg msgqMsg;
    Int          status = RcmClient_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_free", handle, msg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_free",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_free",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (msg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_free",
                             status,
                             "Message passed is NULL!");
        goto leave;
    }

    msgqMsg = (MessageQ_Msg)_RcmClient_getPacketAddr (msg);
    rval = MessageQ_free (msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_free",
                             rval,
                             "MessageQ free failed");
        status = RcmClient_E_IPCERROR;
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_free", status);

    return status;
}


/*!
 *  @brief      Get symbol index for given name of the function
 *
 *  @param      handle      Instance handle
 *  @param      name     Name of the function
 *  @param     *index    Index value returned for the function
 */
Int RcmClient_getSymbolIndex (RcmClient_Handle      handle,
                              String                name,
                              UInt32              * index)
{
    Int                 len;
    RcmClient_Packet  * packet;
    UInt16              msgId;
    MessageQ_Msg        msgqMsg;
    Int                 rval;
    UInt16              serverStatus;
    RcmClient_Message * rcmMsg          = NULL;
    Int                 status          = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_getSymbolIndex", handle, name,
                index);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (name == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             status,
                             "Symbol name passed is NULL!");
        goto leave;
    }
    if (index == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             status,
                             "Index pointer passed is NULL!");
        goto leave;
    }
    *index = RcmClient_E_INVALIDFXNIDX;

    /* Allocate a message */
    len = String_len (name) + 1;
    status = RcmClient_alloc (handle, len, &rcmMsg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             status,
                             "Error allocating RCM message");
        goto leave;
    }

    /* Copy the function name into the message payload */
    rcmMsg->dataSize = len;  //TODO this is not proper!
    String_cpy ((Char *)rcmMsg->data, name);

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (rcmMsg);
    packet->desc |= (RcmClient_Desc_SYM_IDX << RcmClient_Desc_TYPE_SHIFT);
    msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_EXECFAILED;
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg (handle, msgId, &rcmMsg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             rval,
                             "Get return message failed");
        goto leave;
    }

    /* Extract return value and free message */
    packet = _RcmClient_getPacketAddr (rcmMsg);
    serverStatus = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                        RcmClient_Desc_TYPE_SHIFT);
    switch (serverStatus) {
    case RcmServer_Status_SUCCESS:
        break;

    case RcmServer_Status_SYMBOL_NOT_FOUND:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             serverStatus,
                             "Given symbol not found");
        status = RcmClient_E_SYMBOLNOTFOUND;
        goto leave;

    default:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_getSymbolIndex",
                             serverStatus,
                             "Server returned error");
        status = RcmClient_E_SERVERERROR;
        goto leave;
    }

    /* Extract return value */
    *index = rcmMsg->data[0];

leave:
    if (rcmMsg != NULL) {
        RcmClient_free (handle, rcmMsg);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_getSymbolIndex", status);

    return status;
}


/*!
 *  @brief      Release the jobId acquired from the RCM server
 *
 *  @param      handle       Instance pointer
 *  @param      jobId     jobId
 *
 *  @sa         RcmClient_acquireJobId
 */
Int RcmClient_releaseJobId (RcmClient_Handle handle, UInt16 jobId)
{
    RcmClient_Message * msg             = NULL;
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    UInt16              msgId;
    Int                 rval;
    UInt16              serverStatus;
    Int                 status          = RcmClient_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_releaseJobId", handle, jobId);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }

    /* Allocate a message */
    status = RcmClient_alloc (handle, sizeof(UInt16), &msg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             status,
                             "Error allocating RCM message");
        goto leave;
    }

    /* Classify this message */
    packet = _RcmClient_getPacketAddr (msg);
    packet->desc |= RcmClient_Desc_JOB_REL << RcmClient_Desc_TYPE_SHIFT;
    msgId = packet->msgId;

    /* Set the return address to this instance's message queue */
    msgqMsg = (MessageQ_Msg)packet;
    MessageQ_setReplyQueue (handle->msgQue, msgqMsg);

    /* Marshal the job id into the message payload */
    *(UInt16 *)(&msg->data[0]) = jobId;

    /* Send the message to the server */
    rval = MessageQ_put ((MessageQ_QueueId)handle->serverMsgQ, msgqMsg);
    if (rval < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             rval,
                             "Unable to send message to the server");
        status = RcmClient_E_FAIL;
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg (handle, msgId, &msg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             status,
                             "Get return message failed");
        goto leave;
    }

    /* Check message status for error */
    packet = _RcmClient_getPacketAddr (msg);
    serverStatus = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                        RcmClient_Desc_TYPE_SHIFT);

    switch (serverStatus) {
    case RcmServer_Status_SUCCESS:
        break;

    case RcmServer_Status_JobNotFound:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             jobId,
                             "Job ID not found");
        status = RcmClient_E_JOBIDNOTFOUND;
        break;

    default:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_releaseJobId",
                             serverStatus,
                             "Server error");
        status = RcmClient_E_SERVERERROR;
        break;
    }

leave:
    if (msg != NULL) {
        RcmClient_free (handle, msg);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmClient_releaseJobId", status);

    return status;
}


/*!
 *  @brief      Remove a symbol from RCM server (not implemented at this time)
 *
 *  @param      handle       Instance pointer
 *  @param      name      name of the symbol to be removed
 *
 *  @sa         RcmClient_getSymbolIndex
 */
Int RcmClient_removeSymbol (RcmClient_Handle handle, String name)
{
    Int status = RcmClient_E_NOTSUPPORTED;

    GT_2trace (curTrace, GT_ENTER, "RcmClient_removeSymbol", handle, name);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_removeSymbol",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_removeSymbol",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (name == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_removeSymbol",
                             status,
                             "Symbol name passed is NULL!");
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_removeSymbol", status);

    return status;
}


/*!
 *  @brief      Waits till invoked remote function completes return message
 *              will contain result and context
 *
 *  @param      handle       Instance pointer
 *  @param      msgId     message Id of message expected
 *  @param      returnMsg returned message
 */
Int RcmClient_waitUntilDone (RcmClient_Handle       handle,
                             UInt16                 msgId,
                             RcmClient_Message   ** returnMsg)
{
    RcmClient_Message * rtnMsg;
    Int                 status = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmClient_waitUntilDone", handle, msgId,
                returnMsg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_waitUntilDone",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_waitUntilDone",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (msgId == RcmClient_INVALIDMSGID) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_waitUntilDone",
                             status,
                             "Invalid msgId passed!");
        goto leave;
    }
    if (returnMsg == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_waitUntilDone",
                             status,
                             "Return message pointer passed is NULL!");
        goto leave;
    }

    /* Get the return message from the server */
    status = _RcmClient_getReturnMsg(handle, msgId, &rtnMsg);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmClient_waitUntilDone",
                             status,
                             "Get return message failed");
        *returnMsg = NULL;
        goto leave;
    }
    *returnMsg = rtnMsg;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmClient_waitUntilDone", status);

    return status;
}


/*!
 *  @brief      Generate unique message ID for RCM messages
 *
 *  @param      handle    Instance handle
 */
static
UInt16 _RcmClient_genMsgId (RcmClient_Object * handle)
{
    IArg    key;
    /* FIXME: (KW) ADD Check for msgID  = 0 in the calling function */
    UInt16  msgId   = 0;
    Int     status  = RcmClient_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "_RcmClient_genMsgId", handle);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_genMsgId",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_genMsgId",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }

    key = IGateProvider_enter (handle->gate);
    msgId = (handle->msgId == 0xFFFF ? handle->msgId = 1 : ++(handle->msgId));
    IGateProvider_leave (handle->gate, key);

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmClient_genMsgId", status);

    return msgId;
}


/*!
 *  @brief      A thread safe algorithm for message delivery
 *              This function is called to pickup a specified return message
 *              from the server. messages are taken from the queue and either
 *              delivered to the mailbox if not the specified message or
 *              returned to the caller. The calling thread might play the role
 *              of mailman or simply wait on a list of recipients.
 *
 *              This algorithm guarantees that a waiting recipient is released
 *              as soon as his message arrives. All new mail is placed in the
 *              mailbox until the recipient arrives to collect it. The messages
 *              can arrive in any order and will be processed as they arrive.
 *              message delivery is never stalled waiting on absent recipient.
 *
 *  @param      handle     Instance handle
 *  @param      msgId      Message expected from the RCM server
 *  @param    **returnMsg  Return message ptr from the RCM server
 */
static
Int _RcmClient_getReturnMsg (RcmClient_Object     * handle,
                             const UInt16           msgId,
                             RcmClient_Message   ** returnMsg)
{
    List_Elem         * elem;
    Recipient         * recipient;
    RcmClient_Packet  * packet;
    Bool                messageDelivered;
    MessageQ_Msg        msgqMsg             = NULL;
    Bool                messageFound        = FALSE;
    Int                 queueLockAcquired   = 0;
    Int                 rval;
    Int                 status              = RcmClient_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_RcmClient_getReturnMsg", handle, msgId,
                returnMsg);

    if (RcmClient_module->setupRefCount == 0) {
        status = RcmClient_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_getReturnMsg",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmClient_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmClient_getReturnMsg",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }

    *returnMsg = NULL;

    /* Keep trying until message found */
    while (!messageFound) {

        /* Acquire the mailbox lock */
        rval = OsalSemaphore_pend (handle->mbxLock, OSALSEMAPHORE_WAIT_FOREVER);
        if (rval < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmClient_getReturnMsg",
                                 rval,
                                 "handle->mbxLock pend failed");
            status = RcmClient_E_FAIL;
            goto leave;
        }

        /* Search new mail list for message */
        elem = NULL;
        while ((elem = List_next (handle->newMail, elem)) != NULL) {
            packet = _getPacketAddrElem (elem);
            if (msgId == packet->msgId) {
                List_remove (handle->newMail, elem);
                *returnMsg = &packet->message;
                messageFound = TRUE;
                break;
            }
        }

        if (messageFound) {
            /* Release the mailbox lock */
            rval = OsalSemaphore_post (handle->mbxLock);
            if (rval < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmClient_getReturnMsg",
                                     rval,
                                     "handle->mbxLock post failed");
                status = RcmClient_E_FAIL;
                goto leave;
            }
        }
        else {
            /* Attempt the message queue lock */
            queueLockAcquired = OsalSemaphore_pend (handle->queueLock,
                                        OSALSEMAPHORE_WAIT_NONE);
            if ((queueLockAcquired < 0) &&
                (queueLockAcquired != OSALSEMAPHORE_E_WAITNONE)) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmClient_getReturnMsg",
                                     status,
                                     "handle->queueLock failed");
                status = RcmClient_E_FAIL;
                goto leave;
            }

            if (! (queueLockAcquired < 0)) {
                /*
                 * MailMan role
                 */

                /* Deliver new mail until message found */
                while (!messageFound) {
                    /* Get message from queue if available (non-blocking) */
                    if (NULL == msgqMsg) {
                        rval = MessageQ_get (handle->msgQue, &msgqMsg,
                                                WAIT_NONE);
                        if ((rval != MessageQ_E_TIMEOUT) && (rval < 0)) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmClient_getReturnMsg",
                                                 rval,
                                                 "handle->MessageQ get failed");
                            status = RcmClient_E_LOSTMSG;
                            goto leave;
                        }
                    }

                    while (NULL != msgqMsg) {
                        /* Check if message found */
                        packet = _getPacketAddrMsgqMsg (msgqMsg);
                        messageFound = (msgId == packet->msgId);
                        if (messageFound) {
                            *returnMsg = &packet->message;

                            /* Search wait list for new mailman */
                            elem = NULL;
                            while ((elem =
                                List_next (handle->recipients, elem)) != NULL) {
                                recipient = (Recipient *)elem;
                                if (NULL == recipient->msg) {
                                    rval = OsalSemaphore_post(recipient->event);
                                    if (rval < 0) {
                                        GT_setFailureReason (curTrace,
                                                             GT_4CLASS,
                                                             "RcmClient_"
                                                             "getReturnMsg",
                                                             rval,
                                                             "recipient->event "
                                                             "post failed");
                                        status = RcmClient_E_FAIL;
                                        goto leave;
                                    }
                                    break;
                                }
                            }

                            /* Release the message queue lock */
                            rval = OsalSemaphore_post (handle->queueLock);
                            if (rval < 0) {
                                GT_setFailureReason (curTrace,
                                                     GT_4CLASS,
                                                     "_RcmClient_getReturnMsg",
                                                     rval,
                                                     "handle->queueLock post "
                                                     "failed");
                                status = RcmClient_E_FAIL;
                                goto leave;
                            }

                            /* Release the mailbox lock */
                            rval = OsalSemaphore_post (handle->mbxLock);
                            if (rval < 0) {
                                GT_setFailureReason (curTrace,
                                                     GT_4CLASS,
                                                     "_RcmClient_getReturnMsg",
                                                     rval,
                                                     "handle->mbxLock post "
                                                     "failed");
                                status = RcmClient_E_FAIL;
                                goto leave;
                            }
                            break;
                        }
                        else {
                            /*
                             * Deliver message to mailbox
                             */

                            /* Search recipient list for message owner */
                            elem = NULL;
                            messageDelivered = FALSE;
                            while ((elem =
                                List_next (handle->recipients, elem)) != NULL) {
                                recipient = (Recipient *)elem;
                                if (recipient->msgId == packet->msgId) {
                                    recipient->msg = &packet->message;
                                    rval = OsalSemaphore_post(recipient->event);
                                    if (rval < 0) {
                                        GT_setFailureReason (curTrace,
                                                             GT_4CLASS,
                                                             "RcmClient_"
                                                             "getReturnMsg",
                                                             rval,
                                                             "recipient->event "
                                                             "post failed");
                                        status = RcmClient_E_FAIL;
                                        goto leave;
                                    }
                                    messageDelivered = TRUE;
                                    break;
                                }
                            }

                            /* Add undelivered message to new mail list */
                            if (!messageDelivered) {
                                /* Use the elem in the MessageQ hdr */
                                elem = (List_Elem *)&packet->msgqHeader;
                                List_put (handle->newMail, elem);
                            }
                        }

                        /* Get next message from queue if available */
                        rval = MessageQ_get (handle->msgQue, &msgqMsg,
                                                WAIT_NONE);
                        if ((rval != MessageQ_E_TIMEOUT) && (rval < 0)) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmClient_getReturnMsg",
                                                 rval,
                                                 "handle->MessageQ get failed");
                            status = RcmClient_E_LOSTMSG;
                            goto leave;
                        }
                    }

                    if (!messageFound) {
                        /*
                         * Message queue empty
                         */

                        /* Release the mailbox lock */
                        rval = OsalSemaphore_post (handle->mbxLock);
                        if (rval < 0) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmClient_getReturnMsg",
                                                 rval,
                                                 "uhandle->mbxLock post failed");
                            status = RcmClient_E_FAIL;
                            goto leave;
                        }

                        /* Get next message, this blocks the thread */
                        rval = MessageQ_get (handle->msgQue, &msgqMsg,
                                                MessageQ_FOREVER);
                        if (rval < 0) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmClient_getReturnMsg",
                                                 rval,
                                                 "handle->MessageQ get failed");
                            status = RcmClient_E_LOSTMSG;
                            goto leave;
                        }

                        if (msgqMsg == NULL) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmClient_getReturnMsg",
                                                 rval,
                                                 "Message received is NULL");
                            status = RcmClient_E_LOSTMSG;
                            goto leave;
                        }

                        /* Acquire the mailbox lock */
                        rval = OsalSemaphore_pend (handle->mbxLock,
                                                   OSALSEMAPHORE_WAIT_FOREVER);
                        if (rval < 0) {
                            GT_setFailureReason (curTrace,
                                                GT_4CLASS,
                                                "_RcmClient_getReturnMsg",
                                                rval,
                                                "handle->queueLock fails");
                            status = RcmClient_E_FAIL;
                        }
                    }
                }
            }
            else {
                /* Construct recipient on local stack */
                Recipient self;
                self.msgId = msgId;
                self.msg = NULL;
                self.event = OsalSemaphore_create (OsalSemaphore_Type_Counting,
                                    0);
                if (self.event ==  NULL) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         RcmClient_E_FAIL,
                                         "Thread event construct fails");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }

                /* Add recipient to wait list */
                elem = &self.elem;
                List_put (handle->recipients, elem);

                /* Release the mailbox lock */
                rval = OsalSemaphore_post (handle->mbxLock);
                if (rval < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "handle->mbxLock post fails");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }

                /* Wait on event */
                rval = OsalSemaphore_pend (self.event,
                                            OSALSEMAPHORE_WAIT_FOREVER);
                if (rval < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "Thread event pend fails");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }

                /* Acquire the mailbox lock */
                rval = OsalSemaphore_pend (handle->mbxLock,
                                            OSALSEMAPHORE_WAIT_FOREVER);
                if (rval < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "handle->mbxLock pend fails");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }

                if (NULL != self.msg) {
                    /* Pickup message */
                    *returnMsg = self.msg;
                    messageFound = TRUE;
                }

                /* Remove recipient from wait list */
                List_remove (handle->recipients, elem);
#ifdef HAVE_ANDROID_OS
                /* Android bionic Semdelete code returns -1 if count == 0 */
                rval = OsalSemaphore_post (self.event);
                if (rval < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "self.event post fails");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }
#endif /* ifdef HAVE_ANDROID_OS */
                rval = OsalSemaphore_delete (&(self.event));
                if (rval < 0){
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "Thread event delete failed");
                    status = RcmClient_E_FAIL;
                    goto leave;
                }

                /* Release the mailbox lock */
                rval = OsalSemaphore_post (handle->mbxLock);
                if (rval < 0){
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmClient_getReturnMsg",
                                         rval,
                                         "handle->mbxLock post fails");
                    status = RcmClient_E_FAIL;
                }
            }
        }
    } /* while (!messageFound) */

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmClient_getReturnMsg", status);

    return status;
}


/*!
 *  @brief      Get packet addres from RCM message
 */
static
RcmClient_Packet * _RcmClient_getPacketAddr (RcmClient_Message * msg)
{
    Int offset = (Int)&(((RcmClient_Packet *)0)->message);

    return ((RcmClient_Packet *)((Char *)msg - offset));
}


/*!
 *  @brief      Get packet addres from MessageQ message
 */
static
RcmClient_Packet * _getPacketAddrMsgqMsg (MessageQ_Msg msg)
{
    Int offset = (Int)&(((RcmClient_Packet *)0)->msgqHeader);

    return ((RcmClient_Packet *)((Char *)msg - offset));
}


/*!
 *  @brief      Get packet address from List element
 */
static
RcmClient_Packet * _getPacketAddrElem (List_Elem * elem)
{
    Int offset = (Int)&(((RcmClient_Packet *)0)->msgqHeader);

    return ((RcmClient_Packet *)((Char *)elem - offset));
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
