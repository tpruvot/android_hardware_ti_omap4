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
/*============================================================================
 *  @file   ProcMMU.c
 *
 *  @brief  Enabler for Ducati processor
 *
 *  ============================================================================
 */

/* OS-specific headers */
#include <host_os.h>
#include <sys/types.h>
#include <fcntl.h>

/* OSAL & Utils headers */
#include <Std.h>
#include <OsalPrint.h>
#include <Trace.h>
#include <UsrUtilsDrv.h>
#include <Memory.h>
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>

/* Module level headers */
#include <ProcMMU.h>
#include <ProcMgr.h>

#if defined (__cplusplus)
extern "C" {
#endif

/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define PROC_MMU_MPU_M3_DRIVER_NAME  "/dev/iovmm-omap0"

#define PROC_MMU_DSP_DRIVER_NAME     "/dev/iovmm-omap1"

/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Driver handle for ProcMgr in this process.
 */
static Int32 ProcMMU_MPU_M3_handle = -1;

/*!
 *  @brief  Driver handle for ProcMgr in this process.
 */
static Int32 ProcMMU_DSP_handle = -1;

/*!
 *  @brief  Reference count for the driver handle.
 */
static UInt32 DucatiMMU_refCount = 0;
static UInt32 TeslaMMU_refCount = 0;
static sem_t sem_refCount;

/* Attributes of L2 page tables for DSP MMU.*/
struct pageInfo {
    /* Number of valid PTEs in the L2 PT*/
    UInt32 num_entries;
};

enum pageType {
    SECTION         = 0,
    LARGE_PAGE      = 1,
    SMALL_PAGE      = 2,
    SUPER_SECTION   = 3
};

static void start(void) __attribute__((constructor));

void start(void)
{
    sem_init(&sem_refCount, 0, 1);
}

/*
 *  @brief  Decides a TLB entry size
 *
 */
static Int32
ProcMMU_getEntrySize (UInt32 pa, UInt32 size, enum pageType *sizeTlb,
                     UInt32 *entrySize)
{
    Int32   status          = 0, i;
    UInt32  msize;
    UInt32 pagesize[] = { PAGE_SIZE_16MB, PAGE_SIZE_1MB, PAGE_SIZE_64KB,
                                                         PAGE_SIZE_4KB};

    GT_4trace (curTrace, GT_ENTER, "ProcMMU_getEntrySize", pa, size, sizeTlb,
                entrySize);

   for (i = 0; i < 4 && pa % pagesize[i]; i++) {}

    /*  First check the page alignment*/
    if (i == 4) {
        status = ProcMMU_E_INVALIDARG;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMMU_getEntrySize",
                             status,
                             "Invalid page aligment");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        return status;
    }

    msize = (size < pagesize[i]) ? size : pagesize[i];

    /*  Now decide the entry size */
    if (msize >= PAGE_SIZE_16MB) {
        *sizeTlb   = SUPER_SECTION;
        *entrySize = PAGE_SIZE_16MB;
    } else if (msize >= PAGE_SIZE_1MB) {
        *sizeTlb   = SECTION;
        *entrySize = PAGE_SIZE_1MB;
    } else if (msize >= PAGE_SIZE_64KB) {
        *sizeTlb   = LARGE_PAGE;
        *entrySize = PAGE_SIZE_64KB;
    } else if (msize >= PAGE_SIZE_4KB) {
        *sizeTlb   = SMALL_PAGE;
        *entrySize = PAGE_SIZE_4KB;
    } else
        status = ProcMMU_E_INVALIDARG;

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_getEntrySize", status);

    return status;
}


/*!
 *  @brief  Add DSP MMU entries corresponding to given MPU-Physical address
 *          and DSP-virtual address
 *
 *  @sa     ProcMMU_getentrysize
 */
static Int32
ProcMMU_addEntry (UInt32  *physAddr, UInt32 *dspAddr, UInt32 size, Int proc)
{
    UInt32              mappedSize  = 0;
    enum pageType       sizeTlb     = SECTION;
    UInt32              entrySize   = 0;
    Int32               status      = 0;
    struct Iotlb_entry  tlbEntry;

    GT_3trace (curTrace, GT_ENTER, "ProcMMU_addEntry", *physAddr, *dspAddr,
                size);

    while ((mappedSize < size) && (status == 0)) {
        status = ProcMMU_getEntrySize (*physAddr, (size - mappedSize),
                                        &sizeTlb, &entrySize);
        if (status < 0) {
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_addEntry",
                                 status,
                                 "getEntrySize failed!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            break;
        }

        if (sizeTlb == SUPER_SECTION)
            tlbEntry.pgsz = MMU_CAM_PGSZ_16M;
        else if (sizeTlb == SECTION)
            tlbEntry.pgsz = MMU_CAM_PGSZ_1M;
        else if (sizeTlb == LARGE_PAGE)
            tlbEntry.pgsz = MMU_CAM_PGSZ_64K;
        else if (sizeTlb == SMALL_PAGE)
            tlbEntry.pgsz = MMU_CAM_PGSZ_4K;

        tlbEntry.elsz   = MMU_RAM_ELSZ_16;
        tlbEntry.endian = MMU_RAM_ENDIAN_LITTLE;
        tlbEntry.mixed  = MMU_RAM_MIXED;
        tlbEntry.prsvd  = MMU_CAM_P;
        tlbEntry.valid  = MMU_CAM_V;
        tlbEntry.da     = *dspAddr;
        tlbEntry.pa     = *physAddr;

        if (proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
            status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCSETTLBENT,
                                                            &tlbEntry);
        else if (proc == MultiProc_getId("Tesla"))
            status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCSETTLBENT,
                                                            &tlbEntry);
        if (status < 0) {
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_addEntry",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            break;
        }

        mappedSize  += entrySize;
        *physAddr   += entrySize;
        *dspAddr   += entrySize;
    }

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_addEntry", status);

    return status;
}

/*!
 *  @brief  Add DSP MMU entries corresponding to given MPU-Physical address
 *
 *
 *  @sa     ProcMMU_getentrysize
 */

Int32
ProcMMU_Map (UInt32 mpuAddr, UInt32 *da, UInt32 numOfBuffers, UInt32 size,
                                  UInt32 memPoolId, UInt32 flags, Int proc)
{
    Int32               status      = 0;
    struct ProcMMU_map_entry map_entry;

    GT_3trace (curTrace, GT_ENTER, "ProcMMU_Map", mpuAddr, *da,
                size);
    if(numOfBuffers == 0){
              status = ProcMMU_E_INVALIDARG;
              goto err_out;
    }

    map_entry.mpuAddr = mpuAddr;
    map_entry.memPoolId = memPoolId;
    map_entry.flags = flags;
    map_entry.numOfBuffers = numOfBuffers;
    map_entry.size = size;
    map_entry.da = da;
    if (proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
        status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCMEMMAP, &map_entry);
    else if (proc == MultiProc_getId("Tesla"))
        status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCMEMMAP, &map_entry);
    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_Map", status);
err_out:
    return status;
}

/*!
 *  @brief  Remove remote processor entries correspodning to given address
 *
 *
 *  @sa     ProcMMU_Map
 */


Int
ProcMMU_UnMap (UInt32 mappedAddr, Int proc)
{
    Int    status = 0;
    GT_1trace (curTrace, GT_ENTER, "ProcMMU_UnMap", mappedAddr);

    if (proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
        status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCMEMUNMAP, &mappedAddr);
    else if (proc == MultiProc_getId("Tesla"))
        status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCMEMUNMAP, &mappedAddr);
    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_UnMap", status);
    return status;
}

/*!
 *  @brief  flushes the cache entries for the given buffer mpuAddr
 *
 *
 *  @sa     ProcMMU_InvMemory
 */



Int
ProcMMU_FlushMemory (PVOID mpuAddr, UInt32 size, Int proc)
{
    Int    status = 0;
    struct ProcMMU_dmm_dma_entry flush_entry;
    GT_2trace (curTrace, GT_ENTER, "ProcMMU_FlushMemory", mpuAddr, size);
    if((mpuAddr == NULL) ||(size == 0)){
            status = ProcMMU_E_INVALIDARG;
            return status;
     }
    flush_entry.mpuAddr = mpuAddr;
    flush_entry.size = size;
    flush_entry.dir = DMA_BIDIRECTIONAL;
    if (proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
        status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCMEMFLUSH, &flush_entry);
    else if (proc == MultiProc_getId("Tesla"))
        status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCMEMFLUSH, &flush_entry);
    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_FlushMemory", status);
    return status;
}


/*!
 *  @brief  Invalidates the cache entries for the given buffer mpuAddr
 *
 *
 *  @sa     ProcMMU_InvMemory
 */



Int
ProcMMU_InvMemory(PVOID mpuAddr, UInt32 size, Int proc)
{
    Int    status = 0;
    struct ProcMMU_dmm_dma_entry inv_entry;
    GT_2trace (curTrace, GT_ENTER, "ProcMMU_InvMemory", mpuAddr, size);
    if((mpuAddr == NULL) ||(size == 0)){
            status = ProcMMU_E_INVALIDARG;
            return status;
     }
    inv_entry.mpuAddr = mpuAddr;
    inv_entry.size = size;
    inv_entry.dir = DMA_BIDIRECTIONAL;
    if (proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
        status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCMEMINV, &inv_entry);
    else if (proc == MultiProc_getId("Tesla"))
        status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCMEMINV, &inv_entry);
    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_InvMemory", status);
    return status;
}


/*!
 *  @brief  Creates a Virtual memory pool for remote processor
 *
 *  @sa     ProcMMU_DeleteVMPool
 */
Int32
ProcMMU_CreateVMPool (UInt32    poolId,
                      UInt32    size,
                      UInt32    daBegin,
                      UInt32    daEnd,
                      UInt32    flags,
                      Int       proc)
{
    Int                         status      = 0;
    struct ProcMMU_VaPool_entry poolInfo;

    GT_3trace (curTrace, GT_ENTER, "ProcMMU_CreateVMPool", poolId, daBegin,
            size);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if ((!MultiProc_isValidRemoteProc (proc)) || (size == 0)
                                              || ((Int32)poolId < 0)) {
        status = ProcMMU_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMMU_CreateVMPool",
                             status,
                             "Invalid argument specified!");

    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        poolInfo.poolId= poolId;
        poolInfo.size = size;
        poolInfo.daBegin = daBegin;
        poolInfo.daEnd = daEnd;
        poolInfo.flags = flags;
        if ((proc == MultiProc_getId ("AppM3")) ||
            (proc == MultiProc_getId ("SysM3"))) {
            status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCCREATEPOOL,
                            &poolInfo);
        }
        else if (proc == MultiProc_getId ("Tesla")) {
            status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCCREATEPOOL, &poolInfo);
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_CreateVMPool", status);

    return status;
}


/*!
 *  @brief  Deletes the Virtual memory pool
 *
 *  @sa     ProcMMU_CreateVMPool
 */
Int32
ProcMMU_DeleteVMPool (UInt32 poolId, Int proc)
{
    Int status = 0;

    GT_1trace (curTrace, GT_ENTER, "ProcMMU_DeleteVMPool", poolId);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if ((!MultiProc_isValidRemoteProc (proc)) || ((Int32)poolId < 0)) {
        status = ProcMMU_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMMU_DeleteVMPool",
                             status,
                             "Invalid argument specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if ((proc == MultiProc_getId ("AppM3")) ||
            (proc == MultiProc_getId ("SysM3"))) {
            status = ioctl (ProcMMU_MPU_M3_handle, IOVMM_IOCDELETEPOOL,
                            &poolId);
        }
        else if (proc == MultiProc_getId ("Tesla")) {
            status = ioctl (ProcMMU_DSP_handle, IOVMM_IOCDELETEPOOL, &poolId);
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_DeleteVMPool", status);

    return status;
}

/*!
 *  @brief  Function to Program Processor MMU.
 *
 *  @sa     ProcMgrDrvUsr_open
 */
UInt32
ProcMMU_init (UInt32 aPhyAddr, Int proc)
{
    UInt32 status           = 0;
    UInt32 physAddr         = 0;
    UInt32 numL4Entries;
    UInt32 i                = 0;
    UInt32 numL3MemEntries  = 0;
    UInt32 virtAddr         = 0;
    struct Mmu_entry *L4regions;
    struct Memory_entry *L3regions;
    UInt32 cpuRev;

    GT_1trace (curTrace, GT_ENTER, "ProcMMU_init", aPhyAddr);

    status = ProcMgr_getCpuRev (&cpuRev);
    if (status < 0) {
        Osal_printf ("Error in deciding the OMAP Revision [0x%x]\n", status);
        goto error_exit;
    }

    if (proc == MultiProc_getId ("Tesla")) {
        numL4Entries = (sizeof(L4MapDsp) / sizeof(struct Mmu_entry));
        numL3MemEntries = sizeof(L3MemoryRegionsDsp) /
                sizeof(struct Memory_entry);
        L4regions = (struct Mmu_entry *)L4MapDsp;
        L3regions = (struct Memory_entry *)L3MemoryRegionsDsp;
        physAddr = TESLA_BASEIMAGE_PHYSICAL_ADDRESS;
    }
    else {
        if (cpuRev == OMAP4_REV_ES1_0) {
            numL4Entries = (sizeof(L4Map_1) / sizeof(struct Mmu_entry));
            L4regions = (struct Mmu_entry *)L4Map_1;
        }
        else {
            numL4Entries = (sizeof(L4Map) / sizeof(struct Mmu_entry));
            L4regions = (struct Mmu_entry *)L4Map;
        }
        numL3MemEntries = sizeof(L3MemoryRegions) /
                      sizeof(struct Memory_entry);
        L3regions = (struct Memory_entry *)L3MemoryRegions;
        physAddr = DUCATI_BASEIMAGE_PHYSICAL_ADDRESS;
    }

    Osal_printf ("\n  Programming proc %d MMU using linear address \n", proc);

    Osal_printf ("  Programming %s memory regions\n", MultiProc_getName (proc));
    Osal_printf ("=========================================\n");
    for (i = 0; i < numL3MemEntries; i++) {
        Osal_printf ("VA = [0x%x] of size [0x%x] at PA = [0x%x]\n",
                L3regions[i].virtAddr, L3regions[i].size, physAddr);

        virtAddr = L3regions[i].virtAddr;
        if (proc != MultiProc_getId ("Tesla") && cpuRev != OMAP4_REV_ES1_0
                && i > 3) {
            status = ProcMMU_Map (physAddr, &virtAddr, 1, L3regions[i].size, -1,
                                                        DMM_DA_PHYS, proc);
            physAddr += L3regions[i].size;
        }
        else {
            status = ProcMMU_addEntry (&physAddr, &virtAddr,
                                      (L3regions[i].size), proc);
        }
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_init",
                                 status,
                                 "L3MemoryRegion addEntry failed!");
            break;
        }
    }

    if (status >= 0) {
        Osal_printf ("  Programming %s L4 peripherals\n",
                    MultiProc_getName (proc));
        Osal_printf ("=========================================\n");
        for (i = 0; i < numL4Entries; i++) {
            Osal_printf ("PA [0x%x] VA [0x%x] size [0x%x]\n",
                    L4regions[i].physAddr, L4regions[i].virtAddr,
                    L4regions[i].size);
            virtAddr = L4regions[i].virtAddr;
            physAddr = L4regions[i].physAddr;

            if (cpuRev != OMAP4_REV_ES1_0) {
                status = ProcMMU_Map (physAddr, &virtAddr, 1,
                                    (L4regions[i].size), -1, DMM_DA_PHYS, proc);
            }
            else {
                status = ProcMMU_addEntry (&physAddr, &virtAddr,
                                           (L4regions[i].size), proc);
            }

            if (status < 0) {
                Osal_printf ("**** Failed to map Peripheral ****");
                Osal_printf ("Phys addr [0x%x] Virt addr [0x%x] size [0x%x]",
                                L4regions[i].physAddr, L4regions[i].virtAddr,
                                L4regions[i].size);
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMMU_init",
                                     status,
                                     "L4Map addEntry failed!");
                break;
            }
        }
    }

error_exit:
    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_init", status);
    return status;
}


/*!
 *  @brief  Function to close the ProcMgr driver.
 *
 *  @sa     ProcMMU_open
 */
Int
ProcMMU_close (Int proc)
{
    Int                 status      = ProcMMU_S_SUCCESS;
    Int                 osStatus    = 0;

    GT_0trace (curTrace, GT_ENTER, "ProcMMU_close");

    sem_wait(&sem_refCount);
    if (proc == MultiProc_getId("Tesla") && --TeslaMMU_refCount) {
        sem_post(&sem_refCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcMMU_close", status);
        return status;
    }

    if ((proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
                                                   && --DucatiMMU_refCount) {
        sem_post(&sem_refCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcMMU_close", status);
        return status;
    }

    if (proc == MultiProc_getId("Tesla")) {
        osStatus = close (ProcMMU_DSP_handle);
        if (osStatus != 0) {
            perror ("ProcMMU_close: " PROC_MMU_DSP_DRIVER_NAME);
            status = ProcMMU_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_close",
                                 status,
                                 "Failed to close ProcMgr driver with OS!");
        }
        else {
            ProcMMU_DSP_handle = 0;
        }
    }
    else {
        osStatus = close (ProcMMU_MPU_M3_handle);
        if (osStatus != 0) {
            perror ("ProcMMU_close: " PROC_MMU_MPU_M3_DRIVER_NAME);
            status = ProcMMU_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_close",
                                 status,
                                 "Failed to close ProcMgr driver with OS!");
        }
        else {
            ProcMMU_MPU_M3_handle = 0;
        }
    }

    sem_post(&sem_refCount);

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_close", status);

    return status;
}


/*!
 *  @brief  Function to open the ProcMMU driver.
 *
 *  @sa     ProcMMU_close
 */
Int
ProcMMU_open (Int proc)
{
    Int32               status          = ProcMMU_S_SUCCESS;
    Int32               osStatus        = 0;

    GT_0trace (curTrace, GT_ENTER, "ProcMMU_open");

    sem_wait(&sem_refCount);
    if (proc == MultiProc_getId("Tesla") && TeslaMMU_refCount++) {
        sem_post(&sem_refCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcMMU_open", status);
        return status;
    }

    if ((proc == MultiProc_getId("AppM3") || proc == MultiProc_getId("SysM3"))
                                                     && DucatiMMU_refCount++) {
        sem_post(&sem_refCount);
        GT_1trace (curTrace, GT_LEAVE, "ProcMMU_open", status);
        return status;
    }

    if (proc == MultiProc_getId("Tesla")){
        ProcMMU_DSP_handle = open (PROC_MMU_DSP_DRIVER_NAME,
                                    O_RDONLY);
        if (ProcMMU_DSP_handle < 0) {
            perror ("ProcMgr driver open: " PROC_MMU_DSP_DRIVER_NAME);
            /*! @retval PROCMGR_E_OSFAILURE Failed to open ProcMgr driver with
                        OS */
            status = ProcMMU_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_open",
                                 status,
                                 "Failed to open ProcMgr driver with OS!");
        }
    }
    else {
        ProcMMU_MPU_M3_handle = open (PROC_MMU_MPU_M3_DRIVER_NAME,
                                        O_RDONLY);
        if (ProcMMU_MPU_M3_handle < 0) {
            perror ("ProcMgr driver open: " PROC_MMU_MPU_M3_DRIVER_NAME);
            status = ProcMMU_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_open",
                                 status,
                                 "Failed to open ProcMgr driver with OS!");
        }
        else {
            osStatus = fcntl (ProcMMU_MPU_M3_handle, F_SETFD, FD_CLOEXEC);
            if (osStatus != 0) {
                /*! @retval PROCMGR_E_OSFAILURE Failed to set file descriptor
                                                flags */
                status = ProcMMU_E_OSFAILURE;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMMU_open",
                                     status,
                                     "Failed to set file descriptor flags!");
            }
        }
    }

    sem_post(&sem_refCount);

    GT_1trace (curTrace, GT_LEAVE, "ProcMMU_open", status);

    return status;
}

/*!
 *  @brief  Function to Register for MMU faults
 *
 *  @sa
 */
Int32 ProcMMU_registerEvent(Int32 procId, Int32 eventfd, bool reg)
{
    Int32 fd = eventfd;
    Int32 status = ProcMMU_S_SUCCESS;

    if (procId == MultiProc_getId("AppM3") || procId == MultiProc_getId("SysM3")) {
        if (reg)
            status = ioctl (ProcMMU_MPU_M3_handle, IOMMU_IOCEVENTREG, &fd);
        else
            status = ioctl (ProcMMU_MPU_M3_handle, IOMMU_IOCEVENTUNREG, &fd);
    }else {
        status = ProcMMU_E_OSFAILURE;
    }

    if (status < 0) {
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMMU_registerEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        return ProcMMU_E_OSFAILURE;
    }
    return ProcMMU_S_SUCCESS;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
