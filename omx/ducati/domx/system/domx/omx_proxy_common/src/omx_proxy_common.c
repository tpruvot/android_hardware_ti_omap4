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
 *  @file  omx_proxy_common.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework OMX Common Proxy .
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_proxy_common\src
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

/* ------compilation control switches ----------------------------------------*/
#define TILER_BUFF

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <string.h>

#include "timm_osal_memory.h"
#include "timm_osal_mutex.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Index.h"
#include "OMX_TI_Core.h"
/*-------program files ----------------------------------------*/
#include "omx_proxy_common.h"
#include "omx_rpc.h"
#include "omx_rpc_stub.h"
#include "omx_rpc_utils.h"

#ifdef TILER_BUFF
#include <ProcMgr.h>
#include <SysLinkMemUtils.h>
#include <mem_types.h>
#include <phase1_d2c_remap.h>
#include <memmgr.h>
#endif

#include <utils/Log.h>
#undef LOG_TAG
#define LOG_TAG "OMX_PROXYCOMMON"
//#define DOMX_DEBUG LOGE
//#define DOMX_ENTER LOGE
//#define DOMX_LEAVE LOGE
//#define DOMX_ERROR LOGE

#ifdef TILER_BUFF
#define PortFormatIsNotYUV 0
static OMX_ERRORTYPE RPC_PrepareBuffer_Remote(PROXY_COMPONENT_PRIVATE *
    pCompPrv, OMX_COMPONENTTYPE * hRemoteComp, OMX_U32 nPortIndex,
    OMX_U32 nSizeBytes, OMX_BUFFERHEADERTYPE * pChironBuf,
    OMX_BUFFERHEADERTYPE * pDucBuf, OMX_PTR pBufToBeMapped);
static OMX_ERRORTYPE RPC_PrepareBuffer_Chiron(PROXY_COMPONENT_PRIVATE *
    pCompPrv, OMX_COMPONENTTYPE * hRemoteComp, OMX_U32 nPortIndex,
    OMX_U32 nSizeBytes, OMX_BUFFERHEADERTYPE * pDucBuf,
    OMX_BUFFERHEADERTYPE * pChironBuf);
static OMX_ERRORTYPE RPC_UTIL_GetNumLines(OMX_COMPONENTTYPE * hComp,
    OMX_U32 nPortIndex, OMX_U32 * nNumOfLines);
static OMX_ERRORTYPE RPC_UnMapBuffer_Ducati(OMX_PTR pBuffer);
static OMX_ERRORTYPE RPC_MapBuffer_Ducati(OMX_U8 * pBuf, OMX_U32 nBufLineSize,
    OMX_U32 nBufLines, OMX_U8 ** pMappedBuf, OMX_PTR pBufToBeMapped);

static OMX_ERRORTYPE RPC_MapMetaData_Host(OMX_BUFFERHEADERTYPE * pBufHdr);
static OMX_ERRORTYPE RPC_UnMapMetaData_Host(OMX_BUFFERHEADERTYPE * pBufHdr);

static OMX_ERRORTYPE _RPC_IsProxyComponent(OMX_HANDLETYPE hComponent,
    OMX_BOOL * bIsProxy);

#endif

#define LINUX_PAGE_SIZE (4 * 1024)
#define MAXNAMESIZE 128

extern COREID TARGET_CORE_ID;
extern char Core_Array[MAX_PROC][MAX_CORENAME_LENGTH];


extern OMX_S32 currentNumOfComps;
extern OMX_HANDLETYPE componentTable[];
extern OMX_PTR pFaultMutex;

extern OMX_BOOL ducatiFault;

/******************************************************************
 *   MACROS - LOCAL
 ******************************************************************/

#define PROXY_checkRpcError() do { \
    if (eRPCError == RPC_OMX_ErrorNone) \
    { \
        DOMX_DEBUG("Corresponding RPC function executed successfully"); \
        eError = eCompReturn; \
    } else \
    { \
        DOMX_ERROR("RPC function returned error 0x%x", eRPCError); \
        switch (eRPCError) \
        { \
            case RPC_OMX_ErrorHardware: \
                eError = OMX_ErrorHardware; \
            break; \
            case RPC_OMX_ErrorInsufficientResources: \
                eError = OMX_ErrorInsufficientResources; \
            break; \
            case RPC_OMX_ErrorBadParameter: \
                eError = OMX_ErrorBadParameter; \
            break; \
            case RPC_OMX_ErrorUnsupportedIndex: \
                eError = OMX_ErrorUnsupportedIndex; \
            break; \
            case RPC_OMX_ErrorTimeout: \
                eError = OMX_ErrorTimeout; \
            break; \
            default: \
                eError = OMX_ErrorUndefined; \
        } \
    } \
    } while(0)


/* ===========================================================================*/
/**
 * @name PROXY_EventHandler()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	OMX_U16 count;
	OMX_BUFFERHEADERTYPE *pLocalBufHdr = NULL;
	OMX_PTR pTmpData = NULL;

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, eEvent=%p, nData1=%p, nData2=%p, pEventData=%p",
	    hComponent, pCompPrv, eEvent, nData1, nData2, pEventData);

	switch (eEvent)
	{

	case OMX_TI_EventBufferRefCount:
		DOMX_DEBUG("Received Ref Count Event");
		/*nData1 will be pBufferHeader, nData2 will be present count. Need to find local
		   buffer header for nData1 which is remote buffer header */

		PROXY_assert((nData1 != 0), OMX_ErrorBadParameter,
		    "Received NULL buffer header from OMX component");

		/*find local buffer header equivalent */
		for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
		{
			if (pCompPrv->tBufList[count].pBufHeaderRemote ==
			    nData1)
			{
				pLocalBufHdr =
				    pCompPrv->tBufList[count].pBufHeader;
				pLocalBufHdr->pBuffer =
				    (OMX_U8 *) pCompPrv->
				    tBufList[count].pBufferActual;
				break;
			}
		}
		PROXY_assert((count != pCompPrv->nTotalBuffers),
		    OMX_ErrorBadParameter,
		    "Received invalid-buffer header from OMX component");

		/*update local buffer header */
		nData1 = (OMX_U32) pLocalBufHdr;
		break;
	case OMX_EventMark:
		DOMX_DEBUG("Received Mark Event");
		PROXY_assert((pEventData != NULL), OMX_ErrorUndefined,
		    "MarkData corrupted");
		pTmpData = pEventData;
		pEventData =
		    ((PROXY_MARK_DATA *) pEventData)->pMarkDataActual;
		TIMM_OSAL_Free(pTmpData);
		break;

	default:
		break;
	}

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, eEvent, nData1, nData2, pEventData);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_EmptyBufferDone()
 * @brief
 * @param
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	OMX_U16 count;
	OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, remoteBufHdr=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, remoteBufHdr, nfilledLen, nOffset, nFlags);

	for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
	{
		if (pCompPrv->tBufList[count].pBufHeaderRemote ==
		    remoteBufHdr)
		{
			pBufHdr = pCompPrv->tBufList[count].pBufHeader;
			pBufHdr->nFilledLen = nfilledLen;
			pBufHdr->nOffset = nOffset;
			pBufHdr->nFlags = nFlags;
			pBufHdr->pBuffer =
			    (OMX_U8 *) pCompPrv->
			    tBufList[count].pBufferActual;
			/* Setting mark info to NULL. This would always be
			   NULL in EBD, whether component has propagated the
			   mark or has generated mark event */
			pBufHdr->hMarkTargetComponent = NULL;
			pBufHdr->pMarkData = NULL;
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Received invalid-buffer header from OMX component");

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.EmptyBufferDone(hComponent,
		    pCompPrv->pILAppData, pBufHdr);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_FillBufferDone()
 * @brief
 * @param
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags,
    OMX_TICKS nTimeStamp, OMX_HANDLETYPE hMarkTargetComponent,
    OMX_PTR pMarkData)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
        RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	OMX_U16 count;
	OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, remoteBufHdr=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, remoteBufHdr, nfilledLen, nOffset, nFlags);

	for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
	{
		if (pCompPrv->tBufList[count].pBufHeaderRemote ==
		    remoteBufHdr)
		{
			pBufHdr = pCompPrv->tBufList[count].pBufHeader;
			pBufHdr->nFilledLen = nfilledLen;
			//LOGE ("FillbufferDone length (%d) ", pBufHdr->nFilledLen);
			pBufHdr->nOffset = nOffset;
			pBufHdr->nFlags = nFlags;
			pBufHdr->pBuffer =
			    (OMX_U8 *) pCompPrv->
			    tBufList[count].pBufferActual;
			pBufHdr->nTimeStamp = nTimeStamp;
			if (pMarkData != NULL)
			{
				/*Update mark info in the buffer header */
				pBufHdr->pMarkData =
				    ((PROXY_MARK_DATA *)
				    pMarkData)->pMarkDataActual;
				pBufHdr->hMarkTargetComponent =
				    ((PROXY_MARK_DATA *)
				    pMarkData)->hComponentActual;
				TIMM_OSAL_Free(pMarkData);
			}

                        // Perform Cache Invalidate for video encoders only
                        if(!strcmp(pCompPrv->cCompName,"OMX.TI.DUCATI1.VIDEO.MPEG4E") || 
                           !strcmp(pCompPrv->cCompName,"OMX.TI.DUCATI1.VIDEO.H264E")) 
                        {
                           // Cache Invalidate in case of non tiler buffers only
                           if ((pCompPrv->tBufList[count].pBufferActual !=
			        pCompPrv->tBufList[count].pBufferToBeMapped) &&
                                (pBufHdr->nFilledLen > 0))
			   {
                                eRPCError =
				    RPC_InvalidateBuffer(pBufHdr->pBuffer,
				    pBufHdr->nFilledLen, TARGET_CORE_ID);
				if (eRPCError != RPC_OMX_ErrorNone)
				{
					TIMM_OSAL_Error
					    ("Invalidate Buffer failed");
					/*Cache operation failed - indicate a hardware error to
					   client via event handler */
					eError =
					    PROXY_EventHandler(hComponent,
					    pCompPrv->pILAppData,
					    OMX_EventError, OMX_ErrorHardware,
					    0, NULL);
					/*EventHandler is not supposed to return any error so not
					   handling it */
					goto EXIT;
				}
			    }
                        }
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Received invalid-buffer header from OMX component");

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.FillBufferDone(hComponent,
		    pCompPrv->pILAppData, pBufHdr);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_EmptyThisBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_EmptyThisBuffer(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	OMX_U32 count = 0;
	OMX_U8 *pBuffer = NULL;
	OMX_U32 pBufToBeMapped = 0;
	OMX_COMPONENTTYPE *pMarkComp = NULL;
	PROXY_COMPONENT_PRIVATE *pMarkCompPrv = NULL;
	OMX_PTR pMarkData = NULL;
	OMX_BOOL bFreeMarkIfError = OMX_FALSE;
	OMX_BOOL bIsProxy = OMX_FALSE;

	PROXY_assert(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	pBuffer = pBufferHdr->pBuffer;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, pBuffer=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, pBuffer, pBufferHdr->nFilledLen,
	    pBufferHdr->nOffset, pBufferHdr->nFlags);

	/*First find the index of this buffer header to retrieve remote buffer header */
	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d ", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the remote header in buffer list");

	/*[NPA] If the buffer is modified in buffer header, force remap.
	   TBD: Even if MODIFIED is set, the pBuffer can be a pre-mapped buffer
	 */
	if ((pBufferHdr->nFlags) & (OMX_BUFFERHEADERFLAG_MODIFIED))
	{
		/*Unmap previously mapped buffer if applicable */
		if (pCompPrv->tBufList[count].pBufferActual !=
		    pCompPrv->tBufList[count].pBufferToBeMapped)
		{
			/*This is a non tiler buffer - needs to be unmapped from tiler space */
			eError =
			    RPC_UnMapBuffer_Ducati((OMX_PTR)
			    (pCompPrv->tBufList[count].pBufferToBeMapped));
			PROXY_assert(eError == OMX_ErrorNone, eError,
			    "UnMap Ducati Buffer returned an error");
		}
		/* Same pBufferHdr will get updated with remote pBuffer and pAuxBuf1 if a 2D buffer */
		eError =
		    RPC_PrepareBuffer_Remote(pCompPrv, pCompPrv->hRemoteComp,
		    pBufferHdr->nInputPortIndex, pBufferHdr->nAllocLen,
		    pBufferHdr, NULL, &pBufToBeMapped);
		PROXY_assert(eError == OMX_ErrorNone, eError,
		    "Unable to map buffer");

		/*Now update the buffer list with new details */
		pCompPrv->tBufList[count].pBufferMapped =
		    (OMX_U32) (pBufferHdr->pBuffer);
		pCompPrv->tBufList[count].pBufferActual = (OMX_U32) pBuffer;
		pCompPrv->tBufList[count].pBufferToBeMapped = pBufToBeMapped;
	} else
	{
		/*Update pBuffer with pBufferMapped stored in input port private
		   AuxBuf1 remains untouched as stored during use or allocate calls */
		pBufferHdr->pBuffer =
		    (OMX_U8 *) pBufferHdr->pInputPortPrivate;
	}

	/*Flushing non tiler buffers only for now */
	if (pCompPrv->tBufList[count].pBufferActual !=
	    pCompPrv->tBufList[count].pBufferToBeMapped)
	{
		if(pBufferHdr->nAllocLen > 0 && (pBufferHdr->nFlags & OMX_BUFFERFLAG_EXTRADATA))
		{
			eRPCError =
				RPC_FlushBuffer(pBuffer, pBufferHdr->nAllocLen,
				TARGET_CORE_ID);
		}
		else if(pBufferHdr->nFilledLen > 0)
		{
			eRPCError =
				RPC_FlushBuffer(pBuffer, pBufferHdr->nFilledLen,
				TARGET_CORE_ID);
		}

		PROXY_assert(eRPCError == RPC_OMX_ErrorNone,
				OMX_ErrorHardware, "Flush Buffer failed");
	}

	if (pBufferHdr->hMarkTargetComponent != NULL)
	{
		pMarkComp =
		    (OMX_COMPONENTTYPE *) (pBufferHdr->hMarkTargetComponent);
		/* Check is mark comp is proxy */
		eError = _RPC_IsProxyComponent(pMarkComp, &bIsProxy);
		PROXY_assert(eError == OMX_ErrorNone, eError, "");

		/*Replacing original mark data with proxy specific structure */
		pMarkData = pBufferHdr->pMarkData;
		pBufferHdr->pMarkData =
		    TIMM_OSAL_Malloc(sizeof(PROXY_MARK_DATA), TIMM_OSAL_TRUE,
		    0, TIMMOSAL_MEM_SEGMENT_INT);
		PROXY_assert(pBufferHdr->pMarkData != NULL,
		    OMX_ErrorInsufficientResources, "Malloc failed");
		bFreeMarkIfError = OMX_TRUE;
		((PROXY_MARK_DATA *) (pBufferHdr->pMarkData))->
		    hComponentActual = pMarkComp;
		((PROXY_MARK_DATA *) (pBufferHdr->pMarkData))->
		    pMarkDataActual = pMarkData;

		/* If proxy comp then replace hMarkTargetComponent with remote
		   handle */
		if (bIsProxy)
		{
			pMarkCompPrv = pMarkComp->pComponentPrivate;
			PROXY_assert(pMarkCompPrv != NULL,
			    OMX_ErrorBadParameter, NULL);

			/* Replacing with remote component handle */
			pBufferHdr->hMarkTargetComponent =
			    ((RPC_OMX_CONTEXT *) pMarkCompPrv->
			    hRemoteComp)->hActualRemoteCompHandle;
		}
	}

	eRPCError =
	    RPC_EmptyThisBuffer(pCompPrv->hRemoteComp, pBufferHdr,
	    pCompPrv->tBufList[count].pBufHeaderRemote, &eCompReturn);

	//changing back the local buffer address
	pBufferHdr->pBuffer =
	    (OMX_U8 *) pCompPrv->tBufList[count].pBufferActual;

	PROXY_checkRpcError();

      EXIT:
	/*If ETB is about to return an error then this means that buffer has not
	   been accepted by the component. Thus the allocated mark data will be
	   lost so free it here. Also replace original mark data in the header */
	if ((eError != OMX_ErrorNone) && bFreeMarkIfError)
	{
		pBufferHdr->hMarkTargetComponent =
		    ((PROXY_MARK_DATA *) (pBufferHdr->pMarkData))->
		    hComponentActual;
		pMarkData =
		    ((PROXY_MARK_DATA *) (pBufferHdr->pMarkData))->
		    pMarkDataActual;
		TIMM_OSAL_Free(pBufferHdr->pMarkData);
		pBufferHdr->pMarkData = pMarkData;
	}

	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_FillThisBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_FillThisBuffer(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	OMX_U32 count = 0;
	OMX_U8 *pBuffer = NULL;
	OMX_U32 pBufToBeMapped = 0;

	PROXY_assert(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	pBuffer = pBufferHdr->pBuffer;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, pBuffer=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, pBuffer, pBufferHdr->nFilledLen,
	    pBufferHdr->nOffset, pBufferHdr->nFlags);

	/*First find the index of this buffer header to retrieve remote buffer header */
	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d ", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the remote header in buffer list");

	/*[NPA] If the buffer is modified in buffer header, force remap.
	   TBD: Even if MODIFIED is set, the pBuffer can be a pre-mapped buffer
	 */
	if ((pBufferHdr->nFlags) & (OMX_BUFFERHEADERFLAG_MODIFIED))
	{

		/*Unmap previously mapped buffer if applicable */
		if (pCompPrv->tBufList[count].pBufferActual !=
		    pCompPrv->tBufList[count].pBufferToBeMapped)
		{
			/*This is a non tiler buffer - needs to be unmapped from tiler space */
			eError =
			    RPC_UnMapBuffer_Ducati((OMX_PTR)
			    (pCompPrv->tBufList[count].pBufferToBeMapped));
			PROXY_assert(eError == OMX_ErrorNone, eError,
			    "UnMap Ducati Buffer returned an error");
		}

		/* Same pBufferHdr will get updated with remote pBuffer and pAuxBuf1 if a 2D buffer */
		eError =
		    RPC_PrepareBuffer_Remote(pCompPrv, pCompPrv->hRemoteComp,
		    pBufferHdr->nOutputPortIndex, pBufferHdr->nAllocLen,
		    pBufferHdr, NULL, &pBufToBeMapped);
		PROXY_assert(eError == OMX_ErrorNone, eError,
		    "Unable to map buffer");

		/*Now update the buffer list with new details */
		pCompPrv->tBufList[count].pBufferMapped =
		    (OMX_U32) (pBufferHdr->pBuffer);
		pCompPrv->tBufList[count].pBufferActual = (OMX_U32) pBuffer;
		pCompPrv->tBufList[count].pBufferToBeMapped = pBufToBeMapped;
	} else
	{
		/*Update pBuffer with pBufferMapped stored in input port private
		   AuxBuf1 remains untouched as stored during use or allocate calls */
		pBufferHdr->pBuffer =
		    (OMX_U8 *) pBufferHdr->pInputPortPrivate;
	}

	/*Flushing non tiler buffers only for now */
	if ((pCompPrv->tBufList[count].pBufferActual !=
	    pCompPrv->tBufList[count].pBufferToBeMapped) &&
            (pBufferHdr->nAllocLen > 0))
	{
		eRPCError =
		    RPC_InvalidateBuffer(pBuffer, pBufferHdr->nAllocLen,
		    TARGET_CORE_ID);
		PROXY_assert(eRPCError == RPC_OMX_ErrorNone,
		    OMX_ErrorHardware, "Flush Buffer failed");
	}

	eRPCError = RPC_FillThisBuffer(pCompPrv->hRemoteComp, pBufferHdr,
	    pCompPrv->tBufList[count].pBufHeaderRemote, &eCompReturn);

	//changing back the local buffer address
	pBufferHdr->pBuffer =
	    (OMX_U8 *) pCompPrv->tBufList[count].pBufferActual;

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_AllocateBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
	OMX_U32 pBufferMapped;
	OMX_U32 pBufHeaderRemote;
	OMX_U32 currentBuffer = 0, i = 0;
	OMX_U8 *pBuffer;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BOOL bSlotFound = OMX_FALSE;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_assert(ppBufferHdr != NULL, OMX_ErrorBadParameter,
	    "Pointer to buffer header is NULL");
	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nPortIndex=%p, pAppPrivate=%p, nSizeBytes=%d",
	    hComponent, pCompPrv, nPortIndex, pAppPrivate, nSizeBytes);

	/*Pick up 1st empty slot */
	for (i = 0; i < pCompPrv->nTotalBuffers; i++)
	{
		if (pCompPrv->tBufList[i].pBufHeader == 0)
		{
			currentBuffer = i;
			bSlotFound = OMX_TRUE;
			break;
		}
	}
	if (!bSlotFound)
	{
		currentBuffer = pCompPrv->nTotalBuffers;
	}

	DOMX_DEBUG("In AB, no. of buffers = %d", pCompPrv->nTotalBuffers);
	PROXY_assert((pCompPrv->nTotalBuffers < MAX_NUM_PROXY_BUFFERS),
	    OMX_ErrorInsufficientResources,
	    "Proxy cannot handle more than MAX buffers");

	//Allocating Local bufferheader to be maintained locally within proxy
	pBufferHeader =
	    (OMX_BUFFERHEADERTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE), TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert((pBufferHeader != NULL), OMX_ErrorInsufficientResources,
	    "Allocation of Buffer Header structure failed");

	pPlatformPrivate =
	    (OMX_TI_PLATFORMPRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_TI_PLATFORMPRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);
	if (pPlatformPrivate == NULL)
	{
		TIMM_OSAL_Free(pBufferHeader);
		PROXY_assert(0, OMX_ErrorInsufficientResources, NULL);
	}
	pBufferHeader->pPlatformPrivate = pPlatformPrivate;

	DOMX_DEBUG(" Calling RPC ");

	eRPCError =
	    RPC_AllocateBuffer(pCompPrv->hRemoteComp, &pBufferHeader,
	    nPortIndex, &pBufHeaderRemote, &pBufferMapped, pAppPrivate,
	    nSizeBytes, &eCompReturn);

	PROXY_checkRpcError();

	if (eError == OMX_ErrorNone)
	{
		DOMX_DEBUG("Allocate Buffer Successful");
		DOMX_DEBUG
		    ("Value of pBufHeaderRemote: %p   LocalBufferHdr :%p",
		    pBufHeaderRemote, pBufferHeader);

		eError =
		    RPC_PrepareBuffer_Chiron(pCompPrv,
		    pCompPrv->hRemoteComp, nPortIndex, nSizeBytes,
		    pBufferHeader, NULL);

		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Free(pBufferHeader);
			TIMM_OSAL_Free(pPlatformPrivate);

			PROXY_assert(0, OMX_ErrorUndefined,
			    "Error while mapping buffer to chiron");
		}
		pBuffer = pBufferHeader->pBuffer;

		/*Map Ducati metadata buffer if present to Host and update in the same field */
		eError = RPC_MapMetaData_Host(pBufferHeader);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Free(pBufferHeader);
			TIMM_OSAL_Free(pPlatformPrivate);

			PROXY_assert(0, OMX_ErrorUndefined,
			    "Error while mapping metadata buffer to chiron");
		}


		/*pBufferMapped here will contain the Y pointer (basically the unity mapped pBuffer)
		   pBufferHeaderRemote is the header that contains both Y, UV pointers */

		pCompPrv->tBufList[currentBuffer].pBufHeader = pBufferHeader;
		pCompPrv->tBufList[currentBuffer].pBufHeaderRemote =
		    pBufHeaderRemote;
		pCompPrv->tBufList[currentBuffer].pBufferMapped =
		    pBufferMapped;
		pCompPrv->tBufList[currentBuffer].pBufferActual =
		    (OMX_U32) pBuffer;
		pCompPrv->tBufList[currentBuffer].pBufferToBeMapped =
		    (OMX_U32) pBuffer;
		pCompPrv->tBufList[currentBuffer].bRemoteAllocatedBuffer =
		    OMX_TRUE;

		//caching actual content of pInportPrivate
		pCompPrv->tBufList[currentBuffer].actualContent =
		    (OMX_U32) pBufferHeader->pInputPortPrivate;
		//filling pInportPrivate with the mapped address to be later used during ETB and FTB calls
		//Need to think on if we need a global actual buffer to mapped buffer data.
		pBufferHeader->pInputPortPrivate = (OMX_PTR) pBufferMapped;

		//keeping track of number of Buffers
		pCompPrv->nAllocatedBuffers++;
		if (pCompPrv->nTotalBuffers < pCompPrv->nAllocatedBuffers)
			pCompPrv->nTotalBuffers = pCompPrv->nAllocatedBuffers;

		*ppBufferHdr = pBufferHeader;
	} else
	{
		TIMM_OSAL_Free(pPlatformPrivate);
		TIMM_OSAL_Free((void *)pBufferHeader);
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_UseBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8 * pBuffer)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
	OMX_ERRORTYPE eCompReturn;
	OMX_U32 pBufferMapped;
	OMX_U32 pBufHeaderRemote;
	OMX_U32 pBufToBeMapped = 0;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_U32 currentBuffer = 0, i = 0;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_BOOL bSlotFound = OMX_FALSE;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_assert(ppBufferHdr != NULL, OMX_ErrorBadParameter,
	    "Pointer to buffer header is NULL");
	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nPortIndex=%p, pAppPrivate=%p, nSizeBytes=%d, pBuffer=%p",
	    hComponent, pCompPrv, nPortIndex, pAppPrivate, nSizeBytes,
	    pBuffer);

	/*Pick up 1st empty slot */
	for (i = 0; i < pCompPrv->nTotalBuffers; i++)
	{
		if (pCompPrv->tBufList[i].pBufHeader == 0)
		{
			currentBuffer = i;
			bSlotFound = OMX_TRUE;
			break;
		}
	}
	if (!bSlotFound)
	{
		currentBuffer = pCompPrv->nTotalBuffers;
	}
	DOMX_DEBUG("In UB, no. of buffers = %d", pCompPrv->nTotalBuffers);

	PROXY_assert((pCompPrv->nTotalBuffers < MAX_NUM_PROXY_BUFFERS),
	    OMX_ErrorInsufficientResources,
	    "Proxy cannot handle more than MAX buffers");

	//Allocating Local bufferheader to be maintained locally within proxy
	pBufferHeader =
	    (OMX_BUFFERHEADERTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE), TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert((pBufferHeader != NULL), OMX_ErrorInsufficientResources,
	    "Allocation of Buffer Header structure failed");

	pPlatformPrivate =
	    (OMX_TI_PLATFORMPRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_TI_PLATFORMPRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);
	if (pPlatformPrivate == NULL)
	{
		TIMM_OSAL_Free(pBufferHeader);
		PROXY_assert(0, OMX_ErrorInsufficientResources, NULL);
	}
	pBufferHeader->pPlatformPrivate = pPlatformPrivate;
        pBufferHeader->nAllocLen = nSizeBytes;
	DOMX_DEBUG("Preparing buffer to Remote Core...");

	pBufferHeader->pBuffer = pBuffer;

	DOMX_DEBUG("Prepared buffer header for preparebuffer function...");

	/*[NPA] With NPA, pBuffer can be NULL */
	if (pBuffer != NULL)
	{
		eError =
		    RPC_PrepareBuffer_Remote(pCompPrv, pCompPrv->hRemoteComp,
		    nPortIndex, nSizeBytes, pBufferHeader, NULL,
		    &pBufToBeMapped);

		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Free(pPlatformPrivate);
			TIMM_OSAL_Free(pBufferHeader);
			PROXY_assert(0, OMX_ErrorUndefined,
			    "ERROR WHILE GETTING FRAME HEIGHT");
		}
	}

	DOMX_DEBUG("Making Remote call...");
	eRPCError =
	    RPC_UseBuffer(pCompPrv->hRemoteComp, &pBufferHeader, nPortIndex,
	    pAppPrivate, nSizeBytes, pBuffer, &pBufferMapped,
	    &pBufHeaderRemote, &eCompReturn);

	PROXY_checkRpcError();

	if (eError == OMX_ErrorNone)
	{
		DOMX_DEBUG("Use Buffer Successful");
		DOMX_DEBUG
		    ("Value of pBufHeaderRemote: %p   LocalBufferHdr :%p",
		    pBufHeaderRemote, pBufferHeader);

		/*Map Ducati metadata buffer if present to Host and update in the same field */
		eError = RPC_MapMetaData_Host(pBufferHeader);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Free(pBufferHeader);
			TIMM_OSAL_Free(pPlatformPrivate);

			PROXY_assert(0, OMX_ErrorUndefined,
			    "Error while mapping metadata buffer to chiron");
		}
		//Storing details of pBufferHeader/Mapped/Actual buffer address locally.
		pCompPrv->tBufList[currentBuffer].pBufHeader = pBufferHeader;
		pCompPrv->tBufList[currentBuffer].pBufHeaderRemote =
		    pBufHeaderRemote;
		pCompPrv->tBufList[currentBuffer].pBufferMapped =
		    pBufferMapped;
		pCompPrv->tBufList[currentBuffer].pBufferActual =
		    (OMX_U32) pBuffer;
		//caching actual content of pInportPrivate
		pCompPrv->tBufList[currentBuffer].actualContent =
		    (OMX_U32) pBufferHeader->pInputPortPrivate;
		pCompPrv->tBufList[currentBuffer].pBufferToBeMapped =
		    pBufToBeMapped;
		pCompPrv->tBufList[currentBuffer].bRemoteAllocatedBuffer =
		    OMX_FALSE;

		//keeping track of number of Buffers
		pCompPrv->nAllocatedBuffers++;
		if (pCompPrv->nTotalBuffers < pCompPrv->nAllocatedBuffers)
			pCompPrv->nTotalBuffers = pCompPrv->nAllocatedBuffers;

		DOMX_DEBUG("Updating no. of buffer to %d",
		    pCompPrv->nTotalBuffers);

		//Restore back original pBuffer
		pBufferHeader->pBuffer = pBuffer;
		//pBufferMapped in pInputPortPrivate acts as key during ETB/FTB calls
		pBufferHeader->pInputPortPrivate = (OMX_PTR) pBufferMapped;

		*ppBufferHdr = pBufferHeader;
	} else
	{
		DOMX_ERROR
		    ("Error in UseBuffer return value, freeing buffer header");
		TIMM_OSAL_Free(pPlatformPrivate);
		TIMM_OSAL_Free((void *)pBufferHeader);
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_FreeBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_FreeBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eCompReturn;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	OMX_U32 count = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone, eTmpError = OMX_ErrorNone;
	OMX_S32 nReturn = 0;
	OMX_U32 pBuffer = 0;
	OMX_BOOL bResetErrVal = OMX_FALSE;

	PROXY_assert(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent=%p, pCompPrv=%p, nPortIndex=%p, pBufferHdr=%p",
	    hComponent, pCompPrv, nPortIndex, pBufferHdr);

	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the mapped address in component private buffer list");

	/*Not having asserts from this point since even if error occurs during
	   unmapping/freeing, still trying to clean up as much as possible */

	/* Will be sending the buffer pointer to Ducati as well. This is to
	   catch errors in case NULL buffer is sent in PA mode */
	if (pBufferHdr->pBuffer == NULL)
	{
		pBuffer = 0;
	} else
	{
		pBuffer = pCompPrv->tBufList[count].pBufferMapped;
	}
	/*Unmap metadata buffer on Chiron if was mapped earlier */
	eTmpError = RPC_UnMapMetaData_Host(pBufferHdr);
	if (eTmpError != OMX_ErrorNone)
	{
		eError = eTmpError;
		DOMX_ERROR("UnMap MetaData returned error 0x%x", eError);
	}

	eRPCError =
	    RPC_FreeBuffer(pCompPrv->hRemoteComp, nPortIndex,
	    pCompPrv->tBufList[count].pBufHeaderRemote, pBuffer,
	    &eCompReturn);

	if ((eError != OMX_ErrorNone) && (eCompReturn == OMX_ErrorNone))
	{
		eTmpError = eError;
		bResetErrVal = OMX_TRUE;
	}
	PROXY_checkRpcError();
	/*If eError has already been set to some error value then it should not
	   be overwritten by an error none value */
	if (bResetErrVal)
	{
		eError = eTmpError;
		bResetErrVal = OMX_FALSE;
	}

	if (pCompPrv->tBufList[count].pBufferActual !=
	    pCompPrv->tBufList[count].pBufferToBeMapped)
	{
		/*This is a non tiler buffer - needs to be unmapped from tiler space */
		eTmpError =
		    RPC_UnMapBuffer_Ducati((OMX_PTR) (pCompPrv->tBufList
			[count].pBufferToBeMapped));
		if (eTmpError != OMX_ErrorNone)
		{
			eError = eTmpError;
			DOMX_ERROR("UnMap Ducati Buffer returned an error");
		}
	}

	if (pCompPrv->tBufList[count].bRemoteAllocatedBuffer == OMX_TRUE)
	{
		/*TODO: Move this and D2C map to separate function along with buffer plugin
		   implementation */
		nReturn =
		    tiler_assisted_phase1_DeMap((OMX_PTR) (pCompPrv->tBufList
			[count].pBufferActual));
		if (nReturn != 0)
		{
			eError = OMX_ErrorUndefined;
			DOMX_ERROR("MemMgr DeMap failed");
		}
	}

	DOMX_DEBUG("Cleaning up Buffer");
	if (pCompPrv->tBufList[count].pBufferMapped)
		RPC_UnMapBuffer(pCompPrv->tBufList[count].pBufferMapped);

	if (pCompPrv->tBufList[count].pBufHeader)
	{
		if (pCompPrv->tBufList[count].pBufHeader->pPlatformPrivate)
			TIMM_OSAL_Free(pCompPrv->tBufList[count].
			    pBufHeader->pPlatformPrivate);

		TIMM_OSAL_Free(pCompPrv->tBufList[count].pBufHeader);
		TIMM_OSAL_Memset(&(pCompPrv->tBufList[count]), 0,
		    sizeof(PROXY_BUFFER_INFO));
	}
	pCompPrv->nAllocatedBuffers--;
/*
TODO : Demap although not very critical
Unmapping
#ifdef TILER_BUFF

SysLinkMemUtils_unmap (UInt32 mappedAddr, ProcMgr_ProcId procId)
How do you access UV mapped buffers from here. (Y is accessbile from tBufList under pBuffermapped.
The UV is not, may be consider adding this to the table
*/

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_SetParameter()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_IN OMX_PTR pParamStruct)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nParamIndex=%d, pParamStruct=%p",
	    hComponent, pCompPrv, nParamIndex, pParamStruct);

	eRPCError =
	    RPC_SetParameter(pCompPrv->hRemoteComp, nParamIndex, pParamStruct,
	    &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_GetParameter()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nParamIndex=%d, pParamStruct=%p",
	    hComponent, pCompPrv, nParamIndex, pParamStruct);

	eRPCError =
	    RPC_GetParameter(pCompPrv->hRemoteComp, nParamIndex, pParamStruct,
	    &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_GetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_GetConfig(OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nConfigIndex, OMX_PTR pConfigStruct)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pConfigStruct != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nConfigIndex=%d, pConfigStruct=%p",
	    hComponent, pCompPrv, nConfigIndex, pConfigStruct);

	eRPCError =
	    RPC_GetConfig(pCompPrv->hRemoteComp, nConfigIndex, pConfigStruct,
	    &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_SetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_SetConfig(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nConfigIndex, OMX_IN OMX_PTR pConfigStruct)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pConfigStruct != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nConfigIndex=%d, pConfigStruct=%p",
	    hComponent, pCompPrv, nConfigIndex, pConfigStruct);

	eRPCError =
	    RPC_SetConfig(pCompPrv->hRemoteComp, nConfigIndex, pConfigStruct,
	    &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_GetState()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_GetState(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE * pState)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_COMPONENTTYPE *hComp = hComponent;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;

	PROXY_require((pState != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent=%p, pCompPrv=%p", hComponent, pCompPrv);

	eRPCError = RPC_GetState(pCompPrv->hRemoteComp, pState, &eCompReturn);

	DOMX_DEBUG("Returned from RPC_GetState, state: ", *pState);

	PROXY_checkRpcError();

      EXIT:
	/* In case of hardware error, component is in unrecoverable state */
	if (eError == OMX_ErrorHardware)
		*pState = OMX_StateInvalid;

	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_SendCommand()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE eCmd,
    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_COMPONENTTYPE *pMarkComp = NULL;
	PROXY_COMPONENT_PRIVATE *pMarkCompPrv = NULL;
	OMX_PTR pMarkData = NULL, pMarkToBeFreedIfError = NULL;
	OMX_U32 i;
	OMX_BOOL bIsProxy = OMX_FALSE;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, eCmd=%d, nParam=%d, pCmdData=%p",
	    hComponent, pCompPrv, eCmd, nParam, pCmdData);

	if (eCmd == OMX_CommandMarkBuffer)
	{
		PROXY_assert(pCmdData != NULL, OMX_ErrorBadParameter, NULL);
		pMarkComp = (OMX_COMPONENTTYPE *)
		    (((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent);
		PROXY_assert(pMarkComp != NULL, OMX_ErrorBadParameter, NULL);

		/* To check if mark comp is a proxy or a real component */
		eError = _RPC_IsProxyComponent(pMarkComp, &bIsProxy);
		PROXY_assert(eError == OMX_ErrorNone, eError, "");

		/*Replacing original mark data with proxy specific structure */
		pMarkData = ((OMX_MARKTYPE *) pCmdData)->pMarkData;
		((OMX_MARKTYPE *) pCmdData)->pMarkData =
		    TIMM_OSAL_Malloc(sizeof(PROXY_MARK_DATA), TIMM_OSAL_TRUE,
		    0, TIMMOSAL_MEM_SEGMENT_INT);
		PROXY_assert(((OMX_MARKTYPE *) pCmdData)->pMarkData != NULL,
		    OMX_ErrorInsufficientResources, "Malloc failed");
		pMarkToBeFreedIfError =
		    ((OMX_MARKTYPE *) pCmdData)->pMarkData;
		((PROXY_MARK_DATA *) (((OMX_MARKTYPE *) pCmdData)->
			pMarkData))->hComponentActual = pMarkComp;
		((PROXY_MARK_DATA *) (((OMX_MARKTYPE *) pCmdData)->
			pMarkData))->pMarkDataActual = pMarkData;

		/* If it is proxy component then replace hMarkTargetComponent
		   with remote handle */
		if (bIsProxy)
		{
			pMarkCompPrv = pMarkComp->pComponentPrivate;
			PROXY_assert(pMarkCompPrv != NULL,
			    OMX_ErrorBadParameter, NULL);

			/* Replacing with remote component handle */
			((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent =
			    ((RPC_OMX_CONTEXT *) pMarkCompPrv->
			    hRemoteComp)->hActualRemoteCompHandle;
		}
	} else if ((eCmd == OMX_CommandStateSet) &&
	    (nParam == (OMX_STATETYPE) OMX_StateLoaded))
	{
		/*Reset any component related cached values here */
		for (i = 0; i < PROXY_MAXNUMOFPORTS; i++)
			pCompPrv->nNumOfLines[i] = 0;
	} else if (eCmd == OMX_CommandPortDisable)
	{
		/*Reset any port related cached values here */
		if (nParam == OMX_ALL)
		{
			for (i = 0; i < PROXY_MAXNUMOFPORTS; i++)
				pCompPrv->nNumOfLines[i] = 0;
		} else
		{
			PROXY_assert(nParam < PROXY_MAXNUMOFPORTS,
			    OMX_ErrorBadParameter, "Invalid Port Number");
			pCompPrv->nNumOfLines[nParam] = 0;
		}
	}

	eRPCError =
	    RPC_SendCommand(pCompPrv->hRemoteComp, eCmd, nParam, pCmdData,
	    &eCompReturn);

	if (eCmd == OMX_CommandMarkBuffer && bIsProxy)
	{
		/*Resetting to original values */
		((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent = pMarkComp;
		((OMX_MARKTYPE *) pCmdData)->pMarkData = pMarkData;
	}

	PROXY_checkRpcError();

      EXIT:
	/*If SendCommand is about to return an error then this means that the
	   command has not been accepted by the component. Thus the allocated mark data
	   will be lost so free it here */
	if ((eError != OMX_ErrorNone) && pMarkToBeFreedIfError)
	{
		TIMM_OSAL_Free(pMarkToBeFreedIfError);
	}
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_GetComponentVersion()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_GetComponentVersion(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE * pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE * pSpecVersion,
    OMX_OUT OMX_UUIDTYPE * pComponentUUID)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = hComponent;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_ERRORTYPE eCompReturn;

	DOMX_ENTER("hComponent=%p, pCompPrv=%p", hComponent, pCompPrv);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_assert(pComponentName != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(pComponentVersion != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(pSpecVersion != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(pComponentUUID != NULL, OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	eRPCError = RPC_GetComponentVersion(pCompPrv->hRemoteComp,
	    pComponentName,
	    pComponentVersion, pSpecVersion, pComponentUUID, &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_GetExtensionIndex()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE * pIndexType)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = hComponent;
	OMX_ERRORTYPE eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_assert(cParameterName != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_assert(pIndexType != NULL, OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent=%p, pCompPrv=%p, cParameterName=%s",
	    hComponent, pCompPrv, cParameterName);

	eRPCError = RPC_GetExtensionIndex(pCompPrv->hRemoteComp,
	    cParameterName, pIndexType, &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}




/* ===========================================================================*/
/**
 * @name PROXY_ComponentRoleEnum()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 * cRole, OMX_IN OMX_U32 nIndex)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("hComponent=%p", hComponent);
	DOMX_DEBUG(" EMPTY IMPLEMENTATION ");

	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_ComponentTunnelRequest()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_ComponentTunnelRequest(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_U32 nPort, OMX_IN OMX_HANDLETYPE hTunneledComp,
    OMX_IN OMX_U32 nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE * pTunnelSetup)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("hComponent=%p", hComponent);
	DOMX_DEBUG(" EMPTY IMPLEMENTATION ");

	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_SetCallbacks()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_SetCallbacks(OMX_HANDLETYPE hComponent,
    OMX_CALLBACKTYPE * pCallBacks, OMX_PTR pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pCallBacks != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent=%p, pCompPrv=%p", hComponent, pCompPrv);

	/*Store App callback and data to proxy- managed by proxy */
	pCompPrv->tCBFunc = *pCallBacks;
	pCompPrv->pILAppData = pAppData;

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_UseEGLImage()
 * @brief : This returns error not implemented by default as no component
 *          implements this. In case there is a requiremet, support for this
 *          can be added later.
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_UseEGLImage(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void *pBuffer)
{
	return OMX_ErrorNotImplemented;
}


/* ===========================================================================*/
/**
 * @name PROXY_ComponentDeInit()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_ComponentDeInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 i = 0, count = 0;

	DOMX_ENTER("hComponent=%p", hComponent);

	if (TIMM_OSAL_ERR_NONE !=
	    TIMM_OSAL_MutexObtain(pFaultMutex, TIMM_OSAL_SUSPEND))
	{
		DOMX_ERROR("Error obtaining the ducati fault mutex");
		DOMX_ERROR
		    ("Continuing clean up despite failing to acquire ducati fault mutex");
		eError = OMX_ErrorUndefined;
	}

	for (i = 0; i < MAX_NUM_COMPS_PER_PROCESS; i++)
	{
		if (componentTable[i] == hComponent)
		{
			componentTable[i] = 0;
			currentNumOfComps -= 1;
			if (currentNumOfComps < 0)
			{
				DOMX_ERROR
				    ("Number of components created by a process cannot be less than zero");
			}
			break;
		}
	}
	TIMM_OSAL_MutexRelease(pFaultMutex);
	if (i == MAX_NUM_COMPS_PER_PROCESS)
	{
		goto EXIT;	/* This component has already been cleaned up */
	}

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	if (ducatiFault)
	{
		/*Call FreeBuffer in case it was not called by the client. If
		   FreeBuffer was already called, the header in the list will be
		   NULL and FreeBuffer will return */
		for (count = 0; count < pCompPrv->nTotalBuffers; count++)
		{
			/* The FreeBuffer API's 2nd argument should be
			   a valid port index. The OMX_BUFFERHEADERTYPE
			   structure containts two fields
			   nOuputPortIndex and nInputPortIndex. The OMX
			   IL standard does not specify what value of
			   port index is an invalid value. Hence, at
			   this point of error recovery, it is not
			   possible to distigush which of the indices
			   contains a valide value. However, this does
			   not matter as the main intent of calling
			   FreeBuffer here is to revert the memory mapping
			   and to de-allocate omx buffer headers. Hence,
			   a default value of '0' is passed as port
			   index */
			hComp->FreeBuffer(hComponent, 0,
			    pCompPrv->tBufList[count].pBufHeader);
		}
	}

	eRPCError = RPC_FreeHandle(pCompPrv->hRemoteComp, &eCompReturn);
	PROXY_checkRpcError();

	eRPCError = RPC_InstanceDeInit(pCompPrv->hRemoteComp);
	if (eRPCError != RPC_OMX_ErrorNone)
	{
		DOMX_ERROR("RPC_InstanceDeInit failed with error 0x%x",
		    eRPCError);
		eError = OMX_ErrorUndefined;
	}

	if (pCompPrv->cCompName)
	{
		TIMM_OSAL_Free(pCompPrv->cCompName);
	}

	if (pCompPrv)
	{
		TIMM_OSAL_Free(pCompPrv);
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name OMX_ProxyCommonInit()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE OMX_ProxyCommonInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	RPC_OMX_HANDLE hRemoteComp;
	OMX_U32 i = 0;
	//OMX_CALLBACKTYPE tProxyCBinfo = {PROXY_EventHandler, PROXY_EmptyBufferDone, PROXY_FillBufferDone};

	DOMX_ENTER("hComponent=%p", hComponent);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	PROXY_assert((currentNumOfComps < MAX_NUM_COMPS_PER_PROCESS),
	    OMX_ErrorInsufficientResources,
	    "Maximum number of components that can be created per process reached");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	pCompPrv->nTotalBuffers = 0;
	pCompPrv->nAllocatedBuffers = 0;
	pCompPrv->proxyEmptyBufferDone = PROXY_EmptyBufferDone;
	pCompPrv->proxyFillBufferDone = PROXY_FillBufferDone;
	pCompPrv->proxyEventHandler = PROXY_EventHandler;

	/*reset num of lines info per port */
	for (i = 0; i < PROXY_MAXNUMOFPORTS; i++)
		pCompPrv->nNumOfLines[i] = 0;

	eRPCError = RPC_InstanceInit(pCompPrv->cCompName, &hRemoteComp);
	PROXY_assert(eRPCError == RPC_OMX_ErrorNone,
	    OMX_ErrorUndefined, "Error initializing RPC");
	PROXY_assert(hRemoteComp != NULL,
	    OMX_ErrorUndefined, "Error initializing RPC");

	//Send the proxy component handle for pAppData
	eRPCError =
	    RPC_GetHandle(hRemoteComp, pCompPrv->cCompName,
	    (OMX_PTR) hComponent, NULL, &eCompReturn);

	PROXY_checkRpcError();
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->hRemoteComp = hRemoteComp;
	} else
	{
		RPC_InstanceDeInit(hRemoteComp);
		goto EXIT;
	}

	hComp->SetCallbacks = PROXY_SetCallbacks;
	hComp->ComponentDeInit = PROXY_ComponentDeInit;
	hComp->UseBuffer = PROXY_UseBuffer;
	hComp->GetParameter = PROXY_GetParameter;
	hComp->SetParameter = PROXY_SetParameter;
	hComp->EmptyThisBuffer = PROXY_EmptyThisBuffer;
	hComp->FillThisBuffer = PROXY_FillThisBuffer;
	hComp->GetComponentVersion = PROXY_GetComponentVersion;
	hComp->SendCommand = PROXY_SendCommand;
	hComp->GetConfig = PROXY_GetConfig;
	hComp->SetConfig = PROXY_SetConfig;
	hComp->GetState = PROXY_GetState;
	hComp->GetExtensionIndex = PROXY_GetExtensionIndex;
	hComp->FreeBuffer = PROXY_FreeBuffer;
	hComp->ComponentRoleEnum = PROXY_ComponentRoleEnum;
	hComp->AllocateBuffer = PROXY_AllocateBuffer;
	hComp->ComponentTunnelRequest = PROXY_ComponentTunnelRequest;
	hComp->UseEGLImage = PROXY_UseEGLImage;

	pCompPrv->hRemoteComp = hRemoteComp;

        if (TIMM_OSAL_ERR_NONE !=
            TIMM_OSAL_MutexObtain(pFaultMutex, TIMM_OSAL_SUSPEND))
        {
          RPC_FreeHandle(hRemoteComp, &eCompReturn);
          RPC_InstanceDeInit(hRemoteComp);
          PROXY_assert(0, OMX_ErrorUndefined,
		    "Error obtaining the Ducati component table mutex");
        }

        for (i = 0; i < MAX_NUM_COMPS_PER_PROCESS; i++)
        {
          if (componentTable[i] == 0)
          {
            componentTable[i] = hComponent;
            currentNumOfComps++;
            break;
          }
        }

        TIMM_OSAL_MutexRelease(pFaultMutex);
	DOMX_DEBUG(" Proxy Initted");

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}




OMX_ERRORTYPE RPC_PrepareBuffer_Chiron(PROXY_COMPONENT_PRIVATE * pCompPrv,
    OMX_COMPONENTTYPE * hRemoteComp, OMX_U32 nPortIndex, OMX_U32 nSizeBytes,
    OMX_BUFFERHEADERTYPE * pDucBuf, OMX_BUFFERHEADERTYPE * pChironBuf)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 nNumOfLines = 1;
	OMX_U8 *pBuffer;

	DSPtr dsptr[2];
	bytes_t lengths[2];
	OMX_U32 i = 0;
	OMX_U32 numBlocks = 0;

	pBuffer = pDucBuf->pBuffer;

	DOMX_ENTER("");

	if (((OMX_TI_PLATFORMPRIVATE *) pDucBuf->
		pPlatformPrivate)->pAuxBuf1 == NULL)
	{
		DOMX_DEBUG("One component buffer");

		if (!(pCompPrv->nNumOfLines[nPortIndex]))
		{
			pCompPrv->nNumOfLines[nPortIndex] = 1;
		}

		dsptr[0] = (OMX_U32) pBuffer;
		numBlocks = 1;
		lengths[0] =
		    LINUX_PAGE_SIZE * ((nSizeBytes + (LINUX_PAGE_SIZE -
			    1)) / LINUX_PAGE_SIZE);
	} else
	{
		DOMX_DEBUG("Two component buffers");
		dsptr[0] = (OMX_U32) pBuffer;
		dsptr[1] =
		    (OMX_U32) (((OMX_TI_PLATFORMPRIVATE *)
			pDucBuf->pPlatformPrivate)->pAuxBuf1);

		if (!(pCompPrv->nNumOfLines[nPortIndex]))
		{
			eError =
			    RPC_UTIL_GetNumLines(hRemoteComp, nPortIndex,
			    &nNumOfLines);
			PROXY_assert((eError == OMX_ErrorNone),
			    OMX_ErrorUndefined,
			    "ERROR WHILE GETTING FRAME HEIGHT");

			pCompPrv->nNumOfLines[nPortIndex] = nNumOfLines;
		} else
		{
			nNumOfLines = pCompPrv->nNumOfLines[nPortIndex];
		}

		lengths[0] = nNumOfLines * LINUX_PAGE_SIZE;
		lengths[1] = nNumOfLines / 2 * LINUX_PAGE_SIZE;
		numBlocks = 2;
	}

	//Map back to chiron
	DOMX_DEBUG("NumBlocks = %d", numBlocks);
	for (i = 0; i < numBlocks; i++)
	{
		DOMX_DEBUG("dsptr[%d] = %p", i, dsptr[i]);
		DOMX_DEBUG("length[%d] = %d", i, lengths[i]);
	}

	pDucBuf->pBuffer =
	    tiler_assisted_phase1_D2CReMap(numBlocks, dsptr, lengths);
	PROXY_assert((pDucBuf->pBuffer != NULL), OMX_ErrorUndefined,
	    "Mapping to Chiron failed");

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


//Takes chiron buffer buffer header and updates with ducati buffer ptr and UV ptr
OMX_ERRORTYPE RPC_PrepareBuffer_Remote(PROXY_COMPONENT_PRIVATE * pCompPrv,
    OMX_COMPONENTTYPE * hRemoteComp, OMX_U32 nPortIndex,
    OMX_U32 nSizeBytes, OMX_BUFFERHEADERTYPE * pChironBuf,
    OMX_BUFFERHEADERTYPE * pDucBuf, OMX_PTR pBufToBeMapped)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 nNumOfLines = 1;
	OMX_U8 *pBuffer;

	DOMX_ENTER("");

	pBuffer = pChironBuf->pBuffer;

	if (!MemMgr_Is2DBlock(pBuffer))
	{

		if (!(pCompPrv->nNumOfLines[nPortIndex]))
		{
			pCompPrv->nNumOfLines[nPortIndex] = 1;
		}

		pChironBuf->pBuffer = NULL;
		eError =
		    RPC_MapBuffer_Ducati(pBuffer, nSizeBytes, nNumOfLines,
		    &(pChironBuf->pBuffer), pBufToBeMapped);
		PROXY_assert(eError == OMX_ErrorNone, eError, "Map failed");
	} else
	{
		if (!(pCompPrv->nNumOfLines[nPortIndex]))
		{
			eError =
			    RPC_UTIL_GetNumLines(hRemoteComp, nPortIndex,
			    &nNumOfLines);
			PROXY_assert((eError == OMX_ErrorNone), eError,
			    "ERROR WHILE GETTING FRAME HEIGHT");

			pCompPrv->nNumOfLines[nPortIndex] = nNumOfLines;
		} else
		{
			nNumOfLines = pCompPrv->nNumOfLines[nPortIndex];
		}

		pChironBuf->pBuffer = NULL;
		((OMX_TI_PLATFORMPRIVATE *) (pChironBuf->
			pPlatformPrivate))->pAuxBuf1 = NULL;

		eError =
		    RPC_MapBuffer_Ducati(pBuffer, LINUX_PAGE_SIZE,
		    nNumOfLines, &(pChironBuf->pBuffer), pBufToBeMapped);
		PROXY_assert(eError == OMX_ErrorNone, eError, "Map failed");
		eError =
		    RPC_MapBuffer_Ducati((OMX_U8 *) ((OMX_U32) pBuffer +
			nNumOfLines * LINUX_PAGE_SIZE), LINUX_PAGE_SIZE,
		    nNumOfLines / 2,
		    (OMX_U8 **) (&((OMX_TI_PLATFORMPRIVATE
				*) (pChironBuf->pPlatformPrivate))->pAuxBuf1),
		    pBufToBeMapped);
		PROXY_assert(eError == OMX_ErrorNone, eError, "Map failed");
		*(OMX_U32 *) pBufToBeMapped = (OMX_U32) pBuffer;
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name RPC_UTIL_GetNumLines()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UTIL_GetNumLines(OMX_COMPONENTTYPE * hRemoteComp,
    OMX_U32 nPortIndex, OMX_U32 * nNumOfLines)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	OMX_PARAM_PORTDEFINITIONTYPE portDef;
	OMX_CONFIG_RECTTYPE sRect;

	DOMX_ENTER("");

	/*initializing Structure */
	portDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	portDef.nVersion.s.nVersionMajor = 0x1;
	portDef.nVersion.s.nVersionMinor = 0x1;
	portDef.nVersion.s.nRevision = 0x0;
	portDef.nVersion.s.nStep = 0x0;

	portDef.nPortIndex = nPortIndex;

	sRect.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	sRect.nVersion.s.nVersionMajor = 0x1;
	sRect.nVersion.s.nVersionMinor = 0x1;
	sRect.nVersion.s.nRevision = 0x0;
	sRect.nVersion.s.nStep = 0x0;

	sRect.nPortIndex = nPortIndex;
	sRect.nLeft = 0;
	sRect.nTop = 0;
	sRect.nHeight = 0;
	sRect.nWidth = 0;

	eRPCError = RPC_GetParameter(hRemoteComp,
	    OMX_TI_IndexParam2DBufferAllocDimension,
	    (OMX_PTR) & sRect, &eCompReturn);
	if (eRPCError == RPC_OMX_ErrorNone)
	{
		DOMX_DEBUG(" PROXY_UTIL Get Parameter Successful");
		eError = eCompReturn;
	} else
	{
		DOMX_ERROR("RPC_GetParameter returned error 0x%x", eRPCError);
		eError = OMX_ErrorUndefined;
		goto EXIT;
	}

	if (eCompReturn == OMX_ErrorNone)
	{
		*nNumOfLines = sRect.nHeight;
	} else if (eCompReturn == OMX_ErrorUnsupportedIndex)
	{
		eRPCError =
		    RPC_GetParameter(hRemoteComp,
		    OMX_IndexParamPortDefinition, (OMX_PTR) & portDef,
		    &eCompReturn);

		if (eRPCError == RPC_OMX_ErrorNone)
		{
			DOMX_DEBUG(" PROXY_UTIL Get Parameter Successful");
			eError = eCompReturn;
		} else
		{
			DOMX_ERROR("RPC_GetParameter returned error 0x%x",
			    eRPCError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

		if (eCompReturn == OMX_ErrorNone)
		{

			//start with 1 meaning 1D buffer
			*nNumOfLines = 1;

			if (portDef.eDomain == OMX_PortDomainVideo)
			{
				*nNumOfLines =
				    portDef.format.video.nFrameHeight;
				//DOMX_DEBUG("Port definition Type is video...");
				//DOMX_DEBUG("&&Colorformat is:%p", portDef.format.video.eColorFormat);
				//DOMX_DEBUG("nFrameHeight is:%d", portDef.format.video.nFrameHeight);
				//*nNumOfLines = portDef.format.video.nFrameHeight;

				//if((portDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar) ||
				//  (portDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar))
				//{
				//DOMX_DEBUG("Setting FrameHeight as Number of lines...");
				//*nNumOfLines = portDef.format.video.nFrameHeight;
				//}
			} else if (portDef.eDomain == OMX_PortDomainImage)
			{
				DOMX_DEBUG
				    ("Image DOMAIN TILER SUPPORT for NV12 format only");
				*nNumOfLines =
				    portDef.format.image.nFrameHeight;
			} else if (portDef.eDomain == OMX_PortDomainAudio)
			{
				DOMX_DEBUG("Audio DOMAIN TILER SUPPORT");
			} else if (portDef.eDomain == OMX_PortDomainOther)
			{
				DOMX_DEBUG("Other DOMAIN TILER SUPPORT");
			} else
			{	//this is the sample component test
				//Temporary - just to get check functionality
				DOMX_DEBUG("Sample component TILER SUPPORT");
				*nNumOfLines = 4;
			}
		} else
		{
			DOMX_ERROR(" ERROR IN RECOVERING UV POINTER");
		}
	} else
	{
		DOMX_ERROR(" ERROR IN RECOVERING UV POINTER");
	}

	DOMX_DEBUG("Port Number: %d :: NumOfLines %d", nPortIndex,
	    *nNumOfLines);

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name RPC_MapBuffer_Ducati()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_MapBuffer_Ducati(OMX_U8 * pBuf, OMX_U32 nBufLineSize,
    OMX_U32 nBufLines, OMX_U8 ** pMappedBuf, OMX_PTR pBufToBeMapped)
{
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap MpuAddr_list_1D = { 0 };
	MemAllocBlock block = { 0 };
	OMX_S32 status;
	OMX_U32 nDiff = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("");

	*(OMX_U32 *) pBufToBeMapped = (OMX_U32) pBuf;

	if (!MemMgr_IsMapped(pBuf) && (nBufLines == 1))
	{
		DOMX_DEBUG
		    ("Buffer is not mapped: Mapping as 1D buffer now..");
		block.pixelFormat = PIXEL_FMT_PAGE;
		block.ptr = (OMX_PTR) (((OMX_U32) pBuf / LINUX_PAGE_SIZE) *
		    LINUX_PAGE_SIZE);
		block.dim.len = (OMX_U32) ((((OMX_U32) pBuf + nBufLineSize +
			    LINUX_PAGE_SIZE - 1) / LINUX_PAGE_SIZE) *
		    LINUX_PAGE_SIZE) - (OMX_U32) block.ptr;
		block.stride = 0;
		nDiff = (OMX_U32) pBuf - (OMX_U32) block.ptr;

		(*(OMX_U32 *) (pBufToBeMapped)) =
		    (OMX_U32) (MemMgr_Map(&block, 1));
		PROXY_assert(*(OMX_U32 *) pBufToBeMapped != 0,
		    OMX_ErrorInsufficientResources,
		    "Map to TILER space failed");
		//*pMappedBuf = MemMgr_Map(&block, 1);
	}

	if (MemMgr_IsMapped((OMX_PTR) (*(OMX_U32 *) pBufToBeMapped)))
	{
		//If Tiler 1D buffer, get corresponding ducati address and send out buffer to ducati
		//For 2D buffers, in phase1, retrive the ducati address (SSPtrs) for Y and UV buffers
		//and send out buffer to ducati
		mapType = ProcMgr_MapType_Tiler;
		MpuAddr_list_1D.mpuAddr =
		    (*(OMX_U32 *) pBufToBeMapped) + nDiff;
		MpuAddr_list_1D.size = nBufLineSize * nBufLines;

		status =
		    SysLinkMemUtils_map(&MpuAddr_list_1D, 1,
		    (UInt32 *) pMappedBuf, mapType, PROC_APPM3);
		PROXY_assert(status >= 0, OMX_ErrorInsufficientResources,
		    "Syslink map failed");
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name RPC_UnMapBuffer_Ducati()
 * @brief
 * @param
 * @return
 * @sa
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UnMapBuffer_Ducati(OMX_PTR pBuffer)
{
	OMX_U32 status = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("");

	status = MemMgr_UnMap(pBuffer);
	PROXY_assert(status == 0, OMX_ErrorUndefined,
	    "MemMgr_UnMap returned an error");

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name RPC_MapMetaData_Host()
 * @brief This utility maps metadata buffer in OMX buffer header to Chiron
 * virtual address space (metadata buffer is TILER 1D buffer in Ducati Virtual
 * space). It overrides the metadata buffer with Chiron address in the same
 * field. Metadata buffer size represents max size (alloc size) that needs to
 * be mapped
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_MapMetaData_Host(OMX_BUFFERHEADERTYPE * pBufHdr)
{
	OMX_PTR pMappedMetaDataBuffer = NULL;
	OMX_U32 nMetaDataSize = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DSPtr dsptr[2];
	bytes_t lengths[2];
	OMX_U32 numBlocks = 0;

	DOMX_ENTER("");

	if ((pBufHdr->pPlatformPrivate != NULL) &&
	    (((OMX_TI_PLATFORMPRIVATE *) pBufHdr->
		    pPlatformPrivate)->pMetaDataBuffer != NULL))
	{

		pMappedMetaDataBuffer = NULL;

		nMetaDataSize =
		    ((OMX_TI_PLATFORMPRIVATE *) pBufHdr->
		    pPlatformPrivate)->nMetaDataSize;
		PROXY_assert((nMetaDataSize != 0), OMX_ErrorBadParameter,
		    "Received ZERO metadata size from Ducati, cannot map");

		dsptr[0] =
		    (OMX_U32) ((OMX_TI_PLATFORMPRIVATE *)
		    pBufHdr->pPlatformPrivate)->pMetaDataBuffer;
		numBlocks = 1;
		lengths[0] =
		    LINUX_PAGE_SIZE * ((nMetaDataSize + (LINUX_PAGE_SIZE -
			    1)) / LINUX_PAGE_SIZE);

		pMappedMetaDataBuffer =
		    tiler_assisted_phase1_D2CReMap(numBlocks, dsptr, lengths);

		PROXY_assert((pMappedMetaDataBuffer != NULL),
		    OMX_ErrorInsufficientResources,
		    "Mapping metadata to Chiron space failed");

		((OMX_TI_PLATFORMPRIVATE *) pBufHdr->
		    pPlatformPrivate)->pMetaDataBuffer =
		    pMappedMetaDataBuffer;
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name RPC_UnMapMetaData_Host()
 * @brief This utility unmaps the previously mapped metadata on host from remote
 * components
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UnMapMetaData_Host(OMX_BUFFERHEADERTYPE * pBufHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_S32 nReturn = 0;

	DOMX_ENTER("");

	if ((pBufHdr->pPlatformPrivate != NULL) &&
	    (((OMX_TI_PLATFORMPRIVATE *) pBufHdr->
		    pPlatformPrivate)->pMetaDataBuffer != NULL))
	{

		nReturn =
		    tiler_assisted_phase1_DeMap((((OMX_TI_PLATFORMPRIVATE *)
			    pBufHdr->pPlatformPrivate)->pMetaDataBuffer));
		PROXY_assert((nReturn == 0), OMX_ErrorUndefined,
		    "Metadata unmap failed");
	}
      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name _RPC_IsProxyComponent()
 * @brief This function calls GetComponentVersion API on the component and
 *        based on the component name decidec whether the component is a proxy
 *        or real component. The naming component convention assumed is
 *        <OMX>.<Company Name>.<Core Name>.<Domain>.<Component Details> with
 *        all characters in upper case for e.g. OMX.TI.DUCATI1.VIDEO.H264E
 * @param hComponent [IN] : The component handle
 *        bIsProxy [OUT]  : Set to true is handle is for a proxy component
 * @return OMX_ErrorNone = Successful
 *
 **/
/* ===========================================================================*/
OMX_ERRORTYPE _RPC_IsProxyComponent(OMX_HANDLETYPE hComponent,
    OMX_BOOL * bIsProxy)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_S8 cComponentName[MAXNAMESIZE] = { 0 }
	, cCoreName[MAX_SERVER_NAME_LENGTH] =
	{
	0};
	OMX_VERSIONTYPE sCompVer, sSpecVer;
	OMX_UUIDTYPE sCompUUID;
	OMX_U32 i = 0, ret = 0;

	eError =
	    OMX_GetComponentVersion(hComponent, (OMX_STRING) cComponentName,
	    &sCompVer, &sSpecVer, &sCompUUID);
	PROXY_assert(eError == OMX_ErrorNone, eError, "");
	ret =
	    sscanf((char *)cComponentName, "%*[^'.'].%*[^'.'].%[^'.'].%*s",
	    cCoreName);
	PROXY_assert(ret == 1, OMX_ErrorBadParameter,
	    "Incorrect component name");
	for (i = 0; i < CORE_MAX; i++)
	{
		if (strcmp((char *)cCoreName, Core_Array[i]) == 0)
			break;
	}
	PROXY_assert(i < CORE_MAX, OMX_ErrorBadParameter,
	    "Unknown core name");

	/* If component name indicates remote core, it means proxy
	   component */
	if ((i == CORE_SYSM3) || (i == CORE_APPM3) || (i == CORE_TESLA))
		*bIsProxy = OMX_TRUE;
	else
		*bIsProxy = OMX_FALSE;

      EXIT:
	return eError;
}
