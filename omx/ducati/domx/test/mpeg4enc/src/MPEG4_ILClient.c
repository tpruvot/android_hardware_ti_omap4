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
*   @file  MPEG4_ILClient.c
*   This file contains the structures and macros that are used in the Application which tests the OpenMAX component
*
*  @path \WTSD_DucatiMMSW\omx\khronos1.1\omx_MPEG4_enc\test
*
*  @rev 1.0
*/

/****************************************************************
 * INCLUDE FILES
 ****************************************************************/
/**----- system and platform files ----------------------------**/





/**-------program files ----------------------------------------**/
#include "MPEG4_ILClient.h"
#ifdef OMX_TILERTEST
#define OMX_MPEG4E_TILERTEST
#ifdef MPEG4_LINUX_CLIENT
#define OMX_MPEG4E_LINUX_TILERTEST
#else
#define OMX_MPEG4E_DUCATIAPPM3_TILERTEST
#endif
#else
#ifdef MPEG4_LINUX_CLIENT
		/*Enable the corresponding flags */
		/*HeapBuf & Shared Region Defines */
#define OMX_MPEG4E_SRCHANGES
#define OMX_MPEG4E_BUF_HEAP
#endif
#define OMX_MPEG4E_NONTILERTEST
    //#define TIMESTAMPSPRINT

#endif

#ifdef OMX_MPEG4E_BUF_HEAP
#include <unistd.h>
#include <RcmClient.h>
#include <HeapBuf.h>
#include <SharedRegion.h>
#endif

#ifdef OMX_MPEG4E_LINUX_TILERTEST
	/*Tiler APIs */
#include <memmgr.h>
#else
	/*Tiler APIs */
#include <memmgr/memmgr.h>
#endif

#ifdef OMX_MPEG4E_BUF_HEAP
extern HeapBuf_Handle heapHandle;
#endif

#ifdef OMX_MPEG4E_DUCATIAPPM3_TILERTEST
#define STRIDE_8BIT (16 * 1024)
#define STRIDE_16BIT (32 * 1024)
#endif


#ifdef OMX_MPEG4E_LINUX_TILERTEST
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)
#endif


#define MPEG4_INPUT_READY	1
#define MPEG4_OUTPUT_READY	2
#define MPEG4_ERROR_EVENT	4
#define MPEG4_EOS_EVENT 8
#define MPEG4_STATETRANSITION_COMPLETE 16
#define MPEG4_APP_INPUTPORT 0
#define MPEG4_APP_OUTPUTPORT 1

#define NUMTIMESTOPAUSE 1

#define MPEG4_INPUT_BUFFERCOUNTACTUAL 5
#define MPEG4_OUTPUT_BUFFERCOUNTACTUAL 3
#define GETHANDLE_FREEHANDLE_LOOPCOUNT 3

#define MAXFRAMESTOREADFROMFILE 1000

/*Globals*/
static TIMM_OSAL_PTR MPEG4VE_Events;
static TIMM_OSAL_PTR MPEG4VE_CmdEvent;


static OMX_U32 nFramesRead = 0;	/*Frames read from the input file */
static OMX_U32 TempCount = 0;	/* loop count to test Pause->Exectuing */

static OMX_U32 OutputFilesize = 0;
static OMX_U32 MPEG4ClientStopFrameNum;


/*Global Settings*/
static OMX_U8 DynamicSettingsCount = 0;
static OMX_U8 DynamicSettingsArray[9];
static OMX_U8 DynFrameCountArray_Index[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static OMX_U8 DynFrameCountArray_FreeIndex[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static OMX_U8 PauseFrameNum[NUMTIMESTOPAUSE] = { 5 };

#define TODO_ENC_MPEG4 1	// temporary to be deleted later on


/*-------------------------function prototypes ------------------------------*/

/*Resource allocation & Free Functions*/
static OMX_ERRORTYPE MPEG4ENC_FreeResources(MPEG4E_ILClient * pAppData);
static OMX_ERRORTYPE MPEG4ENC_AllocateResources(MPEG4E_ILClient * pAppData);

/*Fill Data into the Buffer from the File for the ETB calls*/
OMX_ERRORTYPE MPEG4ENC_FillData(MPEG4E_ILClient * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf);

/*Static Configurations*/
static OMX_ERRORTYPE MPEG4ENC_SetAllParams(MPEG4E_ILClient * pAppData);
OMX_ERRORTYPE MPEG4ENC_SetAdvancedParams(MPEG4E_ILClient * pAppData);
OMX_ERRORTYPE MPEG4ENC_SetParamsFromInitialDynamicParams(MPEG4E_ILClient *
    pAppData);

/*Runtime Configurations*/
OMX_ERRORTYPE OMXMPEG4Enc_ConfigureRunTimeSettings(MPEG4E_ILClient *
    pApplicationData, OMX_MPEG4E_DynamicConfigs * MPEG4EConfigStructures,
    OMX_U32 FrameNumber);
OMX_ERRORTYPE MPEG4E_Populate_BitFieldSettings(OMX_U32 BitFileds,
    OMX_U8 * Count, OMX_U8 * Array);
OMX_ERRORTYPE MPEG4E_Update_DynamicSettingsArray(OMX_U8 RemCount,
    OMX_U32 * RemoveArray);


/*Wait for state transition complete*/
static OMX_ERRORTYPE MPEG4ENC_WaitForState(MPEG4E_ILClient * pAppData);

/*Call Back Functions*/
OMX_ERRORTYPE MPEG4ENC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData);

OMX_ERRORTYPE MPEG4ENC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);

OMX_ERRORTYPE MPEG4ENC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer);

/*Error Handling*/
void OMXMPEG4Enc_Client_ErrorHandling(MPEG4E_ILClient * pApplicationData,
    OMX_U32 nErrorIndex, OMX_ERRORTYPE eGenError);
OMX_STRING MPEG4_GetErrorString(OMX_ERRORTYPE error);

#ifndef MPEG4_LINUX_CLIENT
/*Main Entry Function*/
void OMXMPEG4Enc_TestEntry(Int32 paramSize, void *pParam);
#endif
/*Complete Functionality Implementation*/
OMX_ERRORTYPE OMXMPEG4Enc_CompleteFunctionality(MPEG4E_ILClient *
    pApplicationData);

/*In Executing Loop Functionality*/
OMX_ERRORTYPE OMXMPEG4Enc_InExecuting(MPEG4E_ILClient * pApplicationData);

/*Dynamic Port Configuration*/
OMX_ERRORTYPE OMXMPEG4Enc_DynamicPortConfiguration(MPEG4E_ILClient *
    pApplicationData, OMX_U32 nPortIndex);


/*Tiler Buffer <-> 1D*/
OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_1Dto2D(OMX_PTR pDst2D, OMX_PTR pSrc1D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D);
OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_2Dto1D(OMX_PTR pDst1D, OMX_PTR pSrc2D,
    OMX_U32 nSize1D, OMX_U32 nHeight2D, OMX_U32 nWidth2D, OMX_U32 nStride2D);

/*additional Testcases*/
OMX_ERRORTYPE OMXMPEG4Enc_GetHandle_FreeHandle(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXMPEG4Enc_LoadedToIdlePortDisable(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXMPEG4Enc_LoadedToIdlePortEnable(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXMPEG4Enc_LessthanMinBufferRequirementsNum(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXMPEG4Enc_SizeLessthanMinBufferRequirements(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE OMXMPEG4Enc_UnalignedBuffers(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE MPEG4E_FreeDynamicClientParams(MPEG4E_ILClient *
    pApplicationData);
OMX_ERRORTYPE MPEG4E_SetClientParams(MPEG4E_ILClient * pApplicationData);


static OMX_ERRORTYPE MPEG4ENC_FreeResources(MPEG4E_ILClient * pAppData)
{
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U32 i, j;

	GOTO_EXIT_IF(!pAppData, OMX_ErrorBadParameter);

	MPEG4CLIENT_ENTER_PRINT();

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
		if (i == MPEG4_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
				    bInAllocatebuffer)
				{
#ifdef OMX_MPEG4E_BUF_HEAP
					HeapBuf_free(heapHandle,
					    pAppData->pInBuff[j]->pBuffer,
					    pAppData->pInBuff[j]->nAllocLen);
#elif defined (OMX_MPEG4E_LINUX_TILERTEST)
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
		} else if (i == MPEG4_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
				    bInAllocatebuffer)
				{
#ifdef OMX_MPEG4E_BUF_HEAP
					HeapBuf_free(heapHandle,
					    pAppData->pOutBuff[j]->pBuffer,
					    pAppData->pOutBuff[j]->nAllocLen);
#elif defined (OMX_MPEG4E_LINUX_TILERTEST)
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}


static OMX_ERRORTYPE MPEG4ENC_AllocateResources(MPEG4E_ILClient * pAppData)
{
	OMX_U32 retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_PORT_PARAM_TYPE tPortParams;
	OMX_U8 *pTmpBuffer;	/*Used with Use Buffer calls */
	OMX_U32 i, j;

#ifdef OMX_MPEG4E_LINUX_TILERTEST
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;
#endif
	MPEG4CLIENT_ENTER_PRINT();
	GOTO_EXIT_IF(!pAppData, OMX_ErrorBadParameter);

	pHandle = pAppData->pHandle;
#ifdef OMX_MPEG4E_LINUX_TILERTEST
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
		if (tPortDef.nPortIndex == MPEG4_APP_INPUTPORT)
		{
			pAppData->pInBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pInBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/* Create a pipes for Input Buffers.. used to queue data from the callback. */
			MPEG4CLIENT_TRACE_PRINT("\nCreating i/p pipe\n");
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
				if (pAppData->MPEG4_TestCaseParams->
				    bInAllocatebuffer)
				{
					/*Min Buffer requirements depends on the Amount of Stride.....here No padding so Width==Stride */
					eError =
					    OMX_AllocateBuffer(pAppData->
					    pHandle, &(pAppData->pInBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    ((pAppData->MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
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
#ifdef OMX_MPEG4E_BUF_HEAP
					pTmpBuffer =
					    HeapBuf_alloc(heapHandle,
					    (pAppData->MPEG4_TestCaseParams->
						width *
						pAppData->
						MPEG4_TestCaseParams->height *
						3 / 2), 0);

#elif defined (OMX_MPEG4E_LINUX_TILERTEST)
					MemReqDescTiler[0].pixelFormat =
					    PIXEL_FMT_8BIT;
					MemReqDescTiler[0].dim.area.width = pAppData->MPEG4_TestCaseParams->width;	/*width */
					MemReqDescTiler[0].dim.area.height = pAppData->MPEG4_TestCaseParams->height;	/*height */
					MemReqDescTiler[0].stride =
					    STRIDE_8BIT;

					MemReqDescTiler[1].pixelFormat =
					    PIXEL_FMT_16BIT;
					MemReqDescTiler[1].dim.area.width = pAppData->MPEG4_TestCaseParams->width / 2;	/*width */
					MemReqDescTiler[1].dim.area.height = pAppData->MPEG4_TestCaseParams->height / 2;	/*height */
					MemReqDescTiler[1].stride =
					    STRIDE_16BIT;

					//MemReqDescTiler.reserved
					/*call to tiler Alloc */
					MPEG4CLIENT_TRACE_PRINT
					    ("\nBefore tiler alloc for the Input buffer \n");
					TilerAddr =
					    MemMgr_Alloc(MemReqDescTiler, 2);
					MPEG4CLIENT_TRACE_PRINT
					    ("\nTiler buffer allocated is %x\n",
					    TilerAddr);
					pTmpBuffer = (OMX_U8 *) TilerAddr;

#else
					pTmpBuffer =
					    TIMM_OSAL_Malloc(((pAppData->
						    MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
						    height * 3 / 2)),
					    TIMM_OSAL_TRUE, 32,
					    TIMMOSAL_MEM_SEGMENT_EXT);
#endif
					GOTO_EXIT_IF((pTmpBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer before UB = %x\n",
					    pTmpBuffer);
					pTmpBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pTmpBuffer,
					    2);
					MPEG4CLIENT_TRACE_PRINT
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
					    ((pAppData->MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
						    height * 3 / 2)),
					    pTmpBuffer);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer SR after UB = %x\n",
					    pAppData->pInBuff[j]->pBuffer);
					pAppData->pInBuff[j]->pBuffer =
					    SharedRegion_getPtr(pAppData->
					    pInBuff[j]->pBuffer);
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer after UB = %x\n",
					    pAppData->pInBuff[j]->pBuffer);
#endif
					MPEG4CLIENT_TRACE_PRINT
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

			MPEG4CLIENT_TRACE_PRINT
			    ("\nSetting i/p ready event\n");
			retval =
			    TIMM_OSAL_EventSet(MPEG4VE_Events,
			    MPEG4_INPUT_READY, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			MPEG4CLIENT_TRACE_PRINT("\ni/p ready event set\n");
		} else if (tPortDef.nPortIndex == MPEG4_APP_OUTPUTPORT)
		{

			pAppData->pOutBuff =
			    TIMM_OSAL_Malloc((sizeof(OMX_BUFFERHEADERTYPE *) *
				tPortDef.nBufferCountActual), TIMM_OSAL_TRUE,
			    0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->pOutBuff == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/* Create a pipes for Output Buffers.. used to queue data from the callback. */
			MPEG4CLIENT_TRACE_PRINT("\nCreating o/p pipe\n");
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
				if (pAppData->MPEG4_TestCaseParams->
				    bOutAllocatebuffer)
				{
					/*Min Buffer requirements depends on the Amount of Stride.....here No padding so Width==Stride */
					eError =
					    OMX_AllocateBuffer(pAppData->
					    pHandle, &(pAppData->pOutBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    (pAppData->MPEG4_TestCaseParams->
						width *
						pAppData->
						MPEG4_TestCaseParams->
						height));
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
#ifdef OMX_MPEG4E_BUF_HEAP
					pTmpBuffer =
					    HeapBuf_alloc(heapHandle,
					    (pAppData->MPEG4_TestCaseParams->
						width *
						pAppData->
						MPEG4_TestCaseParams->height),
					    0);

#elif defined (OMX_MPEG4E_LINUX_TILERTEST)
					MemReqDescTiler[0].pixelFormat =
					    PIXEL_FMT_PAGE;
					MemReqDescTiler[0].dim.len =
					    pAppData->MPEG4_TestCaseParams->
					    width *
					    pAppData->MPEG4_TestCaseParams->
					    height;
					MemReqDescTiler[0].stride = 0;
					//MemReqDescTiler.stride
					//MemReqDescTiler.reserved
					/*call to tiler Alloc */
					MPEG4CLIENT_TRACE_PRINT
					    ("\nBefore tiler alloc for the Output buffer \n");
					TilerAddr =
					    MemMgr_Alloc(MemReqDescTiler, 1);
					MPEG4CLIENT_TRACE_PRINT
					    ("\nTiler buffer allocated is %x\n",
					    TilerAddr);
					pTmpBuffer = (OMX_U8 *) TilerAddr;
#else
					pTmpBuffer =
					    TIMM_OSAL_Malloc(((pAppData->
						    MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
						    height)), TIMM_OSAL_TRUE,
					    0, TIMMOSAL_MEM_SEGMENT_EXT);
#endif
					GOTO_EXIT_IF((pTmpBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer before UB = %x\n",
					    pTmpBuffer);
					pTmpBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pTmpBuffer,
					    2);
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer SR before UB = %x\n",
					    pTmpBuffer);
#endif
					MPEG4CLIENT_TRACE_PRINT
					    ("\ncall to use buffer\n");
					eError =
					    OMX_UseBuffer(pAppData->pHandle,
					    &(pAppData->pOutBuff[j]),
					    tPortDef.nPortIndex, pAppData,
					    ((pAppData->MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
						    height)), pTmpBuffer);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer SR after UB = %x\n",
					    pAppData->pOutBuff[j]->pBuffer);
					pAppData->pOutBuff[j]->pBuffer =
					    SharedRegion_getPtr(pAppData->
					    pOutBuff[j]->pBuffer);
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer after UB = %x\n",
					    pAppData->pOutBuff[j]->pBuffer);
#endif

					MPEG4CLIENT_TRACE_PRINT
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

			MPEG4CLIENT_TRACE_PRINT
			    ("\nSetting o/p ready event\n");
			retval =
			    TIMM_OSAL_EventSet(MPEG4VE_Events,
			    MPEG4_OUTPUT_READY, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			MPEG4CLIENT_TRACE_PRINT("\no/p ready event set\n");
		} else
		{
			MPEG4CLIENT_TRACE_PRINT
			    ("Port index Assigned is Neither Input Nor Output");
			eError = OMX_ErrorBadPortIndex;
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		}
	}
#ifdef OMX_MPEG4E_LINUX_TILERTEST
	if (MemReqDescTiler)
		TIMM_OSAL_Free(MemReqDescTiler);

#endif
      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_STRING MPEG4_GetErrorString(OMX_ERRORTYPE error)
{
	OMX_STRING errorString;
	MPEG4CLIENT_ENTER_PRINT();

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


OMX_ERRORTYPE MPEG4ENC_FillData(MPEG4E_ILClient * pAppData,
    OMX_BUFFERHEADERTYPE * pBuf)
{
	OMX_U32 nRead = 0;
	OMX_U32 framesizetoread = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifdef OMX_MPEG4E_TILERTEST
	OMX_U8 *pTmpBuffer = NULL, *pActualPtr = NULL;
	OMX_U32 size1D = 0;
#endif
#ifdef  OMX_MPEG4E_LINUX_TILERTEST
	OMX_U32 Addoffset = 0;
#endif

	MPEG4CLIENT_ENTER_PRINT();

	framesizetoread =
	    pAppData->MPEG4_TestCaseParams->width *
	    pAppData->MPEG4_TestCaseParams->height * 3 / 2;

	/*Tiler Interface code */
#ifdef OMX_MPEG4E_TILERTEST
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
	    ((MPEG4ClientStopFrameNum - 1) > nFramesRead))
	{
		nFramesRead++;
		MPEG4CLIENT_TRACE_PRINT
		    ("\nnumber of bytes read from %d frame=%d", nFramesRead,
		    nRead);
		eError =
		    OMXMPEG4_Util_Memcpy_1Dto2D(pBuf->pBuffer, pTmpBuffer,
		    size1D, pAppData->MPEG4_TestCaseParams->height,
		    pAppData->MPEG4_TestCaseParams->width, STRIDE_8BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
		if (nRead <
		    (pAppData->MPEG4_TestCaseParams->height *
			pAppData->MPEG4_TestCaseParams->width))
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
			    (pAppData->MPEG4_TestCaseParams->height *
			    pAppData->MPEG4_TestCaseParams->width);
			size1D -=
			    (pAppData->MPEG4_TestCaseParams->height *
			    pAppData->MPEG4_TestCaseParams->width);
		}
#ifdef OMX_MPEG4E_DUCATIAPPM3_TILERTEST
		eError =
		    OMXMPEG4_Util_Memcpy_1Dto2D(((OMX_TI_PLATFORMPRIVATE *)
			pBuf->pPlatformPrivate)->pAuxBuf1, pTmpBuffer, size1D,
		    (pAppData->MPEG4_TestCaseParams->height / 2),
		    pAppData->MPEG4_TestCaseParams->width, STRIDE_16BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif
#ifdef OMX_MPEG4E_LINUX_TILERTEST
		Addoffset =
		    (pAppData->MPEG4_TestCaseParams->height * STRIDE_8BIT);
		eError =
		    OMXMPEG4_Util_Memcpy_1Dto2D((pBuf->pBuffer + Addoffset),
		    pTmpBuffer, size1D,
		    (pAppData->MPEG4_TestCaseParams->height / 2),
		    pAppData->MPEG4_TestCaseParams->width, STRIDE_16BIT);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif

	} else
	{
		/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
		if (MPEG4ClientStopFrameNum == MAXFRAMESTOREADFROMFILE)
		{
			/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
			MPEG4ClientStopFrameNum = nFramesRead;
		}
		nFramesRead++;
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
		MPEG4CLIENT_TRACE_PRINT
		    ("end of file reached and EOS flag has been set");
	}

#endif

#ifdef OMX_MPEG4E_NONTILERTEST
  /**********************Original Code*************************************************/
	if ((!feof(pAppData->fIn)) &&
	    ((nRead =
		    fread(pBuf->pBuffer, sizeof(OMX_U8), framesizetoread,
			pAppData->fIn)) != 0) &&
	    ((MPEG4ClientStopFrameNum - 1) > nFramesRead))
	{

		MPEG4CLIENT_TRACE_PRINT
		    ("\nnumber of bytes read from %d frame=%d", nFramesRead,
		    nRead);
		nFramesRead++;

	} else
	{
		/*incase of partial record update the EOS */
		if (MPEG4ClientStopFrameNum == MAXFRAMESTOREADFROMFILE)
		{
			/*Incase of Full Record update the StopFrame nUmber to limit the ETB & FTB calls */
			MPEG4ClientStopFrameNum = nFramesRead;

		}
		nFramesRead++;
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
		MPEG4CLIENT_TRACE_PRINT
		    ("end of file reached and EOS flag has been set");
	}
	/**********************Original Code  ENDS*************************************************/
#endif
	pBuf->nFilledLen = nRead;
	pBuf->nOffset = 0;	/*offset is Zero */
	pBuf->nTimeStamp = (OMX_S64) nFramesRead;
      EXIT:
#ifdef OMX_MPEG4E_TILERTEST
	if (pActualPtr != NULL)
	{
		TIMM_OSAL_Free(pActualPtr);
	}
#endif
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_ERRORTYPE MPEG4E_Populate_BitFieldSettings(OMX_U32 BitFields,
    OMX_U8 * Count, OMX_U8 * Array)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 j, LCount = 0, Temp[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	OMX_S8 i;
	OMX_U32 Value = 0;

	MPEG4CLIENT_ENTER_PRINT();
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE MPEG4E_Update_DynamicSettingsArray(OMX_U8 RemCount,
    OMX_U32 * RemoveArray)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 i, j, LCount = 0, LRemcount = 0;
	OMX_U8 LocalDynArray[9], LocalDynArray_Index[9];
	OMX_BOOL bFound = OMX_FALSE;

	MPEG4CLIENT_ENTER_PRINT();

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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}


OMX_ERRORTYPE MPEG4ENC_SetParamsFromInitialDynamicParams(MPEG4E_ILClient *
    pApplicationData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MPEG4E_ILClient *pAppData;
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

	MPEG4CLIENT_ENTER_PRINT();

	OMX_TEST_INIT_STRUCT_PTR(&tAVCParams, OMX_VIDEO_PARAM_AVCTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tVidEncBitRate,
	    OMX_VIDEO_PARAM_BITRATETYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tVideoParams,
	    OMX_VIDEO_PARAM_PORTFORMATTYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortParams, OMX_PORT_PARAM_TYPE);
	OMX_TEST_INIT_STRUCT_PTR(&tPortDef, OMX_PARAM_PORTDEFINITIONTYPE);

	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;
	pHandle = pAppData->pHandle;

	eError =
	    MPEG4E_Populate_BitFieldSettings(pAppData->MPEG4_TestCaseParams->
	    nBitEnableDynamic, &DynamicSettingsCount, DynamicSettingsArray);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	for (i = 0; i < DynamicSettingsCount; i++)
	{
		switch (DynamicSettingsArray[i])
		{
		case 0:
			/*Frame rate */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bFramerate = OMX_TRUE;
				LFrameRate =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynFrameRate.
				    nFramerate[DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}

			break;
		case 1:
			/*Bitrate */
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bBitrate = OMX_TRUE;
				LBitRate =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynBitRate.
				    nBitrate[DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;
			}
			/*check to remove from the DynamicSettingsArray list */
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 2:
			/*set config to OMX_TI_VIDEO_CONFIG_ME_SEARCHRANGE */

			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tMESearchrange,
				    OMX_VIDEO_CONFIG_MESEARCHRANGETYPE);
				tMESearchrange.nPortIndex =
				    MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &tMESearchrange);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tMESearchrange.eMVAccuracy =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nMVAccuracy[DynFrameCountArray_Index[i]];
				tMESearchrange.nHorSearchRangeP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nHorSearchRangeP[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nVerSearchRangeP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nVerSearchRangeP[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nHorSearchRangeB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nHorSearchRangeB[DynFrameCountArray_Index
				    [i]];
				tMESearchrange.nVerSearchRangeB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 3:
			/*set config to OMX_CONFIG_INTRAREFRESHVOPTYPE */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tForceFrame,
				    OMX_CONFIG_INTRAREFRESHVOPTYPE);
				tForceFrame.nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &tForceFrame);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tForceFrame.IntraRefreshVOP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 4:
			/*set config to OMX_TI_VIDEO_CONFIG_QPSETTINGS */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tQPSettings,
				    OMX_VIDEO_CONFIG_QPSETTINGSTYPE);
				tQPSettings.nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &tQPSettings);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tQPSettings.nQpI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMinI[DynFrameCountArray_Index[i]];
				tQPSettings.nQpP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMinP[DynFrameCountArray_Index[i]];
				tQPSettings.nQpOffsetB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpOffsetB[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMaxB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxB[DynFrameCountArray_Index[i]];
				tQPSettings.nQpMinB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 5:
			/*set config to OMX_VIDEO_CONFIG_AVCINTRAPERIOD */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				bIntraperiod = OMX_TRUE;
				LIntraPeriod =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynIntraFrmaeInterval.
				    nIntraFrameInterval
				    [DynFrameCountArray_Index[i]];
				DynFrameCountArray_Index[i]++;

			}
			/*check to remove from the DynamicSettingsArray list */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 6:
			/*set config to OMX_IndexConfigVideoNalSize */
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{
				/*Set Port Index */
				OMX_TEST_INIT_STRUCT_PTR(&tNALUSize,
				    OMX_VIDEO_CONFIG_NALSIZE);
				tNALUSize.nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tNALUSize.nNaluBytes =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		case 7:
			/*set config to OMX_TI_VIDEO_CONFIG_SLICECODING */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tSliceCodingparams,
				    OMX_VIDEO_CONFIG_SLICECODINGTYPE);
				tSliceCodingparams.nPortIndex =
				    MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigSliceSettings,
				    &tSliceCodingparams);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tSliceCodingparams.eSliceMode =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynSliceSettings.
				    eSliceMode[DynFrameCountArray_Index[i]];
				tSliceCodingparams.nSlicesize =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}

			break;
		case 8:
			/*set config to OMX_TI_VIDEO_CONFIG_PIXELINFO */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] == 0)
			{

				OMX_TEST_INIT_STRUCT_PTR(&tPixelInfo,
				    OMX_VIDEO_CONFIG_PIXELINFOTYPE);
				tPixelInfo.nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    (OMX_INDEXTYPE)
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &tPixelInfo);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				tPixelInfo.nWidth =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynPixelInfo.
				    nWidth[DynFrameCountArray_Index[i]];
				tPixelInfo.nHeight =
				    pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] == -1)
			{
				nRemoveList[RemoveIndex++] =
				    DynamicSettingsArray[i];
			}
			break;
		default:
			MPEG4CLIENT_ERROR_PRINT("Invalid inputs");
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
			if (pAppData->MPEG4_TestCaseParams->TestCaseId == 6)
			{
				tPortDef.nBufferCountActual = MPEG4_INPUT_BUFFERCOUNTACTUAL;	/*currently hard coded & can be taken from test parameters */
			} else
			{
				/*check for the valid Input buffer count that the user configured with */
				if (pAppData->MPEG4_TestCaseParams->
				    nNumInputBuf >=
				    pAppData->MPEG4_TestCaseParams->
				    maxInterFrameInterval)
				{
					tPortDef.nBufferCountActual =
					    pAppData->MPEG4_TestCaseParams->
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
			    pAppData->MPEG4_TestCaseParams->width;
			tPortDef.format.video.nStride =
			    pAppData->MPEG4_TestCaseParams->width;
			tPortDef.format.video.nFrameHeight =
			    pAppData->MPEG4_TestCaseParams->height;
			tPortDef.format.video.eColorFormat =
			    pAppData->MPEG4_TestCaseParams->inputChromaFormat;

			/*settings for OMX_IndexParamVideoPortFormat */
			tVideoParams.eColorFormat =
			    pAppData->MPEG4_TestCaseParams->inputChromaFormat;

		}
		if (tPortDef.eDir == OMX_DirOutput)
		{
			/*set the actual number of buffers required */
			if (pAppData->MPEG4_TestCaseParams->TestCaseId == 6)
			{
				tPortDef.nBufferCountActual = MPEG4_OUTPUT_BUFFERCOUNTACTUAL;	/*currently hard coded & can be taken from test parameters */
			} else
			{
				if (pAppData->MPEG4_TestCaseParams->
				    maxInterFrameInterval > 1)
				{
					/*check for the valid Output buffer count that the user configured with */
					if (pAppData->MPEG4_TestCaseParams->
					    nNumOutputBuf >= 1)
					{
						tPortDef.nBufferCountActual =
						    pAppData->
						    MPEG4_TestCaseParams->
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
					if (pAppData->MPEG4_TestCaseParams->
					    nNumOutputBuf >= 1)
					{
						tPortDef.nBufferCountActual =
						    pAppData->
						    MPEG4_TestCaseParams->
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
			    pAppData->MPEG4_TestCaseParams->width;
			tPortDef.format.video.nStride =
			    pAppData->MPEG4_TestCaseParams->width;
			tPortDef.format.video.nFrameHeight =
			    pAppData->MPEG4_TestCaseParams->height;
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
		tAVCParams.eProfile = pAppData->MPEG4_TestCaseParams->profile;
		tAVCParams.eLevel = pAppData->MPEG4_TestCaseParams->level;
		if (bIntraperiod == OMX_TRUE)
		{
			tAVCParams.nPFrames = LIntraPeriod;
		}
		tAVCParams.nBFrames =
		    pAppData->MPEG4_TestCaseParams->maxInterFrameInterval - 1;
		tAVCParams.bEnableFMO = pAppData->MPEG4_TestCaseParams->bFMO;
		tAVCParams.bEntropyCodingCABAC =
		    pAppData->MPEG4_TestCaseParams->bCABAC;
		tAVCParams.bconstIpred =
		    pAppData->MPEG4_TestCaseParams->bConstIntraPred;
		tAVCParams.eLoopFilterMode =
		    pAppData->MPEG4_TestCaseParams->bLoopFilter;
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
	    pAppData->MPEG4_TestCaseParams->RateControlAlg;
	if (bBitrate)
	{
		tVidEncBitRate.nTargetBitrate = LBitRate;
	}
	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamVideoBitrate,
	    &tVidEncBitRate);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	/*Update the DynamicSettingsArray */
	eError = MPEG4E_Update_DynamicSettingsArray(RemoveIndex, nRemoveList);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}



static OMX_ERRORTYPE MPEG4ENC_SetAllParams(MPEG4E_ILClient * pAppData)
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

	MPEG4CLIENT_ENTER_PRINT();

	if (!pAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pHandle = pAppData->pHandle;

	/*Profile Level settings */
	OMX_TEST_INIT_STRUCT_PTR(&tProfileLevel,
	    OMX_VIDEO_PARAM_PROFILELEVELTYPE);
	tProfileLevel.nPortIndex = MPEG4_APP_OUTPUTPORT;

	eError =
	    OMX_GetParameter(pHandle, OMX_IndexParamVideoProfileLevelCurrent,
	    &tProfileLevel);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tProfileLevel.eProfile = pAppData->MPEG4_TestCaseParams->profile;
	tProfileLevel.eLevel = pAppData->MPEG4_TestCaseParams->level;

	eError =
	    OMX_SetParameter(pHandle, OMX_IndexParamVideoProfileLevelCurrent,
	    &tProfileLevel);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*BitStream Format settings */
	OMX_TEST_INIT_STRUCT_PTR(&tBitStreamFormat,
	    OMX_VIDEO_PARAM_AVCBITSTREAMFORMATTYPE);
	tBitStreamFormat.nPortIndex = MPEG4_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoBitStreamFormatSelect,
	    &tBitStreamFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	tBitStreamFormat.eStreamFormat =
	    pAppData->MPEG4_TestCaseParams->BitStreamFormat;
	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoBitStreamFormatSelect,
	    &tBitStreamFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Encoder Preset settings */
	OMX_TEST_INIT_STRUCT_PTR(&tEncoderPreset,
	    OMX_VIDEO_PARAM_ENCODER_PRESETTYPE);
	tEncoderPreset.nPortIndex = MPEG4_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoEncoderPreset,
	    &tEncoderPreset);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tEncoderPreset.eEncodingModePreset =
	    pAppData->MPEG4_TestCaseParams->EncodingPreset;
	tEncoderPreset.eRateControlPreset =
	    pAppData->MPEG4_TestCaseParams->RateCntrlPreset;

	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoEncoderPreset,
	    &tEncoderPreset);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Image Type settings */
	OMX_TEST_INIT_STRUCT_PTR(&tInputImageFormat,
	    OMX_VIDEO_PARAM_FRAMEDATACONTENTTYPE);
	tInputImageFormat.nPortIndex = MPEG4_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoFrameDataContentSettings,
	    &tInputImageFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	tInputImageFormat.eContentType =
	    pAppData->MPEG4_TestCaseParams->InputContentType;
	if (tInputImageFormat.eContentType != OMX_Video_Progressive)
	{
		tInputImageFormat.eInterlaceCodingType =
		    pAppData->MPEG4_TestCaseParams->InterlaceCodingType;
	}
	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoFrameDataContentSettings,
	    &tInputImageFormat);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Transform BlockSize settings */
	OMX_TEST_INIT_STRUCT_PTR(&tTransformBlockSize,
	    OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE);
	tTransformBlockSize.nPortIndex = MPEG4_APP_OUTPUTPORT;
	eError =
	    OMX_GetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoTransformBlockSize,
	    &tTransformBlockSize);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	tTransformBlockSize.eTransformBlocksize =
	    pAppData->MPEG4_TestCaseParams->TransformBlockSize;

	eError =
	    OMX_SetParameter(pHandle,
	    (OMX_INDEXTYPE) OMX_TI_IndexParamVideoTransformBlockSize,
	    &tTransformBlockSize);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	if ((pAppData->MPEG4_TestCaseParams->bFMO) &&
	    ((pAppData->MPEG4_TestCaseParams->nBitEnableAdvanced >> 1) == 0))
	{
		/*Basic FMO  settings */
		OMX_TEST_INIT_STRUCT_PTR(&tAVCFMO,
		    OMX_VIDEO_PARAM_AVCSLICEFMO);
		tAVCFMO.nPortIndex = MPEG4_APP_OUTPUTPORT;
		eError =
		    OMX_GetParameter(pHandle, OMX_IndexParamVideoSliceFMO,
		    &tAVCFMO);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

		tAVCFMO.nNumSliceGroups =
		    pAppData->MPEG4_TestCaseParams->nNumSliceGroups;
		tAVCFMO.nSliceGroupMapType =
		    pAppData->MPEG4_TestCaseParams->nSliceGroupMapType;
		tAVCFMO.eSliceMode =
		    pAppData->MPEG4_TestCaseParams->eSliceMode;

		eError =
		    OMX_SetParameter(pHandle, OMX_IndexParamVideoSliceFMO,
		    &tAVCFMO);
		GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	}
#if 0
	/*Set the Advanced Setting if any */
	eError = MPEG4ENC_SetAdvancedParams(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif

	/*Settings from Initial DynamicParams */
	eError = MPEG4ENC_SetParamsFromInitialDynamicParams(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

	/* This method will wait for the component to get to the desired state. */
static OMX_ERRORTYPE MPEG4ENC_WaitForState(MPEG4E_ILClient * pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents, numRemaining;

	MPEG4CLIENT_ENTER_PRINT();

	/* Wait for an event */
	uRequestedEvents =
	    (MPEG4_STATETRANSITION_COMPLETE | MPEG4_ERROR_EVENT);
	retval =
	    TIMM_OSAL_EventRetrieve(MPEG4VE_CmdEvent, uRequestedEvents,
	    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents, TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	if (pRetrievedEvents & MPEG4_ERROR_EVENT)
	{
		eError = pAppData->eAppError;
	} else
	{
		/*transition Complete */
		eError = OMX_ErrorNone;
	}

      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4ENC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	MPEG4CLIENT_ENTER_PRINT();

	if (!ptrAppData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		retval =
		    TIMM_OSAL_EventSet(MPEG4VE_CmdEvent,
		    MPEG4_STATETRANSITION_COMPLETE, TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		break;

	case OMX_EventError:
		MPEG4CLIENT_ERROR_PRINT
		    ("received from the component in EventHandler=%d",
		    (OMX_ERRORTYPE) nData1);
		retval =
		    TIMM_OSAL_EventSet(MPEG4VE_Events, MPEG4_ERROR_EVENT,
		    TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		retval =
		    TIMM_OSAL_EventSet(MPEG4VE_CmdEvent, MPEG4_ERROR_EVENT,
		    TIMM_OSAL_EVENT_OR);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorBadParameter);
		break;

	case OMX_EventBufferFlag:
		if (nData2 == OMX_BUFFERFLAG_EOS)
		{
			retval =
			    TIMM_OSAL_EventSet(MPEG4VE_Events,
			    MPEG4_EOS_EVENT, TIMM_OSAL_EVENT_OR);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
		}
		MPEG4CLIENT_TRACE_PRINT("Event: EOS has reached the client");
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4ENC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	MPEG4E_ILClient *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	OMX_U8 i = 0;
	MPEG4CLIENT_ENTER_PRINT();
#ifdef TIMESTAMPSPRINT		/*to check the prder of output timestamp information....will be removed later */
	if (pAppData->nFillBufferDoneCount < 20)
	{
		MPEG4CLIENT_TRACE_PRINT("\nthe Timestamp received =%ld",
		    pBuffer->nTimeStamp);
		i = pAppData->nFillBufferDoneCount;
		pAppData->TimeStampsArray[i] = pBuffer->nTimeStamp;
		MPEG4CLIENT_TRACE_PRINT
		    ("\ncount val= %d the Timestamp saved in the array[%d]= %d",
		    i, pAppData->nFillBufferDoneCount,
		    pAppData->TimeStampsArray[i]);
		pAppData->nFillBufferDoneCount++;
		MPEG4CLIENT_TRACE_PRINT("\ncount val after increment =%d",
		    pAppData->nFillBufferDoneCount);
	}
#endif
#ifdef OMX_MPEG4E_SRCHANGES
	MPEG4CLIENT_TRACE_PRINT("\npBuffer SR after FBD = %x\n",
	    pBuffer->pBuffer);
	pBuffer->pBuffer = SharedRegion_getPtr(pBuffer->pBuffer);
	MPEG4CLIENT_TRACE_PRINT("\npBuffer after FBD = %x\n",
	    pBuffer->pBuffer);
#endif

	MPEG4CLIENT_TRACE_PRINT("\nWrite the buffer to the Output pipe");
	retval =
	    TIMM_OSAL_WriteToPipe(pAppData->OpBuf_Pipe, &pBuffer,
	    sizeof(pBuffer), TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);
	MPEG4CLIENT_TRACE_PRINT("\nSet the Output Ready event");
	retval =
	    TIMM_OSAL_EventSet(MPEG4VE_Events, MPEG4_OUTPUT_READY,
	    TIMM_OSAL_EVENT_OR);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);


      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE MPEG4ENC_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_PTR ptrAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	MPEG4E_ILClient *pAppData = ptrAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	MPEG4CLIENT_ENTER_PRINT();

#ifdef OMX_MPEG4E_SRCHANGES
	MPEG4CLIENT_TRACE_PRINT("\npBuffer SR after EBD = %x\n",
	    pBuffer->pBuffer);
	pBuffer->pBuffer = SharedRegion_getPtr(pBuffer->pBuffer);
	MPEG4CLIENT_TRACE_PRINT("\npBuffer after EBD = %x\n",
	    pBuffer->pBuffer);
#endif
	MPEG4CLIENT_TRACE_PRINT("\nWrite the buffer to the Input pipe");
	retval =
	    TIMM_OSAL_WriteToPipe(pAppData->IpBuf_Pipe, &pBuffer,
	    sizeof(pBuffer), TIMM_OSAL_SUSPEND);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);
	MPEG4CLIENT_TRACE_PRINT("\nSet the Input Ready event");
	retval =
	    TIMM_OSAL_EventSet(MPEG4VE_Events, MPEG4_INPUT_READY,
	    TIMM_OSAL_EVENT_OR);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE), OMX_ErrorBadParameter);


      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

#ifdef MPEG4_LINUX_CLIENT
void main()
#else
void OMXMPEG4Enc_TestEntry(Int32 paramSize, void *pParam)
#endif
{
	MPEG4E_ILClient pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 test_index = 0, i;
	/*Memory leak detect */
	OMX_U32 mem_count_start = 0;
	OMX_U32 mem_size_start = 0;
	OMX_U32 mem_count_end = 0;
	OMX_U32 mem_size_end = 0;
#ifndef MPEG4_LINUX_CLIENT
	Memory_Stats stats;
#endif

#ifdef MPEG4_LINUX_CLIENT
	OMX_U32 setup;
#ifdef OMX_MPEG4E_LINUX_TILERTEST
	/*Only domx setup will be done by platform init since basic ipc setup has been
	   done by syslink daemon */
	setup = 2;
#else
	/*Will do basic ipc setup, call proc load and aetup domx as well */
	setup = 1;
#endif
	//Need to call mmplatform_init on linux
	//mmplatform_init(setup);
	MPEG4CLIENT_TRACE_PRINT
	    ("\nWait until RCM Server is created on other side. Press any key after that\n");
	getchar();
#endif

#ifndef MPEG4_LINUX_CLIENT
	/*Setting the Trace Group */
	TraceGrp = TIMM_OSAL_GetTraceGrp();
	TIMM_OSAL_SetTraceGrp((TraceGrp | (1 << 3)));	/*to VideoEncoder */

	MPEG4CLIENT_TRACE_PRINT("\ntracegroup=%x", TraceGrp);
#endif

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*allocate memory for pAppData.MPEG4_TestCaseParams */
	pAppData.MPEG4_TestCaseParams = NULL;
	pAppData.MPEG4_TestCaseParams =
	    (MPEG4E_TestCaseParams *)
	    TIMM_OSAL_Malloc(sizeof(MPEG4E_TestCaseParams), TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_EXT);
	GOTO_EXIT_IF((pAppData.MPEG4_TestCaseParams == TIMM_OSAL_NULL),
	    OMX_ErrorInsufficientResources);
	/*allocate memory for pAppData.MPEG4_TestCaseParamsDynamic */
	pAppData.MPEG4_TestCaseParamsDynamic = NULL;
	pAppData.MPEG4_TestCaseParamsDynamic =
	    (MPEG4E_TestCaseParamsDynamic *)
	    TIMM_OSAL_Malloc(sizeof(MPEG4E_TestCaseParamsDynamic),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	GOTO_EXIT_IF((pAppData.MPEG4_TestCaseParamsDynamic == TIMM_OSAL_NULL),
	    OMX_ErrorInsufficientResources);
	/*allocate memory for pAppData.MPEG4_TestCaseParamsAdvanced */
	pAppData.MPEG4_TestCaseParamsAdvanced = NULL;
	pAppData.MPEG4_TestCaseParamsAdvanced =
	    (MPEG4E_TestCaseParamsAdvanced *)
	    TIMM_OSAL_Malloc(sizeof(MPEG4E_TestCaseParamsAdvanced),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	GOTO_EXIT_IF((pAppData.MPEG4_TestCaseParamsAdvanced ==
		TIMM_OSAL_NULL), OMX_ErrorInsufficientResources);


	eError = MPEG4E_SetClientParams(&pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

#ifdef TIMESTAMPSPRINT		/*will be removed later */
	/*print the received timestamps information */
	for (i = 0; i < 20; i++)
	{
		pAppData.TimeStampsArray[i] = 0;
	}
	pAppData.nFillBufferDoneCount = 0;
#endif


#ifdef TODO_ENC_MPEG4		//temporary to be deleted later on
	pAppData.MPEG4_TestCaseParams->TestCaseId = 0;
#endif

	/*Check for the Test case ID */
	switch (pAppData.MPEG4_TestCaseParams->TestCaseId)
	{
	case 0:
		/*call to Total functionality */
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);
#if 0
		Memory_getStats(NULL, &stats);
		MPEG4CLIENT_INFO_PRINT("Total size = %d", stats.totalSize);
		MPEG4CLIENT_INFO_PRINT("Total free size = %d",
		    stats.totalFreeSize);
		MPEG4CLIENT_INFO_PRINT("Largest Free size = %d",
		    stats.largestFreeSize);
#endif

		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif

		eError = OMXMPEG4Enc_CompleteFunctionality(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif


		break;
	case 1:
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		/*call to gethandle & Freehandle: Simple Test */
		for (i = 0; i < GETHANDLE_FREEHANDLE_LOOPCOUNT; i++)
		{
			mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
			mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

			MPEG4CLIENT_INFO_PRINT
			    ("\nbefore Test case Start: Value from GetMemCounter = %d",
			    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
			MPEG4CLIENT_INFO_PRINT
			    ("\nbefore Test case Start: Value from GetMemUsage = %d",
			    mem_size_start);
#endif
			eError = OMXMPEG4Enc_GetHandle_FreeHandle(&pAppData);

			mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
			mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
			MPEG4CLIENT_INFO_PRINT
			    ("\n After Test case Run: Value from GetMemCounter = %d",
			    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
			MPEG4CLIENT_INFO_PRINT
			    ("\n After Test case Run: Value from GetMemUsage = %d",
			    mem_size_end);
			if (mem_count_start != mem_count_end)
			{
				MPEG4CLIENT_ERROR_PRINT
				    ("Memory leak detected. Bytes lost = %d",
				    (mem_size_end - mem_size_start));
			}
#endif
		}

		break;

	case 2:
		/*call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being Disabled */
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif

		eError = OMXMPEG4Enc_LoadedToIdlePortDisable(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif

		break;
	case 3:
		/*call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif
		eError = OMXMPEG4Enc_LoadedToIdlePortEnable(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif

		break;
	case 4:
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		/*Unaligned Buffers : client allocates the buffers (use buffer)
		   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif
		eError = OMXMPEG4Enc_UnalignedBuffers(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif
		break;
	case 5:
		/*less than the min size requirements : client allocates the buffers (use buffer)/component allocate (allocate buffer)
		   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with the required num of buffers */
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif

		eError =
		    OMXMPEG4Enc_SizeLessthanMinBufferRequirements(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif
		break;
	case 6:
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		/* Different Number of input & output buffers satisfying the min buffers requirements
		   complete functionality tested */
		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif

		eError = OMXMPEG4Enc_CompleteFunctionality(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif
		break;
	case 7:
		/* Different Number of input & output buffers Not satisfying the min buffers requirements
		   call to gethandle, Loaded->Idle, Idle->Loaded & Freehandle: Simple Test: ports being allocated with less num of buffers */
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		mem_count_start = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_start = TIMM_OSAL_GetMemUsage();
#endif

		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemCounter = %d",
		    mem_count_start);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\nbefore Test case Start: Value from GetMemUsage = %d",
		    mem_size_start);
#endif

		eError =
		    OMXMPEG4Enc_LessthanMinBufferRequirementsNum(&pAppData);

		mem_count_end = TIMM_OSAL_GetMemCounter();
#ifndef MPEG4_LINUX_CLIENT
		mem_size_end = TIMM_OSAL_GetMemUsage();
#endif
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemCounter = %d",
		    mem_count_end);
#ifndef MPEG4_LINUX_CLIENT
		MPEG4CLIENT_INFO_PRINT
		    ("\n After Test case Run: Value from GetMemUsage = %d",
		    mem_size_end);
		if (mem_count_start != mem_count_end)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Memory leak detected. Bytes lost = %d",
			    (mem_size_end - mem_size_start));
		}
#endif
		break;


	default:
		MPEG4CLIENT_TRACE_PRINT("\nRunning TestCaseId=%d",
		    pAppData.MPEG4_TestCaseParams->TestCaseId);

		MPEG4CLIENT_TRACE_PRINT("\nNot Implemented");
		//printf("\nNot Implemented\n");

	}
	OutputFilesize = 0;
	nFramesRead = 0;
	//TIMM_OSAL_EndMemDebug();
      EXIT:
	/*free memory for pAppData.MPEG4_TestCaseParams */
	if (pAppData.MPEG4_TestCaseParams)
	{
		TIMM_OSAL_Free(pAppData.MPEG4_TestCaseParams);
	}
	/*free memory for pAppData.MPEG4_TestCaseParamsDynamic */
	if (pAppData.MPEG4_TestCaseParamsDynamic)
	{
		/*free up the individual elements memory */
		eError = MPEG4E_FreeDynamicClientParams(&pAppData);
		if (eError != OMX_ErrorNone)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("Error during freeup of dynamic testcase param structures= %x",
			    eError);
		}
		TIMM_OSAL_Free(pAppData.MPEG4_TestCaseParamsDynamic);
	}
	/*free memory for pAppData.MPEG4_TestCaseParamsAdvanced */
	if (pAppData.MPEG4_TestCaseParamsAdvanced)
	{
		TIMM_OSAL_Free(pAppData.MPEG4_TestCaseParamsAdvanced);
	}
	MPEG4CLIENT_TRACE_PRINT("Reached the end of the programming");


}

OMX_ERRORTYPE OMXMPEG4Enc_CompleteFunctionality(MPEG4E_ILClient *
    pApplicationData)
{
	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	MPEG4CLIENT_ENTER_PRINT();
	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	pAppData->pHandle = pHandle;

#if 0
	/*Set the Component Static Params */
	MPEG4CLIENT_TRACE_PRINT
	    ("\nSet the Component Initial Parameters (in Loaded state)");
	eError = MPEG4ENC_SetAllParams(pAppData);
	if (eError != OMX_ErrorNone)
	{
		OMXMPEG4Enc_Client_ErrorHandling(pAppData, 1, eError);
		goto EXIT;
	}
#endif

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	if (eError != OMX_ErrorNone)
	{
		OMXMPEG4Enc_Client_ErrorHandling(pAppData, 1, eError);
		goto EXIT;
	}

	MPEG4CLIENT_TRACE_PRINT
	    ("\nAllocate the resources for the state trasition to Idle to complete");
	eError = MPEG4ENC_AllocateResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> executing state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = OMX_GetState(pHandle, &(pAppData->eState));
	MPEG4CLIENT_TRACE_PRINT
	    ("\nTrasition to executing state is completed");

	/*Executing Loop */
	eError = OMXMPEG4Enc_InExecuting(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*Command to goto Idle */
	MPEG4CLIENT_TRACE_PRINT("\ncall to goto executing ->Idle state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\nFree up the Component Resources");
	eError = MPEG4ENC_FreeResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/*close the files after processing has been done */
	MPEG4CLIENT_TRACE_PRINT("\nClose the Input & Output file");
	if (pAppData->fIn)
		fclose(pAppData->fIn);
	if (pAppData->fOut)
		fclose(pAppData->fOut);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_ERRORTYPE OMXMPEG4Enc_InExecuting(MPEG4E_ILClient * pApplicationData)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_BUFFERHEADERTYPE *pBufferIn = NULL;
	OMX_BUFFERHEADERTYPE *pBufferOut = NULL;
	OMX_U32 actualSize, fwritesize = 0, fflushstatus;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 uRequestedEvents, pRetrievedEvents, numRemaining;
	OMX_U8 FrameNumToProcess = 1, PauseFrameNumIndex = 0, i;	//,nCountFTB=1;
	OMX_MPEG4E_DynamicConfigs OMXConfigStructures;
	//OMX_BOOL bEOS=OMX_FALSE;
#ifdef OMX_MPEG4E_LINUX_TILERTEST
	MemAllocBlock *MemReqDescTiler;
	OMX_PTR TilerAddr = NULL;
#endif


	MPEG4CLIENT_ENTER_PRINT();
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
#ifdef OMX_MPEG4E_LINUX_TILERTEST
	MemReqDescTiler =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	GOTO_EXIT_IF((MemReqDescTiler == TIMM_OSAL_NULL),
	    OMX_ErrorInsufficientResources);
#endif

	/*Check: partial or full record & in case of partial record check for stop frame number */
	if (pAppData->MPEG4_TestCaseParams->TestType == PARTIAL_RECORD)
	{
		MPEG4ClientStopFrameNum =
		    pAppData->MPEG4_TestCaseParams->StopFrameNum;
		MPEG4CLIENT_TRACE_PRINT
		    ("\nStopFrame in Partial record case=%d\n",
		    MPEG4ClientStopFrameNum);
	} else if (pAppData->MPEG4_TestCaseParams->TestType == FULL_RECORD)
	{
		/*go ahead with processing until it reaches EOS..update the "MPEG4ClientStopFrameNum" with sufficiently big value: used in case of FULL RECORD */
		MPEG4ClientStopFrameNum = MAXFRAMESTOREADFROMFILE;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT("\nInvalid parameter:TestType ");
		goto EXIT;
	}
	while ((eError == OMX_ErrorNone) &&
	    ((pAppData->eState == OMX_StateExecuting) ||
		(pAppData->eState == OMX_StatePause)))
	{

		/* Wait for an event (input/output/error) */
		MPEG4CLIENT_TRACE_PRINT
		    ("\nWait for an event (input/output/error)\n");
		uRequestedEvents =
		    (MPEG4_INPUT_READY | MPEG4_OUTPUT_READY |
		    MPEG4_ERROR_EVENT | MPEG4_EOS_EVENT);
		retval =
		    TIMM_OSAL_EventRetrieve(MPEG4VE_Events, uRequestedEvents,
		    TIMM_OSAL_EVENT_OR_CONSUME, &pRetrievedEvents,
		    TIMM_OSAL_SUSPEND);
		MPEG4CLIENT_TRACE_PRINT("\nEvent recd. = %d\n",
		    pRetrievedEvents);
		GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
		    OMX_ErrorInsufficientResources);

		/*Check for Error Event & Exit upon receiving the Error */
		GOTO_EXIT_IF((pRetrievedEvents & MPEG4_ERROR_EVENT),
		    pAppData->eAppError);

		/*Check for OUTPUT READY Event */
		numRemaining = 0;
		if (pRetrievedEvents & MPEG4_OUTPUT_READY)
		{
			MPEG4CLIENT_TRACE_PRINT("\nin Output Ready Event");
			retval =
			    TIMM_OSAL_GetPipeReadyMessageCount(pAppData->
			    OpBuf_Pipe, &numRemaining);
			GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
			    OMX_ErrorBadParameter);
			MPEG4CLIENT_TRACE_PRINT("\nOutput Msg count=%d",
			    numRemaining);
			while (numRemaining)
			{
				/*read from the pipe */
				MPEG4CLIENT_TRACE_PRINT
				    ("\nRead Buffer from the Output Pipe to give the Next FTB call");
				retval =
				    TIMM_OSAL_ReadFromPipe(pAppData->
				    OpBuf_Pipe, &pBufferOut,
				    sizeof(pBufferOut), &actualSize,
				    TIMM_OSAL_SUSPEND);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorNotReady);
				if (pBufferOut->nFilledLen)
				{
					MPEG4CLIENT_TRACE_PRINT
					    ("\nWrite the data into output file before it is given for the next FTB call");
					fwritesize =
					    fwrite((pBufferOut->pBuffer +
						(pBufferOut->nOffset)),
					    sizeof(OMX_U8),
					    pBufferOut->nFilledLen,
					    pAppData->fOut);
					fflushstatus = fflush(pAppData->fOut);
					if (fflushstatus != 0)
						MPEG4CLIENT_ERROR_PRINT
						    ("\nWhile fflush operation");
					OutputFilesize +=
					    pBufferOut->nFilledLen;
				}

				MPEG4CLIENT_TRACE_PRINT
				    ("\ntimestamp received is=%ld  ",
				    pBufferOut->nTimeStamp);
				MPEG4CLIENT_TRACE_PRINT
				    ("\nfilledlen=%d  fwritesize=%d, OutputFilesize=%d",
				    pBufferOut->nFilledLen, fwritesize,
				    OutputFilesize);
				MPEG4CLIENT_TRACE_PRINT
				    ("\nupdate the nFilledLen & nOffset");
				pBufferOut->nFilledLen = 0;
				pBufferOut->nOffset = 0;
				MPEG4CLIENT_TRACE_PRINT
				    ("\nBefore the FTB call");
#ifdef OMX_MPEG4E_SRCHANGES
				pBufferOut->pBuffer =
				    (char *)SharedRegion_getSRPtr(pBufferOut->
				    pBuffer, 2);
#endif

				eError =
				    OMX_FillThisBuffer(pHandle, pBufferOut);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
#ifdef OMX_MPEG4E_SRCHANGES
				pBufferOut->pBuffer =
				    SharedRegion_getPtr(pBufferOut->pBuffer);
#endif

				retval =
				    TIMM_OSAL_GetPipeReadyMessageCount
				    (pAppData->OpBuf_Pipe, &numRemaining);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorBadParameter);
				MPEG4CLIENT_TRACE_PRINT
				    ("\nOutput Msg count=%d", numRemaining);

			}

			/*if(bEOS){
			   goto EXIT;
			   } */
		}

		/*Check for EOS Event */
		if ((pRetrievedEvents & MPEG4_EOS_EVENT))
		{
#ifdef TIMESTAMPSPRINT		/*to check the prder of output timestamp information....will be removed later */
			/*print the received timestamps information */
			for (i = 0; i < pAppData->nFillBufferDoneCount; i++)
			{
				MPEG4CLIENT_TRACE_PRINT("\nTimeStamp[%d]=%d",
				    i, pAppData->TimeStampsArray[i]);
				//printf( "TimeStamp[%d]=%d",i,pAppData->TimeStampsArray[i]);
			}
#endif
			eError = OMX_ErrorNone;
			//bEOS=OMX_TRUE;
			goto EXIT;

		}

		/*Check for Input Ready Event */
		numRemaining = 0;
		if ((pRetrievedEvents & MPEG4_INPUT_READY))
		{

			/*continue with the ETB calls only if MPEG4ClientStopFrameNum!=FrameNumToProcess */
			if ((MPEG4ClientStopFrameNum >= FrameNumToProcess))
			{
				MPEG4CLIENT_TRACE_PRINT
				    ("\nIn Input Ready event");
				retval =
				    TIMM_OSAL_GetPipeReadyMessageCount
				    (pAppData->IpBuf_Pipe, &numRemaining);
				GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
				    OMX_ErrorBadParameter);
				MPEG4CLIENT_TRACE_PRINT
				    ("\nInput Msg count=%d", numRemaining);
				while ((numRemaining) &&
				    ((MPEG4ClientStopFrameNum >=
					    FrameNumToProcess)))
				{
					/*read from the pipe */
					MPEG4CLIENT_TRACE_PRINT
					    ("\nRead Buffer from the Input Pipe to give the Next ETB call");
					retval =
					    TIMM_OSAL_ReadFromPipe(pAppData->
					    IpBuf_Pipe, &pBufferIn,
					    sizeof(pBufferIn), &actualSize,
					    TIMM_OSAL_SUSPEND);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorNotReady);
#if 0
					/*Check for the Buffer to BufHdr mapping */
#ifdef OMX_MPEG4E_BUF_HEAP
					/*free the older buffer & allocate the new */
					HeapBuf_free(heapHandle,
					    pBufferIn->pBuffer,
					    (pAppData->MPEG4_TestCaseParams->
						width *
						pAppData->
						MPEG4_TestCaseParams->height *
						3 / 2));
					pBufferIn->pBuffer =
					    HeapBuf_alloc(heapHandle,
					    (pAppData->MPEG4_TestCaseParams->
						width *
						pAppData->
						MPEG4_TestCaseParams->height *
						3 / 2), 0);

#elif defined (OMX_MPEG4E_LINUX_TILERTEST)
					/*free the older buffer & allocate the new */
					MemMgr_Free(pBufferIn->pBuffer);

					MemReqDescTiler[0].pixelFormat =
					    PIXEL_FMT_8BIT;
					MemReqDescTiler[0].dim.area.width = pAppData->MPEG4_TestCaseParams->width;	/*width */
					MemReqDescTiler[0].dim.area.height = pAppData->MPEG4_TestCaseParams->height;	/*height */
					MemReqDescTiler[0].stride =
					    STRIDE_8BIT;

					MemReqDescTiler[1].pixelFormat =
					    PIXEL_FMT_16BIT;
					MemReqDescTiler[1].dim.area.width = pAppData->MPEG4_TestCaseParams->width / 2;	/*width */
					MemReqDescTiler[1].dim.area.height = pAppData->MPEG4_TestCaseParams->height / 2;	/*height */
					MemReqDescTiler[1].stride =
					    STRIDE_16BIT;

					//MemReqDescTiler.reserved
					/*call to tiler Alloc */
					MPEG4CLIENT_TRACE_PRINT
					    ("\nBefore tiler alloc for the Input buffer \n");
					TilerAddr =
					    MemMgr_Alloc(MemReqDescTiler, 2);
					MPEG4CLIENT_TRACE_PRINT
					    ("\nTiler buffer allocated is %x\n",
					    TilerAddr);
					pBufferIn->pBuffer =
					    (OMX_U8 *) TilerAddr;

#else
					/*free the older buffer & allocate the new */
					TIMM_OSAL_Free(pBufferIn->pBuffer);
					pBufferIn->pBuffer =
					    TIMM_OSAL_Malloc(((pAppData->
						    MPEG4_TestCaseParams->
						    width *
						    pAppData->
						    MPEG4_TestCaseParams->
						    height * 3 / 2)),
					    TIMM_OSAL_TRUE, 32,
					    TIMMOSAL_MEM_SEGMENT_EXT);
#endif
					GOTO_EXIT_IF((pBufferIn->pBuffer ==
						TIMM_OSAL_NULL),
					    OMX_ErrorInsufficientResources);

#endif
					MPEG4CLIENT_TRACE_PRINT
					    ("\nRead Data from the Input File");
					eError =
					    MPEG4ENC_FillData(pAppData,
					    pBufferIn);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
					MPEG4CLIENT_TRACE_PRINT
					    ("\nCall to ETB");
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer before ETB = %x\n",
					    pBufferIn->pBuffer);
					pBufferIn->pBuffer =
					    (char *)
					    SharedRegion_getSRPtr(pBufferIn->
					    pBuffer, 2);
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer SR before ETB = %x\n",
					    pBufferIn->pBuffer);
#endif

					eError =
					    OMX_EmptyThisBuffer(pHandle,
					    pBufferIn);
					GOTO_EXIT_IF((eError !=
						OMX_ErrorNone), eError);
#ifdef OMX_MPEG4E_SRCHANGES
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer SR after ETB = %x\n",
					    pBufferIn->pBuffer);
					pBufferIn->pBuffer =
					    SharedRegion_getPtr(pBufferIn->
					    pBuffer);
					MPEG4CLIENT_TRACE_PRINT
					    ("\npBuffer after ETB = %x\n",
					    pBufferIn->pBuffer);
#endif

					/*Send command to Pause */
					if ((PauseFrameNum[PauseFrameNumIndex]
						== FrameNumToProcess) &&
					    (PauseFrameNumIndex <
						NUMTIMESTOPAUSE))
					{
						MPEG4CLIENT_TRACE_PRINT
						    ("\ncommand to go Exectuing -> Pause state");
						eError =
						    OMX_SendCommand(pHandle,
						    OMX_CommandStateSet,
						    OMX_StatePause, NULL);
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);

						eError =
						    MPEG4ENC_WaitForState
						    (pHandle);
						GOTO_EXIT_IF((eError !=
							OMX_ErrorNone),
						    eError);
						PauseFrameNumIndex++;
						MPEG4CLIENT_TRACE_PRINT
						    ("\nComponent is in Paused state");
					}
#ifndef TODO_ENC_MPEG4
					/*Call to Dynamic Settings */
					//MPEG4CLIENT_TRACE_PRINT( "\nConfigure the RunTime Settings");
					//eError = OMXMPEG4Enc_ConfigureRunTimeSettings(pAppData,&OMXConfigStructures,FrameNumToProcess);
					//GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
#endif

					/*Increase the frame count that has been sent for processing */
					FrameNumToProcess++;

					retval =
					    TIMM_OSAL_GetPipeReadyMessageCount
					    (pAppData->IpBuf_Pipe,
					    &numRemaining);
					GOTO_EXIT_IF((retval !=
						TIMM_OSAL_ERR_NONE),
					    OMX_ErrorBadParameter);
					MPEG4CLIENT_TRACE_PRINT
					    ("\nInput Msg count=%d",
					    numRemaining);
				}
			} else
			{
				MPEG4CLIENT_TRACE_PRINT
				    ("\nNothing todo ..check for other events");
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
			MPEG4CLIENT_TRACE_PRINT
			    ("\ncommand to go Pause -> Executing state");
			eError =
			    OMX_SendCommand(pHandle, OMX_CommandStateSet,
			    OMX_StateExecuting, NULL);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			eError = MPEG4ENC_WaitForState(pHandle);
			GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
			TempCount = 0;

		}

		/*get the Component State */
		eError = OMX_GetState(pHandle, &(pAppData->eState));
		MPEG4CLIENT_TRACE_PRINT("\nComponent Current State=%d",
		    pAppData->eState);

	}
      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXMPEG4Enc_ConfigureRunTimeSettings(MPEG4E_ILClient *
    pApplicationData, OMX_MPEG4E_DynamicConfigs * MPEG4EConfigStructures,
    OMX_U32 FrameNumber)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_U32 nRemoveList[9];
	OMX_U8 i, RemoveIndex = 0;


	MPEG4CLIENT_ENTER_PRINT();

	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;
	pHandle = pAppData->pHandle;

	for (i = 0; i < DynamicSettingsCount; i++)
	{
		switch (DynamicSettingsArray[i])
		{
		case 0:
			/*set config to OMX_CONFIG_FRAMERATETYPE */
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynFrameRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tFrameRate.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoFramerate,
				    &(MPEG4EConfigStructures->tFrameRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				MPEG4EConfigStructures->tFrameRate.
				    xEncodeFramerate =
				    ((pAppData->MPEG4_TestCaseParamsDynamic->
					DynFrameRate.
					nFramerate[DynFrameCountArray_Index
					    [i]]) << 16);

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoFramerate,
				    &(MPEG4EConfigStructures->tFrameRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tBitRate.nPortIndex =
				    MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoBitrate,
				    &(MPEG4EConfigStructures->tBitRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tBitRate.
				    nEncodeBitrate =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynBitRate.
				    nBitrate[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoBitrate,
				    &(MPEG4EConfigStructures->tBitRate));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tMESearchrange.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &(MPEG4EConfigStructures->
					tMESearchrange));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tMESearchrange.
				    eMVAccuracy =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nMVAccuracy[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tMESearchrange.
				    nHorSearchRangeP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nHorSearchRangeP[DynFrameCountArray_Index
				    [i]];
				MPEG4EConfigStructures->tMESearchrange.
				    nVerSearchRangeP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nVerSearchRangeP[DynFrameCountArray_Index
				    [i]];
				MPEG4EConfigStructures->tMESearchrange.
				    nHorSearchRangeB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nHorSearchRangeB[DynFrameCountArray_Index
				    [i]];
				MPEG4EConfigStructures->tMESearchrange.
				    nVerSearchRangeB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.
				    nVerSearchRangeB[DynFrameCountArray_Index
				    [i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoMESearchRange,
				    &(MPEG4EConfigStructures->
					tMESearchrange));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynForceFrame.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tForceframe.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &(MPEG4EConfigStructures->tForceframe));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tForceframe.
				    IntraRefreshVOP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynForceFrame.
				    ForceIFrame[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoIntraVOPRefresh,
				    &(MPEG4EConfigStructures->tForceframe));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynQpSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tQPSettings.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &(MPEG4EConfigStructures->tQPSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tQPSettings.nQpI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpI[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMaxI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxI[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMinI =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMinI[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpP[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMaxP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxP[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMinP =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMinP[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.
				    nQpOffsetB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpOffsetB[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMaxB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMaxB[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tQPSettings.nQpMinB =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.
				    nQpMinB[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoQPSettings,
				    &(MPEG4EConfigStructures->tQPSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tAVCIntraPeriod.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoAVCIntraPeriod,
				    &(MPEG4EConfigStructures->
					tAVCIntraPeriod));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tAVCIntraPeriod.
				    nIDRPeriod =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynIntraFrmaeInterval.
				    nIntraFrameInterval
				    [DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoAVCIntraPeriod,
				    &(MPEG4EConfigStructures->
					tAVCIntraPeriod));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tNALUSize.nPortIndex =
				    MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(MPEG4EConfigStructures->tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tNALUSize.nNaluBytes =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynNALSize.
				    nNaluSize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_IndexConfigVideoNalSize,
				    &(MPEG4EConfigStructures->tNALUSize));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{
				/*Set Port Index */
				MPEG4EConfigStructures->tSliceSettings.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigSliceSettings,
				    &(MPEG4EConfigStructures->
					tSliceSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tSliceSettings.
				    eSliceMode =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynSliceSettings.
				    eSliceMode[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tSliceSettings.
				    nSlicesize =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynSliceSettings.
				    nSlicesize[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigSliceSettings,
				    &(MPEG4EConfigStructures->
					tSliceSettings));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
			if (pAppData->MPEG4_TestCaseParamsDynamic->
			    DynPixelInfo.
			    nFrameNumber[DynFrameCountArray_Index[i]] ==
			    FrameNumber)
			{

				/*Set Port Index */
				MPEG4EConfigStructures->tPixelInfo.
				    nPortIndex = MPEG4_APP_OUTPUTPORT;
				eError =
				    OMX_GetConfig(pHandle,
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &(MPEG4EConfigStructures->tPixelInfo));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				MPEG4EConfigStructures->tPixelInfo.nWidth =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynPixelInfo.
				    nWidth[DynFrameCountArray_Index[i]];
				MPEG4EConfigStructures->tPixelInfo.nHeight =
				    pAppData->MPEG4_TestCaseParamsDynamic->
				    DynPixelInfo.
				    nHeight[DynFrameCountArray_Index[i]];

				eError =
				    OMX_SetConfig(pHandle,
				    OMX_TI_IndexConfigVideoPixelInfo,
				    &(MPEG4EConfigStructures->tPixelInfo));
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

				DynFrameCountArray_Index[i]++;
				/*check to remove from the DynamicSettingsArray list */
				if (pAppData->MPEG4_TestCaseParamsDynamic->
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
	eError = MPEG4E_Update_DynamicSettingsArray(RemoveIndex, nRemoveList);

      EXIT:
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}


/*
Copies 1D buffer to 2D buffer. All heights, widths etc. should be in bytes.
The function copies the lower no. of bytes i.e. if nSize1D < (nHeight2D * nWidth2D)
then nSize1D bytes is copied else (nHeight2D * nWidth2D) bytes is copied.
This function does not return any leftover no. of bytes, the calling function
needs to take care of leftover bytes on its own
*/
OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_1Dto2D(OMX_PTR pDst2D, OMX_PTR pSrc1D,
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}


/*
Copies 2D buffer to 1D buffer. All heights, widths etc. should be in bytes.
The function copies the lower no. of bytes i.e. if nSize1D < (nHeight2D * nWidth2D)
then nSize1D bytes is copied else (nHeight2D * nWidth2D) bytes is copied.
This function does not return any leftover no. of bytes, the calling function
needs to take care of leftover bytes on its own
*/
OMX_ERRORTYPE OMXMPEG4_Util_Memcpy_2Dto1D(OMX_PTR pDst1D, OMX_PTR pSrc2D,
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}


OMX_ERRORTYPE OMXMPEG4Enc_DynamicPortConfiguration(MPEG4E_ILClient *
    pApplicationData, OMX_U32 nPortIndex)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MPEG4E_ILClient *pAppData;

	MPEG4CLIENT_ENTER_PRINT();

	/*validate the Input Prameters */
	if (pApplicationData == NULL)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	pAppData = pApplicationData;

	/*validate the Port Index */
	if (nPortIndex > MPEG4_APP_OUTPUTPORT)
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
	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}

void OMXMPEG4Enc_Client_ErrorHandling(MPEG4E_ILClient * pApplicationData,
    OMX_U32 nErrorIndex, OMX_ERRORTYPE eGenError)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MPEG4E_ILClient *pAppData;

	MPEG4CLIENT_ENTER_PRINT();

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
		MPEG4CLIENT_ERROR_PRINT("%s  During FreeHandle",
		    MPEG4_GetErrorString(eGenError));
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
	MPEG4CLIENT_EXIT_PRINT(eError);

}


OMX_ERRORTYPE OMXMPEG4Enc_GetHandle_FreeHandle(MPEG4E_ILClient *
    pApplicationData)
{
	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;

	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;




}

OMX_ERRORTYPE OMXMPEG4Enc_LoadedToIdlePortDisable(MPEG4E_ILClient *
    pApplicationData)
{

	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;


	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;
	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/*call OMX_Sendcommand with cmd as Port Disable */
	eError =
	    OMX_SendCommand(pAppData->pHandle, OMX_CommandPortDisable,
	    OMX_ALL, NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXMPEG4Enc_LoadedToIdlePortEnable(MPEG4E_ILClient *
    pApplicationData)
{

	MPEG4E_ILClient *pAppData;
	OMX_HANDLETYPE pHandle = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE appCallbacks;
	TIMM_OSAL_ERRORTYPE retval = TIMM_OSAL_ERR_UNKNOWN;

	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*for the allocate resorces call fill the height & width*/
	pAppData->MPEG4_TestCaseParams->width = 176;
	pAppData->MPEG4_TestCaseParams->height = 144;

	MPEG4CLIENT_TRACE_PRINT
	    ("\nAllocate the resources for the state trasition to Idle to complete");
	eError = MPEG4ENC_AllocateResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\nFree up the Component Resources");
	eError = MPEG4ENC_FreeResources(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;

}

OMX_ERRORTYPE OMXMPEG4Enc_UnalignedBuffers(MPEG4E_ILClient * pApplicationData)
{
	MPEG4E_ILClient *pAppData;
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


	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;
	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
/*update the height & width*/
	pAppData->MPEG4_TestCaseParams->width = 176;
	pAppData->MPEG4_TestCaseParams->height = 144;


	MPEG4CLIENT_TRACE_PRINT
	    ("\nAllocate the resources for the state trasition to Idle to complete");
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
		if (tPortDef.nPortIndex == MPEG4_APP_INPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
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
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == MPEG4_APP_OUTPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
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
		if (i == MPEG4_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
		} else if (i == MPEG4_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;
}

OMX_ERRORTYPE OMXMPEG4Enc_SizeLessthanMinBufferRequirements(MPEG4E_ILClient *
    pApplicationData)
{
	MPEG4E_ILClient *pAppData;
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


	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
	/*update the height & width */
	pAppData->MPEG4_TestCaseParams->width = 176;
	pAppData->MPEG4_TestCaseParams->height = 100;

	MPEG4CLIENT_TRACE_PRINT
	    ("\nAllocate the resources for the state trasition to Idle to complete");
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
		if (tPortDef.nPortIndex == MPEG4_APP_INPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height * 3 / 2)), TIMM_OSAL_TRUE,
				    0, TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);

				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pInBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == MPEG4_APP_OUTPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
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
		if (i == MPEG4_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
		} else if (i == MPEG4_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);

	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;



}

OMX_ERRORTYPE OMXMPEG4Enc_LessthanMinBufferRequirementsNum(MPEG4E_ILClient *
    pApplicationData)
{

	MPEG4E_ILClient *pAppData;
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


	MPEG4CLIENT_ENTER_PRINT();
	/*Get the AppData */
	pAppData = pApplicationData;

	/*create event */
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_Events);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);
	retval = TIMM_OSAL_EventCreate(&MPEG4VE_CmdEvent);
	GOTO_EXIT_IF((retval != TIMM_OSAL_ERR_NONE),
	    OMX_ErrorInsufficientResources);

	/*initialize the call back functions before the get handle function */
	appCallbacks.EventHandler = MPEG4ENC_EventHandler;
	appCallbacks.EmptyBufferDone = MPEG4ENC_EmptyBufferDone;
	appCallbacks.FillBufferDone = MPEG4ENC_FillBufferDone;

	eError = OMX_Init();
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
	/* Load the MPEG4ENCoder Component */
	eError =
	    OMX_GetHandle(&pHandle,
	    (OMX_STRING) "OMX.TI.DUCATI1.VIDEO.MPEG4E", pAppData,
	    &appCallbacks);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	pAppData->pHandle = pHandle;

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Loaded -> Idle state");
	/* OMX_SendCommand expecting OMX_StateIdle */
	eError =
	    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle,
	    NULL);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

/*Allocate Buffers*/
	/*update the height & width */
	pAppData->MPEG4_TestCaseParams->width = 176;
	pAppData->MPEG4_TestCaseParams->height = 144;

	MPEG4CLIENT_TRACE_PRINT
	    ("\nAllocate the resources for the state trasition to Idle to complete");
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
		if (tPortDef.nPortIndex == MPEG4_APP_INPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height * 3 / 2)), TIMM_OSAL_TRUE,
				    0, TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);

				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pInBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height * 3 / 2)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);
			}

		} else if (tPortDef.nPortIndex == MPEG4_APP_OUTPUTPORT)
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
					    MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), TIMM_OSAL_TRUE, 0,
				    TIMMOSAL_MEM_SEGMENT_EXT);
				GOTO_EXIT_IF((pTmpBuffer == TIMM_OSAL_NULL),
				    OMX_ErrorInsufficientResources);
				eError =
				    OMX_UseBuffer(pAppData->pHandle,
				    &(pAppData->pOutBuff[j]),
				    tPortDef.nPortIndex, pAppData,
				    ((pAppData->MPEG4_TestCaseParams->width *
					    pAppData->MPEG4_TestCaseParams->
					    height)), pTmpBuffer);
				GOTO_EXIT_IF((eError != OMX_ErrorNone),
				    eError);

			}
		}


	}

	/* Wait for initialization to complete.. Wait for Idle state of component  */
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

	MPEG4CLIENT_TRACE_PRINT("\ncall to goto Idle -> Loaded state");
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
		if (i == MPEG4_APP_INPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual - 1; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
		} else if (i == MPEG4_APP_OUTPUTPORT)
		{
			for (j = 0; j < tPortDef.nBufferCountActual; j++)
			{
				if (!pAppData->MPEG4_TestCaseParams->
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
	eError = MPEG4ENC_WaitForState(pAppData);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);


	MPEG4CLIENT_TRACE_PRINT("\nGot the Component Handle =%x", pHandle);

	/* UnLoad the Encoder Component */
	MPEG4CLIENT_TRACE_PRINT("\ncall to FreeHandle()");
	eError = OMX_FreeHandle(pHandle);
	GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);



      EXIT:
	/* De-Initialize OMX Core */
	MPEG4CLIENT_TRACE_PRINT("\ncall OMX Deinit");
	eError = OMX_Deinit();

	/*Delete the Events */
	TIMM_OSAL_EventDelete(MPEG4VE_Events);
	TIMM_OSAL_EventDelete(MPEG4VE_CmdEvent);


	if (eError != OMX_ErrorNone)
	{
		MPEG4CLIENT_ERROR_PRINT("\n%s", MPEG4_GetErrorString(eError));
	}

	MPEG4CLIENT_EXIT_PRINT(eError);
	return eError;


}

OMX_ERRORTYPE MPEG4E_SetClientParams(MPEG4E_ILClient * pApplicationData)
{

	MPEG4E_ILClient *pAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 testcasenum, NumParamCount = 0, i, j = 0;
	char configFile[] = "MPEG4enc_basic_tc1.cfg";
	char EncConfigFile[50];
	FILE *fpconfigFile;
	char line[256];
	char file_string[256];
	char file_name[256];
	OMX_S32 file_parameter, MaxParam[10] =
	    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;
	MPEG4CLIENT_INFO_PRINT("enter the testcase number");
	scanf("%d", &testcasenum);

/*=================================================================================*/
/* Get the Basic Settings params from the file 																							  */
/*=================================================================================*/
#ifndef TODO_ENC_MPEG4
	//sprintf(EncConfigFile,"%s_Basic_TC_%d.cfg",configFile,testcasenum);
#endif

#ifdef TODO_ENC_MPEG4
	strcpy(EncConfigFile, configFile);
#endif

	MPEG4CLIENT_INFO_PRINT("open file EncoderBasicSettings.cfg ");
	fpconfigFile = fopen(EncConfigFile, "r");
	if (!fpconfigFile)
	{
		MPEG4CLIENT_ERROR_PRINT
		    ("%s - File not found. Place the file in the location of the base_image",
		    EncConfigFile);
		return OMX_ErrorInsufficientResources;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("testcase_ID = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->TestCaseId = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the testcase_ID. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%s", file_string);
		MPEG4CLIENT_INFO_PRINT("Input file = %s", file_string);
		pAppData->fIn = fopen(file_string, "rb");
		if (!pAppData->fIn)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("%s - File not found. Place the file in the location of the base_image/input",
			    file_string);
			return OMX_ErrorInsufficientResources;
		}
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the input file. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%s", file_string);
		MPEG4CLIENT_INFO_PRINT("Output file = %s", file_string);
		pAppData->fOut = fopen(file_string, "wb");
		if (!pAppData->fOut)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("%s - File not found. Place the file in the location of the base_image/output",
			    file_string);
			return OMX_ErrorInsufficientResources;
		}
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the output file. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("width = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->width = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the width. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("height = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->height = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the height. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("profile = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->profile = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the profile. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("level = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->level = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the level. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("chroma format = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->inputChromaFormat =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the inputchromaformat. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("InputContentType = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->InputContentType =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the InputContentType. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("InterlaceCodingType = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->InterlaceCodingType =
		    file_parameter;
	} else
	{
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the InterlaceCodingType. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bLoopFilter = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->bLoopFilter = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the bLoopFilter. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bCABAC = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->bCABAC = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the bCABAC. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bFMO = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->bFMO = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the bFMO. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bConstIntraPred = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->bConstIntraPred =
		    file_parameter;
	} else
	{
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the bConstIntraPred. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nNumSliceGroups = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->nNumSliceGroups =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nNumSliceGroups. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nSliceGroupMapType = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->nSliceGroupMapType =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nSliceGroupMapType. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("eSliceMode = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->eSliceMode = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the eSliceMode. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("RateControlAlg = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->RateControlAlg =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the RateControlAlg. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("TransformBlockSize = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->TransformBlockSize =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the TransformBlockSize. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("EncodingPreset = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->EncodingPreset =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the EncodingPreset. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("RateCntrlPreset = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->RateCntrlPreset =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the RateCntrlPreset. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("BitStreamFormat = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->BitStreamFormat =
		    file_parameter;
	} else
	{
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the BitStreamFormat. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("maxInterFrameInterval = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->maxInterFrameInterval =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the maxInterFrameInterval. Check the file syntax");
		goto EXIT;
	}
	pAppData->MPEG4_TestCaseParams->nBitEnableAdvanced = 0;
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nBitEnableAdvanced = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->nBitEnableAdvanced =
		    file_parameter;
	} else
	{
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nBitEnableAdvanced. Check the file syntax");
		goto EXIT;
	}
	pAppData->MPEG4_TestCaseParams->nBitEnableDynamic = 0;
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nBitEnableDynamic = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->nBitEnableDynamic =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nBitEnableDynamic. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nNumInputBuf = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->nNumInputBuf = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nNumInputBuf. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("nNumOutputBuf = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->nNumOutputBuf =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the nNumOutputBuf. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bInAllocatebuffer = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->bInAllocatebuffer =
		    file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the bInAllocatebuffer. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("bOutAllocatebuffer = %d",
		    file_parameter);
		pAppData->MPEG4_TestCaseParams->eSliceMode = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the eSliceMode. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("TestType = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->TestType = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the TestType. Check the file syntax");
		goto EXIT;
	}
	if (fgets(line, 254, fpconfigFile))
	{
		sscanf(line, "%d", &file_parameter);
		MPEG4CLIENT_INFO_PRINT("StopFrameNum = %d", file_parameter);
		pAppData->MPEG4_TestCaseParams->StopFrameNum = file_parameter;
	} else
	{
		eError = OMX_ErrorBadParameter;
		MPEG4CLIENT_ERROR_PRINT
		    ("Could not get the StopFrameNum. Check the file syntax");
		goto EXIT;
	}
	/*close the Basic settings file */
	fclose(fpconfigFile);
	fpconfigFile = NULL;

/*=================================================================================*/
/* Get the Dynamic params from the file                                                                                               */
/*=================================================================================*/
	if (pAppData->MPEG4_TestCaseParams->nBitEnableDynamic)
	{
		sprintf(EncConfigFile, "%s_Dynamic_TC_%d.cfg", configFile,
		    testcasenum);
		MPEG4CLIENT_INFO_PRINT
		    ("open file Encoder dynamic Settings.cfg ");
		fpconfigFile = fopen(EncConfigFile, "r");
		if (!fpconfigFile)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("%s - File not found. Place the file in the location of the base_image",
			    EncConfigFile);
			return OMX_ErrorInsufficientResources;
		}
		//FrameRate DynFrameRate;
		MPEG4CLIENT_INFO_PRINT("get the DynFrameRate values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the frame rate settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the Frame rate params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynFrameRate.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynFrameRate.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynFrameRate.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[0] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynFrameRate.
			    nFramerate =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynFrameRate.nFramerate == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the frame rate values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynFrameRate.nFramerate[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the frame rate values . Check the file syntax");
				goto EXIT;
			}
		}
		//BitRate DynBitRate;
		MPEG4CLIENT_INFO_PRINT("get the DynBitRate values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the bit rate settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the Bitrate params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynBitRate.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[1] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynBitRate.
			    nBitrate =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynBitRate.nBitrate == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			/*get the bitrate values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynBitRate.nBitrate[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the bitrate values . Check the file syntax");
				goto EXIT;
			}
		}
		//MESearchRange DynMESearchRange;
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the ME search Range settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the ME search Range params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynMESearchRange.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynMESearchRange.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[2] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nMVAccuracy =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynMESearchRange.nMVAccuracy ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nHorSearchRangeP =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynMESearchRange.nHorSearchRangeP ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nVerSearchRangeP =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynMESearchRange.nVerSearchRangeP ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nHorSearchRangeB =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynMESearchRange.nHorSearchRangeB ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nVerSearchRangeB =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynMESearchRange.nVerSearchRangeB ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the MEsearch range values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.nMVAccuracy[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the nMVAccuracy values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nHorSearchRangeP[i] = MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the nHorSearchRangeP values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nVerSearchRangeP[i] = MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the nVerSearchRangeP values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nHorSearchRangeB[i] = MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the nHorSearchRangeB values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nVerSearchRangeB[i] = MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the nVerSearchRangeB values . Check the file syntax");
				goto EXIT;
			}
		}
		/////ForceFrame DynForceFrame;
		MPEG4CLIENT_INFO_PRINT("get the DynForceFrame values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the force frame settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the Force frame params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynForceFrame.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynForceFrame.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynForceFrame.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[3] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynForceFrame.
			    ForceIFrame =
			    (OMX_BOOL *) TIMM_OSAL_Malloc((sizeof(OMX_BOOL) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynForceFrame.ForceIFrame == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynForceFrame.ForceIFrame[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the forceframe values . Check the file syntax");
				goto EXIT;
			}
		}
		//QpSettings DynQpSettings;
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for theQp settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the Force frame params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynQpSettings.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[4] = NumParamCount;
		if (NumParamCount)
		{
			//QpI
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpI =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpI == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpIMax
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMaxI =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMaxI == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpIMin
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMinI =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMinI == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpP
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpP =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpP == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpPMax
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMaxP =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMaxP == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpPMin
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMinP =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMinP == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpB
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpOffsetB =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpOffsetB == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpBMax
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMaxB =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMaxB == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			//QpBMin
			pAppData->MPEG4_TestCaseParamsDynamic->DynQpSettings.
			    nQpMinB =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynQpSettings.nQpMinB == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpI[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpI . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxI[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpMaxI . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinI[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpMinI . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpP[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpP . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxP[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpMaxP . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinP[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpPMin . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpOffsetB[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpoffsetB . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxB[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpMaxB . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinB[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the QpBMin . Check the file syntax");
				goto EXIT;
			}
		}
		//IntraFrameInterval DynIntraFrmaeInterval;
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the IntrFrameInterval settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the intrafarme interval params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynIntraFrmaeInterval.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynIntraFrmaeInterval.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.nFrameNumber[i] =
			    MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[5] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.nIntraFrameInterval =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynIntraFrmaeInterval.nIntraFrameInterval ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the bitrate values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynIntraFrmaeInterval.
					    nIntraFrameInterval[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the IntrFrameInterval values . Check the file syntax");
				goto EXIT;
			}
		}
		//NALSize DynNALSize;
		MPEG4CLIENT_INFO_PRINT("get the DynNALSize values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the DynNAL settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the DynNALSize params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynNALSize.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[6] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynNALSize.
			    nNaluSize =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynNALSize.nNaluSize == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the NALSize values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynNALSize.nNaluSize[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the NALSize values . Check the file syntax");
				goto EXIT;
			}
		}
		//SliceCodingSettings DynSliceSettings;
		MPEG4CLIENT_INFO_PRINT("get the DynSliceSettings values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the DynSliceSettings settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the DynSliceSettings params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynSliceSettings.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynSliceSettings.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[7] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.eSliceMode =
			    (OMX_VIDEO_AVCSLICEMODETYPE *)
			    TIMM_OSAL_Malloc((sizeof
				(OMX_VIDEO_AVCSLICEMODETYPE) * NumParamCount),
			    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynSliceSettings.eSliceMode ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.nSlicesize =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynSliceSettings.nSlicesize ==
				TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the SliceMOde values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynSliceSettings.eSliceMode[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the slicemode values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynSliceSettings.nSlicesize[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the SliceSize values . Check the file syntax");
				goto EXIT;
			}
		}
		//PixelInfo DynPixelInfo;
		MPEG4CLIENT_INFO_PRINT("get the DynPixelInfo values");
		if (fgets(line, 254, fpconfigFile))
		{
			NumParamCount = 0;
			j = sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
			    &MaxParam[0], &MaxParam[1], &MaxParam[2],
			    &MaxParam[3], &MaxParam[4], &MaxParam[5],
			    &MaxParam[6], &MaxParam[7], &MaxParam[8],
			    &MaxParam[9]);
			for (i = 0; i < j; i++)
			{
				MPEG4CLIENT_INFO_PRINT("Param = %d",
				    MaxParam[i]);
				if (MaxParam[i] != -1)
				{
					NumParamCount++;
				} else
				{
					break;
				}
			}
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the frame numbers for the DynPixelInfo settings. Check the file syntax");
			goto EXIT;
		}
		/*allocate memory for the DynPixelInfo params */
		pAppData->MPEG4_TestCaseParamsDynamic->DynPixelInfo.
		    nFrameNumber =
		    (OMX_S32 *) TIMM_OSAL_Malloc((sizeof(OMX_S32) *
			(NumParamCount + 1)), TIMM_OSAL_TRUE, 0,
		    TIMMOSAL_MEM_SEGMENT_EXT);
		GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
			DynPixelInfo.nFrameNumber == TIMM_OSAL_NULL),
		    OMX_ErrorInsufficientResources);
		/*get the frame numbers in the structure */
		for (i = 0; i < NumParamCount + 1; i++)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynPixelInfo.
			    nFrameNumber[i] = MaxParam[i];
		}
		/*update for the Free Index count */
		DynFrameCountArray_FreeIndex[8] = NumParamCount;
		if (NumParamCount)
		{
			pAppData->MPEG4_TestCaseParamsDynamic->DynPixelInfo.
			    nWidth =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynPixelInfo.nWidth == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);
			pAppData->MPEG4_TestCaseParamsDynamic->DynPixelInfo.
			    nHeight =
			    (OMX_U32 *) TIMM_OSAL_Malloc((sizeof(OMX_U32) *
				NumParamCount), TIMM_OSAL_TRUE, 0,
			    TIMMOSAL_MEM_SEGMENT_EXT);
			GOTO_EXIT_IF((pAppData->MPEG4_TestCaseParamsDynamic->
				DynPixelInfo.nHeight == TIMM_OSAL_NULL),
			    OMX_ErrorInsufficientResources);

			/*get the width&height values in the structure */
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynPixelInfo.nWidth[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the Width values . Check the file syntax");
				goto EXIT;
			}
			if (fgets(line, 254, fpconfigFile))
			{
				j = sscanf(line,
				    "%d %d %d %d %d %d %d %d %d %d",
				    &MaxParam[0], &MaxParam[1], &MaxParam[2],
				    &MaxParam[3], &MaxParam[4], &MaxParam[5],
				    &MaxParam[6], &MaxParam[7], &MaxParam[8],
				    &MaxParam[9]);
				for (i = 0; i < NumParamCount; i++)
				{
					MPEG4CLIENT_INFO_PRINT("Param = %d",
					    MaxParam[i]);
					pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynPixelInfo.nHeight[i] =
					    MaxParam[i];
				}
			} else
			{
				eError = OMX_ErrorBadParameter;
				MPEG4CLIENT_ERROR_PRINT
				    ("Could not get the Height values . Check the file syntax");
				goto EXIT;
			}
		}

		/*close the Dynamic settings file */
		fclose(fpconfigFile);
		fpconfigFile = NULL;
	}
/*=================================================================================*/
/* Get the Advanced params from the file                                                                                               */
/*=================================================================================*/
	if (pAppData->MPEG4_TestCaseParams->nBitEnableAdvanced)
	{
		sprintf(EncConfigFile, "%s_Advanced_TC_%d.cfg", configFile,
		    testcasenum);
		MPEG4CLIENT_INFO_PRINT
		    ("open file EncoderAdvanced Settings.cfg ");
		fpconfigFile = fopen(EncConfigFile, "r");
		if (!fpconfigFile)
		{
			MPEG4CLIENT_ERROR_PRINT
			    ("%s - File not found. Place the file in the location of the base_image",
			    EncConfigFile);
			return OMX_ErrorInsufficientResources;
		}
		//NALUSettings NALU;
		//NALU.nStartofSequence
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT("NALU.nStartofSequence = %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->NALU.
			    nStartofSequence = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the NALU.nStartofSequence. Check the file syntax");
			goto EXIT;
		}
		//NALU.nEndofSequence
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT("NALU.nEndofSequence = %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->NALU.
			    nEndofSequence = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the NALU.nEndofSequence. Check the file syntax");
			goto EXIT;
		}
		//NALU.nIDR
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT("NALU.nIDR = %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->NALU.nIDR =
			    file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the NALU.nIDR. Check the file syntax");
			goto EXIT;
		}
		//NALU.nIntraPicture
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT("NALU.nIntraPicture = %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->NALU.
			    nIntraPicture = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the NALU.nIntraPicture. Check the file syntax");
			goto EXIT;
		}
		//NALU.nNonIntraPicture
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT("NALU.nNonIntraPicture = %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->NALU.
			    nNonIntraPicture = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the NALU.nNonIntraPicture. Check the file syntax");
			goto EXIT;
		}

		//MEBlockSize MEBlock;
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.eMinBlockSizeP = %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.
			    eMinBlockSizeP = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.eMinBlockSizeP Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.eMinBlockSizeB = %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.
			    eMinBlockSizeB = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->MEBlock.eMinBlockSizeB Check the file syntax");
			goto EXIT;
		}
		//IntraRefreshSettings IntraRefresh;
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.eRefreshMode = %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.
			    eRefreshMode = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.eRefreshMode Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.nRate= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.
			    nRate = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraRefresh.nRate Check the file syntax");
			goto EXIT;
		}
		//VUISettings VUI;
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->VUI.bAspectRatioPresent= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->VUI.
			    bAspectRatioPresent = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->VUI.bAspectRatioPresent Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->VUI.eAspectRatio= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->VUI.
			    eAspectRatio = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->VUI.eAspectRatio Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->VUI.bFullRange= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->VUI.
			    bFullRange = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->VUI.bFullRange Check the file syntax");
			goto EXIT;
		}

		//IntrapredictionSettings IntraPred;
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra4x4Enable= %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.
			    nLumaIntra4x4Enable = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra4x4Enable Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra8x8Enable= %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.
			    nLumaIntra8x8Enable = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra8x8Enable Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra16x16Enable= %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.
			    nLumaIntra16x16Enable = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nLumaIntra16x16Enable  Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%x", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nChromaIntra8x8Enable= %x",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.
			    nChromaIntra8x8Enable = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.nChromaIntra8x8Enable Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.eChromaComponentEnable= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.
			    eChromaComponentEnable = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->IntraPred.eChromaComponentEnable Check the file syntax");
			goto EXIT;
		}

		//MPEG4EDataSyncSettings DataSync;
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.inputDataMode= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.
			    inputDataMode = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.inputDataMode Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.outputDataMode= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.
			    outputDataMode = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.outputDataMode Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.numInputDataUnits= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.
			    numInputDataUnits = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.numInputDataUnits Check the file syntax");
			goto EXIT;
		}
		if (fgets(line, 254, fpconfigFile))
		{
			sscanf(line, "%d", &file_parameter);
			MPEG4CLIENT_INFO_PRINT
			    ("pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.numOutputDataUnits= %d",
			    file_parameter);
			pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.
			    numOutputDataUnits = file_parameter;
		} else
		{
			eError = OMX_ErrorBadParameter;
			MPEG4CLIENT_ERROR_PRINT
			    ("Could not get the pAppData->MPEG4_TestCaseParamsAdvanced->DataSync.numOutputDataUnits Check the file syntax");
			goto EXIT;
		}

		/*close the Advanced settings file */
		fclose(fpconfigFile);
		fpconfigFile = NULL;
	}
      EXIT:
	if (fpconfigFile)
	{
		fclose(fpconfigFile);
	}
	return eError;


}				/*end setparameters_client */


OMX_ERRORTYPE MPEG4E_FreeDynamicClientParams(MPEG4E_ILClient *
    pApplicationData)
{

	MPEG4E_ILClient *pAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i;

	if (!pApplicationData)
	{
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}
	/*Get the AppData */
	pAppData = pApplicationData;

	for (i = 0; i < 9; i++)
	{
		switch (i)
		{
		case 0:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynFrameRate.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynFrameRate.nFramerate)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynFrameRate.nFramerate);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
			}
			break;
		case 1:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynBitRate.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynBitRate.nBitrate)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynBitRate.nBitrate);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
			}
			break;
		case 2:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynMESearchRange.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{

				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.nMVAccuracy)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.nMVAccuracy);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.nHorSearchRangeP)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nHorSearchRangeP);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.nVerSearchRangeP)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nVerSearchRangeP);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.nHorSearchRangeB)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nHorSearchRangeB);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynMESearchRange.nVerSearchRangeB)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynMESearchRange.
					    nVerSearchRangeB);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}

			}
			break;
		case 3:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynForceFrame.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynForceFrame.ForceIFrame)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynForceFrame.ForceIFrame);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}

			}
			break;
		case 4:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynQpSettings.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpI)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpI);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMaxI)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxI);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMinI)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinI);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpP)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpP);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMaxP)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxP);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMinP)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinP);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpOffsetB)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpOffsetB);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMaxB)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMaxB);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynQpSettings.nQpMinB)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynQpSettings.nQpMinB);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
			}
			break;
		case 5:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynIntraFrmaeInterval.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynIntraFrmaeInterval.nIntraFrameInterval)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynIntraFrmaeInterval.
					    nIntraFrameInterval);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
			}
			break;
		case 6:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynNALSize.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynNALSize.nNaluSize)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynNALSize.nNaluSize);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}


			}
			break;
		case 7:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynSliceSettings.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynSliceSettings.eSliceMode)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynSliceSettings.eSliceMode);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynSliceSettings.nSlicesize)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynSliceSettings.nSlicesize);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}

			}
			break;
		case 8:
			TIMM_OSAL_Free(pAppData->MPEG4_TestCaseParamsDynamic->
			    DynPixelInfo.nFrameNumber);
			if (DynFrameCountArray_FreeIndex[i])
			{
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynPixelInfo.nWidth)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynPixelInfo.nWidth);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
				if (pAppData->MPEG4_TestCaseParamsDynamic->
				    DynPixelInfo.nHeight)
				{
					TIMM_OSAL_Free(pAppData->
					    MPEG4_TestCaseParamsDynamic->
					    DynPixelInfo.nHeight);
				} else
				{
					eError = OMX_ErrorUndefined;
					MPEG4CLIENT_ERROR_PRINT
					    ("Something has gone Wrong..........................!!!!!!!!!!!!!");
				}
			}
			break;
		}
	}

      EXIT:
	return eError;

}
