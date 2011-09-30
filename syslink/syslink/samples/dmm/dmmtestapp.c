/*
 *  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*============================================================================
 *  @file   dmmtestapp.c
 *
 *  @brief  DMM test cases
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>

/* RCM headers */
#include <RcmClient.h>
#include <RcmServer.h>

/* IPC headers */
#include <IpcUsr.h>
#include <ProcMgr.h>
#include <SysLinkMemUtils.h>

/* Tiler header file */
#include <tilermem.h>
#include <tilermgr.h>
#include <memmgr.h>

/* RCM headers */
#include <RcmClient.h>

/* Sample headers */
#include <MemAllocTest_Config.h>

#define MAX_ERRORS		100
#define DMMTEST_PAGE_SIZE	0x1000

/* work around for segfault when atoi gets a null ptr */
#define ATOI(p) ((p) ? (atoi(p)) : (0))

enum {
	DMM_BUFFER = 1,
	USE_BUFFER = 2,
	MAP_BUFFER = 3,
	FLUSH_BUFFER_NEG = 4,
	MAP_NO_UNMAP = 5,
	MAP_IO_BUFFER = 6,
	MMU_FAULT_1 = 7,
	COPY_BUFFER = 8,
	DMM_FILE_COPY = 9,
	DMM_FILE_TILER_COPY = 10,
	COPY_BUFFER_RANGE  = 11,
	DMM_TESTS_NUM = COPY_BUFFER_RANGE
};
/*!
 *  @brief  Structure defining RCM remote function arguments
 */
typedef struct {
	uint32_t num_bytes;
	 /*!< Size of the Buffer */
	Ptr buf_ptr;
	 /*!< Buffer that is passed */
} RCM_Remote_FxnArgs;

/*!
 *  @brief  Structure defining RCM remote function arguments for
 *          buffer copy to another buffer
 */
struct rcm_remote_bufcopy_fxnargs {
	uint32_t num_bytes;
	 /*!< Size of the Buffer */
	Ptr src_ptr;
	/*!< Buffer that is passed */
	Ptr dest_ptr;
	/*!< Buffer that is received */
};

uint32_t           fxn_buffer_copy_idx;
uint32_t           fxn_buffer_test_idx;
uint32_t           fxn_exit_idx;
static RcmClient_Message  *return_msg;
static RcmClient_Handle   rcm_client_handle;
ProcMgr_Handle		proc_mgr_handle;
ProcMgr_Handle		proc_mgr_handle1;

/*
 *  ======== ipcSetup ========
 */
int ipcSetup(int proc_id)
{
	int			status = 0;
	Ipc_Config		config;
	RcmClient_Params	rcm_client_params;
	char			*remote_server_name;
	int			count = 0;
	ProcMgr_AttachParams	attachParams;
	ProcMgr_State		state;
	int			proc_sysm3 = 0;
	int			proc_appm3 = 0;

	Osal_printf("ipcSetup: Setup IPC componnets\n");

	Ipc_getConfig(&config);
	status = Ipc_setup(&config);
	if (status < 0) {
		Osal_printf("ipcSetup: Error in Ipc_setup [0x%x]\n", status);
		goto exit;
	}
	Osal_printf("ipcSetup: Ipc_setup status [0x%x]\n", status);

	if (proc_id == PROC_SYSM3)
		proc_sysm3 = 1;
	else if (proc_id == PROC_APPM3) {
		proc_sysm3 = 1;
		proc_appm3 = 1;
	} else {
		Osal_printf("ipcSetup: INVALID PROCID\n");
		goto exit;
	}

	if (proc_sysm3) {
		/* Open a handle to the ProcMgr instance. */
		status = ProcMgr_open(&proc_mgr_handle, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ipcSetup: Error in ProcMgr_open[0x%x]\n",
									status);
			goto exit;
		}

		Osal_printf("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
		ProcMgr_getAttachParams(NULL, &attachParams);
		/* Default params will be used if NULL is passed. */
		status = ProcMgr_attach(proc_mgr_handle, &attachParams);
		if (status < 0) {
			Osal_printf("ipcSetup: ProcMgr_attach failed [0x%x]\n",
									status);
		} else {
			Osal_printf("ipcSetup: ProcMgr_attach status: [0x%x]\n",
									status);
			state = ProcMgr_getState(proc_mgr_handle);
			Osal_printf("ipcSetup: After attach: ProcMgr_getState\n"
						"state [0x%x]\n", state);
		}
	}

	if (proc_appm3) {
		/* Open a handle to the ProcMgr instance. */
		status = ProcMgr_open(&proc_mgr_handle1, PROC_APPM3);
		if (status < 0) {
			Osal_printf("ipcSetup: Error in ProcMgr_open[0x%x]\n",
									status);
			goto exit;
		}
		Osal_printf("ipcSetup: ProcMgr_open Status [0x%x]\n", status);
		ProcMgr_getAttachParams(NULL, &attachParams);
		/* Default params will be used if NULL is passed. */
		status = ProcMgr_attach(proc_mgr_handle1, &attachParams);
		if (status < 0) {
			Osal_printf("ipcSetup: ProcMgr_attach failed [0x%x]\n",
									status);
		} else {
			Osal_printf("ipcSetup: ProcMgr_attach status: [0x%x]\n",
									status);
			state = ProcMgr_getState(proc_mgr_handle1);
			Osal_printf("ipcSetup: After attach: ProcMgr_getState\n"
						"    state [0x%x]\n", state);
		}
	}

	/* Set up RCM */

	/* Rcm client module init*/
	Osal_printf("RCM Client module init.\n");
	RcmClient_init();

	/* Rcm client module params init*/
	Osal_printf("RCM Client module params init.\n");
	status = RcmClient_Params_init(&rcm_client_params);
	if (status < 0) {
		Osal_printf("Error in RCM Client instance params init\n");
		goto exit;
	} else {
		Osal_printf("RCM Client instance params init passed\n");
	}

	if (proc_id == PROC_SYSM3)
		remote_server_name = RCM_SERVER_NAME_SYSM3;
	else if (proc_id == PROC_APPM3)
		remote_server_name = RCM_SERVER_NAME_APPM3;
	else
		remote_server_name = NULL;

	rcm_client_params.heapId = RCM_MSGQ_TILER_HEAPID;

	/* create an rcm client instance */
	Osal_printf("Creating RcmClient instance %s.\n", remote_server_name);
	rcm_client_params.callbackNotification = 0;

	while ((rcm_client_handle == NULL) && (count++ < MAX_CREATE_ATTEMPTS)) {
		status = RcmClient_create(remote_server_name,
				&rcm_client_params, &rcm_client_handle);
		if (status < 0) {
			if (status == RcmClient_E_SERVERNOTFOUND) {
				Osal_printf("Unable to open remote server"
						"%d time\n", count);
			} else {
				Osal_printf("Error in RCM Client create status"
							"= %d\n", status);
				goto exit;
			}
		} else {
			Osal_printf("RCM Client create passed\n");
		}
	}
	if (MAX_CREATE_ATTEMPTS <= count) {
		Osal_printf("Timeout... could not connect with remote"
								"server\n");
	}

	Osal_printf("\nQuerying server for fxnBufferTest() function index\n");

	status = RcmClient_getSymbolIndex(rcm_client_handle, "fxnBufferTest",
							&fxn_buffer_test_idx);
	if (status < 0)
		Osal_printf("Error getting symbol index [0x%x]\n", status);
	else {
		Osal_printf("fxnBufferTest() symbol index [0x%x]\n",
						fxn_buffer_test_idx);
	}

	Osal_printf("\nQuerying server for fxnBufferCopy() function index\n");
	status = RcmClient_getSymbolIndex(rcm_client_handle,
			"fxnBufferCopy", &fxn_buffer_copy_idx);
	if (status < 0)
		Osal_printf("Error getting symbol index [0x%x]\n", status);
	else {
		Osal_printf("fxnBufferCopy() symbol index [0x%x]\n",
						fxn_buffer_copy_idx);
	}

	Osal_printf("\nQuerying server for fxnExit() function index\n");

	status = RcmClient_getSymbolIndex(rcm_client_handle, "fxnExit",
							&fxn_exit_idx);
	if (status < 0)
		Osal_printf("Error getting symbol index [0x%x]\n", status);
	else
		Osal_printf("fxnExit() symbol index [0x%x]\n", fxn_exit_idx);

exit:
	Osal_printf("ipcSetup: Leaving ipcSetup()\n");
	return status;
}

/*!
 *  @brief  Function to demonstarate that flush fails on buffers that
 *          are not mapped to Device
 *
 *  @param  size    Size of the buffer to flush
 *
 *  @sa
 */
int test_flushfailuretest(int size)
{
	void	*buf_ptr = 0;
	int	err;
	int	tc_passed = 0;

	buf_ptr = (void *)malloc(size);
	if (buf_ptr == 0) {
		Osal_printf("Malloc memory failed.\n");
		goto exit;
	}

	err = ProcMgr_flushMemory(buf_ptr, size, PROC_SYSM3);
	if (err < 0) {
		Osal_printf("Flush memory failed for buffer 0x%x\n",
						(uint32_t)buf_ptr);
		goto exit;
	} else {
		Osal_printf("Flush memory success for buffer 0x%x\n",
						(uint32_t)buf_ptr);
	}

	err = ProcMgr_invalidateMemory(buf_ptr, size, PROC_SYSM3);
	if (err < 0) {
		Osal_printf("Invalidate memory failed for buffer 0x%x\n",
						(uint32_t)buf_ptr);
		goto exit;
	} else {
		Osal_printf("Invalidate memory success for buffer 0x%x\n",
						(uint32_t)buf_ptr);
		tc_passed = 1;
	}

exit:
	if (buf_ptr)
		free(buf_ptr);

	return tc_passed;
}


/*!
 *  @brief  Function to stress test Mapping and Unmapping of DMM
 *          buffers.
 *
 *  @param  size             Size of the buffer to Map
 *  @param  num_of_buffers   Number of buffers to Map
 *  @param  unmap            Specify unmap as true if unmap needs
 *                           to be called in this test case. if unmap
 *                           is set to false, then it is used to validate
 *                           resource cleanup handling the unmapping part.
 *  @sa
 */
int test_mapbuffertest(int size, int num_of_buffers, bool unmap)
{
	int				status = 0;
	int				*addresses = 0;
	void				**buffers = 0;
	void				*buf_ptr = 0;
	SyslinkMemUtils_MpuAddrToMap	mpuAddrList[1];
	uint32_t			mappedAddr;
	ProcMgr_MapType			mapType;
	int				tc_passed = 0;
	int				i = 0;

	Osal_printf("Running map buffer test with %d buffers of size %d.\n",
							num_of_buffers, size);

	if (size <= 0 || num_of_buffers < 0) {
		Osal_printf("Error: Invalid argument.\n");
		goto exit;
	}

	addresses = malloc(sizeof(int) * num_of_buffers);
	if (!addresses) {
		Osal_printf("Error: malloc returned null.\n");
		goto exit;
	}
	memset(addresses, 0x0, sizeof(int) * num_of_buffers);

	buffers = malloc(sizeof(void *) * num_of_buffers);
	if (!buffers) {
		Osal_printf("Error: malloc returned null.\n");
		goto exit;
	}
	memset(buffers, 0x0, sizeof(void *) * num_of_buffers);

	tc_passed = 1;
	while (i < num_of_buffers && tc_passed != 0) {
		buf_ptr = (void *)malloc(size);
		if (buf_ptr == NULL) {
			Osal_printf("Error: malloc returned null.\n");
			tc_passed = 0;
			goto exit_wbufs;
		} else {
			Osal_printf("malloc returned 0x%x.\n",
						(uint32_t)buf_ptr);
			buffers[i] = buf_ptr;
		}

		mapType = ProcMgr_MapType_Virt;
		mpuAddrList[0].mpuAddr = (uint32_t)buf_ptr;
		mpuAddrList[0].size = size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mappedAddr,
						mapType, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("SysLinkMemUtils_map() failed for 0x%x "
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				 mpuAddrList[0].size);
			tc_passed = 0;
			goto exit_wbufs;
		} else {
			Osal_printf("MPU Address = 0x%x"
					"Mapped Address = 0x%x, size = 0x%x\n",
					mpuAddrList[0].mpuAddr,
					mappedAddr, mpuAddrList[0].size);
			Osal_printf("Mapped Base Address = 0x%x\n",
						mappedAddr & 0xfffff000);
		}

		Osal_printf("Flushing mpuAddr 0x%x of size 0x%x\n",
				mpuAddrList[0].mpuAddr, mpuAddrList[0].size);
		status = ProcMgr_flushMemory(buf_ptr, size, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_flushMemory() failed for 0x%x "
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				 mpuAddrList[0].size);
			tc_passed = 0;
			goto exit_wbufs;
		} else {
			addresses[i] = mappedAddr;
			i++;
		}
	}

	if (unmap == false)
		return tc_passed;	/* memory is not freed */

	/* unmap the buffers */
	for (i = 0; i < num_of_buffers && buffers[i] != 0; i++) {
		Osal_printf("Unmapping 0x%x for buffer 0x%x\n", addresses[i],
				buffers[i]);
		status = SysLinkMemUtils_unmap(addresses[i], PROC_SYSM3);
		if (status < 0) {
			Osal_printf("SysLinkMemUtils_unmap() failed"
						"for 0x%x ", addresses[i]);
			tc_passed = 0;
			goto exit_wbufs;
		}
	}

exit_wbufs:
	for (i = 0; i < num_of_buffers && buffers[i] != 0; i++)
		free(buffers[i]);
	Osal_printf("Freed %d buffers.\n", i);

exit:
	if (buffers)
		free(buffers);
	if (addresses)
		free(addresses);

	return tc_passed;
}


/*!
 *  @brief  Function to validate use buffer functionality used with Tiler
 *
 *  @param  size             Size of the buffer to Map
 *  @param  iterations       Number of iterations to run the test
 *  @sa
 */
int test_usebuffer(int size, int iterations)
{
	uint32_t			*buf_ptr = NULL;
	int				map_size;
	SyslinkMemUtils_MpuAddrToMap	mpuAddrList[1];
	uint32_t			mappedAddr = 0;
	int				status;
	unsigned int			i;
	RcmClient_Message		*rcmMsg = NULL;
	int				rcmMsgSize;
	RCM_Remote_FxnArgs		*fxnArgs = NULL;
	int				count = 0;
	void				*tilVaPtr = NULL;
	MemAllocBlock			block;
	int				tiler_buf_length;
	uint32_t			*dataPtr = NULL;
	uint32_t			dummyMappedAddr = 0;
	int				tc_passed = 1;

	Osal_printf("Running Tiler use buffer test case\n");

	/* in case of error, cleanup will work */
	return_msg = NULL;
	tiler_buf_length = (size + DMMTEST_PAGE_SIZE - 1)
					& ~(DMMTEST_PAGE_SIZE - 1);
	map_size = tiler_buf_length + DMMTEST_PAGE_SIZE - 1;

	while (iterations-- > 0 && tc_passed != 0) {
		buf_ptr = (void *)malloc(map_size);
		dataPtr = (void *)(((uint32_t)buf_ptr + DMMTEST_PAGE_SIZE - 1)
						& ~(DMMTEST_PAGE_SIZE - 1));

		Osal_printf("Calling malloc of size 0x%x. Iteration %d\n",
							map_size, iterations);
		if (buf_ptr == NULL) {
			Osal_printf("Error: malloc returned null.\n");
			tc_passed = 0;
			break;
		} else {
			Osal_printf("malloc returned 0x%x. Passed to Tiler"
				"0x%x\n", (uint32_t)buf_ptr, (uint32_t)dataPtr);
		}

		 /* allocate aligned buffer */
		memset(&block, 0, sizeof(block));
		block.pixelFormat = PIXEL_FMT_PAGE;
		block.dim.len = tiler_buf_length;
		block.stride = 0;
		block.ptr = dataPtr;
		tilVaPtr = MemMgr_Map(&block, 1);
		Osal_printf("Tiler VA Address = 0x%x, tiler_buf_length"
			"= 0x%x\n", (uint32_t)tilVaPtr, tiler_buf_length);
		if (tilVaPtr == NULL) {
			tc_passed = 0;
			goto exit_alloc;
		}

		mpuAddrList[0].mpuAddr = (uint32_t)tilVaPtr;
		mpuAddrList[0].size = tiler_buf_length;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mappedAddr,
					ProcMgr_MapType_Tiler, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("Error mapping buffer\n");
			tc_passed = 0;
			goto exit_alloc;
		}

		Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				mappedAddr, mpuAddrList[0].size);

		/*
		* Doing a dummy mapping to allow iommu flush this buffer
		*/
		mpuAddrList[0].mpuAddr = (uint32_t)buf_ptr;
		mpuAddrList[0].size = map_size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &dummyMappedAddr,
					ProcMgr_MapType_Virt, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("Error mapping (#2) buffer\n");
			tc_passed = 0;
			goto exit_memmap;
		}

		/* Do actual test here */
		for (i = 0; i < tiler_buf_length/sizeof(uint32_t); i++) {
			dataPtr[i] = 0;
			dataPtr[i] = (0xbeef0000 | i);

			if (dataPtr[i] != (0xbeef0000 | i)) {
				Osal_printf("Readback failed at address 0x%x\n",
								&buf_ptr[i]);
				Osal_printf("\tExpected: [0x%x]\tActual:"
				"[0x%x]\n", (0xbeef0000 | i), buf_ptr[i]);
				tc_passed = 0;
			}
		}
		ProcMgr_flushMemory(dataPtr, tiler_buf_length, PROC_SYSM3);

		/* allocate a remote command message */
		Osal_printf("Allocating RCM message\n");
		rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
		status = RcmClient_alloc(rcm_client_handle, rcmMsgSize, &rcmMsg);
		if (status < 0) {
			Osal_printf("Error allocating RCM message\n");
			tc_passed = 0;
			goto exit_rcm_fail;
		}
		return_msg = rcmMsg;

		/* fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_test_idx;
		fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
		fxnArgs->num_bytes = tiler_buf_length;
		fxnArgs->buf_ptr   = (Ptr)mappedAddr;

		status = RcmClient_exec(rcm_client_handle, rcmMsg, &return_msg);
		if (status < 0) {
			Osal_printf(" RcmClient_execerror.\n");
			tc_passed = 0;
			goto exit_rcm_fail;
		} else {
			/* Check the buffer data */
			Osal_printf("Testing data...\n");
			count = 0;
			ProcMgr_invalidateMemory(dataPtr, tiler_buf_length,
								PROC_SYSM3);

			for (i = 0; i < tiler_buf_length/sizeof(uint32_t) &&
						count < MAX_ERRORS; i++) {
				if (dataPtr[i] != ~(0xbeef0000 | i)) {
					Osal_printf("ERROR: Data mismatch at"
						"offset 0x%x\n",
						i * sizeof(uint32_t));
					Osal_printf("\tExpected:[0x%x]\tActual:"
						"[0x%x]\n", ~(0xbeef0000 | i),
						dataPtr[i]);
					count++;
				}
			}
			if (count > 0)
				tc_passed  = 0;
			else
				Osal_printf("Test data is ok.\n");
		}

		/* Set the memory to some other value to avoid a
		 * potential future false positive
		 */
		for (i = 0; i < tiler_buf_length/sizeof(uint32_t); i++)
			buf_ptr[i] = 0xdeadbeef;

exit_rcm_fail:
		if (return_msg) {
			RcmClient_free(rcm_client_handle, return_msg);
			return_msg = NULL;
		}

		SysLinkMemUtils_unmap(dummyMappedAddr, PROC_SYSM3);
exit_memmap:
		SysLinkMemUtils_unmap(mappedAddr, PROC_SYSM3);
exit_alloc:
		if (tilVaPtr) {
			MemMgr_UnMap(tilVaPtr);
			tilVaPtr = NULL;
		}
		if (buf_ptr) {
			free(buf_ptr);
			buf_ptr = NULL;
		}
	}


	/* return message to the heap */
	Osal_printf("Calling RcmClient_free\n");

	return tc_passed;
}

/*!
 *  @brief Function to validate mapping io buffer functionality used with Tiler
 *
 *  @param  size             Size of the buffer to Map
 *  @param  iterations       Number of iterations to run the test
 *  @sa
 */
int test_iobuffertest(int size, int iterations)
{
	SyslinkMemUtils_MpuAddrToMap	mpuAddrList[1];
	uint32_t			mappedAddr;
	int				status;
	unsigned int			i;
	RcmClient_Message		*rcmMsg = NULL;
	int				rcmMsgSize;
	RCM_Remote_FxnArgs		*fxnArgs;
	int				count = 0;
	uint32_t			ssptr;
	uint32_t			*map_base;
	int				fd;
	int				tc_passed = 1;
	int 				tiler_width;
	int				tiler_height;

	Osal_printf("Running IO Map use buffer test case\n");
	Osal_printf("WARNING WARNING WARNING !\n");
	Osal_printf("This Test case is currently using Tiler as IO Address\n"
			"for dynamic mapping and unmapping. Restart the\n "
			"Syslink daemon if you want to run any Tiler related"
			"Tests after this test!\n");

	/* calculate an image size to approximately represent the buffer size */
	tiler_width = 16384;
	tiler_height = size/tiler_width + 1;
	if (tiler_height == 1) {
		tiler_width = size;
	}

	/* in case of error, cleanup will work */
	return_msg = NULL;


	while (iterations-- > 0 && tc_passed != 0) {
		TilerMgr_Open();
		Osal_printf("Calling tilerAlloc.\n");
		ssptr = TilerMgr_Alloc(PIXEL_FMT_8BIT, tiler_width, tiler_height);
		if (ssptr == 0) {
			Osal_printf("Error: tilerAlloc returned null.\n");
			status = -1;
			tc_passed = 0;
			goto exit;
		}

		fd = open("/dev/mem", O_RDWR|O_SYNC);
		if (fd >= 0) {
			map_base = mmap(0, size, PROT_READ | PROT_WRITE,
						MAP_SHARED, fd, ssptr);
			if (map_base == (void *) -1) {
				Osal_printf("Failed to do memory mapping\n");
				close(fd);
				tc_passed = 0;
				goto exit_mmap_fail;
			}
		} else {
			Osal_printf("FAILED opening /dev/mem...continue\n");
			fd = -1;
			tc_passed = 0;
			goto exit_mmap_fail;
		}

		for (i = 0; i < size/sizeof(uint32_t); i++) {
			map_base[i] = 0;
			map_base[i] = (0xbeef0000 | i);

			if (map_base[i] != (0xbeef0000 | i)) {
				Osal_printf("Readback failed at address 0x%x\n",
								&map_base[i]);
				Osal_printf("\tExpected: [0x%x]\tActual:"
				"[0x%x]\n", (0xbeef0000 | i), map_base[i]);
			}
		}

		mpuAddrList[0].mpuAddr = (uint32_t)map_base;
		mpuAddrList[0].size = size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mappedAddr,
					ProcMgr_MapType_Virt, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("Error allocating RCM message\n");
			tc_passed = 0;
			goto exit_memmap_fail;
		}

		Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
					"size = 0x%x\n", mpuAddrList[0].mpuAddr,
					mappedAddr, mpuAddrList[0].size);

		/* allocate a remote command message */
		rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
		status = RcmClient_alloc(rcm_client_handle, rcmMsgSize, &rcmMsg);
		if (status < 0) {
			Osal_printf("Error allocating RCM message\n");
			tc_passed  = 0;
			goto exit_rcm_fail;
		}
		return_msg = rcmMsg;

		/* fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_test_idx;
		fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
		fxnArgs->num_bytes = size;
		fxnArgs->buf_ptr   = (void *)mappedAddr;

		status = RcmClient_exec(rcm_client_handle, rcmMsg, &return_msg);
		if (status < 0) {
			Osal_printf(" RcmClient_execerror.\n");
			tc_passed = 0;
			goto exit_rcm_fail;
		} else {
			/* Check the buffer data */
			Osal_printf("Testing data\n");
			count = 0;
			for (i = 0; i < size/sizeof(uint32_t) &&
						count < MAX_ERRORS; i++) {
				if (map_base[i] != ~(0xbeef0000 | i)) {
					Osal_printf("ERROR: Data mismatch"
						"at offset 0x%x\n",
						i * sizeof(uint32_t));
					Osal_printf("\tExpected:[0x%x]\tActual:"
						"[0x%x]\n", ~(0xbeef0000 | i),
						map_base[i]);
					count++;
				}
			}
			if (count > 0)
				tc_passed = 0;
		}

		/* Set the memory to some other value to avoid a
		 * potential future false positive
		 */
		for (i = 0; i < size/sizeof(uint32_t); i++)
			map_base[i] = 0xdeadbeef;

exit_rcm_fail:
		if (return_msg) {
			RcmClient_free(rcm_client_handle, return_msg);
			return_msg = NULL;
		}

		SysLinkMemUtils_unmap(mappedAddr, PROC_SYSM3);
exit_memmap_fail:
		munmap(map_base, size);
exit_mmap_fail:
		if (fd >= 0)
			close(fd);
		TilerMgr_Free(ssptr);
		TilerMgr_Close();
	}

exit:
	return tc_passed;
}

/*!
 *  @brief  Function to validate DMM  buffer functionality.
 *
 *  @param  size             Size of the buffer to Map
 *  @param  iterations       Number of iterations to run the test
 *  @param  align            Alignment of user buffer(power of 2)
 *  @sa
 */
int test_dmmbuffer(int size, int iterations, int align)
{
	uint32_t *buf_ptr;
	int map_size = size;
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap    mpuAddrList[1];
	uint32_t mappedAddr;
	int status;
	unsigned int i;
	RcmClient_Message *rcmMsg = NULL;
	int rcmMsgSize;
	RCM_Remote_FxnArgs *fxnArgs;
	int count = 0;
	int tc_passed = 1;

	/* allocate a remote command message */
	Osal_printf("Allocating RCM message\n");
	rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
	status = RcmClient_alloc(rcm_client_handle, rcmMsgSize, &rcmMsg);
	if (status < 0) {
		Osal_printf("Error allocating RCM message\n");
		tc_passed = 0;
		goto exit;
	}

	while (iterations-- > 0 && tc_passed != 0) {
		Osal_printf("Calling memory allocator. Iteration %d\n",
							iterations);
		status = 0;
		if (align) {
#ifdef HAVE_POSIX_MEMALIGN
			status = posix_memalign((void **)&buf_ptr, align,
								map_size);
#else
			Osal_printf("memalign not supported, using malloc\n");
			buf_ptr = (void *)malloc(map_size);
#endif
		} else
			buf_ptr = (void *)malloc(map_size);

		if (buf_ptr == NULL || status) {
			Osal_printf("Error: memory allocator returned null.\n");
			tc_passed = 0;
			goto exit_nomem;
		} else {
			Osal_printf("memory allocator returned 0x%x.\n",
						(uint32_t)buf_ptr);
		}

		mapType = ProcMgr_MapType_Virt;
		mpuAddrList[0].mpuAddr = (uint32_t)buf_ptr;
		mpuAddrList[0].size = map_size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mappedAddr,
							mapType, PROC_SYSM3);
		Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
					"size = 0x%x\n", mpuAddrList[0].mpuAddr,
					mappedAddr, mpuAddrList[0].size);

		/* Do actual test here */
		for (i = 0; i < map_size/sizeof(uint32_t); i++) {
			buf_ptr[i] = 0;
			buf_ptr[i] = (0xbeef0000 | i);

			if (buf_ptr[i] != (0xbeef0000 | i)) {
				Osal_printf("Readback failed at address 0x%x\n",
								&buf_ptr[i]);
				Osal_printf("\tExpected: [0x%x]\tActual:"
				"[0x%x]\n", (0xbeef0000 | i), buf_ptr[i]);
				tc_passed = 0;
			}
		}
		ProcMgr_flushMemory(buf_ptr, map_size, PROC_SYSM3);

		/* fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_test_idx;
		fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
		fxnArgs->num_bytes = map_size;
		fxnArgs->buf_ptr   = (Ptr)mappedAddr;

		status = RcmClient_exec(rcm_client_handle, rcmMsg, &return_msg);
		if (status < 0) {
			Osal_printf(" RcmClient_execerror.\n");
			tc_passed = 0;
		} else {
			/* Check the buffer data */
			Osal_printf("Testing data\n");
			count = 0;
			ProcMgr_invalidateMemory(buf_ptr, map_size, PROC_SYSM3);

			for (i = 0; i < map_size/sizeof(uint32_t) &&
						count < MAX_ERRORS; i++) {
				if (buf_ptr[i] != ~(0xbeef0000 | i)) {
					Osal_printf("ERROR: Data mismatch at"
						"offset 0x%x\n",
						i * sizeof(uint32_t));
					Osal_printf("\tExpected:[0x%x]\tActual:"
						"[0x%x]\n", ~(0xbeef0000 | i),
						buf_ptr[i]);
					count++;
				}
			}
			if (count > 0)
				tc_passed = 0;
		}

		/* Set the memory to some other value to avoid a
		 * potential future false positive
		 */
		for (i = 0; i < map_size/sizeof(uint32_t); i++)
			buf_ptr[i] = 0xdeadbeef;

		SysLinkMemUtils_unmap(mappedAddr, PROC_SYSM3);
		free(buf_ptr);
	}

exit_nomem:
	/* return message to the heap */
	Osal_printf("Calling RcmClient_free\n");
	RcmClient_free(rcm_client_handle, return_msg);
exit:
	return tc_passed;
}

/*!
 *  @brief  Function to generate MMU fault
 *
 *  @param  fault_addr  The address used to generate the fault
 *
 *  @sa
 */
int test_mmufault(unsigned int fault_addr)
{
	int status = 0;
	RcmClient_Message *rcmMsg = NULL;
	int rcmMsgSize;
	RCM_Remote_FxnArgs *fxnArgs;
	int tc_passed = 1;

	/* allocate a remote command message */
	Osal_printf("Allocating RCM message\n");
	rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
	status = RcmClient_alloc(rcm_client_handle, rcmMsgSize, &rcmMsg);
	if (status < 0) {
		Osal_printf("Error allocating RCM message\n");
		tc_passed = 0;
		goto exit;
	}

	/* fill in the remote command message */
	rcmMsg->fxnIdx = fxn_buffer_test_idx;
	fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
	fxnArgs->num_bytes = 0x1000;
	fxnArgs->buf_ptr   = (Ptr)fault_addr;

	Osal_printf("Attempting MMU fault at address 0x%x\n", fault_addr);

	status = RcmClient_exec(rcm_client_handle, rcmMsg, &return_msg);
	if (status == 0) {
		Osal_printf("Test failed: FAILED TO generate MMU fault\n");
		tc_passed = 0;
	}
exit:
	return tc_passed;
}



/*!
 *  @brief  Function to compare contents of two buffers.
 *
 *  @param  src_ptr          Pointer to the source buffer.
 *  @param  dest_ptr         Pointer to the destination buffer.
 *  @param  size             Size of the buffers.
 *  @param  max_errors       Maximum number of errors to report.
 *  @sa
 *
 *  This function reports difference between the source and destination
 *  buffers.
 */
int buffer_compare(uint32_t *src_ptr, uint32_t *dest_ptr, unsigned int size,
			int max_errors)
{
	uint8_t *src_byte_ptr = (uint8_t *)src_ptr;
	uint8_t *dest_byte_ptr = (uint8_t *)dest_ptr;
	unsigned int i;
	int count = 0;

	/* Compare buffers in word size chunks. */
	for (i = 0; i < size/sizeof(uint32_t) && count < max_errors; i++) {
		if (dest_ptr[i] != src_ptr[i]) {
			Osal_printf("ERROR: Data mismatch at offset"
				"0x%x\n", i * sizeof(uint32_t));
			Osal_printf("\tExpected: [0x%x]\tActual:"
				"[0x%x]\n", src_ptr[i],
				dest_ptr[i]);
			count++;
		}
	}

	/* Continue comparison for any remaining bytes. */
	for (i *= sizeof(uint32_t); i < size && count < max_errors; i++) {
		if (dest_byte_ptr[i] != src_byte_ptr[i]) {
			Osal_printf("ERROR: Data mismatch at offset"
				"0x%x\n", i);
			Osal_printf("\tExpected: [0x%x]\tActual:"
				"[0x%x]\n", src_byte_ptr[i],
				dest_byte_ptr[i]);
			count++;
		}
	}

	if (count > 0)
		return 0;
	return 1;
}


/*!
 *  @brief  Function to validate buffer copy functionality.
 *
 *  @param  size             Size of the buffer to Map
 *  @param  iterations       Number of iterations to run the test
 *  @sa
 *
 *  This validates copying the content of one user buffer
 */
int test_buffercopy_test(int size, int iterations)
{
	uint32_t *src_ptr = NULL;
	uint32_t *dest_ptr = NULL;
	int map_size = size;
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap mpuAddrList[1];
	uint32_t mapped_dest = 0, mapped_src = 0;
	int status;
	unsigned int i;
	RcmClient_Message *rcmMsg = NULL;
	RcmClient_Message *rcm_ret_msg = NULL;
	int rcmMsgSize;
	struct rcm_remote_bufcopy_fxnargs *fxnArgs;
	int count = 0;
	int tc_passed = 1;

	src_ptr = (void *)malloc(map_size);
	dest_ptr = (void *)malloc(map_size);
	if (src_ptr == NULL || dest_ptr == NULL) {
		Osal_printf("Error: memory allocator returned null.\n");
		tc_passed = 0;
		goto exit;
	} else {
		Osal_printf("memory allocator returned 0x%x.\n",
				(uint32_t)src_ptr, (uint32_t)dest_ptr);
	}

	while (iterations-- > 0 && tc_passed) {
		Osal_printf("Calling memory allocator. Iteration %d\n",
							iterations);
		status = 0;
		mapped_src = 0;
		mapped_dest = 0;

		mapType = ProcMgr_MapType_Virt;
		mpuAddrList[0].mpuAddr = (uint32_t)src_ptr;
		mpuAddrList[0].size = map_size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_src,
							mapType, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("SysLinkMemUtils_map FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}
		Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
					"size = 0x%x\n", mpuAddrList[0].mpuAddr,
					mapped_src, mpuAddrList[0].size);

		mapType = ProcMgr_MapType_Virt;
		mpuAddrList[0].mpuAddr = (uint32_t)dest_ptr;
		mpuAddrList[0].size = map_size;
		status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_dest,
							mapType, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("SysLinkMemUtils_map FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}
		Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
					"size = 0x%x\n", mpuAddrList[0].mpuAddr,
					mapped_dest, mpuAddrList[0].size);

		/* Fill the data here */
		for (i = 0; i < map_size/sizeof(uint32_t); i++) {
			src_ptr[i] = 0;
			src_ptr[i] = (0xbeef0000 | i);
			dest_ptr[i] = 0xffffbabe;
		}
		status = ProcMgr_flushMemory(src_ptr, map_size, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_flushMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}
		/* Flush the destination buffer as well before sending to Ducati
		 * since the cache invalidate operation flushes the dirty cache
		 * line that is not cache aligned and this would lead to data
		 * corruption. Flushing of destination buffer is not required
		 * if this buffer is not touched after mapping it to remote
		 * processor
		 */
		status = ProcMgr_flushMemory(dest_ptr, map_size, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_flushMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}

		/* Allocate a remote command message */
		Osal_printf("Allocating RCM message\n");
		rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
		status = RcmClient_alloc(rcm_client_handle, rcmMsgSize,
								&rcmMsg);
		if (status < 0) {
			Osal_printf("Error allocating RCM message\n");
			tc_passed = 0;
			goto loop_exit;
		}

		/* Fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_copy_idx;
		fxnArgs = (struct rcm_remote_bufcopy_fxnargs *)(&rcmMsg->data);
		fxnArgs->num_bytes = map_size;
		fxnArgs->src_ptr   = (void *)mapped_src;
		fxnArgs->dest_ptr   = (void *)mapped_dest;

		status = RcmClient_exec(rcm_client_handle, rcmMsg,
							&rcm_ret_msg);
		if (status < 0) {
			Osal_printf(" RcmClient_execerror.%d\n", __LINE__);
			tc_passed = 0;
			rcm_ret_msg = rcmMsg;
			goto loop_exit;
		}
		/* Check the buffer data */
		Osal_printf("Testing data\n");
		count = 0;
		status = ProcMgr_invalidateMemory(dest_ptr, map_size,
							PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_invalidateMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}

		tc_passed = buffer_compare(src_ptr, dest_ptr, map_size,
						MAX_ERRORS);

loop_exit:
		if (mapped_src)
			SysLinkMemUtils_unmap(mapped_src, PROC_SYSM3);
		if (mapped_dest)
			SysLinkMemUtils_unmap(mapped_dest, PROC_SYSM3);

		/* Return message to the heap */
		Osal_printf("Calling RcmClient_free\n");
		if (rcm_client_handle && rcm_ret_msg)
			RcmClient_free(rcm_client_handle, rcm_ret_msg);
		rcm_ret_msg = NULL;
	}
exit:
	if (src_ptr)
		free(src_ptr);
	if (dest_ptr)
		free(dest_ptr);

	return tc_passed;
}


/*!
 *  @brief  Function to validate buffer copy functionality with
 *          multiple buffer sizes.
 *
 *  @param  start_size       Staring size of the buffer to Map
 *  @param  size_inc         Step size to increase buffer size
 *  @param  max_size         Maximum size of buffer
 *  @param  iterations       Number of transfers per buffer
 *  @sa
 *
 *  This validates copying the content of one user buffer
 */
int test_buffercopy_sizerange_test(int start_size, int size_inc,
					int max_size, int iterations)
{
	int size;
	int status = 1;

	Osal_printf("test_buffercopy_multitest: starting with size %d,"
		" size increment %d, max size %d\n",
		start_size, size_inc, max_size);

	if (start_size <= 0 || start_size > max_size || size_inc > max_size ||
		size_inc <= 0) {
		Osal_printf("test_buffercopy_multitest: Invalid argument.\n");
		return 0;
	}

	for (size = start_size; size <= max_size && status != 0;
			size += size_inc) {
		Osal_printf("Testing buffer size %d\n", size);
		status = test_buffercopy_test(size, iterations);
	}

	return status;
}


/*!
*  @brief  Function to validate file copy using DMM
*
*  @param  char *           Pointer to source file
*  @param  char *           Pointer to destination file
*  @param  map_size         user specified map size
*  @sa
*/
int test_dmm_filecopy(char *infile, char *outfile, unsigned int map_size)
{
	uint32_t *src_ptr = NULL;
	uint32_t *dest_ptr = NULL;
	ProcMgr_MapType mapType;
	SyslinkMemUtils_MpuAddrToMap mpuAddrList[1];
	uint32_t mapped_dest = 0, mapped_src = 0;
	int status;
	unsigned int i;
	RcmClient_Message *rcmMsg = NULL;
	RcmClient_Message *rcm_ret_msg = NULL;
	int rcmMsgSize;
	struct rcm_remote_bufcopy_fxnargs *fxnArgs;
	int count = 0;
	int read_bytes;
	int in_file = -1, out_file = -1;
	int tc_passed = 0;

	Osal_printf("input file %s\n", infile);
	Osal_printf("output file %s\n", outfile);
	in_file = open(infile, O_RDONLY);
	if (in_file == -1)
		goto exit;
	out_file = open(outfile, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if (out_file == -1)
		goto exit;

	/* User didn't provide map_size to use for file copy, use
	DMMTEST_PAGE_SIZE */
	if (map_size == 0)
		map_size = DMMTEST_PAGE_SIZE;

	src_ptr = malloc(map_size);
	dest_ptr = malloc(map_size);
	if (src_ptr == NULL || dest_ptr == NULL) {
		Osal_printf("Error: memory allocator returned null.\n");
		goto exit;
	} else {
		Osal_printf("memory allocator returned 0x%x.0x%x\n",
				(uint32_t)src_ptr, (uint32_t)dest_ptr);
	}
	/* Initialize buffers */
	for (i = 0; i < map_size/sizeof(uint32_t); i++) {
		src_ptr[i] = 0xffffbabe;
		dest_ptr[i] = 0xbabeffff;
	}

	/* Map source buffer */
	mapType = ProcMgr_MapType_Virt;
	mpuAddrList[0].mpuAddr = (uint32_t)src_ptr;
	mpuAddrList[0].size = map_size;
	status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_src,
						mapType, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("SysLinkMemUtils_map FAILED !! %d\n", __LINE__);
		goto exit;
	}
	Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				mapped_src, mpuAddrList[0].size);
	/* Map destination buffer */
	mapType = ProcMgr_MapType_Virt;
	mpuAddrList[0].mpuAddr = (uint32_t)dest_ptr;
	mpuAddrList[0].size = map_size;
	status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_dest,
						mapType, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("SysLinkMemUtils_map FAILED !! %d\n", __LINE__);
		goto exit;
	}
	Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				mapped_dest, mpuAddrList[0].size);
	/* Flush out the output buffer to keep the buffers cache lines
	 * clean. This step is especially required if the buffers are
	 * not cache aligned since the if the cache lines are not clean
	 * there is a chance for the destination buffer getting
	 * corrupted  when invalidate is called.
	 */
	status = ProcMgr_flushMemory(dest_ptr, map_size, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("ProcMgr_flushMemory FAILED !! %d\n", __LINE__);
		goto exit;
	}

	tc_passed = 1;
	do {
		read_bytes = read(in_file, (void *)src_ptr, map_size);
		Osal_printf("read_bytes = %d\n", read_bytes);
		if (read_bytes <= 0)
			break;

		status = ProcMgr_flushMemory(src_ptr, read_bytes, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_flushMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			break;
		}

		/* Allocate a remote command message */
		Osal_printf("Allocating RCM message\n");
		rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
		status = RcmClient_alloc(rcm_client_handle, rcmMsgSize,
								&rcmMsg);
		if (status < 0) {
			Osal_printf("Error allocating RCM message\n");
			tc_passed = 0;
			break;
		}
		/* Fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_copy_idx;
		fxnArgs = (struct rcm_remote_bufcopy_fxnargs *)(&rcmMsg->data);
		fxnArgs->num_bytes = read_bytes;
		fxnArgs->src_ptr   = (Ptr)mapped_src;
		fxnArgs->dest_ptr   = (Ptr)mapped_dest;

		status = RcmClient_exec(rcm_client_handle, rcmMsg,
							&rcm_ret_msg);
		if (status < 0) {
			Osal_printf(" RcmClient_execerror.%d\n", __LINE__);
			tc_passed = 0;
			rcm_ret_msg = rcmMsg;
			goto loop_exit;
		}
		count = 0;
		status = ProcMgr_invalidateMemory(dest_ptr, read_bytes,
								PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_invalidateMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}

		tc_passed = buffer_compare(src_ptr, dest_ptr, read_bytes,
								MAX_ERRORS);
		write(out_file, (void *)dest_ptr, read_bytes);

loop_exit:
		/* Return message to the heap */
		Osal_printf("Calling RcmClient_free\n");
		if (rcm_client_handle && rcm_ret_msg)
			RcmClient_free(rcm_client_handle, rcm_ret_msg);
		rcm_ret_msg = NULL;

	} while (tc_passed);
exit:
	if (mapped_src)
		SysLinkMemUtils_unmap(mapped_src, PROC_SYSM3);
	if (mapped_dest)
		SysLinkMemUtils_unmap(mapped_dest, PROC_SYSM3);
	if (src_ptr)
		free(src_ptr);
	if (dest_ptr)
		free(dest_ptr);
	/* Close opened files */
	if (in_file >= 0)
		close(in_file);
	if (out_file >= 0)
		close(out_file);

	return tc_passed;
}

/*!
*  @brief  Function to validate file copy using Tiler
*
*  @param  char *           Pointer to source file
*  @param  char *           Pointer to destination file
*  @param  map_size         user specified map size
*  @sa
*/
int test_dmm_filecopy_tiler(char *infile, char *outfile, unsigned int map_size)
{
	uint32_t *src_ptr = NULL;
	uint32_t *dest_ptr = NULL;
	SyslinkMemUtils_MpuAddrToMap    mpuAddrList[1];
	uint32_t mapped_dest = 0, mapped_src = 0, dummy_mapped = 0;
	int status;
	unsigned int i;
	RcmClient_Message *rcmMsg = NULL;
	RcmClient_Message *rcm_ret_msg = NULL;
	int rcmMsgSize;
	struct rcm_remote_bufcopy_fxnargs *fxnArgs;
	int count = 0;
	int read_bytes;
	int in_file = -1, out_file = -1;
	MemAllocBlock	block;
	void	*til_va_ptr = NULL;
	int tc_passed = 0;

	Osal_printf("input file %s\n", infile);
	Osal_printf("output file %s\n", outfile);
	in_file = open(infile, O_RDONLY);
	if (in_file == -1)
		goto exit;
	out_file = open(outfile, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if (out_file == -1)
		goto exit;

	/* User didn't provide map_size to use for file copy, use
	DMMTEST_PAGE_SIZE */
	if (map_size == 0)
		map_size = DMMTEST_PAGE_SIZE;

	src_ptr = malloc(map_size);
	dest_ptr = malloc(map_size);
	if (src_ptr == NULL || dest_ptr == NULL) {
		Osal_printf("Error: memory allocator returned null.\n");
		goto exit;
	} else {
		Osal_printf("memory allocator returned 0x%x.0x%x\n",
				(uint32_t)src_ptr, (uint32_t)dest_ptr);
	}
	/* Initialize buffers */
	for (i = 0; i < map_size/sizeof(uint32_t); i++) {
		src_ptr[i] = 0xffffbabe;
		dest_ptr[i] = 0xbabeffff;
	}
	/* Allocate a remote command message */
	Osal_printf("Allocating RCM message\n");
	rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
	status = RcmClient_alloc(rcm_client_handle, rcmMsgSize, &rcmMsg);
	if (status < 0) {
		Osal_printf("Error allocating RCM message\n");
		goto exit;
	}

	/* Allocate aligned buffer */
	memset(&block, 0, sizeof(block));
	block.pixelFormat = PIXEL_FMT_PAGE;
	block.dim.len = (map_size + DMMTEST_PAGE_SIZE)
						& ~(DMMTEST_PAGE_SIZE - 1);
	block.stride = 0;
	block.ptr = (void *)((uint32_t)src_ptr & ~(DMMTEST_PAGE_SIZE - 1));
	til_va_ptr = MemMgr_Map(&block, 1);
	Osal_printf("Tiler VA Address = 0x%x, tiler_buf_length"
		"= 0x%x\n", (uint32_t)til_va_ptr, map_size);
	if (til_va_ptr == NULL)
		goto exit;

	mpuAddrList[0].mpuAddr = (uint32_t)((uint32_t)til_va_ptr |
				((uint32_t)src_ptr & (DMMTEST_PAGE_SIZE - 1)));
	mpuAddrList[0].size = map_size;
	status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_src,
				ProcMgr_MapType_Tiler, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("SysLinkMemUtils_map FAILED !!\n");
		goto exit;
	}
	Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
			"size = 0x%x\n", mpuAddrList[0].mpuAddr,
			mapped_src, mpuAddrList[0].size);

	/* Dummy mapping for source buffer */
	mpuAddrList[0].mpuAddr = (uint32_t)src_ptr;
	mpuAddrList[0].size = map_size;
	status = SysLinkMemUtils_map(mpuAddrList, 1, &dummy_mapped,
					ProcMgr_MapType_Virt, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("SysLinkMemUtils_map FAILED !! %d\n", __LINE__);
		goto exit;
	}
	Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				dummy_mapped, mpuAddrList[0].size);

	/* Map destination buffer */
	mpuAddrList[0].mpuAddr = (uint32_t)dest_ptr;
	mpuAddrList[0].size = map_size;
	status = SysLinkMemUtils_map(mpuAddrList, 1, &mapped_dest,
					ProcMgr_MapType_Virt, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("SysLinkMemUtils_map FAILED !! %d\n", __LINE__);
		goto exit;
	}
	Osal_printf("MPU Address = 0x%x     Mapped Address = 0x%x,"
				"size = 0x%x\n", mpuAddrList[0].mpuAddr,
				mapped_dest, mpuAddrList[0].size);
	/* Flush out the output buffer to keep the buffers cache lines
	 * clean. This step is especially required if the buffers are
	 * not cache aligned since the if the cache lines are not clean
	 * there is a chance for the destination buffer getting
	 * corrupted  when invalidate is called.
	 */
	status = ProcMgr_flushMemory(dest_ptr, map_size, PROC_SYSM3);
	if (status < 0) {
		Osal_printf("ProcMgr_flushMemory FAILED !! %d\n", __LINE__);
		goto exit;
	}

	tc_passed = 1;
	do {
		read_bytes = read(in_file, (void *)src_ptr, map_size);
		Osal_printf("read_bytes = %d\n", read_bytes);
		if (read_bytes <= 0)
			break;

		status = ProcMgr_flushMemory(src_ptr, read_bytes, PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_flushMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto exit;
		}
		/* Fill in the remote command message */
		rcmMsg->fxnIdx = fxn_buffer_copy_idx;
		fxnArgs = (struct rcm_remote_bufcopy_fxnargs *)(&rcmMsg->data);
		fxnArgs->num_bytes = read_bytes;
		fxnArgs->src_ptr   = (Ptr)mapped_src;
		fxnArgs->dest_ptr   = (Ptr)mapped_dest;

		status = RcmClient_exec(rcm_client_handle, rcmMsg,
							&rcm_ret_msg);
		if (status < 0) {
			tc_passed = 0;
			Osal_printf(" RcmClient_exec error.\n");
			rcm_ret_msg = rcmMsg;
			goto loop_exit;
		}
		count = 0;
		status = ProcMgr_invalidateMemory(dest_ptr, read_bytes,
							PROC_SYSM3);
		if (status < 0) {
			Osal_printf("ProcMgr_invalidateMemory FAILED !! %d\n",
								__LINE__);
			tc_passed = 0;
			goto loop_exit;
		}

		tc_passed = buffer_compare(src_ptr, dest_ptr, read_bytes,
								MAX_ERRORS);

		write(out_file, (void *)dest_ptr, read_bytes);
loop_exit:
		/* Return message to the heap */
		Osal_printf("Calling RcmClient_free\n");
		if (rcm_client_handle && rcm_ret_msg)
			RcmClient_free(rcm_client_handle, rcm_ret_msg);
		rcm_ret_msg = NULL;
	} while (tc_passed);

exit:
	if (mapped_src)
		SysLinkMemUtils_unmap(mapped_src, PROC_SYSM3);
	if (mapped_dest)
		SysLinkMemUtils_unmap(mapped_dest, PROC_SYSM3);
	if (dummy_mapped)
		SysLinkMemUtils_unmap(dummy_mapped, PROC_SYSM3);
	if (til_va_ptr)
		MemMgr_UnMap(til_va_ptr);
	if (src_ptr)
		free(src_ptr);
	if (dest_ptr)
		free(dest_ptr);
	/* Close opened files */
	if (in_file >= 0)
		close(in_file);
	if (out_file >= 0)
		close(out_file);

	return tc_passed;
}


int ipc_shutdown(int proc_id)
{
	int status = 0;

	/* delete the rcm client */
	Osal_printf("Delete RCM client instance\n");
	status = RcmClient_delete(&rcm_client_handle);
	if (status < 0)
		Osal_printf("Error in RCM Client instance delete\n");

	/* rcm client module destroy*/
	Osal_printf("Destroy RCM client module\n");
	RcmClient_exit();

	if (proc_id == PROC_APPM3) {
		status =  ProcMgr_detach(proc_mgr_handle1);
		Osal_printf("DMMTEST CASE: ProcMgr_detach status [0x%x]\n",
								status);
		status = ProcMgr_close(&proc_mgr_handle1);
		if (status < 0) {
			Osal_printf("DMMTEST: Error in ProcMgr_close [0x%x]\n",
								status);
		} else {
			Osal_printf("DMMTEST: ProcMgr_close status: [0x%x]\n",
								status);
		}
		ProcMgr_detach(proc_mgr_handle);
		Osal_printf("DMMTEST: ProcMgr_detach status [0x%x]\n", status);

		status = ProcMgr_close(&proc_mgr_handle);
	}

	if (proc_id == PROC_SYSM3) {
		status =  ProcMgr_detach(proc_mgr_handle);
		Osal_printf("DMMTEST CASE: ProcMgr_detach status [0x%x]\n",
								status);
		status = ProcMgr_close(&proc_mgr_handle);
		if (status < 0) {
			Osal_printf("DMMTEST: Error in ProcMgr_close [0x%x]\n",
								status);
		} else {
		    Osal_printf("DMMTEST: ProcMgr_close status: [0x%x]\n",
								status);
		}
	}

	status = Ipc_destroy();

	return status;
}

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
	int status = 0;
	int testNo;
	int subTestNo;
	int proc_id;
	Bool validArgs = TRUE;
	int tc_passed = 0;

	Osal_printf("\n== Syslink Mem Utils Sample ==\n");

	if (argc < 3) {
		validArgs = FALSE;
		goto exit;
	}

	testNo = ATOI(argv[1]);
	if (testNo < 1 || testNo > DMM_TESTS_NUM) {
		validArgs = FALSE;
		goto exit;
	}

	/* Determine proc ID based on subtest number */
	subTestNo = ATOI(argv[2]);
	if (subTestNo == 1)
		proc_id = PROC_SYSM3;
	else if (subTestNo == 2)
		proc_id = PROC_APPM3;
	/* else if (subTestNo == 3)
		proc_id = PROC_TESLA; */
	else {
		validArgs = FALSE;
		goto exit;
	}

	status = ipcSetup(proc_id);
	if (status != RcmClient_S_SUCCESS) {
		Osal_printf("IPC SETUP FAILED\n");
		goto exit;
	}

	Osal_printf("Running Test case %d\n", testNo);
	switch (testNo) {
	case DMM_BUFFER:
	{
		int align;
		int not_power_2;

		if (argc < 6)
			align = 0;
		else {
			align = ATOI(argv[5]);
			not_power_2 = align & (align-1);
			if (not_power_2) {
				Osal_printf("ERROR: Alignnment argument"
					"should be power of 2\n");
				break;
			}
		}
		tc_passed = test_dmmbuffer(ATOI(argv[3]), ATOI(argv[4]), align);
		break;
	}
	case USE_BUFFER:
		tc_passed = test_usebuffer(ATOI(argv[3]), ATOI(argv[4]));
		break;
	case MAP_BUFFER:
		tc_passed = test_mapbuffertest(ATOI(argv[3]), ATOI(argv[4]), 1);
		break;
	case FLUSH_BUFFER_NEG:
		tc_passed = test_flushfailuretest(ATOI(argv[3]));
		break;
	case MAP_NO_UNMAP:
		tc_passed = test_mapbuffertest(ATOI(argv[3]), ATOI(argv[4]), 0);
		break;
	case MAP_IO_BUFFER:
		tc_passed = test_iobuffertest(ATOI(argv[3]), ATOI(argv[4]));
		break;
	case MMU_FAULT_1:
	{
		unsigned int fault_address = 0x9A000000;

		tc_passed = test_mmufault(fault_address);
		break;
	}
	case COPY_BUFFER:
		tc_passed = test_buffercopy_test(ATOI(argv[3]), ATOI(argv[4]));
		break;
	case DMM_FILE_COPY:
	{
		unsigned int map_size;
		if (argc < 6)
			map_size = 0;
		else
			map_size = ATOI(argv[5]);
		tc_passed = test_dmm_filecopy(argv[3], argv[4], map_size);
		break;
	}
	case DMM_FILE_TILER_COPY:
	{
		unsigned int map_size;
		if (argc < 6)
			map_size = 0;
		else
			map_size = ATOI(argv[5]);
		tc_passed = test_dmm_filecopy_tiler(argv[3], argv[4], map_size);
		break;
	}
	case COPY_BUFFER_RANGE:
	{
		tc_passed = test_buffercopy_sizerange_test(ATOI(argv[3]),
			ATOI(argv[4]), ATOI(argv[5]), ATOI(argv[6]));
		break;
	}
	default:
		Osal_printf("Invalid Test case number.\n");
	}

	Osal_printf("The test function returned %d\n.", tc_passed);

	status = ipc_shutdown(proc_id);
	if (status < 0)
		Osal_printf("ipc_shutown return error status.\n");

exit:
	if (!validArgs) {
		Osal_printf("Test 1 - DMM buffer\n"
				"\tUsage: ./dmm_daemontest.out 1 <Proc#> "
				"<Buffer Size> <NumIterations> <align>\n");
		Osal_printf("Test 2 - Tiler use buffer\n"
				"\tUsage: ./dmm_daemontest.out 2 <Proc#> "
				"<Buffer Size> <NumIterations>\n");
		Osal_printf("Test 3 - Map Test:\n"
				"\tUsage: ./dmm_daemontest.out 3 <Proc#> "
				 "<Buffer Size> <NumIterations>\n");
		Osal_printf("Test 4 - Flush memory Negetive Test:\n"
				"\tUsage: ./dmm_daemontest.out 4 <Proc#> "
				"<Buffer Size>\n");
		Osal_printf("Test 5 - Map with no Unmap Test:\n"
				"\tUsage: ./dmm_daemontest.out 5 <Proc#> "
				"<Buffer Size> <num_of_buffers>\n");
		Osal_printf("Test 6 - Map IO buffer Test:\n"
				"\tUsage: ./dmm_daemontest.out 6 <Proc#> "
				"<Buffer Size> <num_of_buffers>\n");
		Osal_printf("Test 7 - MMU fault Test case:\n"
				"\tUsage: ./dmm_daemontest.out 7 <Proc#>\n ");
		Osal_printf("Test 8 - Buffer copy test:\n"
				"\tUsage: ./dmm_daemontest.out 8 <Proc#> "
				"<Buffer Size> <NumIterations>\n");
		Osal_printf("Test 9 - File copy using DMM test:\n"
				"\tUsage: ./dmm_daemontest.out 9 <Proc#> "
				"<src file path> <dest file path> "
				"[copy_buf_size]\n");
		Osal_printf("Test 10 - File copy using Tiler test:\n"
				"\tUsage: ./dmm_daemontest.out 10 <Proc#> "
				"<src file path> <dest file path> "
				"<copy_buf_size_in_page_boundary>\n");
		Osal_printf("Test 11 - Buffer copy test over a range of buffer "
				"sizes:\n\tUsage: ./dmm_daemontest.out 11 "
				"<Proc#> <Min buf size> <Size increment> "
				"<Max buf size> <Iterations per size>\n");
	}

	if (tc_passed == 1 && status == 0) {
		Osal_printf("\nTest passed!\n");
		return 0;
	} else {
		Osal_printf("\nTest failed!\n");
		return 1;
	}
}
