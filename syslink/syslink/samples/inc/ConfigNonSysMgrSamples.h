/*
 *  Copyright 2009-2010 Texas Instruments - http://www.ti.com/
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
 *  @file   ConfigNonSysMgrSamples.h
 *
 *  @brief  Header file include for all non-sysmgr related modules
 *  ============================================================================
 */

#ifndef _CONFIG_NONSYSMGRSAMPLES_H_
#define _CONFIG_NONSYSMGRSAMPLES_H_

#define NUM_MEM_ENTRIES                         3
#define RESET_VECTOR_ENTRY_ID                   0

/*! @brief Start of IPC shared memory */
#define SHAREDMEMORY_PHY_BASEADDR               0x9CF00000
#define SHAREDMEMORY_PHY_BASESIZE               0x00100000

/*! @brief Start of IPC shared memory for SysM3 */
#define SHAREDMEMORY_PHY_BASEADDR_SYSM3         0x9CF00000
#define SHAREDMEMORY_PHY_BASESIZE_SYSM3         0x00055000

/*! @brief Start of IPC shared memory AppM3 */
#define SHAREDMEMORY_PHY_BASEADDR_APPM3         0x9CF55000
#define SHAREDMEMORY_PHY_BASESIZE_APPM3         0x00055000

/*! @brief Start of IPC SHM for SysM3 */
#define SHAREDMEMORY_SLV_VRT_BASEADDR_SYSM3     0xA0000000
#define SHAREDMEMORY_SLV_VRT_BASESIZE_SYSM3     0x00055000

/*! @brief Start of IPC SHM for AppM3 */
#define SHAREDMEMORY_SLV_VRT_BASEADDR_APPM3     0xA0055000
#define SHAREDMEMORY_SLV_VRT_BASESIZE_APPM3     0x00055000

/*! @brief Start of Boot load page for SysM3 */
#define BOOTLOADPAGE_SLV_VRT_BASEADDR_SYSM3     0xA0054000
#define BOOTLOADPAGE_SLV_VRT_BASESIZE_SYSM3     0x00001000

/*! @brief Start of Boot load page for AppM3 */
#define BOOTLOADPAGE_SLV_VRT_BASEADDR_APPM3     0xA00A9000
#define BOOTLOADPAGE_SLV_VRT_BASESIZE_APPM3     0x00001000

/*! @brief Start of SW DMM shared memory */
#define SHAREDMEMORY_SWDMM_PHY_BASEADDR         0x9F300000
#define SHAREDMEMORY_SWDMM_PHY_BASESIZE         0x00C00000

/*! @brief Start of SW DMM SHM for Ducati */
#define SHAREDMEMORY_SWDMM_SLV_VRT_BASEADDR     0x81300000
#define SHAREDMEMORY_SWDMM_SLV_VRT_BASESIZE     0x00C00000


OMAP4430PROC_MemEntry  Omap4430_MemEntries[3]  = {
    {
        "DUCATI_SHM_SYSM3",
        /* NAME     : Name of the memory region */
        SHAREDMEMORY_PHY_BASEADDR_SYSM3,
        /* PHYSADDR     : Physical address */
        SHAREDMEMORY_SLV_VRT_BASEADDR_SYSM3,
        /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,
        /* MASTERVIRTADDR : Master virtual address (if known) */
        SHAREDMEMORY_SLV_VRT_BASESIZE_SYSM3,
        /* SIZE         : Size of the memory region */
        true,
        /* SHARE : Shared access memory? */
    },
    {
        "DUCATI_SHM_APPM3",
        /* NAME     : Name of the memory region */
        SHAREDMEMORY_PHY_BASEADDR_APPM3,
        /* PHYSADDR : Physical address */
        SHAREDMEMORY_SLV_VRT_BASEADDR_APPM3,
        /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,
        /* MASTERVIRTADDR : Master virtual address (if known) */
        SHAREDMEMORY_SLV_VRT_BASESIZE_APPM3,
        /* SIZE         : Size of the memory region */
        true,
        /* SHARE : Shared access memory? */
    },
    {
        "DUCATI_SHM_SWDMM",
        /* NAME     : Name of the memory region */
        SHAREDMEMORY_SWDMM_PHY_BASEADDR,
        /* PHYSADDR         : Physical address */
        SHAREDMEMORY_SWDMM_SLV_VRT_BASEADDR,
        /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,
        /* MASTERVIRTADDR : Master virtual address (if known) */
        SHAREDMEMORY_SWDMM_SLV_VRT_BASESIZE,
        /* SIZE        : Size of the memory region */
        true,
        /* SHARE : Shared access memory? */
    }
};

static UInt32   procId;
ProcMgr_Handle  procHandle;
Handle          proc4430Handle;

static inline Void ProcUtil_setup (Void)
{
    ProcMgr_Config          cfg;
    OMAP4430PROC_Config     procConfig;
    OMAP4430PROC_Params     procParams;
    ProcMgr_AttachParams    ducatiParams;
    ProcMgr_Params          procMgrParams;
    Int                     status = 0;

    ProcMgr_getConfig (&cfg);
    status = ProcMgr_setup (&cfg);
    if (status < 0) {
        Osal_printf ("ProcMgr_setup failed %d\n", status);
    }

    OMAP4430PROC_getConfig (&procConfig);
    status = OMAP4430PROC_setup (&procConfig);
    if (status < 0) {
        Osal_printf ("OMAP4430PROC_setup failed %d\n", status);
    }

    OMAP4430PROC_Params_init (NULL, &procParams);
    procParams.numMemEntries       = NUM_MEM_ENTRIES;
    procParams.memEntries          = Omap4430_MemEntries;
    procParams.resetVectorMemEntry = RESET_VECTOR_ENTRY_ID;
    proc4430Handle = OMAP4430PROC_create (procId, &procParams);
    if (proc4430Handle == NULL) {
        Osal_printf ("OMAP4430PROC_setup failed %d\n", status);
    }

    ProcMgr_Params_init (NULL, &procMgrParams);
    procMgrParams.procHandle = proc4430Handle;
    procHandle = ProcMgr_create (2, &procMgrParams);

    if (procHandle == NULL) {
        Osal_printf ("Procmgr_create %x\n", procHandle);
    }

    status = ProcMgr_attach (procHandle, &ducatiParams);
    if (status < 0) {
        Osal_printf ("ProcMgr_attach status %d\n", status);
    }
}


static inline Void ProcUtil_load (Char * imgPath)
{
    UInt32                  fileId;
    UInt32                  entryPoint = 0;
    ProcMgr_StartParams     startParams;
    Char *                  imagePath;
    Int                     status;

    startParams.proc_id = procId;
    if (2 == procId)
        imagePath = imgPath;
    else if (3 == procId)
        imagePath = imgPath;

    status = ProcMgr_load (procHandle, imagePath,
                    procId, &imagePath, &entryPoint,
                    &fileId, startParams.proc_id);
    Osal_printf ("ProcMgr Load status %d\n", status);
}


static inline Void ProcUtil_start (Void)
{
    Int                     status = 0;
    ProcMgr_StartParams     startParams;

    startParams.proc_id = procId;
    status = ProcMgr_start (procHandle, 0, &startParams);
    if (status < 0) {
        Osal_printf ("Started procmgr status %d\n", status);
    }
}


static inline Void ProcUtil_shutdown (Void)
{
    ProcMgr_detach (procHandle);
    ProcMgr_delete (&procHandle);
    ProcMgr_destroy ();

    /* Call Omap4430 delete() and destroy()*/
    OMAP4430PROC_delete (&proc4430Handle);
    OMAP4430PROC_destroy ();
}


static inline Void ProcUtil_stop (Void)
{
    ProcMgr_StopParams stopParams;

    Osal_printf ("Entered shutdown_proc\n");

    /* Call proc_stop, proc-detach, procmgr_delete,procmgr destroy*/
    stopParams.proc_id = procId;
    ProcMgr_stop (procHandle, &stopParams);
}


#endif//_CONFIG_NONSYSMGRSAMPLES_H_
