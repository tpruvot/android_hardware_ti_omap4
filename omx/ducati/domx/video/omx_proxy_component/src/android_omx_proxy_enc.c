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
 *  @file  omx_proxy_h264dec.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework Tunnel Proxy component.
 ******************************************************************************
 This is the proxy specific wrapper that passes the component name to the
 generic proxy init()
 The proxy wrapper also does some runtime/static time onfig on per proxy basis
 This is a thin wrapper that is called when componentiit() of the proxy is
 called static OMX_ERRORTYPE PROXY_Wrapper_init(OMX_HANDLETYPE hComponent,
 OMX_PTR pAppData); this layer gets called first whenever a proxy s get handle
 is called
 ******************************************************************************
 *  @path WTSD_DucatiMMSW\omx\omx_il_1_x\omx_proxy_component\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 18-june-2010 Dandawate Saket dsaket@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>


#ifdef _Android
#include <utils/Log.h>
#include <PVHeader.h>

#undef LOG_TAG
#define LOG_TAG "OMX_PROXYENC"
#define DOMX_DEBUG LOGE
#endif
#ifdef __H264_ENC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.H264E"	// needs to be specific for every configuration wrapper
#elif __MPEG4_ENC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.MPEG4E"	// needs to be specific for every configuration wrapper
#endif
#ifdef _OPENCORE
static RPC_OMX_ERRORTYPE ComponentPrivateGetParameters(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	if (nParamIndex == PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX)
	{
		DOMX_DEBUG("PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n");
		PV_OMXComponentCapabilityFlagsType *pPVCapaFlags =
		    (PV_OMXComponentCapabilityFlagsType *)
		    pComponentParameterStructure;
		/*Set PV (opencore) capability flags */
		pPVCapaFlags->iIsOMXComponentMultiThreaded = OMX_TRUE;
		pPVCapaFlags->iOMXComponentSupportsExternalOutputBufferAlloc =
		    OMX_TRUE;//OMX_FALSE;
		pPVCapaFlags->iOMXComponentSupportsExternalInputBufferAlloc =
		    OMX_TRUE;
		pPVCapaFlags->iOMXComponentSupportsMovableInputBuffers =
		    OMX_TRUE;
		pPVCapaFlags->iOMXComponentSupportsPartialFrames = OMX_FALSE;
		pPVCapaFlags->iOMXComponentCanHandleIncompleteFrames =
		    OMX_FALSE;
#ifdef __H264_ENC__
		pPVCapaFlags->iOMXComponentUsesNALStartCodes = OMX_TRUE;
#else
		pPVCapaFlags->iOMXComponentUsesNALStartCodes = OMX_FALSE;
#endif
		pPVCapaFlags->iOMXComponentUsesFullAVCFrames = OMX_TRUE;
		return OMX_ErrorNone;
	}
	return PROXY_GetParameter(hComponent, nParamIndex,
	    pComponentParameterStructure);
}
#endif

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	DOMX_DEBUG
	    ("\n_____________________INSISDE VIDEO ENCODER PROXY WRAPPER__________________________\n");

	pHandle->pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
	if (pHandle->pComponentPrivate == NULL)
	{
		DOMX_DEBUG
		    ("\n ERROR IN ALLOCATING PROXY COMPONENT PRIVATE STRUCTURE");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	pComponentPrivate->cCompName =
	    (OMX_U8 *) TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH *
	    sizeof(OMX_U8), TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	if (pComponentPrivate->cCompName == NULL)
	{
		DOMX_DEBUG
		    ("\n ERROR IN ALLOCATING PROXY COMPONENT NAME STRUCTURE");
		TIMM_OSAL_Free(pComponentPrivate);
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	// Copying component Name - this will be picked up in the proxy common
	assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH);
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);
	eError = OMX_ProxyCommonInit(hComponent);	// Calling Proxy Common Init()

	if (eError != OMX_ErrorNone)
	{
		DOMX_DEBUG("\nError in Initializing Proxy");
		TIMM_OSAL_Free(pComponentPrivate->cCompName);
		TIMM_OSAL_Free(pComponentPrivate);
	}
#ifdef _OPENCORE
	// Make sure private function to component is always assigned
	// after component init.
	pHandle->GetParameter = ComponentPrivateGetParameters;
#endif

      EXIT:
	return eError;
}
