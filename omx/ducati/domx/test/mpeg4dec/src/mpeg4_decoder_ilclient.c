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
 *    mpeg4_decoder_ilclient.c
 *
 * \brief
 *  This file contains IL Client implementation specific to OMX MPEG4 Decoder
 *
 * \version 1.0
 *
 *****************************************************************************
 */

#include <MPEG4_Decoder_ILClient.h>

void main()
{
	MPEG4_Client *pAppData = TIMM_OSAL_NULL;
	OMX_HANDLETYPE pHandle;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pBufferIn = NULL;
	OMX_BUFFERHEADERTYPE *pBufferOut = NULL;
	OMX_CALLBACKTYPE AppCallbacks;
	TIMM_OSAL_ERRORTYPE tTIMMSemStatus;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents;
	OMX_U32 actualSize;
	OMX_U32 buff_remaining = 0;
	OMX_U32 nRead, i;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDefs;
	OMX_CONFIG_RECTTYPE tParamStruct;

	AppCallbacks.EventHandler = MPEG4DEC_EventHandler;
	AppCallbacks.EmptyBufferDone = MPEG4DEC_EmptyBufferDone;
	AppCallbacks.FillBufferDone = MPEG4DEC_FillBufferDone;

	nComponentVersion.s.nVersionMajor = OMX_MPEG4VD_COMP_VERSION_MAJOR;
	nComponentVersion.s.nVersionMinor = OMX_MPEG4VD_COMP_VERSION_MINOR;
	nComponentVersion.s.nRevision = OMX_MPEG4VD_COMP_VERSION_REVISION;
	nComponentVersion.s.nStep = OMX_MPEG4VD_COMP_VERSION_STEP;

	tParamStruct.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	tParamStruct.nVersion = nComponentVersion;
	tParamStruct.nPortIndex = 1;

	tTIMMSemStatus = TIMM_OSAL_EventCreate(&MPEG4vd_Test_Events);
	if (TIMM_OSAL_ERR_NONE != tTIMMSemStatus)
	{
		//      TIMM_OSAL_Error("Error in creating event!");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	tTIMMSemStatus = TIMM_OSAL_EventCreate(&MPEG4VD_CmdEvent);
	if (TIMM_OSAL_ERR_NONE != tTIMMSemStatus)
	{
		TIMM_OSAL_Debug("Error in creating event!\n");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	pAppData =
	    (MPEG4_Client *) TIMM_OSAL_Malloc(sizeof(MPEG4_Client),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!pAppData)
	{
		//  TIMM_OSAL_Error("Error allocating pAppData!");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	memset(pAppData, 0x00, sizeof(MPEG4_Client));

	eError = MPEG4DEC_AllocateResources(pAppData);	// Allocate memory for the structure fields present in the pAppData(MPEG4_Client)
	if (eError != OMX_ErrorNone)
	{
		//    TIMM_OSAL_Error("Error allocating resources in main!");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	TIMM_OSAL_Debug("\nInput the test case number to be executed : ");
	scanf("%d", &Test_case_number);

	switch (Test_case_number)
	{
	case 101:
		pAppData->fIn =
		    fopen("../patterns/input/foreman_qcif_15fps_64k.m4v",
		    "rb");
		//The following are the test cae parameters set for each test case.
		Width = 176;
		Height = 144;
		In_buf_size = 5108;	//In_buf_size is set equal to the input file size.
		break;
	case 103:
		pAppData->fIn =
		    fopen
		    ("../patterns/input/vipertrain_p1920x1080_30fps_420pl_100fr_nv12_20MBPS_IPP.m4v",
		    "rb");
		Width = 1920;
		Height = 1080;
		In_buf_size = 5406612;
		break;
	case 105:
		pAppData->fIn =
		    fopen
		    ("../patterns/input/vipertakeoff_p1280x720_24fps_420pl_296fr_RC1_QP30_FR24000_BitRate9000000_test.mpeg4",
		    "rb");
		Width = 1280;
		Height = 720;
		In_buf_size = 5098267;
		break;
	case 113:
		pAppData->fIn =
		    fopen("../patterns/input/CP_envivio.m4v", "rb");
		Width = 352;
		Height = 288;
		In_buf_size = 2000675;
		break;
	case 117:
		pAppData->fIn =
		    fopen
		    ("../patterns/input/V10837_D-Traffic_MP4_ASP_VOP_L5.m4v",
		    "rb");
		Width = 720;
		Height = 576;
		In_buf_size = 8139420;
		break;
	default:		//Presently 137, 139, 140, 141, 142, 143, 144
		pAppData->fIn =
		    fopen("../patterns/input/foreman_qcif_15fps_64k.m4v",
		    "rb");
		Width = 176;
		Height = 144;
		In_buf_size = 5108;
		break;
	}

	if (pAppData->fIn == NULL)
	{
		TIMM_OSAL_Debug("\nCouldnt open i/p file!!!\n");
		goto EXIT;
	}

	pAppData->fOut =
	    fopen("../patterns/output/MPEG4_test_output.yuv", "wb");

	if (pAppData->fOut == NULL)
	{
		TIMM_OSAL_Debug("\nCouldnt open o/p file!!!\n");
		goto EXIT;
	}


	TIMM_OSAL_Debug("\nExecuting test case : %d", Test_case_number);

	pAppData->eState = OMX_StateInvalid;
	pAppData->pCb = &AppCallbacks;


	eError = OMX_Init();

	/* Load the MPEG4Decoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4D", pAppData,
	    pAppData->pCb);
	if ((eError != OMX_ErrorNone) || (pHandle == NULL))
	{
		//TIMM_OSAL_Error("Error in Get Handle function : %s ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	tPortDefs.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	tPortDefs.nVersion = nComponentVersion;
	tPortDefs.bEnabled = OMX_TRUE;
	tPortDefs.bPopulated = OMX_FALSE;
	tPortDefs.eDir = OMX_DirOutput;
	tPortDefs.nPortIndex = 1;
	tPortDefs.nBufferCountActual = 4;
	tPortDefs.eDomain = OMX_PortDomainVideo;
	tPortDefs.bBuffersContiguous = OMX_TRUE;
	tPortDefs.nBufferAlignment = 16;
	tPortDefs.format.video.cMIMEType = NULL;
	tPortDefs.format.video.pNativeRender = NULL;
	tPortDefs.format.video.nFrameWidth = Width;
	tPortDefs.format.video.nFrameHeight = Height;
	tPortDefs.format.video.nStride = Width;
	tPortDefs.format.video.nSliceHeight = Height;
	tPortDefs.format.video.nBitrate = 0;
	tPortDefs.format.video.xFramerate = 0;
	tPortDefs.format.video.bFlagErrorConcealment = OMX_TRUE;
	tPortDefs.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	tPortDefs.format.video.eColorFormat =
	    OMX_COLOR_FormatYUV420PackedSemiPlanar;

	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition,
	    (OMX_PTR) & tPortDefs);
	if (eError != OMX_ErrorNone)
	{
		TIMM_OSAL_Debug("\nerror in SetParam\n");
		goto EXIT;
	}

	eError =
	    OMX_GetParameter(pHandle, OMX_TI_IndexParam2DBufferAllocDimension,
	    (OMX_PTR) & tParamStruct);
	if (eError != OMX_ErrorNone)
	{
		TIMM_OSAL_Debug("\nerror in GetParam\n");
		goto EXIT;
	}

	TIMM_OSAL_Debug
	    ("\ntParamStruct.nWidth = %d,tParamStruct.nHeight = %d\n",
	    tParamStruct.nWidth, tParamStruct.nHeight);

	Out_buf_size = tParamStruct.nWidth * tParamStruct.nHeight * 1.5;

	pAppData->pBuffer1D =
	    (OMX_PTR) TIMM_OSAL_Malloc(Out_buf_size, TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_EXT);
	if (!pAppData->pBuffer1D)
	{
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	pAppData->pHandle = pHandle;
	pAppData->pComponent = (OMX_COMPONENTTYPE *) pHandle;
	MPEG4DEC_SetParamPortDefinition(pAppData);

	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	if (eError != OMX_ErrorNone)
	{
		//TIMM_OSAL_Error("Error in SendCommand()-OMX_StateIdle State set : %s ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}
	TIMM_OSAL_Debug("\nCame back from send command without error\n");

	/* Allocate I/O Buffers */
	for (i = 0; i < NUM_OF_IN_BUFFERS; i++)
	{
		eError =
		    OMX_AllocateBuffer(pHandle, /*&pBufferIn */
		    &pAppData->pInBuff[i], pAppData->pInPortDef->nPortIndex,
		    pAppData, pAppData->pInPortDef->nBufferSize);
	}
	for (i = 0; i < NUM_OF_OUT_BUFFERS; i++)
	{
		eError =
		    OMX_AllocateBuffer(pHandle, /*&pBufferOut */
		    &pAppData->pOutBuff[i], pAppData->pOutPortDef->nPortIndex,
		    pAppData, pAppData->pOutPortDef->nBufferSize);
	}


	/* Wait for initialization to complete.. Wait for Idle stete of component  */
	eError = MPEG4DEC_WaitForState(pHandle, OMX_StateIdle);
	if (eError != OMX_ErrorNone)
	{
		//    TIMM_OSAL_Error("Error %s:    WaitForState has timed out ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting,
	    NULL);
	if (eError != OMX_ErrorNone)
	{
		//   TIMM_OSAL_Error("Error from SendCommand-Executing State set :%s ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	eError = MPEG4DEC_WaitForState(pHandle, OMX_StateExecuting);
	if (eError != OMX_ErrorNone)
	{
		//   TIMM_OSAL_Error("Error %s:    WaitForState has timed out ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	for (i = 0; i < NUM_OF_IN_BUFFERS; i++)
	{
		nRead = MPEG4DEC_FillData(pAppData, pAppData->pInBuff[i]);
		if (nRead <= 0)
		{
			break;
		}

		eError = OMX_EmptyThisBuffer(pHandle, pAppData->pInBuff[i]);
		if (eError != OMX_ErrorNone)
		{
			//    TIMM_OSAL_Error("Error from Empty this buffer : %s ", MPEG4_GetDecoderErrorString(eError));
			goto EXIT;
		}
	}

	for (i = 0; i < NUM_OF_OUT_BUFFERS; i++)
	{
		eError = OMX_FillThisBuffer(pHandle, pAppData->pOutBuff[i]);
		if (eError != OMX_ErrorNone)
		{
			//    TIMM_OSAL_Error("Error from Fill this buffer : %s ", MPEG4_GetDecoderErrorString(eError));
			goto EXIT;
		}
	}

	if (Test_case_number == 140 || Test_case_number == 141 ||
	    Test_case_number == 142 || Test_case_number == 143 ||
	    Test_case_number == 144)
	{
		eError =
		    OMX_SendCommand(pHandle, OMX_CommandStateSet,
		    OMX_StatePause, NULL);
		if (eError != OMX_ErrorNone)
		{
			goto EXIT;
		}

		eError = MPEG4DEC_WaitForState(pHandle, OMX_StatePause);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Debug("\n Test case 140 failed.\n");
			goto EXIT;
		} else if (eError == OMX_ErrorNone)
			TIMM_OSAL_Debug("\n Test case 140 successful.\n");
	}
	if (Test_case_number == 141 || Test_case_number == 143 ||
	    Test_case_number == 144)
	{
		eError =
		    OMX_SendCommand(pHandle, OMX_CommandStateSet,
		    OMX_StateExecuting, NULL);
		if (eError != OMX_ErrorNone)
		{
			goto EXIT;
		}

		eError = MPEG4DEC_WaitForState(pHandle, OMX_StateExecuting);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Debug("\n Test case 141 failed.\n");
			goto EXIT;
		} else if (eError == OMX_ErrorNone)
			TIMM_OSAL_Debug("\n Test case 141 successful.\n");
	}
	if (Test_case_number == 144)
	{
		eError =
		    OMX_SendCommand(pHandle, OMX_CommandStateSet,
		    OMX_StatePause, NULL);
		if (eError != OMX_ErrorNone)
		{
			goto EXIT;
		}

		eError = MPEG4DEC_WaitForState(pHandle, OMX_StatePause);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Debug("\n Test case 144 failed.\n");
			goto EXIT;
		} else if (eError == OMX_ErrorNone)
			TIMM_OSAL_Debug("\n Test case 144 successful.\n");
	}
	if (Test_case_number == 142 || Test_case_number == 143 ||
	    Test_case_number == 144)
	{
		eError =
		    OMX_SendCommand(pHandle, OMX_CommandStateSet,
		    OMX_StateIdle, NULL);
		if (eError != OMX_ErrorNone)
		{
			goto EXIT;
		}

		eError = MPEG4DEC_WaitForState(pHandle, OMX_StateIdle);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Debug("\n Test case %d failed.\n",
			    Test_case_number);
			goto EXIT;
		} else if (eError == OMX_ErrorNone)
			TIMM_OSAL_Debug("\n Test case %d successful.\n",
			    Test_case_number);
	}

	eError = OMX_GetState(pHandle, &pAppData->eState);

	TIMM_OSAL_Debug("\nReturned from Get State\n");

	while (pAppData->eState == OMX_StateExecuting)
	{
		uRequestedEvents =
		    (MPEG4_DEC_EMPTY_BUFFER_DONE | MPEG4_DEC_FILL_BUFFER_DONE
		    | MPEG4_DECODER_ERROR_EVENT |
		    MPEG4_DECODER_END_OF_STREAM);
		tTIMMSemStatus =
		    TIMM_OSAL_EventRetrieve(MPEG4vd_Test_Events,
		    uRequestedEvents, TIMM_OSAL_EVENT_OR_CONSUME,
		    &pRetrievedEvents, TIMM_OSAL_SUSPEND);
		if (TIMM_OSAL_ERR_NONE != tTIMMSemStatus)
		{
			//    TIMM_OSAL_Error("Error in creating event!");
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

		if (pRetrievedEvents & MPEG4_DECODER_END_OF_STREAM)
		{
			TIMM_OSAL_Debug("\nEOS recieved\n");
			if (Test_case_number == 137)
				TIMM_OSAL_Debug
				    ("\n Test case 137 successful\n");

			break;
		}

		if (pRetrievedEvents & MPEG4_DEC_EMPTY_BUFFER_DONE)
		{
			TIMM_OSAL_GetPipeReadyMessageCount(pAppData->
			    IpBuf_Pipe, &buff_remaining);
			TIMM_OSAL_Debug("\nbuff_remaining %d\n",
			    buff_remaining);
			while (buff_remaining && Eos_sent == 0)
			{
				/*read from the pipe */
				TIMM_OSAL_Debug
				    ("\nBefore reading from pipe\n");
				TIMM_OSAL_ReadFromPipe(pAppData->IpBuf_Pipe,
				    &pBufferIn, sizeof(pBufferIn),
				    &actualSize, TIMM_OSAL_NO_SUSPEND);
				TIMM_OSAL_Debug("\ni/p buffer dequeued\n");
				if (pBufferIn == TIMM_OSAL_NULL)
				{
					break;
					//      TIMM_OSAL_Debug("\n Null received from pipe");
				} else
				{
					//      TIMM_OSAL_Debug("\n Header received from pipe = 0x%x", pBufferIn);
				}

				if (pBufferIn->nFilledLen > 3)
				{
					TIMM_OSAL_Debug
					    ("\nFilledLen from component is %d\n",
					    pBufferIn->nFilledLen);
					pBufferIn->nOffset =
					    (pBufferIn->nAllocLen -
					    pBufferIn->nFilledLen);
				} else
				{
					pBufferIn->nOffset =
					    (pBufferIn->nAllocLen -
					    pBufferIn->nFilledLen);
					pBufferIn->nFilledLen = 0;
					pBufferIn->nFlags |=
					    OMX_BUFFERFLAG_EOS;
					Eos_sent = 1;
					if (Test_case_number == 137)
						TIMM_OSAL_Debug
						    ("\n EOS given in an empty i/p frame\n");

				}

				eError =
				    OMX_EmptyThisBuffer(pHandle, pBufferIn);
				if (eError != OMX_ErrorNone)
				{
					//    TIMM_OSAL_Error("Error from Empty this buffer : %s ", MPEG4_GetDecoderErrorString(eError));
					goto EXIT;
				}
				TIMM_OSAL_GetPipeReadyMessageCount(pAppData->
				    IpBuf_Pipe, &buff_remaining);
			};
		}

		if (pRetrievedEvents & MPEG4_DEC_FILL_BUFFER_DONE)
		{
			do
			{
				/*read from the pipe */
				TIMM_OSAL_ReadFromPipe(pAppData->OpBuf_Pipe,
				    &pBufferOut, sizeof(pBufferOut),
				    &actualSize, TIMM_OSAL_NO_SUSPEND);
				eError =
				    OMX_FillThisBuffer(pHandle, pBufferOut);
				if (eError != OMX_ErrorNone)
				{
					//    TIMM_OSAL_Error("Error from Fill this buffer : %s ", MPEG4_GetDecoderErrorString(eError));
					goto EXIT;
				}
				TIMM_OSAL_Debug
				    ("\nSent another output buffer\n");
				TIMM_OSAL_GetPipeReadyMessageCount(pAppData->
				    OpBuf_Pipe, &buff_remaining);
			}
			while (buff_remaining);
		}

		if (pRetrievedEvents & MPEG4_DECODER_ERROR_EVENT)
		{
			eError = OMX_ErrorUndefined;
			break;
		}

		eError = OMX_GetState(pHandle, &pAppData->eState);
		TIMM_OSAL_Debug("\nReturned from Get State\n");
	}

      SKIPDECODE:

	if (Test_case_number != 142 && Test_case_number != 143 &&
	    Test_case_number != 144)
	{

		eError =
		    OMX_SendCommand(pHandle, OMX_CommandStateSet,
		    OMX_StateIdle, NULL);
		if (eError != OMX_ErrorNone)
		{
			//    TIMM_OSAL_Error("Error from SendCommand-Idle State set : %s ", MPEG4_GetDecoderErrorString(eError));
			goto EXIT;
		}

		eError = MPEG4DEC_WaitForState(pHandle, OMX_StateIdle);
		if (eError != OMX_ErrorNone)
		{
			//   TIMM_OSAL_Error("Error %s:    WaitForState has timed out ", MPEG4_GetDecoderErrorString(eError));
			goto EXIT;
		}
	}


	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	if (eError != OMX_ErrorNone)
	{
		//   TIMM_OSAL_Error("Error from SendCommand-Loaded State set : %s ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	/* Free I/O Buffers */
	for (i = 0; i < NUM_OF_IN_BUFFERS; i++)
	{
		eError =
		    OMX_FreeBuffer(pHandle, pAppData->pInPortDef->nPortIndex,
		    pAppData->pInBuff[i]);
	}
	for (i = 0; i < NUM_OF_OUT_BUFFERS; i++)
	{
		eError =
		    OMX_FreeBuffer(pHandle, pAppData->pOutPortDef->nPortIndex,
		    pAppData->pOutBuff[i]);
	}

	eError = MPEG4DEC_WaitForState(pHandle, OMX_StateLoaded);
	if (eError != OMX_ErrorNone)
	{
		//   TIMM_OSAL_Error("Error %s:    WaitForState has timed out ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	/* UnLoad the Decoder Component */
	eError = OMX_FreeHandle(pHandle);
	if ((eError != OMX_ErrorNone))
	{
		//    TIMM_OSAL_Error("Error in Free Handle function : %s ", MPEG4_GetDecoderErrorString(eError));
		goto EXIT;
	}

	eError = OMX_Deinit();

	TIMM_OSAL_Debug("\n Test case successful\n");

      EXIT:
	if (pAppData)
	{
		if (pAppData->fIn)
			fclose(pAppData->fIn);
	}

	MPEG4DEC_FreeResources(pAppData);
	TIMM_OSAL_Free(pAppData);


	tTIMMSemStatus = TIMM_OSAL_EventDelete(MPEG4vd_Test_Events);
	if (TIMM_OSAL_ERR_NONE != tTIMMSemStatus)
	{
		//           TBD
		//    TIMM_OSAL_Error("Error in deleting event!");
		//   eError = OMX_ErrorInsufficientResources;
		//   goto EXIT;
	}

	tTIMMSemStatus = TIMM_OSAL_EventDelete(MPEG4VD_CmdEvent);
	if (TIMM_OSAL_ERR_NONE != tTIMMSemStatus)
	{
		//              TBD
		//    TIMM_OSAL_Debug("Error in creating event!\n");
		//    eError = OMX_ErrorInsufficientResources;
		//    goto EXIT;
	}


}

OMX_ERRORTYPE MPEG4DEC_AllocateResources(MPEG4_Client * pAppData)
{
	OMX_U32 retval;
	OMX_ERRORTYPE eError = OMX_ErrorNone;


	pAppData->pInPortDef =
	    (OMX_PARAM_PORTDEFINITIONTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!pAppData->pInPortDef)
	{
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	pAppData->pOutPortDef =
	    (OMX_PARAM_PORTDEFINITIONTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!pAppData->pOutPortDef)
	{
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	/* Create a pipes for Input and Output Buffers.. used to queue data from the callback. */
	retval =
	    TIMM_OSAL_CreatePipe(&(pAppData->IpBuf_Pipe),
	    sizeof(OMX_BUFFERHEADERTYPE *) * NUM_OF_IN_BUFFERS,
	    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
	if (retval != 0)
	{
		//   TIMM_OSAL_Error("Error: TIMM_OSAL_CreatePipe failed to open");
		eError = OMX_ErrorContentPipeCreationFailed;
		goto EXIT;
	}

	retval =
	    TIMM_OSAL_CreatePipe(&(pAppData->OpBuf_Pipe),
	    sizeof(OMX_BUFFERHEADERTYPE *) * NUM_OF_OUT_BUFFERS,
	    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
	if (retval != 0)
	{
		//   TIMM_OSAL_Error("Error: TIMM_OSAL_CreatePipe failed to open");
		eError = OMX_ErrorContentPipeCreationFailed;
		goto EXIT;
	}



      EXIT:
	return eError;
}

void MPEG4DEC_FreeResources(MPEG4_Client * pAppData)
{
	//TIMM_OSAL_EnteringExt(nTraceGroup);

	if (pAppData->pInPortDef)
		TIMM_OSAL_Free(pAppData->pInPortDef);

	if (pAppData->pOutPortDef)
		TIMM_OSAL_Free(pAppData->pOutPortDef);

	if (pAppData->IpBuf_Pipe)
		TIMM_OSAL_DeletePipe(pAppData->IpBuf_Pipe);

	if (pAppData->OpBuf_Pipe)
		TIMM_OSAL_DeletePipe(pAppData->OpBuf_Pipe);

	if (pAppData->pBuffer1D)
		TIMM_OSAL_Free(pAppData->pBuffer1D);

	//TIMM_OSAL_Exiting(0);
	return;
}


OMX_ERRORTYPE MPEG4DEC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	TIMM_OSAL_ERRORTYPE retval;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		retval =
		    TIMM_OSAL_EventSet(MPEG4VD_CmdEvent,
		    MPEG4_STATETRANSITION_COMPLETE, TIMM_OSAL_EVENT_OR);
		if (retval != TIMM_OSAL_ERR_NONE)
		{
			//    TIMM_OSAL_Debug("\nError in setting the event!\n");
			eError = OMX_ErrorNotReady;
			return eError;
		}
		break;

	case OMX_EventError:
		retval =
		    TIMM_OSAL_EventSet(MPEG4VD_CmdEvent,
		    MPEG4_DECODER_ERROR_EVENT, TIMM_OSAL_EVENT_OR);
		if (retval != TIMM_OSAL_ERR_NONE)
		{
			TIMM_OSAL_Debug("\nError in setting the event!\n");
			eError = OMX_ErrorNotReady;
			return eError;
		}
		break;

	case OMX_EventMark:

		break;

	case OMX_EventPortSettingsChanged:

		break;

	case OMX_EventBufferFlag:
		if (nData2 == OMX_BUFFERFLAG_EOS)
		{
			retval =
			    TIMM_OSAL_EventSet(MPEG4vd_Test_Events,
			    MPEG4_DECODER_END_OF_STREAM, TIMM_OSAL_EVENT_OR);
			if (retval != TIMM_OSAL_ERR_NONE)
			{
				//      TIMM_OSAL_Error("Error in setting the event!");
				eError = OMX_ErrorNotReady;
				return eError;
			}
		}
		break;
	case OMX_EventResourcesAcquired:

		break;
	case OMX_EventComponentResumed:

		break;
	case OMX_EventDynamicResourcesAvailable:

		break;
	case OMX_EventPortFormatDetected:

		break;
	case OMX_EventMax:

		break;

	}			// end of switch

	//TIMM_OSAL_Exiting(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4DEC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	MPEG4_Client *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval;

	retval =
	    TIMM_OSAL_WriteToPipe(pAppData->IpBuf_Pipe, &pBuffHeader,
	    sizeof(pBuffHeader), TIMM_OSAL_SUSPEND);
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		//    TIMM_OSAL_Error("Error writing to Input buffer i/p Pipe!");
		eError = OMX_ErrorNotReady;
		return eError;
	}

	retval =
	    TIMM_OSAL_EventSet(MPEG4vd_Test_Events,
	    MPEG4_DEC_EMPTY_BUFFER_DONE, TIMM_OSAL_EVENT_OR);
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		//    TIMM_OSAL_Error("Error in setting the event!");
		eError = OMX_ErrorNotReady;
		return eError;
	}
	TIMM_OSAL_Debug("\nExiting Empty Buffer Done\n");
	//TIMM_OSAL_Exiting(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4DEC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	MPEG4_Client *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval;
	OMX_U32 nWrite;
	OMX_U32 nHeight2D;
	OMX_U32 nSize1D;
	OMX_U32 nWidth2D;
	OMX_U32 nStride2D = 4096;
	OMX_CONFIG_RECTTYPE tParamStruct;
	tParamStruct.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	tParamStruct.nVersion = nComponentVersion;
	tParamStruct.nPortIndex = 1;

	if (Test_case_number == 139 &&
	    pBuffHeader->nFlags & OMX_BUFFERFLAG_EOS)
		TIMM_OSAL_Debug
		    ("\n Last picture processed.\nTest case 139 successful\n");

	eError =
	    OMX_GetParameter(hComponent,
	    OMX_TI_IndexParam2DBufferAllocDimension,
	    (OMX_PTR) & tParamStruct);
	if (eError != OMX_ErrorNone)
	{
		TIMM_OSAL_Debug("\nerror in getparam\n");
	}

	nWidth2D = tParamStruct.nWidth;
	nHeight2D = tParamStruct.nHeight;
	nSize1D = nWidth2D * nHeight2D;

	//TIMM_OSAL_EnteringExt(nTraceGroup);

	if (pBuffHeader->nFilledLen > 0)
	{

		OMXMPEG4_Util_Memcpy_2Dto1D(pAppData->pBuffer1D,
		    pBuffHeader->pBuffer, nSize1D, nHeight2D, nWidth2D,
		    nStride2D);
		nWrite =
		    fwrite(pAppData->pBuffer1D, sizeof(OMX_U8), nSize1D * 1.5,
		    pAppData->fOut);
		if (nWrite != nSize1D * 1.5)
		{
			TIMM_OSAL_Debug("\nError in writing to file\n");
		}
	}

	pBuffHeader->nFilledLen = 0;

	retval = TIMM_OSAL_WriteToPipe(pAppData->OpBuf_Pipe, &pBuffHeader, sizeof(pBuffHeader), TIMM_OSAL_SUSPEND);	//timeout - TIMM_OSAL_SUSPEND ??
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		//   TIMM_OSAL_Error("Error writing to Output buffer Pipe!");
		eError = OMX_ErrorNotReady;
		return eError;
	}

	retval =
	    TIMM_OSAL_EventSet(MPEG4vd_Test_Events,
	    MPEG4_DEC_FILL_BUFFER_DONE, TIMM_OSAL_EVENT_OR);
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		//   TIMM_OSAL_Error("Error in setting the o/p event!");
		eError = OMX_ErrorNotReady;
		return eError;
	}

	//TIMM_OSAL_Exiting(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4DEC_WaitForState(OMX_HANDLETYPE * pHandle,
    OMX_STATETYPE DesiredState)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents;
	TIMM_OSAL_ERRORTYPE retval;

	/* Wait for an event */
	uRequestedEvents =
	    (MPEG4_STATETRANSITION_COMPLETE | MPEG4_DECODER_ERROR_EVENT);
	retval =
	    TIMM_OSAL_EventRetrieve(MPEG4VD_CmdEvent, uRequestedEvents,
	    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents, TIMM_OSAL_SUSPEND);

	if (TIMM_OSAL_ERR_NONE != retval)
	{
		TIMM_OSAL_Debug("\nError in EventRetrieve !\n");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	if (pRetrievedEvents & MPEG4_DECODER_ERROR_EVENT)
	{
		eError = OMX_ErrorUndefined;	//TODO: Needs to be changed
	}

      EXIT:
	return eError;
}

OMX_U32 MPEG4DEC_FillData(MPEG4_Client * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf)
{
	OMX_U32 nRead = 0;
	OMX_U32 framesizetoread = 0;

	if (!feof(pAppData->fIn))
	{
		fseek(pAppData->fIn, 0, SEEK_END);
		framesizetoread = In_buf_size;	//To be changed.
		fseek(pAppData->fIn, 0, SEEK_SET);
		nRead =
		    fread(pBuf->pBuffer, sizeof(OMX_U8), framesizetoread,
		    pAppData->fIn);

		pBuf->nFilledLen = nRead;
		pBuf->nAllocLen = nRead;
		pBuf->nOffset = 0;

	} else
	{
		nRead = 0;
		pBuf->pBuffer = NULL;
	}

	//TIMM_OSAL_Exiting(nRead);
	return nRead;
}

// May be implemented using Get Parameter
OMX_ERRORTYPE MPEG4DEC_SetParamPortDefinition(MPEG4_Client * pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	//OMX_PORT_PARAM_TYPE portInit;

	if (!pHandle)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	pAppData->pInPortDef->nPortIndex = 0x0;	// Need to put input port index macro from spurthi's interface file
	pAppData->pInPortDef->eDir = OMX_DirInput;
	pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
	pAppData->pInPortDef->nBufferCountMin = 1;

	fseek(pAppData->fIn, 0, SEEK_END);
	pAppData->pInPortDef->nBufferSize = In_buf_size;	// To be changed
	fseek(pAppData->fIn, 0, SEEK_SET);

	pAppData->pOutPortDef->nPortIndex = 0x1;
	pAppData->pOutPortDef->eDir = OMX_DirOutput;
	pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
	pAppData->pOutPortDef->nBufferCountMin = 1;


	pAppData->pOutPortDef->nBufferSize = Out_buf_size;

      EXIT:
	return eError;
}

static OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_2Dto1D(OMX_PTR pDst1D,
    OMX_PTR pSrc2D, OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D,
    OMX_U32 nStride2D)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_U8 *pInBuffer;
	OMX_U8 *pOutBuffer;
	OMX_U32 nSizeLeft, i;

	nSizeLeft = nSize1D;
	pInBuffer = (OMX_U8 *) pSrc2D;
	pOutBuffer = (OMX_U8 *) pDst1D;
	//The lower limit is copied. If nSize1D < H*W then 1Dsize is copied else H*W is copied
	TIMM_OSAL_Debug("\nStarting the 2D to 1D memcpy\n");
	for (i = 0; i < nHeight2D; i++)
	{
		//TIMM_OSAL_Debug("\nCopying row %d\n", i);
		if (nSizeLeft >= nWidth2D)
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nWidth2D);
			//GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),OMX_ErrorUndefined);
		} else
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer,
			    nSizeLeft);
			//GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),OMX_ErrorUndefined);
			break;
		}
		//TIMM_OSAL_Debug("\nCopied row %d, updating pointers\n", i);
		nSizeLeft -= nWidth2D;
		pInBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pInBuffer + nStride2D);
		pOutBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pOutBuffer + nWidth2D);
	}

	nSizeLeft = nSize1D / 2;
	//The lower limit is copied. If nSize1D < H*W then 1Dsize is copied else H*W is copied
	TIMM_OSAL_Debug("\nStarting the 2D to 1D memcpy\n");
	for (i = 0; i < nHeight2D / 2; i++)
	{
		//TIMM_OSAL_Debug("\nCopying row %d\n", i);
		if (nSizeLeft >= nWidth2D)
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nWidth2D);
			//GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),OMX_ErrorUndefined);
		} else
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer,
			    nSizeLeft);
			//GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),OMX_ErrorUndefined);
			break;
		}
		//TIMM_OSAL_Debug("\nCopied row %d, updating pointers\n", i);
		nSizeLeft -= nWidth2D;
		pInBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pInBuffer + nStride2D);
		pOutBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pOutBuffer + nWidth2D);
	}
	TIMM_OSAL_Debug("\nDone copying\n");

      EXIT:
	return eError;

}
