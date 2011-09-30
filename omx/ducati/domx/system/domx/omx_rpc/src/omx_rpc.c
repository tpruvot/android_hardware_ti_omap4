/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  @file  omx_rpc.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 29-Mar-2010 Abhishek Ranka : Revamped DOMX implementation
 *!
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Std.h>
#include <pthread.h>
#include <errno.h>

#include <OMX_Types.h>
#include <timm_osal_interfaces.h>
#include <timm_osal_trace.h>

#include <MultiProc.h>
#include <RcmClient.h>
#include <RcmServer.h>
#include <IpcUsr.h>
#include <ProcMgr.h>

#include <SysLinkMemUtils.h>
#include <hardware_legacy/power.h>

/*-------program files ----------------------------------------*/
#include "omx_rpc.h"
#include "omx_proxy_common.h"
#include "omx_rpc_stub.h"
#include "omx_rpc_skel.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"
#include <utils/Log.h>
#define LOG_TAG "DOMX_RPC"
#define DOMX_DEBUG LOGE
#define DOMX_ERROR LOGE
/* **************************** MACROS DEFINES *************************** */

/*For ipc setup*/
#define SYSM3_PROC_NAME             "SysM3"
#define APPM3_PROC_NAME             "AppM3"
#define SET_WATCHDOG_TIMEOUT  // For customer request to kill mediaserver
#ifdef  SET_WATCHDOG_TIMEOUT
#define WATCHDOG_TIMEOUT 5000
#endif
/*The version nos. start with 1 and keep on incrementing every time there is a
protocol change in DOMX. This is just a marker to ensure that A9-Ducati DOMX
versions are in sync and does not indicate anything else*/
#define DOMX_VERSION 11

/*This is the time in msec for which we'll wait for the Ducati base image to
load and send a PROC_START signal. If the timeout expires, an error will be
returned to the caller*/
#define RPC_TIMEOUT_FOR_DUCATI_IMAGE_LOAD 5000

/*This is the max no. of times it'll try to create RCM client before returning
failure*/
#define RPC_CLIENT_CREATE_MAX_TRIES 20

/*Time in msec for which the task sleeps between consecutive tries to create
RCM client*/
#define RPC_CLIENT_CREATE_TIME_BETWEEN_TRIES 1

static const char kDomxRpcWakeLock[] = "DOmxRpcWakelock";

/* ******************************* EXTERNS ********************************* */
extern char rpcFxns[][MAX_FUNCTION_NAME_LENGTH];
extern rpcSkelArr rpcSkelFxns[];
extern char Core_Array[][MAX_CORENAME_LENGTH];
extern char rcmservertable[][MAX_SERVER_NAME_LENGTH];
extern OMX_U32 heapIdArray[MAX_NUMBER_OF_HEAPS];

extern TIMM_OSAL_PTR testSem;
extern TIMM_OSAL_PTR testSemSys;
#ifdef  SET_WATCHDOG_TIMEOUT
static TIMM_OSAL_PTR pWatchDogSem;
static OMX_BOOL bWatchDogSem = FALSE;
#endif

/* ******************************* GLOBALS ********************************* */
RPC_Object rpcHndl[CORE_MAX];

RcmClient_Handle rcmHndl = NULL;
RcmServer_Handle rcmSrvHndl = NULL;

COREID TARGET_CORE_ID = CORE_MAX;	//Should be configured in the CFG or header file for SYS APP split header.
COREID LOCAL_CORE_ID = CORE_MAX;

//Counter to reflect no. of users
static OMX_U32 nInstanceCount = 0;
OMX_U8 CHIRON_IPC_FLAG = 1;

/*Mutex used to ensure that setup is done only once. It is created when library
  is loaded.*/
OMX_PTR pCreateMutex = NULL;

/*Used in ipc setup/destroy*/
ProcMgr_Handle procMgrHandle = NULL;
ProcMgr_Handle procMgrHandle1 = NULL;

/*Ducati fault handler*/
OMX_S32 currentNumOfComps = 0;
OMX_HANDLETYPE componentTable[MAX_NUM_COMPS_PER_PROCESS] = { 0 };

/*Book keeping for tiler buffers*/
#define MAX_NUMBER_OF_TILER_BUFFERS_PER_PROCESS 128
OMX_U32 tilerBuffers[MAX_NUMBER_OF_TILER_BUFFERS_PER_PROCESS] = { 0 };

OMX_PTR pTilerMutex = NULL;

OMX_BOOL ducatiFault = OMX_FALSE;

static bool bDFH_Created = false;
static pthread_t ducatiFaultHandler;

#ifdef  SET_WATCHDOG_TIMEOUT
pthread_t watchDogFaultHandler;
#endif
/*This mutex protects filling/removing data from componentTable*/
OMX_PTR pFaultMutex = NULL;

/* ************************* EXTERNS, FUNCTION DECLARATIONS ***************************** */
RPC_INDEX fxnExitidx, getFxnIndexFromRemote_skelIdx;
static Int32 fxnExit(UInt32 size, UInt32 * data);
RPC_OMX_ERRORTYPE fxn_exit_caller(void);

static Int32 getFxnIndexFromRemote_skel(UInt32 size, UInt32 * data);

RPC_OMX_ERRORTYPE _RPC_GetRemoteIndices(RPC_OMX_HANDLE hRcmHandle,
    OMX_U32 nFxnIdx);

RPC_OMX_ERRORTYPE _RPC_IpcSetup();
RPC_OMX_ERRORTYPE _RPC_IpcDestroy();
RPC_OMX_ERRORTYPE _RPC_ClientCreate(OMX_STRING cComponentName);
RPC_OMX_ERRORTYPE _RPC_ClientDestroy();

static void *RPC_DucatiFaultHandler(void *);
#ifdef  SET_WATCHDOG_TIMEOUT
static void *RPC_WatchDogFaultHandler(void *);
#endif

/* ===========================================================================*/
/**
 * @name RPC_InstanceInit()
 * @brief RPC instance init is used to bring up a instance of a client - this should be ideally invokable from any core
 *        For this the parameters it would require are
 *        Heap ID - this needs to be configured at startup (CFG) and indicates the heaps available for a RCM client to pick from
 *        Server - this contains the RCM server name that the client should connect to
 *        rcmHndl - Contains the Client once the call is completed
 *        rcmParams -
 *        These values can be picked up from the RPC handle. But an unique identifier is required -Server
 * @param cComponentName  : Pointer to the Components Name that is requires the RCM client to be initialized
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_InstanceInit(OMX_STRING cComponentName,
    RPC_OMX_HANDLE * phRPCCtx)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eTmpError = RPC_OMX_ErrorNone;
	RPC_OMX_CONTEXT *pRPCCtx = NULL;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_BOOL bMutex = OMX_FALSE, bModInit = OMX_FALSE, bIpcSet =
	    OMX_FALSE, bJobIdObtained = OMX_FALSE, bClientCreated = OMX_FALSE;
	OMX_U16 nPoolId = 0, nJobId = 0;

	pRPCCtx =
	    (RPC_OMX_CONTEXT *) TIMM_OSAL_Malloc(sizeof(RPC_OMX_CONTEXT),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	RPC_assert(pRPCCtx != NULL, RPC_OMX_ErrorInsufficientResources,
	    "Malloc failed");

	*(RPC_OMX_CONTEXT **) phRPCCtx = pRPCCtx;

	eError = TIMM_OSAL_MutexObtain(pCreateMutex, TIMM_OSAL_SUSPEND);
	RPC_assert(eError == TIMM_OSAL_ERR_NONE,
	    RPC_OMX_ErrorInsufficientResources, "Mutex lock failed");
	bMutex = OMX_TRUE;

	nInstanceCount++;
	/*For 1st instance, do all the setup and create client */
	if (nInstanceCount == 1)
	{
		/*
		 * Acquire wake lock to prevent userspace from being suspended
		 *   if any client exists
		 */
		acquire_wake_lock(PARTIAL_WAKE_LOCK, kDomxRpcWakeLock);

		eRPCError = _RPC_IpcSetup();
		RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
		    "Basic ipc setup failed");
		bIpcSet = OMX_TRUE;

		LOCAL_CORE_ID = MultiProc_getId(NULL);
		/*Extract target core id from component name */
		eRPCError = RPC_UTIL_GetTargetCore(cComponentName,
		    (OMX_U32 *) (&TARGET_CORE_ID));
		RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
		    "Target core id could not be retrieved");

		eRPCError = RPC_ModInit();
		RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
		    "ModInit failed");
		bModInit = OMX_TRUE;

		/*This will fill in the global rcmHndl */
		eRPCError = _RPC_ClientCreate(cComponentName);
		RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
		    "Client create failed");
		bClientCreated = OMX_TRUE;

		/*Create the Ducati Fault Handler */
		/*pthread_create use instead of TIMM_OSAL_TaskCreate.
		   TIMM_OSAL_TaskCreate, in addition to calling pthread_create
		   allocates memory to maintain TIMM_OSAK_Task context. During
		   fault recovery this memory also needs to be
		   explicitly cleaned. However, when an MMU fault
		   occurs, in order to clean this memory,  there is no way but
		   to invoke TIMM_OSAL_TaskDelete
		   from the context of the thread which we are trying to delete.
		   To avoid this, only option is to use pthread_create direcly,
		   thus avoiding any additional memory allocation.
		   Clean up of the thread is then left to kernel. */

		if (!bDFH_Created)
		{
			if (SUCCESS != pthread_create(&ducatiFaultHandler,
				NULL, RPC_DucatiFaultHandler, NULL))
			{
				eRPCError = RPC_OMX_ErrorUnknown;
			}
			else
			{
				DOMX_DEBUG("ducatiFaultHandler thread created.\n");
				bDFH_Created = true;
			}
		}

		RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
		    "Creation of ducati fault handler failed.");
	}

	/* updating the RCM client handle within rpc context */
	pRPCCtx->ClientHndl[RCM_DEFAULT_CLIENT] = rcmHndl;
	pRPCCtx->ClientHndl[RCM_ADDITIONAL_CLIENT] = NULL;

	eRPCError = RPC_Util_AcquireJobId(pRPCCtx, &nJobId);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone,
	    OMX_ErrorUndefined, "Error getting Job ID");
	pRPCCtx->nJobId = nJobId;
	bJobIdObtained = OMX_TRUE;

	eRPCError = RPC_Util_GetPoolId(pRPCCtx, &nPoolId);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone,
	    OMX_ErrorUndefined, "Error getting Pool ID");
	pRPCCtx->nPoolId = nPoolId;

      EXIT:
	if (eRPCError != RPC_OMX_ErrorNone)
	{
		if (bJobIdObtained)
		{
			eTmpError =
			    RPC_Util_ReleaseJobId(pRPCCtx, pRPCCtx->nJobId);
			if (eTmpError != RPC_OMX_ErrorNone)
			{
				DOMX_ERROR("Release job ID failed");
			}
		}
		if (bClientCreated)
		{
			eTmpError = _RPC_ClientDestroy();
			if (eTmpError != RPC_OMX_ErrorNone)
			{
				DOMX_ERROR("RPC Client destroy failed");
			}
		}
		if (bModInit)
		{
			eTmpError = RPC_ModDeInit();
			if (eTmpError != RPC_OMX_ErrorNone)
			{
				DOMX_ERROR("RPC ModDeInit failed");
			}
		}
		if (bIpcSet)
		{
			eTmpError = _RPC_IpcDestroy();
			if (eTmpError != RPC_OMX_ErrorNone)
			{
				DOMX_ERROR("RPC Ipc Destroy failed");
			}
		}
		if (bMutex)
		{
			nInstanceCount--;
			if (nInstanceCount == 0)
			{
				/* All clients are gone, now safe to release wake lock */
				release_wake_lock(kDomxRpcWakeLock);
			}

			TIMM_OSAL_MutexRelease(pCreateMutex);
		}
		if (pRPCCtx)
		{
			TIMM_OSAL_Free(pRPCCtx);
			pRPCCtx = NULL;
		}
	} else
	{
		TIMM_OSAL_MutexRelease(pCreateMutex);
	}
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_InstanceDeInit()
 * @brief This function Removes or deinitializes RCM client instances. This also manages the number of active users
 *        of a given RCM client
 * @param cComponentName  : Pointer to the Components Name that is active user of the RCM client to be deinitialized
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

RPC_OMX_ERRORTYPE RPC_InstanceDeInit(RPC_OMX_HANDLE hRPCCtx)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone,
	    eTmpError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_BOOL bMutex = OMX_FALSE;
	RPC_OMX_CONTEXT *pRPCCtx = hRPCCtx;
	OMX_U32 i = 0;

	eError = TIMM_OSAL_MutexObtain(pCreateMutex, TIMM_OSAL_SUSPEND);
	RPC_assert(eError == TIMM_OSAL_ERR_NONE,
	    RPC_OMX_ErrorInsufficientResources,
	    "Mutex lock failed. InstanceDeInit failed completely");
	bMutex = OMX_TRUE;

	eTmpError = RPC_Util_ReleaseJobId(pRPCCtx, pRPCCtx->nJobId);
	if (eTmpError != RPC_OMX_ErrorNone)
	{
		TIMM_OSAL_Error("RPC Release Job ID failed");
		eRPCError = eTmpError;
	}

	nInstanceCount--;
	/*For last instance, shut down everything */
	if (nInstanceCount == 0)
	{
		currentNumOfComps = 0;

		/*Free up any remaining tiler buffers */
		for (i = 0; i < MAX_NUMBER_OF_TILER_BUFFERS_PER_PROCESS; i++)
		{
			if (tilerBuffers[i])
			{
				DOMX_DEBUG("Tiler Buffer Freed = 0x%x",
				    tilerBuffers[i]);
				SysLinkMemUtils_free(sizeof(UInt32),
				    (UInt32 *) & tilerBuffers[i]);
				tilerBuffers[i] = 0;
			}
		}

		eTmpError = _RPC_ClientDestroy();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("RPC ClientDestroy failed");
			eRPCError = eTmpError;
		}

		eTmpError = RPC_ModDeInit();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("RPC ModDeInit failed");
			eRPCError = eTmpError;
		}

		eTmpError = _RPC_IpcDestroy();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("ipc destroy failed");
			eRPCError = eTmpError;
		}

		ducatiFault = OMX_FALSE;
#ifdef SET_WATCHDOG_TIMEOUT
		if (bWatchDogSem)
		{
			TIMM_OSAL_SemaphoreRelease(pWatchDogSem);;//Release WatchDog Semaphore
			bWatchDogSem = OMX_FALSE;
			if (SUCCESS !=  pthread_join(watchDogFaultHandler, NULL))
			{
				DOMX_ERROR("WatchDog Join thread failed");
				eRPCError = RPC_OMX_ErrorUndefined;
			}
		}
#endif
		/* All clients are gone, now safe to release wake lock */
		release_wake_lock(kDomxRpcWakeLock);
	}

      EXIT:
	if (bMutex)
		TIMM_OSAL_MutexRelease(pCreateMutex);
	TIMM_OSAL_Free(hRPCCtx);

	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_ModDeInit()
 * @brief This function Removes or deinitializes RCM client instances. This also manages the number of active users
 *        of a given RCM client
 * @param cComponentName  : Pointer to the Components Name that is active user of the RCM client to be deinitialized
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
 /* =========================================================================== */
RPC_OMX_ERRORTYPE RPC_ModDeInit(void)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 status = 0;
	OMX_U32 i = 0;

	DOMX_ENTER("");

	if (rcmSrvHndl)
	{
		status = RcmServer_removeSymbol(rcmSrvHndl, "fxnExit");
		if (status < 0)
		{
			DOMX_ERROR
			    ("RCM Server remove symbol failed, status = %d",
			    status);
			eRPCError = RPC_OMX_RCM_ServerFail;
		}

		status = RcmServer_removeSymbol(rcmSrvHndl,
		    "getFxnIndexFromRemote_skel");
		if (status < 0)
		{
			DOMX_ERROR
			    ("RCM Server remove symbol failed, status = %d",
			    status);
			eRPCError = RPC_OMX_RCM_ServerFail;
		}

		for (i = 0; i < MAX_FUNCTION_LIST; i++)
		{
			status =
			    RcmServer_removeSymbol(rcmSrvHndl, rpcFxns[i]);
			if (status < 0)
			{
				DOMX_ERROR
				    ("RCM Server remove symbol failed, status = %d",
				    status);
				eRPCError = RPC_OMX_RCM_ServerFail;
			}
		}

		status = RcmServer_delete(&rcmSrvHndl);
		if (status < 0)
		{
			TIMM_OSAL_Error
			    ("RCM Server delete failed, status = %d", status);
			eRPCError = RPC_OMX_RCM_ServerFail;
		}
		rcmSrvHndl = NULL;
	}

	RcmServer_exit();

	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_ModInit()
 * @brief This function Creates the Default RCM servers on current processor
 * @param Void
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
 /* =========================================================================== */
RPC_OMX_ERRORTYPE RPC_ModInit(void)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone,
	    eTmpError = RPC_OMX_ErrorNone;
	OMX_U32 i = 0, j = 0, fxIndx = 0;
	OMX_S32 status = 0;
	RcmServer_Params rcmSrvParams;
	OMX_BOOL bCallDestroyIfErr = OMX_FALSE;
	OMX_S8 cRcmServerNameLocal[MAX_SERVER_NAME_LENGTH];
	RcmServer_ThreadPoolDesc sPoolDescArr[] = {
		{
			    RPC_NAME_FOR_GENERAL_POOL,
			    RPC_NUM_THREADS_FOR_GENERAL_POOL,
			    RPC_THREAD_PRIORITY_FOR_GENERAL_POOL,
			    RPC_OS_THREAD_PRIORITY_FOR_GENERAL_POOL,
			    RPC_THREAD_STACKSIZE_FOR_GENERAL_POOL,
		    RPC_THREAD_STACKSEG_FOR_GENERAL_POOL}
	};

	DOMX_ENTER("");

	/*Generate a name for the server */
	eRPCError =
	    RPC_UTIL_GenerateLocalServerName((OMX_STRING)
	    cRcmServerNameLocal);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone,
	    RPC_OMX_ErrorInsufficientResources,
	    "Server name generation failed");

	DOMX_DEBUG("RCM Server Name = %s", cRcmServerNameLocal);

	for (i = 0; i < CORE_MAX; i++)
	{
		rpcHndl[i].rcmHndl[LOCAL_CORE_ID] = NULL;
		rpcHndl[i].heapId[LOCAL_CORE_ID] = heapIdArray[LOCAL_CORE_ID];

		for (j = 0; j < MAX_FUNCTION_LIST; j++)
		{
			rpcHndl[i].rpcFxns[j].rpcFxnIdx = 0;
			rpcHndl[i].rpcFxns[j].FxnName = rpcFxns[j];
		}
		rpcHndl[i].NumOfTXUsers = 0;
	}

//RCM Server config
	RcmServer_init();
	bCallDestroyIfErr = OMX_TRUE;

	DOMX_DEBUG("Calling Server params init");
	status = RcmServer_Params_init(&rcmSrvParams);
	RPC_assert(status >= 0, RPC_OMX_RCM_ServerFail,
	    "Server_setup failed");

	rcmSrvParams.workerPools.length = RPC_NUM_RCM_WORKER_POOLS;
	rcmSrvParams.workerPools.elem = sPoolDescArr;
	DOMX_DEBUG("RCM Server Name = %s", cRcmServerNameLocal);
	status = RcmServer_create((char *)cRcmServerNameLocal, &rcmSrvParams,
	    &rcmSrvHndl);
	RPC_assert(status >= 0, RPC_OMX_RCM_ServerFail,
	    "Server_create failed");
	DOMX_DEBUG("Server created");

	status = RcmServer_addSymbol(rcmSrvHndl, "fxnExit", fxnExit,
	    (UInt32 *) & fxIndx);
	RPC_assert((status >= 0 &&
		fxIndx != 0xFFFFFFFF), RPC_OMX_RCM_ServerFail,
	    "Server_addSymbol failed");

	status = RcmServer_addSymbol(rcmSrvHndl, "getFxnIndexFromRemote_skel",
	    getFxnIndexFromRemote_skel,
	    (UInt32 *) & getFxnIndexFromRemote_skelIdx);
	RPC_assert((status >= 0 &&
		getFxnIndexFromRemote_skelIdx != 0xFFFFFFFF),
	    RPC_OMX_RCM_ServerFail, "Server_addSymbol failed");

	for (i = 0; i < MAX_FUNCTION_LIST; i++)
	{
		status =
		    RcmServer_addSymbol(rcmSrvHndl, rpcFxns[i],
		    rpcSkelFxns[i].FxnPtr,
		    (UInt32 *) (&rpcHndl[LOCAL_CORE_ID].rpcFxns[i].
			rpcFxnIdx));

		RPC_assert((status >= 0 && fxIndx != 0xFFFFFFFF),
		    RPC_OMX_RCM_ServerFail, "Server_addSymbol failed");
		DOMX_DEBUG("%d. Function %s registered", i + 1, rpcFxns[i]);
	}

	//Start the RCM server thread
	RcmServer_start(rcmSrvHndl);
	DOMX_DEBUG("Running RcmServer");

      EXIT:
	DOMX_EXIT("");
	if (eRPCError != RPC_OMX_ErrorNone && bCallDestroyIfErr)
	{
		eTmpError = RPC_ModDeInit();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("ModDeInit failed");
		}
	}
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name fxnExit()
 * @brief
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

static Int32 fxnExit(UInt32 size, UInt32 * data)
{
	OMX_S16 status = 0;	/* success */

	DOMX_DEBUG("Executing fxnExit ");
	DOMX_DEBUG("Releasing testcase semaphore:");
/*
    if(MultiProc_getId(NULL) == APPM3_PROC)
        TIMM_OSAL_SemaphoreRelease (testSem);
    else
        TIMM_OSAL_SemaphoreRelease (testSemSys);
*/
	return status;
}

/* ===========================================================================*/
/**
 * @name fxn_exit_caller()
 * @brief
 * @param void
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE fxn_exit_caller(void)
{
	RcmClient_Message *rcmMsg = NULL;
	RcmClient_Message *rcmRetMsg = NULL;
	OMX_S32 status;
	RPC_OMX_ERRORTYPE rpcError = RPC_OMX_ErrorNone;

	DOMX_ENTER("");
	status = RcmClient_alloc(rcmHndl, sizeof(RcmClient_Message), &rcmMsg);

	if (status < 0)
	{
		DOMX_DEBUG(" Error in allocating RCM msg");
		rpcError = RPC_OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	rcmMsg->fxnIdx = fxnExitidx;
	//Sending Terminate messsage to remote processor
	status = RcmClient_execDpc(rcmHndl, rcmMsg, &rcmRetMsg);

	if (status < 0)
	{
		DOMX_DEBUG(" Error RcmClient_execDpc failed ");
		rpcError = RPC_OMX_RCM_ErrorExecFail;
		goto EXIT;
	}

      EXIT:
	DOMX_EXIT("");
	return rpcError;

}



/* ===========================================================================*/
/**
 * @name : _RPC_GetRemoteIndices()
 * @brief : Calls getFxnIndexFromRemote_skel on the remote core to get all
 *          remote indices in one go.
 * @param hRcmHandle  : The RCM client handle.
 * @param nFxnIdx     : Function index of the remote function that will give
 *                      the indices.
 * @return RPC_OMX_ErrorNone = Successful
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE _RPC_GetRemoteIndices(RPC_OMX_HANDLE hRcmHandle,
    OMX_U32 nFxnIdx)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	RcmClient_Handle hRcmClient = (RcmClient_Handle) hRcmHandle;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;
	OMX_U32 nPos = 0, i = 0, nPacketSize = PACKET_SIZE;
	OMX_S32 status = 0;

	DOMX_ENTER("");
	status = RcmClient_alloc(hRcmClient, nPacketSize, &pPacket);
	RPC_assert(status >= 0, RPC_OMX_ErrorInsufficientResources,
	    "Error allocating RCM message frame");
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];
	pPacket->fxnIdx = nFxnIdx;
	status = RcmClient_exec(hRcmClient, pPacket, &pRetPacket);
	if (status < 0)
	{
		RcmClient_free(hRcmClient, pPacket);
		pPacket = NULL;
		RPC_assert(0, RPC_OMX_RCM_ErrorExecFail,
		    "RcmClient_exec failed");
	}
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	for (i = 0; i < MAX_FUNCTION_LIST; i++)
	{
		RPC_GETFIELDVALUE(pMsgBody, nPos,
		    rpcHndl[TARGET_CORE_ID].rpcFxns[i].rpcFxnIdx, RPC_INDEX);

		DOMX_DEBUG("%d. Function index obtained: %d for %s", i + 1,
		    rpcHndl[TARGET_CORE_ID].rpcFxns[i].rpcFxnIdx,
		    rpcHndl[TARGET_CORE_ID].rpcFxns[i].FxnName);
	}
	RcmClient_free(rcmHndl, pRetPacket);

      EXIT:
	DOMX_EXIT("eRPCError : %d", eRPCError);
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name : getFxnIndexFromRemote_skel()
 * @brief : Puts all the OMX function indices in the message body in one shot.
 *          The remote core can then retrieve all the indices with one call
 *          rather than calling get function index for all functions.
 * @param size   : Size of the incoming RCM message (parameter used in the RCM
 *                 alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 *
 */
/* ===========================================================================*/
Int32 getFxnIndexFromRemote_skel(UInt32 size, UInt32 * data)
{
	OMX_U32 i = 0, nPos = 0;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;

	DOMX_ENTER("");

	pRPCMsg = (RPC_OMX_MESSAGE *) (data);
	pMsgBody = &pRPCMsg->msgBody[0];

	for (i = 0; i < MAX_FUNCTION_LIST; i++)
	{
		RPC_SETFIELDVALUE(pMsgBody, nPos,
		    rpcHndl[LOCAL_CORE_ID].rpcFxns[i].rpcFxnIdx, RPC_INDEX);
		DOMX_DEBUG("Function index added for %d %s : %d", i + 1,
		    rpcHndl[LOCAL_CORE_ID].rpcFxns[i].FxnName,
		    rpcHndl[LOCAL_CORE_ID].rpcFxns[i].rpcFxnIdx);
	}

	DOMX_EXIT("");
	return 0;
}



/* ===========================================================================*/
/**
 * @name _RPC_GetRemoteDOMXVersion()
 * @brief This function is used by DOMX to communicate with its remote
 *        counterpart on Ducati and ensure that they are in sync. This function
 *        does not have any OMX counterpart and is used only internally by DOMX.
 * @param hRcmHandle  : The RCM client handle.
 * @param nFxnIdx     : Function index of the remote function that will give the
 *                      version
 * @param *nVer       : The version no. returned by the remote side.
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE _RPC_GetRemoteDOMXVersion(RPC_OMX_HANDLE hRcmHandle,
    OMX_U32 nFxnIdx, OMX_U32 * nVer)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	RcmClient_Handle hRcmClient = (RcmClient_Handle) hRcmHandle;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;
	OMX_U32 nPos = 0, nPacketSize = PACKET_SIZE;
	OMX_S32 status = 0;

	status = RcmClient_alloc(hRcmClient, nPacketSize, &pPacket);
	RPC_assert(status >= 0, RPC_OMX_ErrorInsufficientResources,
	    "Error Allocating RCM Message Frame");
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];
	pPacket->fxnIdx = nFxnIdx;
	status = RcmClient_exec(hRcmClient, pPacket, &pRetPacket);
	if (status < 0)
	{
		RcmClient_free(hRcmClient, pPacket);
		pPacket = NULL;
		RPC_assert(0, RPC_OMX_RCM_ErrorExecFail,
		    "RcmClient_exec failed");
	}
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	/*Get the DOMX version */
	RPC_GETFIELDVALUE(pMsgBody, nPos, *nVer, OMX_U32);
	RcmClient_free(rcmHndl, pRetPacket);

      EXIT:
	return eRPCError;
}



/*===============================================================*/
/** @fn _RPC_ClientCreate : This function creates RCM Client and gets symbol
 *                          indices for all functions.
 *
 */
/*===============================================================*/
RPC_OMX_ERRORTYPE _RPC_ClientCreate(OMX_STRING cComponentName)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone,
	    eTmpError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	RcmClient_Params rcmParams;
	OMX_S8 cRcmServerNameTarget[MAX_SERVER_NAME_LENGTH];;
	OMX_S32 status = 0;
	OMX_BOOL bCallDestroyIfErr = OMX_FALSE;
	OMX_U32 nVer = 0, i = 0;
	OMX_U32 nGetDOMXVersionIdx = 0, nGetFxnFromRemoteIdx = 0;

	eRPCError = RPC_UTIL_GetTargetServerName(cComponentName,
	    (OMX_STRING) cRcmServerNameTarget);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
	    "Get target server name failed");
	DOMX_DEBUG(" RCM Server Name To connected to: %s",
	    cRcmServerNameTarget);

	/* RCM client configuration */
	RcmClient_init();
	bCallDestroyIfErr = OMX_TRUE;

	status = RcmClient_Params_init(&rcmParams);
	RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail,
	    "Client_Params_init failed");
	rcmParams.heapId = heapIdArray[LOCAL_CORE_ID];
	DOMX_DEBUG(" Heap ID configured : %d", rcmParams.heapId);
	if (CHIRON_IPC_FLAG && (LOCAL_CORE_ID == CORE_APPM3))
	{
		/*Hardcoded as heapId 0 when running on APPM3. This is done only for
		   WHEN RUNNING ON MPU-APPM3 - "Both use HeapId 0" Work around will be
		   fixed when Independent Heaps are available across MPU and APPM3 */
		rcmParams.heapId = 1;
	}
	DOMX_DEBUG(" Heap ID configured : %d", rcmParams.heapId);

	DOMX_DEBUG("Calling client create with server name = %s",
	    cRcmServerNameTarget);

	/*We wait for Ducati base image to be loaded in _RPC_IpcSetup function.
	  However, after loading Ducati needs to run for some time before the
	  RCM server on Ducati is created. If the client create function here
	  is called before the Ducati server is up, it'll fail. Hence if the
	  client create function fails, keep trying again for some time with
	  a short sleep between successive tries to allow the Ducati server
	  to come up */
	for (i = 0; i < RPC_CLIENT_CREATE_MAX_TRIES; i++)
	{
		status =
		    RcmClient_create((char *)cRcmServerNameTarget, &rcmParams,
		    &rcmHndl);
		if (status < 0)
		{
			DOMX_DEBUG("Client create failed. \
			    Will sleep for some time and try again.");
			eError =
			    TIMM_OSAL_SleepTask
			    (RPC_CLIENT_CREATE_TIME_BETWEEN_TRIES);
			if (eError != TIMM_OSAL_ERR_NONE)
			{
				DOMX_WARN("Sleep task failed. \
				    Will continue without sleeping");
			}
		} else
		{
			DOMX_DEBUG("Client created. Connected to Server");
			break;
		}
	}
	RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail,
	    "RCM ClientCreate failed. Cannot Establish the connection");

	/*Checking DOMX version */
	DOMX_DEBUG("Checking DOMX version");
	status = RcmClient_getSymbolIndex(rcmHndl, "getDOMXVersion",
	    (UInt32 *) (&nGetDOMXVersionIdx));
	RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail,
	    "GetSymbolIndex failed");
	eRPCError =
	    _RPC_GetRemoteDOMXVersion(rcmHndl, nGetDOMXVersionIdx, &nVer);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
	    "Failed to get remote DOMX version");
	RPC_assert(nVer == DOMX_VERSION, RPC_OMX_ErrorUndefined,
	    "Version mismatch detected - A9 and Ducati DOMX versions not in sync");

	/* Getting remote function indices */
	DOMX_DEBUG("Getting Symbols");
	status =
	    RcmClient_getSymbolIndex(rcmHndl, "getFxnIndexFromRemote_skel",
	    (UInt32 *) (&nGetFxnFromRemoteIdx));
	RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail,
	    "GetSymbolIndex failed");
	eRPCError = _RPC_GetRemoteIndices(rcmHndl, nGetFxnFromRemoteIdx);
	RPC_assert(eRPCError == RPC_OMX_ErrorNone, eRPCError,
	    "Failed to get remote function indices");

	DOMX_DEBUG("Calling RCM_getSymbolIndex(fxnExit)");
	status = RcmClient_getSymbolIndex(rcmHndl, "fxnExit",
	    (UInt32 *) (&fxnExitidx));
	RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail,
	    "GetSymbolIndex failed");

      EXIT:
	if (eRPCError != RPC_OMX_ErrorNone && bCallDestroyIfErr)
	{
		eTmpError = _RPC_ClientDestroy();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("Client destruction failed");
		}
	}
	return eRPCError;
}



/*===============================================================*/
/** @fn _RPC_ClientDestroy : This function destroys RCM Client.
 *
 */
/*===============================================================*/
RPC_OMX_ERRORTYPE _RPC_ClientDestroy()
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 status = 0;

	if (rcmHndl)
	{
		status = RcmClient_delete(&rcmHndl);
		if (status < 0)
		{
			TIMM_OSAL_Error
			    ("Error in RcmClient_delete. Error Code: %d",
			    status);
			eRPCError = RPC_OMX_RCM_ClientFail;
		}
		rcmHndl = NULL;
	}

	RcmClient_exit();

	return eRPCError;
}



/*===============================================================*/
/** @fn _RPC_IpcSetup : This function performs basic ipc setup.
 *
 */
/*===============================================================*/
RPC_OMX_ERRORTYPE _RPC_IpcSetup()
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone,
	    eTmpError = RPC_OMX_ErrorNone;
	OMX_U16 procId = 0, remoteId = 0;
	Ipc_Config config;
	OMX_S32 status = 0;
	OMX_BOOL bCallDestroyIfErr = OMX_FALSE;
	ProcMgr_AttachParams attachParams;

	DOMX_DEBUG("Setup IPC components");

	Ipc_getConfig(&config);
	status = Ipc_setup(&config);
	RPC_assert(status >= 0, RPC_OMX_ErrorHardware, "SysMgr Setup failed");
	bCallDestroyIfErr = OMX_TRUE;

	procId = MultiProc_getId(SYSM3_PROC_NAME);
	remoteId = MultiProc_getId(APPM3_PROC_NAME);

	/*Call open for SysM3 */
	status = ProcMgr_open(&procMgrHandle, procId);
	RPC_assert(status >= 0, RPC_OMX_ErrorHardware,
	    "ProcMgr open failed for SysM3");
	DOMX_DEBUG("ProcMgr_open Status [0x%x]", status);
	ProcMgr_getAttachParams(NULL, &attachParams);
	status = ProcMgr_attach(procMgrHandle, &attachParams);
	RPC_assert(status >= 0, RPC_OMX_ErrorHardware,
	    "Error in ProcMgr_attach");
	DOMX_DEBUG("ProcMgr_attach status: [0x%x]\n", status);

	/*Call open for AppM3 */
	status = ProcMgr_open(&procMgrHandle1, remoteId);
	RPC_assert(status >= 0, RPC_OMX_ErrorHardware,
	    "ProcMgr open failed for appM3");
	DOMX_DEBUG("ProcMgr_open Status [0x%x]\n", status);
	ProcMgr_getAttachParams(NULL, &attachParams);
	status = ProcMgr_attach(procMgrHandle1, &attachParams);
	RPC_assert(status >= 0, RPC_OMX_ErrorHardware,
	    "Error in ProcMgr_attach");
	DOMX_DEBUG("ProcMgr_attach status: [0x%x]\n", status);

	/*Wait for PROC_START on AppM3 - this is to ensure that Ducati is up
	  and running before the use case can start*/
	status = ProcMgr_waitForEvent(1, PROC_START,
	    RPC_TIMEOUT_FOR_DUCATI_IMAGE_LOAD);
	RPC_assert(status != PROCMGR_E_TIMEOUT, RPC_OMX_ErrorTimeout,
	    "Ducati base image loading timed out");
	RPC_assert(status == PROCMGR_SUCCESS, RPC_OMX_ErrorUndefined,
	    "Error while waiting for PROC_START");
	DOMX_DEBUG("ProcMgr_waitForMultipleEvents successful");

      EXIT:
	if (eRPCError != RPC_OMX_ErrorNone && bCallDestroyIfErr)
	{
		eTmpError = _RPC_IpcDestroy();
		if (eTmpError != RPC_OMX_ErrorNone)
		{
			TIMM_OSAL_Error("ipc destroy failed");
		}
	}
	DOMX_DEBUG("Leaving _RPC_IpcSetup()");
	return eRPCError;
}



/*===============================================================*/
/** @fn _RPC_IpcDestroy : This function destroys the basic ipc modules.
 *
 */
/*===============================================================*/
RPC_OMX_ERRORTYPE _RPC_IpcDestroy()
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 status = 0;

	if (procMgrHandle1)
	{
		status = ProcMgr_detach(procMgrHandle1);
		if (status < 0)
		{
			eRPCError = RPC_OMX_ErrorHardware;
			TIMM_OSAL_Error("\nError in ProcMgr_detach 0x%x\n",
			    status);
		}
		status = ProcMgr_close(&procMgrHandle1);
		procMgrHandle1 = NULL;
		if (status < 0)
		{
			eRPCError = RPC_OMX_ErrorHardware;
			TIMM_OSAL_Error("\nError in ProcMgr_close 0x%x\n",
			    status);
		}
	}

	if (procMgrHandle)
	{
		status = ProcMgr_detach(procMgrHandle);
		if (status < 0)
		{
			eRPCError = RPC_OMX_ErrorHardware;
			TIMM_OSAL_Error("\nError in ProcMgr_detach 0x%x\n",
			    status);
		}
		status = ProcMgr_close(&procMgrHandle);
		procMgrHandle = NULL;
		if (status < 0)
		{
			eRPCError = RPC_OMX_ErrorHardware;
			TIMM_OSAL_Error("Error in ProcMgr_close 0x%x",
			    status);
		}
	}

	DOMX_DEBUG("Closing IPC");
	status = Ipc_destroy();
	if (status < 0)
	{
		eRPCError = RPC_OMX_ErrorHardware;
		DOMX_ERROR("Error in Ipc_destroy 0x%x", status);
	}

	return eRPCError;
}



/*===============================================================*/
/** @fn RPC_Setup : This function is called when the the DOMX library is
 *                  loaded. It creates a mutex, which is used to synchronize
 *                  init/deinit in multi-instance scenarios.
 *
 */
/*===============================================================*/
void __attribute__ ((constructor)) RPC_Setup(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexCreate(&pCreateMutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of default mutex failed");
	}

	eError = TIMM_OSAL_MutexCreate(&pFaultMutex);

	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of ducati fault mutex failed.");
	}

	eError = TIMM_OSAL_MutexCreate(&pTilerMutex);

	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of tiler mutex failed.");
	}

}



/*===============================================================*/
/** @fn RPC_Destroy : This function is called when the the DOMX library is
 *                    unloaded. It destroys the mutex which was created by
 *                    RPC_Setup().
 *
 */
/*===============================================================*/
void __attribute__ ((destructor)) RPC_Destroy(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexDelete(pCreateMutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Destruction of default mutex failed");
	}

	eError = TIMM_OSAL_MutexDelete(pFaultMutex);

	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Deletion of ducati fault mutex failed.");
	}

	eError = TIMM_OSAL_MutexDelete(pTilerMutex);

	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Deletion of tiler mutex failed.");
	}
}

/*===============================================================*/
/** @fn RPC_MemAlloc
 */
/*===============================================================*/
Int32 RPC_MemAlloc(UInt32 * dataSize, UInt32 * data)
{
	OMX_U32 i;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	eError = TIMM_OSAL_MutexObtain(pTilerMutex, TIMM_OSAL_SUSPEND);
	RPC_assert(eError == TIMM_OSAL_ERR_NONE,
	    RPC_OMX_ErrorInsufficientResources,
	    "Mutex lock failed. RPC_MemAlloc failed completely");

	for (i = 0; i < MAX_NUMBER_OF_TILER_BUFFERS_PER_PROCESS; i++)
	{
		if (tilerBuffers[i] == 0)
		{
			tilerBuffers[i] =
			    SysLinkMemUtils_alloc(dataSize, data);
			DOMX_DEBUG("Tiler Buffer Allocated = 0x%x",
			    tilerBuffers[i]);
			if (tilerBuffers[i] == 0)
			{
				DOMX_ERROR("Null pointer allocated.");
			}
			TIMM_OSAL_MutexRelease(pTilerMutex);
			return tilerBuffers[i];
		}
	}
	TIMM_OSAL_MutexRelease(pTilerMutex);
      EXIT:
	DOMX_ERROR("Not enough space for book keeping of tiler buffers.");
	return 0;
}

/*===============================================================*/
/** @fn RPC_MemFree
 */
/*===============================================================*/
Int32 RPC_MemFree(UInt32 * dataSize, UInt32 * data)
{
	OMX_U32 i;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	Int32 ret;

	eError = TIMM_OSAL_MutexObtain(pTilerMutex, TIMM_OSAL_SUSPEND);
	RPC_assert(eError == TIMM_OSAL_ERR_NONE,
	    RPC_OMX_ErrorInsufficientResources,
	    "Mutex lock failed. RPC_MemFree failed completely");

	for (i = 0; i < MAX_NUMBER_OF_TILER_BUFFERS_PER_PROCESS; i++)
	{
		if (tilerBuffers[i] == (OMX_U32) * data)
		{
			DOMX_DEBUG("Tiler Buffer Freed = 0x%x",
			    tilerBuffers[i]);
			tilerBuffers[i] = 0;
			ret = SysLinkMemUtils_free(dataSize, data);
			TIMM_OSAL_MutexRelease(pTilerMutex);
			return ret;
		}
	}
	TIMM_OSAL_MutexRelease(pTilerMutex);
      EXIT:
	DOMX_ERROR("Invalid tiler buffer free");
	return 0;
}


/*===============================================================*/
/** @fn RPC_DucatiFaultHandler : This function listens to Ducati
 *                    MMU faults and Ducati Proc Stop due to
 *                    Syslink Daemon Crash and posts error notificaion
 *                    to the Client. It also sets a global variable
 *                    which prevents any further calls to Ducati.
 */
/*===============================================================*/
static void *RPC_DucatiFaultHandler(void *data)
{
	OMX_U32 status = 0;
	OMX_U32 count;
	/* The fault handler does not wait for PROC_START event.
	   The events array values are used to index into the
	   events_name array. The values are PROC_MMU_FAULT=0,
	   PROC_ERROR=1, PROC_STOP=2, and PROC_WATCHDOG=4.
	   Thus the events_name array contains "PROC_START" as
	   a place holder. Otherwise indexing into events_name
	   using values of events will print incorrect event
	   received. */
	char *events_name[] =
	    { "MMUFault", "PROC_ERROR", "PROC_STOP", "PROC_START",
		"PROC_WATCHDOG"
	};
	ProcMgr_EventType events[] =
	    { PROC_MMU_FAULT, PROC_ERROR, PROC_STOP, PROC_WATCHDOG };

	UInt i;
	RPC_OMX_ERRORTYPE tRPCError = RPC_OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;

	DOMX_ENTER("Starting the DOMX MMU fault recovery handler\n");
	DOMX_DEBUG("Awaiting fatal errors from AppM3, patch applied\n");

	status = ProcMgr_waitForMultipleEvents(1,	/* AppM3 ID */
	    events, 4, -1, &i);

	if (status != PROCMGR_SUCCESS)
	{
		DOMX_WARN("Error ProcMgr_waitForMultipleEvents %d\n",
		    status);
		DOMX_WARN("Cannot perform error recovery.\n");
		goto EXIT;
	}

	DOMX_ERROR("Received %s notification from %s\n",
	    events_name[events[i]], "AppM3");

	ducatiFault = OMX_TRUE;

#ifdef SET_WATCHDOG_TIMEOUT
	if (SUCCESS != pthread_create(&watchDogFaultHandler,
		              NULL, RPC_WatchDogFaultHandler, NULL))
	{
		DOMX_DEBUG("Error Creating WatchDog Thread \n");
		goto EXIT;
	}
#endif
	for (i = 0; i < MAX_NUM_COMPS_PER_PROCESS; i++)
	{
		if (componentTable[i])
		{
			pHandle = (OMX_COMPONENTTYPE *) componentTable[i];

			pCompPrv =
			    (PROXY_COMPONENT_PRIVATE *)
			    pHandle->pComponentPrivate;

                        DOMX_DEBUG("Sending fault Notification\n");

			tRPCError =
			    pCompPrv->proxyEventHandler(componentTable[i],
			    pCompPrv->pILAppData, OMX_EventError,
			    OMX_ErrorHardware, 0, NULL);

                        DOMX_DEBUG("Sent fault Notification\n");
		}
	}

      EXIT:
	DOMX_EXIT("Closing the DOMX MMU fault recovery handler.\n");
        bDFH_Created = false;
	return NULL;
}

#ifdef SET_WATCHDOG_TIMEOUT
static void *RPC_WatchDogFaultHandler(void *data)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	DOMX_ENTER("ENTER WDT fault recovery handler.\n");

	if (TIMM_OSAL_SemaphoreCreate(&pWatchDogSem, 0) != TIMM_OSAL_ERR_NONE)
	{
		DOMX_ERROR("WatchDog Semaphore Create failed!");
	}
	else
	{
		DOMX_DEBUG("WatchDog Semaphore Create successfully");
		bWatchDogSem = OMX_TRUE;
		eError = TIMM_OSAL_SemaphoreObtain(pWatchDogSem, WATCHDOG_TIMEOUT);
		if ( eError == TIMM_OSAL_ERR_NONE)
		{
			DOMX_DEBUG("WatchDog Semaphore Signaled");
			goto EXIT;
		}
		else
		{
			DOMX_DEBUG("WatchDog Obtain Failed ");
			if(eError == TIMM_OSAL_WAR_TIMEOUT)
			{
				DOMX_DEBUG("WatchDog Semaphore Timed Out Exitting Process");
			}
                        TIMM_OSAL_SemaphoreDelete(pWatchDogSem);
			abort();
		}
	}
EXIT:
	TIMM_OSAL_SemaphoreDelete(pWatchDogSem);
	DOMX_EXIT("Closing the WDT fault recovery handler.\n");
	return NULL;
}
#endif
