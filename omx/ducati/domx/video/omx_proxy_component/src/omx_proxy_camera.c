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
 *  @file  omx_proxy_camera.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework Tunnel Proxy component.
 ******************************************************************************
 This is the proxy specific wrapper that passes the component name to the
 generic proxy init() The proxy wrapper also does some runtime/static time
 config on per proxy basis This is a thin wrapper that is called when
 componentiit() of the proxy is called  static OMX_ERRORTYPE PROXY_Wrapper_init
 (OMX_HANDLETYPE hComponent, OMX_PTR pAppData);
 this layer gets called first whenever a proxy s get handle is called
 ******************************************************************************
 *  @path WTSD_DucatiMMSW\omx\omx_il_1_x\omx_proxy_component\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "omx_proxy_common.h"
#include <OMX_TI_IVCommon.h>
#include <OMX_TI_Index.h>
#include <stdlib.h>
/* Tiler APIs */
#include "timm_osal_mutex.h"
#include <pthread.h>
#include <SysLinkMemUtils.h>
#include <ProcMgr.h>
#include <sys/time.h>
#include <memmgr.h>
#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.CAMERA"
/*Needs to be specific for every configuration wrapper*/

#define DEFAULT_DCC 1
#ifdef _Android
#define DCC_PATH  "/system/etc/omapcam/"
#else
#define DCC_PATH  "/usr/share/omapcam/"
#endif
#define LINUX_PAGE_SIZE (4 * 1024)

#define _PROXY_OMX_INIT_PARAM(param,type) do {		\
	TIMM_OSAL_Memset((param), 0, sizeof (type));	\
	(param)->nSize = sizeof (type);			\
	(param)->nVersion.s.nVersionMajor = 1;		\
	(param)->nVersion.s.nVersionMinor = 1;		\
	} while(0)

/* Incase of multiple instance, making sure DCC is initialized only for
   first instance */
static OMX_S16 numofInstance = 0;
int dcc_flag = 0;
TIMM_OSAL_PTR cam_mutex = NULL;

/* To store DCC buffer size */
OMX_S32 dccbuf_size = 0;

/* Ducati Mapped Addr  */
unsigned int pMappedBuf;
MemAllocBlock *MemReqDescTiler;
OMX_PTR TilerAddr = NULL;

OMX_S32 read_DCCdir(OMX_PTR, OMX_STRING *, OMX_U16);
OMX_ERRORTYPE DCC_Init(OMX_HANDLETYPE);
OMX_ERRORTYPE send_DCCBufPtr(OMX_HANDLETYPE hComponent);
void DCC_DeInit();
OMX_ERRORTYPE PROXY_ComponentDeInit(OMX_HANDLETYPE);
OMX_ERRORTYPE PROXY_SetConfig(OMX_HANDLETYPE, OMX_INDEXTYPE, OMX_PTR);
OMX_ERRORTYPE PROXY_GetConfig(OMX_HANDLETYPE, OMX_INDEXTYPE, OMX_PTR);
OMX_ERRORTYPE CameraMaptoTilerDuc(OMX_TI_CONFIG_SHAREDBUFFER *,
    unsigned int *, OMX_PTR *);
COREID TARGET_CORE_ID = CORE_APPM3;

static OMX_ERRORTYPE ComponentPrivateDeInit(OMX_IN OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;

	if (dcc_flag)
	{
		eOsalError =
		    TIMM_OSAL_MutexObtain(cam_mutex, TIMM_OSAL_SUSPEND);
		if (eOsalError != TIMM_OSAL_ERR_NONE)
		{
			TIMM_OSAL_Error("Mutex Obtain failed");
		}

		if (numofInstance == 1)
		{
			DCC_DeInit();
		}
		numofInstance = numofInstance - 1;

		eOsalError = TIMM_OSAL_MutexRelease(cam_mutex);
		PROXY_assert(eOsalError == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "Mutex release failed");
	}

	eError = PROXY_ComponentDeInit(hComponent);

      EXIT:
	return eError;
}

OMX_ERRORTYPE CameraMaptoTilerDuc(OMX_TI_CONFIG_SHAREDBUFFER *
    pConfigSharedBuffer, unsigned int *pDucMappedBuf,
    OMX_PTR * pBufToBeMapped)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 nBufLineSize = 0;
	MemAllocBlock block = { 0 };
	OMX_U32 nDiff = 0;
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap MpuAddr_list_1D = { 0 };
	OMX_S32 status = 0;

	nBufLineSize = pConfigSharedBuffer->nSharedBuffSize;
	if (!MemMgr_IsMapped(pConfigSharedBuffer->pSharedBuff))
	{
		DOMX_DEBUG
		    ("Buffer is not mapped: Mapping as 1D buffer now..");
		block.pixelFormat = PIXEL_FMT_PAGE;
		block.ptr =
		    (OMX_PTR) (((OMX_U32) pConfigSharedBuffer->pSharedBuff /
			LINUX_PAGE_SIZE) * LINUX_PAGE_SIZE);
		block.dim.len =
		    (OMX_U32) ((((OMX_U32) pConfigSharedBuffer->pSharedBuff +
			    nBufLineSize + LINUX_PAGE_SIZE -
			    1) / LINUX_PAGE_SIZE) * LINUX_PAGE_SIZE) -
		    (OMX_U32) block.ptr;
		block.stride = 0;

		nDiff =
		    (OMX_U32) pConfigSharedBuffer->pSharedBuff -
		    (OMX_U32) block.ptr;

		*pBufToBeMapped = MemMgr_Map(&block, 1);

		PROXY_assert(*pBufToBeMapped != 0,
		    OMX_ErrorInsufficientResources,
		    "Map to TILER space failed");
	}

	if (MemMgr_IsMapped((OMX_PTR) * pBufToBeMapped))
	{

		mapType = ProcMgr_MapType_Tiler;
		MpuAddr_list_1D.mpuAddr = (OMX_U32) (*pBufToBeMapped) + nDiff;

		MpuAddr_list_1D.size = pConfigSharedBuffer->nSharedBuffSize;
		status = SysLinkMemUtils_map(&MpuAddr_list_1D, 1,
		    pDucMappedBuf, mapType, PROC_APPM3);

		PROXY_assert(status >= 0, OMX_ErrorInsufficientResources,
		    "Syslink map failed");
		pConfigSharedBuffer->pSharedBuff = (OMX_PTR) (*pDucMappedBuf);

	}
      EXIT:
	return eError;
}

/* ===========================================================================*/
/**
 * @name CameraGetConfig()
 * @brief For some specific indices, buffer allocated on A9 side
 *        needs to be mapped and sent to Ducati.
 * @param
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/

static OMX_ERRORTYPE CameraGetConfig(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_TI_CONFIG_SHAREDBUFFER *pConfigSharedBuffer = NULL;
	OMX_PTR pTempSharedBuff = NULL;
	unsigned int pDucMappedBuf = 0;
	OMX_PTR pBufToBeMapped = NULL;
	OMX_U32 status = 0;

	switch (nParamIndex)
	{
	case OMX_TI_IndexConfigExifTags:
	case OMX_TI_IndexConfigCamCapabilities:
	case OMX_TI_IndexConfigAAAskipBuffer:
		pConfigSharedBuffer =
		    (OMX_TI_CONFIG_SHAREDBUFFER *)
		    pComponentParameterStructure;

		pTempSharedBuff = pConfigSharedBuffer->pSharedBuff;

		eError =
		    CameraMaptoTilerDuc(pConfigSharedBuffer, &pDucMappedBuf,
		    &pBufToBeMapped);
		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in mapping to TILER or Ducati");

                eError =
                  RPC_FlushBuffer(pTempSharedBuff,
                  pConfigSharedBuffer->nSharedBuffSize, TARGET_CORE_ID);
                PROXY_assert(eError == OMX_ErrorNone, OMX_ErrorHardware,
                    "Flush Buffer failed");

		eError =
		    PROXY_GetConfig(hComponent, nParamIndex,
		    pConfigSharedBuffer);
		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in GetConfig");

                eError =
                  RPC_InvalidateBuffer(pTempSharedBuff,
                  pConfigSharedBuffer->nSharedBuffSize, TARGET_CORE_ID);
                PROXY_assert(eError == OMX_ErrorNone, OMX_ErrorHardware,
                    "Invalidate Buffer failed");

		pConfigSharedBuffer->pSharedBuff = pTempSharedBuff;

		goto EXIT;
		break;

	default:
		break;
	}


	return PROXY_GetConfig(hComponent, nParamIndex,
	    pComponentParameterStructure);
      EXIT:
	if (pDucMappedBuf)
	{
		status = SysLinkMemUtils_unmap(pDucMappedBuf, PROC_APPM3);
		if (status <= 0)
			DOMX_DEBUG("Syslink unmap failed with status %d",
			    status);
		pDucMappedBuf = 0;
	}
	if (pBufToBeMapped)
	{
		status = MemMgr_UnMap(pBufToBeMapped);
		if (status != 0)
			DOMX_DEBUG("MemMgr unmap failed with status %d",
			    status);
		pBufToBeMapped = NULL;
	}
	return eError;
}

/* ===========================================================================*/
/**
 * @name CameraSetConfig()
 * @brief For some specific indices, buffer allocated on A9 side needs to
 *        be mapped and sent to Ducati.
 * @param
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/


static OMX_ERRORTYPE CameraSetConfig(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_TI_CONFIG_SHAREDBUFFER *pConfigSharedBuffer = NULL;
	OMX_PTR pTempSharedBuff = NULL;
	/* Stores the Ducati Mapped addr for the user space allocated buffers for CameraCap/Exif Tags */
	unsigned int pDucMappedBuf = 0;
	OMX_PTR pBufToBeMapped = NULL;
	OMX_U32 status = 0;
	switch (nParamIndex)
	{
	case OMX_TI_IndexConfigExifTags:
	case OMX_TI_IndexConfigCamCapabilities:
	case OMX_TI_IndexConfigAAAskipBuffer:
		pConfigSharedBuffer =
		    (OMX_TI_CONFIG_SHAREDBUFFER *)
		    pComponentParameterStructure;

		pTempSharedBuff = pConfigSharedBuffer->pSharedBuff;

		eError =
		    CameraMaptoTilerDuc(pConfigSharedBuffer, &pDucMappedBuf,
		    &pBufToBeMapped);
		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in mapping to TILER or Ducati");

		eError =
		    RPC_FlushBuffer(pTempSharedBuff,
		    pConfigSharedBuffer->nSharedBuffSize, TARGET_CORE_ID);
		PROXY_assert(eError == OMX_ErrorNone, OMX_ErrorHardware,
		    "Flush Buffer failed");

		eError =
		    PROXY_SetConfig(hComponent, nParamIndex,
		    pConfigSharedBuffer);
		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in GetConfig");

		pConfigSharedBuffer->pSharedBuff = pTempSharedBuff;

		goto EXIT;
		break;

	default:
		break;
	}


	return PROXY_SetConfig(hComponent, nParamIndex,
	    pComponentParameterStructure);

      EXIT:
	if (pDucMappedBuf)
	{
		status = SysLinkMemUtils_unmap(pDucMappedBuf, PROC_APPM3);
		if (status <= 0)
			DOMX_DEBUG("Syslink unmap failed with status %d",
			    status);
		pDucMappedBuf = 0;
	}
	if (pBufToBeMapped)
	{
		status = MemMgr_UnMap(pBufToBeMapped);
		if (status != 0)
			DOMX_DEBUG("MemMgr unmap failed with status %d",
			    status);
		pBufToBeMapped = NULL;
	}

	return eError;
}

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE dcc_eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;
	DOMX_ENTER("_____________________INSIDE CAMERA PROXY"
	    "WRAPPER__________________________\n");
	pHandle->pComponentPrivate = (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
	if (pHandle->pComponentPrivate == NULL)
	{
		DOMX_ERROR(" ERROR IN ALLOCATING PROXY COMPONENT"
		    "PRIVATE STRUCTURE");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	pComponentPrivate->cCompName =
	    TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	/*Copying component Name - this will be picked up in the proxy common */
	assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH);
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);
	/*Calling Proxy Common Init() */
	eError = OMX_ProxyCommonInit(hComponent);
	if (eError != OMX_ErrorNone)
	{
		DOMX_ERROR("\Error in Initializing Proxy");
		TIMM_OSAL_Free(pComponentPrivate->cCompName);
		TIMM_OSAL_Free(pComponentPrivate);
		goto EXIT;
	}

	pHandle->ComponentDeInit = ComponentPrivateDeInit;
	pHandle->GetConfig = CameraGetConfig;
	pHandle->SetConfig = CameraSetConfig;
	char *val = getenv("SET_DCC");
	dcc_flag = val ? strtol(val, NULL, 0) : DEFAULT_DCC;
	DOMX_DEBUG(" DCC: 0 - disabled 1 - enabled : val: %d", dcc_flag);

	if (dcc_flag)
	{
		eOsalError =
		    TIMM_OSAL_MutexObtain(cam_mutex, TIMM_OSAL_SUSPEND);
		PROXY_assert(eOsalError == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "Mutex lock failed");

		if (numofInstance == 0)
		{
			dcc_eError = DCC_Init(hComponent);
			if (dcc_eError != OMX_ErrorNone)
			{
				DOMX_DEBUG(" Error in DCC Init");
			}
		}

		numofInstance = numofInstance + 1;

		/* Configure Ducati to use DCC buffer from A9 side
		 *ONLY* if DCC_Init is successful. */
		if (dcc_eError == OMX_ErrorNone)
		{
			dcc_eError = send_DCCBufPtr(hComponent);
			if (dcc_eError != OMX_ErrorNone)
			{
				DOMX_DEBUG(" Error in Sending DCC Buf ptr");
			}
		}

		eOsalError = TIMM_OSAL_MutexRelease(cam_mutex);
		PROXY_assert(eOsalError == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "Mutex release failed");
	}
      EXIT:
	return eError;
}

/* ===========================================================================*/
/**
 * @name DCC_Init()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE DCC_Init(OMX_HANDLETYPE hComponent)
{
	OMX_TI_PARAM_DCCURIINFO param;
	OMX_PTR ptempbuf;
	OMX_U16 nIndex = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap MpuAddr_list_1D = { 0 };
	OMX_S32 status = 0;
    OMX_STRING dcc_cfg_file[200];
	OMX_U16 i;
	_PROXY_OMX_INIT_PARAM(&param, OMX_TI_PARAM_DCCURIINFO);

	DOMX_ENTER("ENTER");

	/* Read the the DCC URI info */
	for (nIndex = 0; eError != OMX_ErrorNoMore; nIndex++)
	{
		param.nIndex = nIndex;
		eError =
		    OMX_GetParameter(hComponent,
		    OMX_TI_IndexParamDccUriInfo, &param);

		PROXY_assert((eError == OMX_ErrorNone) ||
		    (eError == OMX_ErrorNoMore), eError,
		    "Error in GetParam for Dcc URI info");


		if (eError == OMX_ErrorNone)
		{
			DOMX_DEBUG("DCC URI's %s ", param.sDCCURI);
            dcc_cfg_file[nIndex] =
			    TIMM_OSAL_Malloc(sizeof(OMX_U8) *
			    (strlen(DCC_PATH) + MAX_URI_LENGTH + 1),
			    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
            PROXY_assert(dcc_cfg_file[nIndex] != NULL,
			    OMX_ErrorInsufficientResources, "Malloc failed");
            strcpy(dcc_cfg_file[nIndex], DCC_PATH);
            strcat(dcc_cfg_file[nIndex], (OMX_STRING) param.sDCCURI);
            //strcat(dcc_cfg_file[nIndex], "/");
            strcat(dcc_cfg_file[nIndex], ".cfg");
		}
	}
	/* setting  back errortype OMX_ErrorNone */
	if (eError == OMX_ErrorNoMore)
	{
		eError = OMX_ErrorNone;
	}

    dccbuf_size = read_DCCdir(NULL, dcc_cfg_file, nIndex);

	PROXY_assert(dccbuf_size > 0, OMX_ErrorInsufficientResources,
	    "No DCC files found, switching back to default DCC");

	MemReqDescTiler =
	    (MemAllocBlock *) TIMM_OSAL_Malloc((sizeof(MemAllocBlock) * 2),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT);
	PROXY_assert(MemReqDescTiler != NULL,
	    OMX_ErrorInsufficientResources, "Malloc failed");

	/* Allocate 1D Tiler buffer for 'N'DCC files  */
	MemReqDescTiler[0].pixelFormat = PIXEL_FMT_PAGE;
	MemReqDescTiler[0].dim.len = dccbuf_size;
	MemReqDescTiler[0].stride = 0;
	TilerAddr = MemMgr_Alloc(MemReqDescTiler, 1);
	PROXY_assert(TilerAddr != NULL,
	    OMX_ErrorInsufficientResources, "ERROR Allocating 1D TILER BUF");

	ptempbuf = TilerAddr;
    dccbuf_size = read_DCCdir(ptempbuf, dcc_cfg_file, nIndex);

	PROXY_assert(dccbuf_size > 0, OMX_ErrorInsufficientResources,
	    "ERROR in copy DCC files into buffer");

	mapType = ProcMgr_MapType_Tiler;
	MpuAddr_list_1D.mpuAddr = ((OMX_U32) TilerAddr);
	MpuAddr_list_1D.size = dccbuf_size;
	status = SysLinkMemUtils_map(&MpuAddr_list_1D, 1,
	    &pMappedBuf, mapType, PROC_APPM3);

	PROXY_assert(status >= 0, OMX_ErrorInsufficientResources,
	    "Syslink map failed");

	DOMX_EXIT("eError: %d", eError);
      EXIT:
	for (i = 0; i < nIndex - 1; i++)
	{
        if (dcc_cfg_file[i])
            TIMM_OSAL_Free(dcc_cfg_file[i]);
	}
	return eError;

}

/* ===========================================================================*/
/**
 * @name send_DCCBufPtr()
 * @brief : Sending the DCC uri buff addr to ducati
 * @param void
 * @return return = 0 is successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

OMX_ERRORTYPE send_DCCBufPtr(OMX_HANDLETYPE hComponent)
{
	OMX_TI_CONFIG_SHAREDBUFFER uribufparam;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	_PROXY_OMX_INIT_PARAM(&uribufparam, OMX_TI_CONFIG_SHAREDBUFFER);
	uribufparam.nPortIndex = OMX_ALL;

	DOMX_ENTER("ENTER");

	uribufparam.nSharedBuffSize = dccbuf_size;
	uribufparam.pSharedBuff = (OMX_U8 *) pMappedBuf;

	DOMX_DEBUG("SYSLINK MAPPED ADDR:  0x%x sizeof buffer %d",
	    uribufparam.pSharedBuff, uribufparam.nSharedBuffSize);

	eError =
	    OMX_SetParameter(hComponent, OMX_TI_IndexParamDccUriBuffer,
	    &uribufparam);
	if (eError != OMX_ErrorNone)
	{
		DOMX_ERROR(" Error in SetParam for DCC Uri Buffer 0x%x", eError);
	}

	DOMX_EXIT("EXIT");
	return eError;
}

/* ===========================================================================*/
/**
 * @name read_DCCdir()
 * @brief : copies all the dcc profiles into the allocated 1D-Tiler buffer
 *          and returns the size of the buffer.
 * @param void : OMX_PTR is null then returns the size of the DCC directory
 * @return return = size of the DCC directory or error in case of any failures
 *		    in file read or open
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_S32 read_DCCdir(OMX_PTR buffer, OMX_STRING * dcc_cfg_file, OMX_U16 numofURI)
{
    FILE* pDCCcfg;
	OMX_S32 lSize;
	OMX_S32 dcc_buf_size = 0;
	struct dirent *dir;
	OMX_U16 i = 0;
	OMX_S32 ret = 0;
	DOMX_ENTER("ENTER");
	for (i = 0; i < numofURI - 1; i++)
	{
        pDCCcfg =  fopen(dcc_cfg_file[i], "rb");
        DOMX_DEBUG(" Config file %s \n", dcc_cfg_file[i]);

        if(pDCCcfg != NULL)
        {
            DIR *d;
            OMX_STRING path[200];
            size_t result;
            while(fscanf (pDCCcfg, "%s", path) > 0)
            {
                DOMX_DEBUG(" Trying to open directory %s \n", path);

                d = opendir(path);
		if (d)
		{
                    FILE *pFile;
                    OMX_STRING filename;
                    char temp[200];
                    OMX_STRING dotdot = "..";

                    DOMX_DEBUG(" Read each filename \n");


			/* read each filename */
			while ((dir = readdir(d)) != NULL)
			{
				filename = dir->d_name;
                        strcpy(temp, path);
				strcat(temp, filename);

                        DOMX_DEBUG(" Filename %s\n", temp);
				if ((*filename != *dotdot))
				{
                    DOMX_DEBUG("\n\t DCC Profiles copying into buffer => %s mpu_addr: %p \n",
					    temp, buffer);

					pFile = fopen(temp, "rb");
					if (pFile == NULL)
					{
                        DOMX_DEBUG("File open error\n");

						ret = -1;
					} else
					{
						fseek(pFile, 0, SEEK_END);
						lSize = ftell(pFile);
						rewind(pFile);
						/* buffer is not NULL then copy all the DCC profiles into buffer
						   else return the size of the DCC directory */
						if (buffer)
						{
							// copy file into the buffer:
							result =
							    fread(buffer, 1,
							    lSize, pFile);
							if (result != lSize)
							{
                                DOMX_DEBUG("fread: Reading error\n");

								ret = -1;
							}
							buffer =
							    buffer + lSize;
						}
						/* getting the size of the total dcc files available in FS */
						dcc_buf_size =
						    dcc_buf_size + lSize;
						fclose(pFile);
					}
				}
			}
			closedir(d);
		}
                else
                {
                    DOMX_DEBUG(" Trying to open file %s \n", path);
                    FILE *pFile = fopen(path, "rb");
                    if(pFile != NULL)
                    {
                        DOMX_DEBUG("\n\t DCC Profiles copying into buffer => %s mpu_addr: %p\n", path, buffer);

                        fseek(pFile, 0, SEEK_END);
                        lSize = ftell(pFile);
                        rewind(pFile);
                        /* buffer is not NULL then copy all the DCC profiles into buffer
                           else return the size of the DCC directory */
                        if (buffer)
                        {
                            // copy file into the buffer:
                            result =
                                fread(buffer, 1,
                                lSize, pFile);
                            if (result != lSize)
                            {
                                DOMX_DEBUG("fread: Reading error\n");

                                ret = -1;
                            }
                            buffer =
                                buffer + lSize;
                        }
                        /* getting the size of the total dcc files available in FS */
                        dcc_buf_size =
                            dcc_buf_size + lSize;
			fclose(pFile);
                    }
                    else
                    {
                        //ERROR - no such directory or file present
                        DOMX_DEBUG("No such directory or file present!!!\n");

                    }
                }
            }
            fclose(pDCCcfg);
        }
        else
        {
            DOMX_DEBUG("Config file open error\n");
        }
	}
	if (ret == 0)
		ret = dcc_buf_size;


	DOMX_EXIT("return %d", ret);
	return ret;
}

/* ===========================================================================*/
/**
 * @name DCC_Deinit()
 * @brief
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void DCC_DeInit()
{
	DOMX_ENTER("ENTER");
	if (pMappedBuf)
		SysLinkMemUtils_unmap(pMappedBuf, PROC_APPM3);
	if (TilerAddr)
		MemMgr_Free(TilerAddr);
	if (MemReqDescTiler)
		TIMM_OSAL_Free(MemReqDescTiler);
	DOMX_EXIT("EXIT");
}



/*===============================================================*/
/** @fn Cam_Setup : This function is called when the the OMX Camera library is
 *                  loaded. It creates a mutex, which is used during DCC_Init()
 */
/*===============================================================*/
void __attribute__ ((constructor)) Cam_Setup(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexCreate(&cam_mutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of default mutex failed");
	}
}


/*===============================================================*/
/** @fn Cam_Destroy : This function is called when the the OMX Camera library is
 *                    unloaded. It destroys the mutex which was created by
 *                    Core_Setup().
 *
 */
/*===============================================================*/
void __attribute__ ((destructor)) Cam_Destroy(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexDelete(cam_mutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Destruction of default mutex failed");
	}
}
