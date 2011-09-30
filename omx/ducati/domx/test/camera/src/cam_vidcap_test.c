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
* @file cam_vidcap_test.c
*
* This File contains test case for video capture.
*
* @path  domx/test/camera/src
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
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)

/* camera test case include files */
#include "camera_test.h"

#define debug	3
#define dprintf(level, fmt, arg...) do {			\
	if (debug >= level) {				\
		printf("DAEMON: " fmt , ## arg); } } while (0)


#define FRAMES_TO_PREVIEW	150
/*some global variables */
OMX_ERRORTYPE eError = OMX_ErrorNone;
OMX_HANDLETYPE hComp;
OMX_CALLBACKTYPE oCallbacks;
SampleCompTestCtxt *pContext;
SampleCompTestCtxt oAppData;

OMX_U32 test_case_id;
static TIMM_OSAL_PTR CameraEvents;
static TIMM_OSAL_PTR myEventIn;

/* camera events */
#define EVENT_CAMERA_FBD 1
#define EVENT_CAMERA_DQ 2

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
/* @ fn getomxformat: Get OMX Pixel format based on       */
/* the format string                                      */
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
		dprintf(0, "Some error occured in even handler"
		    "nData1=%d, nData2=%d\n", nData1, nData2);
		break;

	case OMX_EventMark:
		break;

	case OMX_EventPortSettingsChanged:
		dprintf(0, "Received PortSettingChanged event from"
		    "camera on Port = %d\n", nData1);
		pContext->bEventPortSettingsChanged = OMX_TRUE;
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
	int buffer_index, buffer_to_q;
	unsigned int numRemainingIn = 0;
	FILE *pFile;
	OMX_U8 *pLine;
	OMX_U32 nLines, nLineSz, nLineInc;
	int ii;
	unsigned int actualSize = 0;
	pthread_t thread_id;
	int policy;
	int rc = 0;
	struct sched_param param;
	size_t retval;
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

	while (OMX_ErrorNone == err)
	{
		uRequestedEvents = EVENT_CAMERA_FBD;

		dprintf(0, " going to wait for event retreive in thread \n");
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

			buffer_index = (int)pBuffHeader->pAppPrivate;
			dprintf(2, "\n buffer_index = %d\n", buffer_index);
			pPortParam =
			    &(pContext->sPortParam[pBuffHeader->
				nOutputPortIndex]);
			dprintf(2, "buffer index = %d remaing messages=%d\n",
			    (int)pBuffHeader->pAppPrivate, numRemainingIn);

			pFile = pPortParam->pOutputFile;

			if (pBuffHeader->nOutputPortIndex ==
			    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
			{
				dprintf(3, "Preview port frame Done number ="
				    "%d\n", (int)pPortParam->nCapFrame);
				dprintf(0, "nOffset = %d, nLines = %d,"
				    "nStride = %d, nLinesize = %d, pFile = %x\n",
				    pBuffHeader->nOffset, pPortParam->nHeight,
				    pPortParam->nWidth, pPortParam->nStride,
				    pFile);
				pLine = pBuffHeader->pBuffer +
				    pBuffHeader->nOffset;
				nLines = pPortParam->nHeight +
				    pPortParam->nHeight / 2;
				nLineSz = pPortParam->nWidth;
				nLineInc = pPortParam->nStride;
				while (nLines--)
				{
					retval = fwrite(pLine, nLineSz, 1,
					    pFile);
					pLine += nLineInc;
				}
				if (OMX_COLOR_FormatYUV420SemiPlanar ==
				    pPortParam->eColorFormat)
				{
					nLines = pPortParam->nHeight / 2;
					while (nLines--)
					{
						fwrite(pLine, nLineSz, 1,
						    pFile);
						pLine += nLineInc;
					}
				}
				dprintf(0, "calling fillthisbuffer for %d\
						buffer\n", buffer_index);
				omx_fillthisbuffer(buffer_index,
				    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);
				dprintf(0, "fillthisbuffer returns\n");

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
	eError = 0;
	TIMM_OSAL_ERRORTYPE retval;

	buffer_index = (int)pBuffHeader->pAppPrivate;
	dprintf(2, " FillBufferDone Port = %d ",
	    (int)pBuffHeader->nOutputPortIndex);
	dprintf(2, "\n index of buffer = %d\n",
	    (int)pBuffHeader->pAppPrivate);
	dprintf(2, "Filled Length is  = %d ", (int)pBuffHeader->nFilledLen);
	pPortParam = &(pContext->sPortParam[pBuffHeader->nOutputPortIndex]);
	if (pBuffHeader->nOutputPortIndex !=
	    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
	{
		dprintf(0, "Error in FBD Port number is not as expected\n");
		eError = -1;
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	if (pPortParam->nCapFrame++ >= FRAMES_TO_PREVIEW)
	{
		TIMM_OSAL_SemaphoreRelease(pContext->hExitSem);
		return eError;
	}
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
		dprintf(0, "Error in FillBufferDone\n");
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
	OMX_U32 i;
	struct port_param *sPort;
	uint buffersize;
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;

	dprintf(1, "Number of buffers to be allocated =0%d \n",
	    (uint) pPortDef->nBufferCountActual);
	dprintf(1, "SampleTest_AllocateBuffer width = %d height = %d\n",
	    (int)pPortDef->format.video.nFrameWidth,
	    (int)pPortDef->format.video.nFrameHeight);

	/* struct to hold data particular to Port */
	sPort = &(pContext->sPortParam[pPortDef->nPortIndex]);
	if (pPortDef->format.video.eColorFormat == OMX_COLOR_FormatCbYCrY)
		buffersize =
		    4096 * (pPortDef->format.video.nFrameHeight + 32);
	else if (pPortDef->format.video.eColorFormat ==
	    OMX_COLOR_FormatYUV420SemiPlanar)
	{
		dprintf(2, "Allocating buffers for NV12 format \n");
		buffersize =
		    4096 * ((pPortDef->format.video.nFrameHeight + 32) +
		    (pPortDef->format.video.nFrameHeight >> 2) + 32);
	}
	MemReqDescTiler = (MemAllocBlock *)
	    TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!MemReqDescTiler)
		dprintf(0, "can't allocate TILER memory \n");
	for (i = 0; i < pPortDef->nBufferCountActual; i++)
	{
		memset((void *)MemReqDescTiler, 0,
		    (sizeof(MemAllocBlock) * 2));
		MemReqDescTiler[0].pixelFormat = PIXEL_FMT_8BIT;
		MemReqDescTiler[0].dim.area.width =
		    (pPortDef->format.video.nFrameWidth + 32);
		MemReqDescTiler[0].dim.area.height =
		    pPortDef->format.video.nFrameHeight + 32;
		MemReqDescTiler[0].stride = STRIDE_8BIT;
		MemReqDescTiler[1].pixelFormat = PIXEL_FMT_16BIT;
		MemReqDescTiler[1].dim.area.width =
		    pPortDef->format.video.nFrameWidth / 2 + 32;
		MemReqDescTiler[1].dim.area.height =
		    pPortDef->format.video.nFrameHeight / 2 + 32;
		MemReqDescTiler[1].stride = STRIDE_16BIT;
		/*call to tiler Alloc */
		dprintf(3, "Before tiler alloc \n");
		TilerAddr = MemMgr_Alloc(MemReqDescTiler, 2);
		dprintf(3, "Tiler buffer allocated is %x\n",
		    (unsigned int)TilerAddr);
		eError = OMX_UseBuffer(pContext->hComp, &pBufferHdr,
		    pPortDef->nPortIndex, 0, buffersize, TilerAddr);
		OMX_TEST_BAIL_IF_ERROR(eError);
		pBufferHdr->pAppPrivate = (OMX_PTR) i;
		pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
		pBufferHdr->nVersion.s.nVersionMajor = 1;
		pBufferHdr->nVersion.s.nVersionMinor = 1;
		pBufferHdr->nVersion.s.nRevision = 0;
		pBufferHdr->nVersion.s.nStep = 0;
		sPort->bufferheader[i] = pBufferHdr;
		sPort->nCapFrame = 0;
	}			/*end of for loop for buffer count times */
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
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	OMX_CONFIG_CAMOPERATINGMODETYPE tCamOpMode;
	OMX_CONFIG_SENSORSELECTTYPE tSenSelect;
	OMX_COLOR_FORMATTYPE omx_pixformat;
	OMX_HANDLETYPE pHandle;
	struct port_param *PrevPort;
	int retval;

	dprintf(1, "In SetFormat() function \n");
	/*get the corresponding omx pix format */
	omx_pixformat = getomxformat(image_fmt);
	if (-1 == omx_pixformat)
		dprintf(0, " pixel cmdformat not supported \n");
	dprintf(3, "OmxPixelFormat = %d\n", omx_pixformat);

	OMX_TEST_INIT_STRUCT_PTR(&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tCamOpMode,
	    OMX_CONFIG_CAMOPERATINGMODETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tSenSelect, OMX_CONFIG_SENSORSELECTTYPE);

	dprintf(0, "going to set the operating Mode SetFormat function \n");
	/* set the usecase */
	pHandle = pContext->hComp;
	tCamOpMode.eCamOperatingMode = OMX_CaptureVideo;
	eError = OMX_SetParameter(pHandle,
	    OMX_IndexCameraOperatingMode, &tCamOpMode);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(0, "operating Mode set correctly \n");

	PrevPort = &(pContext->sPortParam[pContext->nPrevPortIndex]);
	PrevPort->nWidth = width;
	PrevPort->nHeight = height;
	PrevPort->nStride = 4096;
	PrevPort->eColorFormat = omx_pixformat;

	OMX_TEST_INIT_STRUCT_PTR(&(PrevPort->tVNFMode),
	    OMX_PARAM_VIDEONOISEFILTERTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(PrevPort->tYUVRange),
	    OMX_PARAM_VIDEOYUVRANGETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(PrevPort->tVidStabParam),
	    OMX_CONFIG_BOOLEANTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(PrevPort->tVidStabConfig),
	    OMX_CONFIG_FRAMESTABTYPE);

	/* initialize the value of VNF VSTAB and YUVRANGE in the port
	   structure */
	PrevPort->tVNFMode.eMode = OMX_VideoNoiseFilterModeOn;
	PrevPort->tVNFMode.nPortIndex = pContext->nPrevPortIndex;
	PrevPort->tYUVRange.eYUVRange = OMX_ITURBT601;
	PrevPort->tYUVRange.nPortIndex = pContext->nPrevPortIndex;
	PrevPort->tVidStabParam.bEnabled = OMX_FALSE;
	PrevPort->tVidStabConfig.bStab = OMX_FALSE;
	PrevPort->tVidStabConfig.nPortIndex = pContext->nPrevPortIndex;

	/* Make the PortSettingChanged flag to False so that when it becomes
	   true we will have to look upon the Stride set by the component */
	pContext->bEventPortSettingsChanged = OMX_FALSE;

	dprintf(3, "SetFormat Calling GetParameter \n");
	portCheck.nPortIndex = pContext->nPrevPortIndex;
	dprintf(1, "Preview portindex = %d \n", (int)portCheck.nPortIndex);
	eError = OMX_GetParameter(pHandle,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);

	portCheck.format.video.nFrameWidth = width;
	portCheck.format.video.nFrameHeight = height;
	portCheck.format.video.eColorFormat = omx_pixformat;
	portCheck.format.video.nStride = PrevPort->nStride;
	portCheck.format.video.xFramerate = 30 << 16;

	/* fill some default buffer count as of now.  */
	portCheck.nBufferCountActual = DEFAULT_BUFF_CNT;
	PrevPort->nActualBuffer = DEFAULT_BUFF_CNT;
	eError = OMX_SetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* check if parameters are set correctly by calling GetParameter() */
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(1, "PRV Width = %ld\n", portCheck.format.video.nFrameWidth);
	dprintf(1, "PRV Height = %ld\n", portCheck.format.video.nFrameHeight);
	dprintf(1, "PRV IMG FMT = %x\n", portCheck.format.video.eColorFormat);
	dprintf(1, "PRV portCheck.nBufferSize =%ld\n", portCheck.nBufferSize);
	dprintf(1, " PRV portCheck.nBufferCountMin = %ld\n",
	    portCheck.nBufferCountMin);
	dprintf(1, "PRV portCheck.nBufferCountActual = %ld\n",
	    portCheck.nBufferCountActual);
	dprintf(1, "PRV portCheck.format.video.nStride =%ld\n",
	    portCheck.format.video.nStride);

	/* set the VNF , VSTAB and YUV RANGE to the port */
	eError = OMX_SetParameter(pHandle,
	    OMX_IndexParamVideoNoiseFilter, &(PrevPort->tVNFMode));
	OMX_TEST_BAIL_IF_ERROR(eError);
	eError = OMX_SetParameter(pHandle,
	    OMX_IndexParamVideoCaptureYUVRange, &(PrevPort->tYUVRange));
	OMX_TEST_BAIL_IF_ERROR(eError);
	eError = OMX_SetParameter(pHandle,
	    OMX_IndexParamFrameStabilisation, &(PrevPort->tVidStabParam));
	OMX_TEST_BAIL_IF_ERROR(eError);

	tSenSelect.eSensor = OMX_PrimarySensor;
	eError = OMX_SetParameter(pHandle,
	    OMX_TI_IndexConfigSensorSelect, &tSenSelect);
	OMX_TEST_BAIL_IF_ERROR(eError);

	eError = OMX_SetConfig(pHandle,
	    OMX_IndexConfigCommonFrameStabilisation,
	    &(PrevPort->tVidStabConfig));
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(0, "We should expect this to print,\
			going to wait to change the nStride\n");
	while (pContext->bEventPortSettingsChanged)
	{
		dprintf(0, " Inside the portsetting changed in\
					SetFormat function\n");
		pContext->bEventPortSettingsChanged = OMX_FALSE;
		eError = OMX_GetParameter(pHandle,
		    OMX_IndexParamPortDefinition, &portCheck);
		OMX_TEST_BAIL_IF_ERROR(eError);
		PrevPort->nStride = portCheck.format.video.nStride;
		dprintf(0, "nStride received from the component\
					is %d \n", PrevPort->nStride);
		eError = OMX_SetParameter(pHandle,
		    OMX_IndexParamPortDefinition, &portCheck);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

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
		dprintf(2, "\n Done with OMX_FreeHandle()\
					with error = 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	eError = OMX_Deinit();
	dprintf(2, "\n Done with OMX_Deinit() with error = 0x%x\n", eError);
	OMX_TEST_BAIL_IF_ERROR(eError);
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "ERROR from omx_comp_release() \n");
	return eError;
}


/*========================================================*/
/* @ fn test_camera_video_capture : function to test the  */
/* camera preview test case called from the main() and    */
/* passed arguments width, height and image format        */
/*========================================================*/
int test_camera_video_capture(int width, int height, char *image_fmt)
{
	int i;
	char outputfilename[100];

	pContext = &oAppData;
	memset(pContext, 0x0, sizeof(SampleCompTestCtxt));

	/* Initialize the callback handles */
	oCallbacks.EventHandler = SampleTest_EventHandler;
	oCallbacks.EmptyBufferDone = SampleTest_EmptyBufferDone;
	oCallbacks.FillBufferDone = SampleTest_FillBufferDone;

	/* video and pre port indexes */
	pContext->nVideoPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_VIDEO;
	pContext->nPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;

	/* build the output file name according the resolution */
	sprintf(outputfilename, "/mnt/mmc/camera_bin/out_%dx%d.%s",
	    width, height, image_fmt);
	dprintf(0, "output filename = %s\n", outputfilename);

	/*open the file for writing the captured frames */
	pContext->sPortParam[OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW].pOutputFile =
	    fopen(outputfilename, "w+");
	if (!pContext->
	    sPortParam[OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW].pOutputFile)
		dprintf(0, "ERROR in opening output file \n");

	/* Initialize the Semaphores 1. for events and other for
	 * FillBufferDone */
	TIMM_OSAL_SemaphoreCreate(&pContext->hStateSetEvent, 0);
	TIMM_OSAL_SemaphoreCreate(&pContext->hExitSem, 0);

	dprintf(3, "calling OMX_init() \n");
	/* Initialize the OMX component */
	eError = OMX_Init();
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "calling OMX_GetHandle() \n");
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

	dprintf(2, "preview port enabled successfully \n");

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
		dprintf(0, "ERROR in creating event\n");
		eError = OMX_ErrorInsufficientResources;
	}
	eError = TIMM_OSAL_CreateTask((void *)&pContext->processFbd,
	    (void *)Camera_processfbd, 0, pContext, (10 * 1024), -1,
	    (signed char *)"CAMERA_FBD_TASK");

	dprintf(3, "\n Calling the FillThisBuffer for 0%d times\n",
	    DEFAULT_BUFF_CNT);
	for (i = 0; i < DEFAULT_BUFF_CNT; i++)
		omx_fillthisbuffer(i, OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);

	TIMM_OSAL_SemaphoreObtain(pContext->hExitSem, TIMM_OSAL_SUSPEND);
	omx_switch_to_loaded();
	dprintf(2, "\n Calling platform deinit()\n");


      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "\n ERROR test_camera_video_capture() function");
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

	while (!((test_case_id > 0) && (test_case_id <= 12)))
	{
		dprintf(0, "Select test case ID (1 - 12): video_capture "
		    "for NV12 format \n");
		fflush(stdout);
		dprintf(0, "Enter the Option for VideoCapture now: ");
		scanf("%d", &test_case_id);
	}
	dprintf(1, "Test Case ID = 0%d\n", test_case_id);

	switch (test_case_id)
	{
	case 1:
	{
		dprintf(0, "Going to test resolution "
		    "176x144 format NV12\n");
		eError = test_camera_video_capture(176, 144, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
						test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 2:
	{
		dprintf(0, "Going to test resolution "
		    "320x240 format NV12\n");
		eError = test_camera_video_capture(320, 240, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 3:
	{
		dprintf(0, "Going to test resolution "
		    "640x480 format NV12\n");
		eError = test_camera_video_capture(640, 480, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 4:
	{
		dprintf(0, "Going to test resolution 768x576"
		    "format NV12\n");
		eError = test_camera_video_capture(768, 576, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 5:
	{
		dprintf(0, "Going to test resolution "
		    "800x600 format NV12\n");
		eError = test_camera_video_capture(800, 600, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 6:
	{
		dprintf(0, "Going to test resolution "
		    "1024x768 format NV12\n");
		eError = test_camera_video_capture(1024, 768, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 7:
	{
		dprintf(0, "Going to test resolution "
		    "128x96 format NV12\n");
		eError = test_camera_video_capture(128, 96, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 8:
	{
		dprintf(0, "Going to test resolution "
		    "1600x1200 format NV12\n");
		eError = test_camera_video_capture(1600, 1200, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
						test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 9:
	{
		dprintf(0, "Going to test resolution "
		    "1280x1024 formatNV12\n");
		eError = test_camera_video_capture(1280, 1024, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 10:
	{
		dprintf(0, "Going to test resolution " "64x64 format NV12\n");
		eError = test_camera_video_capture(64, 64, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 11:
	{
		dprintf(0, "Going to test resolution "
		    "1920x1080 format YUVY\n");
		eError = test_camera_video_capture(1920, 1080, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
							test= 0x%x\n", eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 12:
	{
		dprintf(0, "Going to test resolution "
		    "1280x720 format YUVY\n");
		eError = test_camera_video_capture(1280, 720, "NV12");
		if (!eError)
			dprintf(0, "eError From Video Capture\
						test= 0x%x\n", eError);
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
