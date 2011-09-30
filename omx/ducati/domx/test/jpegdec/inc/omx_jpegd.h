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
*
*
* This File contains testcase table for JPEG decoder.
*
* @path
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ==========================================================================
*! Initial Version
* =========================================================================== */

/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/

/*-------program files ----------------------------------------*/
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IVCommon.h>
#include <OMX_TI_IVCommon.h>
#include <timm_osal_interfaces.h>


/* defines the major version of the Jpeg Decoder Component */
#define OMX_JPEGD_TEST_COMP_VERSION_MAJOR      1
#define OMX_JPEGD_TEST_COMP_VERSION_MINOR      1
#define OMX_JPEGD_TEST_COMP_VERSION_REVISION   0
#define OMX_JPEGD_TEST_COMP_VERSION_STEP       0

/* Maximum Number of ports for the Jpeg Decoder Comp */
#define  OMX_JPEGD_TEST_NUM_PORTS   2

/* Deafult portstartnumber of Jpeg Decoder comp */
#define  DEFAULT_START_PORT_NUM     0

/* Input port Index for the Jpeg Decoder OMX Comp */
#define OMX_JPEGD_TEST_INPUT_PORT   0

/* Output port Index for the Jpeg Decoder OMX Comp */
#define OMX_JPEGD_TEST_OUTPUT_PORT  1

#define OMX_JPEGD_TEST_PIPE_SIZE      1024
#define OMX_JPEGD_TEST_PIPE_MSG_SIZE  4	//128 /* Do not want to use this so check */

/*
 *     M A C R O S
 */

#define JPEGD_STRUCT_INIT(_s_, _name_)	\
    TIMM_OSAL_Memset(&(_s_), 0x0, sizeof(_name_));	\
    (_s_).nSize = sizeof(_name_);		\
    (_s_).nVersion.s.nVersionMajor = OMX_JPEGD_TEST_COMP_VERSION_MAJOR;	  \
    (_s_).nVersion.s.nVersionMinor = OMX_JPEGD_TEST_COMP_VERSION_MINOR;	  \
    (_s_).nVersion.s.nRevision     = OMX_JPEGD_TEST_COMP_VERSION_REVISION; \
    (_s_).nVersion.s.nStep         = OMX_JPEGD_TEST_COMP_VERSION_STEP;


/* ======================================================================= */
/**
 * @def JPEGD_ASSERT  - Macro to check Parameters. Exit with passed status
 * argument if the condition assert fails. Note that use of this requires
 * declaration of local variable and a label named
 * "EXIT" typically at the end of the function
 */
/* ======================================================================= */

//#define TIMM_OSAL_Malloc(size) \
//     TIMM_OSAL_Malloc(size, TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT)

#define JPEGD_ASSERT   JPEGD_PARAMCHECK
#define JPEGD_REQUIRE  JPEGD_PARAMCHECK
#define JPEGD_ENSURE   JPEGD_PARAMCHECK

#define JPEGD_PARAMCHECK(C,V,S)  \
if (!(C)) {\
eError = V;\
TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,S);\
TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,":*****Failed\r\n");\
goto EXIT; }

/*===================================================================*/
/**
 * JPEG_DEC_GOTO_EXIT_IF macro is used to return from functions
 * when the given condition is satisfied. If CONDITION evaluates to TRUE,
 * it causes control to jump to the label EXIT, which is typically
 * placed at the end of the function. Status returned by the function
 * will be set to the value passed in ERRORCODE.
 */
/*===================================================================*/

#define JPEG_DEC_GOTO_EXIT_IF(CONDITION, ERRORCODE) {\
    if ((CONDITION)) {\
        if(ERRORCODE != OMX_ErrorNone)\
        TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_OMXIMGDEC,"  %s " #CONDITION \
        ", %s line %d", ERRORCODE, __FILE__, __LINE__);\
        eError = ERRORCODE;\
        goto EXIT;\
    }\
}

/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/


/****************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
****************************************************************/
/*--------data declarations -----------------------------------*/

/* typedef enum OMX_JPEG_DECODESOURCETYPE
{
    OMX_JPEG_DecodeSourceMemory
} OMX_JPEG_DECODESOURCETYPE; */

/*Jpeg Decoder Test Case Object*/
typedef struct JPEGDTestCaseParams
{
	OMX_U32 unOpImageWidth;
	OMX_U32 unOpImageHeight;
	OMX_U32 unSliceHeight;
	//OMX_U32 unQFactor;
	OMX_COLOR_FORMATTYPE eInColorFormat;
	OMX_COLOR_FORMATTYPE eOutColorFormat;
	OMX_JPEG_UNCOMPRESSEDMODETYPE eOutputImageMode;
	OMX_BOOL bIsApplnMemAllocatorforInBuff;
	OMX_BOOL bIsApplnMemAllocatorforOutBuff;

} JPEGDTestCaseParams;



/********************/
/* For EXIF reading */
typedef struct OMX_EXIF_THUMBNAIL_INFO
{
	OMX_U32 ImageWidth;
	OMX_U32 ImageHeight;
	OMX_U32 StripOffset;
	OMX_U32 RowsPerStrip;
	OMX_U32 StripByteCount;
	OMX_U32 PlanarConfiguration;
	OMX_U32 YCbCrSubSamplingOffset;
	OMX_U32 YCbCrPositioning;
	OMX_U32 SamplesPerPixel;
	OMX_U32 PhotometricInterpretation;
	OMX_U32 Compression;
	OMX_U32 BitsPerSampleOffset;
	OMX_U32 JPEGInterchangeFormat;
	OMX_U32 JPEGInterchangeFormatLength;
	//OMX_U32   JPEGImageOffset;

	OMX_U32 nFields;	/*total number of fields in the EXIF Thumbnail that are to be supported */

} OMX_EXIF_THUMBNAIL_INFO;


typedef enum
{
	EXIF_ERROR,
	EXIF_NOT_SUPPORTED,
	EXIF_BITSTREAM_FORMAT_ERROR,
	EXIF_SUCCESS
} OMX_EXIF_RETURN_PARAM;


typedef enum
{
	PARSE_EXIF_INFO,
	WRITE_EXIF_INFO
} EXIF_OPERATION;


typedef enum
{
	TAG_TYPE_BYTE = 0x1,
	TAG_TYPE_ASCII = 0x2,
	TAG_TYPE_SHORT = 0x3,
	TAG_TYPE_LONG = 0x4,
	TAG_TYPE_RATIONAL = 0x5,
	TAG_TYPE_UNDEFINED = 0x7,
	TAG_TYPE_SLONG = 0x9,
	TAG_TYPE_SRATIONAL = 0xA
} EXIF_TAG_TYPE;


typedef enum
{				// Enumerator for the file orientation
	Exif_OrientationUnknown = 0x00,
	Exif_OrientationTopLeft = 0x01,
	Exif_OrientationTopRight = 0x02,
	Exif_OrientationBottomRight = 0x03,
	Exif_OrientationBottomLeft = 0x04,
	Exif_OrientationLeftTop = 0x05,
	Exif_OrientationRightTop = 0x06,
	Exif_OrientationRightBottom = 0x07,
	Exif_OrientationLeftBottom = 0x08
} Exif_Orientation;


typedef struct OMX_EXIF_INFO_SUPPORTED
{
	Exif_Orientation /*uint16 */ Orientation;
	char *pCreationDateTime;
	char *pLastChangeDateTime;
	char *pImageDescription;
	char *pMake;
	char *pModel;
	char *pSoftware;
	char *pArtist;
	char *pCopyright;
	OMX_U32 ulImageWidth;
	OMX_U32 ulImageHeight;
	OMX_U32 ulExifSize;
	OMX_EXIF_THUMBNAIL_INFO *pThumbnailInfo;
	OMX_U32 nFields;	/*total number of fields in the EXIF format that are to be supported */

} OMX_EXIF_INFO_SUPPORTED;


/*Jpeg Decoder Test Case Object*/
typedef struct JpegDecoderTestObject
{
	/* Semaphores for storing various events */
	TIMM_OSAL_PTR pOmxJpegdecSem_dummy;
	TIMM_OSAL_PTR pOmxJpegdecSem_events;
	TIMM_OSAL_PTR pOmxJpegdecSem_ETB;
	TIMM_OSAL_PTR pOmxJpegdecSem_FTB;

	/* Data pipes for both the ports */
	TIMM_OSAL_PTR dataPipes[OMX_JPEGD_TEST_NUM_PORTS];

	/*OMX_JPEG DEC Handle */
	OMX_PTR hOMXJPEGD;

	OMX_BUFFERHEADERTYPE *pInBufHeader, *pOutBufHeader;

	/*Set parameter structures */
	OMX_PORT_PARAM_TYPE tJpegdecPortInit;
	OMX_PARAM_PORTDEFINITIONTYPE tInputPortDefnType;
	OMX_PARAM_PORTDEFINITIONTYPE tOutputPortDefnType;
	OMX_IMAGE_PARAM_PORTFORMATTYPE tJpegdecPortFormat;
	//OMX_IMAGE_PARAM_QFACTORTYPE tQfactor;
	OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE tUncompressedMode;
	OMX_IMAGE_PARAM_DECODE_SUBREGION pSubRegionDecode;

	/*Buffer header sent during EmptyThisBuffer Call */
	OMX_BUFFERHEADERTYPE tBufHeader;

	/*TestCase Parameter structure */
	JPEGDTestCaseParams tTestcaseParam;

	/** Pointer to EXIF Data Structure */
	OMX_EXIF_INFO_SUPPORTED *pExifInfoSupport;

} JpegDecoderTestObject;


/*--------function prototypes ---------------------------------*/

OMX_TestStatus JPEGD_TestFrameMode(uint32_t uMsg, void *pParam,
    uint32_t paramSize);
OMX_TestStatus JPEGD_TestSliceMode(uint32_t uMsg, void *pParam,
    uint32_t paramSize);

OMX_ERRORTYPE OMX_JPEGD_TEST_EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBufferHeader);

OMX_ERRORTYPE OMX_JPEGD_TEST_FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBufferHeader);

OMX_ERRORTYPE OMX_JPEGD_TEST_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData);

OMX_TestStatus JPEGD_TestEntry_Exif(uint32_t uMsg, void *pParam,
    uint32_t paramSize);
