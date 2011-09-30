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
 *  @file  omx_rpc_stub.h
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\inc
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

#ifndef OMX_RPC_STUBH
#define OMX_RPC_STUBH

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include "omx_rpc.h"
#include "omx_rpc_internal.h"

	RPC_OMX_ERRORTYPE RPC_GetHandle(RPC_OMX_HANDLE hRPCCtx,
	    OMX_STRING cComponentName, OMX_PTR pAppData,
	    OMX_CALLBACKTYPE * pCallBacks, OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_FreeHandle(RPC_OMX_HANDLE hRPCCtx,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_UseBuffer(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr, OMX_U32 nPortIndex,
	    OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8 * pBuffer,
	    OMX_U32 * pBufferMapped, OMX_U32 * pBufHeaderRemote,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_SetParameter(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_GetParameter(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_FillThisBuffer(RPC_OMX_HANDLE hRPCCtx,
	    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_EmptyThisBuffer(RPC_OMX_HANDLE hRPCCtx,
	    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_FreeBuffer(RPC_OMX_HANDLE hRPCCtx,
	    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_U32 BufHdrRemote,
	    OMX_U32 pBuffer, OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_SetConfig(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_GetConfig(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_GetComponentVersion(RPC_OMX_HANDLE hRPCCtx,
	    OMX_STRING pComponentName, OMX_VERSIONTYPE * pComponentVersion,
	    OMX_VERSIONTYPE * pSpecVersion, OMX_UUIDTYPE * pComponentUUID,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_SendCommand(RPC_OMX_HANDLE hRPCCtx,
	    OMX_COMMANDTYPE eCmd, OMX_U32 nParam, OMX_PTR pCmdData,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_GetState(RPC_OMX_HANDLE hRPCCtx,
	    OMX_STATETYPE * pState, OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_GetExtensionIndex(RPC_OMX_HANDLE hComponent,
	    OMX_STRING cParameterName, OMX_INDEXTYPE * pIndexType,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_AllocateBuffer(RPC_OMX_HANDLE hRPCCtx,
	    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
	    OMX_IN OMX_U32 nPortIndex, OMX_U32 * pBufHeaderRemote,
	    OMX_U32 * pBufferMapped, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes,
	    OMX_ERRORTYPE * nCmdStatus);

	RPC_OMX_ERRORTYPE RPC_ComponentTunnelRequest(RPC_OMX_HANDLE hRPCCtx,
	    OMX_IN OMX_U32 nPort, RPC_OMX_HANDLE hTunneledremoteHandle,
	    OMX_U32 nTunneledPort,
	    OMX_INOUT OMX_TUNNELSETUPTYPE * pTunnelSetup,
	    OMX_ERRORTYPE * nCmdStatus);

/*Empty Stubs*/
	OMX_ERRORTYPE RPC_EventHandler(RPC_OMX_HANDLE hRPCCtx,
	    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
	    OMX_U32 nData2, OMX_PTR pEventData);
	OMX_ERRORTYPE RPC_EmptyBufferDone(RPC_OMX_HANDLE hRPCCtx,
	    OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBuffer);
	OMX_ERRORTYPE RPC_FillBufferDone(RPC_OMX_HANDLE hRPCCtx,
	    OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBuffer);

#endif
