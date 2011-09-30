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
 *  @file  omx_rpc_stub.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC Stub implementations.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 30-Apr-2010 Abhishek Ranka : Fixed GetExtension issue
 *!
 *! 29-Mar-2010 Abhishek Ranka : Revamped DOMX implementation
 *!
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/
/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <string.h>
#include <stdio.h>

#include "omx_rpc.h"
#include "omx_rpc_utils.h"
#include "omx_proxy_common.h"
#include "omx_rpc_stub.h"
#include <OMX_TI_Common.h>
#include <timm_osal_interfaces.h>
#include <MultiProc.h>

/******************************************************************
 *   EXTERNS
 ******************************************************************/
extern RPC_Object rpcHndl[CORE_MAX];
extern COREID TARGET_CORE_ID;

extern OMX_BOOL ducatiFault;
/******************************************************************
 *   MACROS - LOCAL
 ******************************************************************/

/* When this is defined ETB/FTB calls are made in sync mode. Undefining will
 * result in these calls being sent via async mode. Sync mode leads to correct
 * functionality as per OMX spec but has a slight performance penalty. Async
 * mode sacrifices strict adherence to spec for some gain in performance. */
#define RPC_SYNC_MODE


#define RPC_getPacket(HRCM, nPacketSize, pPacket) do { \
    status = RcmClient_alloc(HRCM, nPacketSize, &pPacket); \
    RPC_assert(status >= 0, RPC_OMX_ErrorInsufficientResources, \
           "Error Allocating RCM Message Frame"); \
    } while(0)

#define RPC_sendPacket_sync(HRCM, pPacket, fxnIdx, pRetPacket) do { \
    pPacket->fxnIdx = fxnIdx; \
    status = RcmClient_exec(HRCM, pPacket, &pRetPacket); \
    if(status < 0) { \
    RPC_freePacket(HRCM, pRetPacket); \
    pPacket = NULL; \
    pRetPacket = NULL; \
    RPC_assert(0, RPC_OMX_RCM_ErrorExecFail, \
           "RcmClient_exec failed"); \
    } \
    } while(0)

#define RPC_sendPacket_async(HRCM, pPacket, fxnIdx) do { \
    pPacket->fxnIdx = fxnIdx; \
    status = RcmClient_execCmd(HRCM, pPacket); \
    if(status < 0) { \
    RPC_freePacket(HRCM, pPacket); \
    pPacket = NULL; \
    RPC_assert(0, RPC_OMX_RCM_ErrorExecFail, \
           "RcmClient_exec failed"); \
    } \
    } while(0)

#define RPC_freePacket(HRCM, pPacket) do { \
   if(pPacket!=NULL) RcmClient_free(HRCM, pPacket); \
   } while(0)

#define RPC_checkAsyncErrors(rcmHndl, pPacket) do { \
    status = RcmClient_checkForError(rcmHndl, &pPacket); \
    if(status < 0) { \
        RPC_freePacket(rcmHndl, pPacket); \
        pPacket = NULL; \
    } \
    RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail, \
        "Async error check returned error"); \
    } while(0)

#define RPC_exitOnDucatiFault() do { \
   if(ducatiFault == OMX_TRUE) { \
       RPC_assert(0, RPC_OMX_ErrorHardware, "Ducati Fault Detected. Function exec blocked"); \
   } \
   } while(0)

/* ===========================================================================*/
/**
 * @name RPC_GetHandle()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param hComp: This is the handle into which the remote Component handle is returned.
                        it's handle with actual OMX Component handle that recides on the specified core
 * @param cComponentName: Name of the component that is to be created on Remote core
 * @param pCallBacks:
 * @param pAppData: equal to pAppData passed in original GetHandle.
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
 *                    This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetHandle(RPC_OMX_HANDLE hRPCCtx,
    OMX_STRING cComponentName, OMX_PTR pAppData,
    OMX_CALLBACKTYPE * pCallBacks, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;
	OMX_U32 dataOffset = 0;
	OMX_U32 dataOffset2 = 0;
	OMX_U32 offset = 0;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPos = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = NULL;
	OMX_HANDLETYPE hActualComp = NULL;

	OMX_S16 status;
	RPC_INDEX fxnIdx;
	OMX_STRING CallingCorercmServerName;

	OMX_S32 pid = 0;	//For TILER memory allocation changes

	DOMX_ENTER("");
	DOMX_DEBUG("RPC_GetHandle: Recieved GetHandle request from %s",
	    cComponentName);

	RPC_exitOnDucatiFault();

	RPC_UTIL_GetLocalServerName(cComponentName,
	    &CallingCorercmServerName);
	DOMX_DEBUG(" RCM Server Name Calling on Current Core: %s",
	    CallingCorercmServerName);

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_HANDLE].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>offset(cParameterName)|>pAppData|>offset(CallingCorercmServerName)|>pid|
	//>--cComponentName--|>--CallingCorercmServerName--|
	//<hComp]

	dataOffset =
	    sizeof(OMX_U32) + sizeof(OMX_PTR) + sizeof(OMX_U32) +
	    sizeof(OMX_S32);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, dataOffset, OMX_U32);
	//To update with RPC macros
	strcpy((char *)(pMsgBody + dataOffset), cComponentName);

	RPC_SETFIELDVALUE(pMsgBody, nPos, pAppData, OMX_PTR);

	dataOffset2 = dataOffset + 128;	//max size of omx comp name
	RPC_SETFIELDOFFSET(pMsgBody, nPos, dataOffset2, OMX_U32);
	//To update with RPC macros
	strcpy((char *)(pMsgBody + dataOffset2), CallingCorercmServerName);

	//Towards TILER memory allocation changes
	pid = getpid();
	RPC_SETFIELDVALUE(pMsgBody, nPos, pid, OMX_S32);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;

	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);

	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (*eCompReturn == OMX_ErrorNone)
	{
		offset = dataOffset2 + 32;	//max size of rcm server name
		RPC_GETFIELDVALUE(pMsgBody, offset, hComp, RPC_OMX_HANDLE);
		DOMX_DEBUG("Received Remote Handle 0x%x", hComp);
		hCtx->remoteHandle = hComp;
		/* The handle received above is used for all communications
		   with the remote component but is not the actual component
		   handle (it is actually the rpc context handle which
		   contains lot of other info). The handle recd. below is the
		   actual remote component handle. This is used at present for
		   mark buffer implementation since in that case it is not
		   feasible to send the context handle */
		RPC_GETFIELDVALUE(pMsgBody, offset, hActualComp,
		    OMX_HANDLETYPE);
		hCtx->hActualRemoteCompHandle = hActualComp;
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}




/* ===========================================================================*/
/**
 * @name RPC_FreeHandle()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FreeHandle(RPC_OMX_HANDLE hRPCCtx,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_S16 status;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_FREE_HANDLE].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp]
	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;

}

/* ===========================================================================*/
/**
 * @name RPC_SetParameter()
 * @brief
 * @param hComp: This is the handle on the Remote core, the proxy will replace
 *        it's handle with actual OMX Component handle that recides on the specified core
 * @param nParamIndex: same as nParamIndex received at the Proxy
 * @param pCompParam: same as nPCompParam recieved at the Proxy
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
          This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SetParameter(RPC_OMX_HANDLE hRPCCtx,
    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
    OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 structSize = 0;
	OMX_U32 offset = 0;

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_SET_PARAMETER].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);

	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nParamIndex|>offset(pCompParam)|>--pCompParam--]
	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nParamIndex, OMX_INDEXTYPE);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_INDEXTYPE) + sizeof(OMX_U32);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompParam);
	RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCompParam, structSize);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_GetParameter()
 * @brief
 * @param hComp: This is the handle on the Remote core, the proxy will replace
 *        it's handle with actual OMX Component handle that recides on the specified core
 * @param nParamIndex: same as nParamIndex received at the Proxy
 * @param pCompParam: same as nPCompParam recieved at the Proxy
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
          This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetParameter(RPC_OMX_HANDLE hRPCCtx,
    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 structSize = 0;
	OMX_U32 offset = 0;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_PARAMETER].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nParamIndex|>offset(pCompParam)|><--pCompParam--]
	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nParamIndex, OMX_INDEXTYPE);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_INDEXTYPE) + sizeof(OMX_U32);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompParam);
	RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCompParam, structSize);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{
		RPC_GETFIELDCOPYGEN(pMsgBody, offset, pCompParam, structSize);
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_SetConfig()
 * @brief
 * @param hComp: This is the handle on the Remote core, the proxy will replace
 *        it's handle with actual OMX Component handle that recides on the specified core
 * @param nParamIndex: same as nParamIndex received at the Proxy
 * @param pCompParam: same as nPCompParam recieved at the Proxy
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
          This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SetConfig(RPC_OMX_HANDLE hRPCCtx,
    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
    OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 structSize = 0;
	OMX_U32 offset = 0;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_SET_CONFIG].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nConfigIndex|>offset(pCompConfig)|>--pCompConfig--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nConfigIndex, OMX_INDEXTYPE);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_INDEXTYPE) + sizeof(OMX_U32);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompConfig);
	RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCompConfig, structSize);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_GetConfig()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetConfig(RPC_OMX_HANDLE hRPCCtx,
    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 structSize = 0;
	OMX_U32 offset = 0;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_CONFIG].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nConfigIndex|>offset(pCompConfig)|><--pCompConfig--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nConfigIndex, OMX_INDEXTYPE);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_INDEXTYPE) + sizeof(OMX_U32);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompConfig);
	RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCompConfig, structSize);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == RPC_OMX_ErrorNone)
	{
		RPC_GETFIELDCOPYGEN(pMsgBody, offset, pCompConfig,
		    structSize);
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_SendCommand()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SendCommand(RPC_OMX_HANDLE hRPCCtx,
    OMX_COMMANDTYPE eCmd, OMX_U32 nParam, OMX_PTR pCmdData,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 structSize = 0;
	OMX_U32 offset = 0;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_SEND_CMD].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>eCmd|nParam|>offset(pCmdData)|!><--pCmdData--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, eCmd, OMX_COMMANDTYPE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nParam, OMX_U32);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_COMMANDTYPE) +
	    sizeof(OMX_U32) + sizeof(OMX_U32);

	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	if (pCmdData != NULL && eCmd == OMX_CommandMarkBuffer)
	{
		/*The RPC_UTIL_GETSTRUCTSIZE will not work here since OMX_MARKTYPE structure
		   does not have nSize field */
		structSize = sizeof(OMX_MARKTYPE);
		RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCmdData, structSize);
	} else if (pCmdData != NULL)
	{
		structSize = RPC_UTIL_GETSTRUCTSIZE(pCmdData);
		RPC_SETFIELDCOPYGEN(pMsgBody, offset, pCmdData, structSize);
	}

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_AllocateBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_AllocateBuffer(RPC_OMX_HANDLE hRPCCtx,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr, OMX_IN OMX_U32 nPortIndex,
    OMX_U32 * pBufHeaderRemote, OMX_U32 * pBufferMapped, OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 offset = 0;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BUFFERHEADERTYPE *pBufferHdr = *ppBufferHdr;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_ALLOCATE_BUFFER].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nPortIndex|>pAppPrivate|>nSizeBytes|
	//<pBufHeaderRemote|<offset(BufHeaderRemotecontents)|<offset(platformprivate)|
	//<--BufHeaderRemotecontents--|!<--pPlatformPrivate--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pAppPrivate, OMX_PTR);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nSizeBytes, OMX_U32);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{

		RPC_GETFIELDVALUE(pMsgBody, nPos, *pBufHeaderRemote, OMX_U32);

		RPC_GETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);
		//save platform private before overwriting
		pPlatformPrivate = (*ppBufferHdr)->pPlatformPrivate;
		//RPC_GETFIELDCOPYTYPE(pMsgBody, offset, pBufferHdr, OMX_BUFFERHEADERTYPE);
		/*Copying each field of the header separately due to padding issues in
		   the buffer header structure */
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nSize,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nVersion,
		    OMX_VERSIONTYPE);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pBuffer,
		    OMX_U8 *);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nAllocLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nFilledLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nOffset,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pAppPrivate,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pPlatformPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pInputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pOutputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->hMarkTargetComponent, OMX_HANDLETYPE);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pMarkData,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nTickCount,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nTimeStamp,
		    OMX_TICKS);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nFlags,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->nInputPortIndex, OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->nOutputPortIndex, OMX_U32);

		(*ppBufferHdr)->pPlatformPrivate = pPlatformPrivate;

#ifdef TILER_BUFF
		DOMX_DEBUG(" Copying plat pvt. ");
		RPC_GETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);
		if (offset != 0)
		{
			RPC_GETFIELDCOPYTYPE(pMsgBody, offset,
			    (OMX_TI_PLATFORMPRIVATE
				*) ((*ppBufferHdr)->pPlatformPrivate),
			    OMX_TI_PLATFORMPRIVATE);
			DOMX_DEBUG("Done copying plat pvt., aux buf = 0x%x",
			    ((OMX_TI_PLATFORMPRIVATE
				    *) ((*ppBufferHdr)->
				    pPlatformPrivate))->pAuxBuf1);
		}
#endif

		*pBufferMapped = (OMX_U32) (*ppBufferHdr)->pBuffer;
	} else
	{
		DOMX_DEBUG("OMX Error received: 0x%x",
		    pRPCMsg->msgHeader.nOMXReturn);
	}

      EXIT:
	DOMX_EXIT("");
	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_UseBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param hComp: This is the handle on the Remote core, the proxy will replace
                 it's handle with actual OMX Component handle that recides on the specified core
 * @param ppBufferHdr:
 * @param nPortIndex:
 * @param pAppPrivate:
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
 *                    This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UseBuffer(RPC_OMX_HANDLE hRPCCtx,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr, OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8 * pBuffer,
    OMX_U32 * pBufferMapped, OMX_U32 * pBufHeaderRemote,
    OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 offset = 0;
	OMX_U32 mappedAddress = 0x00;
	OMX_U32 mappedAddress2 = 0x00;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BUFFERHEADERTYPE *pBufferHdr = *ppBufferHdr;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_USE_BUFFER].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	mappedAddress = (OMX_U32) (*ppBufferHdr)->pBuffer;

#ifdef TILER_BUFF
	DOMX_DEBUG(" Getting aux buf");
	mappedAddress2 =
	    (OMX_U32) ((OMX_TI_PLATFORMPRIVATE
		*) ((*ppBufferHdr)->pPlatformPrivate))->pAuxBuf1;
#endif

	DOMX_DEBUG(" DEBUG - MAPPING - pBuffer = %x", pBuffer);
	DOMX_DEBUG(" DEBUG - MAPPING - mappedAddress = %x", mappedAddress);

	//Marshalled:[>hComp|>nPortIndex|>pAppPrivate|>nSizeBytes|
	//>mappedAddress|!>mappedAddress2|<pBufHeaderRemote|
	//<offset(BufHeaderRemotecontents)|<offset(platformprivate)|
	//<--pBufferHdr--|!<--pPlatformPrivate--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pAppPrivate, OMX_PTR);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nSizeBytes, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, mappedAddress, OMX_U32);

#ifdef TILER_BUFF
	RPC_SETFIELDVALUE(pMsgBody, nPos, mappedAddress2, OMX_U32);
#endif

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	//If you are here then Send is successful and you have go the return value
	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{

		RPC_GETFIELDVALUE(pMsgBody, nPos, *pBufHeaderRemote, OMX_U32);

		DOMX_DEBUG(" Getting offset for buffer header:");
		RPC_GETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);
		DOMX_DEBUG(" Copying buffer header at offset:%d", offset);

		//save platform private before overwriting
		pPlatformPrivate = (*ppBufferHdr)->pPlatformPrivate;

		DOMX_DEBUG(" Copying buffer header at offset:%d", offset);
		//RPC_GETFIELDCOPYTYPE(pMsgBody, offset, pBufferHdr, OMX_BUFFERHEADERTYPE);
		/*Copying each field of the header separately due to padding issues in
		   the buffer header structure */
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nSize,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nVersion,
		    OMX_VERSIONTYPE);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pBuffer,
		    OMX_U8 *);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nAllocLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nFilledLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nOffset,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pAppPrivate,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pPlatformPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pInputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->pOutputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->hMarkTargetComponent, OMX_HANDLETYPE);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->pMarkData,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nTickCount,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nTimeStamp,
		    OMX_TICKS);
		RPC_GETFIELDVALUE(pMsgBody, offset, pBufferHdr->nFlags,
		    OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->nInputPortIndex, OMX_U32);
		RPC_GETFIELDVALUE(pMsgBody, offset,
		    pBufferHdr->nOutputPortIndex, OMX_U32);

		(*ppBufferHdr)->pPlatformPrivate = pPlatformPrivate;
		*pBufferMapped = mappedAddress;

#ifdef TILER_BUFF
		DOMX_DEBUG(" Copying plat pvt. ");

		RPC_GETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);
		if (offset != 0)
		{
			RPC_GETFIELDCOPYTYPE(pMsgBody, offset,
			    (OMX_TI_PLATFORMPRIVATE
				*) ((*ppBufferHdr)->pPlatformPrivate),
			    OMX_TI_PLATFORMPRIVATE);
			DOMX_DEBUG("Done copying plat pvt., aux buf = 0x%x",
			    ((OMX_TI_PLATFORMPRIVATE
				    *) ((*ppBufferHdr)->
				    pPlatformPrivate))->pAuxBuf1);
		}
#endif
	} else
	{
		DOMX_DEBUG("OMX Error received: 0x%x",
		    pRPCMsg->msgHeader.nOMXReturn);
		*pBufferMapped = 0;
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_FreeBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FreeBuffer(RPC_OMX_HANDLE hRPCCtx,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_U32 BufHdrRemote, OMX_U32 pBuffer,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_FREE_BUFFER].
	    rpcFxnIdx;
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>nPortIndex|>BufHdrRemote|

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, nPortIndex, OMX_U32);

	RPC_SETFIELDVALUE(pMsgBody, nPos, BufHdrRemote, OMX_U32);
	/* Buffer is being sent separately only to catch NULL buffer errors
	   in PA mode */
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBuffer, OMX_U32);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_EmptyThisBuffer()
 * @brief
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

RPC_OMX_ERRORTYPE RPC_EmptyThisBuffer(RPC_OMX_HANDLE hRPCCtx,
    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;
	OMX_U8 *pAuxBuf1 = NULL;
#ifdef RPC_SYNC_MODE
	RcmClient_Message *pRetPacket = NULL;
#endif

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_EMPTYTHISBUFFER].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>BufHdrRemote|>nFilledLen|>nOffset|>nFlags|>nTimeStamp]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos,
	    (OMX_BUFFERHEADERTYPE *) BufHdrRemote, OMX_BUFFERHEADERTYPE *);

	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->pBuffer, OMX_U8 *);
	/*Check if valid platformPrivate exists, if so set AuxBuf1. It is always marshalled
	   even though it could be NULL */
	if (pBufferHdr->pPlatformPrivate != NULL)
	{
		pAuxBuf1 =
		    ((OMX_TI_PLATFORMPRIVATE
			*) (pBufferHdr->pPlatformPrivate))->pAuxBuf1;
	}
	RPC_SETFIELDVALUE(pMsgBody, nPos, pAuxBuf1, OMX_U8 *);

	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nFilledLen, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nOffset, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nFlags, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nTimeStamp, OMX_TICKS);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->hMarkTargetComponent,
	    OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->pMarkData, OMX_PTR);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nAllocLen, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nOutputPortIndex,
	    OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nInputPortIndex,
	    OMX_U32);

	DOMX_DEBUG(" pBufferHdr = %x BufHdrRemote %x", pBufferHdr,
	    BufHdrRemote);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
#ifdef RPC_SYNC_MODE
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);
#else
	RPC_sendPacket_async(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx);
	RPC_checkAsyncErrors(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket);

	*eCompReturn = OMX_ErrorNone;
#endif

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_FillThisBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FillThisBuffer(RPC_OMX_HANDLE hRPCCtx,
    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;
	OMX_U8 *pAuxBuf1 = NULL;
#ifdef RPC_SYNC_MODE
	RcmClient_Message *pRetPacket = NULL;
#endif

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_FILLTHISBUFFER].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>BufHdrRemote|>nFilledLen|>nOffset|>nFlags]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);
	RPC_SETFIELDVALUE(pMsgBody, nPos,
	    (OMX_BUFFERHEADERTYPE *) BufHdrRemote, OMX_BUFFERHEADERTYPE *);

	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->pBuffer, OMX_U8 *);
	/*Check if valid platformPrivate exists, if so set AuxBuf1. It is always marshalled
	   even though it could be NULL */
	if (pBufferHdr->pPlatformPrivate != NULL)
	{
		pAuxBuf1 =
		    ((OMX_TI_PLATFORMPRIVATE
			*) (pBufferHdr->pPlatformPrivate))->pAuxBuf1;
	}
	RPC_SETFIELDVALUE(pMsgBody, nPos, pAuxBuf1, OMX_U8 *);

	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nFilledLen, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nOffset, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nFlags, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nAllocLen, OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nOutputPortIndex,
	    OMX_U32);
	RPC_SETFIELDVALUE(pMsgBody, nPos, pBufferHdr->nInputPortIndex,
	    OMX_U32);

	DOMX_DEBUG(" pBufferHdr = %x BufHdrRemote %x", pBufferHdr,
	    BufHdrRemote);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
#ifdef RPC_SYNC_MODE
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);
#else
	RPC_sendPacket_async(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx);
	RPC_checkAsyncErrors(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket);

	*eCompReturn = OMX_ErrorNone;
#endif

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_GetState()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetState(RPC_OMX_HANDLE hRPCCtx, OMX_STATETYPE * pState,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	OMX_S16 status;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 offset = 0;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	*pState = OMX_StateInvalid;

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_STATE].
	    rpcFxnIdx;

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>offset(pState)|<--pState--]
	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{
		RPC_GETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);
		RPC_GETFIELDCOPYTYPE(pMsgBody, offset, pState, OMX_STATETYPE);
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetComponentVersion()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetComponentVersion(RPC_OMX_HANDLE hRPCCtx,
    OMX_STRING pComponentName, OMX_VERSIONTYPE * pComponentVersion,
    OMX_VERSIONTYPE * pSpecVersion, OMX_UUIDTYPE * pComponentUUID,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;
	OMX_U32 offset = 0;
	VERSION_INFO *pVer;
	OMX_U32 nPacketSize = PACKET_SIZE;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;
	OMX_S16 status;
	RPC_INDEX fxnIdx;
	OMX_U32 nPos = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	DOMX_ENTER("");

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_VERSION].
	    rpcFxnIdx;

	if ((NULL == pComponentName)
	    || (NULL == pComponentVersion)
	    || (NULL == pSpecVersion) || (NULL == eCompReturn))
	{
		goto EXIT;
	}
	//Allocating remote command message

	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>offset(pComponentName)|>offset(pComponentVersion)|
	//>offset(pSpecVersion)|>offset(pComponentUUID)|
	//<--pComponentName--|<>--pComponentVersion--|
	//<>--pSpecVersion|<--pComponentUUID--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);

	offset = GET_PARAM_DATA_OFFSET;	// strange - why this offset vs just 4 bytes?
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{						 /*** OMX return***/
		pVer =
		    ((VERSION_INFO *) (pMsgBody +
			(OMX_U32) GET_PARAM_DATA_OFFSET));
		strcpy(pComponentName, pVer->cName);
		TIMM_OSAL_Memcpy(pComponentVersion, &(pVer->sVersion.s),
		    sizeof(pVer->sVersion.s));
		TIMM_OSAL_Memcpy(pSpecVersion, &(pVer->sSpecVersion.s),
		    sizeof(pVer->sSpecVersion.s));
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetExtensionIndex()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetExtensionIndex(RPC_OMX_HANDLE hRPCCtx,
    OMX_STRING cParameterName, OMX_INDEXTYPE * pIndexType,
    OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_MESSAGE *pRPCMsg = NULL;
	RPC_OMX_BYTE *pMsgBody = NULL;
	OMX_U32 offset = 0;
	OMX_U32 nPos = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	RPC_OMX_HANDLE hComp = hCtx->remoteHandle;

	OMX_U32 nPacketSize = PACKET_SIZE;
	RcmClient_Message *pPacket = NULL;
	RcmClient_Message *pRetPacket = NULL;

	OMX_S16 status;
	RPC_INDEX fxnIdx;

	RPC_exitOnDucatiFault();

	fxnIdx =
	    rpcHndl[TARGET_CORE_ID].rpcFxns[RPC_OMX_FXN_IDX_GET_EXT_INDEX].
	    rpcFxnIdx;

	//Allocating remote command message
	RPC_getPacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], nPacketSize,
	    pPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	//Marshalled:[>hComp|>offset(cParameterName)|<--pIndexType--|
	//>--cParameterName--]

	RPC_SETFIELDVALUE(pMsgBody, nPos, hComp, RPC_OMX_HANDLE);

	offset =
	    sizeof(RPC_OMX_HANDLE) + sizeof(OMX_U32) + sizeof(OMX_INDEXTYPE);
	RPC_SETFIELDOFFSET(pMsgBody, nPos, offset, OMX_U32);

	//can assert this value at proxy
	if (strlen(cParameterName) <= 128)
	{
		strcpy((OMX_STRING) (pMsgBody + offset), cParameterName);
	}

	pPacket->poolId = hCtx->nPoolId;
	pPacket->jobId = hCtx->nJobId;
	RPC_sendPacket_sync(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    fxnIdx, pRetPacket);
	pRPCMsg = (RPC_OMX_MESSAGE *) (&pRetPacket->data);
	pMsgBody = &pRPCMsg->msgBody[0];

	*eCompReturn = pRPCMsg->msgHeader.nOMXReturn;

	if (pRPCMsg->msgHeader.nOMXReturn == OMX_ErrorNone)
	{
		RPC_GETFIELDCOPYTYPE(pMsgBody, offset, pIndexType,
		    OMX_INDEXTYPE);
	}

	RPC_freePacket(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pRetPacket);

      EXIT:
	return eRPCError;

}

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_EventHandler(RPC_OMX_HANDLE hRPCCtx, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
	return RPC_OMX_ErrorNone;
}

OMX_ERRORTYPE RPC_EmptyBufferDone(RPC_OMX_HANDLE hRPCCtx, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE * pBuffer)
{
	return RPC_OMX_ErrorNone;
}

OMX_ERRORTYPE RPC_FillBufferDone(RPC_OMX_HANDLE hRPCCtx, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE * pBuffer)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_ComponentTunnelRequest(RPC_OMX_HANDLE hRPCCtx,
    OMX_IN OMX_U32 nPort, RPC_OMX_HANDLE hTunneledremoteHandle,
    OMX_U32 nTunneledPort, OMX_INOUT OMX_TUNNELSETUPTYPE * pTunnelSetup,
    OMX_ERRORTYPE * nCmdStatus)
{
	return RPC_OMX_ErrorNone;
}
