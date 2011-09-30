/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2010, Texas Instruments Incorporated
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
 *  @file   RcmServer.c
 *
 *  @brief  Remote Command Message Server Module.
 *
 *          The RcmServer processes inbound messages received from an RcmClient.
 *          After processing a message, the server will return the message to
 *          the client.
 *
 *  ============================================================================
 */

/* Standard headers */
#include <host_os.h>
#include <pthread.h>
#include <sched.h>

/* Utility and IPC headers */
#include <Std.h>
#include <Trace.h>
#include <ti/ipc/MessageQ.h>
#include <GateMutex.h>
#include <OsalSemaphore.h>
#include <Memory.h>
#include <List.h>
#include <String.h>

/* Module level headers */
#include <RcmTypes.h>
#include <RcmServer.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * RCM Server Macros
 * =============================================================================
 */
#define RCMSERVER_MAX_TABLES        8
#define RCMSERVER_POOL_MAP_LEN      4
#define WAIT_FOREVER                0xFFFFFFFF
#define WAIT_NONE                   0x0
#define MAX_NAME_LEN                32
#define RCMSERVER_DESC_TYPE_MASK    0x0F00      /* Field mask */
#define RCMSERVER_DESC_TYPE_SHIFT   8           /* Field shift width */

#define _RCM_KeyResetValue          0x07FF      /* Key reset value */
#define _RCM_KeyMask                0x7FF00000  /* Key mask in function index */
#define _RCM_KeyShift               20  /* Key bit position in function index */

#define RcmServer_E_InvalidFxnIdx   (-101)
#define RcmServer_E_JobIdNotFound   (-102)
#define RcmServer_E_PoolIdNotFound  (-103)


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/* function table element */
typedef struct RcmServer_FxnTabElem_tag {
    String                      name;
    RcmServer_MsgFxn            addr;
    UInt16                      key;
} RcmServer_FxnTabElem;

/* Array of RcmServer_FxnTabElems */
typedef struct RcmServer_FxnTabElemAry_tag {
    Int                         length;
    RcmServer_FxnTabElem *      elem;
} RcmServer_FxnTabElemAry;

/* RcmServer worker thread pool object */
typedef struct RcmServer_ThreadPool_tag {
    String                      name;       /* Pool name */
    Int                         count;      /* Thread count (at create time) */
    Int                         priority;   /* Thread priority */
    Int                         osPriority; /* OS-specific thread priority */
    SizeT                       stackSize;  /* Thread stack size */
    String                      stackSeg;   /* Thread stack placement */
    OsalSemaphore_Handle        sem;        /* Message semaphore (counting) */
    List_Object                 threadList; /* List of worker threads */
    List_Object                 readyQueue; /* Queue of messages */
    IGateProvider_Handle        readyQueueGate; /* message queue list gate */
} RcmServer_ThreadPool;

/* RCM Server instance object structure */
typedef struct RcmServer_Object_tag {
    IGateProvider_Handle     gate;         /* Message id gate */
    OsalSemaphore_Handle     run;          /* Synch for starting RCM server */
    MessageQ_Handle          serverQue;    /* Inbound message queue */
    pthread_t                serverThread; /* Server thread object */
    RcmServer_FxnTabElemAry  fxnTabStatic; /* Static function table */
    RcmServer_FxnTabElem *   fxnTab [RCMSERVER_MAX_TABLES];
                                           /* Function table base pointers */
    UInt16                   key;          /* Function index key */
    UInt16                   jobId;        /* Job id tracker */
    Bool                     shutdown;     /* Signal shutdown by application */
    Int                      poolMap0Len;  /* Length of static table */
    RcmServer_ThreadPool *   poolMap [RCMSERVER_POOL_MAP_LEN];
    List_Handle              jobList;      /* List of job stream queues */
    IGateProvider_Handle     jobListGate;  /* Job stream queue gate */
} RcmServer_Object;

/* RCM Worker Thread object structure */
typedef struct {
    List_Elem                   elem;
    UInt16                      jobId;      /* Current job stream id */
    pthread_t                   thread;     /* Server thread object */
    Bool                        terminate;  /* Thread terminate flag */
    RcmServer_ThreadPool *      pool;       /* Worker pool */
    RcmServer_Object *          server;     /* Server instance */
} RcmServer_WorkerThread;

/* RCM Job Stream object structure */
typedef struct {
    List_Elem                   elem;
    UInt16                      jobId;      /* Job stream id */
    Bool                        empty;      /* True if no messages on server */
    List_Object                 msgQue;     /* Queue of messages */
    IGateProvider_Handle        msgQueGate; /* Queue of messages gate */
} RcmServer_JobStream;

/* structure for RcmServer module state */
typedef struct RcmServer_ModuleObject_tag {
    String             name;
    IHeap_Handle       heap;
    Int32              setupRefCount;    /* Ref. cnt for no. of times setup or*/
                                         /* destroy was called in this process*/
    RcmServer_Config   defaultCfg;       /* Default config values */
    RcmServer_Params   defaultInstParams; /* Default instance create params */
} RcmServer_ModuleObject;


/* =============================================================================
 *  Private functions
 * =============================================================================
 */
static Int _RcmServer_Instance_init (RcmServer_Object         * obj,
                                     String                     name,
                                     const RcmServer_Params   * params);

static Int _RcmServer_Instance_finalize (RcmServer_Object * obj);

static Int _RcmServer_acqJobId (RcmServer_Object * obj, UInt16 * jobIdPtr);

static Int _RcmServer_dispatch (RcmServer_Object  * obj,
                                RcmClient_Packet  * packet);

static Int _RcmServer_execMsg (RcmServer_Object * obj, RcmClient_Message * msg);

static Int _RcmServer_getFxnAddr (RcmServer_Object    * obj,
                                  UInt32                fxnIdx,
                                  RcmServer_MsgFxn    * addrPtr);

static UInt16 _RcmServer_getNextKey (RcmServer_Object * obj);

static Int _RcmServer_getSymIdx (RcmServer_Object * obj,
                                 String             name,
                                 UInt32           * index);

static Int _RcmServer_getPool (RcmServer_Object       * obj,
                               RcmClient_Packet       * packet,
                               RcmServer_ThreadPool  ** poolP);

static Void _RcmServer_process (RcmServer_Object  * obj,
                                RcmClient_Packet  * packet);

static Int _RcmServer_relJobId (RcmServer_Object * obj, UInt16 jobId);

static Void _RcmServer_serverThrFxn (IArg arg);

static Void _RcmServer_setStatusCode (RcmClient_Packet * packet, UInt16 code);

static Void _RcmServer_workerThrFxn (IArg arg);

#define RcmServer_Module_heap() (NULL)


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*
 *  @var    RcmServer_state
 *
 *  @brief  RcmServer state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
RcmServer_ModuleObject RcmServer_module_obj =
{
    .defaultCfg.maxNameLen                  = MAX_NAME_LEN,
    .defaultCfg.maxTables                   = RCMSERVER_MAX_TABLES,
    .defaultCfg.poolMapLen                  = RCMSERVER_POOL_MAP_LEN,
    .defaultInstParams.priority             = RCMSERVER_REGULAR_PRIORITY,
    .defaultInstParams.osPriority           = RCMSERVER_INVALID_OS_PRIORITY,
    .defaultInstParams.stackSize            = 0,
    .defaultInstParams.stackSeg             = "",
    .defaultInstParams.workerPools.length   = 0,   /* worker pools */
    .defaultInstParams.workerPools.elem     = NULL,
    .defaultInstParams.fxns.length          = 0,   /* function table */
    .defaultInstParams.fxns.elem            = NULL,

    .setupRefCount                          = 0
};

/*
 *  @var    RcmServer_module
 *
 *  @brief  Pointer to RcmServer_module_obj .
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
RcmServer_ModuleObject * RcmServer_module = &RcmServer_module_obj;


/*
 *  ======== RcmServer_init ========
 *
 *  This function must be serialized by the caller
 */
Void
RcmServer_init (Void)
{
    Int status = RcmServer_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmServer_init");

    /* TBD: Protect from multiple threads. */
    RcmServer_module->setupRefCount++;

    /* This is needed at runtime so should not be in
     * SYSLINK_BUILD_OPTIMIZE.
     */
    if (RcmServer_module->setupRefCount > 1) {
        status = RcmServer_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "RcmServer module has been already setup in this process.\n"
                   " RefCount: [%d]\n",
                   RcmServer_module->setupRefCount);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmServer_init", status);
}


/*
 *  ======== RcmServer_exit ========
 *
 *  This function must be serialized by the caller
 */
Void
RcmServer_exit (Void)
{
    Int status = RcmServer_S_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "RcmServer_exit");

    /* TBD: Protect from multiple threads. */
    RcmServer_module->setupRefCount--;

    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (RcmServer_module->setupRefCount < 0) {
        status = RcmServer_S_ALREADYCLEANEDUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "RcmServer module has been already been cleaned up in this"
                   " process.\n    RefCount: [%d]\n",
                   RcmServer_module->setupRefCount);
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmServer_exit", status);
}


/*
 *  ======== RcmServer_Params_init ========
 */
Int
RcmServer_Params_init (RcmServer_Params * params)
{
    Int status = RcmServer_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "RcmServer_Params_init", params);
    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_Params_init",
                             status,
                             "Module is in an invalid state!");
    }
    else if (params == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_Params_init",
                             status,
                             "Argument of type (RcmServer_Params *) passed "
                             "is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        params->priority = RcmServer_module->defaultInstParams.priority;

        /* Server thread */
        params->priority = RCMSERVER_REGULAR_PRIORITY;
        params->osPriority = RCMSERVER_INVALID_OS_PRIORITY;
        params->stackSize = 0;  /* use system default */
        params->stackSeg = "";

        /* Default pool */
        params->defaultPool.name = NULL;
        params->defaultPool.count = 0;
        params->defaultPool.priority = RCMSERVER_REGULAR_PRIORITY;
        params->defaultPool.osPriority = RCMSERVER_INVALID_OS_PRIORITY;
        params->defaultPool.stackSize = 0;  /* use system default */
        params->defaultPool.stackSeg = "";

        /* Worker pools */
        params->workerPools.length = 0;
        params->workerPools.elem = NULL;

        /* Function table */
        params->fxns.length = 0;
        params->fxns.elem = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "RcmServer_Params_init", status);

    return status;
}


/*
 *  ======== RcmServer_create ========
 */
Int
RcmServer_create (String                name,
                  RcmServer_Params    * params,
                  RcmServer_Handle    * handle)
{
    RcmServer_Object  * obj;
    Int                 status = RcmServer_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "RcmServer_create", name, params, handle);

    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (params == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Argument of type (RcmServer_Params *) passed "
                             "is NULL!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Handle passed is NULL!");
        goto leave;
    }
    if (name == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Name passed is NULL!");
        goto leave;
    }
    if (String_len (name) > RcmServer_module->defaultCfg.maxNameLen) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Server name too long");
        goto leave;
    }

    /* Initialize the handle ptr */
    *handle = (RcmServer_Handle)NULL;

    /* Allocate the object */
    obj = (RcmServer_Object *)Memory_calloc (RcmServer_Module_heap(),
                                             sizeof (RcmServer_Object), 0);
    if (obj == NULL) {
        status = RcmServer_E_NOMEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_create",
                             status,
                             "Memory allocation failed for handle!");
        goto leave;
    }

    /* Object-specific initialization */
    status = _RcmServer_Instance_init (obj, name, params);
    if (status < 0) {
        _RcmServer_Instance_finalize (obj);
        Memory_free (NULL, obj, sizeof (RcmServer_Object));
        status = RcmServer_E_FAIL;
        goto leave;
    }

    /* Success, return opaque pointer */
    *handle = (RcmServer_Handle)obj;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmServer_create", status);

    return status;
}


/*
 *  ======== _RcmServer_Instance_init ========
 */
static Int
_RcmServer_Instance_init (RcmServer_Object        * obj,
                          String                    name,
                          const RcmServer_Params  * params)
{
    List_Params                 listP;
    MessageQ_Params             mqParams;
    pthread_attr_t              threadP;
    struct sched_param          schedParam;
    Int                         i;
    Int                         j;
    SizeT                       size;
    Char                      * cp;
    RcmServer_ThreadPool      * poolAry;
    RcmServer_WorkerThread    * worker;
    List_Handle                 listH;
    Int                         status      = RcmServer_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_RcmServer_Instance_init", obj, name,
                params);

    /* Initialize instance state */
    obj->shutdown            = FALSE;
    obj->key                 = 0;
    obj->jobId               = 0xFFFF;
    obj->run                 = NULL;
    obj->serverQue           = NULL;
    obj->serverThread        = 0;
    obj->fxnTabStatic.length = 0;
    obj->fxnTabStatic.elem   = NULL;
    obj->poolMap0Len         = 0;
    obj->jobList             = NULL;

    /* Initialize the function table */
    for (i = 0; i < RcmServer_module->defaultCfg.maxTables; i++) {
        obj->fxnTab [i] = NULL;
    }

    /* Initialize the worker pool map */
    for (i = 0; i < RcmServer_module->defaultCfg.poolMapLen; i++) {
        obj->poolMap [i] = NULL;
    }

    /* Create the instance gate */
    obj->gate = (IGateProvider_Handle) GateMutex_create ();
    GT_assert (curTrace, (obj->gate != NULL));
    if (obj->gate == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create mutex!");
        goto leave;
    }

    /* Create list for job objects */
    List_Params_init (&listP);
    obj->jobListGate = (IGateProvider_Handle) GateMutex_create ();
    GT_assert (curTrace, (obj->jobListGate != NULL));
    if (obj->jobListGate == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create mutex!");
        goto leave;
    }
    listP.gateHandle = obj->jobListGate;
    obj->jobList = List_create (&listP);
    GT_assert (curTrace, (obj->jobList != NULL));
    if (obj->jobList == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create jobList!");
        goto leave;
    }

    /* Create the static function table */
    if (params->fxns.length > 0) {
        obj->fxnTabStatic.length = params->fxns.length;

        /* Allocate static function table */
        size = params->fxns.length * sizeof (RcmServer_FxnTabElem);
        obj->fxnTabStatic.elem = Memory_calloc (RcmServer_Module_heap(),
                                                    size, 0);
        if (obj->fxnTabStatic.elem == NULL) {
            status = RcmServer_E_NOMEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_init",
                                 status,
                                 "Memory allocation failed for function "
                                 "array!");
            goto leave;
        }
        obj->fxnTabStatic.elem [0].name = NULL;

        /* Allocate a single block to store all name strings */
        for (size = 0, i = 0; i < params->fxns.length; i++) {
            size += String_len (params->fxns.elem [i].name) + 1;
        }

        cp = Memory_calloc ( RcmServer_Module_heap(), size, 0);
        if (cp == NULL) {
            status = RcmServer_E_NOMEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_init",
                                 status,
                                 "Memory allocation failed for function "
                                 "names!");
            goto leave;
        }

        /* Copy function table data into allocated memory blocks */
        for (i = 0; i < params->fxns.length; i++) {
            String_cpy (cp, params->fxns.elem [i].name);
            obj->fxnTabStatic.elem [i].name = cp;
            cp += (String_len (params->fxns.elem [i].name) + 1);
            obj->fxnTabStatic.elem [i].addr = params->fxns.elem [i].addr;
            obj->fxnTabStatic.elem [i].key = 0;
        }

        /* Hook up the static function table */
        obj->fxnTab [0] = obj->fxnTabStatic.elem;
    }

    /* Create static worker pools */
    if ((params->workerPools.length + 1) > RCMSERVER_POOL_MAP_LEN) {
        status = RcmServer_E_NOMEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Exceeded max allowed worker pools!");
        goto leave;
    }
    obj->poolMap0Len = params->workerPools.length + 1; /* worker + default */

    /* Allocate the static worker pool table */
    size = obj->poolMap0Len * sizeof (RcmServer_ThreadPool);
    obj->poolMap [0] = (RcmServer_ThreadPool *) Memory_calloc (
                                RcmServer_Module_heap(), size, sizeof (Ptr));
    if (obj->poolMap [0] == NULL) {
        status = RcmServer_E_NOMEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Memory allocation failed for thread pool array!");
        goto leave;
    }

    /* Convenient alias */
    poolAry = obj->poolMap [0];

    /* Allocate a single block to store all name strings */
    size = sizeof (SizeT) /* block size */
            + (params->defaultPool.name == NULL ? 1 :
                String_len (params->defaultPool.name) + 1);

    for (i = 0; i < params->workerPools.length; i++) {
        size += (params->workerPools.elem [i].name == NULL ? 0 :
                    String_len (params->workerPools.elem [i].name) + 1);
    }

    cp = Memory_calloc (RcmServer_Module_heap(), size, sizeof (Ptr));
    if (cp == NULL) {
        status = RcmServer_E_NOMEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Memory allocation failed for worker pool names!");
        goto leave;
    }

    *(SizeT *)cp = size;
    cp += sizeof (SizeT);

    /* Initialize the default worker pool, poolAry [0] */
    if (params->defaultPool.name != NULL) {
        String_cpy (cp, params->defaultPool.name);
        poolAry [0].name = cp;
        cp += (String_len (params->defaultPool.name) + 1);
    }
    else {
        poolAry [0].name = cp;
        *cp++ = '\0';
    }

    poolAry [0].count      = params->defaultPool.count;
    poolAry [0].priority   = params->defaultPool.priority;
    poolAry [0].osPriority = params->defaultPool.osPriority;
    poolAry [0].stackSize  = params->defaultPool.stackSize;
    poolAry [0].stackSeg   = NULL;   /* TODO */
    poolAry [0].sem        = NULL;

    /* ThreadList is static, no gate protection required */
    List_construct (&(poolAry [0].threadList), NULL);

    poolAry [0].readyQueueGate = (IGateProvider_Handle) GateMutex_create ();
    GT_assert (curTrace, (poolAry [0].readyQueueGate != NULL));
    if (poolAry [0].readyQueueGate == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create mutex!");
        goto leave;
    }
    listP.gateHandle = poolAry [0].readyQueueGate;
    List_construct (&(poolAry [0].readyQueue), &listP);

    poolAry [0].sem = OsalSemaphore_create (OsalSemaphore_Type_Counting, 0);
    if (poolAry [0].sem ==  NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create semaphore for default pool!");
        goto leave;
    }

    /* Initialize the static worker pools, poolAry [1..(n-1)] */
    for (i = 0; i < params->workerPools.length; i++) {
        if (params->workerPools.elem [i].name != NULL) {
            String_cpy (cp, params->workerPools.elem [i].name);
            poolAry [i+1].name = cp;
            cp += (String_len (params->workerPools.elem [i].name) + 1);
        }
        else {
            poolAry [i+1].name = NULL;
        }

        poolAry [i+1].count      = params->workerPools.elem [i].count;
        poolAry [i+1].priority   = params->workerPools.elem [i].priority;
        poolAry [i+1].osPriority = params->workerPools.elem [i].osPriority;
        poolAry [i+1].stackSize  = params->workerPools.elem [i].stackSize;
        poolAry [i+1].stackSeg   = NULL;  /* TODO */

        /* ThreadList is static, no gate protection required */
        List_construct (&(poolAry [i+1].threadList), NULL);

        poolAry [i+1].readyQueueGate =
                    (IGateProvider_Handle) GateMutex_create ();
        GT_assert (curTrace, (poolAry [i+1].readyQueueGate != NULL));
        if (poolAry [0].readyQueueGate == NULL) {
            status = RcmServer_E_FAIL;
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create mutex!");
            goto leave;
        }
        listP.gateHandle = poolAry [i+1].readyQueueGate;
        List_construct (&(poolAry [i+1].readyQueue), &listP);

        /* Create the run synchronizer */
        poolAry [i+1].sem  = OsalSemaphore_create (OsalSemaphore_Type_Counting,
                                                    0);
        if (poolAry [i+1].sem == NULL) {
            status = RcmServer_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_init",
                                 status,
                                 "Unable to create sync for RCM pool threads!");
            goto leave;
        }
    }

    /* Create the worker threads in each static pool */
    for (i = 0; i < obj->poolMap0Len; i++) {
        for (j = 0; j < poolAry [i].count; j++) {
            /* Allocate worker thread object */
            size = sizeof (RcmServer_WorkerThread);
            worker = Memory_calloc (RcmServer_Module_heap(), size,
                                        sizeof (Ptr));
            if (worker == NULL) {
                status = RcmServer_E_NOMEMORY;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_Instance_init",
                                     status,
                                     "Memory allocation failed for worker "
                                     "thread!");
                goto leave;
            }

            /* Initialize worker thread object */
            worker->jobId     = RcmClient_DISCRETEJOBID;
            worker->thread    = 0;
            worker->terminate = FALSE;
            worker->pool      = &(poolAry [i]);
            worker->server    = obj;

            /* add worker thread to worker pool */
            listH = &(poolAry [i].threadList);
            List_putHead (listH, &(worker->elem));

            /* Create worker thread */
            pthread_attr_init (&threadP);
            pthread_attr_getschedparam (&threadP, &schedParam);
            if (poolAry [i].priority > 0) {
                schedParam.sched_priority += 1;
            }
            else if (poolAry [i].priority < 0) {
                schedParam.sched_priority -= 1;
            }
            pthread_attr_setschedparam (&threadP, &schedParam);

            status = pthread_create (&(worker->thread), &threadP,
                                     (Void *) &_RcmServer_workerThrFxn,
                                     (Void *) worker);
            if (status < 0) {
                status = RcmServer_E_FAIL;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_Instance_init",
                                     status,
                                     "Could not create worker thread!");
                goto leave;
            }
        }
    }

    /* Create the semaphore used to release the server thread */
    obj->run = OsalSemaphore_create (OsalSemaphore_Type_Binary, 0);
    if (obj->run == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create run semaphore for RCM server "
                             "thread!");
        goto leave;
    }

    /* Create the message queue for inbound messages */
    MessageQ_Params_init (&mqParams);
    obj->serverQue = MessageQ_create (name, &mqParams);
    if (NULL == obj->serverQue) {
        status = RcmServer_E_MSGQCREATEFAILED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create MessageQ");
        goto leave;
    }

    /* Create the server thread */
    pthread_create (&(obj->serverThread), NULL,
                    (Void *)&_RcmServer_serverThrFxn, (Void *)obj);

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_Instance_init", status);

    return status;
}


/*
 *  ======== RcmServer_delete ========
 */
Int
RcmServer_delete (RcmServer_Handle * handlePtr)
{
    RcmServer_Object  * obj     = NULL;
    Int                 status  = RcmServer_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "RcmServer_delete", handlePtr);

    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_delete",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handlePtr == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_delete",
                             status,
                             "handlePtr pointer passed is NULL!");
        goto leave;
    }
    if (*handlePtr == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_delete",
                             status,
                             "*handlePtr passed is NULL!");
        goto leave;
    }

    /* Finalize the object */
    obj = (RcmServer_Object *)(*handlePtr);
    status = _RcmServer_Instance_finalize (obj);

    /* TODO: Should the object be freed if finalize returns an error? */

    /* Free the object memory */
    Memory_free (RcmServer_Module_heap(), (RcmServer_Object *)*handlePtr,
                    sizeof (RcmServer_Object));
    *handlePtr = NULL;

leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmServer_delete", status);

    return status;
}


/*
 *  ======== _RcmServer_Instance_finalize ========
 */
static Int
_RcmServer_Instance_finalize (RcmServer_Object * obj)
{
    Int                         i;
    Int                         j;
    Int                         size;
    Char                      * cp;
    UInt                        tabCount;
    RcmServer_FxnTabElem      * fdp;
    RcmServer_ThreadPool      * poolAry;
    RcmServer_WorkerThread    * worker;
    List_Elem                 * elem;
    List_Handle                 listH;
    List_Handle                 msgQueH;
    RcmClient_Packet          * packet;
    MessageQ_Msg                msgqMsg;
    RcmServer_JobStream       * job;
    Int                         rval;
    Int                         status  = RcmClient_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "_RcmServer_Instance_finalize", obj);

    /* Block until server thread exits */
    obj->shutdown = TRUE;

    if (obj->serverThread != 0) {
        status = MessageQ_unblock (obj->serverQue);
        if (status < 0) {
            status = RcmServer_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 status,
                                 "MessageQ_unblock failed!");
            goto leave;
        }
        status = pthread_join (obj->serverThread, NULL);
        if (status < 0) {
            status = RcmServer_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 status,
                                 "Server thread did not exit properly!");
            goto leave;
        }
    }

    /* Delete any remaining job objects (there should not be any) */
    while ((elem = List_get (obj->jobList)) != NULL) {
        job = (RcmServer_JobStream *)elem;

        /* Return any remaining messages (there should not be any) */
        msgQueH = &job->msgQue;

        while ((elem = List_get (msgQueH)) != NULL) {
            packet = (RcmClient_Packet *)elem;
            GT_2trace (curTrace,
                       GT_3CLASS,
                       "_RcmServer_Instance_finalize: Returning unprocessed "
                       "message, jobId = 0x%x, packet = 0x%x",
                       job->jobId, packet);
            _RcmServer_setStatusCode (packet, RcmServer_Status_Unprocessed);
            msgqMsg = &packet->msgqHeader;
            rval = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
            if (rval < 0) {
                GT_2trace (curTrace,
                           GT_4CLASS,
                           "_RcmServer_Instance_finalize: Unable to return "
                           "msg 0x%x from job stream 0x%x back to Client",
                           rval, job->jobId);
            }
        }

        /* Finalize the job stream object */
        List_destruct (&job->msgQue);
        status = GateMutex_delete ((GateMutex_Handle *)&(job->msgQueGate));
        if (status < 0) {
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_finalize",
                             status,
                             "Unable to delete mutex");
            status = RcmClient_E_FAIL;
            goto leave;
        }

        Memory_free (RcmServer_Module_heap(), job,
                        sizeof (RcmServer_JobStream));
    }

    /* Convenient alias */
    poolAry = obj->poolMap [0];

    /* Free all the static pool resources */
    for (i = 0; i < obj->poolMap0Len; i++) {

        /* Free all the worker thread objects */
        listH = &(poolAry [i].threadList);

        /* Mark each worker thread for termination */
        elem = NULL;
        while ((elem = List_next (listH, elem)) != NULL) {
            worker = (RcmServer_WorkerThread *)elem;
            worker->terminate = TRUE;
        }

        /* Unblock each worker thread so it can terminate */
        elem = NULL;
        while ((elem = List_next (listH, elem)) != NULL) {
            status = OsalSemaphore_post (poolAry [i].sem);
            if (status != OSALSEMAPHORE_SUCCESS) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_Instance_finalize",
                                     status,
                                     "Sem post failed for worker pool!");
                status = RcmServer_E_FAIL;
                goto leave;
            }
        }

        /* Wait for each worker thread to terminate */
        elem = NULL;
        while ((elem = List_get (listH)) != NULL) {
            worker = (RcmServer_WorkerThread *)elem;
            status = pthread_join (worker->thread, NULL);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_Instance_finalize",
                                     status,
                                     "Server thread did not exit properly!");
                status = RcmServer_E_FAIL;
                goto leave;
            }

            /* Not required for unix Thread_delete(&worker->thread); */

            /* Free the worker thread object */
            Memory_free (RcmServer_Module_heap(), worker,
                            sizeof (RcmServer_WorkerThread));
        }

        /* Free up pool resources */
#ifdef HAVE_ANDROID_OS
        /* Android bionic Semdelete code returns -1 if count == 0 */
        rval = OsalSemaphore_post (poolAry [i].sem);
        if (rval < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 rval,
                                 "Sem post failed for worker pool cleanup!");
            status = RcmServer_E_FAIL;
            goto leave;
        }
#endif /* ifdef HAVE_ANDROID_OS */
        status = OsalSemaphore_delete (&(poolAry [i].sem));
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 status,
                                 "Unable to delete RCM worker pool sync!");
            status = RcmServer_E_FAIL;
            goto leave;
        }
        List_destruct (&(poolAry [i].threadList));

        /* Return any remaining messages on the readyQueue */
        msgQueH = &(poolAry [i].readyQueue);

        while ((elem = List_get (msgQueH)) != NULL) {
            packet = (RcmClient_Packet *)elem;
            GT_2trace (curTrace,
                       GT_3CLASS,
                       "_RcmServer_Instance_finalize: Returning unprocessed "
                       "message, msgId = 0x%x, packet = 0x%x",
                       packet->msgId, packet);
            _RcmServer_setStatusCode (packet, RcmServer_Status_Unprocessed);
            msgqMsg = &packet->msgqHeader;
            rval = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
            if (rval < 0) {
                GT_2trace (curTrace,
                           GT_4CLASS,
                           "_RcmServer_Instance_finalize: Unable to return "
                           "msg 0x%x from pool 0x%x back to Client",
                           rval, packet->message.poolId);
            }
        }

        List_destruct (&(poolAry [i].readyQueue));
        status = GateMutex_delete (
                    (GateMutex_Handle *)&(poolAry [i].readyQueueGate));
        if (status < 0) {
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_finalize",
                             status,
                             "Unable to delete mutex");
            status = RcmClient_E_FAIL;
            goto leave;
        }
    }

    /* Free the name block for the static pools */
    if ((obj->poolMap [0] != NULL) && (obj->poolMap [0]->name != NULL)) {
        cp = obj->poolMap [0]->name;
        cp -= sizeof (SizeT);
        Memory_free (RcmServer_Module_heap(), (Ptr)cp, *(SizeT *)cp);
    }

    /* Free the static worker pool table */
    if (obj->poolMap [0] != NULL) {
        Memory_free (RcmServer_Module_heap(), (Ptr)(obj->poolMap [0]),
                        obj->poolMap0Len * sizeof (RcmServer_ThreadPool));
        obj->poolMap [0] = NULL;
    }

    /* TODO: free up all dynamic worker pools */
#if 0
    /* Free all dynamic worker pools */
    for (p = 1; p < RcmServer_POOL_MAP_LEN; p++) {
        if ((poolAry = obj->poolMap [p]) == NULL) {
            continue;
        }
    }
#endif

    /* Free up the dynamic function tables and any leftover name strings */
    for (i = 1; i < RCMSERVER_MAX_TABLES; i++) {
        if (obj->fxnTab [i] != NULL) {
            tabCount = (1 << (i + 4));
            for (j = 0; j < tabCount; j++) {
                if (((obj->fxnTab [i])+j)->name != NULL) {
                    cp = ((obj->fxnTab [i])+j)->name;
                    size = String_len (cp) + 1;
                    Memory_free (RcmServer_Module_heap(), cp, size);
                }
            }
            fdp = obj->fxnTab [i];
            size = tabCount * sizeof (RcmServer_FxnTabElem);
            Memory_free (RcmServer_Module_heap(), fdp, size);
            obj->fxnTab [i] = NULL;
        }
    }

    if (obj->serverThread != 0) {
        status = pthread_join (obj->serverThread, NULL);
    }

    /* TODO: Handle unprocessed messages in the serverQue */
    if (obj->serverQue != NULL) {
        status = MessageQ_delete (&(obj->serverQue));
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 status,
                                 "Error in MessageQ_delete!");
            status = RcmServer_E_FAIL;
        }
    }

    if (obj->run != NULL) {
        status = OsalSemaphore_delete (&(obj->run));
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_Instance_finalize",
                                 status,
                                 "Unable to delete RCM server run thread sync");
            status = RcmServer_E_FAIL;
            goto leave;
        }
    }

    /* Free the name block for the static function table */
    if ((obj->fxnTabStatic.elem != NULL) &&
        (obj->fxnTabStatic.elem [0].name != NULL)) {
        for (size = 0, i = 0; i < obj->fxnTabStatic.length; i++) {
            size += String_len (obj->fxnTabStatic.elem [i].name) + 1;
        }
        Memory_free (RcmServer_Module_heap(), obj->fxnTabStatic.elem [0].name,
                        size);
    }

    /* Free the static function table */
    if (obj->fxnTabStatic.elem != NULL) {
        Memory_free (RcmServer_Module_heap(), obj->fxnTabStatic.elem,
                    obj->fxnTabStatic.length * sizeof (RcmServer_FxnTabElem));
    }

    /* Destruct the instance gate */
    status = GateMutex_delete ((GateMutex_Handle *)&(obj->gate));
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_finalize",
                             status,
                             "Unable to delete mutex");
        status = RcmClient_E_FAIL;
        goto leave;
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_Instance_finalize", status);

    return status;
}


/*
 *  ======== RcmServer_addSymbol ========
 */
Int
RcmServer_addSymbol (RcmServer_Handle   handle,
                     String             funcName,
                     RcmServer_MsgFxn   addr,
                     UInt32           * index)
{
    Int                     len;
    UInt                    i;
    UInt                    j;
    UInt                    tabCount;
    SizeT                   tabSize;
    UInt32                  fxnIdx      = 0xFFFFFFFF;
    RcmServer_FxnTabElem  * slot        = NULL;
    Int                     status      = RcmServer_S_SUCCESS;

    GT_4trace (curTrace, GT_ENTER, "RcmServer_addSymbol", handle, funcName, addr,
                index);

    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "Invalid handle passed!");
        goto leave;
    }
    if (funcName == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "Remote function name passed is NULL!");
        goto leave;
    }
    if (addr == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "Remote function address is NULL!");
        goto leave;
    }
    if (index == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "Index pointer passed is NULL!");
        goto leave;
    }

    /* Protect the symbol table while changing it */
    /* TODO: key = IGateProvider_enter (handle->gate); */

    /* Look for an empty slot to use */
    for (i = 1; i < RcmServer_module->defaultCfg.maxTables; i++) {
        if (handle->fxnTab [i] != NULL) {
            for (j = 0; j < (1 << (i + 4)); j++) {
                if (((handle->fxnTab [i])+j)->addr == 0) {
                    slot = (handle->fxnTab [i]) + j;  /* Found empty slot*/
                    break;
                }
            }
        }
        else {
            /* All previous tables are full, allocate a new table */
            tabCount = (1 << (i + 4));
            tabSize = tabCount * sizeof (RcmServer_FxnTabElem);
            handle->fxnTab [i] = (RcmServer_FxnTabElem *)Memory_alloc (
                                RcmServer_Module_heap(), tabSize, sizeof (Ptr));
            if (handle->fxnTab [i] == NULL) {
                status = RcmServer_E_NOMEMORY;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "RcmServer_addSymbol",
                                     status,
                                     "Memory allocation failed for function "
                                     "table!");
                goto leave_gate;
            }

            /* Initialize the new table */
            for (j = 0; j < tabCount; j++) {
                ((handle->fxnTab [i]) + j)->addr = 0;
                ((handle->fxnTab [i]) + j)->name = NULL;
                ((handle->fxnTab [i]) + j)->key = 0;
            }

            /* Use first slot in new table */
            j = 0;
            slot = (handle->fxnTab [i]) + j;
        }

        /* If new slot found, break out of loop */
        if (slot != NULL) {
            break;
        }
    }

    /* Insert new symbol into slot */
    if (slot != NULL) {
        slot->addr = addr;
        len = String_len (funcName) + 1;
        slot->name = (String)Memory_alloc (RcmServer_Module_heap(), len,
                                            sizeof (Char *));
        if (slot->name == NULL) {
            slot->name = NULL;
            status = RcmServer_E_NOMEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "RcmServer_addSymbol",
                                 status,
                                 "Memory allocation failed for function name!");
            goto leave_gate;
        }

        String_cpy (slot->name, funcName);
        slot->key = _RcmServer_getNextKey (handle);
        fxnIdx = ((slot->key << _RCM_KeyShift) | (i << 12) | j);
    }
    /* Error, no more room to add new symbol */
    else {
        status = RcmServer_E_SYMBOLTABLEFULL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_addSymbol",
                             status,
                             "No room to add new symbol!");
    }

leave_gate:
    /* TODO: IGateProvider_leave (handle->gate, key); */
leave:
    /* On success, return new function index */
    if (status >= 0) {
        *index = fxnIdx;
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmServer_addSymbol", status);

    return status;
}


/*
 *  ======== RcmServer_removeSymbol ========
 */
Int
RcmServer_removeSymbol (RcmServer_Handle handle, String name)
{
    UInt32                  fxnIdx;
    UInt                    tabIdx;
    UInt                    tabOff;
    RcmServer_FxnTabElem  * slot;
    Int                     status = RcmServer_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "RcmServer_removeSymbol", handle, name);

    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_removeSymbol",
                             status,
                             "Module is in an invalid state!");
        goto leave;
    }
    if (handle == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_removeSymbol",
                             status,
                             "handle passed is NULL!");
        goto leave;
    }
    if (name == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_removeSymbol",
                             status,
                             "Symbol name passed is NULL!");
        goto leave;
    }

    /* Protect the symbol table while changing it */
    /* TODO: key = IGateProvider_enter (handle->gate); */

    /* Find the symbol in the table */
    status = _RcmServer_getSymIdx (handle, name, &fxnIdx);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_removeSymbol",
                             status,
                             "Given symbol not found!");
        goto leave_gate;
    }

    /* Static symbols have bit-31 set, cannot remove these symbols */
    if (fxnIdx & 0x80000000) {
        status = RcmServer_E_SYMBOLSTATIC;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_removeSymbol",
                             status,
                             "Cannot remove static symbol!");
        goto leave_gate;
    }

    /* get slot pointer */
    tabIdx = (fxnIdx & 0xF000) >> 12;
    tabOff = (fxnIdx & 0xFFF);
    slot = (handle->fxnTab [tabIdx]) + tabOff;

    /* clear the table index */
    slot->addr = 0;
    if (slot->name != NULL) {
        Memory_free (RcmServer_Module_heap(), slot->name,
                        String_len (slot->name) + 1);
        slot->name = NULL;
    }
    slot->key = 0;

leave_gate:
    /* TODO: IGateProvider_leave (handle->gate, key); */
leave:
    GT_1trace (curTrace, GT_LEAVE, "RcmServer_removeSymbol", status);

    return status;
}


/*
 *  ======== RcmServer_start ========
 */
Int
RcmServer_start (RcmServer_Handle handle)
{
    Int status = RcmServer_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "RcmServer_start", handle);

    if (RcmServer_module->setupRefCount == 0) {
        status = RcmServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_start",
                             status,
                             "Module is in an invalid state!");
    }
    else if (handle == NULL) {
        status = RcmServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "RcmServer_start",
                             status,
                             "Invalid handle passed!");
    }
    else {
        /* Signal the run synchronizer, unblocks the server thread */
        status = OsalSemaphore_post (handle->run);
#ifdef HAVE_ANDROID_OS
        /* Signal once more for Android */
        status = OsalSemaphore_post (handle->run);
#endif /* ifdef HAVE_ANDROID_OS */
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "RcmServer_start",
                                 status,
                                 "Semaphore post for RcmServer_start failed!");
            status = RcmServer_E_FAIL;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "RcmServer_start", status);

    return status;
}


/*
 *  ======== _RcmServer_acqJobId ========
 */
static Int
_RcmServer_acqJobId (RcmServer_Object * obj, UInt16 * jobIdPtr)
{
    IArg                    key;
    Int                     count;
    UInt16                  jobId;
    List_Elem             * elem;
    RcmServer_JobStream   * job;
    Int                     status = RcmServer_S_SUCCESS;
    List_Params             listP;

    GT_2trace (curTrace, GT_ENTER, "_RcmServer_acqJobId", obj, jobIdPtr);

    /* Enter critical section */
    key = IGateProvider_enter (obj->gate);

    /* Compute new job id */
    for (count = 0xFFFF; count > 0; count--) {
        /* Generate a new job id */
        jobId = (obj->jobId == 0xFFFF ? obj->jobId = 1 : ++(obj->jobId));

        /* Verify job id is not in use */
        elem = NULL;
        while ((elem = List_next (obj->jobList, elem)) != NULL) {
            job = (RcmServer_JobStream *)elem;
            if (jobId == job->jobId) {
                jobId = RcmClient_DISCRETEJOBID;
                break;
            }
        }

        if (jobId != RcmClient_DISCRETEJOBID) {
            break;
        }
    }

    /* Check if job id was acquired */
    if (jobId == RcmClient_DISCRETEJOBID) {
        *jobIdPtr = RcmClient_DISCRETEJOBID;
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_acqJobId",
                             status,
                             "No job id available!");
        goto leave;
    }

    /* Create a new job steam object */
    job = Memory_calloc (RcmServer_Module_heap(), sizeof (RcmServer_JobStream),
                            sizeof (Ptr) );
    if (job == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_acqJobId",
                             RcmServer_E_FAIL,
                             "Memory allocation failed for job stream!");
        status = RcmServer_E_NOMEMORY;
        goto leave;
    }

    /* Initialize new job stream object */
    job->jobId = jobId;
    job->empty = TRUE;
    List_Params_init (&listP);
    job->msgQueGate = (IGateProvider_Handle) GateMutex_create ();
    GT_assert (curTrace, (job->msgQueGate != NULL));
    if (job->msgQueGate == NULL) {
        status = RcmServer_E_FAIL;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_Instance_init",
                             status,
                             "Unable to create mutex!");
        goto leave;
    }
    listP.gateHandle = job->msgQueGate;
    List_construct (&(job->msgQue), &listP);

    /* Put new job stream object at end of server list */
    List_put (obj->jobList, (List_Elem *)job);

    /* Return new job id */
    *jobIdPtr = jobId;

leave:
    /* Leave critical section */
    IGateProvider_leave (obj->gate, key);

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_acqJobId", status);

    return status;
}


/*
 *  ======== _RcmServer_dispatch ========
 *
 *  Return Value
 *      < 0: error
 *        0: success, job stream queue
 *      > 0: success, ready queue
 *
 *  Pool id description
 *
 *  Static Worker Pools
 *  --------------------------------------------------------------------
 *  15      1 = static pool
 *  14:8    reserved
 *  7:0     offset: 0 - 255
 *
 *  Dynamic Worker Pools
 *  --------------------------------------------------------------------
 *  15      0 = dynamic pool
 *  14:7    key: 0 - 255
 *  6:5     index: 1 - 3
 *  4:0     offset: 0 - [7, 15, 31]
 */
static Int
_RcmServer_dispatch (RcmServer_Object * obj, RcmClient_Packet * packet)
{
    IArg                    key;
    List_Elem             * elem;
    List_Handle             listH;
    RcmServer_ThreadPool  * pool;
    UInt16                  jobId;
    RcmServer_JobStream   * job;
    Int                     status = RcmServer_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "_RcmServer_dispatch", obj, packet);

    /* Get the target pool id from the message */
    status = _RcmServer_getPool (obj, packet, &pool);
    if (status < 0) {
        goto leave;
    }

    /* Discrete jobs go on the end of the ready queue */
    jobId = packet->message.jobId;

    if (jobId == RcmClient_DISCRETEJOBID) {
        listH = &pool->readyQueue;
        List_put (listH, (List_Elem *)packet);

        /* Dispatch a new worker thread */
        status = OsalSemaphore_post (pool->sem);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_dispatch",
                                 status,
                                 "Could not post pool semaphore!");
        }
    }
    /* Must be a job stream message */
    else {
        /* Must protect job list while searching it */
        key = IGateProvider_enter (obj->gate);

        /* Find the job stream object in the list */
        elem = NULL;
        while ((elem = List_next (obj->jobList, elem)) != NULL) {
            job = (RcmServer_JobStream *)elem;
            if (job->jobId == jobId) {
                break;
            }
        }

        if (elem == NULL) {
            status = RcmServer_E_JobIdNotFound;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_dispatch",
                                 status,
                                 "Failed to find job stream!");
        }
        /* If job object is empty, place message directly on ready queue */
        else if (job->empty) {
            job->empty = FALSE;
            listH = &pool->readyQueue;
            List_put (listH, (List_Elem *)packet);

            /* Dispatch a new worker thread */
            status = OsalSemaphore_post (pool->sem);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_dispatch",
                                     RcmServer_E_INVALIDSTATE,
                                     "Could not post pool semaphore!");
            }
        }
        /* Place message on job queue */
        else {
            listH = &job->msgQue;
            List_put (listH, (List_Elem *)packet);
        }
        IGateProvider_leave (obj->gate, key);
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_dispatch", status);

    return status;
}


/*
 *  ======== _RcmServer_execMsg ========
 */
static Int
_RcmServer_execMsg (RcmServer_Object * obj, RcmClient_Message * msg)
{
    RcmServer_MsgFxn    fxn;
    Int                 status;

    GT_2trace (curTrace, GT_ENTER, "_RcmServer_execMsg", obj, msg);

    status = _RcmServer_getFxnAddr (obj, msg->fxnIdx, &fxn);
    if (status >= 0) {
        msg->result = (*fxn)(msg->dataSize, msg->data);
    }

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_execMsg", status);

    return status;
}


/*
 *  ======== _RcmServer_getFxnAddr ========
 *
 *  The function index (fxnIdx) uses the following format. Note that the
 *  format differs for static vs. dynamic functions. All static functions
 *  are in fxnTab [0], all dynamic functions are in fxnTab [1 - 8].
 *
 *  Bits    Description
 *
 *  Static Function Index
 *  --------------------------------------------------------------------
 *  31      static/dynamic function flag
 *              0 = dynamic function
 *              1 = static function
 *  30:20   reserved
 *  19:16   reserved
 *  15:0    offset: 0 - 65,535
 *
 *  Dynamic Function Index
 *  --------------------------------------------------------------------
 *  31      static/dynamic function flag
 *              0 = dynamic function
 *              1 = static function
 *  30:20   key
 *  19:16   reserved
 *  15:12   index: 1 - 8
 *  11:0    offset: 0 - [31, 63, 127, 255, 511, 1023, 2047, 4095]
 */
static Int
_RcmServer_getFxnAddr (RcmServer_Object   * obj,
                       UInt32               fxnIdx,
                       RcmServer_MsgFxn   * addrPtr)
{
    UInt                    i;
    UInt                    j;
    UInt16                  key;
    RcmServer_FxnTabElem  * slot;
    RcmServer_MsgFxn        addr = 0;
    Int                     status = RcmServer_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_RcmServer_getFxnAddr", obj, fxnIdx,
                addrPtr);

    /* Static functions have bit-31 set */
    if (fxnIdx & 0x80000000) {
        j = (fxnIdx & 0x0000FFFF);
        if (j < (obj->fxnTabStatic.length)) {
            /* Fetch the function address from the table */
            slot = (obj->fxnTab [0])+j;
            addr = slot->addr;
        }
        else {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_getFxnAddr",
                                 fxnIdx,
                                 "Invalid function index!");
            status = RcmServer_E_InvalidFxnIdx;
        }
    }
    /* Must be a dynamic function */
    else {
        /* Extract the key from the function index */
        key = ((fxnIdx & _RCM_KeyMask) >> _RCM_KeyShift);
        i = (fxnIdx & 0xF000) >> 12;
        if ((i > 0) && (i < RCMSERVER_MAX_TABLES) && \
            (obj->fxnTab [i] != NULL)) {
            /* Fetch the function address from the table */
            j = (fxnIdx & 0x0FFF);
            slot = (obj->fxnTab [i]) + j;
            addr = slot->addr;

            /* Validate the key */
            if (key != slot->key) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_getFxnAddr",
                                     fxnIdx,
                                     "Invalid function index!");
                status = RcmServer_E_InvalidFxnIdx;
            }
        }
        else {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_getFxnAddr",
                                 fxnIdx,
                                 "Invalid function index!");
            status = RcmServer_E_InvalidFxnIdx;
        }
    }

    if (status >= 0) {
        *addrPtr = addr;
    }

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_getFxnAddr", status);

    return status;
}


/*
 *  ======== _RcmServer_getSymIdx ========
 *
 *  Must have table gate before calling this function.
 */
static Int
_RcmServer_getSymIdx (RcmServer_Object * obj, String name, UInt32 * index)
{
    UInt                    i;
    UInt                    j;
    UInt                    len;
    RcmServer_FxnTabElem  * slot;
    UInt32                  fxnIdx = 0xFFFFFFFF;
    Int                     status = RcmServer_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_RcmServer_getSymIdx", obj, name, index);

    /* Search tables for given function name */
    for (i = 0; i < RCMSERVER_MAX_TABLES; i++) {
        if (obj->fxnTab [i] != NULL) {
            len = (i == 0) ? obj->fxnTabStatic.length : (1 << (i + 4));
            for (j = 0; j < len; j++) {
                slot = (obj->fxnTab [i]) + j;
                if ((((obj->fxnTab [i]) + j)->name != NULL) &&
                    (String_cmp (((obj->fxnTab [i]) + j)->name, name) == 0) ) {
                    if (i == 0) {
                        fxnIdx = 0x80000000 | j;
                    }
                    else {
                        fxnIdx = ((slot->key << _RCM_KeyShift) | (i << 12) | j);
                    }
                    break;
                }
            }
        }

        if (fxnIdx != 0xFFFFFFFF) {
            break;
        }
    }

    /* Log an error if the symbol was not found */
    if (fxnIdx == 0xFFFFFFFF) {
        status = RcmServer_E_SYMBOLNOTFOUND;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_getSymIdx",
                             status,
                             "Given symbol not found!");
    }

    /* If success, return symbol index */
    if (status >= 0) {
        *index = fxnIdx;
    }

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_getSymIdx", status);

    return status;
}


/*
 *  ======== _RcmServer_getNextKey ========
 */
static UInt16
_RcmServer_getNextKey (RcmServer_Object * obj)
{
    IArg    gateKey;
    UInt16  key;

    GT_1trace (curTrace, GT_ENTER, "_RcmServer_getNextKey", obj);

    gateKey = IGateProvider_enter (obj->gate);

    if (obj->key <= 1) {
        obj->key = _RCM_KeyResetValue;  /* don't use 0 as a key value */
    }
    else {
        (obj->key)--;
    }
    key = obj->key;

    IGateProvider_leave (obj->gate,gateKey);

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_getNextKey", key);

    return key;
}


/*
 *  ======== _RcmServer_getPool ========
 */
static Int
_RcmServer_getPool (RcmServer_Object      * obj,
                    RcmClient_Packet      * packet,
                    RcmServer_ThreadPool ** poolP)
{
    UInt16  poolId;
    UInt16  offset;
    Int     status = RcmServer_S_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_RcmServer_getPool", obj, packet, poolP);

    poolId = packet->message.poolId;

    /* Static pools have bit-15 set */
    if (poolId & 0x8000) {
        offset = (poolId & 0x00FF);
        if (offset < obj->poolMap0Len) {
            *poolP = &(obj->poolMap [0]) [offset];
        }
        else {
            *poolP = NULL;
            status = RcmServer_E_PoolIdNotFound;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_getPool",
                                 status,
                                 "Pool id not found1");
        }
    }
    /* Must be a dynamic pool */
    else {
        /* TODO */
        status = RcmServer_E_FAIL;
    }

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_getPool", status);

    return status;
}


/*
 *  ======== _RcmServer_process ========
 */
static Void
_RcmServer_process (RcmServer_Object * obj, RcmClient_Packet * packet)
{
    String              name;
    UInt32              fxnIdx;
    RcmServer_MsgFxn    fxn;
    RcmClient_Message * rcmMsg;
    MessageQ_Msg        msgqMsg;
    UInt16              messageType;
    UInt16              jobId;
    Int                 rval;
    Int                 status      = RcmServer_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "_RcmServer_process", obj, packet);

    /* Decode the message */
    rcmMsg = &packet->message;
    msgqMsg = &packet->msgqHeader;

    /* Extract the message type from the packet descriptor field */
    messageType = ((RcmClient_Desc_TYPE_MASK & packet->desc) >>
                        RcmClient_Desc_TYPE_SHIFT);

    /* Process the given message */
    switch (messageType) {
    case RcmClient_Desc_RCM_MSG:
        rval = _RcmServer_execMsg(obj, rcmMsg);
        if (rval < 0) {
            switch (rval) {
            case RcmServer_E_InvalidFxnIdx:
                _RcmServer_setStatusCode (packet,
                                            RcmServer_Status_INVALID_FXN);
                break;
            default:
                _RcmServer_setStatusCode (packet,
                                            RcmServer_Status_ERROR);
                break;
            }
        }
        else if (rcmMsg->result < 0) {
            _RcmServer_setStatusCode (packet, RcmServer_Status_MSG_FXN_ERR);
        }
        else {
            _RcmServer_setStatusCode (packet, RcmServer_Status_SUCCESS);
        }

        status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_process",
                                 status,
                                 "Unable to send the response back to the "
                                 "client!");
        }
        break;

    case RcmClient_Desc_CMD:
        status = _RcmServer_execMsg(obj, rcmMsg);
        /* If all went well, free the message */
        if ((status >= 0) && (rcmMsg->result >= 0)) {
            status = MessageQ_free (msgqMsg);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_process",
                                     status,
                                     "Unable to free the RCM Cmd MessageQ!");
            }
        }
        /* An error occurred, must return message to client */
        else {
            if (status < 0) {
                /* Error trying to process the message */
                switch (status) {
                case RcmServer_E_InvalidFxnIdx:
                    _RcmServer_setStatusCode (packet,
                                                RcmServer_Status_INVALID_FXN);
                    break;
                default:
                    _RcmServer_setStatusCode (packet,
                                                RcmServer_Status_ERROR);
                    break;
                }
            }
            else  {
                /* Error in message function */
                _RcmServer_setStatusCode (packet, RcmServer_Status_MSG_FXN_ERR);
            }

            /* Send error message back to client */
            status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_process",
                                     status,
                                     "Unable to send the response back to the "
                                     "client!");
            }
        }
        break;

    case RcmClient_Desc_DPC:
        rval = _RcmServer_getFxnAddr (obj, rcmMsg->fxnIdx, &fxn);
        if (rval < 0) {
            _RcmServer_setStatusCode (packet,
                                        RcmServer_Status_SYMBOL_NOT_FOUND);
        }

        /* TODO: copy the context into a buffer */

        status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_process",
                                 status,
                                 "Unable to send the response back to the "
                                 "client!");
        }

        /* invoke the function with a null context */
        (*fxn)(0, NULL);
        break;

    case RcmClient_Desc_SYM_ADD:
        break;

    case RcmClient_Desc_SYM_IDX:
        name = (String)rcmMsg->data;
        rval = _RcmServer_getSymIdx(obj, name, &fxnIdx);

        if (rval < 0) {
            _RcmServer_setStatusCode (
                packet, RcmServer_Status_SYMBOL_NOT_FOUND);
        }
        else {
            _RcmServer_setStatusCode (packet, RcmServer_Status_SUCCESS);
            rcmMsg->data[0] = fxnIdx;
            rcmMsg->result = 0;
        }

        status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_process",
                                 status,
                                 "Unable to send the response back to the "
                                 "client!");
        }
        break;

    case RcmClient_Desc_JOB_ACQ:
        rval = _RcmServer_acqJobId(obj, &jobId);

        if (rval < 0) {
            _RcmServer_setStatusCode (packet, RcmServer_Status_ERROR);
        }
        else {
            _RcmServer_setStatusCode (packet, RcmServer_Status_SUCCESS);
            *(UInt16 *)(&rcmMsg->data[0]) = jobId;
            rcmMsg->result = 0;
        }

        status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_process",
                                 status,
                                 "Unable to send the response back to the "
                                 "client!");
        }
        break;

    case RcmClient_Desc_JOB_REL:
        jobId = (UInt16)(rcmMsg->data[0]);
        rval = _RcmServer_relJobId(obj, jobId);

        if (rval < 0) {
            switch (rval) {
            case RcmServer_E_JobIdNotFound:
                _RcmServer_setStatusCode (packet,
                                            RcmServer_Status_JobNotFound);
                break;
            default:
                _RcmServer_setStatusCode (packet, RcmServer_Status_ERROR);
                break;
            }
            rcmMsg->result = rval;
        }
        else {
            _RcmServer_setStatusCode (packet, RcmServer_Status_SUCCESS);
            rcmMsg->result = 0;
        }

        status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_RcmServer_process",
                                 status,
                                 "Unable to send the response back to the "
                                 "client!");
        }
        break;

    default:
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_process",
                             status,
                             "Unknown message received!");
        /* TODO
         * Raise an error
         * Set the return status code in message
         *     RcmServer_Status_INVALID_MSG_TYPE
         * Return the message
         * Clear the error block
         */
        break;
    }

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_process", status);
}


/*
 *  ======== _RcmServer_relJobId ========
 */
static Int
_RcmServer_relJobId (RcmServer_Object * obj, UInt16 jobId)
{
    IArg                    key;
    List_Elem             * elem;
    List_Handle             msgQueH;
    RcmClient_Packet      * packet;
    RcmServer_JobStream   * job;
    MessageQ_Msg            msgqMsg;
    Int                     rval;
    Int                     status  = RcmServer_S_SUCCESS;

    GT_2trace (curTrace, GT_ENTER, "_RcmServer_relJobId", obj, jobId);

    /* Must protect job list while searching and modifying it */
    key = IGateProvider_enter (obj->gate);

    /* Find the job stream object in the list */
    elem = NULL;
    while ((elem = List_next (obj->jobList, elem)) != NULL) {
        job = (RcmServer_JobStream *)elem;

        /* remove the job stream object from the list */
        if (job->jobId == jobId) {
            List_remove (obj->jobList, elem);
            break;
        }
    }

    if (elem == NULL) {
        status = RcmServer_E_JobIdNotFound;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_relJobId",
                             jobId,
                             "Failed to find jobId!");
        goto leave;
    }

    /* Return any pending messages on the message queue */
    msgQueH = &job->msgQue;

    while ((elem = List_get (msgQueH)) != NULL) {
        packet = (RcmClient_Packet *)elem;
        GT_2trace (curTrace,
                   GT_1CLASS,
                   "_RcmServer_relJobId: Returning unprocessed message"
                   " jobId = 0x%x, packet = 0x%x", jobId, (IArg)packet);
        _RcmServer_setStatusCode (packet, RcmServer_Status_Unprocessed);
        msgqMsg = &packet->msgqHeader;
        rval = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
        if (rval < 0) {
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_relJobId",
                             rval,
                             "Unable to send the response back to the "
                             "client!");
        }
    }

    /* Finalize the job stream object */
    List_destruct (&job->msgQue);
    status = GateMutex_delete ((GateMutex_Handle *)&(job->msgQueGate));
    if (status < 0) {
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "_RcmServer_Instance_finalize",
                         status,
                         "Unable to delete mutex");
        status = RcmClient_E_FAIL;
        goto leave;
    }
    Memory_free (RcmServer_Module_heap(), (Ptr)job,
                    sizeof (RcmServer_JobStream));

leave:
    IGateProvider_leave (obj->gate, key);

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_relJobId", status);

    return status;
}


/*
 *  ======== _RcmServer_serverThrFxn ========
 */
static Void
_RcmServer_serverThrFxn (IArg arg)
{
    RcmClient_Packet  * packet;
    MessageQ_Msg        msgqMsg;
    Int                 status;
    Bool                running     = TRUE;
    RcmServer_Object  * obj         = (RcmServer_Object *)arg;

    /* Wait until ready to run */
    status = OsalSemaphore_pend (obj->run, OSALSEMAPHORE_WAIT_FOREVER);
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "_RcmServer_serverThrFxn",
                             status,
                             "Semaphore_pend failure!");
        goto leave;
    }

    /* Main processing loop */
    while (running) {
        GT_1trace (curTrace,
                   GT_ENTER,
                   "_RcmServer_serverThrFxn obj->serverThread = 0x%x",
                   (IArg)(obj->serverThread));

        /* Block until message arrives */
        do {
            status = MessageQ_get (obj->serverQue, &msgqMsg, MessageQ_FOREVER);
            if ((status < 0) && (status != MessageQ_E_UNBLOCKED)) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_serverThrFxn",
                                     status,
                                     "Semaphore_pend failure!");
                status = RcmServer_E_FAIL;
                /* Keep running and hope for the best */
            }
        } while ((msgqMsg == NULL) && !obj->shutdown);

        /* If shutdown, exit this thread */
        if (obj->shutdown) {
            running = FALSE;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "_RcmServer_serverThrFxn terminating thread = 0x%x",
                       (IArg)(obj->serverThread));
            if (msgqMsg == NULL ) {
                continue;
            }
            else {
                /* TODO: Decide whether to terminate without processing the
                 *       message. Currently, will process the message and then
                 *       terminate
                 */
            }
        }

        packet = (RcmClient_Packet *)msgqMsg;

        GT_2trace (curTrace,
                   GT_1CLASS,
                   "_RcmServer_serverThrFxn message received, "
                   "thread = 0x%x, packet = 0x%x",
                   (IArg)(obj->serverThread), (IArg)packet);

        if ((packet->message.poolId == RcmClient_DEFAULTPOOLID)
            && ((obj->poolMap [0])[0].count == 0)) {
            /* In-band (server thread) message processing */
            _RcmServer_process (obj, packet);
        }
        else {
            /* Out-of-band (worker thread) message processing */
            status = _RcmServer_dispatch (obj, packet);
            /* If error, message was not dispatched; must return to client */
            if (status < 0) {
                switch (status) {
                case RcmServer_E_JobIdNotFound:
                    _RcmServer_setStatusCode (packet,
                                                RcmServer_Status_JobNotFound);
                    break;

                case RcmServer_E_PoolIdNotFound:
                    _RcmServer_setStatusCode (packet,
                                                RcmServer_Status_PoolNotFound);
                    break;

                default:
                    _RcmServer_setStatusCode (packet, RcmServer_Status_ERROR);
                    break;
                }
                packet->message.result = status;

                /* Return the message to the client */
                status = MessageQ_put (MessageQ_getReplyQueue (msgqMsg), msgqMsg);
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "_RcmServer_serverThrFxn",
                                         status,
                                         "Unable to send the response back to "
                                         "the client!");
                }
            }
        }
    }

leave:
    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_serverThrFxn", status);
}


/*
 *  ======== _RcmServer_setStatusCode ========
 */
static Void
_RcmServer_setStatusCode (RcmClient_Packet * packet, UInt16 code)
{
    /* Code must be 0 - 15, it has to fit in a 4-bit field */
    GT_assert (curTrace, (code < 16));

    packet->desc &= ~(RcmClient_Desc_TYPE_MASK);
    packet->desc |= ((code << RcmClient_Desc_TYPE_SHIFT)
                            & RcmClient_Desc_TYPE_MASK);
}


/*
 *  ======== _RcmServer_workerThrFxn ========
 */
static Void
_RcmServer_workerThrFxn (IArg arg)
{
    RcmClient_Packet          * packet;
    List_Elem                 * elem;
    List_Handle                 listH;
    List_Handle                 readyQueueH;
    UInt16                      jobId;
    IArg                        key;
    RcmServer_ThreadPool      * pool;
    RcmServer_JobStream       * job;
    RcmServer_WorkerThread    * obj;
    Bool                        running;
    Int                         rval;
    Int                         rval1;

    GT_1trace (curTrace, GT_ENTER, "_RcmServer_workerThrFxn", arg);

    obj = (RcmServer_WorkerThread *)arg;
    readyQueueH = &obj->pool->readyQueue;
    packet = NULL;
    running = TRUE;

    /* Main processing loop */
    while (running) {
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "_RcmServer_workerThrFxn waiting for job, thread = 0x%x",
                   (IArg)(obj->thread));

        /* If no current message, wait until signaled to run */
        if (packet == NULL) {
            rval = OsalSemaphore_pend (obj->pool->sem,
                                        OSALSEMAPHORE_WAIT_FOREVER);
            if (rval < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_RcmServer_workerThrFxn",
                                     rval,
                                     "Semaphore pend failed!");
            }
        }

        /* Check if thread should terminate */
        if (obj->terminate) {
            running = FALSE;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "_RcmServer_workerThrFxn: terminating thread = 0x%x",
                       (IArg)(obj->thread));
            continue;
        }

        /* Get next message from ready queue */
        packet = (RcmClient_Packet *)List_get (readyQueueH);
        if (packet == NULL) {
            GT_1trace (curTrace,
                       GT_2CLASS,
                       "_RcmServer_workerThrFxn: Ready queue is empty "
                       " worker thread = 0x%x", (IArg) obj->thread);
            continue;
        }

        GT_2trace (curTrace,
                   GT_1CLASS,
                   "_RcmServer_workerThrFxn: job received "
                   "thread = 0x%x packet = 0x%x",
                    (IArg)obj->thread, (IArg)packet);

        /* Remember the message job id */
        jobId = packet->message.jobId;

        /* Process the message */
        _RcmServer_process(obj->server, packet);
        packet = NULL;

        /* If this worker thread just finished processing a job message,
         * queue up the next message for this job id. As an optimization,
         * if the message is addressed to this worker's pool, then don't
         * signal the semaphore, just get the next message from the queue
         * and processes it. This keeps the current thread running instead
         * of switching to another thread.
         */
        if (jobId != RcmClient_DISCRETEJOBID) {

            /* Must protect job list while searching it */
            key = IGateProvider_enter (obj->server->gate);

            /* Find the job object in the list */
            elem = NULL;
            while ((elem = List_next (obj->server->jobList, elem)) != NULL) {
                job = (RcmServer_JobStream *)elem;
                if (job->jobId == jobId) {
                    break;
                }
            }

            /* If job object not found, it is not an error */
            if (elem == NULL) {
                IGateProvider_leave (obj->server->gate, key);
                continue;
            }

            /* Found the job object */
            listH = &job->msgQue;

            /* Get next job message and queue it */
            do {
                elem = List_get (listH);
                if (elem == NULL) {
                    job->empty = TRUE;  /* no more messages */
                    break;
                }
                else {
                    /* Get target pool id */
                    packet = (RcmClient_Packet *)elem;
                    rval = _RcmServer_getPool (obj->server, packet, &pool);
                    /* If error, return the message to the client */
                    if (rval < 0) {
                        switch (rval) {
                        case RcmServer_E_PoolIdNotFound:
                            _RcmServer_setStatusCode (packet,
                                              RcmServer_Status_PoolNotFound);
                            break;

                        default:
                            _RcmServer_setStatusCode (packet,
                                                        RcmServer_Status_ERROR);
                            break;
                        }
                        packet->message.result = rval;

                        rval1 = MessageQ_put (
                                MessageQ_getReplyQueue (&packet->msgqHeader),
                                &packet->msgqHeader);
                        if (rval1 < 0) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmServer_workerThrFxn",
                                                 rval,
                                                 "Unable to send the response "
                                                 "back to the client!");
                        }
                    }
                    /* Packet is valid, queue it in the corresponding pool's
                     * ready queue */
                    else {
                        listH = &pool->readyQueue;
                        List_put (listH, elem);
                        packet = NULL;
                        rval = OsalSemaphore_post (pool->sem);
                        if (rval < 0) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_RcmServer_workerThrFxn",
                                                 rval,
                                                 "Pool Ssemaphore post failed");
                        }
                    }

                    /* Loop around and wait to be run again */
                }
            } while (rval < 0);

            IGateProvider_leave (obj->server->gate, key);
        }
    }  /* while (running) */

    GT_1trace (curTrace, GT_LEAVE, "_RcmServer_workerThrFxn", rval);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
