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

/*==============================================================================
 *  @file   TilerSyslinkApp.c
 *
 *  @brief  The Syslink Test sample to validate Syslink Mem utils functinalit
 *
 *  ============================================================================
 */

 /* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>
#include <Memory.h>
#include <String.h>
#include <Trace.h>

/* Tiler header file */
#include <tilermgr.h>

/* IPC headers */
#include <IpcUsr.h>
#include <ProcMgr.h>
#include <SysLinkMemUtils.h>


/* RCM headers */
#include <RcmClient.h>

/* Sample headers */
#include <MemAllocTest_Config.h>
#include "TilerSyslinkApp.h"

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          SysM3
 */
#define TILERAPP_SYSM3ONLY_IMAGE  "./MemAllocServer_MPUSYS_Test_Core0.xem3"

/*!
 *  @brief  Name of the SysM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define TILERAPP_SYSM3_IMAGE      "./Notify_MPUSYS_reroute_Test_Core0.xem3"

/*!
 *  @brief  Name of the AppM3 baseImage to be used for sample execution with
 *          AppM3
 */
#define TILERAPP_APPM3_IMAGE      "./MemAllocServer_MPUAPP_Test_Core1.xem3"

/* Defines for the default HeapBufMP being configured in the System */
#define RCM_MSGQ_TILER_HEAPNAME         "Heap0"
#define RCM_MSGQ_TILER_HEAP_BLOCKS      256
#define RCM_MSGQ_TILER_HEAP_ALIGN       128
#define RCM_MSGQ_TILER_MSGSIZE          256
#define RCM_MSGQ_TILER_HEAPID           0
#define RCM_MSGQ_DOMX_HEAPNAME          "Heap1"
#define RCM_MSGQ_DOMX_HEAP_BLOCKS       256
#define RCM_MSGQ_DOMX_HEAP_ALIGN        128
#define RCM_MSGQ_DOMX_MSGSIZE           256
#define RCM_MSGQ_DOMX_HEAPID            1
#define RCM_MSGQ_HEAP_SR                1

#define DUCATI_DMM_POOL_0_ID            0
#define DUCATI_DMM_POOL_0_START         0x90000000
#define DUCATI_DMM_POOL_0_SIZE          0x10000000

#define ROUND_DOWN_TO2POW(x, N)         ((x) & ~((N)-1))
#define ROUND_UP_TO2POW(x, N)           ROUND_DOWN_TO2POW((x) + (N) - 1, N)

/* =============================================================================
 * Structs & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining RCM remote function arguments
 */
typedef struct {
    UInt numBytes;
     /*!< Size of the Buffer */
    Ptr bufPtr;
     /*!< Buffer that is passed */
} RCM_Remote_FxnArgs;

 /* ===========================================================================
 *  APIs
 * ============================================================================
 */
/*!
 *  @brief       Function to Test use buffer functionality using Tiler and
 *               Syslink IPC
 *
 *  @param procId       The Proc ID with which this functionality is verified
 *  @param useTiler     Flag to enable TILER allocation
 *  @param numTrials    Number of times to run use buffer test
 *
 *  @sa
 */
Int SyslinkUseBufferTest (Int procId, Bool useTiler, UInt numTrials)
{
    Int                             fd                  = -1;
    void *                          mapBase;
    SyslinkMemUtils_MpuAddrToMap    mpuAddrList[1];
    UInt32                          mapSize             = 4096;
    UInt32                          mappedAddr;
    Ipc_Config                      config;
    Int                             status              = 0;
    Int                             procIdSysM3         = PROC_SYSM3;
    Int                             procIdAppM3         = PROC_APPM3;
#if defined (SYSLINK_USE_LOADER)
    Char *                          imageNameSysM3      = NULL;
    Char *                          imageNameAppM3      = NULL;
    UInt32                          fileIdSysM3;
    UInt32                          fileIdAppM3;
#endif
    ProcMgr_StartParams             startParams;
    ProcMgr_StopParams              stopParams;
    ProcMgr_Handle                  procMgrHandleSysM3;
    ProcMgr_Handle                  procMgrHandleAppM3;
    ProcMgr_AttachParams            attachParams;
    ProcMgr_State                   state;
    RcmClient_Handle                rcmClientHandle     = NULL;
    RcmClient_Params                rcmClientParams;
    Char *                          remoteServerName;

    UInt                            fxnBufferTestIdx;
    UInt                            fxnExitIdx;

    RcmClient_Message             * rcmMsg              = NULL;
    RcmClient_Message             * returnMsg           = NULL;
    UInt                            rcmMsgSize;
    RCM_Remote_FxnArgs            * fxnArgs;
    Int                             count               = 0;
    Int                             maxCount            = 3;
    UInt32                          entryPoint          = 0;
    UInt                            i;
    Ptr                             bufPtr              = NULL;
    ProcMgr_MapType                 mapType;
    UInt                            k;
    UInt                          * uintBuf;
    HeapBufMP_Params                heapbufmpParams;
    UInt32                          srCount;
    SharedRegion_Entry              srEntry;
    HeapBufMP_Handle                heapHandle          = NULL;
    SizeT                           heapSize            = 0;
    Ptr                             heapBufPtr          = NULL;
    HeapBufMP_Handle                heapHandle1         = NULL;
    SizeT                           heapSize1           = 0;
    Ptr                             heapBufPtr1         = NULL;
    IHeap_Handle                    srHeap              = NULL;

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    printf("RCM procId= %d\n", procId);

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleSysM3,
                           procIdSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }
    else {
        Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
        ProcMgr_getAttachParams (NULL, &attachParams);
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (procMgrHandleSysM3, &attachParams);
        if (status < 0) {
            Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
            state = ProcMgr_getState (procMgrHandleSysM3);
            Osal_printf ("After attach: ProcMgr_getState\n"
                         "    state [0x%x]\n", status);
        }
    }

    if (status >= 0 && procId == procIdAppM3) {
        /* Open a handle to the ProcMgr instance. */
        status = ProcMgr_open (&procMgrHandleAppM3, procIdAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
        }
        else {
            Osal_printf ("ProcMgr_open Status [0x%x]\n", status);
            ProcMgr_getAttachParams (NULL, &attachParams);
            /* Default params will be used if NULL is passed. */
            status = ProcMgr_attach (procMgrHandleAppM3, &attachParams);
            if (status < 0) {
                Osal_printf ("ProcMgr_attach failed [0x%x]\n", status);
            }
            else {
                Osal_printf ("ProcMgr_attach status: [0x%x]\n", status);
                state = ProcMgr_getState (procMgrHandleAppM3);
                Osal_printf ("After attach: ProcMgr_getState\n"
                             "    state [0x%x]\n", status);
            }
        }
    }

    if (status >= 0) {
#if defined (SYSLINK_USE_LOADER)
        if (procId == procIdSysM3)
            imageNameSysM3 = TILERAPP_SYSM3ONLY_IMAGE;
        else if (procId == procIdAppM3)
            imageNameSysM3 = TILERAPP_SYSM3_IMAGE;

        Osal_printf ("ProcMgr_load: Loading the SysM3 image %s\n",
                     imageNameSysM3);
        status = ProcMgr_load (procMgrHandleSysM3, imageNameSysM3, 2,
                               &imageNameSysM3, &entryPoint, &fileIdSysM3,
                               procIdSysM3);
        Osal_printf ("ProcMgr_load SysM3 image Status [0x%x]\n", status);
#endif
        if (status >= 0) {
            startParams.proc_id = procIdSysM3;
            status = ProcMgr_start (procMgrHandleSysM3, entryPoint,
                                     &startParams);
            Osal_printf ("ProcMgr_start SysM3 Status [0x%x]\n", status);
        }
    }

    if (status >= 0 && procId == procIdAppM3) {
#if defined (SYSLINK_USE_LOADER)
        imageNameAppM3 = TILERAPP_APPM3_IMAGE;
        Osal_printf ("ProcMgr_load: Loading the AppM3 image %s\n",
                     imageNameAppM3);
        status = ProcMgr_load (procMgrHandleAppM3, imageNameAppM3, 2,
                                &imageNameAppM3, &entryPoint, &fileIdAppM3,
                                procIdAppM3);
        Osal_printf ("ProcMgr_load AppM3 image Status [0x%x]\n", status);
#endif
        if (status >= 0) {
            startParams.proc_id = procIdAppM3;
            status  = ProcMgr_start (procMgrHandleAppM3, entryPoint,
                                     &startParams);
            Osal_printf ("ProcMgr_start AppM3 Status [0x%x]\n", status);
        }
    }

    status = ProcMgr_createDMMPool (DUCATI_DMM_POOL_0_ID,
                                    DUCATI_DMM_POOL_0_START,
                                    DUCATI_DMM_POOL_0_SIZE,
                                    procIdSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_createDMMPool [0x%x]\n", status);
        goto exit_sysm3_start;
    }

    srCount = SharedRegion_getNumRegions();
    Osal_printf ("SharedRegion_getNumRegions = %d\n", srCount);
    for (i = 0; i < srCount; i++) {
        status = SharedRegion_getEntry (i, &srEntry);
        Osal_printf ("SharedRegion_entry #%d: base = 0x%x len = 0x%x "
                        "ownerProcId = %d isValid = %d cacheEnable = %d "
                        "cacheLineSize = 0x%x createHeap = %d name = %s\n",
                        i, srEntry.base, srEntry.len, srEntry.ownerProcId,
                        (Int)srEntry.isValid, (Int)srEntry.cacheEnable,
                        srEntry.cacheLineSize, (Int)srEntry.createHeap,
                        srEntry.name);
    }

    /* Create the heap to be used by RCM and register it with MessageQ */
    /* TODO: Do this dynamically by reading from the IPC config from the
     *       baseimage using Ipc_readConfig() */
    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = RCM_MSGQ_TILER_HEAP_ALIGN;
        heapbufmpParams.numBlocks  = RCM_MSGQ_TILER_HEAP_BLOCKS;
        heapbufmpParams.blockSize  = RCM_MSGQ_TILER_MSGSIZE;
        heapSize = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize = 0x%x\n", heapSize);

        srHeap = SharedRegion_getHeap (RCM_MSGQ_HEAP_SR);
        if (srHeap == NULL) {
            status = MEMORYOS_E_FAIL;
            Osal_printf ("SharedRegion_getHeap failed for srHeap:"
                         " [0x%x]\n", srHeap);
        }
        else {
            Osal_printf ("Before Memory_alloc = 0x%x\n", srHeap);
            heapBufPtr = Memory_alloc (srHeap, heapSize, 0);
            if (heapBufPtr == NULL) {
                status = MEMORYOS_E_MEMORY;
                Osal_printf ("Memory_alloc failed for ptr: [0x%x]\n",
                             heapBufPtr);
            }
            else {
                heapbufmpParams.name           = RCM_MSGQ_TILER_HEAPNAME;
                heapbufmpParams.sharedAddr     = heapBufPtr;
                Osal_printf ("Before HeapBufMP_Create: [0x%x]\n", heapBufPtr);
                heapHandle = HeapBufMP_create (&heapbufmpParams);
                if (heapHandle == NULL) {
                    status = HeapBufMP_E_FAIL;
                    Osal_printf ("HeapBufMP_create failed for Handle:"
                                 "[0x%x]\n", heapHandle);
                }
                else {
                    /* Register this heap with MessageQ */
                    status = MessageQ_registerHeap (heapHandle,
                                                    RCM_MSGQ_TILER_HEAPID);
                    if (status < 0) {
                        Osal_printf ("MessageQ_registerHeap failed!\n");
                    }
                }
            }
        }
    }

    if (status >= 0) {
        HeapBufMP_Params_init (&heapbufmpParams);
        heapbufmpParams.sharedAddr = NULL;
        heapbufmpParams.align      = RCM_MSGQ_DOMX_HEAP_ALIGN;
        heapbufmpParams.numBlocks  = RCM_MSGQ_DOMX_HEAP_BLOCKS;
        heapbufmpParams.blockSize  = RCM_MSGQ_DOMX_MSGSIZE;
        heapSize1 = HeapBufMP_sharedMemReq (&heapbufmpParams);
        Osal_printf ("heapSize1 = 0x%x\n", heapSize1);

        heapBufPtr1 = Memory_alloc (srHeap, heapSize1, 0);
        if (heapBufPtr1 == NULL) {
            status = MEMORYOS_E_MEMORY;
            Osal_printf ("Memory_alloc failed for ptr: [0x%x]\n",
                         heapBufPtr1);
        }
        else {
            heapbufmpParams.name           = RCM_MSGQ_DOMX_HEAPNAME;
            heapbufmpParams.sharedAddr     = heapBufPtr1;
            Osal_printf ("Before HeapBufMP_Create: [0x%x]\n", heapBufPtr1);
            heapHandle1 = HeapBufMP_create (&heapbufmpParams);
            if (heapHandle1 == NULL) {
                status = HeapBufMP_E_FAIL;
                Osal_printf ("HeapBufMP_create failed for Handle:"
                             "[0x%x]\n", heapHandle1);
            }
            else {
                /* Register this heap with MessageQ */
                status = MessageQ_registerHeap (heapHandle1,
                                                RCM_MSGQ_DOMX_HEAPID);
                if (status < 0) {
                    Osal_printf ("MessageQ_registerHeap failed!\n");
                }
            }
        }
    }
    if (status < 0) {
        goto exit;
    }

    ///////////////////////// Set up RCM /////////////////////////

    /* Rcm client module init*/
    Osal_printf ("RCM Client module init.\n");
    RcmClient_init ();

    /* Rcm client module params init*/
    Osal_printf ("RCM Client module params init.\n");
    status = RcmClient_Params_init (&rcmClientParams);
    if (status < 0) {
        Osal_printf ("Error in RCM Client instance params init \n");
        goto exit;
    } else {
        Osal_printf ("RCM Client instance params init passed \n");
    }

    if (procId == procIdSysM3) {
        remoteServerName = RCM_SERVER_NAME_SYSM3;
    }
    else {
        remoteServerName = RCM_SERVER_NAME_APPM3;
    }
    rcmClientParams.heapId = RCM_MSGQ_TILER_HEAPID;

    /* create an rcm client instance */
    Osal_printf ("Creating RcmClient instance %s.\n", remoteServerName);
    rcmClientParams.callbackNotification = 0; /* disable asynchronous exec */

    while ((rcmClientHandle == NULL) && (count++ < MAX_CREATE_ATTEMPTS)) {
        status = RcmClient_create (remoteServerName, &rcmClientParams,
                                    &rcmClientHandle);
        if (status < 0) {
            if (status == RcmClient_E_SERVERNOTFOUND) {
                Osal_printf ("Unable to open remote server %d time\n", count);
            }
            else {
                Osal_printf ("Error in RCM Client create \n");
                goto exit;
            }
        } else {
            Osal_printf ("RCM Client create passed \n");
        }
    }
    if (MAX_CREATE_ATTEMPTS <= count) {
        Osal_printf ("Timeout... could not connect with remote server\n");
    }


    Osal_printf ("\nQuerying server for fxnBufferTest() function index \n");

    status = RcmClient_getSymbolIndex (rcmClientHandle, "fxnBufferTest",
                                            &fxnBufferTestIdx);
    if (status < 0)
        Osal_printf ("Error getting symbol index [0x%x]\n", status);
    else
        Osal_printf ("fxnBufferTest() symbol index [0x%x]\n", fxnBufferTestIdx);

    Osal_printf ("\nQuerying server for fxnExit() function index \n");

    status = RcmClient_getSymbolIndex (rcmClientHandle, "fxnExit",
                                            &fxnExitIdx);
    if (status < 0)
        Osal_printf ("Error getting symbol index [0x%x]\n", status);
    else
        Osal_printf ("fxnExit() symbol index [0x%x]\n", fxnExitIdx);

    for(k = 0; k < numTrials; k++)
    {
        /////////////////////////// Allocate & map buffer //////////////////////
        if (useTiler) {
            TilerMgr_Open();
            Osal_printf ("Calling tilerAlloc.\n");
            bufPtr = (Ptr)TilerMgr_Alloc(PIXEL_FMT_8BIT, mapSize, 1);
            if (bufPtr == NULL) {
                Osal_printf ("Error: tilerAlloc returned null.\n");
                status = -1;
                return status;
            }
            else {
                Osal_printf ("tilerAlloc returned 0x%x.\n", (UInt)bufPtr);
            }

            Osal_printf ("Opening /dev/mem.\n");
            fd = open ("/dev/mem", O_RDWR|O_SYNC);
            if (fd) {
                mapBase = mmap(0, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                                fd, (UInt)bufPtr);
                if (mapBase == (void *) -1) {
                    Osal_printf ("Failed to do memory mapping \n");
                    close (fd);
                    return -1;
                }
            }
            else {
                Osal_printf ("Failed opening /dev/mem file\n");
                return -2;
            }
        }
        else {
                Osal_printf ("Calling malloc.\n");
                mapBase = (Ptr)malloc(mapSize);
                if (mapBase == NULL) {
                    Osal_printf ("Error: malloc returned null.\n");
                    return -1;
                }
                else {
                    Osal_printf ("malloc returned 0x%x.\n", (UInt)mapBase);
                }
        }

        mapType = ProcMgr_MapType_Virt;
        printf("map_base = 0x%x \n", (UInt32)mapBase);
        mpuAddrList[0].mpuAddr = (UInt32)mapBase;
        mpuAddrList[0].size = mapSize;
        status = SysLinkMemUtils_map (mpuAddrList, 1, &mappedAddr,
                                mapType, PROC_SYSM3);
        Osal_printf ("MPU Address = 0x%x     Mapped Address = 0x%x,"
                     "size = 0x%x\n", mpuAddrList[0].mpuAddr,
                     mappedAddr, mpuAddrList[0].size);

        //////////////// Do actual test here ////////////////

        uintBuf = (UInt*)mapBase;
        for(i = 0; i < mapSize / sizeof(UInt); i++) {
            uintBuf[i] = 0;
            uintBuf[i] = (0xbeef0000 | i);

            if (uintBuf[i] != (0xbeef0000 | i)) {
                Osal_printf ("Readback failed at address 0x%x\n", &uintBuf[i]);
                Osal_printf ("\tExpected: [0x%x]\tActual: [0x%x]\n",
                                (0xbeef0000 | i), uintBuf[i]);
            }
        }
        if (!useTiler) {
            ProcMgr_flushMemory (mapBase, mapSize, PROC_SYSM3);
        }

        // allocate a remote command message
        Osal_printf ("Allocating RCM message\n");
        rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
        status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
        if (status < 0) {
            Osal_printf ("Error allocating RCM message\n");
            goto exit;
        }

        // fill in the remote command message
        rcmMsg->fxnIdx = fxnBufferTestIdx;
        fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);
        fxnArgs->numBytes = mapSize;
        fxnArgs->bufPtr   = (Ptr)mappedAddr;

        status = RcmClient_exec (rcmClientHandle, rcmMsg, &returnMsg);
        if (status < 0) {
            Osal_printf (" RcmClient_exec error. \n");
        }
        else {
            // Check the buffer data
            Osal_printf ("Testing data\n");
            count = 0;
            if (!useTiler) {
                ProcMgr_invalidateMemory (mapBase, mapSize, PROC_SYSM3);
            }
            for(i = 0; i < mapSize / sizeof(UInt) && count < maxCount; i++) {
                if (uintBuf[i] != ~(0xbeef0000 | i)) {
                    Osal_printf ("ERROR: Data mismatch at offset 0x%x\n",
                                    i * sizeof(UInt));
                    Osal_printf ("\tExpected: [0x%x]\tActual: [0x%x]\n",
                                    ~(0xbeef0000 | i), uintBuf[i]);
                    count ++;
                }
            }

            if (count == 0)
                Osal_printf ("Test passed!\n");
        }

        // Set the memory to some other value to avoid a
        // potential future false positive
        for(i = 0; i < mapSize / sizeof(UInt); i++) {
            uintBuf[i] = 0xdeadbeef;
        }

        // return message to the heap
        Osal_printf ("Calling RcmClient_free\n");
        RcmClient_free (rcmClientHandle, returnMsg);

        ///////////////////// Cleanup //////////////////////

        SysLinkMemUtils_unmap (mappedAddr, PROC_SYSM3);

        if (useTiler) {
            Osal_printf ("Freeing TILER buffer\n");

            if (munmap(mapBase,mapSize) == -1)
                Osal_printf ("Memory Unmap failed.\n");
            else
                Osal_printf ("Memory Unmap successful.\n");
            close(fd);

            TilerMgr_Free((Int)bufPtr);

            TilerMgr_Close();
        }
        else {
            free(bufPtr);
        }
    }

    //////////////////////// Shutdown RCM /////////////////////

    // allocate a remote command message
    rcmMsgSize = sizeof(RCM_Remote_FxnArgs);
    status = RcmClient_alloc (rcmClientHandle, rcmMsgSize, &rcmMsg);
    if (status < 0) {
        Osal_printf ("Error allocating RCM message\n");
        goto exit;
    }

    // fill in the remote command message
    rcmMsg->fxnIdx = fxnExitIdx;
    fxnArgs = (RCM_Remote_FxnArgs *)(&rcmMsg->data);

    // execute the remote command message
    Osal_printf ("calling RcmClient_execDpc \n");
    status = RcmClient_execDpc (rcmClientHandle, rcmMsg, &returnMsg);
    if (status < 0) {
        Osal_printf ("RcmClient_execDpc error. \n");
        goto exit;
    }

    // return message to the heap
    Osal_printf ("calling RcmClient_free \n");
    RcmClient_free (rcmClientHandle, returnMsg);

    /* delete the rcm client */
    Osal_printf ("Delete RCM client instance \n");
    status = RcmClient_delete (&rcmClientHandle);
    if (status < 0) {
        Osal_printf ("Error in RCM Client instance delete\n");
    }

    /* rcm client module destroy*/
    Osal_printf ("Destroy RCM client module \n");
    RcmClient_exit ();

    /* Cleanup the default HeapBufMP registered with MessageQ */
    status = MessageQ_unregisterHeap (RCM_MSGQ_DOMX_HEAPID);
    if (status < 0) {
        Osal_printf ("Error in MessageQ_unregisterHeap [0x%x]\n", status);
    }

    if (heapHandle1) {
        status = HeapBufMP_delete (&heapHandle1);
        if (status < 0) {
            Osal_printf ("Error in HeapBufMP_delete [0x%x]\n", status);
        }
    }

    if (heapBufPtr1) {
        Memory_free (srHeap, heapBufPtr1, heapSize1);
    }

    status = MessageQ_unregisterHeap (RCM_MSGQ_TILER_HEAPID);
    if (status < 0) {
        Osal_printf ("Error in MessageQ_unregisterHeap [0x%x]\n", status);
    }

    if (heapHandle) {
        status = HeapBufMP_delete (&heapHandle);
        if (status < 0) {
            Osal_printf ("Error in HeapBufMP_delete [0x%x]\n", status);
        }
    }

    if (heapBufPtr) {
        Memory_free (srHeap, heapBufPtr, heapSize);
    }

    status = ProcMgr_deleteDMMPool (DUCATI_DMM_POOL_0_ID, procIdSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_deleteDMMPool:status = 0x%x\n", status);
    }

    if (procId == procIdAppM3) {
        stopParams.proc_id = procIdAppM3;
        status = ProcMgr_stop (procMgrHandleAppM3, &stopParams);
        Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);
    }

exit_sysm3_start:
    stopParams.proc_id = procIdSysM3;
    status = ProcMgr_stop (procMgrHandleSysM3, &stopParams);
    Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);

    if (procId == procIdAppM3) {
        status = ProcMgr_detach (procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_detach(AppM3): status = 0x%x\n",
                            status);
        }

        status = ProcMgr_close (&procMgrHandleAppM3);
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_close(AppM3): status = 0x%x\n",
                            status);
        }
    }

    status = ProcMgr_detach (procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_detach(SysM3): status = 0x%x\n", status);
    }

    status = ProcMgr_close (&procMgrHandleSysM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_close(SysM3): status = 0x%x\n", status);
    }

    status = Ipc_destroy();
    Osal_printf ("Ipc_destroy status: [0x%x]\n", status);

    Osal_printf ("SyslinkUseBufferTest done!\n");

exit:
    return status;
}


/*!
 *  @brief       Function to test the retrieval of Pages for
 *               both Tiler and non-Tiler buffers
 *
 *  @param     void
 *
 *  @sa
 */
Int SyslinkVirtToPhysPagesTest(void)
{
    Int                             status          = 0;
    UInt32                          remoteAddr;
    Int                             numOfIterations = 1;
    UInt32                          physEntries[10];
    UInt32                          numOfPages      = 10;
    UInt32                          temp;
    Ipc_Config                      config;
#if 0
    Int                           * p;
    UInt32                          mappedAddr;
    SyslinkMemUtils_MpuAddrToMap    mpuAddrList[1];
    UInt32                          sizeOfBuffer = 0x1000;
#endif

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    Osal_printf ("Testing SyslinkVirtToPhysTest\n");
    remoteAddr = 0x60000000;
    do {
        status = SysLinkMemUtils_virtToPhysPages (remoteAddr, numOfPages,
                                                    physEntries, PROC_SYSM3);
        if (status < 0) {
            Osal_printf ("SysLinkMemUtils_virtToPhysPages failure,status"
                        " = 0x%x\n",(UInt32)status);
            return status;
        }

        for (temp = 0; temp < numOfPages; temp++) {
            Osal_printf ("remoteAddr = [0x%x]  physAddr = [0x%x]\n", remoteAddr,
                                                            physEntries[temp]);
            remoteAddr += 4096;
        }
        numOfIterations--;
    } while (numOfIterations > 0);

#if 0
    p = (int *)malloc(sizeOfBuffer);
    mpuAddrList[0].mpuAddr = (UInt32)p;
    mpuAddrList[0].size = sizeOfBuffer + 0x1000;
    status = SysLinkMemUtils_map (mpuAddrList, 1, &mappedAddr,
                            ProcMgr_MapType_Virt, PROC_SYSM3);
    mappedAddr = (UInt32)p;
    numOfPages = 3;
    status = SysLinkMemUtils_virtToPhysPages (mappedAddr, numOfPages,
                                               physEntries, PROC_SYSM3);
    for (temp = 0; temp < numOfPages; temp++) {
        Osal_printf ("remoteAddr = [0x%x]  physAddr = [0x%x]\n",
                    (mappedAddr + (temp*4096)), physEntries[temp]);
    }
    SysLinkMemUtils_unmap(mappedAddr, PROC_SYSM3);
    free(p);
#endif

    Ipc_destroy ();

    return status;
}


/*!
 *  @brief       Function to test the retrieval phsical address for
 *               a given Co-Processor virtual address.
 *
 *  @param     void
 *
 *  @sa
 */
Int SyslinkVirtToPhysTest (Void)
{
    UInt32  remoteAddr;
    UInt32  physAddr;
    Int     numOfIterations = 10;

    Osal_printf ("Testing SyslinkVirtToPhysTest\n");
    remoteAddr = 0x60000000;
    do {
        SysLinkMemUtils_virtToPhys (remoteAddr, &physAddr, PROC_SYSM3);
        Osal_printf ("remoteAddr = [0x%x]  physAddr = [0x%x]\n", remoteAddr,
                                                            physAddr);
        remoteAddr += 4096;
        numOfIterations--;
    } while (numOfIterations > 0);

    return 0;
}


/*!
 *  @brief       Function to test multiple calls to map/unmap
 *
 *  @param   numTrials  Number of times to call map/unmap
 *
 *  @sa
 */
Int SyslinkMapUnMapTest (UInt numTrials)
{
    Int                             status              = 0;
    Ptr                             bufPtr;
    UInt                            bufSize;
    UInt                            i;
    SyslinkMemUtils_MpuAddrToMap    mpuAddrList[1];
    UInt32                          mappedAddr;
    Ipc_Config                      config;
    ProcMgr_Handle                  procMgrHandleClient;

    // Randomize
    srand(time(NULL));

    Ipc_getConfig (&config);
    status = Ipc_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in Ipc_setup [0x%x]\n", status);
    }

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandleClient, PROC_SYSM3);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }
    else {
        Osal_printf ("ProcMgr_open Status [0x%x]\n", status);

        for(i = 0; i < numTrials; i++) {
            // Generate random size to allocate (up to 64K)
            bufSize = (rand() & 0xFFFF);
            if (bufSize == 0)
                bufSize = 1;

            Osal_printf ("Calling malloc with size %d.\n", bufSize);
            bufPtr = (Ptr) malloc(bufSize);

            if (bufPtr == NULL) {
                Osal_printf ("Error: malloc returned null.\n");
                return -1;
            }
            else {
                Osal_printf ("malloc returned 0x%x.\n", (UInt)bufPtr);
            }

            mpuAddrList[0].mpuAddr = (UInt32)bufPtr;
            mpuAddrList[0].size = bufSize;
            status = SysLinkMemUtils_map (mpuAddrList, 1, &mappedAddr,
                                            ProcMgr_MapType_Virt, PROC_SYSM3);
            if (status < 0) {
                Osal_printf ("SysLinkMemUtils_map failed with status [0x%x].\n",
                                status);
                return -2;
            }
            Osal_printf ("MPU Address = 0x%x     Mapped Address = 0x%x\n",
                                    mpuAddrList[0].mpuAddr, mappedAddr);

            status = SysLinkMemUtils_unmap (mappedAddr, PROC_SYSM3);
            if (status < 0) {
                Osal_printf ("SysLinkMemUtils_unmap failed with status "
                             "[0x%x].\n", status);
                return -3;
            }

            free(bufPtr);
        }
    }

    status = ProcMgr_close (&procMgrHandleClient);
    Osal_printf ("ProcMgr_close status: [0x%x]\n", status);

    status = Ipc_destroy ();
    Osal_printf ("Ipc_destroy status: [0x%x]\n", status);

    Osal_printf ("Map/UnMap test passed!\n");
    return 0;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
