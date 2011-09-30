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
/*****************************************************************************/
/* dlw_client.c                                                              */
/*                                                                           */
/* DLW implementation of client functions required by dynamic loader API.    */
/* Please see list of client-required API functions in dload_api.h.          */
/*                                                                           */
/* DLW is expected to run on the DSP.  It uses C6x RTS functions for file    */
/* I/O and memory management (both host and target memory).                  */
/*                                                                           */
/* A loader that runs on a GPP for the purposes of loading C6x code onto a   */
/* DSP will likely need to re-write all of the functions contained in this   */
/* module.                                                                   */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <Std.h>
#include <UsrUtilsDrv.h>
#include <Memory.h>

#include "ArrayList.h"
#include "dload.h"
#include "dload_api.h"
#include "dload4430.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "dlw_debug.h"
#include "dlw_dsbt.h"
#include "dlw_trgmem.h"
#include "ProcMgr.h"

/*---------------------------------------------------------------------------*/
/* Global flag to control debug output.                                      */
/*---------------------------------------------------------------------------*/
#if LOADER_DEBUG
Bool debugging_on = 1;
#endif

#if LOADER_DEBUG
void* memLeakTestInfoArray[100];
int memLeakTestInfoNum;
#endif

/*****************************************************************************/
/* Client Provided File I/O                                                  */
/*****************************************************************************/
/*****************************************************************************/
/* DLIF_FSEEK() - Seek to a position in specified file.                      */
/*****************************************************************************/
int DLIF_fseek(LOADER_FILE_DESC *stream, int32_t offset, int origin)
{
    return fseek(stream, offset, origin);
}

/*****************************************************************************/
/* DLIF_FTELL() - Return the current position in the given file.             */
/*****************************************************************************/
int32_t DLIF_ftell(LOADER_FILE_DESC *stream)
{
    return ftell(stream);
}

/*****************************************************************************/
/* DLIF_FREAD() - Read data from file into a host-accessible data buffer     */
/*      that can be accessed through "ptr".                                  */
/*****************************************************************************/
size_t DLIF_fread(void *ptr, size_t size, size_t nmemb,
                  LOADER_FILE_DESC *stream)
{
    return fread(ptr, size, nmemb, stream);
}

/*****************************************************************************/
/* DLIF_FCLOSE() - Close a file that was opened on behalf of the core        */
/*      loader. Core loader has ownership of the file pointer, but client    */
/*      has access to file system.                                           */
/*****************************************************************************/
int32_t DLIF_fclose(LOADER_FILE_DESC *fd)
{
    return fclose(fd);
}

/*****************************************************************************/
/* Client Provided Host Memory Management                                    */
/*****************************************************************************/
/*****************************************************************************/
/* DLIF_MALLOC() - Allocate host memory suitable for loader scratch space.   */
/*****************************************************************************/
void* DLIF_malloc(size_t size)
{
    void *ptr = NULL;
    ptr = malloc(size*sizeof(uint8_t));

#if LOADER_DEBUG
    if (ptr) {
        DLIF_trace("DLIF_malloc. %d [0x%x]\n", memLeakTestInfoNum, ptr);
        memLeakTestInfoArray[memLeakTestInfoNum++] = ptr;
    }
#endif

    return ptr;
}

/*****************************************************************************/
/* DLIF_FREE() - Free host memory previously allocated with DLIF_malloc().   */
/*****************************************************************************/
void DLIF_free(void* ptr)
{
#if LOADER_DEBUG
    int i = 0;
    int j = 0;
    for(i = 0; i < memLeakTestInfoNum; i++){
        if (memLeakTestInfoArray[i] == ptr) {
            DLIF_trace("DLIF_free. %d\n", i);
            break;
        }
    }
    if (i < memLeakTestInfoNum) {
        for(j = i; j < (memLeakTestInfoNum-1); j++) {
            memLeakTestInfoArray[j] = memLeakTestInfoArray[j+1];
        }
        memLeakTestInfoNum--;
#endif
        free(ptr);
#if LOADER_DEBUG
    }
    else {
        DLIF_trace("Failed to free ptr 0x%x.\n", ptr);
    }
#endif
}

/*****************************************************************************/
/* Client Provided Target Memory Management                                  */
/*****************************************************************************/
#define DUCATI_BOOTVECS_ADDR            0x0
#define PHYS_BOOTVECS_ADDR              0x9D000000
#define DUCATI_BOOTVECS_LEN             0x4000
#define TESLA_PHYS_OFFSET               0x400000

#define DUCATI_MEM_CODE_SYSM3_ADDR      0x4000
#define PHYS_MEM_CODE_SYSM3_ADDR        PHYS_BOOTVECS_ADDR + DUCATI_BOOTVECS_LEN
#define DUCATI_MEM_CODE_SYSM3_LEN       0xFC000

#define DUCATI_MEM_CODE_APPM3_ADDR      0x100000
#define PHYS_MEM_CODE_APPM3_ADDR        PHYS_MEM_CODE_SYSM3_ADDR + DUCATI_MEM_CODE_SYSM3_LEN
#define DUCATI_MEM_CODE_APPM3_LEN       0x300000

#define DUCATI_MEM_HEAP1_APPM3_ADDR     0x400000
#define PHYS_MEM_HEAP1_APPM3_ADDR       PHYS_MEM_CODE_APPM3_ADDR + DUCATI_MEM_CODE_APPM3_LEN
#define DUCATI_MEM_HEAP1_APPM3_LEN      0xC00000

#define DUCATI_MEM_CONST_SYSM3_ADDR     0x80000000
#define PHYS_MEM_CONST_SYSM3_ADDR       PHYS_MEM_HEAP1_APPM3_ADDR + DUCATI_MEM_HEAP1_APPM3_LEN
#define DUCATI_MEM_CONST_SYSM3_LEN      0x40000

#define DUCATI_MEM_HEAP_SYSM3_ADDR      0x80040000
#define PHYS_MEM_HEAP_SYSM3_ADDR        PHYS_MEM_CONST_SYSM3_ADDR + DUCATI_MEM_CONST_SYSM3_LEN
#define DUCATI_MEM_HEAP_SYSM3_LEN       0xC0000

#define DUCATI_MEM_CONST_APPM3_ADDR     0x80100000
#define PHYS_MEM_CONST_APPM3_ADDR       PHYS_MEM_HEAP_SYSM3_ADDR + DUCATI_MEM_HEAP_SYSM3_LEN
#define DUCATI_MEM_CONST_APPM3_LEN      0x180000

#define DUCATI_MEM_HEAP2_APPM3_ADDR     0x80280000
#define PHYS_MEM_HEAP2_APPM3_ADDR       PHYS_MEM_CONST_APPM3_ADDR + DUCATI_MEM_CONST_APPM3_LEN
#define DUCATI_MEM_HEAP2_APPM3_LEN      0x1D60000

#define DUCATI_MEM_TRACEBUF_ADDR        0x81FE0000
#define PHYS_MEM_TRACEBUF_ADDR          PHYS_MEM_HEAP2_APPM3_ADDR + DUCATI_MEM_HEAP2_APPM3_LEN
#define DUCATI_MEM_TRACEBUF_LEN         0x20000

#define DUCATI_MEM_IPC_SHMEM_LEN        (DUCATI_MEM_IPC_HEAP0_LEN + DUCATI_MEM_IPC_HEAP1_LEN)

#define DUCATI_MEM_IPC_HEAP0_ADDR       0xA0000000
#define PHYS_MEM_IPC_HEAP0_ADDR         (PHYS_BOOTVECS_ADDR - DUCATI_MEM_IPC_SHMEM_LEN)
#define DUCATI_MEM_IPC_HEAP0_LEN        0x54000

#define DUCATI_MEM_IPC_HEAP1_ADDR       0xA0054000
#define PHYS_MEM_IPC_HEAP1_ADDR         PHYS_MEM_IPC_HEAP0_ADDR + DUCATI_MEM_IPC_HEAP0_LEN
#define DUCATI_MEM_IPC_HEAP1_LEN        0xAC000

#define TESLA_MEM_CODE_DSP_ADDR         0x20000000
#define PHYS_MEM_CODE_DSP_ADDR          (PHYS_BOOTVECS_ADDR - TESLA_PHYS_OFFSET)
#define TESLA_MEM_CODE_DSP_LEN          0x80000

#define TESLA_MEM_CONST_DSP_ADDR        0x20080000
#define PHYS_MEM_CONST_DSP_ADDR         PHYS_MEM_CODE_DSP_ADDR + TESLA_MEM_CODE_DSP_LEN
#define TESLA_MEM_CONST_DSP_LEN         0x80000

#define TESLA_MEM_HEAP_DSP_ADDR         0x20100000
#define PHYS_MEM_HEAP_DSP_ADDR          PHYS_MEM_CONST_DSP_ADDR + TESLA_MEM_CONST_DSP_LEN
#define TESLA_MEM_HEAP_DSP_LEN          0x1F0000

#define TESLA_EXT_RAM                   0x20000000
#define PHYS_TESLA_EXT_ADDR             0x9A000000
#define TESLA_EXT_LEN                   0x2000000

#define TESLA_MEM_IPC_HEAP0_ADDR        0x30000000
#define TESLA_MEM_IPC_HEAP0_LEN         DUCATI_MEM_IPC_HEAP0_LEN

#define TESLA_MEM_IPC_HEAP1_ADDR        0x30054000
#define TESLA_MEM_IPC_HEAP1_LEN         DUCATI_MEM_IPC_HEAP1_LEN

struct mem_entry {
    unsigned long ducati_virt_addr;
    unsigned long mpu_phys_addr;
    unsigned long size;
    unsigned long mpu_virt_addr;
    unsigned long used;
};


static struct  mem_entry memory_regions[] = {
    {DUCATI_BOOTVECS_ADDR,PHYS_BOOTVECS_ADDR, DUCATI_BOOTVECS_LEN,0,0},
    {DUCATI_MEM_CODE_SYSM3_ADDR, PHYS_MEM_CODE_SYSM3_ADDR, DUCATI_MEM_CODE_SYSM3_LEN,0,0},
    {DUCATI_MEM_CODE_APPM3_ADDR, PHYS_MEM_CODE_APPM3_ADDR, DUCATI_MEM_CODE_APPM3_LEN,0,0},
    {DUCATI_MEM_HEAP1_APPM3_ADDR, PHYS_MEM_HEAP1_APPM3_ADDR, DUCATI_MEM_HEAP1_APPM3_LEN,0,0},
    {DUCATI_MEM_CONST_SYSM3_ADDR, PHYS_MEM_CONST_SYSM3_ADDR, DUCATI_MEM_CONST_SYSM3_LEN,0,0},
    {DUCATI_MEM_HEAP_SYSM3_ADDR, PHYS_MEM_HEAP_SYSM3_ADDR, DUCATI_MEM_HEAP_SYSM3_LEN,0,0},
    {DUCATI_MEM_CONST_APPM3_ADDR, PHYS_MEM_CONST_APPM3_ADDR, DUCATI_MEM_CONST_APPM3_LEN,0,0},
    {DUCATI_MEM_HEAP2_APPM3_ADDR, PHYS_MEM_HEAP2_APPM3_ADDR, DUCATI_MEM_HEAP2_APPM3_LEN,0,0},
    {DUCATI_MEM_TRACEBUF_ADDR, PHYS_MEM_TRACEBUF_ADDR, DUCATI_MEM_TRACEBUF_LEN,0,0},
    {DUCATI_MEM_IPC_HEAP0_ADDR, PHYS_MEM_IPC_HEAP0_ADDR, DUCATI_MEM_IPC_HEAP0_LEN,0,0},
    {DUCATI_MEM_IPC_HEAP1_ADDR, PHYS_MEM_IPC_HEAP1_ADDR, DUCATI_MEM_IPC_HEAP1_LEN,0,0},

    {TESLA_MEM_CODE_DSP_ADDR, PHYS_MEM_CODE_DSP_ADDR, TESLA_MEM_CODE_DSP_LEN,0,0},
    {TESLA_MEM_CONST_DSP_ADDR, PHYS_MEM_CONST_DSP_ADDR, TESLA_MEM_CONST_DSP_LEN,0,0},
    {TESLA_MEM_HEAP_DSP_ADDR, PHYS_MEM_HEAP_DSP_ADDR, TESLA_MEM_HEAP_DSP_LEN,0,0},
    {TESLA_MEM_IPC_HEAP0_ADDR, PHYS_MEM_IPC_HEAP0_ADDR, TESLA_MEM_IPC_HEAP0_LEN,0,0},
    {TESLA_MEM_IPC_HEAP1_ADDR, PHYS_MEM_IPC_HEAP1_ADDR, TESLA_MEM_IPC_HEAP1_LEN,0,0},

/*    {TESLA_EXT_RAM, PHYS_TESLA_EXT_ADDR, TESLA_EXT_LEN,0,0},*/
};

/* Helper function used by DLIF module. */
unsigned long translate_addr(void * client_handle, unsigned long target_addr)
{
    int i = 0;
    int num_entries = sizeof(memory_regions)/sizeof(struct mem_entry);
    unsigned long seg_offset;

    for(i=0; i< num_entries; i++){
        if (target_addr >= memory_regions[i].ducati_virt_addr && target_addr <
            (memory_regions[i].ducati_virt_addr + memory_regions[i].size)) {
            break;
        }
        else {
            continue;
        }
    }

    if(i == num_entries)
        return 0;
    else {
        seg_offset = target_addr - memory_regions[i].ducati_virt_addr;
        return (memory_regions[i].mpu_phys_addr + seg_offset);
    }
}

/*****************************************************************************/
/* DLIF_INITMEM() - Initialize the target memory.                            */
/*****************************************************************************/
BOOL DLIF_initMem(void* client_handle, uint32_t dynMemAddr, uint32_t size)
{
    if (dynMemAddr == 0)
        return FALSE;

    return DLTMM_init(client_handle, dynMemAddr, size);
}

/*****************************************************************************/
/* DLIF_DEINITMEM() - De-initialize the target memory.                      */
/*****************************************************************************/
BOOL DLIF_deinitMem(void* client_handle)
{
    return DLTMM_deinit(client_handle);
}

/*****************************************************************************/
/* DLIF_ALLOCATE() - Return the load address of the segment/section          */
/*      described in its parameters and record the run address in            */
/*      run_address field of DLOAD_MEMORY_REQUEST.                           */
/*****************************************************************************/
BOOL DLIF_allocate(void * client_handle, struct DLOAD_MEMORY_REQUEST *targ_req)
{
    /*-----------------------------------------------------------------------*/
    /* Get pointers to API segment and file descriptors.                     */
    /*-----------------------------------------------------------------------*/
    struct DLOAD_MEMORY_SEGMENT* obj_desc = targ_req->segment;
    DLoad4430_Object *clientObj = (DLoad4430_Object *)client_handle;

    obj_desc->flags = targ_req->flags;

    if (clientObj->fileId == 0xFFFFFFFF) {
        /*
         * If baseimage has not yet been loaded, the first image to be loaded
         * should be the baseimage.  So, we'll grant the requested target
         * address.
         */
        /*
         * Mark the segments as non-relocatable so that they are properly
         * released when the file is unloaded.
         */
        obj_desc->flags &= ~DLOAD_SF_relocatable;
    }
    else {
        if (!DLTMM_init(client_handle, (uint32_t)clientObj->dynLoadMem,
                        clientObj->dynLoadMemSize)) {
            DLIF_error(DLET_MEMORY,
                       "Failed to initialize target memory for dyn loading.\n");
            return FALSE;
        }

        /*-------------------------------------------------------------------*/
        /* Request target memory for this segment from the "blob".           */
        /*-------------------------------------------------------------------*/
        if (!DLTMM_malloc(client_handle, targ_req, obj_desc))
        {
            DLIF_error(DLET_MEMORY,
                       "Failed to allocate target memory for segment.\n");
            return FALSE;
        }
    }

    /*-----------------------------------------------------------------------*/
    /* Target memory request was successful.                                 */
    /*-----------------------------------------------------------------------*/
    return 1;
}

/*****************************************************************************/
/* DLIF_RELEASE() - Unmap or free target memory that was previously          */
/*      allocated by DLIF_allocate().                                        */
/*****************************************************************************/
BOOL DLIF_release(void* client_handle, struct DLOAD_MEMORY_SEGMENT* ptr)
{
    void *physAddr;
    Memory_MapInfo mapinfo;
    Memory_UnmapInfo unmapinfo;
    int status;

#if LOADER_DEBUG
    if (debugging_on)
        DLIF_trace("DLIF_free: %d bytes starting at 0x%x\n",
                   ptr->memsz_in_bytes, ptr->target_address);
#endif

    /*-----------------------------------------------------------------------*/
    /* Find the target memory packet associated with this address and mark it*/
    /* as available (will also merge with adjacent free packets).            */
    /*-----------------------------------------------------------------------*/
    if (!(ptr->flags & DLOAD_SF_relocatable)) {
        physAddr = (void *)translate_addr(client_handle,
                                          (unsigned long)(ptr->target_address));

        if (physAddr == NULL) {
            DLIF_error(DLET_MEMORY, "The target address is out of range\n");
            return FALSE;
        }

        UsrUtilsDrv_setup ();
        mapinfo.src = (unsigned long)physAddr;
        mapinfo.size =  ptr->memsz_in_bytes;
        status = Memory_map (&mapinfo);
        if (status < 0) {
            DLIF_error(DLET_MEMORY,
                       "Memory_map failed for Physical Address 0x%x Exiting\n",
                       (UInt32)mapinfo.src);
        }
        else {
            unmapinfo.addr = mapinfo.dst;
            unmapinfo.size = mapinfo.size;
            memset ((void *)mapinfo.dst, 0, ptr->memsz_in_bytes);
            status = Memory_unmap (&unmapinfo);
            if (status < 0) {
                DLIF_error (DLET_MEMORY, "Memory_unmap failed\n");
            }
        }
        UsrUtilsDrv_destroy ();
        if (status < 0) {
            return FALSE;
        }
    }
    else {
        DLTMM_free(client_handle, ptr->target_address);
    }

    return TRUE;
}

/*****************************************************************************/
/* DLIF_COPY() - Copy data from file to host-accessible memory.              */
/*      Returns a host pointer to the data in the host_address field of the  */
/*      DLOAD_MEMORY_REQUEST object.                                         */
/*****************************************************************************/
BOOL DLIF_copy(void* client_handle, struct DLOAD_MEMORY_REQUEST* targ_req)
{
    struct DLOAD_MEMORY_SEGMENT* obj_desc = targ_req->segment;
    LOADER_FILE_DESC* f = targ_req->fp;
    DLoad4430_Object *clientObj = (DLoad4430_Object *)client_handle;
    void *dstAddr = NULL;
    Memory_MapInfo mapinfo;
    int status;

    dstAddr = (void *)translate_addr(client_handle,
                                     (unsigned long)(obj_desc->target_address));
    if (dstAddr == NULL) {
        DLIF_error(DLET_MEMORY, "The target address is out of range\n");
        return FALSE;
    }

    UsrUtilsDrv_setup ();
    mapinfo.src = (unsigned long)dstAddr;
    mapinfo.size =  targ_req->segment->memsz_in_bytes;
    status = Memory_map(&mapinfo);
    UsrUtilsDrv_destroy ();
    if (status < 0) {
        DLIF_error(DLET_MEMORY,
                   "Memory_map failed for Physical Address 0x%x Exiting\n",
                   (UInt32)mapinfo.src);
        return FALSE;
    } else {
        targ_req->host_address  = (void *)mapinfo.dst;
    }
#if LOADER_DEBUG
    if (debugging_on) {
        DLIF_trace("=============================================\n");
        DLIF_trace("mapinfo.mpu_virt_addr is 0x%x\n",
                   (unsigned int)mapinfo.dst);
        DLIF_trace("mapinfo.ducati_virt_addr is 0x%x\n",
                   (unsigned int)obj_desc->target_address);
        DLIF_trace("mapinfo.mpu_phys_addr is 0x%x\n",
                   (unsigned int)mapinfo.src);
        DLIF_trace("mapinfo.size is 0x%x\n",
                   (unsigned int)mapinfo.size);
    }
#endif
    if((unsigned long)targ_req->host_address == (unsigned long)(-1)) {
        DLIF_error(DLET_MEMORY,
                   "Failed to do memory mapping for address [0x%x]\n",targ_req->host_address);
        return FALSE;
    }

    /*-------------------------------------------------------------------*/
    /* As required by API, copy the described segment into memory from   */
    /* file.  We're the client, not the loader, so we can use fseek() and*/
    /* fread().                                                          */
    /*-------------------------------------------------------------------*/
    /* ??? I don't think we want to do this if we are allocating target  */
    /*   memory for the run only placement of this segment.  If it is the*/
    /*   load placement or both load and run placement, then we can do   */
    /*   the copy.                                                       */
    /*-------------------------------------------------------------------*/
    memset(targ_req->host_address, 0, obj_desc->memsz_in_bytes);
    fseek(f,targ_req->offset,SEEK_SET);
    fread(targ_req->host_address,obj_desc->objsz_in_bytes,1,f);

    /*-------------------------------------------------------------------*/
    /* Once we have target address for this allocation, add debug        */
    /* information about this segment to the debug record for the module */
    /* that is currently being loaded.                                   */
    /*-------------------------------------------------------------------*/
    if (clientObj->DLL_debug)
    {
        /*---------------------------------------------------------------*/
        /* Add information about this segment's location to the segment  */
        /* debug information associated with the module that is          */
        /* currently being loaded.                                       */
        /*---------------------------------------------------------------*/
        /* ??? We need a way to determine whether the target address in  */
        /*     the segment applies to the load address of the segment or */
        /*     the run address.  For the time being, we assume that it   */
        /*     applies to both (that is, the dynamic loader does not     */
        /*     support separate load and run placement for a given       */
        /*     segment).                                                 */
        /*---------------------------------------------------------------*/
        DLDBG_add_segment_record(client_handle, obj_desc);
    }

#if LOADER_DEBUG
    if (debugging_on)
        DLIF_trace("DLIF_allocate: buffer 0x%x\n", targ_req->host_address);
#endif

    return TRUE;
}

/*****************************************************************************/
/* DLIF_READ() - Read content from target memory address into host-          */
/*      accessible buffer.                                                   */
/*****************************************************************************/
BOOL DLIF_read(void* client_handle, void *ptr, size_t size, size_t nmemb,
               TARGET_ADDRESS src)
{
    if (!memcpy(ptr, (const void *)src, size * nmemb))
        return FALSE;

    return TRUE;
}

/*****************************************************************************/
/* DLIF_WRITE() - Write updated (relocated) segment contents to target       */
/*      memory.                                                              */
/*****************************************************************************/
BOOL DLIF_write(void* client_handle, struct DLOAD_MEMORY_REQUEST* req)
{
    int status;
    Memory_UnmapInfo unmapinfo;

    /*-----------------------------------------------------------------------*/
    /* Nothing to do since we are relocating directly into target memory.    */
    /*-----------------------------------------------------------------------*/
    if (req->host_address) {
        unmapinfo.addr = (UInt32)req->host_address;
        unmapinfo.size = req->segment->memsz_in_bytes;
        status = Memory_unmap (&unmapinfo);
        if (status < 0)
                return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

/*****************************************************************************/
/* DLIF_EXECUTE() - Transfer control to specified target address.            */
/*****************************************************************************/
int32_t DLIF_execute(void* client_handle, TARGET_ADDRESS exec_addr)
{
    /*-----------------------------------------------------------------------*/
    /* This call will only work if the host and target are the same instance.*/
    /* The compiler may warn about this conversion from an object to a       */
    /* function pointer.                                                     */
    /*-----------------------------------------------------------------------*/
    return ((int32_t(*)())(exec_addr))();
}


/*****************************************************************************/
/* Client Provided Communication Mechanisms to assist with creation of       */
/* DLLView debug information.  Client needs to know exactly when a segment   */
/* is being loaded or unloaded so that it can keep its debug information     */
/* up to date.                                                               */
/*****************************************************************************/
/*****************************************************************************/
/* DLIF_LOAD_DEPENDENT() - Perform whatever maintenance is needed in the     */
/*      client when loading of a dependent file is initiated by the core     */
/*      loader.  Open the dependent file on behalf of the core loader,       */
/*      then invoke the core loader to get it into target memory. The core   */
/*      loader assumes ownership of the dependent file pointer and must ask  */
/*      the client to close the file when it is no longer needed.            */
/*                                                                           */
/*      If debug support is needed under the Braveheart model, then create   */
/*      a host version of the debug module record for this object.  This     */
/*      version will get updated each time we allocate target memory for a   */
/*      segment that belongs to this module.  When the load returns, the     */
/*      client will allocate memory for the debug module from target memory  */
/*      and write the host version of the debug module into target memory    */
/*      at the appropriate location.  After this takes place the new debug   */
/*      module needs to be added to the debug module list.  The client will  */
/*      need to update the tail of the DLModules list to link the new debug  */
/*      module onto the end of the list.                                     */
/*                                                                           */
/*****************************************************************************/
int DLIF_load_dependent(void* client_handle, const char* so_name)
{
    DLoad4430_Object *clientObj = (DLoad4430_Object *)client_handle;

    /*-----------------------------------------------------------------------*/
    /* Find the path and file name associated with the given so_name in the  */
    /* client's file registry.                                               */
    /*-----------------------------------------------------------------------*/
    /* If we can't find the so_name in the file registry (or if the registry */
    /* is not implemented yet), we'll open the file using the so_name.       */
    /*-----------------------------------------------------------------------*/
    int to_ret = 0;
    FILE* fp = fopen(so_name, "rb");

    /*-----------------------------------------------------------------------*/
    /* We need to make sure that the file was properly opened.               */
    /*-----------------------------------------------------------------------*/
    if (!fp)
    {
        DLIF_error(DLET_FILE, "Can't open dependent file '%s'.\n", so_name);
        return 0;
    }

    /*-----------------------------------------------------------------------*/
    /* If the dynamic loader is providing debug support for a DLL View plug- */
    /* in or script of some sort, then we are going to create a host version */
    /* of the debug module record for the so_name module.                    */
    /*-----------------------------------------------------------------------*/
    /* In the Braveheart view of the world, debug support is only to be      */
    /* provided if the DLModules symbol is defined in the base image.        */
    /* We will set up a DLL_debug flag when the command to load the base     */
    /* image is issued.                                                      */
    /*-----------------------------------------------------------------------*/
    if (clientObj->DLL_debug)
        DLDBG_add_host_record(client_handle, so_name);

    /*-----------------------------------------------------------------------*/
    /* Tell the core loader to proceed with loading the module.              */
    /*-----------------------------------------------------------------------*/
    /* Note that the client is turning ownership of the file pointer over to */
    /* the core loader at this point. The core loader will need to ask the   */
    /* client to close the dependent file when it is done using the dependent*/
    /* file pointer.                                                         */
    /*-----------------------------------------------------------------------*/
    to_ret = DLOAD_load(clientObj->loaderHandle, fp, 0, NULL);

    /*-----------------------------------------------------------------------*/
    /* If the dependent load was successful, update the DLModules list in    */
    /* target memory as needed.                                              */
    /*-----------------------------------------------------------------------*/
    if (to_ret != 0)
    {
        /*-------------------------------------------------------------------*/
        /* We will need to copy the information from our host version of the */
        /* debug record into actual target memory.                           */
        /*-------------------------------------------------------------------*/
        if (clientObj->DLL_debug)
        {
            /*---------------------------------------------------------------*/
            /* Allocate target memory for the module's debug record.  Use    */
            /* host version of the debug information to determine how much   */
            /* target memory we need and how it is to be filled in.          */
            /*---------------------------------------------------------------*/
            /* Note that we don't go through the normal API functions to get */
            /* target memory and write the debug information since we're not */
            /* dealing with object file content here.  The DLL View debug is */
            /* supported entirely on the client side.                        */
            /*---------------------------------------------------------------*/
            DLDBG_add_target_record(client_handle, to_ret);
        }
    }

    /*-----------------------------------------------------------------------*/
    /* Report failure to load dependent.                                     */
    /*-----------------------------------------------------------------------*/
    else
        DLIF_error(DLET_MISC, "Failed load of dependent file '%s'.\n", so_name);

    return to_ret;
}

/*****************************************************************************/
/* DLIF_UNLOAD_DEPENDENT() - Perform whatever maintenance is needed in the   */
/*      client when unloading of a dependent file is initiated by the core   */
/*      loader.  Invoke the DLOAD_unload() function to get the core loader   */
/*      to release any target memory that is associated with the dependent   */
/*      file's segments.                                                     */
/*****************************************************************************/
void DLIF_unload_dependent(void* client_handle, uint32_t file_handle)
{
    DLoad4430_Object *clientObj = (DLoad4430_Object *)client_handle;

    /*-----------------------------------------------------------------------*/
    /* If the specified module is no longer needed, DLOAD_load() will spin   */
    /* through the object descriptors associated with the module and free up */
    /* target memory that was allocated to any segment in the module.        */
    /*-----------------------------------------------------------------------*/
    /* If DLL debugging is enabled, find module debug record associated with */
    /* this module and remove it from the DLL debug list.                    */
    /*-----------------------------------------------------------------------*/
    if (DLOAD_unload(clientObj->loaderHandle, file_handle))
    {
        DSBT_release_entry(file_handle);
        if (clientObj->DLL_debug)
            DLDBG_rm_target_record(client_handle, file_handle);
    }
}

/*****************************************************************************/
/* Client Provided API Functions to Support Logging Warnings/Errors          */
/*****************************************************************************/

/*****************************************************************************/
/* DLIF_WARNING() - Write out a warning message from the core loader.        */
/*****************************************************************************/
void DLIF_warning(LOADER_WARNING_TYPE wtype, const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    printf("<< D L O A D >> WARNING: ");
    vprintf(fmt,ap);
    va_end(ap);
}

/*****************************************************************************/
/* DLIF_ERROR() - Write out an error message from the core loader.           */
/*****************************************************************************/
void DLIF_error(LOADER_ERROR_TYPE etype, const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    printf("<< D L O A D >> ERROR: ");
    vprintf(fmt,ap);
    va_end(ap);
}

/*****************************************************************************/
/* DLIF_trace() - Write out a trace from the core loader.                    */
/*****************************************************************************/
void DLIF_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}

/*****************************************************************************
 * END API FUNCTION DEFINITIONS
 *****************************************************************************/
