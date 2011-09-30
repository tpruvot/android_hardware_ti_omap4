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
* @file omx_jpegd.c
*
* This File contains testcases for the omx jpeg Decoder.
*
* @path
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! =======================================================================
*!  Initial Version
* ========================================================================*/

/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

/*-------program files ----------------------------------------*/
#include <timm_osal_interfaces.h>
#include <timm_osal_trace.h>
#include <timm_osal_error.h>
#include <timm_osal_memory.h>

#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Image.h>
#include <OMX_Index.h>
#include <OMX_TI_Index.h>
#include <OMX_IVCommon.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>


#include "omx_jpegd_util.h"
#include "omx_jpegd_test.h"
#include "omx_jpegd.h"

#define OUTPUT_FILE "JPEGD_0001_320x240_420.jpg"
#define REF_OUTPUT_FILE "Ref_JPEGD_0001_320x240_420.yuv"


#define OMX_LINUX_TILERTEST

#ifdef OMX_LINUX_TILERTEST
/*Tiler APIs*/
#include <memmgr.h>
#include <tiler.h>
#endif

FILE *op_file, *ref_op_file;
int ch1, ch2;
int pass;
int while_pass = 0, loc_diff = 0;


struct
{
	void *start;
	size_t length;
} *buffers;


extern OMX_U32 jpegdec_prev;

int vid1_fd;
#define VIDEO_DEVICE1	"/dev/video1"
#define VIDEO_DEVICE2  "/dev/video2"
int streamon = 0;
static int display_device = 1;

OMX_ERRORTYPE JPEGD_releaseOnSemaphore(OMX_U16 unSemType);
OMX_Status JPEGD_ParseTestCases(uint32_t uMsg, void *pParam,
    uint32_t paramSize);

/* Semaphore type indicators */

#define SEMTYPE_DUMMY  0
#define SEMTYPE_EVENT  1
#define SEMTYPE_ETB    2
#define SEMTYPE_FTB    3

#define MAX_4BIT 15
#define JPEGD_Trace(ARGS,...)  TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,ARGS,##__VA_ARGS__)
#define JPEGD_Error(ARGS,...)   TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,ARGS,##__VA_ARGS__)
#define JPEGD_Entering   TIMM_OSAL_EnteringExt
#define JPEGD_Exiting(ARG) TIMM_OSAL_ExitingExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,ARG)


/* Jpeg Decoder input and output file names along with path */
OMX_U8 omx_jpegd_input_file_path_and_name[MAX_STRING_SIZE];
OMX_U8 omx_jpegd_output_file_path_and_name[MAX_STRING_SIZE];

OMX_STRING strJPEGD = "OMX.TI.DUCATI1.IMAGE.JPEGD";

/* Test context info */
JpegDecoderTestObject TObjD;

#define OMX_JPEGD_INPUT_PATH "/camera_bin/"
#define OMX_JPEGD_OUTPUT_PATH "/camera_bin/output/"

/* ===================================================================== */
/**
* @fn JPEGD_TEST_CalculateBufferSize   Calculates the buffer size for the JPEG Decoder testcases
*
* @param[in] width      Message code.
* @param[in] height     pointer to Function table entry
* @param[in] format    size of the Function table entry structure
*
* @return
*       Size of the buffer
*
*/
/* ===================================================================== */

OMX_S32 JPEGD_TEST_CalculateBufferSize(OMX_IN OMX_S32 width,
    OMX_IN OMX_S32 height, OMX_IN OMX_COLOR_FORMATTYPE format)
{
	OMX_S32 size;

	if (format == OMX_COLOR_FormatYUV420PackedSemiPlanar)
	{
		if ((width & 0x0F) != 0)
			width = width + 16 - (width & 0x0F);
		if ((height & 0x0F) != 0)
			height = height + 16 - (height & 0x0F);
		size = ((width * height) + ((width * height) / 2));

	} else if (format == OMX_COLOR_FormatCbYCrY)
	{
		if ((width & 0x0F) != 0)
			width = width + 16 - (width & 0x0F);
		if ((height & 0x7) != 0)
			height = height + 8 - (height & 0x7);
		size = ((width * height) * 2.0);

	} else if (format == OMX_COLOR_Format16bitRGB565)
	{
		size = ((width * height) * 3);
	} else if (format == OMX_COLOR_Format32bitARGB8888)
	{
		size = ((width * height) * 4);
	} else
		size = 0;

	return (size);
}



/* ==================================================================== */
/**
* @fn JPEGD_TEST_UCP_CalculateSliceBufferSize  Executes the JPEG Decoder testcases
*
* @param[in] width      Message code.
* @param[in] height     pointer to Function table entry
* @param[in] format    size of the Function table entry structure
*
* @return
*
*  @see  <headerfile>.h
*/
/* ==================================================================== */

OMX_S32 JPEGD_TEST_UCP_CalculateSliceBufferSize(OMX_IN OMX_S32 width,
    OMX_IN OMX_S32 height,
    OMX_IN OMX_S32 SliceHeight, OMX_IN OMX_COLOR_FORMATTYPE format)
{

	OMX_S32 size;

	if (format == OMX_COLOR_FormatYUV420PackedSemiPlanar)
	{
		if ((width & 0x0F) != 0)
			width = width + 16 - (width & 0x0F);
		if ((height & 0x0F) != 0)
			height = height + 16 - (height & 0x0F);
		if (SliceHeight == 0)
			size = ((width * height) + ((width * height) / 2));
		else
			size = width * SliceHeight * 1.5;

	} else if (format == OMX_COLOR_FormatYCbYCr)
	{
		if ((width & 0x0F) != 0)
			width = width + 16 - (width & 0x0F);
		if ((height & 0x7) != 0)
			height = height + 8 - (height & 0x7);
		if (SliceHeight == 0)
			size = width * height * 2;
		else
			size = width * SliceHeight * 2;

	} else if (format == OMX_COLOR_Format16bitRGB565)
	{
		if ((width & 0x7) != 0)
			width = width + 8 - (width & 0x7);
		if ((height & 0x7) != 0)
			height = height + 8 - (height & 0x7);
		if (SliceHeight == 0)
			size = width * height;
		else
			size = width * SliceHeight * 2;

	} else if (format == OMX_COLOR_Format32bitARGB8888)
	{
		if ((width & 0x7) != 0)
			width = width + 8 - (width & 0x7);
		if ((height & 0x7) != 0)
			height = height + 8 - (height & 0x7);
		if (SliceHeight == 0)
			size = width * height;
		else
			size = width * SliceHeight * 4;

	} else
		size = 0;


	return (size);
}



/* Application callback Functions */

/* ==================================================================== */
/**
* @fn OMX_JPEGD_TEST_EventHandler  Handles various events like command complete etc
*
* @param[in] hComponent   Handle of the component to be accessed.
* @param[in] pAppData     pointer to Function table entry
* @param[in] eEvent       Event the component notifies to the application
* @param[in] nData1       OMX_ERRORTYPE or OMX_COMMANDTYPE for error or command event
* @param[in] nData2       OMX_STATETYPE for state changes
* @param[in] pEventData   Pointer to additional event specific data
*
* @return
*     OMX_RET_PASS if the test passed
*     OMX_RET_FAIL if the test fails, or possibly other special conditions.
*
*/
/* ==================================================================== */

OMX_ERRORTYPE OMX_JPEGD_TEST_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{

	OMX_ERRORTYPE eError = (OMX_ERRORTYPE) nData1;

	/* Error event is reported through callback */

	if (eEvent == OMX_EventError)
	{
		eError = JPEGD_releaseOnSemaphore(SEMTYPE_EVENT);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while releasing Event semaphore");
	}


	/* CommandComplete event is reported through callback */

	else if (eEvent == OMX_EventCmdComplete)
	{
		if (nData1 == OMX_CommandStateSet)
		{
			if (nData2 == OMX_StateIdle)
			{
				eError =
				    JPEGD_releaseOnSemaphore(SEMTYPE_EVENT);
				JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
				    "Error while releasing Event semaphore");
			} else if (nData2 == OMX_StateExecuting)
			{
				eError =
				    JPEGD_releaseOnSemaphore(SEMTYPE_EVENT);
				JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
				    "Error while releasing Event semaphore");
			}

			/* Till now not supported */
			else if (nData2 == OMX_StatePause)
			{
				eError =
				    JPEGD_releaseOnSemaphore(SEMTYPE_EVENT);
				JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
				    "Error while releasing Event semaphore");
			} else if (nData2 == OMX_StateLoaded)
			{
				eError =
				    JPEGD_releaseOnSemaphore(SEMTYPE_EVENT);
				JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
				    "Error while releasing Event semaphore");
			}
		} else if (nData1 == OMX_CommandFlush)
		{

		}
	}

      EXIT:
	return eError;
}



/* =================================================================== */
/**
* @fn OMX_JPEGD_TEST_EmptyBufferDone   Returns emptied buffers from I/p port to application
*
* @param[in] hComponent       Handle of the component to be accessed.
* @param[in] pAppData         pointer to Function table entry
* @param[in] pBufferHeader    size of the Function table entry structure
*
*
* @return
*     OMX_RET_PASS if the test passed
*     OMX_RET_FAIL if the test fails, or possibly other special conditions.
*
*/
/* =================================================================== */

OMX_ERRORTYPE OMX_JPEGD_TEST_EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBufferHeader)
{

	OMX_ERRORTYPE eError = OMX_ErrorUndefined;

	JPEGD_Trace("OMX_JPEGD_TEST_EmptyBufferDone");

	if (pBufferHeader->nAllocLen == 0)
		JPEGD_Trace("[ZERO length]");

	if (pBufferHeader->nFlags & OMX_BUFFERFLAG_EOS)
	{
		pBufferHeader->nFlags &= ~OMX_BUFFERFLAG_EOS;
		JPEGD_Trace("[EOS detected]");
	}

	/* Release EmptyThisBuffer semaphore */
	eError = JPEGD_releaseOnSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while releasing EmptyThisBuffer semaphore");

	eError = OMX_ErrorNone;
      EXIT:
	return eError;

}

/* ==================================================================== */
/**
* @fn OMX_JPEGD_TEST_FillBufferDone  Returns filled buffers from O/P port to the application for emptying
*
* @param[out] hComponent     Handle of the component to be accessed.
* @param[out] pAppData       pointer to Function table entry
* @param[out] pBufferHeader  size of the Function table entry structure
*
* @return
*     OMX_RET_PASS if the test passed
*     OMX_RET_FAIL if the test fails,or possibly other special conditions.
*
*/
/* ===================================================================== */

OMX_ERRORTYPE OMX_JPEGD_TEST_FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBufferHeader)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	JpegDecoderTestObject *hTObj = &(TObjD);

	JPEGD_Trace("OMX_JPEGD_TEST_FillBufferDone");


	/* Update the output buffer */
	hTObj->pOutBufHeader->nOutputPortIndex =
	    pBufferHeader->nOutputPortIndex;
	hTObj->pOutBufHeader->pBuffer = pBufferHeader->pBuffer;
	hTObj->pOutBufHeader->nFilledLen = pBufferHeader->nFilledLen;
	hTObj->pOutBufHeader->nOffset = pBufferHeader->nOffset;
	hTObj->pOutBufHeader->nAllocLen = pBufferHeader->nAllocLen;
	hTObj->pOutBufHeader->nFlags = pBufferHeader->nFlags;
	hTObj->pOutBufHeader->pAppPrivate = pBufferHeader->pAppPrivate;

	if (hTObj->pOutBufHeader->nOutputPortIndex ==
	    OMX_JPEGD_TEST_OUTPUT_PORT)
	{
		JPEGD_Trace("output");
	} else
		JPEGD_Trace("Unknown port");


	if (hTObj->pOutBufHeader->nAllocLen == 0)
		JPEGD_Trace("[ZERO length]");

	if (hTObj->pOutBufHeader->nFlags & OMX_BUFFERFLAG_EOS)
	{
		hTObj->pOutBufHeader->nFlags &= ~OMX_BUFFERFLAG_EOS;
		JPEGD_Trace("[EOS detected]");
	}

	/* Release FillThisBuffer semaphore */
	eError = JPEGD_releaseOnSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while releasing FillThisBuffer semaphore");

	eError = OMX_ErrorNone;
      EXIT:
	return eError;


}


/* ===================================================================== */
/*
 * @fn JPEGD_pendOnSemaphore()
 * Pends on a semaphore
 * unSemType => 1 : Event Semaphore 2: EmptyThisBuffer semaphore 3: FillThisBuffer semaphore
 */
/* ===================================================================== */

OMX_ERRORTYPE JPEGD_pendOnSemaphore(OMX_U16 unSemType)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	JpegDecoderTestObject *hTObj = &(TObjD);


	if (unSemType == SEMTYPE_DUMMY)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreObtain(hTObj->pOmxJpegdecSem_dummy,
		    TIMM_OSAL_SUSPEND);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "");
	} else if (unSemType == SEMTYPE_EVENT)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreObtain(hTObj->pOmxJpegdecSem_events,
		    TIMM_OSAL_SUSPEND);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "");
	} else if (unSemType == SEMTYPE_ETB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreObtain(hTObj->pOmxJpegdecSem_ETB,
		    TIMM_OSAL_SUSPEND);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "");
	} else if (unSemType == SEMTYPE_FTB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreObtain(hTObj->pOmxJpegdecSem_FTB,
		    TIMM_OSAL_SUSPEND);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "");
	}

	eError = OMX_ErrorNone;
      EXIT:
	return eError;
}



/* =================================================================== */
/*
 * @fn JPEGD_releaseOnSemaphore()
 * Pends on a semaphore
 * unSemType => 1 : Event Semaphore 2: EmptyThisBuffer semaphore 3: FillThisBuffer semaphore
 */
/* =================================================================== */

OMX_ERRORTYPE JPEGD_releaseOnSemaphore(OMX_U16 unSemType)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	JpegDecoderTestObject *hTObj = &(TObjD);

	if (unSemType == SEMTYPE_DUMMY)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreRelease(hTObj->pOmxJpegdecSem_dummy);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error while relasing dummy semaphore");
	} else if (unSemType == SEMTYPE_EVENT)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreRelease(hTObj->pOmxJpegdecSem_events);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error while relasing Event semaphore");
	} else if (unSemType == SEMTYPE_ETB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreRelease(hTObj->pOmxJpegdecSem_ETB);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error while relasing EmptyThisBuffer semaphore");
	} else if (unSemType == SEMTYPE_FTB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreRelease(hTObj->pOmxJpegdecSem_FTB);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error while relasing FillThisBuffer semaphore");
	}

	eError = OMX_ErrorNone;
      EXIT:
	return eError;

}

/* ==================================================================== */
/*
 * @fn JPEGD_CreateSemaphore()
 * Pends on a semaphore
 * unSemType => 1 : Event Semaphore, 2: EmptyThisBuffer semaphore, 3: FillThisBuffer semaphore
 */
/* ==================================================================== */

OMX_ERRORTYPE JPEGD_CreateSemaphore(OMX_U16 unSemType)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	JpegDecoderTestObject *hTObj = &(TObjD);


	if (unSemType == SEMTYPE_DUMMY)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreCreate(&hTObj->pOmxJpegdecSem_dummy,
		    0);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating dummy semaphore");
	} else if (unSemType == SEMTYPE_EVENT)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreCreate(&hTObj->pOmxJpegdecSem_events,
		    0);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating event semaphore");
	} else if (unSemType == SEMTYPE_ETB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreCreate(&hTObj->pOmxJpegdecSem_ETB, 0);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating EmptyThisBuffer semaphore");
	} else if (unSemType == SEMTYPE_FTB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreCreate(&hTObj->pOmxJpegdecSem_FTB, 0);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating FillThisBuffer semaphore");
	}

	eError = OMX_ErrorNone;
      EXIT:
	return eError;

}

/* ===================================================================== */
/*
 * @fn JPEGD_DeleteSemaphore()
 * Pends on a semaphore
 * unSemType => 0 : Dummy Semaphore 1 : Event Semaphore 2: EmptyThisBuffer semaphore  3:FillThisBuffer semaphore
 */
/* ===================================================================== */

OMX_ERRORTYPE JPEGD_DeleteSemaphore(OMX_U16 unSemType)
{
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	JpegDecoderTestObject *hTObj = &(TObjD);


	if (unSemType == SEMTYPE_DUMMY)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreDelete(hTObj->pOmxJpegdecSem_dummy);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating dummy semaphore");
	} else if (unSemType == SEMTYPE_EVENT)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreDelete(hTObj->pOmxJpegdecSem_events);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating event semaphore");
	} else if (unSemType == SEMTYPE_ETB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreDelete(hTObj->pOmxJpegdecSem_ETB);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating EmptyThisBuffer semaphore");
	} else if (unSemType == SEMTYPE_FTB)
	{
		bReturnStatus =
		    TIMM_OSAL_SemaphoreDelete(hTObj->pOmxJpegdecSem_FTB);
		JPEGD_ASSERT(bReturnStatus == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources,
		    "Error creating FillThisBuffer semaphore");
	}

	eError = OMX_ErrorNone;
      EXIT:
	return eError;

}



/* ===================================================================== */
/**
* @fn JPEGD_ParseTestCases     Parse the parameters passed to the JPEG Decoder
*
* @param[in] uMsg         Message code.
* @param[in] pParam       pointer to Function table entry containing test case parameters
* @param[in] paramSize    size of the Function table entry structure
*
* @return
*     OMX_STATUS_OK if the testcase parameters are parsed correctly,
*     OMX_ERROR_INVALID_PARAM if there are wrong parameters.
*
*/
/* ===================================================================== */
OMX_Status JPEGD_ParseTestCases(uint32_t uMsg, void *pParam,
    uint32_t paramSize)
{
	OMX_Status status = OMX_STATUS_OK;
	OMX_TEST_CASE_ENTRY *testcaseEntry;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	char testData[MAX_STRING_SIZE];
	char *subStr;
	char *token[30] = { };
	OMX_S32 count = 0;
	JpegDecoderTestObject *hTObj = &TObjD;
	testcaseEntry = (OMX_TEST_CASE_ENTRY *) pParam;

	/* Point to test case array */
	if (uMsg != OMX_MSG_EXECUTE)
	{
		status = OMX_ERROR_INVALID_PARAM;
		goto EXIT;
	}

	/* Parse test case parameters */
	testcaseEntry->pDynUserData = testcaseEntry->pUserData;
	strcpy(testData, testcaseEntry->pDynUserData);
	subStr = strtok(testData, " ");

	while (subStr != NULL)
	{
		token[count] = subStr;
		count++;
		subStr = strtok(NULL, " ");
	}

	/* Parse input file name along with path */
	strcpy((char *)omx_jpegd_input_file_path_and_name,
	    OMX_JPEGD_INPUT_PATH);
	strcat((char *)omx_jpegd_input_file_path_and_name, token[0]);
	JPEGD_Trace("omx_jpegd_input_file_path_and_name: %s",
	    omx_jpegd_input_file_path_and_name);

	/* Parse output file name along with path */
	strcpy((char *)omx_jpegd_output_file_path_and_name,
	    OMX_JPEGD_OUTPUT_PATH);
	strcat((char *)omx_jpegd_output_file_path_and_name, token[1]);
	JPEGD_Trace("omx_jpegd_output_file_path_and_name: %s",
	    omx_jpegd_output_file_path_and_name);

	/* Parse input image width value */
	hTObj->tTestcaseParam.unOpImageWidth = atoi(token[2]);
	JPEGD_Trace("Input Width: %d", hTObj->tTestcaseParam.unOpImageWidth);

	/* Parse input image height value */
	hTObj->tTestcaseParam.unOpImageHeight = atoi(token[3]);
	JPEGD_Trace("Input Height: %d",
	    hTObj->tTestcaseParam.unOpImageHeight);

	/* Parse slice height value */
	hTObj->tTestcaseParam.unSliceHeight = atoi(token[4]);
	JPEGD_Trace("Slice Height: %d", hTObj->tTestcaseParam.unSliceHeight);


	/* Parse input image color format */
	if (!strcmp((char *)token[5], "OMX_COLOR_FormatCbYCrY"))
	{
		hTObj->tTestcaseParam.eInColorFormat = OMX_COLOR_FormatCbYCrY;
		JPEGD_Trace
		    ("Input Image color format is OMX_COLOR_FormatCbYCrY");
	} else if (!strcmp((char *)token[5],
		"OMX_COLOR_FormatYUV420PackedSemiPlanar"))
	{
		hTObj->tTestcaseParam.eInColorFormat =
		    OMX_COLOR_FormatYUV420PackedSemiPlanar;
		JPEGD_Trace
		    ("Input Image color format is OMX_COLOR_FormatYUV420PackedSemiPlanar");
	} else if (!strcmp((char *)token[5],
		"OMX_COLOR_FormatYUV444Interleaved"))
	{
		hTObj->tTestcaseParam.eInColorFormat =
		    OMX_COLOR_FormatYUV444Interleaved;
		JPEGD_Trace
		    ("Input Image color format is OMX_COLOR_FormatYUV444Interleaved");
	} else
	{
		JPEGD_Trace("Unsupported color format parameter\n");
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	/* Parse output image color format */
	if (!strcmp((char *)token[6], "OMX_COLOR_FormatCbYCrY"))
	{
		hTObj->tTestcaseParam.eOutColorFormat =
		    OMX_COLOR_FormatCbYCrY;
		JPEGD_Trace
		    ("Output Image color format is OMX_COLOR_FormatCbYCrY");
	} else if (!strcmp((char *)token[6],
		"OMX_COLOR_FormatYUV420PackedSemiPlanar"))
	{
		hTObj->tTestcaseParam.eOutColorFormat =
		    OMX_COLOR_FormatYUV420PackedSemiPlanar;
		JPEGD_Trace
		    ("Output Image color format is OMX_COLOR_FormatYUV420PackedSemiPlanar");
	} else if (!strcmp((char *)token[6], "OMX_COLOR_Format16bitRGB565"))
	{
		hTObj->tTestcaseParam.eOutColorFormat =
		    OMX_COLOR_Format16bitRGB565;
		JPEGD_Trace
		    ("Output Image color format is OMX_COLOR_Format16bitRGB565");
	} else if (!strcmp((char *)token[6], "OMX_COLOR_Format32bitARGB8888"))
	{
		hTObj->tTestcaseParam.eOutColorFormat =
		    OMX_COLOR_Format32bitARGB8888;
		JPEGD_Trace
		    ("Output Image color format is OMX_COLOR_Format32bitARGB8888");
	} else
	{
		JPEGD_ASSERT(0, OMX_ErrorBadParameter,
		    "Unsupported color format parameter\n");
	}

	/* Parse input uncompressed image mode value  */
	if (!strcmp((char *)token[7], "OMX_JPEG_UncompressedModeFrame"))
	{
		hTObj->tTestcaseParam.eOutputImageMode =
		    OMX_JPEG_UncompressedModeFrame;
		JPEGD_Trace
		    ("Uncompressed image mode: OMX_JPEG_UncompressedModeFrame");
	} else if (!strcmp((char *)token[7],
		"OMX_JPEG_UncompressedModeSlice"))
	{
		hTObj->tTestcaseParam.eOutputImageMode =
		    OMX_JPEG_UncompressedModeSlice;
		JPEGD_Trace
		    ("Uncompressed image mode: OMX_JPEG_UncompressedModeSlice");
	} else
	{
		JPEGD_ASSERT(0, OMX_ErrorBadParameter,
		    "Unsupported color format parameter\n");
	}


	/* Parse X Origin value */
	hTObj->pSubRegionDecode.nXOrg = atoi(token[8]);
	JPEGD_Trace("X Origin: %d", hTObj->pSubRegionDecode.nXOrg);

	/* Parse X Origin value */
	hTObj->pSubRegionDecode.nYOrg = atoi(token[9]);
	JPEGD_Trace("Y Origin: %d", hTObj->pSubRegionDecode.nYOrg);


	/* Parse X Length value */
	hTObj->pSubRegionDecode.nXLength = atoi(token[10]);
	JPEGD_Trace("X Length: %d", hTObj->pSubRegionDecode.nXLength);

	/* Parse Y Length value */
	hTObj->pSubRegionDecode.nYLength = atoi(token[11]);
	JPEGD_Trace("Y Length: %d", hTObj->pSubRegionDecode.nYLength);


	/* Read bIsApplnMemAllocatorforInBuff flag */
	if (!strcmp((char *)token[12], "OMX_TRUE"))
	{
		hTObj->tTestcaseParam.bIsApplnMemAllocatorforInBuff =
		    OMX_TRUE;
		JPEGD_Trace("bIsApplnMemAllocatorforInBuff = OMX_TRUE");
	} else if (!strcmp((char *)token[12], "OMX_FALSE"))
	{
		hTObj->tTestcaseParam.bIsApplnMemAllocatorforInBuff =
		    OMX_FALSE;
		JPEGD_Trace(" bIsApplnMemAllocatorforInBuff = OMX_FALSE");
	} else
	{
		JPEGD_ASSERT(0, OMX_ErrorBadParameter,
		    "Invalid test case parameter value\n");
	}

	/* Read bIsApplnMemAllocatorforOutBuff flag */
	if (!strcmp((char *)token[13], "OMX_TRUE"))
	{
		hTObj->tTestcaseParam.bIsApplnMemAllocatorforOutBuff =
		    OMX_TRUE;
		JPEGD_Trace("bIsApplnMemAllocatorforOutBuff = OMX_TRUE");
	} else if (!strcmp((char *)token[13], "OMX_FALSE"))
	{
		hTObj->tTestcaseParam.bIsApplnMemAllocatorforOutBuff =
		    OMX_FALSE;
		JPEGD_Trace("bIsApplnMemAllocatorforOutBuff = OMX_FALSE");
	} else
	{
		JPEGD_ASSERT(0, OMX_ErrorBadParameter,
		    "Invalid test case parameter value\n");
	}

	eError = OMX_ErrorNone;

      EXIT:
	if (!eError == OMX_ErrorNone)
	{
		status = OMX_ERROR_INVALID_PARAM;
	}
	return status;
}

OMX_U32 getv4l2pixformat(OMX_COLOR_FORMATTYPE eOutColorFormat)
{

	switch (eOutColorFormat)
	{
	case OMX_COLOR_FormatYCbYCr:
	{
		return V4L2_PIX_FMT_YUYV;
		break;
	}
	case OMX_COLOR_FormatCbYCrY:
	{
		return V4L2_PIX_FMT_UYVY;
		break;
	}
	case OMX_COLOR_FormatYUV420SemiPlanar:
	{
		return V4L2_PIX_FMT_NV12;
		break;
	}
	default:
	{
		printf("\nD Color Format currently not supported \n");
		return -1;
		break;
	}
	}
}

void SetFormatforDSSvid(unsigned int width, unsigned int height)
{
	struct v4l2_format format;
	JpegDecoderTestObject *hTObj = &TObjD;
	int result;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	format.fmt.pix.pixelformat =
	    getv4l2pixformat(hTObj->tTestcaseParam.eOutColorFormat);
	printf("\nD going to set width=%d, height =%d, format = %d\n", width,
	    height, format.fmt.pix.pixelformat);
	if (format.fmt.pix.pixelformat == -1)
	{
		printf("\nD We didn't get the pixel format correct \n");
		return;
	}
	/* set format of the picture captured */
	result = ioctl(vid1_fd, VIDIOC_S_FMT, &format);
	if (result != 0)
	{
		perror("VIDIOC_S_FMT");
	}
	return;
}

uint getDSSBuffers(uint count, char *omxbuffer, uint length,
    unsigned int width, unsigned int height)
{
	int result;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer filledbuffer;
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
	printf("\nD DSS driver allocated %d buffers \n", reqbuf.count);
	if (reqbuf.count != count)
	{
		printf("\nD DSS DRIVER FAILED TO ALLOCATE THE REQUESTED");
		printf(" NUMBER OF BUFFERS. NOT FORCING STRICT ERROR CHECK");
		printf(" AND SO THE APPLIACTION MAY FAIL.\n");
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
		printf("%d: buffer.length=%d, buffer.m.offset=%d\n",
		    i, buffer.length, buffer.m.offset);
		if (buffers[i].length > length)
		{
			printf
			    ("\n\nD WE GOT A PROBLEM, BUFFER LENGHT SUPPLIED BY dss IS LESS THAN DECODED PICTURE SIZE \n");
			printf("\n OMX FILLED LENGTH = %d\n", length);
		}
		buffers[i].length = buffer.length;
		buffers[i].start = mmap(NULL, buffer.length, PROT_READ |
		    PROT_WRITE, MAP_SHARED, vid1_fd, buffer.m.offset);
		if (buffers[i].start == MAP_FAILED)
		{
			perror("mmap");
			return -1;
		}

		printf("Buffers[%d].start = %x  length = %d\n", i,
		    buffers[i].start, buffers[i].length);
	}
	printf("\nD copying data from jpeg dec buffer to DSS buffer \n");
	printf("\nD input address = %x, output address = %x, length = %d\n",
	    buffers[0].start, omxbuffer, length);

	/* copy the omx buffer data into the dss buffer. this is necessary because omx buffer is 1D and
	 * dss buffer is 2D with 4096 bytes as stride
	 */
	for (i = 0; i < height; i++)
	{
		/* copy same data in to two buffer and use two buffers to display the image
		 * With one buffer DQbuffer will not work. Reason: DSS wants alteast one buffer
		 * to be in QUE to display the contents on the screen
		 */
		memcpy((buffers[0].start + (i * 4096)),
		    (omxbuffer + (i * 2 * width)), (width * 2));
		memcpy((buffers[1].start + (i * 4096)),
		    (omxbuffer + (i * 2 * width)), (width * 2));
		//
		//for ( j=0; j< wt/2; ++j) {
		//      printf("%8x, ", (uint)(*(uint *)(buffers[0].start + (i *4096) + (j))));
		//}
		printf("\n");

	}

	printf("\nD going to Queing the Buffer \n");
	filledbuffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	filledbuffer.memory = V4L2_MEMORY_MMAP;
	filledbuffer.flags = 0;
	filledbuffer.index = 0;
	result = ioctl(vid1_fd, VIDIOC_QBUF, &filledbuffer);


	if (result != 0)
	{
		perror("VIDIOC_QBUF");
		return 1;
	}
	printf("\nD Queing Successfull\n, starting Streaming \n");
	if (streamon == 0)
	{
		printf("\n STREAM ON DONE !!! \n");
		result = ioctl(vid1_fd, VIDIOC_STREAMON, &filledbuffer.type);
		printf("\n STREAMON result = %d\n", result);
		if (result != 0)
		{
			perror("VIDIOC_STREAMON FAILED FAILED FAILED FAILED");

		}
		streamon = 1;
	}
	printf("\n DQBUF CALLED\n");

	filledbuffer.index = 1;
	result = ioctl(vid1_fd, VIDIOC_QBUF, &filledbuffer);
	if (result != 0)
	{
		perror("VIDIOC_QBUF");
		return 1;
	}
	for (i = 0; i < 200; ++i)
	{
		result = ioctl(vid1_fd, VIDIOC_DQBUF, &filledbuffer);
		if (result != 0)
		{
			perror("VIDIOC_DQBUF FAILED FAILED FAILED FAILED");
		}
		filledbuffer.index = i % 2;
		result = ioctl(vid1_fd, VIDIOC_QBUF, &filledbuffer);
		if (result != 0)
		{
			perror("VIDIOC_QBUF");
			return 1;
		}
	}
	result = ioctl(vid1_fd, VIDIOC_DQBUF, &filledbuffer);
	printf("\n DQBUF result = %d\n", result);
	if (result != 0)
	{
		perror("VIDIOC_DQBUF FAILED FAILED FAILED FAILED");
	}
	/* what else do you want, shown all buffers on the screen
	 * going to disable the streaming
	 */
	for (i = 0; i < reqbuf.count; i++)
	{
		if (buffers[i].start)
			munmap(buffers[i].start, buffers[i].length);
	}

	result = ioctl(vid1_fd, VIDIOC_STREAMOFF, &filledbuffer.type);
	if (result != 0)
	{
		perror("VIDIOC_STREAMOFF");
		return 1;
	}


	return 0;

}


void open_video1(int width, int height)
{
	int result;
	struct v4l2_capability capability;
	struct v4l2_format format;
	if (display_device == 1)
	{
		vid1_fd = open(VIDEO_DEVICE1, O_RDWR);
		display_device = 2;
	} else
	{
		vid1_fd = open(VIDEO_DEVICE2, O_RDWR);
		display_device = 1;
	}
	if (vid1_fd <= 0)
	{
		printf("\nD Failed to open the DSS Video Device\n");
		exit(-1);
	}
	printf("\nD opened Video Device 1 for preview \n");
	result = ioctl(vid1_fd, VIDIOC_QUERYCAP, &capability);
	if (result != 0)
	{
		perror("VIDIOC_QUERYCAP");
		goto ERROR_EXIT;
	}
	if (capability.capabilities & V4L2_CAP_STREAMING == 0)
	{
		printf("VIDIOC_QUERYCAP indicated that driver is not "
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
	SetFormatforDSSvid(width, height);


	return;
      ERROR_EXIT:
	close(vid1_fd);
	exit(-1);
}



/* ==================================================================== */
/**

}; * @fn JPEGD_TestFrameMode    Executes the JPEG Decoder testcases For Frame mode
*
* @param[in] uMsg         Message code.
* @param[in] pParam       pointer to Function table entry
* @param[in] paramSize    size of the Function table entry structure
*
* @return
*     OMX_RET_PASS if the test passed,
*     OMX_RET_FAIL if the test fails,or possibly other special conditions.
*
*/
/* ==================================================================== */


OMX_TestStatus JPEGD_TestFrameMode(uint32_t uMsg, void *pParam,
    uint32_t paramSize)
{
	OMX_Status status = OMX_STATUS_OK;
	OMX_TestStatus testResult = OMX_RET_PASS;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	OMX_CALLBACKTYPE tJpegDecoderCb;
	OMX_U32 nframeSize, nInputBitstreamSize;
	OMX_U32 *p_in, *p_out;
	OMX_U32 unOpImageWidth, unOpImageHeight, unSliceHeight;
	OMX_COLOR_FORMATTYPE eInColorFormat, eOutColorFormat;
	OMX_JPEG_UNCOMPRESSEDMODETYPE eOutputImageMode;
	OMX_BOOL bIsApplnMemAllocatorforInBuff,
	    bIsApplnMemAllocatorforOutBuff;
	FILE *ipfile, *opfile;
	OMX_S32 nreadSize;
	OMX_COMPONENTTYPE *component;
	OMX_U32 actualSize = 0;
	JpegDecoderTestObject *hTObj = &TObjD;
	int totalRead = 0;
	char *currPtr = NULL;
#ifdef OMX_LINUX_TILERTEST
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddrIn = NULL;
	OMX_PTR TilerAddrOut = NULL;
#endif


	JPEGD_Trace("*** Test Case : %s ***", __FUNCTION__);
	printf("\n P JPEGD_TestFrameMode ");

	status = JPEGD_ParseTestCases(uMsg, pParam, paramSize);

	if (status != OMX_STATUS_OK)
	{
		testResult = OMX_RET_FAIL;
		printf("\n P FAIL ");
		goto EXIT;
	}

	unOpImageWidth = hTObj->tTestcaseParam.unOpImageWidth;
	unOpImageHeight = hTObj->tTestcaseParam.unOpImageHeight;
	unSliceHeight = hTObj->tTestcaseParam.unSliceHeight;
	eInColorFormat = hTObj->tTestcaseParam.eInColorFormat;
	eOutColorFormat = hTObj->tTestcaseParam.eOutColorFormat;
	eOutputImageMode = hTObj->tTestcaseParam.eOutputImageMode;
	bIsApplnMemAllocatorforInBuff =
	    hTObj->tTestcaseParam.bIsApplnMemAllocatorforInBuff;
	bIsApplnMemAllocatorforOutBuff =
	    hTObj->tTestcaseParam.bIsApplnMemAllocatorforOutBuff;

	/* Input file size */
	ipfile =
	    fopen((const char *)omx_jpegd_input_file_path_and_name, "rb");
	if (ipfile == NULL)
	{
		JPEGD_Trace("Error opening the file %s",
		    omx_jpegd_input_file_path_and_name);
		printf("\n P Error in opening input file ");
		goto EXIT;
	} else
	{
		fseek(ipfile, 0, SEEK_END);
		nInputBitstreamSize = ftell(ipfile);	//-bitstream_start;
		fseek(ipfile, 0, SEEK_SET);
		fclose(ipfile);
	}

	if (jpegdec_prev == 1)
		open_video1(unOpImageWidth, unOpImageHeight);
	nframeSize =
	    JPEGD_TEST_CalculateBufferSize(unOpImageWidth, unOpImageHeight,
	    eOutColorFormat);

    /**************************************************************************************/

	/* Create a dummy semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_DUMMY);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating dummy semaphore");
	JPEGD_Trace("Dummy semaphore created");
	printf("\n P Dummy Semaphore Created ");

	/* Create an Event semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating Event semaphore");
	JPEGD_Trace("Event semaphore created");
	printf("\n P Event Semaphore Created ");

	/* Create an EmptyThisBuffer semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating EmptyThisBuffer semaphore");
	JPEGD_Trace("EmptyThisBuffer semaphore created");
	printf("\n P EmplyThisBuffer semaphore created ");

	/* Create a FillThisBuffer semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating FillThisBuffer semaphore");
	JPEGD_Trace("FillThisBuffer semaphore created");
	printf("\n P FillThisBuffer Semaphore Created ");


    /***************************************************************************************/

	/* Set call back functions */
	tJpegDecoderCb.EmptyBufferDone = OMX_JPEGD_TEST_EmptyBufferDone;
	tJpegDecoderCb.EventHandler = OMX_JPEGD_TEST_EventHandler;
	tJpegDecoderCb.FillBufferDone = OMX_JPEGD_TEST_FillBufferDone;

	/* Load the OMX_JPG_DEC Component */
	hTObj->hOMXJPEGD = NULL;
	eError = OMX_GetHandle(&hTObj->hOMXJPEGD, strJPEGD, hTObj,
	    &tJpegDecoderCb);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while obtaining component handle");
	JPEGD_Trace("OMX_GetHandle successful");
	printf("\n P OMX_GetHandle successful ");

	/* Handle of the component to be accessed */
	component = (OMX_COMPONENTTYPE *) hTObj->hOMXJPEGD;

	/*Initialize size and version information for all structures */
	JPEGD_STRUCT_INIT(hTObj->tJpegdecPortInit, OMX_PORT_PARAM_TYPE);
	JPEGD_STRUCT_INIT(hTObj->tInputPortDefnType,
	    OMX_PARAM_PORTDEFINITIONTYPE);
	JPEGD_STRUCT_INIT(hTObj->tOutputPortDefnType,
	    OMX_PARAM_PORTDEFINITIONTYPE);
	JPEGD_STRUCT_INIT(hTObj->tUncompressedMode,
	    OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE);
	JPEGD_STRUCT_INIT(hTObj->pSubRegionDecode,
	    OMX_IMAGE_PARAM_DECODE_SUBREGION);

    /***************************************************************************************/

	/*  Test Set Parameters OMX_PORT_PARAM_TYPE */
	hTObj->tJpegdecPortInit.nPorts = 2;
	hTObj->tJpegdecPortInit.nStartPortNumber = 0;
	eError =
	    OMX_SetParameter(component, OMX_IndexParamImageInit,
	    &hTObj->tJpegdecPortInit);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PORT_PARAM_TYPE ");
	JPEGD_Trace("OMX_SetParameter image param done");
	printf("\n P OMX_SetParameter image param done ");

	/* Test Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for input port */
	hTObj->tInputPortDefnType.nPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->tInputPortDefnType.eDir = OMX_DirInput;
	hTObj->tInputPortDefnType.nBufferCountActual = 1;
	hTObj->tInputPortDefnType.nBufferCountMin = 1;
	hTObj->tInputPortDefnType.nBufferSize = nInputBitstreamSize;
	hTObj->tInputPortDefnType.bEnabled = OMX_TRUE;
	hTObj->tInputPortDefnType.bPopulated = OMX_FALSE;
	hTObj->tInputPortDefnType.eDomain = OMX_PortDomainImage;
	hTObj->tInputPortDefnType.format.image.cMIMEType = "OMXJPEGD";
	hTObj->tInputPortDefnType.format.image.pNativeRender = 0;
	hTObj->tInputPortDefnType.format.image.nFrameWidth =
	    nInputBitstreamSize;
	hTObj->tInputPortDefnType.format.image.nFrameHeight = 1;
	hTObj->tInputPortDefnType.format.image.nStride = 1;
	hTObj->tInputPortDefnType.format.image.nSliceHeight = 1;
	hTObj->tInputPortDefnType.format.image.bFlagErrorConcealment =
	    OMX_FALSE;
	hTObj->tInputPortDefnType.format.image.eCompressionFormat =
	    OMX_IMAGE_CodingJPEG;
	hTObj->tInputPortDefnType.format.image.eColorFormat = eInColorFormat;
	hTObj->tInputPortDefnType.format.image.pNativeWindow = 0x0;
	hTObj->tInputPortDefnType.bBuffersContiguous = OMX_FALSE;
	hTObj->tInputPortDefnType.nBufferAlignment = 0;

	eError = OMX_SetParameter(component, OMX_IndexParamPortDefinition,
	    &hTObj->tInputPortDefnType);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for input port");
	JPEGD_Trace("OMX_SetParameter input port defn done successfully");
	printf("\n P OMX_SetParameter input port defn done successfully");

	/* Test Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for output port */
	hTObj->tOutputPortDefnType.nPortIndex = OMX_JPEGD_TEST_OUTPUT_PORT;
	hTObj->tOutputPortDefnType.eDir = OMX_DirOutput;
	hTObj->tOutputPortDefnType.nBufferCountActual = 1;
	hTObj->tOutputPortDefnType.nBufferCountMin = 1;
	hTObj->tOutputPortDefnType.nBufferSize = nframeSize;
	hTObj->tOutputPortDefnType.bEnabled = OMX_TRUE;
	hTObj->tInputPortDefnType.bPopulated = OMX_FALSE;
	hTObj->tOutputPortDefnType.eDomain = OMX_PortDomainImage;
	hTObj->tOutputPortDefnType.format.image.cMIMEType = "OMXJPEGD";
	hTObj->tOutputPortDefnType.format.image.pNativeRender = 0;
	hTObj->tOutputPortDefnType.format.image.nFrameWidth = unOpImageWidth;
	hTObj->tOutputPortDefnType.format.image.nFrameHeight =
	    unOpImageHeight;
	hTObj->tOutputPortDefnType.format.image.nStride = unOpImageWidth;
	hTObj->tOutputPortDefnType.format.image.nSliceHeight = unSliceHeight;
	hTObj->tOutputPortDefnType.format.image.bFlagErrorConcealment =
	    OMX_FALSE;
	hTObj->tOutputPortDefnType.format.image.eCompressionFormat =
	    OMX_IMAGE_CodingUnused;
	hTObj->tOutputPortDefnType.format.image.eColorFormat =
	    eOutColorFormat;
	hTObj->tOutputPortDefnType.format.image.pNativeWindow = 0x0;
	hTObj->tOutputPortDefnType.bBuffersContiguous = OMX_FALSE;
	hTObj->tOutputPortDefnType.nBufferAlignment = 64;

	eError =
	    OMX_SetParameter(hTObj->hOMXJPEGD, OMX_IndexParamPortDefinition,
	    &hTObj->tOutputPortDefnType);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for output port");
	JPEGD_Trace("OMX_SetParameter output port defn done successfully");
	printf("\n P OMX_SetParameter output port defn done successfully ");

	/* Test Set Parameteres OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE */
	hTObj->tUncompressedMode.nPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->tUncompressedMode.eUncompressedImageMode = eOutputImageMode;

	eError = OMX_SetParameter(hTObj->hOMXJPEGD,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamJPEGUncompressedMode,
	    &hTObj->tUncompressedMode);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE");
	JPEGD_Trace
	    ("OMX_SetParameter input image uncompressed mode done successfully");
	printf
	    ("\n OMX_SetParameter input image uncompressed mode done successfully");

	/* Set Parameteres OMX_IMAGE_PARAM_DECODE_SUBREGION   */
	eError = OMX_SetParameter(component,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamDecodeSubregion,
	    &hTObj->pSubRegionDecode);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_IMAGE_PARAM_DECODE_SUBREGION for sub region decode");
	JPEGD_Trace("OMX_SetParameter sub region decode done successfully");
	printf("\n P OMX_SetParameter sub region decode done successfully ");


	/* Create data pipe for input port */
	bReturnStatus =
	    TIMM_OSAL_CreatePipe(&hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT],
	    OMX_JPEGD_TEST_PIPE_SIZE, OMX_JPEGD_TEST_PIPE_MSG_SIZE, 1);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorContentPipeCreationFailed,
	    "Error while creating input pipe for JPEGD test object");

	/* Create data pipe for output port */
	bReturnStatus =
	    TIMM_OSAL_CreatePipe(&hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], OMX_JPEGD_TEST_PIPE_SIZE,
	    OMX_JPEGD_TEST_PIPE_MSG_SIZE, 1);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorContentPipeCreationFailed,
	    "Error while creating output pipe for JPEGD test object");


    /**************************************************************************************/

	/* Move to idle state */
	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while issueing Idle command");

#ifdef OMX_LINUX_TILERTEST
	printf("\n TILER BUFFERS \n");
	MemReqDescTiler =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!MemReqDescTiler)
		printf
		    ("\nD can't allocate memory for Tiler block allocation \n");
#endif

	if (bIsApplnMemAllocatorforInBuff == OMX_TRUE)
	{
#ifdef OMX_LINUX_TILERTEST
		MemReqDescTiler[0].pixelFormat = TILFMT_PAGE;
		MemReqDescTiler[0].dim.len = nInputBitstreamSize;
		MemReqDescTiler[0].stride = 0;

		printf
		    ("\nBefore tiler alloc for the Codec Internal buffer %d\n");
		TilerAddrIn = MemMgr_Alloc(MemReqDescTiler, 1);
		printf("\nTiler buffer allocated is %x\n", TilerAddrIn);
		p_in = (OMX_U32 *) TilerAddrIn;

#else
		printf("\n +++WHY HERE ???? +++\n");
		/* Request the component to allocate input buffer and buffer header */
		p_in =
		    (OMX_U32 *) TIMM_OSAL_MallocExtn(nInputBitstreamSize,
		    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT, NULL);
		JPEGD_ASSERT(NULL != p_in, eError,
		    "Error while allocating input buffer by OMX component");

#endif
		eError =
		    OMX_UseBuffer(component, &(hTObj->pInBufHeader),
		    OMX_JPEGD_TEST_INPUT_PORT, NULL, nInputBitstreamSize,
		    (OMX_U8 *) p_in);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating input buffer by OMX client");
	} else
	{
		eError = OMX_AllocateBuffer(component, &hTObj->pInBufHeader,
		    OMX_JPEGD_TEST_INPUT_PORT, NULL, nInputBitstreamSize);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating input buffer by OMX component");
	}


	if (bIsApplnMemAllocatorforOutBuff == OMX_TRUE)
	{
#ifdef OMX_LINUX_TILERTEST
		MemReqDescTiler[0].pixelFormat = TILFMT_PAGE;
		MemReqDescTiler[0].dim.len = nframeSize;
		MemReqDescTiler[0].stride = 0;

		printf
		    ("\nBefore tiler alloc for the Codec Internal buffer %d\n");
		TilerAddrOut = MemMgr_Alloc(MemReqDescTiler, 1);
		printf("\nTiler buffer allocated is %x\n", TilerAddrOut);
		p_out = (OMX_U32 *) TilerAddrOut;

#else
		/* Request the component to allocate output buffer and buffer header */
		p_out =
		    (OMX_U32 *) TIMM_OSAL_MallocExtn(nframeSize,
		    TIMM_OSAL_TRUE, 64, TIMMOSAL_MEM_SEGMENT_EXT, NULL);
		JPEGD_ASSERT(NULL != p_out, eError,
		    "Error while allocating output buffer by OMX component");
#endif
		eError =
		    OMX_UseBuffer(component, &(hTObj->pOutBufHeader),
		    OMX_JPEGD_TEST_OUTPUT_PORT, NULL, nframeSize,
		    (OMX_U8 *) p_out);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating output buffer by OMX client");

	} else
	{
		eError = OMX_AllocateBuffer(component, &hTObj->pOutBufHeader,
		    OMX_JPEGD_TEST_OUTPUT_PORT, NULL, nframeSize);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating output buffer by OMX component");
	}
	hTObj->pOutBufHeader->nAllocLen = nframeSize;

    /*************************************************************************************/

	/*Move to Executing state */
	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet,
	    OMX_StateExecuting, NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	ipfile =
	    fopen((const char *)omx_jpegd_input_file_path_and_name, "rb");
	if (ipfile == NULL)
	{
		JPEGD_Trace("Error opening the file %s",
		    omx_jpegd_input_file_path_and_name);
		goto EXIT;
	}

	TIMM_OSAL_Memset(hTObj->pInBufHeader->pBuffer, 0,
	    nInputBitstreamSize);

	currPtr = (char *)hTObj->pInBufHeader->pBuffer;
	do
	{
		nreadSize = -1;
		nreadSize = fread(currPtr, 1, nInputBitstreamSize, ipfile);
		if (feof(ipfile))
		{
			break;
		}
		totalRead += nreadSize;
		currPtr += nreadSize;
	}
	while (totalRead < nInputBitstreamSize);
	fclose(ipfile);


	/* Write the input buffer info to the input data pipe */
	bReturnStatus =
	    TIMM_OSAL_WriteToPipe(hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT],
	    &hTObj->pInBufHeader, sizeof(hTObj->pInBufHeader),
	    TIMM_OSAL_SUSPEND);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	/* Write the output buffer info to the output data pipe */
	bReturnStatus =
	    TIMM_OSAL_WriteToPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], &hTObj->pOutBufHeader,
	    sizeof(hTObj->pOutBufHeader), TIMM_OSAL_SUSPEND);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");


    /********************************************************************************************/

	hTObj->pInBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
	hTObj->pInBufHeader->nInputPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->pInBufHeader->nAllocLen = nreadSize;
	hTObj->pInBufHeader->nFilledLen = hTObj->pInBufHeader->nAllocLen;
	hTObj->pInBufHeader->nOffset = 0;

	eError = component->EmptyThisBuffer(component, hTObj->pInBufHeader);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	bReturnStatus =
	    TIMM_OSAL_ReadFromPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], &hTObj->pOutBufHeader,
	    sizeof(hTObj->pOutBufHeader), &actualSize, 0);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	hTObj->pOutBufHeader->nOutputPortIndex = OMX_JPEGD_TEST_OUTPUT_PORT;

	TIMM_OSAL_Memset(hTObj->pOutBufHeader->pBuffer, 0,
	    hTObj->pOutBufHeader->nAllocLen);
	eError = component->FillThisBuffer(component, hTObj->pOutBufHeader);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Send input and output buffers to the component for processing */
	bReturnStatus =
	    TIMM_OSAL_ReadFromPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_INPUT_PORT], &hTObj->pInBufHeader,
	    sizeof(hTObj->pInBufHeader), &actualSize, 0);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	eError = JPEGD_pendOnSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Wait till output  buffer is processed */
	eError = JPEGD_pendOnSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

    /***************************************************************************************/

	/* allocate 1 buffers from the DSS driver */
	if (jpegdec_prev == 1)
		getDSSBuffers(2, hTObj->pOutBufHeader->pBuffer,
		    hTObj->pOutBufHeader->nFilledLen, unOpImageWidth,
		    unOpImageHeight);
	opfile =
	    fopen((const char *)omx_jpegd_output_file_path_and_name, "wb");
	if (opfile == NULL)
	{
		JPEGD_Trace("Error opening the file %s",
		    omx_jpegd_output_file_path_and_name);
		printf("\n P Error in opening output file ");
		goto EXIT;
	}

	fwrite(hTObj->pOutBufHeader->pBuffer, 1,
	    hTObj->pOutBufHeader->nFilledLen, opfile);
	fclose(opfile);
	if (jpegdec_prev == 1)
		close(vid1_fd);

	if (hTObj->pInBufHeader)
	{
#ifdef OMX_LINUX_TILERTEST
		MemMgr_Free(hTObj->pInBufHeader->pBuffer);
		printf("\nBACK FROM INPUT BUFFER mmgr_free call \n");
#endif

		eError = OMX_FreeBuffer(component, OMX_JPEGD_TEST_INPUT_PORT,
		    hTObj->pInBufHeader);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while Dellocating input buffer by OMX component");
	}
	/* Request the component to free up output buffers and buffer headers */


	if (hTObj->pOutBufHeader)
	{
#ifdef OMX_LINUX_TILERTEST
		MemMgr_Free(hTObj->pOutBufHeader->pBuffer);
#endif

		eError = OMX_FreeBuffer(component, OMX_JPEGD_TEST_OUTPUT_PORT,
		    hTObj->pOutBufHeader);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while Dellocating output buffer by OMX component");
	}


	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	eError = JPEGD_pendOnSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Delete input port data pipe */
	bReturnStatus =
	    TIMM_OSAL_DeletePipe(hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT]);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources,
	    "Error while deleting input pipe of test object");

	/* Create output port data pipe */
	bReturnStatus =
	    TIMM_OSAL_DeletePipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT]);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources,
	    "Error while deleting output pipe of test object");

	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Delete a dummy semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_DUMMY);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating dummy semaphore");
	JPEGD_Trace("Dummy semaphore deleted");
	printf("\n Dummy semaphore deleted");

	/* Delete an Event semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating Event semaphore");
	JPEGD_Trace("Event semaphore deleted");
	printf("\n Event semaphore deleted");

	/* Delete an EmptyThisBuffer semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating EmptyThisBuffer semaphore");
	JPEGD_Trace("EmptyThisBuffer semaphore deleted");
	printf("\n EmptyThisBuffer semaphore deleted");

	/* Delete a FillThisBuffer semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating FillThisBuffer semaphore");
	JPEGD_Trace("FillThisBuffer semaphore deleted");
	printf("\nFillThisBuffer semaphore deleted");


 /*************************************************************************************/

	/* Calling OMX_Core OMX_FreeHandle function to unload a component */
	eError = OMX_FreeHandle(component);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while unloading JPEGD component");
	JPEGD_Trace("OMX Test: OMX_FreeHandle() successful");
	printf("\n OMX Test: OMX_FreeHandle() successful");
	eError = OMX_ErrorNone;

      EXIT:
	if (eError != OMX_ErrorNone)
		testResult = OMX_RET_FAIL;


	return testResult;
}


/* ==================================================================== */
/**
* @fn JPEGD_TestSliceMode    Executes the JPEG Decoder testcases For Slice mode
*
* @param[in] uMsg         Message code.
* @param[in] pParam       pointer to Function table entry
* @param[in] paramSize    size of the Function table entry structure
*
* @return
*     OMX_RET_PASS if the test passed,
*     OMX_RET_FAIL if the test fails,or possibly other special conditions.
*
*/
/* ==================================================================== */


OMX_TestStatus JPEGD_TestSliceMode(uint32_t uMsg, void *pParam,
    uint32_t paramSize)
{
	OMX_Status status = OMX_STATUS_OK;
	OMX_TestStatus testResult = OMX_RET_PASS;
	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	OMX_CALLBACKTYPE tJpegDecoderCb;
	OMX_S32 counter;
	OMX_U32 nframeSize, nframeSizeslice, nInputBitstreamSize;
	OMX_U32 *p_in, *p_out;
	OMX_U32 unOpImageWidth, unOpImageHeight, unSliceHeight;
	OMX_COLOR_FORMATTYPE eInColorFormat, eOutColorFormat;
	OMX_JPEG_UNCOMPRESSEDMODETYPE eOutputImageMode;
	OMX_BOOL bIsApplnMemAllocatorforInBuff,
	    bIsApplnMemAllocatorforOutBuff;
	FILE *ipfile, *opfile;
	char fout_name[100], slice_ext[20];;
	OMX_S32 nreadSize;
	OMX_COMPONENTTYPE *component;
	OMX_U32 actualSize = 0, slice_idx;
	JpegDecoderTestObject *hTObj = &TObjD;
	int totalRead = 0;
	char *currPtr = NULL;
	OMX_U32 nOutputBytes;	//, unSliceDimension;

#ifdef OMX_LINUX_TILERTEST
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddrIn = NULL;
	OMX_PTR TilerAddrOut = NULL;
#endif

	JPEGD_Trace("*** Test Case : %s ***", __FUNCTION__);


	status = JPEGD_ParseTestCases(uMsg, pParam, paramSize);

	if (status != OMX_STATUS_OK)
	{
		testResult = OMX_RET_FAIL;
		goto EXIT;
	}

	unOpImageWidth = hTObj->tTestcaseParam.unOpImageWidth;
	unOpImageHeight = hTObj->tTestcaseParam.unOpImageHeight;
	unSliceHeight = hTObj->tTestcaseParam.unSliceHeight;
	eInColorFormat = hTObj->tTestcaseParam.eInColorFormat;
	eOutColorFormat = hTObj->tTestcaseParam.eOutColorFormat;
	eOutputImageMode = hTObj->tTestcaseParam.eOutputImageMode;
	bIsApplnMemAllocatorforInBuff =
	    hTObj->tTestcaseParam.bIsApplnMemAllocatorforInBuff;
	bIsApplnMemAllocatorforOutBuff =
	    hTObj->tTestcaseParam.bIsApplnMemAllocatorforOutBuff;

	/* Input file size */
	ipfile =
	    fopen((const char *)omx_jpegd_input_file_path_and_name, "rb");
	if (ipfile == NULL)
	{
		JPEGD_Trace("Error opening the file %s",
		    omx_jpegd_input_file_path_and_name);
		goto EXIT;
	} else
	{
		fseek(ipfile, 0, SEEK_END);
		nInputBitstreamSize = ftell(ipfile);	//-bitstream_start;
		fseek(ipfile, 0, SEEK_SET);
		fclose(ipfile);
	}

	nframeSize =
	    JPEGD_TEST_CalculateBufferSize(unOpImageWidth, unOpImageHeight,
	    eOutColorFormat);
	nframeSizeslice =
	    JPEGD_TEST_UCP_CalculateSliceBufferSize(unOpImageWidth,
	    unOpImageHeight, unSliceHeight, eOutColorFormat);

    /**************************************************************************************/

	/* Create a dummy semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_DUMMY);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating dummy semaphore");
	JPEGD_Trace("Dummy semaphore created");

	/* Create an Event semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating Event semaphore");
	JPEGD_Trace("Event semaphore created");

	/* Create an EmptyThisBuffer semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating EmptyThisBuffer semaphore");
	JPEGD_Trace("EmptyThisBuffer semaphore created");

	/* Create a FillThisBuffer semaphore */
	eError = JPEGD_CreateSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating FillThisBuffer semaphore");
	JPEGD_Trace("FillThisBuffer semaphore created");


    /***************************************************************************************/

	/* Set call back functions */
	tJpegDecoderCb.EmptyBufferDone = OMX_JPEGD_TEST_EmptyBufferDone;
	tJpegDecoderCb.EventHandler = OMX_JPEGD_TEST_EventHandler;
	tJpegDecoderCb.FillBufferDone = OMX_JPEGD_TEST_FillBufferDone;

	/* Load the OMX_JPG_DEC Component */
	hTObj->hOMXJPEGD = NULL;
	eError = OMX_GetHandle(&hTObj->hOMXJPEGD, strJPEGD, hTObj,
	    &tJpegDecoderCb);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while obtaining component handle");
	JPEGD_Trace("OMX_GetHandle successful");

	/* Handle of the component to be accessed */
	component = (OMX_COMPONENTTYPE *) hTObj->hOMXJPEGD;

	/*Initialize size and version information for all structures */
	JPEGD_STRUCT_INIT(hTObj->tJpegdecPortInit, OMX_PORT_PARAM_TYPE);
	JPEGD_STRUCT_INIT(hTObj->tInputPortDefnType,
	    OMX_PARAM_PORTDEFINITIONTYPE);
	JPEGD_STRUCT_INIT(hTObj->tOutputPortDefnType,
	    OMX_PARAM_PORTDEFINITIONTYPE);
	JPEGD_STRUCT_INIT(hTObj->tUncompressedMode,
	    OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE);
	JPEGD_STRUCT_INIT(hTObj->pSubRegionDecode,
	    OMX_IMAGE_PARAM_DECODE_SUBREGION);

    /***************************************************************************************/

	/*  Test Set Parameters OMX_PORT_PARAM_TYPE */
	hTObj->tJpegdecPortInit.nPorts = 2;
	hTObj->tJpegdecPortInit.nStartPortNumber = 0;
	eError =
	    OMX_SetParameter(component, OMX_IndexParamImageInit,
	    &hTObj->tJpegdecPortInit);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PORT_PARAM_TYPE ");
	JPEGD_Trace("OMX_SetParameter image param done");

	/* Test Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for input port */
	hTObj->tInputPortDefnType.nPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->tInputPortDefnType.eDir = OMX_DirInput;
	hTObj->tInputPortDefnType.nBufferCountActual = 1;
	hTObj->tInputPortDefnType.nBufferCountMin = 1;
	hTObj->tInputPortDefnType.nBufferSize = nInputBitstreamSize;
	hTObj->tInputPortDefnType.bEnabled = OMX_TRUE;
	hTObj->tInputPortDefnType.bPopulated = OMX_FALSE;
	hTObj->tInputPortDefnType.eDomain = OMX_PortDomainImage;
	hTObj->tInputPortDefnType.format.image.cMIMEType = "OMXJPEGD";
	hTObj->tInputPortDefnType.format.image.pNativeRender = 0;
	hTObj->tInputPortDefnType.format.image.nFrameWidth =
	    nInputBitstreamSize;
	hTObj->tInputPortDefnType.format.image.nFrameHeight = 1;
	hTObj->tInputPortDefnType.format.image.nStride = 1;
	hTObj->tInputPortDefnType.format.image.nSliceHeight = 1;
	hTObj->tInputPortDefnType.format.image.bFlagErrorConcealment =
	    OMX_FALSE;
	hTObj->tInputPortDefnType.format.image.eCompressionFormat =
	    OMX_IMAGE_CodingJPEG;
	hTObj->tInputPortDefnType.format.image.eColorFormat = eInColorFormat;
	hTObj->tInputPortDefnType.format.image.pNativeWindow = 0x0;
	hTObj->tInputPortDefnType.bBuffersContiguous = OMX_FALSE;
	hTObj->tInputPortDefnType.nBufferAlignment = 0;

	eError = OMX_SetParameter(component, OMX_IndexParamPortDefinition,
	    &hTObj->tInputPortDefnType);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for input port");
	JPEGD_Trace("OMX_SetParameter input port defn done successfully");


	/* Test Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for output port */
	hTObj->tOutputPortDefnType.nPortIndex = OMX_JPEGD_TEST_OUTPUT_PORT;
	hTObj->tOutputPortDefnType.eDir = OMX_DirOutput;
	hTObj->tOutputPortDefnType.nBufferCountActual = 1;
	hTObj->tOutputPortDefnType.nBufferCountMin = 1;
	hTObj->tOutputPortDefnType.nBufferSize = nframeSizeslice;
	hTObj->tOutputPortDefnType.bEnabled = OMX_TRUE;
	hTObj->tInputPortDefnType.bPopulated = OMX_FALSE;
	hTObj->tOutputPortDefnType.eDomain = OMX_PortDomainImage;
	hTObj->tOutputPortDefnType.format.image.cMIMEType = "OMXJPEGD";
	hTObj->tOutputPortDefnType.format.image.pNativeRender = 0;
	hTObj->tOutputPortDefnType.format.image.nFrameWidth = unOpImageWidth;
	hTObj->tOutputPortDefnType.format.image.nFrameHeight = unSliceHeight;
	hTObj->tOutputPortDefnType.format.image.nStride = unOpImageWidth;
	hTObj->tOutputPortDefnType.format.image.nSliceHeight = unSliceHeight;
	hTObj->tOutputPortDefnType.format.image.bFlagErrorConcealment =
	    OMX_FALSE;
	hTObj->tOutputPortDefnType.format.image.eCompressionFormat =
	    OMX_IMAGE_CodingUnused;
	hTObj->tOutputPortDefnType.format.image.eColorFormat =
	    eOutColorFormat;
	hTObj->tOutputPortDefnType.format.image.pNativeWindow = 0x0;
	hTObj->tOutputPortDefnType.bBuffersContiguous = OMX_FALSE;
	hTObj->tOutputPortDefnType.nBufferAlignment = 64;

	eError =
	    OMX_SetParameter(hTObj->hOMXJPEGD, OMX_IndexParamPortDefinition,
	    &hTObj->tOutputPortDefnType);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_PARAM_PORTDEFINITIONTYPE for output port");
	JPEGD_Trace("OMX_SetParameter output port defn done successfully");


	/* Test Set Parameteres OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE */
	hTObj->tUncompressedMode.nPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->tUncompressedMode.eUncompressedImageMode = eOutputImageMode;

	eError = OMX_SetParameter(hTObj->hOMXJPEGD,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamJPEGUncompressedMode,
	    &hTObj->tUncompressedMode);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE");
	JPEGD_Trace
	    ("OMX_SetParameter input image uncompressed mode done successfully");

	/* Set Parameteres OMX_IMAGE_PARAM_DECODE_SUBREGION   */
	eError = OMX_SetParameter(component,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamDecodeSubregion,
	    &hTObj->pSubRegionDecode);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in Set Parameters OMX_IMAGE_PARAM_DECODE_SUBREGION for sub region decode");
	JPEGD_Trace("OMX_SetParameter sub region decode done successfully");


	/*if(hTObj->tOutputPortDefnType.format.image.nSliceHeight!=0)
	   {
	   unSliceDimension = unOpImageWidth * unSliceHeight * 1.5;
	   }
	   else
	   {
	   unSliceDimension = unOpImageWidth *unOpImageHeight * 1.5;
	   } */

	/* Create data pipe for input port */
	bReturnStatus =
	    TIMM_OSAL_CreatePipe(&hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT],
	    OMX_JPEGD_TEST_PIPE_SIZE, OMX_JPEGD_TEST_PIPE_MSG_SIZE, 1);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorContentPipeCreationFailed,
	    "Error while creating input pipe for JPEGD test object");

	/* Create data pipe for output port */
	bReturnStatus =
	    TIMM_OSAL_CreatePipe(&hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], OMX_JPEGD_TEST_PIPE_SIZE,
	    OMX_JPEGD_TEST_PIPE_MSG_SIZE, 1);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorContentPipeCreationFailed,
	    "Error while creating output pipe for JPEGD test object");


    /**************************************************************************************/

	/* Move to idle state */
	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while issueing Idle command");

#ifdef OMX_LINUX_TILERTEST
	printf("\n TILER BUFFERS \n");
	MemReqDescTiler =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	if (!MemReqDescTiler)
		printf
		    ("\nD can't allocate memory for Tiler block allocation \n");
#endif



	if (bIsApplnMemAllocatorforInBuff == OMX_TRUE)
	{
#ifdef OMX_LINUX_TILERTEST
		MemReqDescTiler[0].pixelFormat = TILFMT_PAGE;
		MemReqDescTiler[0].dim.len = nInputBitstreamSize;
		MemReqDescTiler[0].stride = 0;

		printf
		    ("\nBefore tiler alloc for the Codec Internal buffer %d\n",
		    TilerAddrIn);
		TilerAddrIn = MemMgr_Alloc(MemReqDescTiler, 1);
		printf("\nTiler buffer allocated is %x\n", TilerAddrIn);
		p_in = (OMX_U32 *) TilerAddrIn;

#else
		/* Request the component to allocate input buffer and buffer header */
		p_in =
		    (OMX_U32 *) TIMM_OSAL_MallocExtn(nInputBitstreamSize,
		    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT, NULL);
		JPEGD_ASSERT(NULL != p_in, eError,
		    "Error while allocating input buffer by OMX component");
#endif

		eError =
		    OMX_UseBuffer(component, &(hTObj->pInBufHeader),
		    OMX_JPEGD_TEST_INPUT_PORT, NULL, nInputBitstreamSize,
		    (OMX_U8 *) p_in);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating input buffer by OMX client");
	} else
	{
		eError = OMX_AllocateBuffer(component, &hTObj->pInBufHeader,
		    OMX_JPEGD_TEST_INPUT_PORT, NULL, nInputBitstreamSize);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating input buffer by OMX component");
	}

	if (bIsApplnMemAllocatorforOutBuff == OMX_TRUE)
	{
#ifdef OMX_LINUX_TILERTEST
		MemReqDescTiler[0].pixelFormat = TILFMT_PAGE;
		MemReqDescTiler[0].dim.len = nframeSize;
		MemReqDescTiler[0].stride = 0;

		printf
		    ("\nBefore tiler alloc for the Codec Internal buffer %d\n",
		    TilerAddrOut);
		TilerAddrOut = MemMgr_Alloc(MemReqDescTiler, 1);
		printf("\nTiler buffer allocated is %x\n", TilerAddrOut);
		p_out = (OMX_U32 *) TilerAddrOut;

#else

		/* Request the component to allocate output buffer and buffer header */
		p_out =
		    (OMX_U32 *) TIMM_OSAL_MallocExtn(nframeSizeslice,
		    TIMM_OSAL_TRUE, 64, TIMMOSAL_MEM_SEGMENT_EXT, NULL);
		JPEGD_ASSERT(NULL != p_out, eError,
		    "Error while allocating output buffer by OMX component");
#endif
		eError =
		    OMX_UseBuffer(component, &(hTObj->pOutBufHeader),
		    OMX_JPEGD_TEST_OUTPUT_PORT, NULL, nframeSizeslice,
		    (OMX_U8 *) p_out);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating output buffer by OMX client");
	} else
	{
		eError = OMX_AllocateBuffer(component, &hTObj->pInBufHeader,
		    OMX_JPEGD_TEST_OUTPUT_PORT, NULL, nframeSize);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while allocating output buffer by OMX component");
	}

	hTObj->pOutBufHeader->nAllocLen = nframeSizeslice;

    /*************************************************************************************/

	/*Move to Executing state */
	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet,
	    OMX_StateExecuting, NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	ipfile =
	    fopen((const char *)omx_jpegd_input_file_path_and_name, "rb");
	if (ipfile == NULL)
	{
		JPEGD_Trace("Error opening the file %s",
		    omx_jpegd_input_file_path_and_name);
		goto EXIT;
	}

	TIMM_OSAL_Memset(hTObj->pInBufHeader->pBuffer, 0,
	    nInputBitstreamSize);

	currPtr = (char *)hTObj->pInBufHeader->pBuffer;
	do
	{
		nreadSize = -1;
		nreadSize = fread(currPtr, 1, nInputBitstreamSize, ipfile);
		if (feof(ipfile))
		{
			break;
		}
		totalRead += nreadSize;
		currPtr += nreadSize;
	}
	while (totalRead < nInputBitstreamSize);
	fclose(ipfile);


	/* Write the input buffer info to the input data pipe */
	bReturnStatus =
	    TIMM_OSAL_WriteToPipe(hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT],
	    &hTObj->pInBufHeader, sizeof(hTObj->pInBufHeader),
	    TIMM_OSAL_SUSPEND);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	/* Write the output buffer info to the output data pipe */
	bReturnStatus =
	    TIMM_OSAL_WriteToPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], &hTObj->pOutBufHeader,
	    sizeof(hTObj->pOutBufHeader), TIMM_OSAL_SUSPEND);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");


    /********************************************************************************************/

	/* Send input and output buffers to the component for processing */
	bReturnStatus =
	    TIMM_OSAL_ReadFromPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_INPUT_PORT], &hTObj->pInBufHeader,
	    sizeof(hTObj->pInBufHeader), &actualSize, 0);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	hTObj->pInBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
	hTObj->pInBufHeader->nInputPortIndex = OMX_JPEGD_TEST_INPUT_PORT;
	hTObj->pInBufHeader->nAllocLen = nreadSize;
	hTObj->pInBufHeader->nFilledLen = hTObj->pInBufHeader->nAllocLen;
	hTObj->pInBufHeader->nOffset = 0;

	eError = component->EmptyThisBuffer(component, hTObj->pInBufHeader);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	eError = JPEGD_pendOnSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	bReturnStatus =
	    TIMM_OSAL_ReadFromPipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT], &hTObj->pOutBufHeader,
	    sizeof(hTObj->pOutBufHeader), &actualSize, 0);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "");

	hTObj->pOutBufHeader->nOutputPortIndex = OMX_JPEGD_TEST_OUTPUT_PORT;
	TIMM_OSAL_Memset(hTObj->pOutBufHeader->pBuffer, 0,
	    hTObj->pOutBufHeader->nAllocLen);

	counter = 0;
	nOutputBytes = 0;

#if 0
	/* Clean already existing output file */
	opfile =
	    fopen((const char *)omx_jpegd_output_file_path_and_name, "w+b");
	if (opfile == NULL)
	{
		JPEGD_Trace("Error opening the output file %s\r",
		    omx_jpegd_output_file_path_and_name);
		goto EXIT;
	}
	fclose(opfile);
#endif

	slice_idx = 0;
	while (nOutputBytes < nframeSize)	// here loop over total buffer size
	{
		counter++;
		JPEGD_Trace("Slice Number: %d\r", counter);
		strcpy(fout_name,
		    (char *)omx_jpegd_output_file_path_and_name);
		if (nframeSizeslice > 0)
		{
			slice_idx++;
			eError =
			    component->FillThisBuffer(component,
			    hTObj->pOutBufHeader);
			JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

			/* Wait till output  buffer is processed */
			eError = JPEGD_pendOnSemaphore(SEMTYPE_FTB);
			JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");
			nOutputBytes += nframeSizeslice;
			JPEGD_Trace
			    ("Semaphores and no of bytes compleated %d\r",
			    nOutputBytes);

			sprintf(slice_ext, "_%d", slice_idx);
			strcat(fout_name, slice_ext);

			opfile = fopen(fout_name, "wb");
			if (opfile == NULL)
			{
				JPEGD_Trace
				    ("Error opening the output file %s\r",
				    fout_name);
				goto EXIT;
			}

			fwrite(hTObj->pOutBufHeader->pBuffer, 1,
			    hTObj->pOutBufHeader->nFilledLen, opfile);
			fclose(opfile);
		}
	}

    /***************************************************************************************/

	/* Request the component to free up input  buffers and buffer headers */
	if (hTObj->pInBufHeader)
	{
#ifdef OMX_LINUX_TILERTEST
		MemMgr_Free(hTObj->pInBufHeader->pBuffer);
		printf("\nBACK FROM INPUT BUFFER mmgr_free call \n");
#endif

		eError = OMX_FreeBuffer(component, OMX_JPEGD_TEST_INPUT_PORT,
		    hTObj->pInBufHeader);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while Dellocating input buffer by OMX component");
	}
	/* Request the component to free up output buffers and buffer headers */
	if (hTObj->pOutBufHeader)
	{
#ifdef OMX_LINUX_TILERTEST
		MemMgr_Free(hTObj->pOutBufHeader->pBuffer);
#endif

		eError = OMX_FreeBuffer(component, OMX_JPEGD_TEST_OUTPUT_PORT,
		    hTObj->pOutBufHeader);
		JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
		    "Error while Dellocating output buffer by OMX component");
	}
	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	eError = JPEGD_pendOnSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Delete input port data pipe */
	bReturnStatus =
	    TIMM_OSAL_DeletePipe(hTObj->dataPipes[OMX_JPEGD_TEST_INPUT_PORT]);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources,
	    "Error while deleting input pipe of test object");

	/* Create output port data pipe */
	bReturnStatus =
	    TIMM_OSAL_DeletePipe(hTObj->
	    dataPipes[OMX_JPEGD_TEST_OUTPUT_PORT]);
	JPEGD_REQUIRE(bReturnStatus == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources,
	    "Error while deleting output pipe of test object");

	eError =
	    OMX_SendCommand(component, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError, "");

	/* Delete a dummy semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_DUMMY);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating dummy semaphore");
	JPEGD_Trace("Dummy semaphore deleted");

	/* Delete an Event semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_EVENT);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating Event semaphore");
	JPEGD_Trace("Event semaphore deleted");

	/* Delete an EmptyThisBuffer semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_ETB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating EmptyThisBuffer semaphore");
	JPEGD_Trace("EmptyThisBuffer semaphore deleted");

	/* Delete a FillThisBuffer semaphore */
	eError = JPEGD_DeleteSemaphore(SEMTYPE_FTB);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error in creating FillThisBuffer semaphore");
	JPEGD_Trace("FillThisBuffer semaphore deleted");

	/* Free input and output buffer pointers */
/*    if(bIsApplnMemAllocatorforInBuff==OMX_TRUE && p_in != NULL)
    {
        TIMM_OSAL_Free(p_in);
    }
    if(bIsApplnMemAllocatorforOutBuff==OMX_TRUE && p_out != NULL)
    {
        TIMM_OSAL_Free(p_out);
    }*/

    /*************************************************************************************/

	/* Calling OMX_Core OMX_FreeHandle function to unload a component */
	eError = OMX_FreeHandle(component);
	JPEGD_ASSERT(eError == OMX_ErrorNone, eError,
	    "Error while unloading JPEGD component");
	JPEGD_Trace("OMX Test: OMX_FreeHandle() successful");
	eError = OMX_ErrorNone;

      EXIT:
	if (eError != OMX_ErrorNone)
		testResult = OMX_RET_FAIL;


	return testResult;

}
