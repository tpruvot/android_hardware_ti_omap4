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
 *	H264_ILClient.h
 *
 * \brief
 *  This file contains methods that provides the functionality for the H264
 *  Video Decoder Test Application
 *
 * \version 1.0
 *
 *****************************************************************************
 */

#ifndef _H264_Display_ILClient_
#define _H264_Display_ILClient_

/****************************************************************
 * INCLUDE FILES
 ****************************************************************/

#include "stdio.h"
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IVCommon.h>
#include <timm_osal_error.h>
#include <timm_osal_trace.h>
#include <timm_osal_memory.h>

/** Number of input buffers in the H264 Decoder IL Client */
#define NUM_OF_IN_BUFFERS 1

/** Number of output buffers in the H264 Decoder IL Client */
#define NUM_OF_OUT_BUFFERS 20

/** Macro to initialize memset and initialize the OMX structure */
#define OMX_H264VD_TEST_INIT_STRUCT_PTR(_s_, _name_)       \
 memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision  = 0x0;       \
    (_s_)->nVersion.s.nStep   = 0x0;


/* ========================================================================== */
/** H264_Client is the structure definition for the H264 Decoder IL Client
*
* @param pHandle               OMX Handle
* @param pComponent            Component Data structure
* @param pCb                   Callback function pointer
* @param eState                Current OMX state
* @param pInPortDef            Structure holding input port definition
* @param pOutPortDef           Structure holding output port definition
* @param eCompressionFormat    Format of the input data
* @param pH264                 <TBD>
* @param pInBuff               Input Buffer pointer
* @param pOutBuff              Output Buffer pointer
* @param IpBuf_Pipe            Input Buffer Pipe
* @param OpBuf_Pipe            Output Buffer Pipe
* @param fIn                   File pointer of input file
* @param fInFrmSz              File pointer of Frame Size file (unused)
* @param fOut                  Output file pointer
* @param ColorFormat           Input color format
* @param nWidth                Width of the input vector
* @param nHeight               Height of the input vector
* @param nEncodedFrm           Total number of encoded frames
*/
/* ========================================================================== */
typedef struct H264_Client
{
	OMX_HANDLETYPE pHandle;
	OMX_COMPONENTTYPE *pComponent;
	OMX_CALLBACKTYPE *pCb;
	OMX_STATETYPE eState;
	OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef;
	OMX_PARAM_PORTDEFINITIONTYPE *pOutPortDef;
	OMX_U8 eCompressionFormat;
	OMX_VIDEO_PARAM_AVCTYPE *pH264;

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

} H264_Client;

/* ========================================================================== */
/** H264_TestCaseParamsType is the structure definition to define the input
* test vector properties
*
* @param SR_Id          SR ID that this testcase validates
* @param InFile         Input file location
* @param FrmSzFile      File name of Frm Size parameter (unused)
* @param OutFile        Output file location
* @param width          Width of the input stream
* @param height         Height of the input stream
* @param ColorFormat    Format of the input data
*/
/* ========================================================================== */
typedef struct H264_Decoder_TestCaseParamsType
{
	char SR_Id[10];
	char InFile[100];
	char OutFile[100];
	OMX_U32 width;
	OMX_U32 height;
	OMX_COLOR_FORMATTYPE ColorFormat;

} H264_Decoder_TestCaseParamsType;

#endif //_H264_ILClient_
