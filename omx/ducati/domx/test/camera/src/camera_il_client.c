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
* @file camera_il_client.c
*
* This File contains testcases for the camera features from A9 Camera Proxy.
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
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

/* camera test case include files */
#include "camera_test.h"

#define VIDEO_DEVICE1 "/dev/video1"

#define OMX_TILERTEST

#define ROUND_UP32(x)   (((x) + 31) & ~31)
#define ROUND_UP16(x)   (((x) + 15) & ~15)

#define debug	0
#define dprintf(level,fmt, arg...) do {			\
	if ( debug >= level ){				\
	printf( "DAEMON: " fmt , ## arg);}} while (0)


#define E(STR) "OMX CAM TEST ERROR: %s:%s --> " STR, __FILE__, __FUNCTION__

#define OMXCAM_USE_BUFFER

#define CAM_EVENT_CMD_STATE_CHANGE      (1 << 0)
#define CAM_EVENT_MAIN_LOOP_EXIT        (1 << 1)

#define CAM_FRAME_TO_MAKE_STILL_CAPTURE     6
#define CAM_ZOOM_IN_START_ON_FRAME          (CAM_FRAME_TO_MAKE_STILL_CAPTURE + 1)	// Count from 0
#define CAM_ZOOM_IN_FRAMES                  2
#define CAM_ZOOM_IN_STEP                    0x10000

#define CAM_FRAMES_TO_CAPTURE           \
    (                                   \
          CAM_ZOOM_IN_START_ON_FRAME    \
        + CAM_ZOOM_IN_FRAMES            \
        + 1                             \
    )



#define GOTO_EXIT_IF(_CONDITION,_ERROR) {                                       \
    if ((_CONDITION)) {                                                         \
        TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXCAM, "Error :: %s : %s : %d :: ", __FILE__, __FUNCTION__, __LINE__);  \
        TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM, "Exiting because: %s \n", #_CONDITION);                          \
        eError = (_ERROR);                                                      \
        goto EXIT;                                                              \
    }                                                                           \
}

/*Tiler APIs*/
#include <memmgr.h>
#include <tiler.h>
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)

#define PROFILE 1
#ifdef PROFILE
typedef struct
{
	int gettimecount;
	int sec;
	int usec;
} keeptime;
keeptime timearray[100];
typedef struct
{
	int bufferindex;
	int sec;
	int usec;
	int fbdtime;
	int fillbuffertimestamp;
} trackframedone;
trackframedone framedoneprofile[20000];
int framedonecount = 0;
struct timeval tp;
#endif

int zoom_prv;
int TotalCapFrame = 0;
OMX_ERRORTYPE eError = OMX_ErrorNone;
OMX_HANDLETYPE hComp = NULL;
OMX_CALLBACKTYPE oCallbacks;
SampleCompTestCtxt *pContext;
SampleCompTestCtxt oAppData;

/* command line arguments */
unsigned int cmdwidth, cmdheight;
char *cmdformat;
OMX_COLOR_FORMATTYPE omx_pixformat;
int test_camera_preview(int width, int height, char *image_format);
int test_camera_capture(int width, int height, char *image_fmt);
int test_camera_zoom(void);
int vid1_fd = 0;
OMX_U32 v4l2pixformat;
FILE *OutputFile;
unsigned int cmd_width, cmd_height;
int video_capture = 0;
int dss_enable = 0;
static TIMM_OSAL_PTR myEventIn;
#define EVENT_CAMERA_FBD 1
#define EVENT_CAMERA_DQ 2

/* data declarations */
typedef struct
{
	OMX_HANDLETYPE pHandle;
	OMX_U32 nCapruredFrames;
	OMX_U32 nPreviewPortIndex;
	OMX_BUFFERHEADERTYPE **ppPreviewBuffers;
	OMX_U32 nPreviewBuffers;
	OMX_U32 nCapturePortIndex;
	OMX_BUFFERHEADERTYPE **ppCaptureBuffers;
	OMX_U32 nCaptureBuffers;
	FILE *pPrvOutputFile;
	FILE *pCapOutputFile;
	OMX_U32 nPrvWidth;
	OMX_U32 nPrvHeight;
	OMX_U32 nPrvStride;
	OMX_U32 nCapWidth;
	OMX_U32 nCapHeight;
	OMX_U32 nCapStride;
	OMX_U32 nThumbWidth;
	OMX_U32 nThumbHeight;
	OMX_COLOR_FORMATTYPE ePrvFormat;
	OMX_COLOR_FORMATTYPE eCapColorFormat;
	OMX_IMAGE_CODINGTYPE eCapCompressionFormat;
	OMX_U32 nPrvBytesPerPixel;
	OMX_U32 nCapBytesPerPixel;
	OMX_MIRRORTYPE eCaptureMirror;
	OMX_S32 nCaptureRot;
	OMX_SENSORSELECT eSenSelect;
} test_OMX_CAM_AppData_t;

struct
{
	void *start;
	size_t length;
} *buffers;
static TIMM_OSAL_PTR camera_frame_ready = NULL;
static TIMM_OSAL_PTR CameraEvents = NULL;
#define OMX_CAM_FRAME_READY   1

/* OMX functions*/
/*========================================================*/
/* @ fn OMX_TEST_ErrorToString :: ERROR  to  STRING   */
/*========================================================*/
OMX_STRING OMX_TEST_ErrorToString(OMX_ERRORTYPE eError)
{
	OMX_STRING errorString;

	switch (eError)
	{
	case OMX_ErrorNone:
		errorString = "ErrorNone";
		break;
	case OMX_ErrorInsufficientResources:
		errorString = "ErrorInsufficientResources";
		break;
	case OMX_ErrorUndefined:
		errorString = "ErrorUndefined";
		break;
	case OMX_ErrorInvalidComponentName:
		errorString = "ErrorInvalidComponentName";
		break;
	case OMX_ErrorComponentNotFound:
		errorString = "ErrorComponentNotFound";
		break;
	case OMX_ErrorInvalidComponent:
		errorString = "ErrorInvalidComponent";
		break;
	case OMX_ErrorBadParameter:
		errorString = "ErrorBadParameter";
		break;
	case OMX_ErrorNotImplemented:
		errorString = "ErrorNotImplemented";
		break;
	case OMX_ErrorUnderflow:
		errorString = "ErrorUnderflow";
		break;
	case OMX_ErrorOverflow:
		errorString = "ErrorOverflow";
		break;
	case OMX_ErrorHardware:
		errorString = "ErrorHardware";
		break;
	case OMX_ErrorInvalidState:
		errorString = "ErrorInvalidState";
		break;
	case OMX_ErrorStreamCorrupt:
		errorString = "ErrorStreamCorrupt";
		break;
	case OMX_ErrorPortsNotCompatible:
		errorString = "ErrorPortsNotCompatible";
		break;
	case OMX_ErrorResourcesLost:
		errorString = "ErrorResourcesLost";
		break;
	case OMX_ErrorNoMore:
		errorString = "ErrorNoMore";
		break;
	case OMX_ErrorVersionMismatch:
		errorString = "ErrorVersionMismatch";
		break;
	case OMX_ErrorNotReady:
		errorString = "ErrorNotReady";
		break;
	case OMX_ErrorTimeout:
		errorString = "ErrorTimeout";
		break;
	case OMX_ErrorSameState:
		errorString = "ErrorSameState";
		break;
	case OMX_ErrorResourcesPreempted:
		errorString = "ErrorResourcesPreempted";
		break;
	case OMX_ErrorPortUnresponsiveDuringAllocation:
		errorString = "ErrorPortUnresponsiveDuringAllocation";
		break;
	case OMX_ErrorPortUnresponsiveDuringDeallocation:
		errorString = "ErrorPortUnresponsiveDuringDeallocation";
		break;
	case OMX_ErrorPortUnresponsiveDuringStop:
		errorString = "ErrorPortUnresponsiveDuringStop";
		break;
	case OMX_ErrorIncorrectStateTransition:
		errorString = "ErrorIncorrectStateTransition";
		break;
	case OMX_ErrorIncorrectStateOperation:
		errorString = "ErrorIncorrectStateOperation";
		break;
	case OMX_ErrorUnsupportedSetting:
		errorString = "ErrorUnsupportedSetting";
		break;
	case OMX_ErrorUnsupportedIndex:
		errorString = "ErrorUnsupportedIndex";
		break;
	case OMX_ErrorBadPortIndex:
		errorString = "ErrorBadPortIndex";
		break;
	case OMX_ErrorPortUnpopulated:
		errorString = "ErrorPortUnpopulated";
		break;
	case OMX_ErrorComponentSuspended:
		errorString = "ErrorComponentSuspended";
		break;
	case OMX_ErrorDynamicResourcesUnavailable:
		errorString = "ErrorDynamicResourcesUnavailable";
		break;
	case OMX_ErrorMbErrorsInFrame:
		errorString = "ErrorMbErrorsInFrame";
		break;
	case OMX_ErrorFormatNotDetected:
		errorString = "ErrorFormatNotDetected";
		break;
	case OMX_ErrorContentPipeOpenFailed:
		errorString = "ErrorContentPipeOpenFailed";
		break;
	case OMX_ErrorContentPipeCreationFailed:
		errorString = "ErrorContentPipeCreationFailed";
		break;
	case OMX_ErrorSeperateTablesUsed:
		errorString = "ErrorSeperateTablesUsed";
		break;
	case OMX_ErrorTunnelingUnsupported:
		errorString = "ErrorTunnelingUnsupported";
		break;
	default:
		errorString = "<unknown>";
		break;
	}
	return errorString;
}

/*========================================================*/
/* @ fn OMX_TEST_StateToString :: STATE  to  STRING   */
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
/* @ fn getv4l2pixformat: Get V4L2 Pixel format based on
the format input from user */
/*========================================================*/
OMX_U32 getv4l2pixformat(const char *image_fmt)
{

	if (!strcmp(image_fmt, "YUYV"))
		return V4L2_PIX_FMT_YUYV;
	else if (!strcmp(image_fmt, "UYVY"))
		return V4L2_PIX_FMT_UYVY;
	else if (!strcmp(image_fmt, "RGB565"))
		return V4L2_PIX_FMT_RGB565;
	else if (!strcmp(image_fmt, "RGB565X"))
		return V4L2_PIX_FMT_RGB565X;
	else if (!strcmp(image_fmt, "RGB24"))
		return V4L2_PIX_FMT_RGB24;
	else if (!strcmp(image_fmt, "RGB32"))
		return V4L2_PIX_FMT_RGB32;
	else if (!strcmp(image_fmt, "NV12"))
		return V4L2_PIX_FMT_NV12;
	else
	{
		printf("fmt has to be NV12, UYVY, RGB565, RGB32, "
		    "RGB24, UYVY or RGB565X!\n");
		return -1;
	}
}

/*========================================================*/
/* @ fn getomxformat: Get OMX Pixel format based on
the format string */
/*========================================================*/
OMX_COLOR_FORMATTYPE getomxformat(const char *format)
{
	dprintf(3, "D format = %s\n", format);

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
		dprintf(3, " **** pixel format = %s\n", "NV12");
		return OMX_COLOR_FormatYUV420SemiPlanar;
	} else
	{
		dprintf(0, "unsupported pixel format!\n");
		return -1;
	}

	return -1;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
	SampleCompTestCtxt *pContext;

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	dprintf(2, "D in Event Handler event = %d\n", eEvent);
	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		if (OMX_CommandStateSet == nData1)
		{
			dprintf(2, " Component Transitioned to %s state \n",
			    OMX_TEST_StateToString((OMX_STATETYPE) nData2));
			pContext->eState = (OMX_STATETYPE) nData2;
			dprintf(1, "\n pContext->eState%d\n",
			    pContext->eState);
			TIMM_OSAL_SemaphoreRelease(pContext->hStateSetEvent);
		} else if (OMX_CommandFlush == nData1)
		{
			/* Nothing to do over here */
		} else if (OMX_CommandPortDisable == nData1)
		{
			dprintf(1, "\nD Port Disabled port number = %d \n",
			    nData2);
			/* Nothing to do over here */
		} else if (OMX_CommandPortEnable == nData1)
		{
			TIMM_OSAL_SemaphoreRelease(pContext->hStateSetEvent);
			dprintf(1, "\nD Port Enable semaphore released \n");
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
/* @ fn SampleTest_EmptyBufferDone :: Application callback    */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffer)
{
	SampleCompTestCtxt *pContext;

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	dprintf(3, "\nDummy Function\n");
	goto OMX_TEST_BAIL;

      OMX_TEST_BAIL:
	return OMX_ErrorNone;
}

/*========================================================*/
/* @ fn omx_fillthisbuffer :: OMX_FillThisBuffer call to component   */
/*========================================================*/
static int omx_fillthisbuffer(int index, int PortNum)
{
	struct port_param *sPortParam;
	sPortParam = &(pContext->sPortParam[PortNum]);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[61].sec = tp.tv_sec;
	timearray[61].usec = tp.tv_usec;
	framedoneprofile[framedonecount].fillbuffertimestamp =
	    tp.tv_sec * 1000000 + tp.tv_usec;
	sPortParam->bufferheader[index]->pMarkData =
	    framedoneprofile[framedonecount].fillbuffertimestamp;
#endif
	eError = OMX_FillThisBuffer(pContext->hComp,
	    sPortParam->bufferheader[index]);

	OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[62].sec = tp.tv_sec;
	timearray[62].usec = tp.tv_usec;
	dprintf(83, "D OMX_FillThisBuffer call takes = %d usec\n",
	    ((timearray[62].sec - timearray[61].sec) * 1000000 +
		timearray[62].usec - timearray[61].usec));
#endif
	return eError;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n D ERROR FROM SEND Q_BUF PARAMS\n");
	}
	return eError;
}

/*========================================================*/
/* @ fn SendbufferToDss :Sending buffers to DSS for displaying   */
/*========================================================*/
int SendbufferToDss(int index)
{
	struct v4l2_buffer filledbuffer;
	int result;
	static int time_to_streamon = 0;


#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[64].sec = tp.tv_sec;
	timearray[64].usec = tp.tv_usec;
#endif
	dprintf(2, "\n IN SendbufferToDss \n");
	filledbuffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	filledbuffer.memory = V4L2_MEMORY_MMAP;
	filledbuffer.flags = 0;
	filledbuffer.index = index;

	dprintf(2, "QQBUF buffer index %d\n", index);
	result = ioctl(vid1_fd, VIDIOC_QBUF, &filledbuffer);
	if (result != 0)
	{
		perror("VIDIOC_QBUF FAILED FAILED FAILED FAIL");
	}
	dprintf(2, "QQBUF comp\n");

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[65].sec = tp.tv_sec;
	timearray[65].usec = tp.tv_usec;
	dprintf(83, "D SendbufferToDss qbuf= %d usec\n",
	    ((timearray[65].sec - timearray[64].sec) * 1000000 +
		timearray[65].usec - timearray[64].usec));
#endif
	if (time_to_streamon == 0)
	{
		dprintf(3, "\n STREAM ON DONE !!! \n");
		result = ioctl(vid1_fd, VIDIOC_STREAMON, &filledbuffer.type);
		dprintf(3, "\n STREAMON result = %d\n", result);
		if (result != 0)
		{
			perror("VIDIOC_STREAMON FAILED FAILED FAILED FAILED");

		}
	}
	++time_to_streamon;
	filledbuffer.flags = 0;

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[66].sec = tp.tv_sec;
	timearray[66].usec = tp.tv_usec;
#endif
	if (time_to_streamon > 1)
	{
		dprintf(2, "\n DQBUF CALLED \n");
		result = ioctl(vid1_fd, VIDIOC_DQBUF, &filledbuffer);
		dprintf(2, "DQBUF buffer index = %d\n", filledbuffer.index);
		if (result != 0)
		{
			perror("VIDIOC_DQBUF FAILED FAILED FAILED FAILED");
		}
	} else
		filledbuffer.index = 0xFF;
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[67].sec = tp.tv_sec;
	timearray[67].usec = tp.tv_usec;
	dprintf(83, "D SendbufferToDss dbuf= %d usec\n",
	    ((timearray[67].sec - timearray[66].sec) * 1000000 +
		timearray[67].usec - timearray[66].usec));
#endif
#if 0
	if (time_to_streamon == 2)
	{
		dprintf(0, "setting up DQ event \n");
		eError =
		    TIMM_OSAL_EventSet(myEventIn, EVENT_CAMERA_DQ,
		    TIMM_OSAL_EVENT_OR);
		if (eError != OMX_ErrorNone)
		{
			TIMM_OSAL_Error("Error from fill Buffer done : ");
		}
	}
#endif
	return filledbuffer.index;
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::   Application callback  */
/*========================================================*/
OMX_ERRORTYPE SampleTest_FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int buffer_index;
	TIMM_OSAL_ERRORTYPE retval;
	int ii;

	buffer_index = (int)pBuffHeader->pAppPrivate;
	dprintf(2, "FBD buffer index %d\n", buffer_index);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	//framedoneprofile[framedonecount].fillbuffertimestamp = (int) pBuffHeader->nTimeStamp;
	framedonecount++;
	framedoneprofile[framedonecount].sec = tp.tv_sec;
	framedoneprofile[framedonecount].usec = tp.tv_usec;
	framedoneprofile[framedonecount].bufferindex = buffer_index;

#endif
	if (TotalCapFrame == 10000)
	{
		TIMM_OSAL_SemaphoreRelease(pContext->hExitSem);
		return eError;
	}
	TotalCapFrame++;
	dprintf(2, "\n D TotalCapFrame = %d writing to pipe", TotalCapFrame);
	retval =
	    TIMM_OSAL_WriteToPipe(pContext->FBD_pipe, &pBuffHeader,
	    sizeof(pBuffHeader), TIMM_OSAL_SUSPEND);
	if (retval != TIMM_OSAL_ERR_NONE)
	{
		dprintf(0, "\n D FAILED FAILED FAILED FAILED\n");
		TIMM_OSAL_Error("Error in writing to pipe!");
		eError = OMX_ErrorNotReady;
		return eError;
	}
	eError =
	    TIMM_OSAL_EventSet(myEventIn, EVENT_CAMERA_FBD,
	    TIMM_OSAL_EVENT_OR);
	if (eError != OMX_ErrorNone)
	{
		TIMM_OSAL_Error("Error from fill Buffer done : ");
	}


	dprintf(2, "D Write to pipe SUCCESSFUL\n");


#ifdef PROFILE
	gettimeofday(&tp, 0);
	framedoneprofile[framedonecount].fbdtime =
	    (tp.tv_sec - framedoneprofile[framedonecount].sec) * 1000000 +
	    tp.tv_usec - framedoneprofile[framedonecount].usec;

#endif
	return OMX_ErrorNone;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "D Error in FillBufferDone\n");
		return eError;
	}
	return OMX_ErrorNone;
}


/*========================================================*/
/* @ fn getDSSBuffers : Get Buffers allocated through DSS   */
/*========================================================*/
uint getDSSBuffers(uint count)
{
	int result;
	struct v4l2_requestbuffers reqbuf;
	int i;

	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = count;
	result = ioctl(vid1_fd, VIDIOC_REQBUFS, &reqbuf);
	if (result != 0)
	{
		perror("VIDEO_REQBUFS");
		return -1;
	}
	dprintf(2, "\nD DSS driver allocated %d buffers \n", reqbuf.count);
	if (reqbuf.count != count)
	{
		dprintf(0, "\nD DSS DRIVER FAILED TO ALLOCATE THE REQUESTED");
		dprintf(0,
		    " NUMBER OF BUFFERS. NOT FORCING STRICT ERROR CHECK");
		dprintf(0, " AND SO THE APPLIACTION MAY FAIL.\n");
	}
	buffers = calloc(reqbuf.count, sizeof(*buffers));

	for (i = 0; i < reqbuf.count; ++i)
	{
		struct v4l2_buffer buffer;
		buffer.type = reqbuf.type;
		buffer.index = i;

		result = ioctl(vid1_fd, VIDIOC_QUERYBUF, &buffer);
		if (result != 0)
		{
			perror("VIDIOC_QUERYBUF");
			return -1;
		}
		dprintf(2, "%d: buffer.length=%d, buffer.m.offset=%d\n",
		    i, buffer.length, buffer.m.offset);
		buffers[i].length = buffer.length;
		buffers[i].start = mmap(NULL, buffer.length, PROT_READ |
		    PROT_WRITE, MAP_SHARED, vid1_fd, buffer.m.offset);
		if (buffers[i].start == MAP_FAILED)
		{
			perror("mmap");
			return -1;
		}

		dprintf(1, "Buffers[%d].start = %x  length = %d\n", i,
		    buffers[i].start, buffers[i].length);
	}
	return 0;

}

/*========================================================*/
/* @ fn SampleTest_AllocateBuffers :   Allocates the Resources
on the available ports  */
/*========================================================*/
OMX_ERRORTYPE SampleTest_AllocateBuffers(OMX_PARAM_PORTDEFINITIONTYPE *
    pPortDef, OMX_U8 * pTmpBuffer)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pBufferHdr;
	OMX_U32 i;
	uint buffersize;
	struct port_param *sPort;

	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;

	dprintf(1, "\nD number of buffers to aloocated =0x%x \n",
	    (uint) pPortDef->nBufferCountActual);
	dprintf(1, "D SampleTest_AllocateBuffer width = %d height = %d\n",
	    (int)pPortDef->format.video.nFrameWidth,
	    (int)pPortDef->format.video.nFrameHeight);

	/* struct to hold data particular to Port */
	sPort = &(pContext->sPortParam[pPortDef->nPortIndex]);

	if (dss_enable == 1)
	{
#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[53].sec = tp.tv_sec;
		timearray[53].usec = tp.tv_usec;
#endif
		getDSSBuffers(pPortDef->nBufferCountActual);
#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[54].sec = tp.tv_sec;
		timearray[54].usec = tp.tv_usec;
		dprintf(3, "\nD time to get buffers from DSS = %d usec\n",
		    ((timearray[54].sec - timearray[53].sec) * 1000000 +
			timearray[54].usec - timearray[53].usec));
#endif
	} else
	{
		/* Allocate the buffers now. Assume that this else part is for the video capture for now.
		 * take into account vnf as of now to be on. */
		if (pPortDef->format.video.eColorFormat ==
		    OMX_COLOR_FormatCbYCrY)
			buffersize =
			    4096 * (pPortDef->format.video.nFrameHeight + 32);
		else if (pPortDef->format.video.eColorFormat ==
		    OMX_COLOR_FormatYUV420SemiPlanar)
		{
			dprintf(2, "Allocating buffers for NV12 format \n");
			buffersize =
			    4096 * ((pPortDef->format.video.nFrameHeight +
				32) +
			    (pPortDef->format.video.nFrameHeight >> 2) + 32);
		}

		MemReqDescTiler =
		    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock)
			* 2), TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
		if (!MemReqDescTiler)
			dprintf(0,
			    "\nD can't allocate memory for Tiler block allocation \n");

	}
	/* Loop to allocate buffers */
	for (i = 0; i < pPortDef->nBufferCountActual; i++)
	{
		if (dss_enable == 0)
		{

			if (pPortDef->format.video.eColorFormat ==
			    OMX_COLOR_FormatCbYCrY)
			{
				memset((void *)MemReqDescTiler, 0,
				    (sizeof(MemAllocBlock) * 2));
				MemReqDescTiler[0].pixelFormat =
				    PIXEL_FMT_16BIT;
				MemReqDescTiler[0].dim.area.width = pPortDef->format.video.nFrameWidth * 2;	/*width */
				MemReqDescTiler[0].dim.area.height = pPortDef->format.video.nFrameHeight + 32;	/*height */
				MemReqDescTiler[0].stride = STRIDE_16BIT;

				//MemReqDescTiler.reserved
				/*call to tiler Alloc */
				dprintf(3,
				    "\nBefore tiler alloc for the Codec Internal buffer\n");
				TilerAddr = MemMgr_Alloc(MemReqDescTiler, 1);
#ifdef PROFILE
				gettimeofday(&tp, 0);
				timearray[54].sec = tp.tv_sec;
				timearray[54].usec = tp.tv_usec;
				dprintf(3,
				    "\nD time to get buffers from tiler = %d usec\n",
				    ((timearray[54].sec -
					    timearray[53].sec) * 1000000 +
					timearray[54].usec -
					timearray[53].usec));
#endif
				dprintf(1, "\nTiler buffer allocated is %x\n",
				    (unsigned int)TilerAddr);
				pTmpBuffer = (OMX_U8 *) TilerAddr;
			} else
			{	//Frame format is NV12
				memset((void *)MemReqDescTiler, 0,
				    (sizeof(MemAllocBlock) * 2));
				MemReqDescTiler[0].pixelFormat =
				    PIXEL_FMT_8BIT;
				MemReqDescTiler[0].dim.area.width = pPortDef->format.video.nFrameWidth;	/*width */
				MemReqDescTiler[0].dim.area.height = pPortDef->format.video.nFrameHeight + 32;	/*height */
				MemReqDescTiler[0].stride = STRIDE_8BIT;

				MemReqDescTiler[1].pixelFormat =
				    PIXEL_FMT_16BIT;
				MemReqDescTiler[1].dim.area.width = pPortDef->format.video.nFrameWidth / 2;	/*width */
				MemReqDescTiler[1].dim.area.height = pPortDef->format.video.nFrameHeight / 2 + 32;	/*height */
				MemReqDescTiler[1].stride = STRIDE_16BIT;

				/*Call to tiler Alloc */
				dprintf(3, "\nBefore tiler alloc \n");
				TilerAddr = MemMgr_Alloc(MemReqDescTiler, 2);
				dprintf(3, "\nTiler buffer allocated is %x\n",
				    (unsigned int)TilerAddr);
				pTmpBuffer = (OMX_U8 *) TilerAddr;
			}
			if (pTmpBuffer == TIMM_OSAL_NULL)
			{
				dprintf(0,
				    "OMX_ErrorInsufficientResources\n");
				return -1;
			}
			sPort->hostbufaddr[i] = (OMX_U32) pTmpBuffer;

		} else
		{
			pTmpBuffer = (OMX_U8 *) buffers[i].start;
			buffersize = buffers[i].length;
			sPort->hostbufaddr[i] = (OMX_U32) pTmpBuffer;
		}

		//dprintf(0,"D Just going to call UseBuffer \n");
#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[55].sec = tp.tv_sec;
		timearray[55].usec = tp.tv_usec;
#endif
		eError =
		    OMX_UseBuffer(pContext->hComp, &pBufferHdr,
		    pPortDef->nPortIndex, 0, buffersize, pTmpBuffer);
		OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[56].sec = tp.tv_sec;
		timearray[56].usec = tp.tv_usec;
		dprintf(0, "\nD time taken in OMX_UseBuffer = %d usec\n",
		    ((timearray[56].sec - timearray[55].sec) * 1000000 +
			timearray[56].usec - timearray[55].usec));
#endif

		dprintf(3, "\n ---- ALLIGNMENT PART ---- \n");

		//pBufferHdr->pAppPrivate = pContext;
		pBufferHdr->pAppPrivate = (OMX_PTR) i;
		pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
		pBufferHdr->nVersion.s.nVersionMajor = 1;
		pBufferHdr->nVersion.s.nVersionMinor = 1;
		pBufferHdr->nVersion.s.nRevision = 0;
		pBufferHdr->nVersion.s.nStep = 0;

		dprintf(1, "D buffer address = %x \n",
		    (unsigned int)sPort->hostbufaddr[i]);
		strcpy((void *)sPort->hostbufaddr[i], "CAMERA DRIVER! YAY");
		dprintf(1, "D the contents of the buffers are %s \n",
		    (char *)sPort->hostbufaddr[i]);
		sPort->bufferheader[i] = pBufferHdr;
		sPort->nCapFrame = 0;
	}			//end of for loop for buffer count times
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n D ERROR FROM SAMPLE_TEST_ALLOCATEBUFFERS\n");
		dprintf(3, "D retruning from allocate buffer \n");
		return eError;
	}
	return eError;
}

/*========================================================*/
/* @ fn SampleTest_DeInitBuffers :   Destroy the resources  */
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
		dprintf(1, "D deinit buffer header = %x \n",
		    (unsigned int)pBufferHdr);
		if (pBufferHdr)
		{
#ifdef PROFILE
			gettimeofday(&tp, 0);
			timearray[91].sec = tp.tv_sec;
			timearray[91].usec = tp.tv_usec;
#endif
			MemMgr_Free(pBufferHdr->pBuffer);
#ifdef PROFILE
			gettimeofday(&tp, 0);
			timearray[92].sec = tp.tv_sec;
			timearray[92].usec = tp.tv_usec;
			dprintf(0, "D MemMgr_Free takes = %d usec\n",
			    ((timearray[92].sec -
				    timearray[91].sec) * 1000000 +
				timearray[92].usec - timearray[91].usec));
#endif
#ifdef PROFILE
			gettimeofday(&tp, 0);
			timearray[93].sec = tp.tv_sec;
			timearray[93].usec = tp.tv_usec;
#endif
			eError =
			    OMX_FreeBuffer(pContext->hComp,
			    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, pBufferHdr);
			OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
			gettimeofday(&tp, 0);
			timearray[94].sec = tp.tv_sec;
			timearray[94].usec = tp.tv_usec;
			dprintf(0, "D OMX_FreeBuffer takes = %d usec\n",
			    ((timearray[94].sec -
				    timearray[93].sec) * 1000000 +
				timearray[94].usec - timearray[93].usec));
#endif
			pBufferHdr = NULL;
		}

	}
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n D ERROR FROM DEINIT BUFFERS CALL\n");
		return eError;
	}
	return eError;
}

/*========================================================*/
/* @ fn SampleTest_TransitionWait : Waits for the transition to be completed ,
 *  incase of loaded to idle Allocates the Resources and while idle to loaded
 *  destroys the resources */
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
		dprintf(0,
		    "\nD get state from sample test transition wait failed\n");
	OMX_TEST_BAIL_IF_ERROR(eError);
	if ((eToState == OMX_StateIdle) &&
	    (pContext->eState == OMX_StateLoaded))
	{
		OMX_U8 *pTmpBuffer;

		/* call GetParameter for preview port */
		OMX_TEST_INIT_STRUCT(tPortDefPreview,
		    OMX_PARAM_PORTDEFINITIONTYPE);
		tPortDefPreview.nPortIndex = pContext->nPrevPortIndex;
		eError = OMX_GetParameter(pContext->hComp,
		    OMX_IndexParamPortDefinition,
		    (OMX_PTR) & tPortDefPreview);
		OMX_TEST_BAIL_IF_ERROR(eError);
		/* now allocate the desired number of buffers for preview port */
		eError = SampleTest_AllocateBuffers(&tPortDefPreview,
		    pTmpBuffer);
		OMX_TEST_BAIL_IF_ERROR(eError);

	} else if ((eToState == OMX_StateLoaded) &&
	    (pContext->eState == OMX_StateIdle))
	{
		eError = SampleTest_DeInitBuffers(pContext);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	dprintf(3, "D ------- obtaining Semaphore \n");
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	dprintf(3, "D Semaphore got released \n");
	/* by this time the component should have come to Idle state */

	if (pContext->eState != eToState)
		OMX_TEST_SET_ERROR_BAIL(OMX_ErrorUndefined,
		    "InComplete Transition\n");

	dprintf(2, " D returning from SampleTest_TransitionWait \n");

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n D ERROR FROM SAMPLETEST_TRANSITIONWAIT\n");
	}
	return eError;
}

/*========================================================*/
/* @ fn omx_switch_to_loaded : Idle -> Loaded Transition */
/*========================================================*/
static int omx_switch_to_loaded()
{
	/* Transition back to Idle state  */
	eError = SampleTest_TransitionWait(OMX_StateIdle);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(3, "\n CALLED IDLE -> LOADED \n");
	/* Transition back to Loaded state */
	eError = SampleTest_TransitionWait(OMX_StateLoaded);
	OMX_TEST_BAIL_IF_ERROR(eError);
	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n D ERROR FROM SEND STREAMOFF PARAMS\n");
	}
	return eError;
}

/*========================================================*/
/* @ fn SetFormatforDSSvid : Set the resolution width and
height of DSS Driver */
/*========================================================*/
void SetFormatforDSSvid(unsigned int width, unsigned int height)
{
	struct v4l2_format format;
	int result;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	format.fmt.pix.pixelformat = v4l2pixformat;
	/* set format of the picture captured */
	result = ioctl(vid1_fd, VIDIOC_S_FMT, &format);
	if (result != 0)
	{
		perror("VIDIOC_S_FMT");
	}
	return;
}

/*========================================================*/
/* @ fn SetFormat : Set the resolution width, height and
Format */
/*========================================================*/
static int SetFormat(int width, int height, OMX_COLOR_FORMATTYPE pix_fmt)
{
	OMX_PARAM_PORTDEFINITIONTYPE portCheck;
	struct port_param *PrevPort;
	int nStride;

	OMX_TEST_INIT_STRUCT_PTR(&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
	dprintf(3, "\nD SetFormat Calling GetParameter \n");
	portCheck.nPortIndex = pContext->nPrevPortIndex;
	dprintf(1, "\nD Preview portindex = %d \n",
	    (int)portCheck.nPortIndex);

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[19].sec = tp.tv_sec;
	timearray[19].usec = tp.tv_usec;
#endif
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[20].sec = tp.tv_sec;
	timearray[20].usec = tp.tv_usec;
	dprintf(0, "\nD GetParameter takes = %d usec\n",
	    ((timearray[20].sec - timearray[19].sec) * 1000000 +
		timearray[20].usec - timearray[19].usec));
#endif

	portCheck.format.video.nFrameWidth = width;
	portCheck.format.video.nFrameHeight = height;
	portCheck.format.video.eColorFormat = pix_fmt;
	portCheck.format.video.nStride = 4096;
	portCheck.format.video.xFramerate = 30 << 16;
	nStride = 4096;
	/* FIXME the BPP is hardcoded here but it should not be less */
	if (video_capture == 1)
	{
		/* video capture is NV12 format. means the buffer size would be
		 * nStride * (height + height /2 + 64 )  64 for VNF on*/
		portCheck.nBufferSize = nStride * (height + height / 2 + 64);
		dprintf(0, "going to set nBufferSize to = %d \n",
		    portCheck.nBufferSize);
	} else
	{
		portCheck.nBufferSize = nStride * height;
	}

	PrevPort = &(pContext->sPortParam[pContext->nPrevPortIndex]);
	PrevPort->nWidth = width;
	PrevPort->nHeight = height;
	PrevPort->nStride = nStride;
	PrevPort->eColorFormat = pix_fmt;

	/* fill some default buffer count as of now.  */
	portCheck.nBufferCountActual = PrevPort->nActualBuffer =
	    DEFAULT_BUFF_CNT;
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[21].sec = tp.tv_sec;
	timearray[21].usec = tp.tv_usec;
#endif
	eError = OMX_SetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[22].sec = tp.tv_sec;
	timearray[22].usec = tp.tv_usec;
	dprintf(0, "\nD OMX_SetParameter takes = %d usec\n",
	    ((timearray[22].sec - timearray[21].sec) * 1000000 +
		timearray[22].usec - timearray[21].usec));
#endif

	/* check if parameters are set correctly by calling GetParameter() */
	eError = OMX_GetParameter(pContext->hComp,
	    OMX_IndexParamPortDefinition, &portCheck);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(1, "\n PRV Width = %d", portCheck.format.video.nFrameWidth);
	dprintf(1, "\n PRV Height = %d", portCheck.format.video.nFrameHeight);
	dprintf(1, "\n PRV IMG FMT = %d",
	    portCheck.format.video.eColorFormat);
	dprintf(1, "\n PRV portCheck.nBufferSize = %ld\n",
	    portCheck.nBufferSize);
	dprintf(1, "\n PRV portCheck.nBufferCountMin = %ld\n",
	    portCheck.nBufferCountMin);
	dprintf(1, "\n PRV portCheck.nBufferCountActual = %ld\n",
	    portCheck.nBufferCountActual);
	dprintf(1, "\n PRV portCheck.format.video.nStride = %ld\n",
	    portCheck.format.video.nStride);

	if (video_capture == 1)
	{

		/* Let us enable the VNF on for now and YUV Range and video
		   stabilization in default state */
		dprintf(3, "\n D BEFOR EENABLING NOISE FILTER MODE ON \n");
		PrevPort->tVNFMode.nSize =
		    sizeof(OMX_PARAM_VIDEONOISEFILTERTYPE);
		PrevPort->tVNFMode.nVersion.s.nVersionMajor = 1;
		PrevPort->tVNFMode.nVersion.s.nVersionMinor = 1;
		PrevPort->tVNFMode.nVersion.s.nRevision = 0;


		PrevPort->tVNFMode.nPortIndex =
		    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;
		PrevPort->tVNFMode.eMode = OMX_VideoNoiseFilterModeOn;
		eError =
		    OMX_SetParameter(pContext->hComp,
		    OMX_IndexParamVideoNoiseFilter, &(PrevPort->tVNFMode));

		dprintf(2,
		    "\n D SETPARAMETER FOR NOISE FILTER MODE RETURNED\n");
		OMX_TEST_BAIL_IF_ERROR(eError);
#if 0
		eError =
		    OMX_SetParameter(pHandle,
		    OMX_IndexParamVideoCaptureYUVRange,
		    &(pContext->tYUVRange));
		OMX_TEST_BAIL_IF_ERROR(eError);
		eError =
		    OMX_SetParameter(pHandle,
		    OMX_IndexParamFrameStabilisation,
		    &(pContext->tVidStabParam));
		OMX_TEST_BAIL_IF_ERROR(eError);

		eError =
		    OMX_SetConfig(pHandle,
		    OMX_IndexConfigCommonFrameStabilisation,
		    &(pContext->tVidStabConfig));
		OMX_TEST_BAIL_IF_ERROR(eError);

		dprintf(0, "\n D CALLING DSS Set Format \n");
#endif
	}

	if (dss_enable)
	{
		SetFormatforDSSvid(width, height);
		dprintf(3, "\n D RETURNed FROM DSS SET FORMAT\n");
	}
      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
		dprintf(0, "\n D ERROR FROM SEND S_FMT PARAMS\n");
	return eError;
}

/*========================================================*/
/* @ fn omx_comp_release : Freeing  and deinitialising the
component */
/*========================================================*/
static int omx_comp_release()
{
	/* Free the OMX handle and call Deinit */
	if (hComp)
	{
		dprintf(2, "D Calling OMX_FreeHandle \n");
		eError = OMX_FreeHandle(hComp);
		dprintf(2, "\n Done with FREEHANDLE with error = %d\n",
		    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
	}

	eError = OMX_Deinit();
	dprintf(2, "\n Done with DEINIT with error = %d\n", eError);
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
		dprintf(0, "\n D ERROR FROM PROCESS_CLOSE\n");
	return eError;

}

/*========================================================*/
/* @ fn usage : Defines correct usage of Test Application */
/*========================================================*/
static void usage(void)
{
	dprintf(0, "\nUsage:\n");
	dprintf(0, "./camera_proxy.out <width> <height> <format> \n");
	dprintf(0, "\twidth in pixels min=%d, max= %d\n", MIN_PREVIEW_WIDTH,
	    MAX_PREVIEW_WIDTH);
	dprintf(0, "\theight in pixels min = %d, max = %d",
	    MIN_PREVIEW_HEIGHT, MAX_PREVIEW_HEIGHT);
	dprintf(0, "\tformat supported= YUYV|UYVY|NV12\n");
}


/*========================================================*/
/* @ fn main :                                            */
/*========================================================*/
int main()
{
	OMX_U32 test_case_id = 0;

#ifdef PROFILE
	int i, s1, u1, average;
	struct timeval tp1;
      startagain:
	gettimeofday(&tp1, 0);
	s1 = tp1.tv_sec;
	u1 = tp1.tv_usec;

	for (i = 0; i < 100; ++i)
	{
		gettimeofday(&tp, 0);
		timearray[0].sec = tp.tv_sec;
		timearray[0].usec = tp.tv_usec;
	}
	timearray[0].gettimecount = 0;
	if (timearray[0].usec < u1)
		goto startagain;
	average = (timearray[0].usec - u1) / 100;
	dprintf(0, "\nD Doing Calibration. \n");
	dprintf(0, "\nD starttime= %d, endtime = %d, average = %d\n", u1,
	    timearray[0].usec, average);
	dprintf(0, "\nD Calibration done for profiling calls \n");

#endif

	/* to load the images on the ducati side through CCS this call is
	 * essential
	 */
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[1].sec = tp.tv_sec;
	timearray[1].usec = tp.tv_usec;
#endif
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[2].sec = tp.tv_sec;
	timearray[2].usec = tp.tv_usec;
	dprintf(0, "\nD mmplatform_init takes = %d usec\n",
	    ((timearray[2].sec - timearray[1].sec) * 1000000 +
		timearray[2].usec - timearray[1].usec));
#endif
	dprintf(0,
	    "\nWait until RCM Server is created on other side. Press any key after that\n");
	getchar();

	while (!(((test_case_id > 0) && (test_case_id < 23)) ||
		((test_case_id > 129) && (test_case_id < 144)) ||
		((test_case_id > 200) && (test_case_id < 214))))
	{
		dprintf(0, "\nSelect test case ID (1 - 11 and 41):\
					Preview UVYV format \n");
		dprintf(0, "\nSelect test case ID (12 - 22): \
					Preview NV12 format \n");
		dprintf(0, "\nSelect test case ID (130 - 143):\
					Capture jpeg format \n");
		dprintf(0, "\nSelect test case ID (201 - 213):\
					Video capture NV12 format \n");
		dprintf(0, "\n Enter Test Case ID to run:");
		fflush(stdout);
		scanf("%d", &test_case_id);
	}


	switch (test_case_id)
	{
	case 1:
	{
		dprintf(0,
		    "\n Going to test resolution 640x480 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(640, 480, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 2:
	{
		dprintf(0,
		    "\n Going to test resolution 864x480 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(864, 480, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 3:
	{
		dprintf(0,
		    "\n Going to test resolution 320x240 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(320, 240, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 4:
	{
		dprintf(0,
		    "\n Going to test resolution 800x600 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(800, 600, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 5:
	{
		dprintf(0,
		    "\n Going to test resolution 176x144 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(176, 144, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 6:
	{
		dprintf(0,
		    "\n Going to test resolution 768x576format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(768, 576, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 7:
	{
		dprintf(0,
		    "\n Going to test resolution 128x96 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(128, 96, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 8:
	{
		dprintf(0, "\n Going to test resolution 64x64 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(64, 64, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 9:
	{
		dprintf(0, "\n Going to test resolution 80x60 formatUYVY\n");
		video_capture = 0;
		eError = test_camera_preview(80, 60, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 10:
	{
		dprintf(0,
		    "\n Going to test resolution 864x486 format UYVY\n");
		video_capture = 0;
		eError = test_camera_preview(864, 486, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 11:
	{
		dprintf(0,
		    "\n Going to test resolution 720x480 format YUVY\n");
		video_capture = 0;
		eError = test_camera_preview(720, 480, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 12:
	{
		dprintf(0,
		    "\n Going to test resolution 864x480 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(864, 480, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 13:
	{
		dprintf(0,
		    "\n Going to test resolution 640x480 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(640, 480, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}


	case 14:
	{
		dprintf(0,
		    "\n Going to test resolution 320x240 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(320, 240, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 15:
	{
		dprintf(0,
		    "\n Going to test resolution 800x600 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(800, 600, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 16:
	{
		dprintf(0,
		    "\n Going to test resolution 176x144 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(176, 144, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 17:
	{
		dprintf(0,
		    "\n Going to test resolution 768x576format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(768, 576, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 18:
	{
		dprintf(0,
		    "\n Going to test resolution 128x96 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(128, 96, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 19:
	{
		dprintf(0, "\n Going to test resolution 64x64 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(64, 64, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 20:
	{
		dprintf(0, "\n Going to test resolution 80x60 formatNV12\n");
		video_capture = 0;
		eError = test_camera_preview(80, 60, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 21:
	{
		dprintf(0,
		    "\n Going to test resolution 864x486 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(864, 486, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 22:
	{
		dprintf(0,
		    "\n Going to test resolution 720x480 format NV12\n");
		video_capture = 0;
		eError = test_camera_preview(720, 480, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}


	case 41:
	{
		dprintf(0,
		    "\n Going to test resolution 864x480 format UVYV\n");
		video_capture = 0;
		eError = test_camera_preview(860, 480, "UYVY");
		if (!eError)
			dprintf(0, "\n eError From Preview test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 130:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case QCIF 176x144: \n");
		zoom_prv = 0;
		test_camera_capture(176, 144, "UYVY");
		break;
	}



	case 131:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: QVGA 320x240\n");
		zoom_prv = 0;
		test_camera_capture(320, 240, "UYVY");
		break;
	}

	case 132:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: PAL 768x576\n");
		zoom_prv = 0;
		test_camera_capture(768, 576, "UYVY");
		break;
	}

	case 133:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: SVGA 800x600\n");
		zoom_prv = 0;
		test_camera_capture(800, 600, "UYVY");
		break;
	}

	case 134:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: XGA 1024x768\n");
		zoom_prv = 0;
		test_camera_capture(1024, 768, "UYVY");
		break;
	}

	case 135:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: SQCIF 128x128\n");
		zoom_prv = 0;
		test_camera_capture(128, 128, "UYVY");
		break;
	}

	case 136:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: UXGA 1600x1200\n");
		zoom_prv = 0;
		test_camera_capture(1600, 1200, "UYVY");
		break;
	}

	case 137:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: XGA 1280x1024\n");
		zoom_prv = 0;
		test_camera_capture(1280, 1024, "UYVY");
		break;
	}

	case 138:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: 64x64\n");
		zoom_prv = 0;
		test_camera_capture(64, 64, "UYVY");
		break;
	}

	case 139:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: 1152x768\n");
		zoom_prv = 0;
		test_camera_capture(1152, 768, "UYVY");
		break;
	}

	case 140:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: 1920x1080\n");
		zoom_prv = 0;
		test_camera_capture(1920, 1080, "UYVY");
		break;
	}

	case 141:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: 1920x1020\n");
		zoom_prv = 0;
		test_camera_capture(1920, 1020, "UYVY");
		break;
	}

	case 142:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case: 1280x720\n");
		zoom_prv = 0;
		test_camera_capture(1280, 720, "UYVY");
		break;
	}

	case 143:
	{
		dprintf(0,
		    "\nD 2. Starting image capture test case 12 MP: 4032x3024\n");
		zoom_prv = 0;
		test_camera_capture(4032, 3024, "UYVY");
		break;
	}

	case 160:
	{
		dprintf(0, "\nD 3. Starting zoom in preview test case: \n");
		zoom_prv = 1;
		test_camera_capture(640, 480, "UYVY");
		break;
	}

	case 201:
	{
		dprintf(0,
		    "\n Going to test resolution 176 X 144 format UYVY for video\n");
		video_capture = 1;
		eError = test_camera_preview(176, 144, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (1) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}
	case 202:
	{
		dprintf(0,
		    "\n Going to test resolution 320 X 240 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(320, 240, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (2) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 203:
	{
		dprintf(0,
		    "\n Going to test resolution 640x480 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(640, 480, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (3) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}


	case 204:
	{
		dprintf(0,
		    "\n Going to test resolution 768 X 576 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(768, 576, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (4) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 205:
	{
		dprintf(0,
		    "\n Going to test resolution 800 X 600 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(800, 600, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (5) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 206:
	{
		dprintf(0,
		    "\n Going to test resolution 1024 X 768 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1024, 768, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (6) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 207:
	{
		dprintf(0,
		    "\n Going to test resolution 128 X 96 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(128, 96, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (7) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 208:
	{
		dprintf(0,
		    "\n Going to test resolution 1600 X 1200 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1600, 1200, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (8) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 209:
	{
		dprintf(0,
		    "\n Going to test resolution 1280 X 1024 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1280, 1024, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 210:
	{
		dprintf(0,
		    "\n Going to test resolution 64x64 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(64, 64, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (10) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 211:
	{
		dprintf(0,
		    "\n Going to test resolution 1080P format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1920, 1080, "NV12");
		if (!eError)
			dprintf(0,
			    "\n (11) eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 212:
	{
		dprintf(0,
		    "\n Going to test resolution 1920 X 1020 format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1920, 1020, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	case 213:
	{
		dprintf(0,
		    "\n Going to test resolution 720P format NV12 for video\n");
		video_capture = 1;
		eError = test_camera_preview(1280, 720, "NV12");
		if (!eError)
			dprintf(0, "\n eError From Video Capture test= %d\n",
			    eError);
		OMX_TEST_BAIL_IF_ERROR(eError);
		break;
	}

	default:
	{
		dprintf(0, " Invalid test ID selection.");
		return -1;
	}
	};

	return 0;

      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n ERROR from main()");
		return eError;
	}
	return eError;
}

/*========================================================*/
/* @ fn open_video1 : Open Video1 Device */
/*========================================================*/
void open_video1()
{
	int result;
	struct v4l2_capability capability;
	struct v4l2_format format;
	vid1_fd = open(VIDEO_DEVICE1, O_RDWR);
	if (vid1_fd <= 0)
	{
		dprintf(0, "\nD Failed to open the DSS Video Device\n");
		exit(-1);
	}
	dprintf(2, "\nD opened Video Device 1 for preview \n");
	result = ioctl(vid1_fd, VIDIOC_QUERYCAP, &capability);
	if (result != 0)
	{
		perror("VIDIOC_QUERYCAP");
		goto ERROR_EXIT;
	}
	if (capability.capabilities & V4L2_CAP_STREAMING == 0)
	{
		dprintf(0, "VIDIOC_QUERYCAP indicated that driver is not "
		    "capable of Streaming \n");
		goto ERROR_EXIT;
	}
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	result = ioctl(vid1_fd, VIDIOC_G_FMT, &format);

	if (result != 0)
	{
		perror("VIDIOC_G_FMT");
		goto ERROR_EXIT;
	}


	return;
      ERROR_EXIT:
	close(vid1_fd);
	exit(-1);
}

#if 0
void Camera_dqBuff(void *threadsArg)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents;

	pthread_t thread_id;
	int policy;
	int rc = 0;
	struct sched_param param;
	struct v4l2_buffer filledbuffer;
	int result;

	thread_id = pthread_self();
	dprintf(0, "\n D NEW THREAD CREATED\n");
	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		printf("<Thread> 1 error %d\n", rc);
	printf("<Thread> %d %d\n", policy, param.sched_priority);
	param.sched_priority = 10;
	rc = pthread_setschedparam(thread_id, SCHED_RR /*policy */ , &param);

	if (rc != 0)
		printf("<Thread> 2 error %d\n", rc);

	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		printf("<Thread> 3 error %d\n", rc);
	printf("<Thread> %d %d %d %d\n", policy, param.sched_priority,
	    sched_get_priority_min(policy), sched_get_priority_max(policy));

	filledbuffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	filledbuffer.memory = V4L2_MEMORY_MMAP;
	filledbuffer.flags = 0;
	uRequestedEvents = EVENT_CAMERA_DQ;

	dprintf(0, "waiting for the DQ even\n");
	err =
	    TIMM_OSAL_EventRetrieve(myEventIn, uRequestedEvents,
	    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents, TIMM_OSAL_SUSPEND);
	if (TIMM_OSAL_ERR_NONE != err)
	{
		dprintf(0, "error = %d pRetrievedEvents\n", err,
		    &pRetrievedEvents);
		TIMM_OSAL_Error("Error in Retrieving event!");
		err = OMX_ErrorUndefined;
	}
	dprintf(0, "DQ event received\n");
	while (1)
	{
		filledbuffer.flags = 0;

#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[66].sec = tp.tv_sec;
		timearray[66].usec = tp.tv_usec;
#endif
		dprintf(0, "DQBUF CALLED \n");
		result = ioctl(vid1_fd, VIDIOC_DQBUF, &filledbuffer);
		dprintf(0, "DQBUF buffer index = %d\n", filledbuffer.index);
		if (result != 0)
		{
			perror("VIDIOC_DQBUF FAILED FAILED FAILED FAILED");
		}
#ifdef PROFILE
		gettimeofday(&tp, 0);
		timearray[67].sec = tp.tv_sec;
		timearray[67].usec = tp.tv_usec;
		dprintf(0, "D Camera_dqBuff dbuf= %d usec\n",
		    ((timearray[67].sec - timearray[66].sec) * 1000000 +
			timearray[67].usec - timearray[66].usec));
#endif
		omx_fillthisbuffer(filledbuffer.index,
		    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);
	}

}
#endif


/*========================================================*/
/* @ fn Camera_processfbd: FillBufferDone Event Processing Loop */
/*========================================================*/
void Camera_processfbd(void *threadsArg)
{
	int lines_to_read;
	OMX_BUFFERHEADERTYPE *pBuffHeader = NULL;
	size_t byteswritten = 0;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents;
	struct port_param *pPortParam;
	int buffer_index;
	unsigned int numRemainingIn = 0;

	int ii;
	unsigned int actualSize = 0;
	pthread_t thread_id;
	int policy;
	int rc = 0;
	int buffer_to_q;
	struct sched_param param;

	thread_id = pthread_self();
	dprintf(0, "\n D NEW THREAD CREATED\n");
	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		printf("<Thread> 1 error %d\n", rc);
	printf("<Thread> %d %d\n", policy, param.sched_priority);
	param.sched_priority = 10;
	rc = pthread_setschedparam(thread_id, SCHED_RR /*policy */ , &param);

	if (rc != 0)
		printf("<Thread> 2 error %d\n", rc);

	rc = pthread_getschedparam(thread_id, &policy, &param);
	if (rc != 0)
		printf("<Thread> 3 error %d\n", rc);
	printf("<Thread> %d %d %d %d\n", policy, param.sched_priority,
	    sched_get_priority_min(policy), sched_get_priority_max(policy));
	//sleep(5);
	while (OMX_ErrorNone == err)
	{
		uRequestedEvents = EVENT_CAMERA_FBD;

		dprintf(0, " going to wait for event retreive in thread \n");
		err =
		    TIMM_OSAL_EventRetrieve(myEventIn, uRequestedEvents,
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
			dprintf(0, "\n Thread entered in the\
				while loop waiting for the read from pipe\n");
			err =
			    TIMM_OSAL_ReadFromPipe(pContext->FBD_pipe,
			    &pBuffHeader, sizeof(pBuffHeader), &actualSize,
			    TIMM_OSAL_SUSPEND);
			if (err != TIMM_OSAL_ERR_NONE)
			{
				printf
				    ("\n<Thread>Read from FBD_pipe unsuccessful,\
					going back to wait for event\n");
				break;
			}
			dprintf(2,
			    "\n D  ReadFromPipe successfully returned\n");

			buffer_index = (int)pBuffHeader->pAppPrivate;
			dprintf(2, "\n D buffer_index = %d\n", buffer_index);
			pPortParam =
			    &(pContext->sPortParam[pBuffHeader->
				nOutputPortIndex]);
			dprintf(3, "D FBD port = %d\n",
			    (int)pBuffHeader->nOutputPortIndex);
			dprintf(2,
			    "D buffer index = %d remaing messages=%d\n",
			    (int)pBuffHeader->pAppPrivate, numRemainingIn);

			dprintf(2, "D ---Filled Length is  = %d ",
			    (int)pBuffHeader->nFilledLen);

			if (pBuffHeader->nOutputPortIndex ==
			    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
			{
				dprintf(3,
				    "D Preview port capture frame Done number = %d\n",
				    (int)pPortParam->nCapFrame);
				pPortParam->nCapFrame++;

				if (video_capture == 1)
				{
					lines_to_read =
					    cmd_height + cmd_height / 2;
					dprintf(3,
					    "\n D START writing frame into the fileD\n");
					for (ii = 0; ii < lines_to_read; ++ii)
					{
						byteswritten =
						    fwrite((char
							*)(pBuffHeader->
							pBuffer +
							(ii * 4096)), 1,
						    cmd_width, OutputFile);
						if (byteswritten != cmd_width)
						{
							dprintf(0,
							    "\n Error!! Couldn't write into file correctly\n");
						}

					}
					dprintf(3,
					    "\n D CAP: File write done...\n");
					omx_fillthisbuffer(buffer_index,
					    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);
				}

				/* send to dss for display if the size of the frame permits */
				if (dss_enable == 1)
				{
					dprintf(3,
					    "\n BEFORE  SendbufferToDss \n");
					buffer_to_q =
					    SendbufferToDss(buffer_index);
					if (buffer_to_q != 0xFF)
					{
						omx_fillthisbuffer
						    (buffer_to_q,
						    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);

					}
				}

				dprintf(2, "\n == CAP FRAMES = %d ==\n",
				    TotalCapFrame);
			}

			dprintf(2, "\n ===FBD DONE ===\n");
			TIMM_OSAL_GetPipeReadyMessageCount(pContext->FBD_pipe,
			    (void *)&numRemainingIn);
			dprintf(2,
			    "D buffer index = %d remaing messages=%d\n",
			    (int)pBuffHeader->pAppPrivate, numRemainingIn);

		}
	}
	dprintf(0, " SHOULD NEVER COME HERE\n");

	return;
}

/*========================================================*/
/* @ fn test_camera_preview : function to test the camera preview
test case called from the main() and passed arguments
width, height and image format */
/*========================================================*/
int test_camera_preview(int width, int height, char *image_fmt)
{
	int jj, i;
	char outputfilename[100];
	OMX_CONFIG_CAMOPERATINGMODETYPE tCamOpMode;
	OMX_ERRORTYPE retval = OMX_ErrorUndefined;
	cmd_width = width;
	cmd_height = height;

	dprintf(1, "\nD value of DSS_enable = %d \n", dss_enable);
	/* right now make dss_enable only for preview
	   if( (video_capture == 0) || (width < 848 && height < 486)  ) {
	   dprintf(0,"\nD the display will be on LCD\n");
	   dss_enable=1;
	   }
	 */
	if (video_capture == 0)
	{
		dprintf(0, "\nD the display will be on LCD\n");
		dss_enable = 1;
	}
	if (dss_enable == 1)
	{
		open_video1();
		dprintf(3, "\n BACK FROM VIDEO DEV OPEN CALL\n");
		v4l2pixformat = getv4l2pixformat((const char *)image_fmt);
		dprintf(3, "\n back form getv4l2pixformat function\n");
		if (-1 == v4l2pixformat)
		{
			dprintf(0, " pixel format not supported \n");
			if (vid1_fd)
				close(vid1_fd);
			return -1;
		}
	}

	omx_pixformat = getomxformat((const char *)image_fmt);
	dprintf(3, "\n back form getomxformat function\n");

	if (-1 == omx_pixformat)
	{
		dprintf(0, " pixel cmdformat not supported \n");
		if (vid1_fd)
			close(vid1_fd);
		return -1;
	}
	dprintf(3, "\n OMX FMT = %d\n", omx_pixformat);

	pContext = &oAppData;
	memset(pContext, 0x0, sizeof(SampleCompTestCtxt));

	/* Initialize the callback handles */
	oCallbacks.EventHandler = SampleTest_EventHandler;
	oCallbacks.EmptyBufferDone = SampleTest_EmptyBufferDone;
	oCallbacks.FillBufferDone = SampleTest_FillBufferDone;

	/* video and pre port indexes */
	pContext->nVideoPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_VIDEO;
	pContext->nPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;

	/* Initialize the Semaphores 1. for events and other for
	 * FillBufferDone */
	TIMM_OSAL_SemaphoreCreate(&pContext->hStateSetEvent, 0);
	TIMM_OSAL_SemaphoreCreate(&pContext->hExitSem, 0);

	dprintf(3, "D call OMX_init \n");
	/* Initialize the OMX component */
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[3].sec = tp.tv_sec;
	timearray[3].usec = tp.tv_usec;
#endif
	eError = OMX_Init();
	OMX_TEST_BAIL_IF_ERROR(eError);

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[4].sec = tp.tv_sec;
	timearray[4].usec = tp.tv_usec;
	dprintf(0, "\nD OMX_init takes = %d usec\n",
	    ((timearray[4].sec - timearray[3].sec) * 1000000 +
		timearray[4].usec - timearray[3].usec));
#endif
	dprintf(3, "D call OMX_getHandle \n");

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[5].sec = tp.tv_sec;
	timearray[5].usec = tp.tv_usec;
#endif
	/* Get the handle of OMX camera component */
	eError = OMX_GetHandle(&hComp,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.CAMERA", pContext,
	    &oCallbacks);
	OMX_TEST_BAIL_IF_ERROR(eError);

#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[6].sec = tp.tv_sec;
	timearray[6].usec = tp.tv_usec;
	dprintf(0, "\nD OMX_GetHandle takes = %d usec\n",
	    ((timearray[6].sec - timearray[5].sec) * 1000000 +
		timearray[6].usec - timearray[5].usec));
#endif
	dprintf(3, "\n D GETHANDLE DONE \n");
	/* store the handle in global structure */
	pContext->hComp = hComp;

	if (video_capture == 1)
	{
		sprintf(outputfilename,
		    "/mnt/mmc/camera_bin/out_%dx%d_%s.yuv", width, height,
		    image_fmt);
		dprintf(2, "\nD output filename = %s\n", outputfilename);
		OutputFile = fopen(outputfilename, "a+");
		if (!OutputFile)
			dprintf(0,
			    "\n D ************ ERROR IN OPENING THE OUTPUT FILE D \n");
		dprintf(1, "\nD Video Output File  = %d\n", (int)OutputFile);

		/* set the usecase for the video capture mode */
		OMX_TEST_INIT_STRUCT_PTR(&tCamOpMode,
		    OMX_CONFIG_CAMOPERATINGMODETYPE);
		dprintf(3,
		    "\n D ***** NOW CALLING THE SET OPERATING MODE ****\n");

		tCamOpMode.eCamOperatingMode = OMX_CaptureVideo;
		eError =
		    OMX_SetParameter(pContext->hComp,
		    OMX_IndexCameraOperatingMode, &tCamOpMode);
		OMX_TEST_BAIL_IF_ERROR(eError);

		dprintf(3,
		    "\n D **** SET THE PARMS FOR OPERATING MODE **** \n");
	}
	/* disable all ports and then enable preview and video out port
	 * enabling two ports are necessary right now for OMX Camera component
	 * to work. */
	dprintf(2, "D Disabling all the ports \n");
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandPortDisable,
	    OMX_ALL, NULL);

	OMX_TEST_BAIL_IF_ERROR(eError);

	/* Enable PREVIEW PORT. Video also comes through preview only */
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandPortEnable,
	    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, NULL);
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	OMX_TEST_BAIL_IF_ERROR(eError);

	dprintf(2, "D preview port enabled successfully \n");

	eError = SetFormat(cmd_width, cmd_height, omx_pixformat);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* change state to idle. in between allocate the buffers and
	 * submit them to port */
	dprintf(2, "\nD Changing state from loaded to Idle \n");
	eError = SampleTest_TransitionWait(OMX_StateIdle);
	OMX_TEST_BAIL_IF_ERROR(eError);
	dprintf(2, "\nD IDLE TO EXECUTING ** \n");

	/* change state Executing */
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[71].sec = tp.tv_sec;
	timearray[71].usec = tp.tv_usec;
#endif
	eError = OMX_SendCommand(pContext->hComp, OMX_CommandStateSet,
	    OMX_StateExecuting, NULL);
	OMX_TEST_BAIL_IF_ERROR(eError);
#ifdef PROFILE
	gettimeofday(&tp, 0);
	timearray[72].sec = tp.tv_sec;
	timearray[72].usec = tp.tv_usec;
	dprintf(0,
	    "\nD time to chage from idle to executing takes = %d usec\n",
	    ((timearray[72].sec - timearray[71].sec) * 1000000 +
		timearray[72].usec - timearray[71].usec));
#endif

	/*wait till the State transition is complete */
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	dprintf(2, "D state changed to Executing \n");
	retval =
	    TIMM_OSAL_CreatePipe(&(pContext->FBD_pipe),
	    sizeof(OMX_BUFFERHEADERTYPE *) * DEFAULT_BUFF_CNT,
	    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
	if (retval != 0)
	{
		TIMM_OSAL_Error("Error: TIMM_OSAL_CreatePipe failed to open");
		eError = OMX_ErrorContentPipeCreationFailed;
	}

	/* Create input data read thread */
	eError = TIMM_OSAL_EventCreate(&myEventIn);
	if (TIMM_OSAL_ERR_NONE != eError)
	{
		TIMM_OSAL_Error("Error in creating event!");
		eError = OMX_ErrorInsufficientResources;
	}
	retval = TIMM_OSAL_CreateTask(
	    (void *)&pContext->processFbd,
	    (void *)Camera_processfbd,
	    0, pContext, (10 * 1024), -1, (signed char *)"CAMERA_FBD_TASK");

#if 0
	/* Create input data read thread */
	retval = TIMM_OSAL_CreateTask(
	    (void *)&pContext->dqBuff,
	    (void *)Camera_dqBuff,
	    0,
	    pContext, (10 * 1024), -1, (signed char *)"CAMERA_DQBUFF_TASK");


#endif
	for (jj = 0; jj < DEFAULT_BUFF_CNT; jj++)
	{
		dprintf(3, " Before FillThisBuffer Call \n");
		omx_fillthisbuffer(jj, OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW);
	}

	TIMM_OSAL_SemaphoreObtain(pContext->hExitSem, TIMM_OSAL_SUSPEND);

#ifdef PROFILE
	dprintf(0, "\nD framedone received at framenumber, timeinusec \n");
	for (i = 0; i < framedonecount; ++i)
		//dprintf(0,"%3d\t%10d\t%10d\t%8d\t%8d\n",
		dprintf(0, "%3d\t%10d\n",
		    framedoneprofile[i + 1].bufferindex,
		    ((framedoneprofile[i + 1].sec -
			    framedoneprofile[i].sec) * 1000000 +
			framedoneprofile[i + 1].usec -
			framedoneprofile[i].usec));
#endif
	omx_switch_to_loaded();
	dprintf(2, "\nCalling platform deinit()\n");


      OMX_TEST_BAIL:
	if (eError != OMX_ErrorNone)
	{
		dprintf(0, "\n ERROR test_camera_preview() function");
	}
	omx_comp_release();
	return eError;
}

/*========================================================*/
/* @ fn OMXColorFormatGetBytesPerPixel : Get BytesPerPixel value
as per the Image Format */
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
/* @ fn WaitForState : Wait for State Change to finish    */
/*========================================================*/
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE * pHandle,
    OMX_STATETYPE DesiredState)
{
	OMX_STATETYPE CurState = OMX_StateInvalid;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_U32 pRetrievedEvents;

	/* get the state of the component */
	eError = OMX_GetState(pHandle, &CurState);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* wait while the state of the component is not desired state */
	do
	{
		/* wait for the STATE change event occured on the camera component */
		retval = TIMM_OSAL_EventRetrieve(CameraEvents,
		    CAM_EVENT_CMD_STATE_CHANGE, TIMM_OSAL_EVENT_AND_CONSUME,
		    &pRetrievedEvents, TIMM_OSAL_SUSPEND);
		/* go to exit if we have woken up because of time out or if some
		 * error have occured
		 * */
		GOTO_EXIT_IF(((retval != TIMM_OSAL_ERR_NONE) &&
			(retval == TIMM_OSAL_WAR_TIMEOUT)), OMX_ErrorTimeout);
		/* retrive the current state of the component */
		eError = OMX_GetState(pHandle, &CurState);
		GOTO_EXIT_IF(((eError != OMX_ErrorNone)), eError);
	}
	while (CurState != DesiredState);

      EXIT:
	return eError;
}

/*========================================================*/
/* @ fn test_OMX_CAM_EventHandler : Event handler of the test
case for capture test    */
/*========================================================*/
static OMX_ERRORTYPE test_OMX_CAM_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent,
    OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	dprintf(2, "\n test_OMX_CAM_EventHandler \n");
	if (!ptrAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
	{
		if (nData1 == OMX_CommandStateSet)
		{
			retval = TIMM_OSAL_EventSet(CameraEvents,
			    CAM_EVENT_CMD_STATE_CHANGE, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);

		}
		break;
	}

	case OMX_EventError:
	{
		dprintf(0, "\n OMX_EventError !!! \n");
		break;
	}

	case OMX_EventBufferFlag:
	case OMX_EventMark:
	case OMX_EventPortSettingsChanged:
	case OMX_EventResourcesAcquired:
	case OMX_EventComponentResumed:
	case OMX_EventDynamicResourcesAvailable:
	case OMX_EventPortFormatDetected:
	case OMX_EventMax:
	{
		dprintf(0, "\n OMX_EventError !!!! \n");
		eError = OMX_ErrorNotImplemented;
		break;
	}
	default:
	{
		dprintf(0, "\n OMX_ErrorBadParameter !!!\n");

		eError = OMX_ErrorBadParameter;
		break;
	}
	}

      EXIT:
	return eError;
}

/*========================================================*/
/* @ fn test_OMX_CAM_EmptyBufferDone : Empty bufferdone callback
function*/
/*========================================================*/
static OMX_ERRORTYPE test_OMX_CAM_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
//    TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXCAM, E("Received Empty buffer done call!!!\n"));
	dprintf(2, "\n  Received Empty buffer done call!!!! \n ");
	goto EXIT;

      EXIT:
	return eError;
}

/*========================================================*/
/* @ fn test_OMX_CAM_FillBufferDone : Fillbufferdone callback
function*/
/*========================================================*/
static OMX_ERRORTYPE test_OMX_CAM_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	test_OMX_CAM_AppData_t *appData = ptrAppData;
	OMX_CONFIG_BOOLEANTYPE bOMX;
	size_t byteswritten = 0;
	dprintf(2, "\nD inside FillBufferDone \n");

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "frame no: %d \n", appData->nCapruredFrames);

	if (!pBuffer)
	{
		dprintf(0, "\nD pBuffer in the FillBufferDone is NULL\n");
		goto EXIT;
	}
	dprintf(3, "\n ****FillBufferDone has come for port %d\n",
	    appData->nPreviewPortIndex);

	if (appData->nPreviewPortIndex == pBuffer->nOutputPortIndex)
	{
		OMX_TEST_INIT_STRUCT_PTR(&bOMX, OMX_CONFIG_BOOLEANTYPE);

		if (zoom_prv == 1)
		{
			/* Make zoom */
			if ((CAM_ZOOM_IN_START_ON_FRAME <=
				appData->nCapruredFrames) &&
			    (appData->nCapruredFrames <
				(CAM_ZOOM_IN_START_ON_FRAME +
				    CAM_ZOOM_IN_FRAMES)))
			{
				OMX_CONFIG_SCALEFACTORTYPE tScaleFactor;

				dprintf(3, "\nD inside ZOOM \n");
				OMX_TEST_INIT_STRUCT_PTR(&tScaleFactor,
				    OMX_CONFIG_SCALEFACTORTYPE);

				eError = OMX_GetConfig(hComponent,
				    OMX_IndexConfigCommonDigitalZoom,
				    &tScaleFactor);

				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				tScaleFactor.xWidth += CAM_ZOOM_IN_STEP;
				tScaleFactor.xHeight += CAM_ZOOM_IN_STEP;

				eError = OMX_SetConfig(hComponent,
				    OMX_IndexConfigCommonDigitalZoom,
				    &tScaleFactor);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		}

		if (zoom_prv == 0)
		{
			if ((appData->nCapruredFrames
				== CAM_FRAME_TO_MAKE_STILL_CAPTURE))
			{
				dprintf(3,
				    "\nD inside enabling Still capture \n");
				OMX_TEST_INIT_STRUCT_PTR(&bOMX,
				    OMX_CONFIG_BOOLEANTYPE);
				bOMX.bEnabled = OMX_TRUE;

				eError = OMX_SetConfig(hComponent,
				    OMX_IndexConfigCapturing, &bOMX);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				dprintf(3,
				    "\n Enabled capture by setconfig \n");
			}

		}
		if ((appData->nCapruredFrames ==
			CAM_FRAME_TO_MAKE_STILL_CAPTURE) && (zoom_prv == 1))
		{
			dprintf(3, "\n captured 6 frames  \n");
		}

		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "PRV: Start write to file...\n");
		dprintf(2, "\nD pBuffer=%x, FilledLen=%x, outputfile=%x \n",
		    (uint) pBuffer->pBuffer,
		    (uint) pBuffer->nFilledLen,
		    (uint) appData->pPrvOutputFile);
		byteswritten = fwrite(pBuffer->pBuffer, 1,
		    pBuffer->nFilledLen, appData->pPrvOutputFile);
		dprintf(2, "\n PRV OP FILE bytes_written = %d \n",
		    byteswritten);
		dprintf(2, "\n pBuffer->nFilledLen = %d \n",
		    (int)pBuffer->nFilledLen);

		if (byteswritten != pBuffer->nFilledLen)
		{
			dprintf(0, "\n D ERROR IN WRITING FILE \n");
		}
		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "PRV: File write done.\n");

		if (appData->nCapruredFrames < CAM_FRAMES_TO_CAPTURE)
		{
			dprintf(3, "\n Calling Fill This Buffer. \n");
			OMX_FillThisBuffer(hComponent, pBuffer);
		} else
		{
			retval = TIMM_OSAL_EventSet(CameraEvents,
			    CAM_EVENT_MAIN_LOOP_EXIT, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
		}
		appData->nCapruredFrames++;
		dprintf(2, "appData->nCapruredFrames = %d\n",
		    appData->nCapruredFrames);

	} else if (appData->nCapturePortIndex == pBuffer->nOutputPortIndex)
	{
		OMX_CONFIG_BOOLEANTYPE bOMX;

		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "CAP: Start write to file...\n");
		byteswritten =
		    fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen,
		    appData->pCapOutputFile);
		dprintf(2, "\n PRV OP FILE bytes_written = %d \n",
		    byteswritten);
		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "CAP: File write done.\n");

		OMX_TEST_INIT_STRUCT_PTR(&bOMX, OMX_CONFIG_BOOLEANTYPE);
		bOMX.bEnabled = OMX_FALSE;

		eError = OMX_SetConfig(hComponent,
		    OMX_IndexConfigCapturing, &bOMX);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

	if (eError == OMX_ErrorUndefined)
		eError = OMX_ErrorNone;
      EXIT:
	return eError;
}

/*========================================================*/
/* @ fn test_OMX_CAM_SetParams : SetParameter function for
all basic configurations for Camera Component
*/
/*========================================================*/
static OMX_ERRORTYPE test_OMX_CAM_SetParams(test_OMX_CAM_AppData_t * appData)
{
	OMX_HANDLETYPE pHandle;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	OMX_CONFIG_CAMOPERATINGMODETYPE tCamOpMode;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_CONFIG_ROTATIONTYPE tCapRot;
	OMX_CONFIG_MIRRORTYPE tCapMir;
	OMX_PARAM_THUMBNAILTYPE tThumbnail;
	OMX_CONFIG_SENSORSELECTTYPE tSenSelect;

	/* if component has not been initialized, go to error */
	GOTO_EXIT_IF((!appData || !(appData->pHandle)), eError);

	/* initialize omx struct to be used to set parameter */
	OMX_TEST_INIT_STRUCT_PTR(&tCamOpMode,
	    OMX_CONFIG_CAMOPERATINGMODETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tSenSelect, OMX_CONFIG_SENSORSELECTTYPE);

	if (zoom_prv == 0)
	{
		OMX_TEST_INIT_STRUCT_PTR(&tCapRot, OMX_CONFIG_ROTATIONTYPE);
		OMX_TEST_INIT_STRUCT_PTR(&tCapMir, OMX_CONFIG_MIRRORTYPE);
		OMX_TEST_INIT_STRUCT_PTR(&tThumbnail,
		    OMX_PARAM_THUMBNAILTYPE);
	}
	pHandle = appData->pHandle;

#if 0
	dprintf(0, "\n Setting up the high quality mode for the capture \n");
	/* Select Usecase for HQ. The default setting is for high speed and */
	tCamOpMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
	eError =
	    OMX_SetParameter(pHandle, OMX_IndexCameraOperatingMode,
	    &tCamOpMode);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif

	dprintf(3, "\n D (4) n");
	/* Disable all ports except Preview (VPB+1) port and Image (IPB+0) port */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandPortDisable, OMX_ALL, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	dprintf(2, "\n All Ports Disabled");
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable,
	    appData->nPreviewPortIndex, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	dprintf(2, "\nENABLED PREVIEW PORT ");


	if (zoom_prv == 0)
	{
		dprintf(3, "\n Sending command to enable Capture Port\n");
		eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable,
		    appData->nCapturePortIndex, NULL);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		dprintf(2, "\n ENABLED IMAGE CAPTURE PORT \n");
	}
	/* Setup Preview Port */
	tPortDef.nPortIndex = appData->nPreviewPortIndex;
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
	    &tPortDef);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(3, "\n D (5) n");

	tPortDef.format.video.eColorFormat = appData->ePrvFormat;
	tPortDef.format.video.nFrameWidth = appData->nPrvWidth;
	tPortDef.format.video.nFrameHeight = appData->nPrvHeight;
	tPortDef.format.video.nStride = appData->nPrvStride;
	tPortDef.format.video.xFramerate = 30 << 16;
	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition,
	    &tPortDef);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(3, "\n D (6) n");
	if (zoom_prv == 0)
	{
		/* Setup Image Capture Port */
		tPortDef.nPortIndex = appData->nCapturePortIndex;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		dprintf(3, "\n D (7) n");

		tPortDef.format.image.eCompressionFormat =
		    appData->eCapCompressionFormat;
		tPortDef.format.image.eColorFormat = appData->eCapColorFormat;
		tPortDef.format.image.nFrameWidth = appData->nCapWidth;
		tPortDef.format.image.nFrameHeight = appData->nCapHeight;
		tPortDef.format.image.nStride = appData->nCapStride;
		dprintf(2,
		    " setting capture port for color format = %x, width = %d, height = %d, stride = %d\n",
		    tPortDef.format.image.eColorFormat,
		    tPortDef.format.image.nFrameWidth,
		    tPortDef.format.image.nFrameHeight,
		    tPortDef.format.image.nStride);
		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		dprintf(3, "\n D (8) n");
		tSenSelect.eSensor = appData->eSenSelect;
		eError =
		    OMX_SetParameter(pHandle, OMX_TI_IndexConfigSensorSelect,
		    &tSenSelect);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		dprintf(3, "\n D (8.a) n");
		tCapRot.nPortIndex = appData->nCapturePortIndex;
		tCapRot.nRotation = appData->nCaptureRot;
		eError =
		    OMX_SetConfig(pHandle, OMX_IndexConfigCommonRotate,
		    &tCapRot);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		dprintf(3, "\n D (9) n");
		tCapMir.nPortIndex = appData->nCapturePortIndex;
		tCapMir.eMirror = appData->eCaptureMirror;
		eError =
		    OMX_SetConfig(pHandle, OMX_IndexConfigCommonMirror,
		    &tCapMir);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		dprintf(3, "\n D (10) n");
		tThumbnail.nPortIndex = appData->nCapturePortIndex;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamThumbnail,
		    &tThumbnail);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		dprintf(3, "\n D (11) n");
		tThumbnail.nHeight = appData->nThumbHeight;
		tThumbnail.nWidth = appData->nThumbWidth;
		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamThumbnail,
		    &tThumbnail);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}
	dprintf(3, "\n D (12) n");
	return OMX_ErrorNone;

      EXIT:
	dprintf(0, "\n\n\n D ERROR OCCURED IN SETCONG F returning \n");
	return eError;
}


/*========================================================*/
/* @ fn test_camera_capture : function to test the camera
Image Capture test case called from the main() and passed
arguments width, height and image format
*/
/*========================================================*/
int test_camera_capture(int width, int height, char *image_fmt)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_CALLBACKTYPE appCallbacks;
	OMX_HANDLETYPE pHandle;
	test_OMX_CAM_AppData_t appData;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	TIMM_OSAL_U32 pRetrievedEvents;
	OMX_U32 i;
	OMX_U32 iBufferSize;
	char cap_outputfilename[100];
#ifdef OMXCAM_USE_BUFFER
	MemAllocBlock *tMemBlocks;
	OMX_U8 *pPreviewBuf[6];
	OMX_U8 *pCaptureBuf[6];
#endif

	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);

	omx_pixformat = getomxformat((const char *)image_fmt);
	dprintf(3, "\n back form getomxformat function\n");

	if (-1 == omx_pixformat)
	{
		dprintf(0, " pixel cmdformat not supported \n");
		close(vid1_fd);
		return -1;
	}
	dprintf(2, "\n OMX FMT = %d\n", omx_pixformat);
	appData.nPreviewPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;
	appData.nCapruredFrames = 0;
	/* for preview frames hardcoded for the QVGA size and UYVY fomat */
	appData.nPrvWidth = 320;
	appData.nPrvHeight = 240;
	appData.ePrvFormat = OMX_COLOR_FormatCbYCrY;
	appData.nPrvBytesPerPixel =
	    OMXColorFormatGetBytesPerPixel(appData.ePrvFormat);
	appData.nPrvStride = 4096;
	//appData.nPrvStride = ROUND_UP16(appData.nPrvWidth) * appData.nPrvBytesPerPixel;

	appData.pPrvOutputFile =
	    fopen("/mnt/mmc/camera_bin/prvout.yuv", "wb");
	dprintf(2, "\nD prev output file = %d\n",
	    (int)appData.pPrvOutputFile);
	GOTO_EXIT_IF((appData.pPrvOutputFile == NULL),
	    OMX_ErrorInsufficientResources);

	/* we work with image port in case of image capture */
	if (zoom_prv == 0)
	{
		appData.nCapturePortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;
		appData.nCapWidth = width;
		appData.nCapHeight = height;
		/* for captured buffer hardcoded the frames for UYVY for now */
		appData.eCapColorFormat = OMX_COLOR_FormatCbYCrY;
		appData.eCapCompressionFormat = OMX_IMAGE_CodingJPEG;
		appData.nCapBytesPerPixel =
		    OMXColorFormatGetBytesPerPixel(appData.eCapColorFormat);
		appData.nCapStride =
		    ROUND_UP16(appData.nCapWidth) * appData.nCapBytesPerPixel;
		appData.nCaptureRot = 0;
		appData.eCaptureMirror = OMX_MirrorNone;
		appData.nThumbWidth = 640;
		appData.nThumbHeight = 480;

		/* HARD CODED THE SENSOR SELECTION = PRIMARY SENSOR */
		appData.eSenSelect = OMX_PrimarySensor;

		if (OMX_IMAGE_CodingJPEG == appData.eCapCompressionFormat)
		{
			sprintf(cap_outputfilename,
			    "/mnt/mmc/camera_bin/cap_out_%dx%d_%s.jpg", width,
			    height, image_fmt);
			dprintf(2, "\nD Captured output filename = %s\n",
			    cap_outputfilename);
			appData.pCapOutputFile =
			    fopen(cap_outputfilename, "wb");
		} else
		{
			sprintf(cap_outputfilename,
			    "/mnt/mmc/camera_bin/cap_out_%dx%d_%s.yuv", width,
			    height, image_fmt);
			dprintf(2, "\nD Captured output filename = %s\n",
			    cap_outputfilename);
			appData.pCapOutputFile =
			    fopen(cap_outputfilename, "wb");
		}
		GOTO_EXIT_IF((appData.pCapOutputFile == NULL),
		    OMX_ErrorInsufficientResources);
	}

	retval = TIMM_OSAL_EventCreate(&CameraEvents);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	dprintf(3, "\n D STRUCT INIT + parameters set \n");

	/* OMX_init() call */
	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(2, "\n D OMX_INIT DONE \n");
	appCallbacks.EventHandler = test_OMX_CAM_EventHandler;
	appCallbacks.EmptyBufferDone = test_OMX_CAM_EmptyBufferDone;
	appCallbacks.FillBufferDone = test_OMX_CAM_FillBufferDone;

	/* GetHandle of Camera component */
	eError = OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.CAMERA",
	    &appData, &appCallbacks);

	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	appData.pHandle = pHandle;
	dprintf(2, "\n D OMX_GETHANDLE DONE\n");

	/* Configuring Camera */
	eError = test_OMX_CAM_SetParams(&appData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(3, "\n D SetParamas Done \n");

	/* Transition Loaded -> Idle  */
	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "Send Command Loaded -> Idle\n");

	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(3, "\n D Now lets allocate Buffers \n");
	/* Allocate buffers */
	/* Preview Port */
	tPortDef.nPortIndex = appData.nPreviewPortIndex;
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
	    &tPortDef);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	dprintf(2,
	    "\n getting preview port for color format = %x, width = %d, height = %d, stride = %d\n",
	    tPortDef.format.image.eColorFormat,
	    tPortDef.format.image.nFrameWidth,
	    tPortDef.format.image.nFrameHeight,
	    tPortDef.format.image.nStride);

	appData.nPreviewBuffers = tPortDef.nBufferCountActual;
	dprintf(2, "\n PRV BUFFERS = %d\n", (int)tPortDef.nBufferCountActual);

	appData.ppPreviewBuffers =
	    TIMM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE *) *
	    appData.nPreviewBuffers, TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_EXT);

	GOTO_EXIT_IF((appData.ppPreviewBuffers == TIMM_OSAL_NULL),
	    OMX_ErrorInsufficientResources);

#ifdef OMXCAM_USE_BUFFER
#ifdef OMX_TILERTEST
	tMemBlocks =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!tMemBlocks)
		dprintf(0,
		    "\nD can't allocate memory for Tiler block allocation \n");


	for (i = 0; i < appData.nPreviewBuffers; i++)
	{
		memset((void *)tMemBlocks, 0, sizeof(tMemBlocks) * 2);
		tMemBlocks[0].pixelFormat = PIXEL_FMT_16BIT;
		tMemBlocks[0].dim.area.width =
		    tPortDef.format.video.nFrameWidth;
		tMemBlocks[0].dim.area.height =
		    tPortDef.format.video.nFrameHeight;
		tMemBlocks[0].stride = STRIDE_16BIT;
		pPreviewBuf[i] = MemMgr_Alloc(tMemBlocks, 1);

		if (!pPreviewBuf[i])
			dprintf(0,
			    "\n CAN NOT ALLOCATE MEMORY for TILER BUFFER \n");
		/* ok buffer has been allcoated by the tiler but this would be with 4096 bytes
		 * of stride. So when using this buffer in the UseBuffer, we need to say this
		 * size to the omx compoenent */
		iBufferSize = 4096 * tPortDef.format.video.nFrameHeight;

		eError = OMX_UseBuffer(pHandle, appData.ppPreviewBuffers + i,
		    tPortDef.nPortIndex, &appData,
		    iBufferSize, pPreviewBuf[i]);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}
#else /* OMX_TILERTEST */

	for (i = 0; i < appData.nPreviewBuffers; i++)
	{
		pPreviewBuf[i] = TIMM_OSAL_Malloc(tPortDef.nBufferSize,
		    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
		eError = OMX_UseBuffer(pHandle, appData.ppPreviewBuffers + i,
		    tPortDef.nPortIndex, &appData,
		    tPortDef.nBufferSize, pPreviewBuf[i]);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


#endif /* OMX_TILERTEST */
#else /* OMXCAM_USE_BUFFER */

	for (i = 0; i < appData.nPreviewBuffers; i++)
	{
		eError = OMX_AllocateBuffer(pHandle,
		    appData.ppPreviewBuffers + i,
		    tPortDef.nPortIndex, &appData, tPortDef.nBufferSize);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

#endif /* OMXCAM_USE_BUFFER */


	if (zoom_prv == 0)
	{
		/* Capture Port */
		tPortDef.nPortIndex = appData.nCapturePortIndex;
		eError = OMX_GetParameter(pHandle,
		    OMX_IndexParamPortDefinition, &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		appData.nCaptureBuffers = tPortDef.nBufferCountActual;
		dprintf(2, "\n CAPTURE BUFFERS = %d\n",
		    (int)tPortDef.nBufferCountActual);
		appData.ppCaptureBuffers = TIMM_OSAL_Malloc(sizeof
		    (OMX_BUFFERHEADERTYPE *) * appData.nCaptureBuffers,
		    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((appData.ppCaptureBuffers == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
#ifdef OMXCAM_USE_BUFFER
#ifdef OMX_TILERTEST
		/* for capture port we are using 1D buffer */
		for (i = 0; i < appData.nCaptureBuffers; i++)
		{
			memset((void *)tMemBlocks, 0, sizeof(tMemBlocks));
			tMemBlocks[0].dim.len = tPortDef.nBufferSize;
			tMemBlocks[0].pixelFormat = TILFMT_PAGE;
			tMemBlocks[0].stride = 0;
			pCaptureBuf[i] = MemMgr_Alloc(tMemBlocks, 1);
			if (!pCaptureBuf[i])
				dprintf(0,
				    "\n FAILED TO ALLOCATE TILER BUFFER FOR CAPTURE \n");
			dprintf(0,
			    "\n Giving parameters for UseBuffer in Capture Buffers\n");
			dprintf(0,
			    "\nPortIndex = %d, nBuuferSize= %d, pCaptureBuffer = %d\n",
			    tPortDef.nPortIndex, tPortDef.nBufferSize,
			    pCaptureBuf[i]);
			eError =
			    OMX_UseBuffer(pHandle,
			    appData.ppCaptureBuffers + i, tPortDef.nPortIndex,
			    &appData, tPortDef.nBufferSize, pCaptureBuf[i]);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}
#else /* OMX_TILERTEST */

		for (i = 0; i < appData.nCaptureBuffers; i++)
		{
			pCaptureBuf[i] =
			    TIMM_OSAL_Malloc(tPortDef.nBufferSize,
			    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
			eError =
			    OMX_UseBuffer(pHandle,
			    appData.ppCaptureBuffers + i, tPortDef.nPortIndex,
			    &appData, tPortDef.nBufferSize, pCaptureBuf[i]);

			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}

#endif /* OMX_TILERTEST */
#else
		for (i = 0; i < appData.nCaptureBuffers; i++)
		{
			eError = OMX_AllocateBuffer(pHandle,
			    appData.ppCaptureBuffers + i,
			    tPortDef.nPortIndex, &appData,
			    tPortDef.nBufferSize);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}
#endif
	}

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "Waiting for Loaded -> Idle\n");
	WaitForState(pHandle, OMX_StateIdle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "Waiting for Loaded -> Idle Done\n");


	/* Transition Idle -> Executing  */

	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet,
	    OMX_StateExecuting, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "\nWaiting for Idle -> Executing\n");

	WaitForState(pHandle, OMX_StateExecuting);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "\nWaiting for Idle -> Executing Done\n");

	/* Main Loop */
	if (zoom_prv == 0)
	{
		for (i = 0; i < 1 /*appData.nCaptureBuffers */ ; i++)
		{
			eError = OMX_FillThisBuffer(pHandle,
			    appData.ppCaptureBuffers[i]);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}
	}

	for (i = 0; i < 1 /*appData.nPreviewBuffers */ ; i++)
	{
		eError = OMX_FillThisBuffer(pHandle,
		    appData.ppPreviewBuffers[i]);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

	dprintf(3, "\n Fill Buffer called \n");
	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM, "\nMain loop\n");

	retval =
	    TIMM_OSAL_EventRetrieve(CameraEvents, CAM_EVENT_MAIN_LOOP_EXIT,
	    TIMM_OSAL_EVENT_AND_CONSUME, &pRetrievedEvents,
	    TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF(((retval != TIMM_OSAL_ERR_NONE) &&
		(retval == TIMM_OSAL_WAR_TIMEOUT)), OMX_ErrorTimeout);


	/* Transition Executing -> Idle */
	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM, "Executing -> Idle\n");
	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "Waiting for Executing -> Idle\n");

	WaitForState(pHandle, OMX_StateIdle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
	    "Waiting for Executing -> Idle Done\n");

	/* Transition Idle -> Loaded */
	TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM, "Idle -> Loaded\n");

	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet,
	    OMX_StateLoaded, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



	for (i = 0; i < appData.nPreviewBuffers; i++)
	{
#ifdef OMXCAM_USE_BUFFER
#ifdef OMX_TILERTEST
		eError = MemMgr_Free(pPreviewBuf[i]);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#else /* OMX_TILERTEST */
		TIMM_OSAL_Free(pPreviewBuf[i]);
#endif /* OMX_TILERTEST */
#endif
		eError = OMX_FreeBuffer(pHandle, appData.nPreviewPortIndex,
		    appData.ppPreviewBuffers[i]);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

	if (zoom_prv == 0)
	{
		for (i = 0; i < appData.nCaptureBuffers; i++)
		{
#ifdef OMXCAM_USE_BUFFER
#ifdef OMX_TILERTEST
			eError = MemMgr_Free(pCaptureBuf[i]);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#else /* OMX_TILERTEST */
			TIMM_OSAL_Free(pCaptureBuf[i]);
#endif /* OMX_TILERTEST */
#endif
			eError = OMX_FreeBuffer(pHandle,
			    appData.nCapturePortIndex,
			    appData.ppCaptureBuffers[i]);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}


		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "Waiting for Idle -> Loaded\n");

		WaitForState(pHandle, OMX_StateLoaded);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "Waiting for Idle -> Loaded Done\n");

	}

	/* Free Component Handle */
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	dprintf(3, "\n D FreeHandle Done \n");

	eError = OMX_Deinit();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	dprintf(3, "\n D Deinit() done \n");

	/* TODO Need to check the EXIT routine */
      EXIT:
	TIMM_OSAL_EventDelete(CameraEvents);
	dprintf(3, "\nD TIMM_OSAL_EventDelete Done \n");
	TIMM_OSAL_Free(appData.ppPreviewBuffers);
	dprintf(3, "\nD TIMM_OSAL_Free Done \n");

	if (eError != OMX_ErrorNone)
		TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "\n\tError!!!\n");
	else
		TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXCAM,
		    "\n\tSuccess!!!\n");
	return eError;
}
