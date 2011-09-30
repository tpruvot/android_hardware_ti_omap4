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
*   @file  MPEG4_ILClient.h
*   This file contains the structures and macros that are used in the Application which tests the OpenMAX component
*
*  @path \WTSD_DucatiMMSW\omx\khronos1.1\omx_mpeg4_enc\test
*
*  @rev 1.0
*/

#ifndef _MPEG4_ILClient_
#define _MPEG4_ILClient_


/****************************************************************
 * INCLUDE FILES
 ****************************************************************/
/**----- system and platform files ----------------------------**/
#ifndef MPEG4_LINUX_CLIENT
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/System.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef MPEG4_LINUX_CLIENT
#include <timm_osal_interfaces.h>
#else
#include <WTSD_DucatiMMSW/platform/osal/timm_osal_interfaces.h>
#endif

#ifndef MPEG4_LINUX_CLIENT
#define MPEG4ENC_TRACE_ENABLE
#ifdef MPEG4ENC_TRACE_ENABLE
	/*overwriting the default trcae flags */
#define TIMM_OSAL_DEBUG_TRACE_DETAIL 0
#define TIMM_OSAL_DEBUG_TRACE_LEVEL 4
#endif
#endif
#ifdef MPEG4_LINUX_CLIENT
#include <timm_osal_trace.h>
#else
#include <WTSD_DucatiMMSW/platform/osal/timm_osal_trace.h>
#endif

/**-------program files ----------------------------------------**/
#ifdef MPEG4_LINUX_CLIENT
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_TI_Video.h>
#include <OMX_TI_Index.h>
#include <OMX_TI_Common.h>
#else
#include "omx_core.h"
#include "omx_component.h"
#include "omx_video.h"
#include "omx_ivcommon.h"
#include "omx_ti_video.h"
#include "omx_ti_index.h"
#include "omx_ti_common.h"
#endif

#ifdef MPEG4_LINUX_CLIENT
#define MPEG4CLIENT_TRACE_PRINT(ARGS,...)  TIMM_OSAL_Debug(ARGS,##__VA_ARGS__)
#define MPEG4CLIENT_ENTER_PRINT()	         TIMM_OSAL_Entering("")
#define MPEG4CLIENT_EXIT_PRINT(ARG)           TIMM_OSAL_Exiting("")
#define MPEG4CLIENT_ERROR_PRINT(ARGS,...)  TIMM_OSAL_Error(ARGS,##__VA_ARGS__ )
#define MPEG4CLIENT_INFO_PRINT(ARGS,...)     TIMM_OSAL_Info(ARGS,##__VA_ARGS__ )
#else
static TIMM_OSAL_TRACEGRP TraceGrp;
#define MPEG4CLIENT_TRACE_PRINT(ARGS,...)   TIMM_OSAL_Debug(ARGS,##__VA_ARGS__)
#define MPEG4CLIENT_ENTER_PRINT()               TIMM_OSAL_Entering("")
#define MPEG4CLIENT_EXIT_PRINT(ARG)            TIMM_OSAL_ExitingExt("")
#define MPEG4CLIENT_ERROR_PRINT(ARGS,...)   TIMM_OSAL_Error(ARGS,##__VA_ARGS__ )
#define MPEG4CLIENT_INFO_PRINT(ARGS,...)     TIMM_OSAL_Info(ARGS,##__VA_ARGS__ )
#endif


#define GOTO_EXIT_IF(_CONDITION,_ERROR) { \
            if((_CONDITION)) { \
                MPEG4CLIENT_ERROR_PRINT("%s : %s : %d :: ", __FILE__, __FUNCTION__,__LINE__);\
                MPEG4CLIENT_ERROR_PRINT(" Exiting because: %s \n", #_CONDITION); \
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
} MPEG4EDataSyncSettings;



/*Dynamic Params Settings*/
typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nFramerate;
} FrameRate;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nBitrate;
} BitRate;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nMVAccuracy;
	OMX_U32 *nHorSearchRangeP;
	OMX_U32 *nVerSearchRangeP;
	OMX_U32 *nHorSearchRangeB;
	OMX_U32 *nVerSearchRangeB;
} MESearchRange;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_BOOL *ForceIFrame;
} ForceFrame;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nQpI;
	OMX_U32 *nQpMaxI;
	OMX_U32 *nQpMinI;
	OMX_U32 *nQpP;
	OMX_U32 *nQpMaxP;
	OMX_U32 *nQpMinP;
	OMX_U32 *nQpOffsetB;
	OMX_U32 *nQpMaxB;
	OMX_U32 *nQpMinB;
} QpSettings;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nIntraFrameInterval;
} IntraFrameInterval;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nNaluSize;
} NALSize;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_VIDEO_AVCSLICEMODETYPE *eSliceMode;
	OMX_U32 *nSlicesize;
} SliceCodingSettings;


typedef struct
{
	OMX_S32 *nFrameNumber;
	OMX_U32 *nWidth;
	OMX_U32 *nHeight;
} PixelInfo;





typedef struct MPEG4E_TestCaseParamsAdvanced
{
	NALUSettings NALU;
	MEBlockSize MEBlock;
	IntraRefreshSettings IntraRefresh;
	VUISettings VUI;
	IntrapredictionSettings IntraPred;
	MPEG4EDataSyncSettings DataSync;
} MPEG4E_TestCaseParamsAdvanced;

typedef struct MPEG4E_TestCaseParamsDynamic
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

} MPEG4E_TestCaseParamsDynamic;


typedef enum MPEG4E_TestType
{
	FULL_RECORD = 0,
	PARTIAL_RECORD = 1
} MPEG4E_TestType;


typedef struct MPEG4E_TestCaseParams
{
	OMX_U8 TestCaseId;
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
} MPEG4E_TestCaseParams;

typedef struct MPEG4E_ILClient
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
	MPEG4E_TestCaseParams *MPEG4_TestCaseParams;
	MPEG4E_TestCaseParamsDynamic *MPEG4_TestCaseParamsDynamic;
	MPEG4E_TestCaseParamsAdvanced *MPEG4_TestCaseParamsAdvanced;
	OMX_S64 TimeStampsArray[20];
	OMX_U8 nFillBufferDoneCount;

} MPEG4E_ILClient;


typedef struct OMX_MPEG4E_DynamicConfigs
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
} OMX_MPEG4E_DynamicConfigs;


#endif /*_MPEG4_ILClient_ */
