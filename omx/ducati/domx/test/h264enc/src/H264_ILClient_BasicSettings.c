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
*   @file  H264_ILClien_BasicSettings.c
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

H264E_TestCaseParams H264_TestCaseTable[1] = {
	{
		    0,
		    "input/foreman_p176x144_30fps_420pl_300fr_nv12.yuv",	/*input file */
		    "output/foreman_p176x144_20input_1output.264",	/*output file */
		    176,
		    144,
		    OMX_VIDEO_AVCProfileHigh,
		    OMX_VIDEO_AVCLevel41,
		    OMX_COLOR_FormatYUV420PackedSemiPlanar,
		    0,		/*progressive or interlace */
		    0,		/*interlace type */
		    OMX_FALSE,	/*bloopfillter */
		    OMX_FALSE,	/*bCABAC */
		    OMX_FALSE,	/*bFMO */
		    OMX_FALSE,	/*bconstIntrapred */
		    0,		/*slicegrps */
		    0,		/*slicegrpmap */
		    OMX_VIDEO_SLICEMODE_AVCDefault,	/*slice mode */
		    OMX_Video_ControlRateDisable,	/*rate cntrl */
		    OMX_Video_Transform_Block_Size_4x4,	/*transform block size */
		    OMX_Video_Enc_User_Defined,	/*Encoder preset */
		    OMX_Video_RC_User_Defined,	/*RC Preset */
		    OMX_Video_BitStreamFormatByte,	/*Bit Stream Select */
		    1,		/*max interframe interval */
		    0,		/*bit enable_advanced */
		    0,		/*bit enable_dynamic */
		    1,		/*NumInputBuf */
		    1,		/*NumOutputBuf */
		    OMX_TRUE,	/*flag_Input_allocate buffer */
		    OMX_TRUE,	/*flag_Output_allocate buffer */
		    PARTIAL_RECORD,	/*test type */
		    10		/*stop frame number in case of partial encode */
	    }
};
