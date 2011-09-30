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
*   @file  H264_ILClien_DynamicSettings.c
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

H264E_TestCaseParamsDynamic H264_TestCaseDynTable[1] = {
	{
		    {
				{2, 20, -1}
				,
				{20, 30, 0}

			}
		    ,
		    {
				{0, 20, -1}
				,
				{128000, 50000, 0}

			}
		    ,
		    {
				{0, 5, -1}
				,
				{2, 2,}
				,
				{144, 100}
				,
				{32, 16}
				,
				{144, 50}
				,
				{16, 16}

			}
		    ,
		    {
				{5, 20, -1}
				,
				{1, 1}
			}
		    ,
		    {
				{0, 3, -1}
				,
				{25, 34}
				,
				{51, 36}
				,
				{0, 30}
				,
				{25, 37}
				,
				{51, 40}
				,
				{0, 35}
				,
				{4, 4}
				,
				{51, 44}
				,
				{0, 38}
			}
		    ,
		    {
				{0, -1}
				,
				{10}
			}
		    ,
		    {
				{-1}
				,
				{0}
			}
		    ,
		    {
				{0, -1}
				,
				{0}
				,
				{0}
			}
		    ,

		    {
				{-1}
				,
				{0}
			}

	    }

};
