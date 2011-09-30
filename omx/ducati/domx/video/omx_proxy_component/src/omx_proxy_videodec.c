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
 *  @file  omx_proxy_videodecoder.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework Tunnel Proxy component.
 *********************************************************************************************
 This is the proxy specific wrapper that passes the component name to the generic proxy init()
 The proxy wrapper also does some runtime/static time onfig on per proxy basis
 This is a thin wrapper that is called when componentiit() of the proxy is called
 static OMX_ERRORTYPE PROXY_Wrapper_init(OMX_HANDLETYPE hComponent, OMX_PTR pAppData);
 this layer gets called first whenever a proxy's get handle is called
 ************************************************************************************************
 *  @path WTSD_DucatiMMSW\omx\omx_il_1_x\omx_proxy_component\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 20-August-2010 Sarthak Aggarwal sarthak@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>

#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER"
/* needs to be specific for every configuration wrapper */

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	DOMX_ENTER("");

	DOMX_DEBUG("Component name provided is %s", COMPONENT_NAME);

	pHandle->pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pHandle->pComponentPrivate != NULL,
	    OMX_ErrorInsufficientResources,
	    "ERROR IN ALLOCATING PROXY COMPONENT PRIVATE STRUCTURE");

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	pComponentPrivate->cCompName =
	    TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pComponentPrivate->cCompName != NULL,
	    OMX_ErrorInsufficientResources,
	    " Error in Allocating space for proxy component table");

	// Copying component Name - this will be picked up in the proxy common
	PROXY_assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH,
	    OMX_ErrorInvalidComponentName,
	    "Length of component name is longer than the max allowed");
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);
	eError = OMX_ProxyCommonInit(hComponent);	// Calling Proxy Common Init()

      EXIT:
	if (eError != OMX_ErrorNone)
	{
		DOMX_DEBUG("Error in Initializing Proxy");
		if (pComponentPrivate->cCompName != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate->cCompName);
			pComponentPrivate->cCompName = NULL;
		}
		if (pComponentPrivate != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate);
			pComponentPrivate = NULL;
		}
	}
	return eError;
}
