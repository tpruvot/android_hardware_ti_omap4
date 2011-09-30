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
* @file OMX_Main.h
*
* This File contains functions declaration for Test Case API
* functionality .
*
* @path
*
* @rev
*/

/* ----- system and platform files ----------------------------*/
#include <stdint.h>
#include <string.h>
#include <timm_osal_types.h>


/**
* THC TestCase return values
*/
typedef enum OMX_TestStatus
{
	OMX_RET_FAIL = 0,
	OMX_RET_PASS = 1,
	OMX_RET_SKIP = 2,
	OMX_RET_ABORT = 3,
	OMX_RET_MEM = 4
} OMX_TestStatus;


/**
* Function Types
*  testcase handling procedures
*/
typedef OMX_TestStatus(*OMXTestproc) (uint32_t uMsg, void *pParam,
    uint32_t paramSize);


/**
* Function Table Entry Structure
*/
typedef struct OMX_TEST_CASE_ENTRY
{

	TIMM_OSAL_CHAR *pTestID;	/* uniquely identifies the test - used in loading/saving scripts */
	TIMM_OSAL_CHAR *pTestDescription;	/* description of test */
	TIMM_OSAL_CHAR *pUserData;	/* user defined data that will be passed to TestProc at runtime */
	TIMM_OSAL_CHAR *pDynUserData;	/* user defined data that will be passed to TestProc at runtime */
	uint32_t depth;		/* depth of item in tree hierarchy */
	OMXTestproc pTestProc;	/* pointer to TestProc function to be called for this test */


} OMX_TEST_CASE_ENTRY;


/**
* TESTCASE Engine message values
*/

#define OMX_MSG_EXECUTE     1
