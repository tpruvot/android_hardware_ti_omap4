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
 *  @file  omx_proxy_common.h
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework OMX Common Proxy.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_proxy_common\
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

#ifndef OMX_PROXY_H
#define OMX_PROXY_H

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/* ------compilation control switches ----------------------------------------*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <OMX_Core.h>
/*-------program files ----------------------------------------*/
#include "omx_rpc.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*OMX versions supported by DOMX*/
#define OMX_VER_MAJOR 0x1
#define OMX_VER_MINOR 0x1

#define MAX_NUM_PROXY_BUFFERS             50
#define MAX_COMPONENT_NAME_LENGTH         128
#define PROXY_MAXNUMOFPORTS               8

/******************************************************************
 *   MACROS - ASSERTS
 ******************************************************************/
#define PROXY_assert  PROXY_paramCheck
#define PROXY_require PROXY_paramCheck
#define PROXY_ensure  PROXY_paramCheck

#define PROXY_paramCheck(C, V, S) do {\
    if (!(C)) { eError = V;\
    DOMX_ERROR("failed check: " #C);\
    DOMX_ERROR(" - returning error: " #V);\
    DOMX_ERROR("Error code: 0x%x", V);\
    if(S) DOMX_ERROR(" - %s", S);\
    goto EXIT; }\
    } while(0)

#define PROXY_CHK_VERSION(_pStruct_, _sName_) do { \
    PROXY_require((((_sName_ *)_pStruct_)->nSize == sizeof(_sName_)), \
                  OMX_ErrorBadParameter, "Incorrect nSize"); \
    PROXY_require(((((_sName_ *)_pStruct_)->nVersion.s.nVersionMajor == \
                  OMX_VER_MAJOR) && \
                  (((_sName_ *)_pStruct_)->nVersion.s.nVersionMinor == \
                  OMX_VER_MINOR)), \
                  OMX_ErrorVersionMismatch, NULL); \
    } while(0)


	typedef OMX_ERRORTYPE(*PROXY_EMPTYBUFFER_DONE) (OMX_HANDLETYPE
	    hComponent, OMX_U32 remoteBufHdr, OMX_U32 nfilledLen,
	    OMX_U32 nOffset, OMX_U32 nFlags);

	typedef OMX_ERRORTYPE(*PROXY_FILLBUFFER_DONE) (OMX_HANDLETYPE
	    hComponent, OMX_U32 remoteBufHdr, OMX_U32 nfilledLen,
	    OMX_U32 nOffset, OMX_U32 nFlags, OMX_TICKS nTimeStamp,
	    OMX_HANDLETYPE hMarkTargetComponent, OMX_PTR pMarkData);

	typedef OMX_ERRORTYPE(*PROXY_EVENTHANDLER) (OMX_HANDLETYPE hComponent,
	    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
	    OMX_U32 nData2, OMX_PTR pEventData);

/*******************************************************************************
* Structures
*******************************************************************************/
/*===============================================================*/
/** PROXY_BUFFER_INFO        : This structure maintains a table of A9 and
 *                             Ducati side buffers and headers.
 *
 * @param pBufHeader         : This is a pointer to the A9 bufferheader.
 *
 * @param pBufHeaderRemote   : This is pointer to Ducati side bufferheader.
 *
 * @param pBufferMapped      : This is the Ducati side buffer.
 *
 * @param pBufferActual      : This is the actual buffer sent by the client.
 *
 * @param actualContent      : Unknown. Remove?
 *
 * @param pAlloc_localBuffCopy : Unknown. Remove?
 *
 * @param pBufferToBeMapped  : This is the pointer that will be used for
 *                             mapping the buffer to Ducati side. For TILER
 *                             buffers, this and pBufferActual will  be the
 *                             same. However for NON TILER buffers, this'll
 *                             be an intermediate pointer. This pointer should
 *                             not be used for r/w or cache operations. It can
 *                             only be used for mapping/unmapping to Ducati
 *                             space.
 * @param bRemoteAllocatedBuffer : True if buffer is allocated by remote core
 *                                 (as in AllocateBuffer case). This is needed
 *                                 to maintain context since in this case the
 *                                 buffer needs to be unmapped during FreeBuffer
 */
/*===============================================================*/
	typedef struct PROXY_BUFFER_INFO
	{
		OMX_BUFFERHEADERTYPE *pBufHeader;
		OMX_U32 pBufHeaderRemote;
		OMX_U32 pBufferMapped;
		OMX_U32 pBufferActual;
		OMX_U32 actualContent;
		OMX_U32 pAlloc_localBuffCopy;
		OMX_U32 pBufferToBeMapped;
		OMX_BOOL bRemoteAllocatedBuffer;
	} PROXY_BUFFER_INFO;

/* ========================================================================== */
/**
* PROXY_COMPONENT_PRIVATE
*
*/
/* ========================================================================== */
	typedef struct PROXY_COMPONENT_PRIVATE
	{
		/* OMX Related Information */
		OMX_CALLBACKTYPE tCBFunc;
		OMX_PTR pILAppData;
		RPC_OMX_HANDLE hRemoteComp;

		PROXY_BUFFER_INFO tBufList[MAX_NUM_PROXY_BUFFERS];
		OMX_U32 nTotalBuffers;
		OMX_U32 nAllocatedBuffers;

		/* PROXY specific data - PROXY PRIVATE DATA */
		char *cCompName;

		PROXY_EMPTYBUFFER_DONE proxyEmptyBufferDone;
		PROXY_FILLBUFFER_DONE proxyFillBufferDone;
		PROXY_EVENTHANDLER proxyEventHandler;

		OMX_U32 nNumOfLines[PROXY_MAXNUMOFPORTS];
	} PROXY_COMPONENT_PRIVATE;

/*===============================================================*/
/** PROXY_MARK_DATA        : A pointer to this structure is sent as mark data to
 *                           the remote core when a MarkBuffer command is made.
 *
 * @param hComponentActual : This is the actual handle of the component to be
 *                           marked. When marked buffers come from the remote
 *                           to the local core then remote handle of the mark
 *                           component is replaced by this in the header.
 *
 * @param pMarkdataActual : This is the mark data set by the client.
 */
/*===============================================================*/
	typedef struct PROXY_MARK_DATA
	{
		OMX_HANDLETYPE hComponentActual;
		OMX_PTR pMarkDataActual;
	} PROXY_MARK_DATA;
/*******************************************************************************
* Functions
*******************************************************************************/
	OMX_ERRORTYPE OMX_ProxyCommonInit(OMX_HANDLETYPE hComponent);
	OMX_ERRORTYPE PROXY_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct);
	OMX_ERRORTYPE PROXY_EmptyThisBuffer(OMX_HANDLETYPE hComponent,
		OMX_BUFFERHEADERTYPE * pBufferHdr);


#endif
