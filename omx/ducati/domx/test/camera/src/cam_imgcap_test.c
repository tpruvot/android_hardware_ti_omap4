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
* @file cam_imgcap_test.c
*
* This File contains testcases for the camera "Image Capture" Feature.
*
*
* @path domx/test/camera/src
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! =======================================================================
*!  Initial Version
* ========================================================================*/


/* standard linux includes */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>

/*omx related include files */
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <timm_osal_interfaces.h>
#include <timm_osal_trace.h>
#include <OMX_Image.h>
#include <OMX_TI_IVCommon.h>
#include <OMX_TI_Index.h>
#include <timm_osal_events.h>
#include <timm_osal_pipes.h>
#include <timm_osal_semaphores.h>
#include <timm_osal_error.h>
#include <timm_osal_task.h>

/* bridge related include files */
#include <RcmClient.h>

/*Tiler APIs*/
#include <memmgr.h>
#include <tiler.h>

/* camera test case include files */
#include "camera_test.h"
#include "dss_lib.h"

#define debug	0
#define dprintf(level, fmt, arg...) do {			\
	if (debug >= level) {				\
		printf("DAEMON: " fmt , ## arg); } } while (0)

#define FRAMES_TO_PREVIEW	20

OMX_ERRORTYPE eError = OMX_ErrorNone;
OMX_HANDLETYPE hComp;
OMX_CALLBACKTYPE oCallbacks;
SampleCompTestCtxt *pContext;
SampleCompTestCtxt oAppData;

OMX_U32 test_case_id;
int vid1_fd;
struct dss_buffers *buffers;
static TIMM_OSAL_PTR myEventIn;
#define EVENT_CAMERA_FBD 1
#define EVENT_CAMERA_DQ 2

#define	PREV_HEIGHT 480
#define PREV_WIDTH  640
#define PREV_FORMAT	OMX_COLOR_FormatCbYCrY

/*========================================================*/
/* @ fn OMX_TEST_StateToString :: STATE  to  STRING       */
/*========================================================*/
OMX_STRING OMX_TEST_StateToString(OMX_STATETYPE eState)
{
	OMX_STRING StateString;
	switch (eState)
	{
	case OMX_StateInvalid:
		StateString = "Invalid";
		break;
	case OMX_StateLoaded:
		StateString = "Loaded";
		break;
	case OMX_StateIdle:
		StateString = "Idle";
		break;
	case OMX_StateExecuting:
		StateString = "Executing";
		break;
	case OMX_StatePause:
		StateString = "Pause";
		break;
	case OMX_StateWaitForResources:
		StateString = "WaitForResources ";
		break;
	default:
		StateString = "<unknown>";
		break;
	}
	return StateString;
}


/*========================================================*/
/* @ fn OMXColorFormatGetBytesPerPixel : Get BytesPerPixel*/
/* value as per the Image Format                          */
/*========================================================*/
static OMX_U32 OMXColorFormatGetBytesPerPixel(OMX_COLOR_FORMATTYPE
    eColorFormat)
{
	OMX_U32 nBytesPerPix;

	switch (eColorFormat)
	{
	case OMX_COLOR_FormatYUV420SemiPlanar:
		nBytesPerPix = 1;
		break;
	case OMX_COLOR_FormatRawBayer10bit:
	case OMX_COLOR_Format16bitRGB565:
	case OMX_COLOR_FormatCbYCrY:
		nBytesPerPix = 2;
		break;
	case OMX_COLOR_Format32bitARGB8888:
		nBytesPerPix = 4;
		break;
	default:
		nBytesPerPix = 2;
	}
	return nBytesPerPix;
}

/*========================================================*/
/* @ fn getomxformat: Get OMX Pixel format based on the   */
/* format string                                          */
/*========================================================*/
OMX_COLOR_FORMATTYPE getomxformat(const char *format)
{
	dprintf(3, "format = %s\n", format);

	if (!strcmp(format, "YUYV"))
	{
		dprintf(3, " pixel format = %s\n", "YUYV");
		return OMX_COLOR_FormatYCbYCr;
	} else if (!strcmp(format, "UYVY"))
	{
		dprintf(3, " pixel format = %s\n", "UYVY");
		return OMX_COLOR_FormatCbYCrY;
	} else if (!strcmp(format, "NV12"))
	{
		dprintf(3, " pixel format = %s\n", "NV12");
		return OMX_COLOR_FormatYUV420SemiPlanar;
	} else
	{
		dprintf(0, "ERROR unsupported pixel format!\n");
		return -1;
	}
	return -1;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
	SampleCompTestCtxt *pContext;

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	dprintf(2, "in Event Handler event = %d\n", eEvent);
	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		if (OMX_CommandStateSet == nData1)
		{
			dprintf(2, " Component Transitioned to %s state \n",
			    OMX_TEST_StateToString((OMX_STATETYPE) nData2));
			pContext->eState = (OMX_STATETYPE) nData2;
			dprintf(1, "\n pContext->eState=%d\n",
			    pContext->eState);
			TIMM_OSAL_SemaphoreRelease(pContext->hStateSetEvent);
		} else if (OMX_CommandFlush == nData1)
		{
			/* Nothing to do over here */
		} else if (OMX_CommandPortDisable == nData1)
		{
			dprintf(1, "\nPort Disabled port number = %d \n",
			    nData2);
			/* Nothing to do over here */
		} else if (OMX_CommandPortEnable == nData1)
		{
			TIMM_OSAL_SemaphoreRelease(pContext->hStateSetEvent);
			dprintf(1, "\nPort Enable semaphore released \n");
			/* Nothing to do over here */
		} else if (OMX_CommandMarkBuffer == nData1)
		{
			/* Nothing to do over here */
		}
		break;

	case OMX_EventError:
		break;

	case OMX_EventMark:
		break;

	case OMX_EventPortSettingsChanged:
		break;

	case OMX_EventBufferFlag:
		break;

	case OMX_EventResourcesAcquired:
		break;

	case OMX_EventComponentResumed:
		break;

	case OMX_EventDynamicResourcesAvailable:
		break;

	case OMX_EventPortFormatDetected:
		break;

	default:
		break;
	}

	goto OMX_TEST_BAIL;

      OMX_TEST_BAIL:
	return OMX_ErrorNone;
}


/*========================================================*/
/*@ fn SampleTest_EmptyBufferDone :: Application callback */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	SampleCompTestCtxt *pContext;

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	dprintf(0, "Dummy Function. This should not be printed.\n");
	goto OMX_TEST_BAIL;

      OMX_TEST_BAIL:
	return OMX_ErrorNone;
}

/*========================================================*/
/* @ fn omx_fillthisbuffer :: OMX_FillThisBuffer call to  */
/* component                                              */
/*========================================================*/
static int omx_fillthisbuffer(int index, int PortNum)
{
	struct port_param *sPortParam;
	sPortParam = &(pContext->sPortParam[PortNum]);
	eError = OMX_FillThisBuffer(pContext->hComp,
	    sPortParam->bufferheader[index]);

	OMX_TEST_BAIL_IF_ERROR(eError);
	return eError;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR OMX_FillThisBuffer()\n");
	return eError;
}

/*========================================================*/
/* @ fn Camera_processfbd: FillBufferDone Event           */
/* Processing Loop                                        */
/*========================================================*/
void Camera_processfbd(void *threadsArg)
{
	OMX_BUFFERHEADERTYPE *pBuffHeader = NULL;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents;
	struct port_param *pPortParam;
	size_t byteswritten = 0;
	int buffer_index, buffer_to_q;
	unsigned int numRemainingIn = 0;
	char outputfilename[100];
	int ii;
	unsigned int actualSize = 0;
	pthread_t thread_id;
	int policy;
	int rc = 0;
	struct sched_param param;

	thread_id = pthread_self();
	dprintf(0, "New Thread Created\n");
	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		dprintf(0, "<Thread> 1 error %d\n", rc);
	printf("<Thread> %d %d\n", policy, param.sched_priority);
	param.sched_priority = 10;
	rc = pthread_setschedparam(thread_id, SCHED_RR /*policy */ , &param);

	if (rc != 0)
		dprintf(0, "<Thread> 2 error %d\n", rc);

	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		dprintf(0, "<Thread> 3 error %d\n", rc);
	dprintf(0, "<Thread> %d %d %d %d\n", policy, param.sched_priority,
	    sched_get_priority_min(policy), sched_get_priority_max(policy));

	pPortParam = &(pContext->sPortParam[OMX_CAMERA_PORT_IMAGE_OUT_IMAGE]);
	while (OMX_ErrorNone == err)
	{
		uRequestedEvents = EVENT_CAMERA_FBD;

		dprintf(3, " going to wait for event retreive in thread \n");
		err = TIMM_OSAL_EventRetrieve(myEventIn, uRequestedEvents,
		    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents,
		    TIMM_OSAL_SUSPEND);
		if (TIMM_OSAL_ERR_NONE != err)
		{
			dprintf(0, "error = %d pRetrievedEvents\n", err,
			    &pRetrievedEvents);
			TIMM_OSAL_Error("Error in Retrieving event!");
			err = OMX_ErrorUndefined;
		}

		/* Read the number of buffers available in pipe */
		TIMM_OSAL_GetPipeReadyMessageCount(pContext->FBD_pipe,
		    (void *)&numRemainingIn);
		while (numRemainingIn)
		{
			err = TIMM_OSAL_ReadFromPipe(pContext->FBD_pipe,
			    &pBuffHeader, sizeof(pBuffHeader), &actualSize,
			    TIMM_OSAL_SUSPEND);
			if (err != TIMM_OSAL_ERR_NONE)
			{
				printf("\n<Thread>Read from FBD_pipe\
				unsuccessful, going back to wait for event\n");
				break;
			}
			dprintf(2, "\n ReadFromPipe successfully returned\n");
			pPortParam =
			    &(pContext->sPortParam[pBuffHeader->
				nOutputPortIndex]);
			dprintf(1, "FillBufferDone for Port = %d\n",
			    (int)pBuffHeader->nOutputPortIndex);
			buffer_index = (int)pBuffHeader->pAppPrivate;
			dprintf(1, " buffer_index = %d\n", buffer_index);
			if (pBuffHeader->nOutputPortIndex ==
			    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
			{
				dprintf(2, "Filled Length is  = %d ",
				    (int)pBuffHeader->nFilledLen);

				dprintf(3, "Preview port frame Done number =\
					%d\n", (int)pPortParam->nCapFrame);
				pPortParam->nCapFrame++;
				dprintf(3, "\n Before SendbufferToDss() \n");
				buffer_to_q = SendbufferToDss(buffer_index,
				    vid1_fd);
				if (buffer_to_q != 0xFF)
				{
					omx_fillthisbuffer(buffer_to_q,
					    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);
				}
				if (pPortParam->nCapFrame ==
				    CAM_FRAME_TO_MAKE_STILL_CAPTURE)
				{
					OMX_CONFIG_BOOLEANTYPE bOMX;
					dprintf(1,
					    "enabling still capture \n");
					OMX_TEST_INIT_STRUCT_PTR(&bOMX,
					    OMX_CONFIG_BOOLEANTYPE);
					bOMX.bEnabled = OMX_TRUE;

					eError =
					    OMX_SetConfig(pContext->hComp,
					    OMX_IndexConfigCapturing, &bOMX);
					if (eError)
						dprintf(0,
						    "ERROR ERROR 0x%x\n",
						    eError);
					else
						dprintf(0, "enabled still\
							capture success\n");
				}
			}
			if (pBuffHeader->nOutputPortIndex ==
			    OMX_CAMERA_PORT_IMAGE_OUT_IMAGE)
			{
				OMX_CONFIG_BOOLEANTYPE bOMX;
				pPortParam->nCapFrame++;
				dprintf(3, "Captured %d frames\n",
				    pPortParam->nCapFrame);
				/* open the output file for writing the
				   frames */
				sprintf(outputfilename,
				    "/mnt/mmc/camera_bin/img_%dx%d_%d.jpg",
				    pPortParam->nWidth, pPortParam->nHeight,
				    pPortParam->nCapFrame);
				dprintf(3, "output filename = %s\n",
				    outputfilename);

				/*open the file for writing the captured image */
				pPortParam->pOutputFile =
				    fopen(outputfilename, "w+");
				if (!pPortParam->pOutputFile)
					dprintf(0,
					    "ERROR in opening the output"
					    " file \n");
				byteswritten =
				    fwrite((pBuffHeader->pBuffer) +
				    pBuffHeader->nOffset, 1,
				    pBuffHeader->nFilledLen,
				    pPortParam->pOutputFile);
				dprintf(1, "bytes written as image frame\
						= %d \n", byteswritten);
				/* need to close the file as each frame will be
				   written in a new file */
				fclose(pPortParam->pOutputFile);
				dprintf(1, "test case id = %d \n",
				    test_case_id);
				if (test_case_id == 17)
				{
					if (pPortParam->nCapFrame == 6)
					{
						dprintf(1, "Disable still"
						    " capture now \n");
						OMX_TEST_INIT_STRUCT_PTR
						    (&bOMX,
						    OMX_CONFIG_BOOLEANTYPE);
						bOMX.bEnabled = OMX_FALSE;

						eError =
						    OMX_SetConfig(pContext->
						    hComp,
						    OMX_IndexConfigCapturing,
						    &bOMX);
						if (eError)
							dprintf(0,
							    "There is an"
							    "ERROR in disabling"
							    "capture\n");
						else
							dprintf(1, "disabled"
							    "still capture"
							    "success\n");
					} else
						omx_fillthisbuffer(0,
						    OMX_CAMERA_PORT_IMAGE_OUT_IMAGE);
				} else
				{
					dprintf(1, "Disable still capture"
					    "now \n");
					OMX_TEST_INIT_STRUCT_PTR(&bOMX,
					    OMX_CONFIG_BOOLEANTYPE);
					bOMX.bEnabled = OMX_FALSE;

					eError =
					    OMX_SetConfig(pContext->hComp,
					    OMX_IndexConfigCapturing, &bOMX);
					if (eError)
						dprintf(0, "1 ERROR in"
						    "disabling capture\n");
					else
						dprintf(1, "disabled still"
						    "capture success\n");
				}

			}

			TIMM_OSAL_GetPipeReadyMessageCount(pContext->FBD_pipe,
			    (void *)&numRemainingIn);
			dprintf(2, " buffer index = %d remaing messages=%d\n",
			    (int)pBuffHeader->pAppPrivate, numRemainingIn);
		}
	}
	dprintf(0, " SHOULD NEVER COME HERE\n");
	return;
}


/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::   Application        */
/* callback                                               */
/*========================================================*/
OMX_ERRORTYPE SampleTest_FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	struct port_param *pPortParam;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int buffer_index;
	OMX_U8 exit_flag = 0;
	TIMM_OSAL_ERRORTYPE retval;
	eError = 0;

	buffer_index = (int)pBuffHeader->pAppPrivate;
	pPortParam = &(pContext->sPortParam[pBuffHeader->nOutputPortIndex]);

	if (pPortParam->nCapFrame >= FRAMES_TO_PREVIEW)
	{
		TIMM_OSAL_SemaphoreRelease(pContext->hExitSem);
		return eError;
	}
	dprintf(3, " Before writing to FBD Pipe \n");
	retval = TIMM_OSAL_WriteToPipe(pContext->FBD_pipe, &pBuffHeader,
	    sizeof(pBuffHeader), TIMM_OSAL_SUSPEND);
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		dprintf(0, "WriteToPipe FAILED\n");
		TIMM_OSAL_Error("ERROR in writing to pipe!");
		eError = OMX_ErrorNotReady;
		return eError;
	}
	eError = TIMM_OSAL_EventSet(myEventIn, EVENT_CAMERA_FBD,
	    TIMM_OSAL_EVENT_OR);
	if (eError != OMX_ErrorNone)
		TIMM_OSAL_Error("Error from fill Buffer done : ");
	dprintf(2, "Writing to pipe is successful\n");
	dprintf(2, "\n FBD Done \n");
	return OMX_ErrorNone;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "ERROR in FillBufferDone\n");
		return eError;
	}
	return OMX_ErrorNone;
}

/*========================================================*/
/* @ fn SampleTest_AllocateBuffers :   Allocates the      */
/* Resources on the available ports                       */
/*========================================================*/
OMX_ERRORTYPE SampleTest_AllocateBuffers(OMX_PARAM_PORTDEFINITIONTYPE *
    pPortDef)
{
	OMX_BUFFERHEADERTYPE *pBufferHdr;
	OMX_PARAM_PORTDEFINITIONTYPE pPortcap;
	OMX_U32 i;
	struct port_param *sPort;
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;

	dprintf(1, "Number of buffers to be allocated =0%d \n",
	    (uint) pPortDef->nBufferCountActual);
	dprintf(1, "SampleTest_AllocateBuffer width = %d height = %d\n",
	    (int)pPortDef->format.video.nFrameWidth,
	    (int)pPortDef->format.video.nFrameHeight);

	/* struct to hold data particular to Port */
	sPort = &(pContext->sPortParam[pPortDef->nPortIndex]);

	buffers = (struct dss_buffers *)calloc(pPortDef->nBufferCountActual,
	    sizeof(struct dss_buffers));
	getDSSBuffers(pPortDef->nBufferCountActual, buffers, vid1_fd);

	for (i = 0; i < pPortDef->nBufferCountActual; i++)
	{
		dprintf(0, "Just going to call UseBuffer \n");
		eError = OMX_UseBuffer(pContext->hComp, &pBufferHdr,
		    pPortDef->nPortIndex, 0, buffers[i].length,
		    (OMX_U8 *) buffers[i].start);
		OMX_TEST_BAIL_IF_ERROR(eError);
		dprintf(3, "\n Buffer Allignment Part \n");

		pBufferHdr->pAppPrivate = (OMX_PTR) i;
		pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
		pBufferHdr->nVersion.s.nVersionMajor = 1;
		pBufferHdr->nVersion.s.nVersionMinor = 1;
		pBufferHdr->nVersion.s.nRevision = 0;
		pBufferHdr->nVersion.s.nStep = 0;

		dprintf(0, "buffer address = %x \n",
		    (unsigned int)buffers[i].start);
		sPort->bufferheader[i] = pBufferHdr;
		sPort->nCapFrame = 0;

	}			/*end of for loop for buffer count times */
	/* now for capture port */

	MemReqDescTiler = (MemAllocBlock *)
	    TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!MemReqDescTiler)
		dprintf(0, "can't allocate memory for\
						Tiler block allocation \n");

	OMX_TEST_INIT_STRUCT(pPortcap, OMX_PARAM_PORTDEFINITIONTYPE);
	pPortcap.nPortIndex = pContext->nImgPortIndex;
	eError =
	    OMX_GetParameter(pContext->hComp, OMX_IndexParamPortDefinition,
	    (OMX_PTR) & pPortcap);
	OMX_TEST_BAIL_IF_ERROR(eError);
	/* struct to hold data particular to Port */
	sPort = &(pContext->sPortParam[pPortcap.nPortIndex]);
	dprintf(0, "Buffer count actual for image port = %d\n",
	    pPortcap.nBufferCountActual);
	for (i = 0; i < pPortcap.nBufferCountActual; ++i)
	{
		memset((void *)MemReqDescTiler, 0, sizeof(MemAllocBlock));
		MemReqDescTiler[0].dim.len = pPortcap.nBufferSize;
		MemReqDescTiler[0].pixelFormat = TILFMT_PAGE;
		MemReqDescTiler[0].stride = 0;
		TilerAddr = MemMgr_Alloc(MemReqDescTiler, 1);
		if (!TilerAddr)
			dprintf(0, "Failed to allocate Tiler Buffers for\
						Capture Port");
		dprintf(3, "Tiler buffer allocated at %x\n",
		    (unsigned int)TilerAddr);
		eError = OMX_UseBuffer(pContext->hComp, &pBufferHdr,
		    pPortcap.nPortIndex, 0, pPortcap.nBufferSize, TilerAddr);
		OMX_TEST_BAIL_IF_ERROR(eError);
		dprintf(3, "\n Buffer alignment for Capture Port\n");
		pBufferHdr->pAppPrivate = (OMX_PTR) i;
		pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
		pBufferHdr->nVersion.s.nVersionMajor = 1;
		pBufferHdr->nVersion.s.nVersionMinor = 1;
		pBufferHdr->nVersion.s.nRevision = 0;
		pBufferHdr->nVersion.s.nStep = 0;
		sPort->bufferheader[i] = pBufferHdr;
		sPort->nCapFrame = 0;
	}
	return OMX_ErrorNone;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "ERROR from SampleTest_AllocateBuffers()\
						eError = %x\n", eError);
		dprintf(3, " Returning from SampleTest_AllocateBuffers()\n");
		return eError;
	}
	return eError;
}

/*========================================================*/
/* @ fn SampleTest_DeInitBuffers :   Destroy the resources*/
/*========================================================*/
OMX_ERRORTYPE SampleTest_DeInitBuffers(SampleCompTestCtxt * pContext)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pBufferHdr;
	OMX_U32 j;

	for (j = 0; j < MAX_NO_BUFFERS; j++)
	{
		pBufferHdr =
		    pContext->sPortParam[OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW].
		    bufferheader[j];
		if (pBufferHdr)
		{
			dprintf(1, "deinit buffer header = %x \n",
			    (unsigned int)pBufferHdr);
			MemMgr_Free(pBufferHdr->pBuffer);
			eError = OMX_FreeBuffer(pContext->hComp,
			    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, pBufferHdr);
			OMX_TEST_BAIL_IF_ERROR(eError);
			pBufferHdr = NULL;
		}
		pBufferHdr =
		    pContext->sPortParam[OMX_CAMERA_PORT_IMAGE_OUT_IMAGE].
		    bufferheader[j];
		if (pBufferHdr)
		{
			dprintf(1, "Deinit buffer header for capture \
			port = %x \n", (unsigned int)pBufferHdr);
			MemMgr_Free(pBufferHdr->pBuffer);
			eError = OMX_FreeBuffer(pContext->hComp,
			    OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, pBufferHdr);
			OMX_TEST_BAIL_IF_ERROR(eError);
			pBufferHdr = NULL;
		}

	}
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "ERROR From SampleTest_DeInitBuffers() \n");
		return eError;
	}
	return eError;
}

/*========================================================*/
/* @ fn SampleTest_TransitionWait : Waits for the         */
/* transition to be completed ,incase of loaded to idle   */
/* allocates the Resources for enabled port and while idle*/
/* to loaded destroys the resources                       */
/*========================================================*/
OMX_ERRORTYPE SampleTest_TransitionWait(OMX_STATETYPE eToState)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDefPreview;

	/* send command to change the state from Loaded to Idle */
	dprintf(2, "\nTransitionWait send command to transition comp to %d\n",
	    eToState);
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandStateSet,
	    eToState, NULL);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* Command is sent to the Component but it will not change the state
	 * to Idle untill we call UseThisBuffer or AllocateBuffer for
	 * nBufferCountActual times.
	 */
	/* Verify that the component is still in Loaded state */
	eError = OMX_GetState(pContext->hComp, &pContext->eState);
	if (eError != OMX_ErrorNone)
		dprintf(0, "get state from sample test\
						transition wait failed\n");
	OMX_TEST_BAIL_IF_ERROR(eError);
	if ((eToState == OMX_StateIdle) &&
	    (pContext->eState == OMX_StateLoaded))
	{

		/* call GetParameter for preview port */
		OMX_TEST_INIT_STRUCT(tPortDefPreview,
		    OMX_PARAM_PORTDEFINITIONTYPE);
		tPortDefPreview.nPortIndex = pContext->nPrevPortIndex;
		eError = OMX_GetParameter(pContext->hComp,
		    OMX_IndexParamPortDefinition,
		    (OMX_PTR) & tPortDefPreview);
		OMX_TEST_BAIL_IF_ERROR(eError);
		/* now allocate the desired number of buffers for preview port */
		eError = SampleTest_AllocateBuffers(&tPortDefPreview);
		OMX_TEST_BAIL_IF_ERROR(eError);

	} else if ((eToState == OMX_StateLoaded) &&
	    (pContext->eState == OMX_StateIdle))
	{
		eError = SampleTest_DeInitBuffers(pContext);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	dprintf(3, "Obtaining Semaphore for state transition \n");
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	dprintf(3, "State Transition Semaphore got released \n");
	/* by this time the component should have come to Idle state */
	if (pContext->eState != eToState)
		OMX_TEST_SET_ERROR_BAIL(OMX_ErrorUndefined,
		    "InComplete Transition\n");

	dprintf(2, " Returning from SampleTest_TransitionWait \n");

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR From SampleTest_TransitionWait() \n");
	return eError;
}

/*========================================================*/
/* @ fn omx_switch_to_loaded : Idle -> Loaded Transition  */
/*========================================================*/
static int omx_switch_to_loaded()
{
	/* Transition back to Idle state  */
	eError = SampleTest_TransitionWait(OMX_StateIdle);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "\n Called Idle To Loaded \n");
	/* Transition back to Loaded state */
	eError = SampleTest_TransitionWait(OMX_StateLoaded);
	OMX_TEST_BAIL_IF_ERROR(eError);
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR from omx_switch_to_loaded()\
					= 0x%x \n", eError);
	return eError;
}

/*========================================================*/
/* @ fn SetFormat : Set the resolution width, height and  */
/* Format                                                 */
/*========================================================*/
static int SetFormat(int width, int height, const char *image_fmt)
{
	OMX_PARAM_PORTDEFINITIONTYPE portCheck;
	struct port_param *PrevPort, *ImgPort;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	OMX_CONFIG_CAMOPERATINGMODETYPE tCamOpMode;
	OMX_CONFIG_ROTATIONTYPE tCapRot;
	OMX_CONFIG_MIRRORTYPE tCapMir;
	OMX_PARAM_THUMBNAILTYPE tThumbnail;
	OMX_CONFIG_SENSORSELECTTYPE tSenSelect;
	int nStride;
	int retval;

	/* initialize the structures */
	OMX_TEST_INIT_STRUCT_PTR(&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tCamOpMode,
	    OMX_CONFIG_CAMOPERATINGMODETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tSenSelect, OMX_CONFIG_SENSORSELECTTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tCapRot, OMX_CONFIG_ROTATIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tCapMir, OMX_CONFIG_MIRRORTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tThumbnail, OMX_PARAM_THUMBNAILTYPE);

	/* set the preview port properties */
	dprintf(3, "\nSetFormat Calling GetParameter \n");
	portCheck.nPortIndex = pContext->nPrevPortIndex;
	dprintf(1, "\nPreview portindex = %d \n", (int)portCheck.nPortIndex);

	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(3, "GetParameter successful\n");

	PrevPort = &(pContext->sPortParam[pContext->nPrevPortIndex]);
	portCheck.format.video.nFrameWidth = PrevPort->nWidth;
	portCheck.format.video.nFrameHeight = PrevPort->nHeight;
	portCheck.format.video.eColorFormat = PrevPort->eColorFormat;
	portCheck.format.video.nStride = PrevPort->nStride;
	portCheck.format.video.xFramerate = 30 << 16;
	nStride = 4096;
	portCheck.nBufferSize = nStride * PrevPort->nHeight;

	/* fill some default buffer count as of now.  */
	portCheck.nBufferCountActual = PrevPort->nActualBuffer
	    = DEFAULT_BUFF_CNT;
	eError = OMX_SetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(3, "SetParameter successful for preview port\n");

	/* check if parameters are set correctly by calling GetParameter() */
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(1, "PRV Width = %ld\n", portCheck.format.video.nFrameWidth);
	dprintf(1, "PRV Height = %ld\n", portCheck.format.video.nFrameHeight);
	dprintf(1, "PRV IMG FMT = %x\n", portCheck.format.video.eColorFormat);
	dprintf(1, "PRV portCheck.nBufferSize =%ld\n", portCheck.nBufferSize);
	dprintf(1, "PRV portCheck.nBufferCountMin = %ld\n",
	    portCheck.nBufferCountMin);
	dprintf(1, "PRV portCheck.nBufferCountActual = %ld\n",
	    portCheck.nBufferCountActual);
	dprintf(1, " PRV portCheck.format.video.nStride = %ld\n",
	    portCheck.format.video.nStride);

	/* set the image port properties */
	dprintf(3, "\nSetFormat Calling GetParameter for image port\n");
	portCheck.nPortIndex = pContext->nImgPortIndex;
	dprintf(1, "\nPreview portindex = %d \n", (int)portCheck.nPortIndex);
	ImgPort = &(pContext->sPortParam[pContext->nImgPortIndex]);
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(3, "GetParameter successful for image port\n");

	if (test_case_id == 17)
	{
		/* Set HIGH SPEED MODE */
		tCamOpMode.eCamOperatingMode =
		    OMX_CaptureImageHighSpeedTemporalBracketing;
		dprintf(0, "\n Lets set the operating mode to be "
		    "OMX_CaptureImageHighSpeedTemporalBracketing\n");
		eError = OMX_SetParameter(pContext->hComp,
		    OMX_IndexCameraOperatingMode, &tCamOpMode);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}
	portCheck.format.image.eCompressionFormat =
	    ImgPort->eCapCompressionFormat;
	portCheck.format.video.nFrameWidth = ImgPort->nWidth;
	portCheck.format.video.nFrameHeight = ImgPort->nHeight;
	portCheck.format.video.eColorFormat = ImgPort->nStride;
	portCheck.format.video.nStride = ImgPort->eColorFormat;
	eError = OMX_SetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(3, "SetParameter successful for image port\n");
	/* set the capture image rotation, mirror and thumbnail properties */

	dprintf(3, "Setting up Capture rotation  \n");
	tCapRot.nPortIndex = pContext->nImgPortIndex;
	tCapRot.nRotation = ImgPort->nCaptureRot;
	eError = OMX_SetConfig(pContext->hComp,
	    OMX_IndexConfigCommonRotate, &tCapRot);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "Setting up Capture Mirror properties \n");
	tCapMir.nPortIndex = pContext->nImgPortIndex;
	tCapMir.eMirror = ImgPort->eCaptureMirror;
	eError = OMX_SetConfig(pContext->hComp,
	    OMX_IndexConfigCommonMirror, &tCapMir);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "Getting Thumbnail Properties \n");
	tThumbnail.nPortIndex = pContext->nImgPortIndex;
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamThumbnail, &tThumbnail);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "Setting Thumbnail Properties \n");
	tThumbnail.nHeight = ImgPort->nThumbHeight;
	tThumbnail.nWidth = ImgPort->nThumbWidth;
	eError = OMX_SetParameter(pContext->hComp,
	    OMX_IndexParamThumbnail, &tThumbnail);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* set the format for preview on display */
	dprintf(3, "Setting format for the Display \n");
	retval = SetFormatforDSSvid(PREV_WIDTH, PREV_HEIGHT, image_fmt,
	    vid1_fd);
	if (retval)
		dprintf(0, "ERROR from SetFormatforDSSvid()\n");

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR from SetFormat(), error = 0x%x\n", eError);
	return eError;
}

/*========================================================*/
/* @ fn omx_comp_release : Freeing and deinitialising the */
/* component                                              */
/*========================================================*/
static int omx_comp_release()
{
	/* Free the OMX handle and call Deinit */
	if (hComp)
	{
		dprintf(2, "Calling OMX_FreeHandle \n");
		eError = OMX_FreeHandle(hComp);
		dprintf(2, "\n Done with OMX_FreeHandle() with\
						error = 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	eError = OMX_Deinit();
	dprintf(2, "\n Done with OMX_Deinit() with error = 0x%x\n", eError);
	OMX_TEST_BAIL_IF_ERROR(eError);
	if (vid1_fd)
	{
		dprintf(2, "closing the video pipeline \n");
		close(vid1_fd);
		vid1_fd = 0;
	}
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR from omx_comp_release() \n");
	return eError;
}

/*========================================================*/
/* @ fn test_image_capture : function to test the image   */
/* capture. Called from main function with arguments      */
/* width, height and image format                         */
/*========================================================*/
int test_image_capture(int width, int height, char *image_fmt)
{
	int i;
	OMX_COLOR_FORMATTYPE omx_pixformat;
	struct port_param *PrevPort, *ImgPort;
	/*open the video 1 device to render the preview before capture */
	vid1_fd = open_video1();
	dprintf(0, "vid1_fd = %d\n", vid1_fd);

	pContext = &oAppData;
	memset(pContext, 0x0, sizeof(SampleCompTestCtxt));

	/* Initialize the callback handles */
	oCallbacks.EventHandler = SampleTest_EventHandler;
	oCallbacks.EmptyBufferDone = SampleTest_EmptyBufferDone;
	oCallbacks.FillBufferDone = SampleTest_FillBufferDone;

	/* image and preview port indexes */
	pContext->nImgPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;
	pContext->nPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;


	/* Initialize the Semaphores 1. for events and other for
	 * FillBufferDone */
	TIMM_OSAL_SemaphoreCreate(&pContext->hStateSetEvent, 0);
	TIMM_OSAL_SemaphoreCreate(&pContext->hExitSem, 0);

	dprintf(3, "Calling OMX_init() \n");
	/* Initialize the OMX component */
	eError = OMX_Init();
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "Calling OMX_GetHandle() \n");
	/* Get the handle of OMX camera component */

	eError = OMX_GetHandle(&hComp,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.CAMERA", pContext,
	    &oCallbacks);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "\n OMX_GetHandle() Done \n");
	/* store the handle in global structure */
	pContext->hComp = hComp;

	/* disable all ports and then enable preview and video out port
	 * enabling two ports are necessary right now for OMX Camera component
	 * to work. */
	dprintf(2, "Disabling all the ports \n");
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandPortDisable,
	    OMX_ALL, NULL);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* Enable PREVIEW PORT */
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandPortEnable,
	    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, NULL);
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(2, "Preview port enabled successfully \n");

	eError = OMX_SendCommand(pContext->hComp, OMX_CommandPortEnable,
	    OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, NULL);
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(2, "Image port enabled successfully \n");

	/*get the corresponding omx pix format */
	omx_pixformat = getomxformat(image_fmt);
	if (-1 == omx_pixformat)
	{
		dprintf(0, " pixel cmdformat not supported \n");
		if (vid1_fd)
			close(vid1_fd);
		return -1;
	}
	dprintf(3, "\n Omx PixelFormat = %d\n", omx_pixformat);


	/* preview format in case of capture is hardcoded to UYVY */
	/* Store the prev frame properties in struct */
	PrevPort = &(pContext->sPortParam[OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW]);
	PrevPort->nWidth = PREV_WIDTH;
	PrevPort->nHeight = PREV_HEIGHT;
	PrevPort->nStride = 4096;
	PrevPort->eColorFormat = PREV_FORMAT;

	/* store the properties of capture frame in struct */
	ImgPort = &(pContext->sPortParam[OMX_CAMERA_PORT_IMAGE_OUT_IMAGE]);
	ImgPort->nWidth = width;
	ImgPort->nHeight = height;
	ImgPort->nBytesPerPixel =
	    OMXColorFormatGetBytesPerPixel(omx_pixformat);
	ImgPort->nStride = ROUND_UP16(width) * ImgPort->nBytesPerPixel;
	ImgPort->eColorFormat = omx_pixformat;
	ImgPort->eCapCompressionFormat = OMX_IMAGE_CodingJPEG;
	ImgPort->nThumbWidth = 640;
	ImgPort->nThumbHeight = 480;
	ImgPort->nCaptureRot = 0;
	ImgPort->eCaptureMirror = OMX_MirrorNone;

	eError = SetFormat(width, height, (const char *)image_fmt);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* change state to idle. in between allocate the buffers and
	 * submit them to port
	 */
	dprintf(2, "Changing state from loaded to Idle \n");
	eError = SampleTest_TransitionWait(OMX_StateIdle);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(2, "Changing state from idle to executing\n");

	/* change state Executing */
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandStateSet,
	    OMX_StateExecuting, NULL);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/*wait till the transition get completed */
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	dprintf(2, "Idle To Executing Done \n");

	eError = TIMM_OSAL_CreatePipe(&(pContext->FBD_pipe),
	    sizeof(OMX_BUFFERHEADERTYPE *) * DEFAULT_BUFF_CNT,
	    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
	if (eError != 0)
	{
		dprintf(0, "ERROR TIMM_OSAL_CreatePipe failed to open\n");
		eError = OMX_ErrorContentPipeCreationFailed;
	}
	/* Create input data read thread */
	eError = TIMM_OSAL_EventCreate(&myEventIn);
	if (TIMM_OSAL_ERR_NONE != eError)
	{
		dprintf(0, "ERROR in creating event!\n");
		eError = OMX_ErrorInsufficientResources;
	}
	eError = TIMM_OSAL_CreateTask((void *)&pContext->processFbd,
	    (void *)Camera_processfbd, 0, pContext, (10 * 1024), -1,
	    (signed char *)"CAMERA_FBD_TASK");

	dprintf(3, "\n Calling the FillThisBuffer for 0%d times\n",
	    DEFAULT_BUFF_CNT);
	for (i = 0; i < DEFAULT_BUFF_CNT; i++)
		omx_fillthisbuffer(i, OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);

	for (i = 0; i < 1; i++)
		omx_fillthisbuffer(i, OMX_CAMERA_PORT_IMAGE_OUT_IMAGE);

	dprintf(1, "waiting for hExit Sem to be release now\n");
	TIMM_OSAL_SemaphoreObtain(pContext->hExitSem, TIMM_OSAL_SUSPEND);

	omx_switch_to_loaded();

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR from test_image_capture() function\n");
	dprintf(1, "release OMX component \n");
	omx_comp_release();
	return eError;
}

/*========================================================*/
/* @ fn main :                                            */
/*========================================================*/
int main()
{
	/* to load the images on the ducati side through CCS this call is
	 * essential
	 */

	while (!((test_case_id > 0) && (test_case_id <= 17)))
	{
		dprintf(0, "Select test case ID (1 - 17)"
		    "Image capture JPEG format \n");
		fflush(stdout);
		dprintf(0, "Enter the Option for image capture now: ");
		scanf("%d", &test_case_id);
	}
	dprintf(1, "Test Case ID = 0%d\n", test_case_id);

	switch (test_case_id)
	{
	case 1:
	{
		dprintf(0, "Resolution 176x144 format JPG\n");
		eError = test_image_capture(176, 144, "UYVY");
		if (!eError)
			dprintf(0, "Case 1 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 2:
	{
		dprintf(0, "Resolution 320x240 format JPG\n");
		eError = test_image_capture(320, 240, "UYVY");
		if (!eError)
			dprintf(0, "Case 2 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 3:
	{
		dprintf(0, "Resolution 768x576 format UYVY\n");
		eError = test_image_capture(768, 576, "UYVY");
		if (!eError)
			dprintf(0, "Case 3 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 4:
	{
		dprintf(0, "Resolution 800x600 format JPG\n");
		eError = test_image_capture(800, 600, "UYVY");
		if (!eError)
			dprintf(0, "Case 4 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 5:
	{
		dprintf(0, "Resolution 1024x768 format JPG\n");
		eError = test_image_capture(1024, 768, "UYVY");
		if (!eError)
			dprintf(0, "Case 5 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 6:
	{
		dprintf(0, "Resolution 128x128 format JPG\n");
		eError = test_image_capture(128, 128, "UYVY");
		if (!eError)
			dprintf(0, "Case 6 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 7:
	{
		dprintf(0, "Resolution 1600x1200 format JPG\n");
		eError = test_image_capture(1600, 1200, "UYVY");
		if (!eError)
			dprintf(0, "Case 7 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 8:
	{
		dprintf(0, "Resolution 1280x1024 format JPG\n");
		eError = test_image_capture(1280, 1024, "UYVY");
		if (!eError)
			dprintf(0, "Case 8 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 9:
	{
		dprintf(0, "Resolution 64x64 format JPG\n");
		eError = test_image_capture(64, 64, "UYVY");
		if (!eError)
			dprintf(0, "Case  9 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 10:
	{
		dprintf(0, "Resolution 1152x768 format JPG\n");
		eError = test_image_capture(1152, 768, "UYVY");
		if (!eError)
			dprintf(0, "Case 10 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 11:
	{
		dprintf(0, "Resolution 1920x1080 format JPG\n");
		eError = test_image_capture(1920, 1080, "UYVY");
		if (!eError)
			dprintf(0, "Case 11 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 12:
	{
		dprintf(0, "Resolution 1920x1020, format JPG\n");
		eError = test_image_capture(1920, 1020, "UYVY");
		if (!eError)
			dprintf(0, "Case 12 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 13:
	{
		dprintf(0, "Resolution 1280x720, format JPG\n");
		eError = test_image_capture(1280, 720, "UYVY");
		if (!eError)
			dprintf(0, "Case 13 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 14:
	{
		dprintf(0, "Resolution 4032x3024, format JPG\n");
		eError = test_image_capture(4032, 3024, "UYVY");
		if (!eError)
			dprintf(0, "Case 14 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 15:
	{
		dprintf(0, "\n Resolution 5376x2, format JPG\n");
		eError = test_image_capture(5376, 2, "UYVY");
		if (!eError)
			dprintf(0, "Case 15 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 16:
	{
		dprintf(0, "\n Resolution 641x481, format JPG\n");
		eError = test_image_capture(641, 481, "UYVY");
		if (!eError)
			dprintf(0, "Case 16 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 17:
	{
		dprintf(0, "\n High Speed Image Capture"
		    "Resolution 1600x1200, format JPG\n");
		eError = test_image_capture(1600, 1200, "UYVY");
		if (!eError)
			dprintf(0, "Case 17 eError= %d\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	};

	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "ERROR from main()");
		return eError;
	}
	return eError;
}
