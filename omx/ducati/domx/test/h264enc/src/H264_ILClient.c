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

/*
*   @file  H264D_ILClient.c
*   This file contains the structures and macros that are used in the Application which tests the OpenMAX component
*
*  @path \WTSD_DucatiMMSW\omx\khronos1.1\omx_h264_dec\test
*
*  @rev 1.0
*/

/****************************************************************
 * INCLUDE FILES
 ****************************************************************/
/**----- system and platform files ----------------------------**/





/**-------program files ----------------------------------------**/

#include "H264_ILClient.h"
#ifdef OMX_TILERTEST
#define OMX_H264E_TILERTEST
#ifdef H264_LINUX_CLIENT
#define OMX_H264E_LINUX_TILERTEST
#else
#define OMX_H264E_DUCATIAPPM3_TILERTEST
#endif
#else
#ifdef H264_LINUX_CLIENT
		/*Enable the corresponding flags */
		/*HeapBuf & Shared Region Defines */
#define OMX_H264E_SRCHANGES
#define OMX_H264E_BUF_HEAP
#endif
#define OMX_H264E_NONTILERTEST

#endif

#ifdef OMX_H264E_BUF_HEAP
#include <unistd.h>
#include <RcmClient.h>
#include <HeapBuf.h>
#include <SharedRegion.h>
#endif

#ifdef OMX_H264E_LINUX_TILERTEST
/*Tiler APIs*/
#include <memmgr.h>

#endif

#ifdef OMX_H264E_BUF_HEAP
extern HeapBuf_Handle heapHandle;
#endif

#ifdef OMX_H264E_DUCATIAPPM3_TILERTEST
#define STRIDE_8BIT (16 * 1024)
#define STRIDE_16BIT (32 * 1024)
#endif


#ifdef OMX_H264E_LINUX_TILERTEST
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)
#endif


#define H264_INPUT_READY	1
#define H264_OUTPUT_READY	2
#define H264_ERROR_EVENT	4
#define H264_EOS_EVENT 8
#define H264_STATETRANSITION_COMPLETE 16
#define H264_APP_INPUTPORT 0
#define H264_APP_OUTPUTPORT 1

#define NUMTIMESTOPAUSE 1

#define H264_INPUT_BUFFERCOUNTACTUAL 5
#define H264_OUTPUT_BUFFERCOUNTACTUAL 3
#define GETHANDLE_FREEHANDLE_LOOPCOUNT 3

#define MAXFRAMESTOREADFROMFILE 1000

/*test Parameters */
extern H264E_TestCaseParams H264_TestCaseTable[1];
extern H264E_TestCaseParamsAdvanced H264_TestCaseAdvTable[1];
extern H264E_TestCaseParamsDynamic H264_TestCaseDynTable[1];

/*Globals*/
static TIMM_OSAL_PTR H264VE_Events;
static TIMM_OSAL_PTR H264VE_CmdEvent;


static OMX_U32 nFramesRead = 0;	/*Frames read from the input file */
static OMX_U32 TempCount = 0;	/* loop count to test Pause->Exectuing */

static OMX_U32 OutputFilesize = 0;
static OMX_U32 H264ClientStopFrameNum;


/*Global Settings*/
static OMX_U8 DynamicSettingsCount = 0;
static OMX_U8 DynamicSettingsArray[9];
static OMX_U8 DynFrameCountArray_Index[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static OMX_U8 PauseFrameNum[NUMTIMESTOPAUSE] = { 5 };

#ifdef TIMESTAMPSPRINT
static OMX_S64 TimeStampsArray[20] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static OMX_U8 nFillBufferDoneCount = 0;
#endif
/*-------------------------function prototypes ------------------------------*/

/*Resource allocation & Free Functions*/
static OMX_ERRORTYPE H264ENC_FreeResources(H264E_ILClient * pAppData);
static OMX_ERRORTYPE H264ENC_AllocateResources(H264E_ILClient * pAppData);

/*Fill Data into the Buffer from the File for the ETB calls*/
OMX_ERRORTYPE H264ENC_FillData(H264E_ILClient * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf);

/*Static Configurations*/
static OMX_ERRORTYPE H264ENC_SetAllParams(H264E_ILClient * pAppData,
    OMX_U32 test_index);
OMX_ERRORTYPE H264ENC_SetAdvancedParams(H264E_ILClient * pAppData,
    OMX_U32 test_index);
OMX_ERRORTYPE H264ENC_SetParamsFromInitialDynamicParams(H264E_ILClient *
    pAppData, OMX_U32 test_index);

/*Runtime Configurations*/
OMX_ERRORTYPE OMXH264Enc_ConfigureRunTimeSettings(H264E_ILClient *
    pApplicationData, OMX_H264E_DynamicConfigs * H264EConfigStructures,
    OMX_U32 test_index, OMX_U32 FrameNumber);
OMX_ERRORTYPE H264E_Populate_BitFieldSettings(OMX_U32 BitFileds,
    OMX_U8 * Count, OMX_U8 * Array);
OMX_ERRORTYPE H264E_Update_DynamicSettingsArray(OMX_U8 RemCount,
    OMX_U32 * RemoveArray);


/*Wait for state transition complete*/
static OMX_ERRORTYPE H264ENC_WaitForState(H264E_ILClient * pAppData);

/*Call Back Functions*/
OMX_ERRORTYPE H264ENC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData);

OMX_ERRORTYPE H264ENC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);

OMX_ERRORTYPE H264ENC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);

/*Error Handling*/
void OMXH264Enc_Client_ErrorHandling(H264E_ILClient * pApplicationData,
    OMX_U32 nErrorIndex, OMX_ERRORTYPE eGenError);
OMX_STRING H264_GetErrorString(OMX_ERRORTYPE error);

#ifndef H264_LINUX_CLIENT
/*Main Entry Function*/
void OMXH264Enc_TestEntry(Int32 paramSize, void *pParam);
#endif
/*Complete Functionality Implementation*/
OMX_ERRORTYPE OMXH264Enc_CompleteFunctionality(H264E_ILClient *
    pApplicationData, OMX_U32 test_index);

/*In Executing Loop Functionality*/
OMX_ERRORTYPE OMXH264Enc_InExecuting(H264E_ILClient * pApplicationData,
    OMX_U32 test_index);

/*Dynamic Port Configuration*/
OMX_ERRORTYPE OMXH264Enc_DynamicPortConfiguration(H264E_ILClient *
    pApplicationData, OMX_U32 nPortIndex);


/*Tiler Buffer <-> 1D*/
OMX_ERRORTYPE OMXH264_Util_Memcpy_1Dto2D(OMX_PTR pDst2D, OMX_PTR pSrc1D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D);
OMX_ERRORTYPE OMXH264_Util_Memcpy_2Dto1D(OMX_PTR pDst1D, OMX_PTR pSrc2D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D);

/*additional Testcases*/
OMX_ERRORTYPE OMXH264Enc_GetHandle_FreeHandle(H264E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXH264Enc_LoadedToIdlePortDisable(H264E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXH264Enc_LoadedToIdlePortEnable(H264E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXH264Enc_LessthanMinBufferRequirementsNum(H264E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXH264Enc_SizeLessthanMinBufferRequirements(H264E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXH264Enc_UnalignedBuffers(H264E_ILClient * pApplicationData);


static OMX_ERRORTYPE H264ENC_FreeResources(H264E_ILClient * pAppData)
{
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U32 i, j;

	GOTO_EXIT_IF(!pAppData, OMX_ErrorBadParameter);

	H264CLIENT_ENTER_PRINT();

	pHandle = pAppData->pHandle;
	/*Initialize the structure */
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);

	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	for (i = 0; i < tPortParams.nPorts; i++)
	{

		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (i == H264_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
#ifdef OMX_H264E_BUF_HEAP
					HeapBuf_free(heapHandle,
					    pAppData->pInBuff[j]->pBuffer,
					    pAppData->pInBuff[j]->nAllocLen);
#elif defined (OMX_H264E_LINUX_TILERTEST)
					/*Free the TilerBuffer */
					MemMgr_Free(pAppData->pInBuff[j]->
					    pBuffer);

#else
					TIMM_OSAL_Free(pAppData->pInBuff[j]->
					    pBuffer);
#endif
				}
				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pInBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		} else if (i == H264_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
#ifdef OMX_H264E_BUF_HEAP
					HeapBuf_free(heapHandle,
					    pAppData->pOutBuff[j]->pBuffer,
					    pAppData->pOutBuff[j]->nAllocLen);
#elif defined (OMX_H264E_LINUX_TILERTEST)
					/*Free the TilerBuffer */
					MemMgr_Free(pAppData->pOutBuff[j]->
					    pBuffer);
#else
					TIMM_OSAL_Free(pAppData->pOutBuff[j]->
					    pBuffer);
#endif
				}

				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pOutBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		}
	}

	if (pAppData->IpBuf_Pipe)
	{
		retval = TIMM_OSAL_DeletePipe(pAppData->IpBuf_Pipe);
		GOTO_EXIT_IF((retval != 0), OMX_ErrorNotReady);
	}

	if (pAppData->OpBuf_Pipe)
	{
		retval = TIMM_OSAL_DeletePipe(pAppData->OpBuf_Pipe);
		GOTO_EXIT_IF((retval != 0), OMX_ErrorNotReady);
	}
	if (pAppData->pInBuff)
		TIMM_OSAL_Free(pAppData->pInBuff);

	if (pAppData->pOutBuff)
		TIMM_OSAL_Free(pAppData->pOutBuff);


      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}


static OMX_ERRORTYPE H264ENC_AllocateResources(H264E_ILClient * pAppData)
{
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U8 *pTmpBuffer;	/*Used with Use Buffer calls */
	OMX_U32 i, j;

#ifdef OMX_H264E_LINUX_TILERTEST
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;
#endif
	H264CLIENT_ENTER_PRINT();
	GOTO_EXIT_IF(!pAppData, OMX_ErrorBadParameter);

	pHandle = pAppData->pHandle;
#ifdef OMX_H264E_LINUX_TILERTEST
	MemReqDescTiler =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	GOTO_EXIT_IF((MemReqDescTiler == TIMM_OSAL_NULL),
	    OMX_ErrorInsufficientResources);
#endif
	/*Initialize the structure */
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (tPortDef.nPortIndex == H264_APP_INPUTPORT)
		{
			pAppData->pInBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pInBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/* Create a pipes for Input Buffers.. used to queue data from the callback. */
			H264CLIENT_TRACE_PRINT("\nCreating i/p pipe\n");
			retval =
			    TIMM_OSAL_CreatePipe(&(pAppData->IpBuf_Pipe),
			    sizeof(OMX_BUFFERHEADERTYPE *) *
			    tPortDef.nBufferCountActual,
			    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorInsufficientResources);

			/*allocate/Use buffer calls for input */
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					/*Min Buffer requirements depends on the Amount of Stride.....here No padding so Width==Stride */
					eError =
					    OMX_AllocateBuffer(pAppData->
					    pHandle, &(pAppData->pInBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    ((pAppData->H264_TestCaseParams->
						    width *
						    pAppData->
						    H264_TestCaseParams->
						    height * 3 / 2)));
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);

					retval =
					    TIMM_OSAL_WriteToPipe(pAppData->
					    IpBuf_Pipe,
					    &(pAppData->pInBuff[j]),
					    sizeof(pAppData->pInBuff[j]),
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorInsufficientResources);
				} else
				{
					/*USE Buffer calls */
#ifdef OMX_H264E_BUF_HEAP
					pTmpBuffer =
					    HeapBuf_alloc(heapHandle,
					    (pAppData->H264_TestCaseParams->
						width *
						pAppData->
						H264_TestCaseParams->height *
						3 / 2), 0);

#elif defined (OMX_H264E_LINUX_TILERTEST)
					MemReqDescTiler[0].pixelFormat =
					    PIXEL_FMT_8BIT;
					MemReqDescTiler[0].dim.area.width = pAppData->H264_TestCaseParams->width;	/*width */
					MemReqDescTiler[0].dim.area.height = pAppData->H264_TestCaseParams->height;	/*height */
					MemReqDescTiler[0].stride =
					    STRIDE_8BIT;

					MemReqDescTiler[1].pixelFormat =
					    PIXEL_FMT_16BIT;
					MemReqDescTiler[1].dim.area.width = pAppData->H264_TestCaseParams->width / 2;	/*width */
					MemReqDescTiler[1].dim.area.height = pAppData->H264_TestCaseParams->height / 2;	/*height */
					MemReqDescTiler[1].stride =
					    STRIDE_16BIT;

					//MemReqDescTiler.reserved
					/*call to tiler Alloc */
					H264CLIENT_TRACE_PRINT
					    ("\nBefore tiler alloc for the Input buffer %d\n");
					TilerAddr =
					    MemMgr_Alloc(MemReqDescTiler, 2);
					H264CLIENT_TRACE_PRINT
					    ("\nTiler buffer allocated is %x\n",
					    TilerAddr);
					pTmpBuffer = (OMX_U8 *) TilerAddr;

#else
					pTmpBuffer =
					    TIMM_OSAL_Malloc(((pAppData->
						    H264_TestCaseParams->
						    width *
						    pAppData->
						    H264_TestCaseParams->
						    height * 3 / 2)),
					    TIMM_OSAL_TRUE, 0,
					    TIMMOSAL_MEM_SEGMENT_EXT);
#endif
					GOTO_EXIT_IF((pTmpBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer before UB = %x\n",
					    pTmpBuffer);
					pTmpBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pTmpBuffer,
					    2);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR before UB = %x\n",
					    pTmpBuffer);
#endif
					GOTO_EXIT_IF((pTmpBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);
					eError =
					    OMX_UseBuffer(pAppData->pHandle,
					    &(pAppData->pInBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    ((pAppData->H264_TestCaseParams->
						    width *
						    pAppData->
						    H264_TestCaseParams->
						    height * 3 / 2)),
					    pTmpBuffer);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR after UB = %x\n",
					    pAppData->pInBuff[j]->pBuffer);
					pAppData->pInBuff[j]->pBuffer =
					    SharedRegion_getPtr(pAppData->
					    pInBuff[j]->pBuffer);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer after UB = %x\n",
					    pAppData->pInBuff[j]->pBuffer);
#endif
					H264CLIENT_TRACE_PRINT
					    ("\nWriting to i/p pipe\n");

					retval =
					    TIMM_OSAL_WriteToPipe(pAppData->
					    IpBuf_Pipe,
					    &(pAppData->pInBuff[j]),
					    sizeof(pAppData->pInBuff[j]),
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorInsufficientResources);

				}

			}

			H264CLIENT_TRACE_PRINT("\nSetting i/p ready event\n");
			retval =
			    TIMM_OSAL_EventSet(H264VE_Events,
			    H264_INPUT_READY, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			H264CLIENT_TRACE_PRINT("\ni/p ready event set\n");
		} else if (tPortDef.nPortIndex == H264_APP_OUTPUTPORT)
		{

			pAppData->pOutBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pOutBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/* Create a pipes for Output Buffers.. used to queue data from the callback. */
			H264CLIENT_TRACE_PRINT("\nCreating o/p pipe\n");
			retval =
			    TIMM_OSAL_CreatePipe(&(pAppData->OpBuf_Pipe),
			    sizeof(OMX_BUFFERHEADERTYPE *) *
			    tPortDef.nBufferCountActual,
			    sizeof(OMX_BUFFERHEADERTYPE *), OMX_TRUE);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorInsufficientResources);
			/*allocate buffers for output */
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (pAppData->H264_TestCaseParams->
				    bOutAllocatebuffer)
				{
					/*Min Buffer requirements depends on the Amount of Stride.....here No padding so Width==Stride */
					eError =
					    OMX_AllocateBuffer(pAppData->
					    pHandle, &(pAppData->pOutBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    (pAppData->H264_TestCaseParams->
						width *
						pAppData->
						H264_TestCaseParams->height));
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
					/*the output buffer nfillelen initially is zero */
					pAppData->pOutBuff[j]->nFilledLen = 0;
					retval =
					    TIMM_OSAL_WriteToPipe(pAppData->
					    OpBuf_Pipe,
					    &(pAppData->pOutBuff[j]),
					    sizeof(pAppData->pOutBuff[j]),
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorInsufficientResources);
				} else
				{
					/*USE Buffer calls */
#ifdef OMX_H264E_BUF_HEAP
					pTmpBuffer =
					    HeapBuf_alloc(heapHandle,
					    (pAppData->H264_TestCaseParams->
						width *
						pAppData->
						H264_TestCaseParams->height),
					    0);

#elif defined (OMX_H264E_LINUX_TILERTEST)
					MemReqDescTiler[0].pixelFormat =
					    PIXEL_FMT_PAGE;
					MemReqDescTiler[0].dim.len =
					    pAppData->H264_TestCaseParams->
					    width *
					    pAppData->H264_TestCaseParams->
					    height;
					MemReqDescTiler[0].stride = 0;
					//MemReqDescTiler.stride
					//MemReqDescTiler.reserved
					/*call to tiler Alloc */
					H264CLIENT_TRACE_PRINT
					    ("\nBefore tiler alloc for the Output buffer %d\n");
					TilerAddr =
					    MemMgr_Alloc(MemReqDescTiler, 1);
					H264CLIENT_TRACE_PRINT
					    ("\nTiler buffer allocated is %x\n",
					    TilerAddr);
					pTmpBuffer = (OMX_U8 *) TilerAddr;
#else
					pTmpBuffer =
					    TIMM_OSAL_Malloc(((pAppData->
						    H264_TestCaseParams->
						    width *
						    pAppData->
						    H264_TestCaseParams->
						    height)), TIMM_OSAL_TRUE,
					    0, TIMMOSAL_MEM_SEGMENT_EXT);
#endif
					GOTO_EXIT_IF((pTmpBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer before UB = %x\n",
					    pTmpBuffer);
					pTmpBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pTmpBuffer,
					    2);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR before UB = %x\n",
					    pTmpBuffer);
#endif
					H264CLIENT_TRACE_PRINT
					    ("\ncall to use buffer\n");
					eError =
					    OMX_UseBuffer(pAppData->pHandle,
					    &(pAppData->pOutBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    ((pAppData->H264_TestCaseParams->
						    width *
						    pAppData->
						    H264_TestCaseParams->
						    height)), pTmpBuffer);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR after UB = %x\n",
					    pAppData->pOutBuff[j]->pBuffer);
					pAppData->pOutBuff[j]->pBuffer =
					    SharedRegion_getPtr(pAppData->
					    pOutBuff[j]->pBuffer);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer after UB = %x\n",
					    pAppData->pOutBuff[j]->pBuffer);
#endif

					H264CLIENT_TRACE_PRINT
					    ("\nWriting to o/p pipe\n");
					retval =
					    TIMM_OSAL_WriteToPipe(pAppData->
					    OpBuf_Pipe,
					    &(pAppData->pOutBuff[j]),
					    sizeof(pAppData->pOutBuff[j]),
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorInsufficientResources);
				}

			}

			H264CLIENT_TRACE_PRINT("\nSetting o/p ready event\n");
			retval =
			    TIMM_OSAL_EventSet(H264VE_Events,
			    H264_OUTPUT_READY, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			H264CLIENT_TRACE_PRINT("\no/p ready event set\n");
		} else
		{
			H264CLIENT_TRACE_PRINT
			    ("Port index Assigned is Neither Input Nor Output");
			eError = OMX_ErrorBadPortIndex;
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}
	}
#ifdef OMX_H264E_LINUX_TILERTEST
	if (MemReqDescTiler)
		TIMM_OSAL_Free(MemReqDescTiler);

#endif
      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_STRING H264_GetErrorString(OMX_ERRORTYPE error)
{
	OMX_STRING errorString;
	H264CLIENT_ENTER_PRINT();

	switch (error)
	{
	case OMX_ErrorNone:
		errorString = "OMX_ErrorNone";
		break;
	case OMX_ErrorInsufficientResources:
		errorString = "OMX_ErrorInsufficientResources";
		break;
	case OMX_ErrorUndefined:
		errorString = "OMX_ErrorUndefined";
		break;
	case OMX_ErrorInvalidComponentName:
		errorString = "OMX_ErrorInvalidComponentName";
		break;
	case OMX_ErrorComponentNotFound:
		errorString = "OMX_ErrorComponentNotFound";
		break;
	case OMX_ErrorInvalidComponent:
		errorString = "OMX_ErrorInvalidComponent";
		break;
	case OMX_ErrorBadParameter:
		errorString = "OMX_ErrorBadParameter";
		break;
	case OMX_ErrorNotImplemented:
		errorString = "OMX_ErrorNotImplemented";
		break;
	case OMX_ErrorUnderflow:
		errorString = "OMX_ErrorUnderflow";
		break;
	case OMX_ErrorOverflow:
		errorString = "OMX_ErrorOverflow";
		break;
	case OMX_ErrorHardware:
		errorString = "OMX_ErrorHardware";
		break;
	case OMX_ErrorInvalidState:
		errorString = "OMX_ErrorInvalidState";
		break;
	case OMX_ErrorStreamCorrupt:
		errorString = "OMX_ErrorStreamCorrupt";
		break;
	case OMX_ErrorPortsNotCompatible:
		errorString = "OMX_ErrorPortsNotCompatible";
		break;
	case OMX_ErrorResourcesLost:
		errorString = "OMX_ErrorResourcesLost";
		break;
	case OMX_ErrorNoMore:
		errorString = "OMX_ErrorNoMore";
		break;
	case OMX_ErrorVersionMismatch:
		errorString = "OMX_ErrorVersionMismatch";
		break;
	case OMX_ErrorNotReady:
		errorString = "OMX_ErrorNotReady";
		break;
	case OMX_ErrorTimeout:
		errorString = "OMX_ErrorTimeout";
		break;
	default:
		errorString = "<unknown>";
	}
	return errorString;
}


OMX_ERRORTYPE H264ENC_FillData(H264E_ILClient * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf)
{
	OMX_U32 nRead = 0;
	OMX_U32 framesizetoread = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifdef OMX_H264E_TILERTEST
	OMX_U8 *pTmpBuffer = NULL, *pActualPtr = NULL;
	OMX_U32 size1D = 0;
#endif
#ifdef  OMX_H264E_LINUX_TILERTEST
	OMX_U32 Addoffset = 0;
#endif

	H264CLIENT_ENTER_PRINT();

	framesizetoread =
	    pAppData->H264_TestCaseParams->width *
	    pAppData->H264_TestCaseParams->height * 3 / 2;

	/*Tiler Interface code */
#ifdef OMX_H264E_TILERTEST
	pTmpBuffer =
	    (OMX_U8 *) TIMM_OSAL_Malloc(framesizetoread, OMX_FALSE, 0, 0);
	if (pTmpBuffer == NULL)
	{
		eError = OMX_ErrorInsufficientResources;
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}
	pActualPtr = pTmpBuffer;
	size1D = framesizetoread;
	if ((!feof(pAppData->fIn)) &&
	    ((nRead =
		    fread(pTmpBuffer, sizeof(OMX_U8), framesizetoread,
			pAppData->fIn)) != 0) &&
	    ((H264ClientStopFrameNum - 1) > nFramesRead))
	{
		nFramesRead++;
		H264CLIENT_TRACE_PRINT
		    ("number of bytes read from %d frame=%d", nFramesRead,
		    nRead);
		eError =
		    OMXH264_Util_Memcpy_1Dto2D(pBuf->pBuffer, pTmpBuffer,
		    size1D, pAppData->H264_TestCaseParams->height,
		    pAppData->H264_TestCaseParams->width, STRIDE_8BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (nRead <
		    (pAppData->H264_TestCaseParams->height *
			pAppData->H264_TestCaseParams->width))
			/*frame data< the required so need not send for processing send an EOS flag with no data */
		{
			pBuf->nFlags = OMX_BUFFERFLAG_EOS;
			pBuf->nFilledLen = 0;
			pBuf->nOffset = 0;	/*offset is Zero */
			pBuf->nTimeStamp = (OMX_S64) nFramesRead;
			goto EXIT;
		} else
		{
			pTmpBuffer +=
			    (pAppData->H264_TestCaseParams->height *
			    pAppData->H264_TestCaseParams->width);
			size1D -=
			    (pAppData->H264_TestCaseParams->height *
			    pAppData->H264_TestCaseParams->width);
		}
#ifdef OMX_H264E_DUCATIAPPM3_TILERTEST
		eError =
		    OMXH264_Util_Memcpy_1Dto2D(((OMX_TI_PLATFORMPRIVATE *)
			pBuf->pPlatformPrivate)->pAuxBuf1, pTmpBuffer, size1D,
		    (pAppData->H264_TestCaseParams->height / 2),
		    pAppData->H264_TestCaseParams->width, STRIDE_16BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif
#ifdef OMX_H264E_LINUX_TILERTEST
		Addoffset =
		    (pAppData->H264_TestCaseParams->height * STRIDE_8BIT);
		eError =
		    OMXH264_Util_Memcpy_1Dto2D((pBuf->pBuffer + Addoffset),
		    pTmpBuffer, size1D,
		    (pAppData->H264_TestCaseParams->height / 2),
		    pAppData->H264_TestCaseParams->width, STRIDE_16BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif

	} else
	{
		/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
		if (H264ClientStopFrameNum == MAXFRAMESTOREADFROMFILE)
		{
			/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
			H264ClientStopFrameNum = nFramesRead;
		}
		nFramesRead++;
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
		H264CLIENT_TRACE_PRINT
		    ("end of file reached and EOS flag has been set");
	}

#endif

#ifdef OMX_H264E_NONTILERTEST
  /**********************Original Code*************************************************/
	if ((!feof(pAppData->fIn)) &&
	    ((nRead =
		    fread(pBuf->pBuffer, sizeof(OMX_U8), framesizetoread,
			pAppData->fIn)) != 0) &&
	    ((H264ClientStopFrameNum - 1) > nFramesRead))
	{

		H264CLIENT_TRACE_PRINT
		    ("number of bytes read from %d frame=%d", nFramesRead,
		    nRead);
		nFramesRead++;

	} else
	{
		/*incase of partial record update the EOS */
		if (H264ClientStopFrameNum == MAXFRAMESTOREADFROMFILE)
		{
			/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
			H264ClientStopFrameNum = nFramesRead;

		}
		nFramesRead++;
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
		H264CLIENT_TRACE_PRINT
		    ("end of file reached and EOS flag has been set");
	}
	/**********************Original Code  ENDS*************************************************/
#endif
	pBuf->nFilledLen = nRead;
	pBuf->nOffset = 0;	/*offset is Zero */
	pBuf->nTimeStamp = (OMX_S64) nFramesRead;
      EXIT:
#ifdef OMX_H264E_TILERTEST
	if (pActualPtr != NULL)
	{
		TIMM_OSAL_Free(pActualPtr);
	}
#endif
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE H264ENC_SetAdvancedParams(H264E_ILClient * pAppData,
    OMX_U32 test_index)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_U8 AdvancedSettingsCount = 0;
	OMX_U8 AdancedSettingsArray[6];
	OMX_U8 i;
	OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE tNALUSettings;
	OMX_VIDEO_PARAM_MEBLOCKSIZETYPE tMEBlockSize;
	OMX_VIDEO_PARAM_INTRAREFRESHTYPE tIntrarefresh;
	OMX_VIDEO_PARAM_VUIINFOTYPE tVUISettings;
	OMX_VIDEO_PARAM_INTRAPREDTYPE tIntraPred;
	OMX_VIDEO_PARAM_AVCADVANCEDFMOTYPE tAdvancedFMO;
	OMX_VIDEO_PARAM_DATASYNCMODETYPE tVidDataMode;

	H264CLIENT_ENTER_PRINT();

	if (!pAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pHandle = pAppData->pHandle;

	/*Get the BitFiledSettings */
	eError =
	    H264E_Populate_BitFieldSettings(pAppData->H264_TestCaseParams->
	    nBitEnableAdvanced, &AdvancedSettingsCount, AdancedSettingsArray);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	for (i = 0; i < AdvancedSettingsCount; i++)
	{
		switch (AdancedSettingsArray[i])
		{
		case 0:
			OMX_TEST_INIT_STRUCT_PTR(&tNALUSettings,
			    OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE);
			/*Set the Port */
			tNALUSettings.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoNALUsettings,
			    &tNALUSettings);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			tNALUSettings.nStartofSequence =
			    H264_TestCaseAdvTable[test_index].NALU.
			    nStartofSequence;
			tNALUSettings.nEndofSequence =
			    H264_TestCaseAdvTable[test_index].NALU.
			    nEndofSequence;
			tNALUSettings.nIDR =
			    H264_TestCaseAdvTable[test_index].NALU.nIDR;
			tNALUSettings.nIntraPicture =
			    H264_TestCaseAdvTable[test_index].NALU.
			    nIntraPicture;
			tNALUSettings.nNonIntraPicture =
			    H264_TestCaseAdvTable[test_index].NALU.
			    nNonIntraPicture;

			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoNALUsettings,
			    &tNALUSettings);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			break;
		case 1:
			/*set param to OMX_TI_VIDEO_PARAM_FMO_ADVANCEDSETTINGS */
			if (pAppData->H264_TestCaseParams->bFMO == OMX_TRUE)
			{
				OMX_TEST_INIT_STRUCT_PTR(&tAdvancedFMO,
				    OMX_VIDEO_PARAM_AVCADVANCEDFMOTYPE);
				/*Set the Port */
				tAdvancedFMO.nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetParameter(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexParamVideoAdvancedFMO,
				    &tAdvancedFMO);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tAdvancedFMO.nNumSliceGroups =
				    H264_TestCaseAdvTable[test_index].FMO.
				    nNumSliceGroups;
				tAdvancedFMO.nSliceGroupMapType =
				    H264_TestCaseAdvTable[test_index].FMO.
				    nSliceGroupMapType;
				tAdvancedFMO.nSliceGroupChangeRate =
				    H264_TestCaseAdvTable[test_index].FMO.
				    nSliceGroupChangeRate;
				tAdvancedFMO.nSliceGroupChangeCycle =
				    H264_TestCaseAdvTable[test_index].FMO.
				    nSliceGroupChangeCycle;
				for (i = 0; i < H264ENC_MAXNUMSLCGPS; i++)
				{
					tAdvancedFMO.nSliceGroupParams[i] =
					    H264_TestCaseAdvTable[test_index].
					    FMO.nSliceGroupParams[i];
				}
				eError =
				    OMX_SetParameter(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexParamVideoAdvancedFMO,
				    &tAdvancedFMO);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			} else
			{
				H264CLIENT_ERROR_PRINT
				    ("Set the FMO flag to TRUE to take these settings into effect");
			}
			break;
		case 2:
			/*set param to OMX_TI_VIDEO_PARAM_ME_BLOCKSIZE */
			OMX_TEST_INIT_STRUCT_PTR(&tMEBlockSize,
			    OMX_VIDEO_PARAM_MEBLOCKSIZETYPE);
			/*Set the Port */
			tMEBlockSize.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoMEBlockSize,
			    &tMEBlockSize);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			tMEBlockSize.eMinBlockSizeP =
			    H264_TestCaseAdvTable[test_index].MEBlock.
			    eMinBlockSizeP;
			tMEBlockSize.eMinBlockSizeB =
			    H264_TestCaseAdvTable[test_index].MEBlock.
			    eMinBlockSizeB;

			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoMEBlockSize,
			    &tMEBlockSize);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			break;
		case 3:
			/*set param to OMX_VIDEO_PARAM_INTRAREFRESHTYPE */
			OMX_TEST_INIT_STRUCT_PTR(&tIntrarefresh,
			    OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
			/*Set the Port */
			tIntrarefresh.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    OMX_IndexParamVideoIntraRefresh, &tIntrarefresh);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			tIntrarefresh.eRefreshMode =
			    H264_TestCaseAdvTable[test_index].IntraRefresh.
			    eRefreshMode;
			if (tIntrarefresh.eRefreshMode ==
			    OMX_VIDEO_IntraRefreshBoth)
			{
				tIntrarefresh.nAirRef =
				    H264_TestCaseAdvTable[test_index].
				    IntraRefresh.nRate;
			} else if (tIntrarefresh.eRefreshMode ==
			    OMX_VIDEO_IntraRefreshCyclic)
			{
				tIntrarefresh.nCirMBs =
				    H264_TestCaseAdvTable[test_index].
				    IntraRefresh.nRate;
			} else if (tIntrarefresh.eRefreshMode ==
			    OMX_VIDEO_IntraRefreshAdaptive)
			{
				tIntrarefresh.nAirMBs =
				    H264_TestCaseAdvTable[test_index].
				    IntraRefresh.nRate;
			} else
			{
				H264CLIENT_ERROR_PRINT
				    ("Unsupported settings");
			}
			eError =
			    OMX_SetParameter(pHandle,
			    OMX_IndexParamVideoIntraRefresh, &tIntrarefresh);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			break;
		case 4:
			/*set param to OMX_VIDEO_PARAM_VUIINFOTYPE */
			OMX_TEST_INIT_STRUCT_PTR(&tVUISettings,
			    OMX_VIDEO_PARAM_VUIINFOTYPE);
			/*Set the Port */
			tVUISettings.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoVUIsettings,
			    &tVUISettings);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			tVUISettings.bAspectRatioPresent =
			    H264_TestCaseAdvTable[test_index].VUI.
			    bAspectRatioPresent;
			tVUISettings.bFullRange =
			    H264_TestCaseAdvTable[test_index].VUI.bFullRange;
			tVUISettings.ePixelAspectRatio =
			    H264_TestCaseAdvTable[test_index].VUI.
			    eAspectRatio;

			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoVUIsettings,
			    &tVUISettings);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			break;
		case 5:
			/*set param to OMX_TI_VIDEO_PARAM_INTRAPRED */
			OMX_TEST_INIT_STRUCT_PTR(&tIntraPred,
			    OMX_VIDEO_PARAM_INTRAPREDTYPE);
			/*Set the Port */
			tIntraPred.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoIntraPredictionSettings,
			    &tIntraPred);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

			tIntraPred.nLumaIntra16x16Enable =
			    H264_TestCaseAdvTable[test_index].IntraPred.
			    nLumaIntra16x16Enable;
			tIntraPred.nLumaIntra8x8Enable =
			    H264_TestCaseAdvTable[test_index].IntraPred.
			    nLumaIntra8x8Enable;
			tIntraPred.nLumaIntra4x4Enable =
			    H264_TestCaseAdvTable[test_index].IntraPred.
			    nLumaIntra4x4Enable;
			tIntraPred.nChromaIntra8x8Enable =
			    H264_TestCaseAdvTable[test_index].IntraPred.
			    nChromaIntra8x8Enable;
			tIntraPred.eChromaComponentEnable =
			    H264_TestCaseAdvTable[test_index].IntraPred.
			    eChromaComponentEnable;

			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoIntraPredictionSettings,
			    &tIntraPred);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			break;
		case 6:
			/*set param to OMX_VIDEO_PARAM_DATASYNCMODETYPE */
			/*Initialize the structure */
			OMX_TEST_INIT_STRUCT_PTR(&tVidDataMode,
			    OMX_VIDEO_PARAM_DATASYNCMODETYPE);
			/*DATA SYNC MODE Settings */
			/*Set the Port */
			tIntraPred.nPortIndex = H264_APP_INPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoDataSyncMode,
			    &tVidDataMode);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			tVidDataMode.eDataMode =
			    H264_TestCaseAdvTable[test_index].DataSync.
			    inputDataMode;
			tVidDataMode.nNumDataUnits =
			    H264_TestCaseAdvTable[test_index].DataSync.
			    numInputDataUnits;
			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoDataSyncMode,
			    &tVidDataMode);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			/*Set the Port */
			tIntraPred.nPortIndex = H264_APP_OUTPUTPORT;
			eError =
			    OMX_GetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoDataSyncMode,
			    &tVidDataMode);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			tVidDataMode.eDataMode =
			    H264_TestCaseAdvTable[test_index].DataSync.
			    outputDataMode;
			tVidDataMode.nNumDataUnits =
			    H264_TestCaseAdvTable[test_index].DataSync.
			    numOutputDataUnits;
			eError =
			    OMX_SetParameter(pHandle,
			    (OMX_INDEXTYPE)
			    OMX_TI_IndexParamVideoDataSyncMode,
			    &tVidDataMode);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			break;
		default:
			H264CLIENT_ERROR_PRINT("Invalid inputs");
		}
	}

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE H264E_Populate_BitFieldSettings(OMX_U32 BitFields,
    OMX_U8 * Count, OMX_U8 * Array)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 j, LCount = 0, Temp[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	OMX_S8 i;
	OMX_U32 Value = 0;

	H264CLIENT_ENTER_PRINT();
	Value = BitFields;
	for (i = 7; i >= 0; i--)
	{
		Temp[i] = Value / (1 << i);
		Value = Value - Temp[i] * (1 << i);
		if (i == 0)
		{
			break;
		}

	}
	for (i = 0, j = 0; i < 8; i++)
	{
		if (Temp[i])
		{
			Array[j++] = i;
			LCount++;
		}
	}
	*Count = LCount;
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE H264E_Update_DynamicSettingsArray(OMX_U8 RemCount,
    OMX_U32 * RemoveArray)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 i, j, LCount = 0, LRemcount = 0;
	OMX_U8 LocalDynArray[9], LocalDynArray_Index[9];
	OMX_BOOL bFound = OMX_FALSE;

	H264CLIENT_ENTER_PRINT();

	for (i = 0; i < DynamicSettingsCount; i++)
	{			/*the elements in the dynamic array list are always in ascending order */
		for (j = LRemcount; j < RemCount; j++)
		{		/*the elements in the remove list are also in ascending order */
			if (RemoveArray[j] == DynamicSettingsArray[i])
			{
				bFound = OMX_TRUE;
				break;
			}
		}
		if (!bFound)
		{
			LocalDynArray[LCount] = DynamicSettingsArray[i];
			LocalDynArray_Index[LCount] =
			    DynFrameCountArray_Index[i];
			LCount++;
		} else
		{
			bFound = OMX_FALSE;
			LRemcount++;
		}

	}

	/*update the DynamicSettingsArray after removing */
	DynamicSettingsCount = LCount;
	for (i = 0; i < DynamicSettingsCount; i++)
	{
		DynamicSettingsArray[i] = LocalDynArray[i];
		DynFrameCountArray_Index[i] = LocalDynArray_Index[i];
	}
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}


OMX_ERRORTYPE H264ENC_SetParamsFromInitialDynamicParams(H264E_ILClient *
    pAppData, OMX_U32 test_index)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_U32 LFrameRate, LBitRate, LIntraPeriod;
	OMX_BOOL bFramerate = OMX_FALSE, bBitrate = OMX_FALSE, bIntraperiod =
	    OMX_FALSE;
	OMX_U32 nRemoveList[9];
	OMX_U8 i, RemoveIndex = 0;

	OMX_VIDEO_CONFIG_MESEARCHRANGETYPE tMESearchrange;
	OMX_CONFIG_INTRAREFRESHVOPTYPE tForceFrame;
	OMX_VIDEO_CONFIG_QPSETTINGSTYPE tQPSettings;
	OMX_VIDEO_CONFIG_SLICECODINGTYPE tSliceCodingparams;
	OMX_VIDEO_CONFIG_NALSIZE tNALUSize;
	OMX_VIDEO_CONFIG_PIXELINFOTYPE tPixelInfo;

	OMX_VIDEO_PARAM_AVCTYPE tAVCParams;
	OMX_VIDEO_PARAM_BITRATETYPE tVidEncBitRate;
	OMX_VIDEO_PARAM_PORTFORMATTYPE tVideoParams;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;

	H264CLIENT_ENTER_PRINT();

	OMX_TEST_INIT_STRUCT_PTR(&tAVCParams, OMX_VIDEO_PARAM_AVCTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tVidEncBitRate,
	    OMX_VIDEO_PARAM_BITRATETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tVideoParams,
	    OMX_VIDEO_PARAM_PORTFORMATTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);

	if (!pAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pHandle = pAppData->pHandle;

	eError =
	    H264E_Populate_BitFieldSettings(pAppData->H264_TestCaseParams->
	    nBitEnableDynamic, &DynamicSettingsCount, DynamicSettingsArray);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	for (i = 0; i < DynamicSettingsCount; i++)
	{
		switch (DynamicSettingsArray[i])
		{
		case 0:
			/*Frame rate */
			if (H264_TestCaseDynTable[test_index].DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bFramerate = OMX_TRUE;
				LFrameRate =
				    H264_TestCaseDynTable[test_index].
				    DynFrameRate.
				    nFramerate[DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}

			break;
		case 1:
			/*Bitrate */
			if (H264_TestCaseDynTable[test_index].DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bBitrate = OMX_TRUE;
				LBitRate =
				    H264_TestCaseDynTable[test_index].
				    DynBitRate.
				    nBitrate[DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;
			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 2:
			/*set config to OMX_TI_VIDEO_CONFIG_ME_SEARCHRANGE */

			if (H264_TestCaseDynTable[test_index].
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tMESearchrange,
				    OMX_VIDEO_CONFIG_MESEARCHRANGETYPE);
				tMESearchrange.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &tMESearchrange);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tMESearchrange.eMVAccuracy =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nMVAccuracy[DynFrameCountArray_Index[i]];
				tMESearchrange.nHorSearchRangeP =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nHorSearchRangeP[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nVerSearchRangeP =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nVerSearchRangeP[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nHorSearchRangeB =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nHorSearchRangeB[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nVerSearchRangeB =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nVerSearchRangeB[DynFrameCountArray_Index
				    [i]];

				eError =
				    OMX_SetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &tMESearchrange);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 3:
			/*set config to OMX_CONFIG_INTRAREFRESHVOPTYPE */
			if (H264_TestCaseDynTable[test_index].DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tForceFrame,
				    OMX_CONFIG_INTRAREFRESHVOPTYPE);
				tForceFrame.nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &tForceFrame);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tForceFrame.IntraRefreshVOP =
				    H264_TestCaseDynTable[test_index].
				    DynForceFrame.
				    ForceIFrame[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &tForceFrame);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 4:
			/*set config to OMX_TI_VIDEO_CONFIG_QPSETTINGS */
			if (H264_TestCaseDynTable[test_index].DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tQPSettings,
				    OMX_VIDEO_CONFIG_QPSETTINGSTYPE);
				tQPSettings.nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &tQPSettings);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tQPSettings.nQpI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpP =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpOffsetB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpOffsetB[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxB[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinB[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &tQPSettings);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 5:
			/*set config to OMX_VIDEO_CONFIG_AVCINTRAPERIOD */
			if (H264_TestCaseDynTable[test_index].
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bIntraperiod = OMX_TRUE;
				LIntraPeriod =
				    H264_TestCaseDynTable[test_index].
				    DynIntraFrmaeInterval.
				    nIntraFrameInterval
				    [DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 6:
			/*set config to OMX_IndexConfigVideoNalSize */
			if (H264_TestCaseDynTable[test_index].DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				/*Set Port Index */
				OMX_TEST_INIT_STRUCT_PTR(&tNALUSize,
				    OMX_VIDEO_CONFIG_NALSIZE);
				tNALUSize.nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tNALUSize.nNaluBytes =
				    H264_TestCaseDynTable[test_index].
				    DynNALSize.
				    nNaluSize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 7:
			/*set config to OMX_TI_VIDEO_CONFIG_SLICECODING */
			if (H264_TestCaseDynTable[test_index].
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tSliceCodingparams,
				    OMX_VIDEO_CONFIG_SLICECODINGTYPE);
				tSliceCodingparams.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigSliceSettings,
				    &tSliceCodingparams);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tSliceCodingparams.eSliceMode =
				    H264_TestCaseDynTable[test_index].
				    DynSliceSettings.
				    eSliceMode[DynFrameCountArray_Index[i]];
				tSliceCodingparams.nSlicesize =
				    H264_TestCaseDynTable[test_index].
				    DynSliceSettings.
				    nSlicesize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigSliceSettings,
				    &tSliceCodingparams);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}

			break;
		case 8:
			/*set config to OMX_TI_VIDEO_CONFIG_PIXELINFO */
			if (H264_TestCaseDynTable[test_index].DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tPixelInfo,
				    OMX_VIDEO_CONFIG_PIXELINFOTYPE);
				tPixelInfo.nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &tPixelInfo);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tPixelInfo.nWidth =
				    H264_TestCaseDynTable[test_index].
				    DynPixelInfo.
				    nWidth[DynFrameCountArray_Index[i]];
				tPixelInfo.nHeight =
				    H264_TestCaseDynTable[test_index].
				    DynPixelInfo.
				    nHeight[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &tPixelInfo);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (H264_TestCaseDynTable[test_index].DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		default:
			H264CLIENT_ERROR_PRINT("Invalid inputs");
		}
	}

	/*Get the Number of Ports */
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamVideoPortFormat,
		    &tVideoParams);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		if (tPortDef.eDir == OMX_DirInput)
		{

			/*set the actual number of buffers required */
			if (pAppData->H264_TestCaseParams->TestCaseId == 6)
			{
				tPortDef.nBufferCountActual = H264_INPUT_BUFFERCOUNTACTUAL;	/*currently hard coded & can be taken from test parameters */
			} else
			{
				/*check for the valid Input buffer count that the user configured with */
				if (pAppData->H264_TestCaseParams->
				    nNumInputBuf >=
				    pAppData->H264_TestCaseParams->
				    maxInterFrameInterval)
				{
					tPortDef.nBufferCountActual =
					    pAppData->H264_TestCaseParams->
					    nNumInputBuf;
				} else
				{
					eError = OMX_ErrorUnsupportedSetting;
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
				}
			}

			/*set the video format settings */
			tPortDef.format.video.nFrameWidth =
			    pAppData->H264_TestCaseParams->width;
			tPortDef.format.video.nStride =
			    pAppData->H264_TestCaseParams->width;
			tPortDef.format.video.nFrameHeight =
			    pAppData->H264_TestCaseParams->height;
			tPortDef.format.video.eColorFormat =
			    pAppData->H264_TestCaseParams->inputChromaFormat;

			/*settings for OMX_IndexParamVideoPortFormat */
			tVideoParams.eColorFormat =
			    pAppData->H264_TestCaseParams->inputChromaFormat;

		}
		if (tPortDef.eDir == OMX_DirOutput)
		{
			/*set the actual number of buffers required */
			if (pAppData->H264_TestCaseParams->TestCaseId == 6)
			{
				tPortDef.nBufferCountActual = H264_OUTPUT_BUFFERCOUNTACTUAL;	/*currently hard coded & can be taken from test parameters */
			} else
			{
				if (pAppData->H264_TestCaseParams->
				    maxInterFrameInterval > 1)
				{
					/*check for the valid Output buffer count that the user configured with */
					if (pAppData->H264_TestCaseParams->
					    nNumOutputBuf >= 2)
					{
						tPortDef.nBufferCountActual =
						    pAppData->
						    H264_TestCaseParams->
						    nNumOutputBuf;
					} else
					{
						eError =
						    OMX_ErrorUnsupportedSetting;
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);
					}
				} else
				{
					/*check for the valid Output buffer count that the user configured with */
					if (pAppData->H264_TestCaseParams->
					    nNumOutputBuf >= 1)
					{
						tPortDef.nBufferCountActual =
						    pAppData->
						    H264_TestCaseParams->
						    nNumOutputBuf;
					} else
					{
						eError =
						    OMX_ErrorUnsupportedSetting;
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);
					}
				}

			}
			/*settings for OMX_IndexParamPortDefinition */
			tPortDef.format.video.nFrameWidth =
			    pAppData->H264_TestCaseParams->width;
			tPortDef.format.video.nStride =
			    pAppData->H264_TestCaseParams->width;
			tPortDef.format.video.nFrameHeight =
			    pAppData->H264_TestCaseParams->height;
			tPortDef.format.video.eCompressionFormat =
			    OMX_VIDEO_CodingAVC;
			if (bFramerate)
			{
				tPortDef.format.video.xFramerate =
				    (LFrameRate << 16);
				tVideoParams.xFramerate = (LFrameRate << 16);
			}
			if (bBitrate)
			{
				tPortDef.format.video.nBitrate = LBitRate;
			}
			/*settings for OMX_IndexParamVideoPortFormat */
			tVideoParams.eCompressionFormat = OMX_VIDEO_CodingAVC;
		}

		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamVideoPortFormat,
		    &tVideoParams);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		/*AVC Settings */
		tAVCParams.nPortIndex = OMX_DirOutput;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamVideoAvc,
		    &tAVCParams);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		tAVCParams.eProfile = pAppData->H264_TestCaseParams->profile;
		tAVCParams.eLevel = pAppData->H264_TestCaseParams->level;
		if (bIntraperiod == OMX_TRUE)
		{
			tAVCParams.nPFrames = LIntraPeriod;
		}
		tAVCParams.nBFrames =
		    pAppData->H264_TestCaseParams->maxInterFrameInterval - 1;
		tAVCParams.bEnableFMO = pAppData->H264_TestCaseParams->bFMO;
		tAVCParams.bEntropyCodingCABAC =
		    pAppData->H264_TestCaseParams->bCABAC;
		tAVCParams.bconstIpred =
		    pAppData->H264_TestCaseParams->bConstIntraPred;
		tAVCParams.eLoopFilterMode =
		    pAppData->H264_TestCaseParams->bLoopFilter;
		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamVideoAvc,
		    &tAVCParams);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

	tVidEncBitRate.nPortIndex = OMX_DirOutput;
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoBitrate,
	    &tVidEncBitRate);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	tVidEncBitRate.eControlRate =
	    pAppData->H264_TestCaseParams->RateControlAlg;
	if (bBitrate)
	{
		tVidEncBitRate.nTargetBitrate = LBitRate;
	}
	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamVideoBitrate,
	    &tVidEncBitRate);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	/*Update the DynamicSettingsArray */
	eError = H264E_Update_DynamicSettingsArray(RemoveIndex, nRemoveList);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}



static OMX_ERRORTYPE H264ENC_SetAllParams(H264E_ILClient * pAppData,
    OMX_U32 test_index)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = NULL;

	OMX_VIDEO_PARAM_PROFILELEVELTYPE tProfileLevel;
	OMX_VIDEO_PARAM_AVCBITSTREAMFORMATTYPE tBitStreamFormat;
	OMX_VIDEO_PARAM_ENCODER_PRESETTYPE tEncoderPreset;
	OMX_VIDEO_PARAM_FRAMEDATACONTENTTYPE tInputImageFormat;
	OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE tTransformBlockSize;
	OMX_VIDEO_PARAM_AVCSLICEFMO tAVCFMO;

	OMX_U32 i;

	H264CLIENT_ENTER_PRINT();

	if (!pAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pHandle = pAppData->pHandle;

	/*Profile Level settings */
	OMX_TEST_INIT_STRUCT_PTR(&tProfileLevel,
	    OMX_VIDEO_PARAM_PROFILELEVELTYPE);
	tProfileLevel.nPortIndex = H264_APP_OUTPUTPORT;

	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoProfileLevelCurrent,
	    &tProfileLevel);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tProfileLevel.eProfile = pAppData->H264_TestCaseParams->profile;
	tProfileLevel.eLevel = pAppData->H264_TestCaseParams->level;

	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamVideoProfileLevelCurrent,
	    &tProfileLevel);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*BitStream Format settings */
	OMX_TEST_INIT_STRUCT_PTR(&tBitStreamFormat,
	    OMX_VIDEO_PARAM_AVCBITSTREAMFORMATTYPE);
	tBitStreamFormat.nPortIndex = H264_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoBitStreamFormatSelect,
	    &tBitStreamFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	tBitStreamFormat.eStreamFormat =
	    pAppData->H264_TestCaseParams->BitStreamFormat;
	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoBitStreamFormatSelect,
	    &tBitStreamFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Encoder Preset settings */
	OMX_TEST_INIT_STRUCT_PTR(&tEncoderPreset,
	    OMX_VIDEO_PARAM_ENCODER_PRESETTYPE);
	tEncoderPreset.nPortIndex = H264_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoEncoderPreset,
	    &tEncoderPreset);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tEncoderPreset.eEncodingModePreset =
	    pAppData->H264_TestCaseParams->EncodingPreset;
	tEncoderPreset.eRateControlPreset =
	    pAppData->H264_TestCaseParams->RateCntrlPreset;

	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoEncoderPreset,
	    &tEncoderPreset);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Image Type settings */
	OMX_TEST_INIT_STRUCT_PTR(&tInputImageFormat,
	    OMX_VIDEO_PARAM_FRAMEDATACONTENTTYPE);
	tInputImageFormat.nPortIndex = H264_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoFrameDataContentSettings,
	    &tInputImageFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	tInputImageFormat.eContentType =
	    pAppData->H264_TestCaseParams->InputContentType;
	if (tInputImageFormat.eContentType != OMX_Video_Progressive)
	{
		tInputImageFormat.eInterlaceCodingType =
		    pAppData->H264_TestCaseParams->InterlaceCodingType;
	}
	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoFrameDataContentSettings,
	    &tInputImageFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Transform BlockSize settings */
	OMX_TEST_INIT_STRUCT_PTR(&tTransformBlockSize,
	    OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE);
	tTransformBlockSize.nPortIndex = H264_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoTransformBlockSize,
	    &tTransformBlockSize);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tTransformBlockSize.eTransformBlocksize =
	    pAppData->H264_TestCaseParams->TransformBlockSize;

	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoTransformBlockSize,
	    &tTransformBlockSize);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	if ((pAppData->H264_TestCaseParams->bFMO) &&
	    ((pAppData->H264_TestCaseParams->nBitEnableAdvanced >> 1) == 0))
	{
		/*Basic FMO  settings */
		OMX_TEST_INIT_STRUCT_PTR(&tAVCFMO,
		    OMX_VIDEO_PARAM_AVCSLICEFMO);
		tAVCFMO.nPortIndex = H264_APP_OUTPUTPORT;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamVideoSliceFMO,
		    &tAVCFMO);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		tAVCFMO.nNumSliceGroups =
		    pAppData->H264_TestCaseParams->nNumSliceGroups;
		tAVCFMO.nSliceGroupMapType =
		    pAppData->H264_TestCaseParams->nSliceGroupMapType;
		tAVCFMO.eSliceMode =
		    pAppData->H264_TestCaseParams->eSliceMode;

		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamVideoSliceFMO,
		    &tAVCFMO);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}

	/*Set the Advanced Setting if any */
	eError = H264ENC_SetAdvancedParams(pAppData, test_index);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Settings from Initial DynamicParams */
	eError =
	    H264ENC_SetParamsFromInitialDynamicParams(pAppData, test_index);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

	/* This method will wait for the component to get to the desired state. */
static OMX_ERRORTYPE H264ENC_WaitForState(H264E_ILClient * pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents, numRemaining;

	H264CLIENT_ENTER_PRINT();

	/* Wait for an event */
	uRequestedEvents = (H264_STATETRANSITION_COMPLETE | H264_ERROR_EVENT);
	retval = TIMM_OSAL_EventRetrieve(H264VE_CmdEvent, uRequestedEvents,
	    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents, TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	if (pRetrievedEvents & H264_ERROR_EVENT)
	{
		eError = pAppData->eAppError;
	} else
	{
		/*transition Complete */
		eError = OMX_ErrorNone;
	}

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE H264ENC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	H264CLIENT_ENTER_PRINT();

	if (!ptrAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		retval =
		    TIMM_OSAL_EventSet(H264VE_CmdEvent,
		    H264_STATETRANSITION_COMPLETE, TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		break;

	case OMX_EventError:
		H264CLIENT_ERROR_PRINT
		    ("received from the component in EventHandler=%d",
		    (OMX_ERRORTYPE) nData1);
		retval =
		    TIMM_OSAL_EventSet(H264VE_Events, H264_ERROR_EVENT,
		    TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		retval =
		    TIMM_OSAL_EventSet(H264VE_CmdEvent, H264_ERROR_EVENT,
		    TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		break;

	case OMX_EventBufferFlag:
		if (nData2 == OMX_BUFFERFLAG_EOS)
		{
			retval =
			    TIMM_OSAL_EventSet(H264VE_Events, H264_EOS_EVENT,
			    TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
		}
		H264CLIENT_TRACE_PRINT("Event: EOS has reached the client");
		break;

	case OMX_EventMark:
	case OMX_EventPortSettingsChanged:
	case OMX_EventResourcesAcquired:
	case OMX_EventComponentResumed:
	case OMX_EventDynamicResourcesAvailable:
	case OMX_EventPortFormatDetected:
	case OMX_EventMax:
		eError = OMX_ErrorNotImplemented;
		break;
	default:
		eError = OMX_ErrorBadParameter;
		break;


	}			// end of switch

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE H264ENC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	H264E_ILClient *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	H264CLIENT_ENTER_PRINT();
#ifdef TIMESTAMPSPRINT		/*to check the prder of output timestamp information....will be removed later */
	TimeStampsArray[nFillBufferDoneCount] = pBuffer->nTimeStamp;
	nFillBufferDoneCount++;
#endif
#ifdef OMX_H264E_SRCHANGES
	H264CLIENT_TRACE_PRINT("\npBuffer SR after FBD = %x\n",
	    pBuffer->pBuffer);
	pBuffer->pBuffer = SharedRegion_getPtr(pBuffer->pBuffer);
	H264CLIENT_TRACE_PRINT("\npBuffer after FBD = %x\n",
	    pBuffer->pBuffer);
#endif

	H264CLIENT_TRACE_PRINT("Write the buffer to the Output pipe");
	retval =
	    TIMM_OSAL_WriteToPipe(pAppData->OpBuf_Pipe, &pBuffer,
	    sizeof(pBuffer), TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);
	H264CLIENT_TRACE_PRINT("Set the Output Ready event");
	retval =
	    TIMM_OSAL_EventSet(H264VE_Events, H264_OUTPUT_READY,
	    TIMM_OSAL_EVENT_OR);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);


      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE H264ENC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	H264E_ILClient *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	H264CLIENT_ENTER_PRINT();

#ifdef OMX_H264E_SRCHANGES
	H264CLIENT_TRACE_PRINT("\npBuffer SR after EBD = %x\n",
	    pBuffer->pBuffer);
	pBuffer->pBuffer = SharedRegion_getPtr(pBuffer->pBuffer);
	H264CLIENT_TRACE_PRINT("\npBuffer after EBD = %x\n",
	    pBuffer->pBuffer);
#endif
	H264CLIENT_TRACE_PRINT("Write the buffer to the Input pipe");
	retval =
	    TIMM_OSAL_WriteToPipe(pAppData->IpBuf_Pipe, &pBuffer,
	    sizeof(pBuffer), TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);
	H264CLIENT_TRACE_PRINT("Set the Input Ready event");
	retval =
	    TIMM_OSAL_EventSet(H264VE_Events, H264_INPUT_READY,
	    TIMM_OSAL_EVENT_OR);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);


      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

#ifdef H264_LINUX_CLIENT
void main()
#else
void OMXH264Enc_TestEntry(Int32 paramSize, void *pParam)
#endif
{
	H264E_ILClient pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 test_index = 0, i;
	/*Memory leak detect */
	OMX_U32 mem_count_start = 0;
	OMX_U32 mem_size_start = 0;
	OMX_U32 mem_count_end = 0;
	OMX_U32 mem_size_end = 0;
#ifndef H264_LINUX_CLIENT
	Memory_Stats stats;
#endif

#ifdef H264_LINUX_CLIENT
	H264CLIENT_TRACE_PRINT
	    ("\nWait until RCM Server is created on other side. Press any key after that\n");
	getchar();
#endif

#ifndef H264_LINUX_CLIENT
	/*Setting the Trace Group */
	TraceGrp = TIMM_OSAL_GetTraceGrp();
	TIMM_OSAL_SetTraceGrp((TraceGrp | (1 << 3)));	/*to VideoEncoder */

	H264CLIENT_TRACE_PRINT("tracegroup=%x", TraceGrp);
#endif

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (test_index = 0; test_index < 1; test_index++)
	{
		H264CLIENT_TRACE_PRINT("Running Testcasenumber=%d",
		    test_index);

		pAppData.H264_TestCaseParams =
		    &H264_TestCaseTable[test_index];
#ifdef TIMESTAMPSPRINT		/*will be removed later */
		/*print the received timestamps information */
		for (i = 0; i < 20; i++)
		{
			TimeStampsArray[i] = 0;
		}
		nFillBufferDoneCount = 0;
#endif

		/*Check for the Test case ID */
		switch (pAppData.H264_TestCaseParams->TestCaseId)
		{
		case 0:
			/*call to Total functionality */
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);
#if 0
			Memory_getStats(NULL, &stats);
			H264CLIENT_INFO_PRINT("Total size = %d",
			    stats.totalSize);
			H264CLIENT_INFO_PRINT("Total free size = %d",
			    stats.totalFreeSize);
			H264CLIENT_INFO_PRINT("Largest Free size = %d",
			    stats.largestFreeSize);
#endif

			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif

			eError =
			    OMXH264Enc_CompleteFunctionality(&pAppData,
			    test_index);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif


			break;
		case 1:
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			/*call to gethandle & Freehandle: Simple Test */
			for (i = 0; i < GETHANDLE_FREEHANDLE_LOOPCOUNT; i++)
			{
				mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
				mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

				H264CLIENT_INFO_PRINT
				    ("before Test case Start: Value from GetMemCounter = %d",
				    mem_count_start);
#ifndef H264_LINUX_CLIENT
				H264CLIENT_INFO_PRINT
				    ("before Test case Start: Value from GetMemUsage = %d",
				    mem_size_start);
#endif
				eError =
				    OMXH264Enc_GetHandle_FreeHandle
				    (&pAppData);

				mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
				mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
				H264CLIENT_INFO_PRINT
				    (" After Test case Run: Value from GetMemCounter = %d",
				    mem_count_end);
#ifndef H264_LINUX_CLIENT
				H264CLIENT_INFO_PRINT
				    (" After Test case Run: Value from GetMemUsage = %d",
				    mem_size_end);
				if (mem_count_start != mem_count_end)
				{
					H264CLIENT_ERROR_PRINT
					    ("Memory leak detected. Bytes lost = %d",
					    (mem_size_end - mem_size_start));
				}
#endif
			}

			break;

		case 2:
			/*call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being Disabled */
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif

			eError =
			    OMXH264Enc_LoadedToIdlePortDisable(&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif

			break;
		case 3:
			/*call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif
			eError = OMXH264Enc_LoadedToIdlePortEnable(&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif

			break;
		case 4:
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			/*Unaligned Buffers : client allocates the buffers (use buffer)
			   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif
			eError = OMXH264Enc_UnalignedBuffers(&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif
			break;
		case 5:
			/*less than the min size requirements : client allocates the buffers (use buffer)/component allocate (allocate buffer)
			   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif

			eError =
			    OMXH264Enc_SizeLessthanMinBufferRequirements
			    (&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif
			break;
		case 6:
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			/* Different Number of input & output buffers satisfying the min buffers requirements
			   complete functionality tested */
			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif

			eError =
			    OMXH264Enc_CompleteFunctionality(&pAppData,
			    test_index);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif
			break;
		case 7:
			/* Different Number of input & output buffers Not satisfying the min buffers requirements
			   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with less num of buffers */
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    ("before Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif

			eError =
			    OMXH264Enc_LessthanMinBufferRequirementsNum
			    (&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef H264_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef H264_LINUX_CLIENT
			H264CLIENT_INFO_PRINT
			    (" After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				H264CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif
			break;


		default:
			H264CLIENT_TRACE_PRINT("Running TestCaseId=%d",
			    pAppData.H264_TestCaseParams->TestCaseId);

			H264CLIENT_TRACE_PRINT("Not Implemented");
			//printf("\nNot Implemented\n");

		}
		OutputFilesize = 0;
		nFramesRead = 0;
		//TIMM_OSAL_EndMemDebug();
	      EXIT:
		H264CLIENT_TRACE_PRINT("Reached the end of the programming");

	}			/*End of test index loop */
}

OMX_ERRORTYPE OMXH264Enc_CompleteFunctionality(H264E_ILClient *
    pApplicationData, OMX_U32 test_index)
{
	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	pAppData->pHandle = pHandle;
	/* Open the input file for data to be rendered.  */
	pAppData->fIn = fopen(pAppData->H264_TestCaseParams->InFile, "rb");
	GOTO_EXIT_IF((pAppData->fIn == NULL), OMX_ErrorUnsupportedSetting);
	/* Open the output file */
	pAppData->fOut = fopen(pAppData->H264_TestCaseParams->OutFile, "wb");
	GOTO_EXIT_IF((pAppData->fOut == NULL), OMX_ErrorUnsupportedSetting);

	/*Set the Component Static Params */
	H264CLIENT_TRACE_PRINT
	    ("Set the Component Initial Parameters (in Loaded state)");
	eError = H264ENC_SetAllParams(pAppData, test_index);
	if (eError != OMX_ErrorNone)
	{
		OMXH264Enc_Client_ErrorHandling(pAppData, 1, eError);
		goto EXIT;
	}

	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	if (eError != OMX_ErrorNone)
	{
		OMXH264Enc_Client_ErrorHandling(pAppData, 1, eError);
		goto EXIT;
	}

	H264CLIENT_TRACE_PRINT
	    ("Allocate the resources for the state trasition to Idle to complete");
	eError = H264ENC_AllocateResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> executing state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = OMX_GetState(pHandle, &(pAppData->eState));
	H264CLIENT_TRACE_PRINT("Trasition to executing state is completed");

	/*Executing Loop */
	eError = OMXH264Enc_InExecuting(pAppData, test_index);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Command to goto Idle */
	H264CLIENT_TRACE_PRINT("call to goto executing ->Idle state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("Free up the Component Resources");
	eError = H264ENC_FreeResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*close the files after processing has been done */
	H264CLIENT_TRACE_PRINT("Close the Input & Output file");
	if (pAppData->fIn)
		fclose(pAppData->fIn);
	if (pAppData->fOut)
		fclose(pAppData->fOut);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_ERRORTYPE OMXH264Enc_InExecuting(H264E_ILClient * pApplicationData,
    OMX_U32 test_index)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_BUFFERHEADERTYPE *pBufferIn = NULL;
	OMX_BUFFERHEADERTYPE *pBufferOut = NULL;
	OMX_U32 actualSize, fwritesize, fflushstatus;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents, numRemaining;
	OMX_U8 FrameNumToProcess = 1, PauseFrameNumIndex = 0, i;	//,nCountFTB=1;
	OMX_H264E_DynamicConfigs OMXConfigStructures;


	H264CLIENT_ENTER_PRINT();
	/*Initialize the ConfigStructures */

	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tFrameRate),
	    OMX_CONFIG_FRAMERATETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tBitRate),
	    OMX_VIDEO_CONFIG_BITRATETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tMESearchrange),
	    OMX_VIDEO_CONFIG_MESEARCHRANGETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tForceframe),
	    OMX_CONFIG_INTRAREFRESHVOPTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tQPSettings),
	    OMX_VIDEO_CONFIG_QPSETTINGSTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tAVCIntraPeriod),
	    OMX_VIDEO_CONFIG_AVCINTRAPERIOD);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tSliceSettings),
	    OMX_VIDEO_CONFIG_SLICECODINGTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&(OMXConfigStructures.tPixelInfo),
	    OMX_VIDEO_CONFIG_PIXELINFOTYPE);


	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;
	/*Populate the Handle */
	pHandle = pAppData->pHandle;

	/*Check: partial or full record & in case of partial record check for stop frame number */
	if (pAppData->H264_TestCaseParams->TestType == PARTIAL_RECORD)
	{
		H264ClientStopFrameNum =
		    pAppData->H264_TestCaseParams->StopFrameNum;
		H264CLIENT_TRACE_PRINT("StopFrame in Partial record case=%d",
		    H264ClientStopFrameNum);
	} else if (pAppData->H264_TestCaseParams->TestType == FULL_RECORD)
	{
		/*go ahead with processing until it reaches EOS..update the "H264ClientStopFrameNum" with sufficiently big value: used in case of FULL RECORD */
		H264ClientStopFrameNum = MAXFRAMESTOREADFROMFILE;
	} else
	{
		eError = OMX_ErrorBadParameter;
		H264CLIENT_ERROR_PRINT("Invalid parameter:TestType ");
		goto EXIT;
	}
	while ((eError == OMX_ErrorNone) &&
	    ((pAppData->eState == OMX_StateExecuting) ||
		(pAppData->eState == OMX_StatePause)))
	{

		/* Wait for an event (input/output/error) */
		H264CLIENT_TRACE_PRINT
		    ("\nWait for an event (input/output/error)\n");
		uRequestedEvents =
		    (H264_INPUT_READY | H264_OUTPUT_READY | H264_ERROR_EVENT |
		    H264_EOS_EVENT);
		retval =
		    TIMM_OSAL_EventRetrieve(H264VE_Events, uRequestedEvents,
		    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents,
		    TIMM_OSAL_SUSPEND);
		H264CLIENT_TRACE_PRINT("\nEvent recd. = %d\n",
		    pRetrievedEvents);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorInsufficientResources);

		/*Check for Error Event & Exit upon receiving the Error */
		GOTO_EXIT_IF((pRetrievedEvents & H264_ERROR_EVENT),
		    pAppData->eAppError);

		/*Check for OUTPUT READY Event */
		numRemaining = 0;
		if (pRetrievedEvents & H264_OUTPUT_READY)
		{
			H264CLIENT_TRACE_PRINT("in Output Ready Event");
			retval =
			    TIMM_OSAL_GetPipeReadyMessageCount(pAppData->
			    OpBuf_Pipe, &numRemaining);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			H264CLIENT_TRACE_PRINT("Output Msg count=%d",
			    numRemaining);
			while (numRemaining)
			{
				/*read from the pipe */
				H264CLIENT_TRACE_PRINT
				    ("Read Buffer from the Output Pipe to give the Next FTB call");
				retval =
				    TIMM_OSAL_ReadFromPipe(pAppData->
				    OpBuf_Pipe, &pBufferOut,
				    sizeof(pBufferOut), &actualSize,
				    TIMM_OSAL_SUSPEND);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorNotReady);
				if (pBufferOut->nFilledLen)
				{
					H264CLIENT_TRACE_PRINT
					    ("Write the data into output file before it is given for the next FTB call");
					fwritesize =
					    fwrite((pBufferOut->pBuffer +
						(pBufferOut->nOffset)),
					    sizeof(OMX_U8),
					    pBufferOut->nFilledLen,
					    pAppData->fOut);
					fflushstatus = fflush(pAppData->fOut);
					if (fflushstatus != 0)
						H264CLIENT_ERROR_PRINT
						    ("While fflush operation");
					OutputFilesize +=
					    pBufferOut->nFilledLen;
				}

				H264CLIENT_TRACE_PRINT
				    ("timestamp received is=%ld  ",
				    pBufferOut->nTimeStamp);
				H264CLIENT_TRACE_PRINT
				    ("filledlen=%d  fwritesize=%d, OutputFilesize=%d",
				    pBufferOut->nFilledLen, fwritesize,
				    OutputFilesize);
				H264CLIENT_TRACE_PRINT
				    ("update the nFilledLen & nOffset");
				pBufferOut->nFilledLen = 0;
				pBufferOut->nOffset = 0;
				H264CLIENT_TRACE_PRINT("Before the FTB call");
#ifdef OMX_H264E_SRCHANGES
				pBufferOut->pBuffer =
				    (char *)SharedRegion_getSRPtr(pBufferOut->
				    pBuffer, 2);
#endif

				eError =
				    OMX_FillThisBuffer(pHandle, pBufferOut);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
#ifdef OMX_H264E_SRCHANGES
				pBufferOut->pBuffer =
				    SharedRegion_getPtr(pBufferOut->pBuffer);
#endif

				retval =
				    TIMM_OSAL_GetPipeReadyMessageCount
				    (pAppData->OpBuf_Pipe, &numRemaining);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorBadParameter);
				H264CLIENT_TRACE_PRINT("Output Msg count=%d",
				    numRemaining);

			}
		}

		/*Check for EOS Event */
		if ((pRetrievedEvents & H264_EOS_EVENT))
		{
#ifdef TIMESTAMPSPRINT		/*to check the prder of output timestamp information....will be removed later */
			/*print the received timestamps information */
			for (i = 0; i < H264ClientStopFrameNum; i++)
			{
				H264CLIENT_TRACE_PRINT("TimeStamp[%d]=%ld", i,
				    TimeStampsArray[i]);
			}
#endif
			eError = OMX_ErrorNone;
			goto EXIT;

		}


		/*Check for Input Ready Event */
		numRemaining = 0;
		if ((pRetrievedEvents & H264_INPUT_READY))
		{

			/*continue with the ETB calls only if H264ClientStopFrameNum!=FrameNumToProcess */
			if ((H264ClientStopFrameNum >= FrameNumToProcess))
			{
				H264CLIENT_TRACE_PRINT
				    ("In Input Ready event");
				retval =
				    TIMM_OSAL_GetPipeReadyMessageCount
				    (pAppData->IpBuf_Pipe, &numRemaining);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorBadParameter);
				H264CLIENT_TRACE_PRINT("Input Msg count=%d",
				    numRemaining);
				while ((numRemaining) &&
				    ((H264ClientStopFrameNum >=
					    FrameNumToProcess)))
				{
					/*read from the pipe */
					H264CLIENT_TRACE_PRINT
					    ("Read Buffer from the Input Pipe to give the Next ETB call");
					retval =
					    TIMM_OSAL_ReadFromPipe(pAppData->
					    IpBuf_Pipe, &pBufferIn,
					    sizeof(pBufferIn), &actualSize,
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorNotReady);
					H264CLIENT_TRACE_PRINT
					    ("Read Data from the Input File");
					eError =
					    H264ENC_FillData(pAppData,
					    pBufferIn);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
					H264CLIENT_TRACE_PRINT("Call to ETB");
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer before ETB = %x\n",
					    pBufferIn->pBuffer);
					pBufferIn->pBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pBufferIn->
					    pBuffer, 2);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR before ETB = %x\n",
					    pBufferIn->pBuffer);
#endif

					eError =
					    OMX_EmptyThisBuffer(pHandle,
					    pBufferIn);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_H264E_SRCHANGES
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer SR after ETB = %x\n",
					    pBufferIn->pBuffer);
					pBufferIn->pBuffer =
					    SharedRegion_getPtr(pBufferIn->
					    pBuffer);
					H264CLIENT_TRACE_PRINT
					    ("\npBuffer after ETB = %x\n",
					    pBufferIn->pBuffer);
#endif

					/*Send command to Pause */
					if ((PauseFrameNum[PauseFrameNumIndex]
						== FrameNumToProcess) &&
					    (PauseFrameNumIndex <
						NUMTIMESTOPAUSE))
					{
						H264CLIENT_TRACE_PRINT
						    ("command to go Exectuing -> Pause state");
						eError =
						    OMX_SendCommand(pHandle,
						    OMX_CommandStateSet,
						    OMX_StatePause, NULL);
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);

						eError =
						    H264ENC_WaitForState
						    (pHandle);
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);
						PauseFrameNumIndex++;
						H264CLIENT_TRACE_PRINT
						    ("Component is in Paused state");
					}

					/*Call to Dynamic Settings */
					H264CLIENT_TRACE_PRINT
					    ("Configure the RunTime Settings");
					eError =
					    OMXH264Enc_ConfigureRunTimeSettings
					    (pAppData, &OMXConfigStructures,
					    test_index, FrameNumToProcess);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);

					/*Increase the frame count that has been sent for processing */
					FrameNumToProcess++;

					retval =
					    TIMM_OSAL_GetPipeReadyMessageCount
					    (pAppData->IpBuf_Pipe,
					    &numRemaining);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorBadParameter);
					H264CLIENT_TRACE_PRINT
					    ("Input Msg count=%d",
					    numRemaining);
				}
			} else
			{
				H264CLIENT_TRACE_PRINT
				    ("Nothing todo ..check for other events");
			}

		}

		/*instead of sleep......to send the executing State command */
		while ((pAppData->eState == OMX_StatePause) &&
		    (TempCount < 10))
		{
			TempCount++;
		}
		if (TempCount == 10)
		{
			H264CLIENT_TRACE_PRINT
			    ("command to go Pause -> Executing state");
			eError =
			    OMX_SendCommand(pHandle, OMX_CommandStateSet,
			    OMX_StateExecuting, NULL);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			eError = H264ENC_WaitForState(pHandle);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			TempCount = 0;

		}

		/*get the Component State */
		eError = OMX_GetState(pHandle, &(pAppData->eState));
		H264CLIENT_TRACE_PRINT("Component Current State=%d",
		    pAppData->eState);

	}
      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXH264Enc_ConfigureRunTimeSettings(H264E_ILClient *
    pApplicationData, OMX_H264E_DynamicConfigs * H264EConfigStructures,
    OMX_U32 test_index, OMX_U32 FrameNumber)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_U32 nRemoveList[9];
	OMX_U8 i, RemoveIndex = 0;


	H264CLIENT_ENTER_PRINT();

	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pHandle = pApplicationData->pHandle;

	for (i = 0; i < DynamicSettingsCount; i++)
	{
		switch (DynamicSettingsArray[i])
		{
		case 0:
			/*set config to OMX_CONFIG_FRAMERATETYPE */
			if (H264_TestCaseDynTable[test_index].DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tFrameRate.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoFramerate,
				    &(H264EConfigStructures->tFrameRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				H264EConfigStructures->tFrameRate.
				    xEncodeFramerate =
				    ((H264_TestCaseDynTable[test_index].
					DynFrameRate.
					nFramerate[DynFrameCountArray_Index
					    [i]]) << 16);

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoFramerate,
				    &(H264EConfigStructures->tFrameRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynFrameRate.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 1:
			/*set config to OMX_VIDEO_CONFIG_BITRATETYPE */
			if (H264_TestCaseDynTable[test_index].DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tBitRate.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoBitrate,
				    &(H264EConfigStructures->tBitRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tBitRate.
				    nEncodeBitrate =
				    H264_TestCaseDynTable[test_index].
				    DynBitRate.
				    nBitrate[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoBitrate,
				    &(H264EConfigStructures->tBitRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynBitRate.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 2:
			/*set config to OMX_TI_VIDEO_CONFIG_ME_SEARCHRANGE */
			if (H264_TestCaseDynTable[test_index].
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tMESearchrange.
				    nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &(H264EConfigStructures->tMESearchrange));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tMESearchrange.
				    eMVAccuracy =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nMVAccuracy[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tMESearchrange.
				    nHorSearchRangeP =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nHorSearchRangeP[DynFrameCountArray_Index
				    [i]];
				H264EConfigStructures->tMESearchrange.
				    nVerSearchRangeP =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nVerSearchRangeP[DynFrameCountArray_Index
				    [i]];
				H264EConfigStructures->tMESearchrange.
				    nHorSearchRangeB =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nHorSearchRangeB[DynFrameCountArray_Index
				    [i]];
				H264EConfigStructures->tMESearchrange.
				    nVerSearchRangeB =
				    H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nVerSearchRangeB[DynFrameCountArray_Index
				    [i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &(H264EConfigStructures->tMESearchrange));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynMESearchRange.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 3:
			/*set config to OMX_CONFIG_INTRAREFRESHVOPTYPE */
			if (H264_TestCaseDynTable[test_index].DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tForceframe.
				    nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &(H264EConfigStructures->tForceframe));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tForceframe.
				    IntraRefreshVOP =
				    H264_TestCaseDynTable[test_index].
				    DynForceFrame.
				    ForceIFrame[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &(H264EConfigStructures->tForceframe));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynForceFrame.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 4:
			/*set config to OMX_TI_VIDEO_CONFIG_QPSETTINGS */
			if (H264_TestCaseDynTable[test_index].DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tQPSettings.
				    nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &(H264EConfigStructures->tQPSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tQPSettings.nQpI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpI[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMaxI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxI[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMinI =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinI[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpP =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpP[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMaxP =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxP[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMinP =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinP[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.
				    nQpOffsetB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpOffsetB[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMaxB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMaxB[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tQPSettings.nQpMinB =
				    H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nQpMinB[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &(H264EConfigStructures->tQPSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynQpSettings.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 5:
			/*set config to OMX_VIDEO_CONFIG_AVCINTRAPERIOD */
			if (H264_TestCaseDynTable[test_index].
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tAVCIntraPeriod.
				    nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoAVCIntraPeriod,
				    &(H264EConfigStructures->
					tAVCIntraPeriod));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tAVCIntraPeriod.
				    nIDRPeriod =
				    H264_TestCaseDynTable[test_index].
				    DynIntraFrmaeInterval.
				    nIntraFrameInterval
				    [DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoAVCIntraPeriod,
				    &(H264EConfigStructures->
					tAVCIntraPeriod));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynIntraFrmaeInterval.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		case 6:
			/*set config to OMX_IndexConfigVideoNalSize */
			if (H264_TestCaseDynTable[test_index].DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tNALUSize.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(H264EConfigStructures->tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tNALUSize.nNaluBytes =
				    H264_TestCaseDynTable[test_index].
				    DynNALSize.
				    nNaluSize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(H264EConfigStructures->tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynNALSize.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}

			break;
		case 7:
			/*set config to OMX_TI_VIDEO_CONFIG_SLICECODING */
			if (H264_TestCaseDynTable[test_index].
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				H264EConfigStructures->tSliceSettings.
				    nPortIndex = H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigSliceSettings,
				    &(H264EConfigStructures->tSliceSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tSliceSettings.
				    eSliceMode =
				    H264_TestCaseDynTable[test_index].
				    DynSliceSettings.
				    eSliceMode[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tSliceSettings.
				    nSlicesize =
				    H264_TestCaseDynTable[test_index].
				    DynSliceSettings.
				    nSlicesize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigSliceSettings,
				    &(H264EConfigStructures->tSliceSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynSliceSettings.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}

			break;
		case 8:
			/*set config to OMX_TI_VIDEO_CONFIG_PIXELINFO */
			if (H264_TestCaseDynTable[test_index].DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{

				/*Set Port Index */
				H264EConfigStructures->tPixelInfo.nPortIndex =
				    H264_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &(H264EConfigStructures->tPixelInfo));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				H264EConfigStructures->tPixelInfo.nWidth =
				    H264_TestCaseDynTable[test_index].
				    DynPixelInfo.
				    nWidth[DynFrameCountArray_Index[i]];
				H264EConfigStructures->tPixelInfo.nHeight =
				    H264_TestCaseDynTable[test_index].
				    DynPixelInfo.
				    nHeight[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &(H264EConfigStructures->tPixelInfo));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (H264_TestCaseDynTable[test_index].
				    DynPixelInfo.
				    nFrameNumber[DynFrameCountArray_Index[i]]
				    == -1)
				{
					nRemoveList[RemoveIndex++] =
					    DynamicSettingsArray[i];
				}
			}
			break;
		default:
			break;
		}
	}

	/*Update the DynamicSettingsArray */
	eError = H264E_Update_DynamicSettingsArray(RemoveIndex, nRemoveList);

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}


/*
Copies 1D buffer to 2D buffer. All heights, widths etc. should be in bytes.
The function copies the lower no. of bytes i.e. if nSize1D < (nHeight2D * nWidth2D)
then nSize1D bytes is copied else (nHeight2D * nWidth2D) bytes is copied.
This function does not return any leftover no. of bytes, the calling function
needs to take care of leftover bytes on its own
*/
OMX_ERRORTYPE OMXH264_Util_Memcpy_1Dto2D(OMX_PTR pDst2D, OMX_PTR pSrc1D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_U8 *pInBuffer;
	OMX_U8 *pOutBuffer;
	OMX_U32 nSizeLeft, i;

	nSizeLeft = nSize1D;
	pInBuffer = (OMX_U8 *) pSrc1D;
	pOutBuffer = (OMX_U8 *) pDst2D;
	//The lower limit is copied. If nSize1D < H*W then 1Dsize is copied else H*W is copied
	for (i = 0; i < nHeight2D; i++)
	{
		if (nSizeLeft >= nWidth2D)
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nWidth2D);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorUndefined);
		} else
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer,
			    nSizeLeft);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorUndefined);
			break;
		}
		nSizeLeft -= nWidth2D;
		pInBuffer = (OMX_U8 *) ((TIMM_OSAL_U32) pInBuffer + nWidth2D);
		pOutBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pOutBuffer + nStride2D);
	}

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}


/*
Copies 2D buffer to 1D buffer. All heights, widths etc. should be in bytes.
The function copies the lower no. of bytes i.e. if nSize1D < (nHeight2D * nWidth2D)
then nSize1D bytes is copied else (nHeight2D * nWidth2D) bytes is copied.
This function does not return any leftover no. of bytes, the calling function
needs to take care of leftover bytes on its own
*/
OMX_ERRORTYPE OMXH264_Util_Memcpy_2Dto1D(OMX_PTR pDst1D, OMX_PTR pSrc2D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D)
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
	for (i = 0; i < nHeight2D; i++)
	{
		if (nSizeLeft >= nWidth2D)
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nWidth2D);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorUndefined);
		} else
		{
			retval =
			    TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer,
			    nSizeLeft);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorUndefined);
			break;
		}
		nSizeLeft -= nWidth2D;
		pInBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pInBuffer + nStride2D);
		pOutBuffer =
		    (OMX_U8 *) ((TIMM_OSAL_U32) pOutBuffer + nWidth2D);
	}

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_ERRORTYPE OMXH264Enc_DynamicPortConfiguration(H264E_ILClient *
    pApplicationData, OMX_U32 nPortIndex)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	H264E_ILClient *pAppData;

	H264CLIENT_ENTER_PRINT();

	/*validate the Input Prameters */
	if (pApplicationData == NULL)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pAppData = pApplicationData;

	/*validate the Port Index */
	if (nPortIndex > H264_APP_OUTPUTPORT)
	{
		eError = OMX_ErrorBadPortIndex;
		goto EXIT;
	}

	/*call OMX_Sendcommand with cmd as Port Disable */
	eError =
	    OMX_SendCommand(pAppData->pHandle, OMX_CommandPortDisable,
	    nPortIndex, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Configure the port with new Settings: tbd */

	/*call OMX_Sendcommand with cmd as Port Enable */
	eError =
	    OMX_SendCommand(pAppData->pHandle, OMX_CommandPortEnable,
	    nPortIndex, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

void OMXH264Enc_Client_ErrorHandling(H264E_ILClient * pApplicationData,
    OMX_U32 nErrorIndex, OMX_ERRORTYPE eGenError)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	H264E_ILClient *pAppData;

	H264CLIENT_ENTER_PRINT();

	/*validate the Input Prameters */
	if (pApplicationData == NULL)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pAppData = pApplicationData;
	switch (nErrorIndex)
	{
	case 1:
		/*Error During Set Params or Error During state Transition Loaded->Idle */
		eError = OMX_FreeHandle(pAppData->pHandle);
		H264CLIENT_ERROR_PRINT("%s  During FreeHandle",
		    H264_GetErrorString(eGenError));
		break;
	case 2:
		/*TODO: Error During Allocate Buffers */
		break;
	case 3:
		/*TODO: state Invalid */
		break;
	case 4:
		/*TODO: Error During state Transition Idle->Executing */
		break;
	case 5:
		/*TODO: Error During state Transition Executing->Pause */
		break;
	case 6:
		/*TODO: Error During state Transition Pause->Executing */
		break;
	case 7:
		/*TODO: Error During state Transition Executing->idle */
		break;
	case 8:
		/*TODO: Error During state Transition Pause->Idle */
		break;
	case 9:
		/*TODO: Error During state Transition Idle->Loaded */
		break;
	case 10:
		/*TODO: Error while in Executing */
		break;
	case 11:
		/*TODO: Error during FreeBuffer */
		break;
	}

      EXIT:
	H264CLIENT_EXIT_PRINT(eError);

}


OMX_ERRORTYPE OMXH264Enc_GetHandle_FreeHandle(H264E_ILClient *
    pApplicationData)
{
	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;

	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;




}

OMX_ERRORTYPE OMXH264Enc_LoadedToIdlePortDisable(H264E_ILClient *
    pApplicationData)
{

	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;


	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;
	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/*call OMX_Sendcommand with cmd as Port Disable */
	eError =
	    OMX_SendCommand(pAppData->pHandle, OMX_CommandPortDisable,
	    OMX_ALL, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXH264Enc_LoadedToIdlePortEnable(H264E_ILClient *
    pApplicationData)
{

	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*for the allocate resorces call fill the height & width*/
	pAppData->H264_TestCaseParams->width = 176;
	pAppData->H264_TestCaseParams->height = 144;

	H264CLIENT_TRACE_PRINT
	    ("Allocate the resources for the state trasition to Idle to complete");
	eError = H264ENC_AllocateResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("Free up the Component Resources");
	eError = H264ENC_FreeResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXH264Enc_UnalignedBuffers(H264E_ILClient * pApplicationData)
{
	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U8 *pTmpBuffer;	/*Used with Use Buffer calls */
	OMX_U32 i, j;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	/*Initialize the structure */
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);


	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;
	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
/*update the height & width*/
	pAppData->H264_TestCaseParams->width = 176;
	pAppData->H264_TestCaseParams->height = 144;


	H264CLIENT_TRACE_PRINT
	    ("Allocate the resources for the state trasition to Idle to complete");
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (tPortDef.nPortIndex == H264_APP_INPUTPORT)
		{
			pAppData->pInBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pInBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/*USE Buffer calls */
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{

				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), TIMM_OSAL_TRUE,
				    0, TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				//pTmpBuffer = (((OMX_U8)pTmpBuffer + 11) & (~11));
				pTmpBuffer = 0x14;
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pInBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == H264_APP_OUTPUTPORT)
		{
			pAppData->pOutBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pOutBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				/*USE Buffer calls */
				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
/*Free Buffers*/
	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (i == H264_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pInBuff[j]->
					    pBuffer);
				}
				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pInBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		} else if (i == H264_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pOutBuff[j]->
					    pBuffer);
				}

				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pOutBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		}
	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE OMXH264Enc_SizeLessthanMinBufferRequirements(H264E_ILClient *
    pApplicationData)
{
	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U8 *pTmpBuffer;	/*Used with Use Buffer calls */
	OMX_U32 i, j;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	/*Initialize the structure */
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);


	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
	/*update the height & width */
	pAppData->H264_TestCaseParams->width = 176;
	pAppData->H264_TestCaseParams->height = 100;

	H264CLIENT_TRACE_PRINT
	    ("Allocate the resources for the state trasition to Idle to complete");
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (tPortDef.nPortIndex == H264_APP_INPUTPORT)
		{
			pAppData->pInBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pInBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/*USE Buffer calls */
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{

				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), TIMM_OSAL_TRUE,
				    0, TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);

				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pInBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == H264_APP_OUTPUTPORT)
		{
			pAppData->pOutBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pOutBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				/*USE Buffer calls */
				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
/*Free Buffers*/
	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (i == H264_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pInBuff[j]->
					    pBuffer);
				}
				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pInBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		} else if (i == H264_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pOutBuff[j]->
					    pBuffer);
				}

				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pOutBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		}
	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;



}

OMX_ERRORTYPE OMXH264Enc_LessthanMinBufferRequirementsNum(H264E_ILClient *
    pApplicationData)
{

	H264E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U8 *pTmpBuffer;	/*Used with Use Buffer calls */
	OMX_U32 i, j;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	/*Initialize the structure */
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);


	H264CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&H264VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&H264VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = H264ENC_EventHandler;
	appCallbacks.EmptyBufferDone = H264ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = H264ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the H264ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle, (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.H264E",
	    pAppData, &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	H264CLIENT_TRACE_PRINT("call to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
	/*update the height & width */
	pAppData->H264_TestCaseParams->width = 176;
	pAppData->H264_TestCaseParams->height = 144;

	H264CLIENT_TRACE_PRINT
	    ("Allocate the resources for the state trasition to Idle to complete");
	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, &tPortParams);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (tPortDef.nPortIndex == H264_APP_INPUTPORT)
		{
			pAppData->pInBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pInBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/*USE Buffer calls */
			for (j = 0; j < tPortDef.nBufferCountActual - 1; j++)
			{

				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), TIMM_OSAL_TRUE,
				    0, TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);

				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pInBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == H264_APP_OUTPUTPORT)
		{
			pAppData->pOutBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pOutBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				/*USE Buffer calls */
				pTmpBuffer =
				    TIMM_OSAL_Malloc(((pAppData->
					    H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->H264_TestCaseParams->width *
					    pAppData->H264_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	H264CLIENT_TRACE_PRINT("call to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
/*Free Buffers*/
	for (i = 0; i < tPortParams.nPorts; i++)
	{
		tPortDef.nPortIndex = i;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition,
		    &tPortDef);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (i == H264_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual - 1; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pInBuff[j]->
					    pBuffer);
				}
				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pInBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		} else if (i == H264_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->H264_TestCaseParams->
				    bInAllocatebuffer)
				{
					TIMM_OSAL_Free(pAppData->pOutBuff[j]->
					    pBuffer);
				}

				eError =
				    OMX_FreeBuffer(pAppData->pHandle,
				    tPortDef.nPortIndex,
				    pAppData->pOutBuff[j]);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}
		}
	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = H264ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	H264CLIENT_TRACE_PRINT("Got the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	H264CLIENT_TRACE_PRINT("call to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	H264CLIENT_TRACE_PRINT("call OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(H264VE_Events);
	TIMM_OSAL_EventDelete(H264VE_CmdEvent);


	if (eError != OMX_ErrorNone)
	{
		H264CLIENT_ERROR_PRINT("%s", H264_GetErrorString(eError));
	}

	H264CLIENT_EXIT_PRINT(eError);
	return eError;


}
