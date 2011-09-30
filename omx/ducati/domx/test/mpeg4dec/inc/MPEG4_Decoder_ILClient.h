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

/*!
 *****************************************************************************
 * \file
 *   MPEG4_Decoder_ILClient.h
 *
 * \brief
 *  This file contains IL Client implementation specific to OMX MPEG4 Decoder
 *
 * \version 1.0
 *
 *****************************************************************************
 */
#include "stdio.h"
#include <string.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IVCommon.h>
#include <OMX_TI_Index.h>
#include <OMX_Index.h>
#include <timm_osal_error.h>
#include <timm_osal_memory.h>
#include <timm_osal_trace.h>
#include <timm_osal_events.h>
#include <timm_osal_pipes.h>
#include <timm_osal_semaphores.h>
#include <timm_osal_error.h>
#include <timm_osal_task.h>

static TIMM_OSAL_PTR MPEG4vd_Test_Events;
static TIMM_OSAL_PTR MPEG4VD_CmdEvent;

/** Event definition to indicate input buffer consumed */
#define MPEG4_DEC_EMPTY_BUFFER_DONE 1

/** Event definition to indicate output buffer consumed */
#define MPEG4_DEC_FILL_BUFFER_DONE   2

/** Event definition to indicate error in processing */
#define MPEG4_DECODER_ERROR_EVENT 4

/** Event definition to indicate End of stream */
#define MPEG4_DECODER_END_OF_STREAM 8

#define MPEG4_STATETRANSITION_COMPLETE 16

static int size_read = 0;


/** Number of input buffers in the H264 Decoder IL Client */
#define NUM_OF_IN_BUFFERS 1

/** Number of output buffers in the H264 Decoder IL Client */
#define NUM_OF_OUT_BUFFERS 4

#define OMX_MPEG4VD_COMP_VERSION_MAJOR 1
#define OMX_MPEG4VD_COMP_VERSION_MINOR 1
#define OMX_MPEG4VD_COMP_VERSION_REVISION 0
#define OMX_MPEG4VD_COMP_VERSION_STEP 0

OMX_U32 Out_buf_size;
OMX_U32 In_buf_size;
OMX_U32 Height;
OMX_U32 Width;

OMX_U32 Test_case_number;

typedef struct MPEG4_Client
{
	OMX_HANDLETYPE pHandle;
	OMX_COMPONENTTYPE *pComponent;
	OMX_CALLBACKTYPE *pCb;
	OMX_STATETYPE eState;
	OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef;
	OMX_PARAM_PORTDEFINITIONTYPE *pOutPortDef;
	OMX_U8 eCompressionFormat;

	OMX_BUFFERHEADERTYPE *pInBuff[NUM_OF_IN_BUFFERS];
	OMX_BUFFERHEADERTYPE *pOutBuff[NUM_OF_OUT_BUFFERS];
	OMX_PTR IpBuf_Pipe;
	OMX_PTR OpBuf_Pipe;

	FILE *fIn;
	FILE *fInFrmSz;
	FILE *fOut;
	OMX_COLOR_FORMATTYPE ColorFormat;
	OMX_U32 nWidth;
	OMX_U32 nHeight;
	OMX_U32 nEncodedFrms;
	OMX_PTR pBuffer1D;

} MPEG4_Client;

OMX_VERSIONTYPE nComponentVersion;
OMX_U8 Eos_sent = 0;

static OMX_ERRORTYPE MPEG4DEC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData);
static OMX_ERRORTYPE MPEG4DEC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);
static OMX_ERRORTYPE MPEG4DEC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);
static OMX_ERRORTYPE MPEG4DEC_WaitForState(OMX_HANDLETYPE * pHandle,
    OMX_STATETYPE DesiredState);
static OMX_U32 MPEG4DEC_FillData(MPEG4_Client * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf);
static void MPEG4DEC_FreeResources(MPEG4_Client * pAppData);
static OMX_ERRORTYPE MPEG4DEC_AllocateResources(MPEG4_Client * pAppData);
static OMX_ERRORTYPE MPEG4DEC_SetParamPortDefinition(MPEG4_Client * pAppData);
static OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_2Dto1D(OMX_PTR pDst1D,
    OMX_PTR pSrc2D, OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D,
    OMX_U32 nStride2D);
