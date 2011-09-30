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
 *! 18-May-2010 Dandawate Saket dsaket@ti.com: Initial Version
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
#define LOG_TAG "OMX_PROXYDEC"
#define DOMX_DEBUG LOGE
#endif

#ifdef __H264_DEC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.H264D"
#elif __MPEG4_DEC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.MPEG4D"
#elif __VP6_DEC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.VP6D"
#elif __VP7_DEC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.VP7D"
#elif __VID_DEC__
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER"
#endif

/* DEFINITIONS for parsing the config information & sequence header for WMV*/
#define VIDDEC_GetUnalignedDword( pb, dw ) \
	(dw) = ((OMX_U32) *(pb + 3) << 24) + \
		((OMX_U32) *(pb + 2) << 16) + \
		((OMX_U16) *(pb + 1) << 8) + *pb;

#define VIDDEC_GetUnalignedDwordEx( pb, dw )   VIDDEC_GetUnalignedDword( pb, dw ); (pb) += sizeof(OMX_U32);
#define VIDDEC_LoadDWORD( dw, p )  VIDDEC_GetUnalignedDwordEx( p, dw )
#define VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((OMX_U32)(OMX_U8)(ch0) | ((OMX_U32)(OMX_U8)(ch1) << 8) |   \
	((OMX_U32)(OMX_U8)(ch2) << 16) | ((OMX_U32)(OMX_U8)(ch3) << 24 ))

#define VIDDEC_FOURCC(ch0, ch1, ch2, ch3)  VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3)

#define FOURCC_WMV3		VIDDEC_FOURCC('W','M','V','3')
#define FOURCC_WMV2		VIDDEC_FOURCC('W','M','V','2')
#define FOURCC_WMV1		VIDDEC_FOURCC('W','M','V','1')
#define FOURCC_WVC1		VIDDEC_FOURCC('W','V','C','1')

#define CSD_POSITION	51 /*Codec Specific Data position on the "stream propierties object"(ASF spec)*/


typedef struct VIDDEC_WMV_RCV_struct {
	OMX_U32 nNumFrames : 24;
	OMX_U32 nFrameType : 8;
	OMX_U32 nID : 32;
	OMX_U32 nStructData : 32;   //STRUCT_C
	OMX_U32 nVertSize;          //STRUCT_A-1
	OMX_U32 nHorizSize;         //STRUCT_A-2
	OMX_U32 nID2 : 32;
	OMX_U32 nSequenceHdr : 32;   //STRUCT_B
} VIDDEC_WMV_RCV_struct;

typedef union VIDDEC_WMV_RCV_header {
	VIDDEC_WMV_RCV_struct *sStructRCV;
	OMX_U8 *pBuffer;
} VIDDEC_WMV_RCV_header;

typedef struct VIDDEC_WMV_VC1_struct {
	OMX_U32 nNumFrames 	: 24;
	OMX_U32 nFrameType 	: 8;
	OMX_U32 nID 		: 32;
	OMX_U32 nStructData : 32;   //STRUCT_C
	OMX_U32 nVertSize;          //STRUCT_A-1
	OMX_U32 nHorizSize;         //STRUCT_A-2
	OMX_U32 nID2 		: 32;
	OMX_U32 nSequenceHdr : 32;   //STRUCT_B
} VIDDEC_WMV_VC1_struct;

typedef union VIDDEC_WMV_VC1_header {
	VIDDEC_WMV_VC1_struct *sStructVC1;
	OMX_U8 *pBuffer;
} VIDDEC_WMV_VC1_header;


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
		    OMX_TRUE;
		pPVCapaFlags->iOMXComponentSupportsExternalInputBufferAlloc =
		    OMX_FALSE;
		pPVCapaFlags->iOMXComponentSupportsMovableInputBuffers =
		    OMX_FALSE;
		pPVCapaFlags->iOMXComponentSupportsPartialFrames = OMX_FALSE;
		pPVCapaFlags->iOMXComponentCanHandleIncompleteFrames =
		    OMX_FALSE;
#if defined(__H264_DEC__) || defined(__VID_DEC__)
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



static OMX_ERRORTYPE PrearrageEmptyThisBuffer(OMX_HANDLETYPE hComponent,
	OMX_BUFFERHEADERTYPE * pBufferHdr)
{

	LOGV("Inside PrearrageEmptyThisBuffer");
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U8* pBuffer = NULL;
	OMX_U8* pData = NULL;
	OMX_U32 nValue = 0;
	OMX_U32 nWidth = 0;
	OMX_U32 nHeight = 0;
	OMX_U32 nActualCompression = 0;
	OMX_U8* pCSD = NULL;
	OMX_U32 nSize_CSD = 0;

	PROXY_assert(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);

	if (pBufferHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG){
		PROXY_assert(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter, NULL);

		pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;
		pBuffer = pBufferHdr->pBuffer;

		VIDDEC_WMV_RCV_header  hBufferRCV;
		VIDDEC_WMV_VC1_header  hBufferVC1;

		LOGV("nFlags: %x", pBufferHdr->nFlags);

		pData = pBufferHdr->pBuffer + 15; /*Position to Width & Height*/
		VIDDEC_LoadDWORD(nValue, pData);
		nWidth = nValue;
		VIDDEC_LoadDWORD(nValue, pData);
		nHeight = nValue;

		pData += 4; /*Position to compression type*/
		VIDDEC_LoadDWORD(nValue, pData);
		nActualCompression = nValue;

		/*Seting pCSD to proper position*/
		pCSD = pBufferHdr->pBuffer;
		pCSD += CSD_POSITION;
		nSize_CSD = pBufferHdr->nFilledLen - CSD_POSITION;

		if(nActualCompression == FOURCC_WMV3){
			hBufferRCV.sStructRCV =
				(VIDDEC_WMV_RCV_struct *)
				TIMM_OSAL_Malloc(sizeof(VIDDEC_WMV_RCV_struct), TIMM_OSAL_TRUE,
				0, TIMMOSAL_MEM_SEGMENT_INT);

			PROXY_assert(hBufferRCV.sStructRCV != NULL,
				OMX_ErrorInsufficientResources, "Malloc failed");

			//From VC-1 spec: Table 265: Sequence Layer Data Structure
			hBufferRCV.sStructRCV->nNumFrames = 0xFFFFFF; /*Infinite frame number*/
			hBufferRCV.sStructRCV->nFrameType = 0xc5; /*0x85 is the value given by ASF to rcv converter*/
			hBufferRCV.sStructRCV->nID = 0x04; /*WMV3*/
			hBufferRCV.sStructRCV->nStructData = 0x018a3106; /*0x06318a01zero fill 0x018a3106*/
			hBufferRCV.sStructRCV->nVertSize = nHeight;
			hBufferRCV.sStructRCV->nHorizSize = nWidth;
			hBufferRCV.sStructRCV->nID2 = 0x0c; /* Fix value */
			hBufferRCV.sStructRCV->nSequenceHdr = 0x00002a9f; /* This value is not provided by parser, so giving a value from a video*/

			LOGV("initial: nStructData: %x", hBufferRCV.sStructRCV->nStructData);
			LOGV("pCSD = %x", (OMX_U32)*pCSD);

			hBufferRCV.sStructRCV->nStructData = (OMX_U32)pCSD[0] << 0  |
				pCSD[1] << 8  |
				pCSD[2] << 16 |
				pCSD[3] << 24;

			LOGV("FINAL: nStructData: %x", hBufferRCV.sStructRCV->nStructData);

			//Copy RCV structure to actual buffer
			assert(pBufferHdr->nFilledLen < pBufferHdr->nAllocLen);
			pBufferHdr->nFilledLen = sizeof(VIDDEC_WMV_RCV_struct);
			TIMM_OSAL_Memcpy(pBufferHdr->pBuffer, (OMX_U8*)hBufferRCV.pBuffer,
				pBufferHdr->nFilledLen);

			//Free aloocated memory
			TIMM_OSAL_Free(hBufferRCV.sStructRCV);
		}
		else if (nActualCompression == FOURCC_WVC1){
			LOGV("VC-1 Advance Profile prearrange");
			OMX_U8* pTempBuf =
				(OMX_U8 *)
				TIMM_OSAL_Malloc(pBufferHdr->nFilledLen, TIMM_OSAL_TRUE,
				0, TIMMOSAL_MEM_SEGMENT_INT);

			PROXY_assert(pTempBuf != NULL,
				OMX_ErrorInsufficientResources, "Malloc failed");

			TIMM_OSAL_Memcpy(pTempBuf, pBufferHdr->pBuffer+52,
				pBufferHdr->nFilledLen-52);
			TIMM_OSAL_Memcpy(pBufferHdr->pBuffer, pTempBuf,
				pBufferHdr->nFilledLen-52);
			pBufferHdr->nFilledLen -= 52;

			TIMM_OSAL_Free(pTempBuf);
		}
	}

	EXIT:
	LOGV("Redirection from PrearrageEmptyThisBuffer to PROXY_EmptyThisBuffer, nFilledLen=%d, nAllocLen=%d", pBufferHdr->nFilledLen, pBufferHdr->nAllocLen);

	return PROXY_EmptyThisBuffer(hComponent, pBufferHdr);
}



OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	DOMX_DEBUG("___INSISDE VIDEO DECODER PROXY WRAPPER__\n");

	pHandle->pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
	if (pHandle->pComponentPrivate == NULL)
	{
		DOMX_DEBUG
		    (" ERROR IN ALLOCATING PROXY COMPONENT PRIVATE STRUCTURE");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	pComponentPrivate->cCompName =
	    (OMX_U8 *) TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH *
	    sizeof(OMX_U8), TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	if (pComponentPrivate->cCompName == NULL)
	{
		DOMX_DEBUG
		    (" ERROR IN ALLOCATING PROXY COMPONENT NAME STRUCTURE");
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
		DOMX_DEBUG("Error in Initializing Proxy");
		TIMM_OSAL_Free(pComponentPrivate->cCompName);
		TIMM_OSAL_Free(pComponentPrivate);
	}
	// Make sure private function to component is always assigned
	// after component init.
#ifdef _OPENCORE
	pHandle->GetParameter = ComponentPrivateGetParameters;
#endif
	pHandle->EmptyThisBuffer = PrearrageEmptyThisBuffer;

	EXIT:
	return eError;
}
