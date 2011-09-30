/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2008-2010, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*==============================================================================
 *  @file   SysLinkMemUtils.c
 *
 *  @brief     This modules provides syslink Mem utils functionality
 *  ============================================================================
 */


/* Linux specific header files */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <Trace.h>
#include <List.h>
#include <OsalSemaphore.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <ProcMgr.h>
#include <SysLinkMemUtils.h>
#include <memmgr.h>
#include <tilermem.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*! @brief Page size */
#define Page_SIZE_4K 4096
/*! @brief Page Mask */
#define Page_MASK(pg_size) (~((pg_size)-1))
/*! @brief Align to lower Page */
#define Page_ALIGN_LOW(addr, pg_size) ((addr) & Page_MASK(pg_size))
/*! @brief Start address of Tiler region */
#define TILER_ADDRESS_START         0x60000000
/*! @brief End address of Tiler region */
#define TILER_ADDRESS_END           0x80000000

/*!
 *  @brief  Alloc parameter structure
 */
typedef struct {
    UInt    pixelFormat;
    /*!< Pixel format */
    UInt    width;
    /*!< Width of the buffer */
    UInt    height;
    /*!< Height of the buffer */
    UInt    length;
    /*!< Length of the buffer */
    UInt    stride;
    /*!< stride */
    Ptr     ptr;
    /*!< Buffer pointer */
    UInt  * reserved;
    /*!< Reserved */
} AllocParams;

/*!
 *  @brief  Parameter for remote MemMgr_alloc
 */
typedef struct {
    UInt        numBuffers;
    /*!< Number of buffer */
    AllocParams params [1];
    /*!< Alloc param struct */
} AllocArgs;

/*!
 *  @brief  Parameter for remote MemMgr_free
 */
typedef struct {
    Ptr bufPtr;
    /*!< Buffer pointer */
} FreeArgs;

/*!
 *  @brief  Structure element used to store a mapped buffer
 */
typedef struct {
    List_Elem   elem;
    /*!< List element */
    Ptr         da;
    /*!< Device address */
    Ptr         ua;
    /*!< User address */
    UInt32      size;
    /*!< Size of the mapping */
} AddrNode;

/*!
 *  @brief  SysLinkMemUtils Module state object
 */
typedef struct SysLinkMemUtils_ModuleObject_tag {
    List_Handle          addrTable;
    /*!< Table where the device address and A9 address will be stored. */
    OsalSemaphore_Handle semList;
    /*!< Semaphore to protect the table */
} SysLinkMemUtils_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    SysLinkMemUtils_state
 *
 *  @brief  SysLinkMemUtils state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
SysLinkMemUtils_ModuleObject SysLinkMemUtils_state =
{
    .addrTable              = NULL,
    .semList                = NULL,
};

/*!
 *  @var    SysLinkMemUtils_module
 *
 *  @brief  Pointer to the SysLinkMemUtils module state
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
SysLinkMemUtils_ModuleObject * SysLinkMemUtils_module = &SysLinkMemUtils_state;


/* =============================================================================
 *  APIs
 * =============================================================================
 */

static Void _SysLinkMemUtils_init (Void) __attribute__((constructor));
/*!
 *  @brief      Setup SysLinkMemUtils module.
 *
 *  @sa         _SysLinkMemUtils_exit
 */
static Void
_SysLinkMemUtils_init (Void)
{
    List_Params             listParams;

    GT_0trace (curTrace, GT_ENTER, "_SysLinkMemUtils_init");

    List_Params_init (&listParams);
    SysLinkMemUtils_module->addrTable = List_create (&listParams);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!SysLinkMemUtils_module->addrTable) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             PROCMGR_E_MEMORY,
                             "Translation list could not be created!");
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    SysLinkMemUtils_module->semList = OsalSemaphore_create (
                                                OsalSemaphore_Type_Counting, 1);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!SysLinkMemUtils_module->semList) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             PROCMGR_E_MEMORY,
                             "List semaphore could not be created!");
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_init");
}


static Void _SysLinkMemUtils_exit (Void) __attribute__((destructor));
/*!
 *  @brief      Free resources allocated in setup part
 *
 *  @sa         _SysLinkMemUtils_init
 */
static Void
_SysLinkMemUtils_exit (Void)
{
    GT_0trace (curTrace, GT_ENTER, "_SysLinkMemUtils_exit");

    List_delete (&SysLinkMemUtils_module->addrTable);
    OsalSemaphore_delete (&SysLinkMemUtils_module->semList);

    GT_0trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_exit");
}


/*!
 *  @brief  Find a node inside the Translation Table which da matches
 *
 *  @params da      Device address
 *
 *  @sa     _SysLinkMemUtils_insertMapElement, _SysLinkMemUtils_removeMapElement
 */
static AddrNode *
_SysLinkMemUtils_findNode (Ptr da)
{
    AddrNode  * node;

    GT_1trace (curTrace, GT_ENTER, "_SysLinkMemUtils_findNode", da);

    for (node = List_next (SysLinkMemUtils_module->addrTable, NULL);
        node != NULL; node = List_next (SysLinkMemUtils_module->addrTable,
            &node->elem)) {
        if (da == node->da) {
            break;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_findNode", node);

    return node;
}


/*!
 *  @brief      Insert an entry into the Translation Table
 *
 *  @param      da      Device address
 *  @param      ua      User address
 *  @param      size    Buffer size
 *
 *  @sa         _SysLinkMemUtils_removeMapElement
 */
static Int32
_SysLinkMemUtils_insertMapElement (Ptr da, Ptr ua, UInt32 size)
{
    AddrNode      * node;
    Int32           status = PROCMGR_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "_SysLinkMemUtils_insertMapElement", da, ua,
                size);

    node = Memory_alloc (NULL, sizeof (AddrNode), 0);
    if (!node) {
        status = PROCMGR_E_MEMORY;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             status,
                             "Error allocating node memory!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        node->ua = ua;
        node->da = da;
        node->size = size;
        OsalSemaphore_pend (SysLinkMemUtils_module->semList,
                            OSALSEMAPHORE_WAIT_FOREVER);
        List_put (SysLinkMemUtils_module->addrTable, &node->elem);
        OsalSemaphore_post (SysLinkMemUtils_module->semList);
    }

    GT_1trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_insertMapElement", status);

    return status;
}


/*!
 *  @brief      Remove and delete entry from the Translation Table
 *
 *  @param      da      Device address
 *
 *  @sa         _SysLinkMemUtils_insertMapElement
 */
static Ptr
_SysLinkMemUtils_removeMapElement (Ptr da)
{
    AddrNode  * node;
    Ptr         addr = NULL;

    GT_1trace (curTrace, GT_ENTER, "_SysLinkMemUtils_removeMapElement", da);

    OsalSemaphore_pend (SysLinkMemUtils_module->semList,
                        OSALSEMAPHORE_WAIT_FOREVER);
    node = _SysLinkMemUtils_findNode (da);
    if (!node || node->da != da) {
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             PROCMGR_E_FAIL,
                             "Entry not found!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        addr = node->ua;
        List_remove (SysLinkMemUtils_module->addrTable, &node->elem);
        Memory_free (NULL, node, sizeof (AddrNode));
    }
    OsalSemaphore_post (SysLinkMemUtils_module->semList);

    GT_1trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_removeMapElement", addr);

    return addr;
}


/*!
 *  @brief      Get the size of a MemAllocBlock array
 *
 *  @param      memBlock        Pointer to a mem block array
 *  @param      numBuffers      Number of mem blocks
 *
 *  @sa         SysLinkMemUtils_alloc
 */
static UInt32
_SysLinkMemUtils_bufferSize (MemAllocBlock * memBlock, UInt numBlocks)
{
    UInt32  i;
    UInt32  size = 0;

    GT_2trace (curTrace, GT_ENTER, "_SysLinkMemUtils_bufferSize", memBlock,
                numBlocks);

    for (i = 0; i < numBlocks; i++, memBlock++) {
        if (memBlock->pixelFormat == PIXEL_FMT_PAGE) {
            size += memBlock->dim.len;
        }
        else {
            size += memBlock->dim.area.height * memBlock->stride;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "_SysLinkMemUtils_bufferSize", size);

    return size;
}


/*!
 *  @brief      Get a valid A9 address from a remote proc address
 *
 *              This function  can be called by an app running
 *              in A9 to access a buffer allocated from remote processor.
 *
 *  @param      da      Device address
 *
 *  @sa         SysLinkMemUtils_alloc, SysLinkMemUtils_free
 */
Ptr
SysLinkMemUtils_DAtoVA (Ptr da)
{
    AddrNode  * node;
    Ptr         addr = NULL;

    GT_1trace (curTrace, GT_ENTER, "SysLinkMemUtils_DAtoVA", da);

#if 1
    /* SysLinkMemUtils_DAtoVA() is now a stub that always      */
    /* returns NULL.  It is necessary to disable this function */
    /* due to the changes to the block lookup that breaks its  */
    /* functionality.                                          */
    (Void)node;
    (Void)da;
    GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         (Char *)__func__,
                         -1,
                         "SysLinkMemUtils_DAtoVA() is unavailable.");
#else
    OsalSemaphore_pend (SysLinkMemUtils_module->semList,
                        OSALSEMAPHORE_WAIT_FOREVER);
    node = _SysLinkMemUtils_findNode (da);
    OsalSemaphore_post (SysLinkMemUtils_module->semList);
    if (node) {
        addr = node->ua + (da - node->da);
    }
#endif

    GT_1trace (curTrace, GT_LEAVE, "SysLinkMemUtils_DAtoVA", addr);

    return addr;
}


/*!
 *  @brief      Allocate memory for remote processor application
 *
 *              This function  is called by remote processor application to
 *              Allocate a buffer.
 *
 *  @param      dataSize    Size of the marshalled data packet
 *  @param      data        Marshalled data packet
 *
 *  @sa         SysLinkMemUtils_free
 */
Int32
SysLinkMemUtils_alloc (UInt32 dataSize, UInt32 * data)
{

    AllocArgs                     * args            = (AllocArgs *)data;
    Int                             i;
    MemAllocBlock                 * memBlock        = NULL;
    Ptr                             allocedPtr      = NULL;
    UInt32                          retAddr         = 0;
    UInt32                          size            = 0;
    Int32                           status          = PROCMGR_SUCCESS;
    SyslinkMemUtils_MpuAddrToMap    mpuAddrList [1];

    GT_2trace (curTrace, GT_ENTER, "SysLinkMemUtils_alloc", dataSize, data);

    memBlock = Memory_calloc (NULL, sizeof (MemAllocBlock) * args->numBuffers,
                                0);
    if (!memBlock) {
        status = PROCMGR_E_MEMORY;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             status,
                             "Error allocating memBlock");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        for (i = 0; i < args->numBuffers; i++) {
            memBlock [i].pixelFormat = args->params [i].pixelFormat;
            memBlock [i].dim.area.width = args->params [i].width;
            memBlock [i].dim.area.height = args->params [i].height;
            memBlock [i].dim.len = args->params [i].length;
        }
    }

    if (status == PROCMGR_SUCCESS) {
        /* Allocation */
        allocedPtr = MemMgr_Alloc (memBlock, args->numBuffers);
        if (!allocedPtr) {
            status = PROCMGR_E_MEMORY;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 (Char *)__func__,
                                 status,
                                 "Error MemMgr buffer");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
    }

    if (status == PROCMGR_SUCCESS) {
        for (i = 0; i < args->numBuffers; i++) {
            args->params [i].stride = memBlock [i].stride;
            args->params [i].ptr = memBlock [i].ptr;
        }
        size = _SysLinkMemUtils_bufferSize (memBlock, args->numBuffers);
        mpuAddrList [0].mpuAddr = (UInt32)allocedPtr;
        mpuAddrList [0].size = size;
        status = SysLinkMemUtils_map (mpuAddrList, 1, &retAddr,
                                      ProcMgr_MapType_Tiler, PROC_SYSM3);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status != PROCMGR_SUCCESS) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 (Char *)__func__,
                                 status,
                                 "Error in SysLinkMemUtils_map");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    if (status == PROCMGR_SUCCESS) {
        status = _SysLinkMemUtils_insertMapElement ((Ptr)retAddr,
                                                        allocedPtr, size);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status != PROCMGR_SUCCESS) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 (Char *)__func__,
                                 status,
                                 "Error in SysLinkMemUtils_InsertMapElement");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    if (status != PROCMGR_SUCCESS) {
        if (retAddr) {
            SysLinkMemUtils_unmap (retAddr, PROC_SYSM3);
        }
        if (allocedPtr) {
            MemMgr_Free (allocedPtr);
        }
    }

    if (memBlock)
        Memory_free (NULL, memBlock, 1);

    GT_1trace (curTrace, GT_LEAVE, "SysLinkMemUtils_alloc", retAddr);

    return retAddr;
}


/*!
 *  @brief      Free memory allocated by SysLinkMemUtils_alloc
 *
 *              This function  is called by remote processor application to
 *              Free a previous allocated userspace buffer.
 *
 *  @param      dataSize    Size of the marshalled data packet
 *  @param      data        Marshalled data packet
 *
 *  @sa         SysLinkMemUtils_alloc
 */
Int32
SysLinkMemUtils_free (UInt32 dataSize, UInt32 * data)
{
    FreeArgs  * args    = (FreeArgs *)data;
    Ptr         ua;
    Int32       status;

    GT_2trace (curTrace, GT_ENTER, "SysLinkMemUtils_free", dataSize, data);

    ua = _SysLinkMemUtils_removeMapElement (args->bufPtr);
    if (!ua) {
        status = PROCMGR_E_INVALIDARG;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             (Char *)__func__,
                             status,
                             "Not valid address");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        status = SysLinkMemUtils_unmap ((UInt32)ua, PROC_SYSM3);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 (Char *)__func__,
                                 status,
                                 "SysLinkMemUtils_unmap failed!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = MemMgr_Free (ua);
    }

    GT_1trace (curTrace, GT_LEAVE, "SysLinkMemUtils_free", status);

    return !!status;
}


/*!
 *  @brief      Function to Map Host processor to Remote processors
 *              module
 *
 *              This function can be called by the application to map their
 *              address space to remote slave's address space.
 *
 *  @param      MpuAddr
 *
 *  @sa         SysLinkMemUtils_unmap
 */
Int
SysLinkMemUtils_map (SyslinkMemUtils_MpuAddrToMap   mpuAddrList [],
                     UInt32                         numOfBuffers,
                     UInt32 *                       mappedAddr,
                     ProcMgr_MapType                memType,
                     ProcMgr_ProcId                 procId)
{
    ProcMgr_Handle  procMgrHandle;
    UInt32          mappedSize;
    Int32           status = PROCMGR_SUCCESS;

    if (numOfBuffers > 1) {
        status = PROCMGR_E_INVALIDARG;
        Osal_printf ("SysLinkMemUtils_map numBufError [0x%x]\n", status);
        return status;
    }

    if (procId == PROC_APPM3) {
        procId = PROC_SYSM3;
    }

    if (memType == ProcMgr_MapType_Tiler) {
        /* TILER addresses are pre-mapped, so just return the TILER ssPtr */
        *mappedAddr = TilerMem_VirtToPhys ((Void *)mpuAddrList [0].mpuAddr);
        return status;
    }

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandle, procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }
    else {
        /* FIX ME: Add Proc reserve call */
        status = ProcMgr_map (procMgrHandle, (UInt32)mpuAddrList [0].mpuAddr,
                        (UInt32)mpuAddrList [0].size, mappedAddr, &mappedSize,
                        memType, procId);
         /* FIX ME: Add the table that keeps the C-D translations */
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_map [0x%x]\n", status);
        }
        else {
            status = ProcMgr_close (&procMgrHandle);
        }
    }

    return status;
}


/*!
 *  @brief      Function to unmap
 *
 *
 *  @param      mappedAddr       The remote address to unmap
 *  @param      procId                 The remote Processor ID
 *
 *  @sa         SysLinkMemUtils_map
 */
Int
SysLinkMemUtils_unmap (UInt32 mappedAddr, ProcMgr_ProcId procId)
{
    ProcMgr_Handle procMgrHandle;
    Int32          status = PROCMGR_SUCCESS;

    /* Open a handle to the ProcMgr instance. */
    if (procId == PROC_APPM3) {
        procId = PROC_SYSM3;
    }

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandle, procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
    }
    else {
        status = ProcMgr_unmap (procMgrHandle, mappedAddr, procId);
        /* FIX ME: Add Proc unreserve call */
        if (status < 0) {
            Osal_printf ("Error in ProcMgr_unmap [0x%x]\n", status);
        }
        else {
            status = ProcMgr_close (&procMgrHandle);
        }
    }

    return status;
}


/*!
 *  @brief      Function to retrieve physical entries given a remote
 *              Processor's virtual address.
 *
 *              This function returns success state of this function
 *
 *  @param      remoteAddr  The slave's address
 *  @param      size        size of buffer
 *  @param      physEntries Translated physical addresses of each Page.
 *  @param      procId      Remote Processor Id.
 *  @param      flags       Used to pass any custom information for optimization.
 *
 *  @sa         SysLinkMemUtils_virtToPhys
 */
Int
SysLinkMemUtils_virtToPhysPages (UInt32         remoteAddr,
                                 UInt32         numOfPages,
                                 UInt32         physEntries[],
                                 ProcMgr_ProcId procId)
{
    Int             i;
    Int32           status = PROCMGR_SUCCESS;
    ProcMgr_Handle  procMgrHandle;

    GT_1trace (curTrace, GT_ENTER, "SysLinkMemUtils_virtToPhys: remote Addr",
                    remoteAddr);

    if (physEntries == NULL || (numOfPages == 0)) {
        Osal_printf("SysLinkMemUtils_virtToPhys: ERROR:Input arguments invalid:"
                    "physEntries = 0x%x, numOfPages = %d\n",
                    (UInt32)physEntries, numOfPages);
        return PROCMGR_E_FAIL;
    }

    if (procId == PROC_APPM3) {
        procId = PROC_SYSM3;
    }

    Osal_printf ("testing with ProcMgr_virtToPhysPages\n");

    /* Open a handle to the ProcMgr instance. */
    status = ProcMgr_open (&procMgrHandle, procId);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
        return PROCMGR_E_FAIL;
    }
    /* TODO: Hack for tiler */
    if(remoteAddr >= TILER_ADDRESS_START && remoteAddr < TILER_ADDRESS_END) {
        for (i = 0; i < numOfPages; i++) {
            physEntries[i] = Page_ALIGN_LOW(remoteAddr, Page_SIZE_4K) + \
                                                                    (4096 * i);
        }
    }
    else {
        status = ProcMgr_virtToPhysPages (procMgrHandle, remoteAddr,
                    numOfPages, physEntries, procId);
    }

    if (status < 0) {
        Osal_printf ("Error in ProcMgr_virtToPhysPages [0x%x]\n", status);
    }
    else {
        for (i = 0; i < numOfPages; i++) {
            Osal_printf("physEntries[%d] = 0x%x\n", i, physEntries[i]);
        }
    }

    status = ProcMgr_close (&procMgrHandle);

    return status;
}


/*!
 *  @brief      Function to retrieve physical address of given a remote
 *              Processor's virtual address.
 *
 *              Return value of less than or equal to zero
 *              indicates the translation failure
 *
 *  @param      remoteAddr  The slave's address
 *  @param      physAddr    Translated physical address.
 *  @param      procId      Remote Processor Id.
  *
 *  @sa         SysLinkMemUtils_virtToPhysPages
 */
Int
SysLinkMemUtils_virtToPhys (UInt32          remoteAddr,
                            UInt32 *        physAddr,
                            ProcMgr_ProcId  procId)
{
    /* FIX ME: Hack for Tiler */
    if (remoteAddr >= TILER_ADDRESS_START && remoteAddr <= TILER_ADDRESS_END) {
        *physAddr = remoteAddr;
        printf("Translated Address = 0x%x\n", remoteAddr);
        return PROCMGR_SUCCESS;
    }
    else {
        Osal_printf("NON-TILER ADDRESS TRANSLATIONS NOT SUPPORTED"
                    "remoteAddr = 0x%x\n",remoteAddr );
        return PROCMGR_E_FAIL;
    }
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
