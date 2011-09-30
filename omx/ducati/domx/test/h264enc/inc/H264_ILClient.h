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
*   @file  H264D_ILClient.h
*   This file contains the structures and macros that are used in the Application which tests the OpenMAX component
*
*  @path \WTSD_DucatiMMSW\omx\khronos1.1\omx_h264_dec\test
*
*  @rev 1.0
*/

#ifndef _H264_ILClient_
#define _H264_ILClient_


/****************************************************************
 * INCLUDE FILES
 ****************************************************************/
/**----- system and platform files ----------------------------**/
#define OMX_TILERTEST
#define H264_LINUX_CLIENT

#ifndef H264_LINUX_CLIENT
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/System.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef H264_LINUX_CLIENT
#include <timm_osal_interfaces.h>
#else
#include <WTSD_DucatiMMSW/platform/osal/timm_osal_interfaces.h>
#endif

#ifndef H264_LINUX_CLIENT
#define H264ENC_TRACE_ENABLE
#ifdef H264ENC_TRACE_ENABLE
	/*overwriting the default trcae flags */
#define TIMM_OSAL_DEBUG_TRACE_DETAIL 0
#define TIMM_OSAL_DEBUG_TRACE_LEVEL 5
#endif
#endif
#ifdef H264_LINUX_CLIENT
#include <timm_osal_trace.h>
#else
#include <WTSD_DucatiMMSW/platform/osal/timm_osal_trace.h>
#endif

/**-------program files ----------------------------------------**/
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_TI_Video.h>
#include <OMX_TI_Index.h>
#include <OMX_TI_Common.h>

#ifdef H264_LINUX_CLIENT
#define H264CLIENT_TRACE_PRINT(ARGS,...)  TIMM_OSAL_Debug(ARGS,##__VA_ARGS__)
#define H264CLIENT_ENTER_PRINT()	         TIMM_OSAL_Entering("")
#define H264CLIENT_EXIT_PRINT(ARG)           TIMM_OSAL_Exiting("")
#define H264CLIENT_ERROR_PRINT(ARGS,...)  TIMM_OSAL_Error(ARGS,##__VA_ARGS__ )
#define H264CLIENT_INFO_PRINT(ARGS,...)     TIMM_OSAL_Info(ARGS,##__VA_ARGS__ )
#else
static TIMM_OSAL_TRACEGRP TraceGrp;
#define H264CLIENT_TRACE_PRINT(ARGS,...)   TIMM_OSAL_DebugExt(TraceGrp,ARGS,##__VA_ARGS__)
#define H264CLIENT_ENTER_PRINT()               TIMM_OSAL_EnteringExt(TraceGrp)
#define H264CLIENT_EXIT_PRINT(ARG)            TIMM_OSAL_ExitingExt(TraceGrp,ARG)
#define H264CLIENT_ERROR_PRINT(ARGS,...)   TIMM_OSAL_ErrorExt(TraceGrp,ARGS,##__VA_ARGS__ )
#define H264CLIENT_INFO_PRINT(ARGS,...)     TIMM_OSAL_InfoExt(TraceGrp,ARGS,##__VA_ARGS__ )
#endif


#define GOTO_EXIT_IF(_CONDITION,_ERROR) { \
            if((_CONDITION)) { \
                H264CLIENT_INFO_PRINT("%s : %s : %d :: ", __FILE__, __FUNCTION__,__LINE__);\
                H264CLIENT_INFO_PRINT(" Exiting because: %s \n", #_CONDITION); \
                eError = (_ERROR); \
                goto EXIT; \
            } \
}


#define OMX_TEST_INIT_STRUCT_PTR(_s_, _name_)   \
 TIMM_OSAL_Memset((_s_), 0x0, sizeof(_name_)); 			\
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision  = 0x0;       	\
    (_s_)->nVersion.s.nStep   = 0x0;

/*Advanced Settings*/
typedef struct
{
	OMX_U32 nStartofSequence;
	OMX_U32 nEndofSequence;
	OMX_U32 nIDR;
	OMX_U32 nIntraPicture;
	OMX_U32 nNonIntraPicture;
} NALUSettings;


typedef struct
{
	OMX_U8 nNumSliceGroups;
	OMX_U8 nSliceGroupMapType;
	OMX_VIDEO_SLICEGRPCHANGEDIRTYPE eSliceGrpChangeDir;
	OMX_U32 nSliceGroupChangeRate;
	OMX_U32 nSliceGroupChangeCycle;
	OMX_U32 nSliceGroupParams[H264ENC_MAXNUMSLCGPS];
} FMOSettings;


typedef struct
{
	OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeP;
	OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeB;
} MEBlockSize;


typedef struct
{
	OMX_VIDEO_INTRAREFRESHTYPE eRefreshMode;
	OMX_U32 nRate;
} IntraRefreshSettings;


typedef struct
{
	OMX_BOOL bAspectRatioPresent;
	OMX_VIDEO_ASPECTRATIOTYPE eAspectRatio;
	OMX_BOOL bFullRange;
} VUISettings;


typedef struct
{
	OMX_U32 nLumaIntra4x4Enable;
	OMX_U32 nLumaIntra8x8Enable;
	OMX_U32 nLumaIntra16x16Enable;
	OMX_U32 nChromaIntra8x8Enable;
	OMX_VIDEO_CHROMACOMPONENTTYPE eChromaComponentEnable;
} IntrapredictionSettings;

typedef struct
{
	OMX_VIDEO_DATASYNCMODETYPE inputDataMode;
	OMX_VIDEO_DATASYNCMODETYPE outputDataMode;
	OMX_U32 numInputDataUnits;
	OMX_U32 numOutputDataUnits;
} H264EDataSyncSettings;



/*Dynamic Params Settings*/
typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nFramerate[10];
} FrameRate;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nBitrate[10];
} BitRate;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nMVAccuracy[10];
	OMX_U32 nHorSearchRangeP[10];
	OMX_U32 nVerSearchRangeP[10];
	OMX_U32 nHorSearchRangeB[10];
	OMX_U32 nVerSearchRangeB[10];
} MESearchRange;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_BOOL ForceIFrame[10];
} ForceFrame;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nQpI[10];
	OMX_U32 nQpMaxI[10];
	OMX_U32 nQpMinI[10];
	OMX_U32 nQpP[10];
	OMX_U32 nQpMaxP[10];
	OMX_U32 nQpMinP[10];
	OMX_U32 nQpOffsetB[10];
	OMX_U32 nQpMaxB[10];
	OMX_U32 nQpMinB[10];
} QpSettings;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nIntraFrameInterval[10];
} IntraFrameInterval;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nNaluSize[10];
} NALSize;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_VIDEO_AVCSLICEMODETYPE eSliceMode[10];
	OMX_U32 nSlicesize[10];
} SliceCodingSettings;


typedef struct
{
	OMX_S32 nFrameNumber[10];
	OMX_U32 nWidth[10];
	OMX_U32 nHeight[10];
} PixelInfo;





typedef struct H264E_TestCaseParamsAdvanced
{
	NALUSettings NALU;
	FMOSettings FMO;
	MEBlockSize MEBlock;
	IntraRefreshSettings IntraRefresh;
	VUISettings VUI;
	IntrapredictionSettings IntraPred;
	H264EDataSyncSettings DataSync;
} H264E_TestCaseParamsAdvanced;

typedef struct H264E_TestCaseParamsDynamic
{
	FrameRate DynFrameRate;
	BitRate DynBitRate;
	MESearchRange DynMESearchRange;
	ForceFrame DynForceFrame;
	QpSettings DynQpSettings;
	IntraFrameInterval DynIntraFrmaeInterval;
	NALSize DynNALSize;
	SliceCodingSettings DynSliceSettings;
	PixelInfo DynPixelInfo;

} H264E_TestCaseParamsDynamic;


typedef enum H264E_TestType
{
	FULL_RECORD = 0,
	PARTIAL_RECORD = 1
} H264E_TestType;


typedef struct H264E_TestCaseParams
{
	OMX_U8 TestCaseId;
	char InFile[100];
	char OutFile[100];
	OMX_U32 width;
	OMX_U32 height;
	OMX_VIDEO_AVCPROFILETYPE profile;
	OMX_VIDEO_AVCLEVELTYPE level;
	OMX_COLOR_FORMATTYPE inputChromaFormat;
	OMX_VIDEO_FRAMECONTENTTYPE InputContentType;
	OMX_VIDEO_INTERLACE_CODINGTYPE InterlaceCodingType;
	OMX_BOOL bLoopFilter;
	OMX_BOOL bCABAC;
	OMX_BOOL bFMO;
	OMX_BOOL bConstIntraPred;
	OMX_U8 nNumSliceGroups;
	OMX_U8 nSliceGroupMapType;
	OMX_VIDEO_AVCSLICEMODETYPE eSliceMode;
	OMX_VIDEO_CONTROLRATETYPE RateControlAlg;
	OMX_VIDEO_TRANSFORMBLOCKSIZETYPE TransformBlockSize;
	OMX_VIDEO_ENCODING_MODE_PRESETTYPE EncodingPreset;
	OMX_VIDEO_RATECONTROL_PRESETTYPE RateCntrlPreset;
	OMX_VIDEO_AVCBITSTREAMFORMATTYPE BitStreamFormat;
	OMX_U32 maxInterFrameInterval;
	OMX_U32 nBitEnableAdvanced;
	OMX_U32 nBitEnableDynamic;
	OMX_U32 nNumInputBuf;
	OMX_U32 nNumOutputBuf;
	OMX_BOOL bInAllocatebuffer;
	OMX_BOOL bOutAllocatebuffer;
	OMX_U8 TestType;
	OMX_U8 StopFrameNum;
} H264E_TestCaseParams;

typedef struct H264E_ILClient
{
	OMX_HANDLETYPE pHandle;
	OMX_STATETYPE eState;
	OMX_ERRORTYPE eAppError;

	OMX_BUFFERHEADERTYPE **pInBuff;
	OMX_BUFFERHEADERTYPE **pOutBuff;

	OMX_PTR IpBuf_Pipe;
	OMX_PTR OpBuf_Pipe;

	FILE *fIn;
	FILE *fOut;
	H264E_TestCaseParams *H264_TestCaseParams;

} H264E_ILClient;


typedef struct OMX_H264E_DynamicConfigs
{
	OMX_CONFIG_FRAMERATETYPE tFrameRate;
	OMX_VIDEO_CONFIG_BITRATETYPE tBitRate;
	OMX_VIDEO_CONFIG_MESEARCHRANGETYPE tMESearchrange;
	OMX_CONFIG_INTRAREFRESHVOPTYPE tForceframe;
	OMX_VIDEO_CONFIG_QPSETTINGSTYPE tQPSettings;
	OMX_VIDEO_CONFIG_AVCINTRAPERIOD tAVCIntraPeriod;
	OMX_VIDEO_CONFIG_NALSIZE tNALUSize;
	OMX_VIDEO_CONFIG_SLICECODINGTYPE tSliceSettings;
	OMX_VIDEO_CONFIG_PIXELINFOTYPE tPixelInfo;
} OMX_H264E_DynamicConfigs;


#endif /*_H264_ILClient_ */
