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
 *  @file   MemoryOS.c
 *
 *  @brief      Linux user Memory interface implementation.
 *
 *              This abstracts the Memory management interface in the user-side.
 *              Allocation, Freeing-up, copy are supported for the user memory
 *              management.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* Linux OS-specific headers */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* OSAL and kernel utils */
#include <MemoryOS.h>
#include <Trace.h>
#include <List.h>
#include <Gate.h>
#include <GateMutex.h>
#include <Bitops.h>
#include <OsalDrv.h>
#include <Atomic_Ops.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Macros
 * =============================================================================
 */
/* Macro to make a correct module magic number with refCount */
#define MEMORYOS_MAKE_MAGICSTAMP(x) ((MEMORYOS_MODULEID << 12u) | (x))


/* =============================================================================
 * Structs & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure for containing
 */
typedef struct MemoryOS_MapTableInfo {
    List_Elem next;
    /*!< Pointer to next entry */
    UInt32    actualAddress;
    /*!< Actual address */
    UInt32    mappedAddress;
    /*!< Mapped address */
    UInt32    size;
    /*!< Size of the region mapped */
} MemoryOS_MapTableInfo;

/*!
 *  @brief  Structure defining state object of system memory manager.
 */
typedef struct MemoryOS_ModuleObject {
    Atomic      refCount;
    /*!< Reference count */
    List_Object mapTable;
    /*!< Head of map table */
    IGateProvider_Handle gateHandle;
    /*!< Lock handle */
} MemoryOS_ModuleObject;


/* =============================================================================
 * Globals
 * =============================================================================
 */
/*!
 *  @brief  Object containing state of the Memory OS module.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
MemoryOS_ModuleObject MemoryOS_state ;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief Initialize the memory os module.
 */
Int32
MemoryOS_setup (void)
{
    Int32  status = MEMORYOS_SUCCESS;
    /* TBD UInt32 key; */

    GT_0trace (curTrace, GT_ENTER, "MemoryOS_setup");

    /* This sets the refCount variable is not initialized, upper 16 bits is
     * written with module Id to ensure correctness of refCount variable.
     */
    Atomic_cmpmask_and_set (&MemoryOS_state.refCount,
                            MEMORYOS_MAKE_MAGICSTAMP(0),
                            MEMORYOS_MAKE_MAGICSTAMP(0));

    if (   Atomic_inc_return (&MemoryOS_state.refCount)
        != MEMORYOS_MAKE_MAGICSTAMP(1u)) {
        status = MEMORYOS_S_ALREADYSETUP;
        GT_0trace (curTrace,
                   GT_2CLASS,
                   "MemoryOS Module already initialized!");
    }
    else {
        /* Create the Gate handle */
        MemoryOS_state.gateHandle = (IGateProvider_Handle)
                                    GateMutex_create ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (MemoryOS_state.gateHandle == NULL) {
            Atomic_set (&MemoryOS_state.refCount,
                        MEMORYOS_MAKE_MAGICSTAMP(0));
            /*! @retval MEMORYOS_E_FAIL Failed to create the local gate */
            status = MEMORYOS_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MemoryOS_setup",
                                 status,
                                 "Failed to create the local gate");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Construct the map table */
            List_construct (&MemoryOS_state.mapTable, NULL);

            status = OsalDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MemoryOS_setup",
                                     status,
                                     "OsalDrv_open failed!");
                MemoryOS_destroy ();
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_setup", status);

    /*! @retval MEMORY_SUCCESS OPeration was successful */
    return status;
}


/*!
 *  @brief      Finalize the memory os module.
 */
Int32
MemoryOS_destroy (void)
{
    Int32 status    = MEMORYOS_SUCCESS;
    Int32 tmpStatus = MEMORYOS_SUCCESS;

    GT_0trace (curTrace, GT_ENTER, "MemoryOS_destroy");

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval MEMORYOS_E_INVALIDSTATE Module was not initialized */
        status = MEMORYOS_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_destroy",
                             status,
                             "Module was not initialized!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (   Atomic_dec_return (&MemoryOS_state.refCount)
            == MEMORYOS_MAKE_MAGICSTAMP(0)) {
            List_destruct (&MemoryOS_state.mapTable);

            /* Delete the gate handle */
            status = GateMutex_delete ((GateMutex_Handle *)&MemoryOS_state.gateHandle);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MemoryOS_destroy",
                                     status,
                                     "GateMutex_delete failed!");
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

            tmpStatus = OsalDrv_close ();
            if ((status >= 0) && (tmpStatus < 0)) {
                status = tmpStatus;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MemoryOS_destroy",
                                     status,
                                     "OsalDrv_close failed!");
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_destroy", status);

    /*! @retval MEMORY_SUCCESS OPeration was successful */
    return status;
}


/*!
 *  @brief      Allocates the specified number of bytes.
 *
 *  @param      ptr pointer where the size memory is allocated.
 *  @param      size amount of memory to be allocated.
 *  @sa         Memory_calloc
 */
Ptr
MemoryOS_alloc (UInt32 size, UInt32 align, UInt32 flags)
{
    Ptr ptr = NULL;

    GT_3trace (curTrace, GT_ENTER, "MemoryOS_alloc", size, align, flags);

    /* check whether the right paramaters are passed or not.*/
    GT_assert (curTrace, (size > 0));

#ifdef HAVE_POSIX_MEMALIGN
    posix_memalign (&ptr, align ? align : 4, size);
#else
    /* Stronger alignment not yet implemented */
    assert ((align == 0) || (align == 4));
    /* Call the Linux API for memory allocation */
    ptr = (Ptr) malloc (size);
#endif
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NULL == ptr) {
        /*! @retval NULL Memory allocation failed */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_alloc",
                             MEMORYOS_E_MEMORY,
                             "Could not allocate memory");
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_alloc", ptr);

    /*! @retval Pointer Memory allocation successful */
    return ptr;
}


/*!
 *  @brief      Allocates the specified number of bytes and memory is
 *              set to zero.
 *
 *  @param      ptr pointer where the size memory is allocated.
 *  @param      size amount of memory to be allocated.
 *  @sa         Memory_alloc
 */
Ptr
MemoryOS_calloc (UInt32 size, UInt32 align, UInt32 flags)
{
    Ptr ptr = NULL;

    GT_3trace (curTrace, GT_ENTER, "MemoryOS_calloc", size, align, flags);

    ptr = MemoryOS_alloc (size, align, flags);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NULL == ptr) {
        /*! @retval NULL Memory allocation failed */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_calloc",
                             MEMORYOS_E_MEMORY,
                             "Could not allocate memory");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ptr = memset (ptr, 0, size);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_calloc", ptr);

    /*! @retval Pointer Memory allocation successful */
    return ptr;
}


/*!
 *  @brief      Frees up the specified chunk of memory.
 *
 *  @param      ptr  Pointer to the previously allocated memory area.
 *  @sa         Memory_alloc
 */
Void
MemoryOS_free (Ptr ptr, UInt32 size, UInt32 flags)
{
    GT_2trace (curTrace, GT_ENTER, "MemoryOS_free", ptr, flags);

    GT_assert (GT_1CLASS, (ptr != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NULL == ptr) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_free",
                             MEMORYOS_E_INVALIDARG,
                             "Pointer NULL for free");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Free the memory pointed by ptr */
        free (ptr);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MemoryOS_free");
}


/*!
 *  @brief      Maps a memory area into virtual space.
 *
 *  @param      mapInfo  Pointer to the area which needs to be mapped.
 *
 *  @sa         Memory_unmap
 */
Int
MemoryOS_map (Memory_MapInfo * mapInfo)
{
    Int                     status   = MEMORYOS_SUCCESS;
    MemoryOS_MapTableInfo * info     = NULL;
    IArg                    key;

    GT_1trace (curTrace, GT_ENTER, "MemoryOS_map", mapInfo);

    GT_assert (curTrace, (NULL != mapInfo));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval MEMORYOS_E_INVALIDSTATE Module was not initialized. */
        status = MEMORYOS_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_map",
                             status,
                             "Module was not initialized!");
    }
    else if (mapInfo == NULL) {
        /*! @retval MEMORYOS_E_INVALIDARG NULL provided for argument mapInfo. */
        status = MEMORYOS_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_map",
                             status,
                             "NULL provided for argument mapInfo");
    }
    else if (mapInfo->src == (UInt32) NULL) {
        /*! @retval MEMORYOS_E_INVALIDARG NULL provided for argument
                                          mapInfo->src. */
        status = MEMORYOS_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_map",
                             status,
                             "NULL provided for argument mapInfo->src");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = IGateProvider_enter (MemoryOS_state.gateHandle);

        mapInfo->dst = OsalDrv_map (mapInfo->src, mapInfo->size);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (mapInfo->dst == (UInt32)NULL) {
            status = MEMORYOS_E_MAP;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MemoryOS_map",
                                 status,
                                 "Failed to map memory to user space!");
        }
        else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
            info = MemoryOS_alloc (sizeof (MemoryOS_MapTableInfo), 0, 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (info == NULL) {
                status = MEMORYOS_E_MEMORY;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MemoryOS_map",
                                     status,
                                     "Failed to allocate memory!");
            }
            else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Initialize the list element */
                List_elemClear ((List_Elem *) info);
                /* Populate the info */
                info->actualAddress = mapInfo->src;
                info->mappedAddress = mapInfo->dst;
                info->size          = mapInfo->size;
                /* Put the info into the list */
                List_putHead ((List_Handle) &MemoryOS_state.mapTable,
                              (List_Elem *) info);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
        }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        IGateProvider_leave (MemoryOS_state.gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_map", status);

    /*! @retval MEMORYOS_SUCESS Operation completed successfully. */
    return status;
}


/*!
 *  @brief      UnMaps a memory area into virtual space.
 *
 *  @param      unmapInfo poiinter to the area which needs to be unmapped.
 *
 *  @sa         Memory_map
 */
Int
MemoryOS_unmap (Memory_UnmapInfo * unmapInfo)
{
    Int         status   = MEMORYOS_SUCCESS;
    List_Elem * info     = NULL;
    IArg        key;

    GT_1trace (curTrace, GT_ENTER, "MemoryOS_unmap", unmapInfo);

    GT_assert (curTrace,(NULL != unmapInfo));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval MEMORYOS_E_INVALIDSTATE Module was not initialized. */
        status = MEMORYOS_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_unmap",
                             status,
                             "Module was not initialized!");
    }
    else if (unmapInfo == NULL) {
        /*! @retval MEMORYOS_E_INVALIDARG NULL provided for argument
                                          unmapInfo. */
        status = MEMORYOS_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_unmap",
                             status,
                             "NULL provided for argument unmapInfo");
    }
    else if (unmapInfo->addr == (UInt32) NULL) {
        /*! @retval MEMORYOS_E_INVALIDARG NULL provided for argument
                                          unmapInfo->addr. */
        status = MEMORYOS_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_unmap",
                             status,
                             "NULL provided for argument unmapInfo->addr");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = IGateProvider_enter (MemoryOS_state.gateHandle);

        /* Delete the node in the map table */
        List_traverse (info, (List_Handle) &MemoryOS_state.mapTable) {
            if (   ((MemoryOS_MapTableInfo *) info)->mappedAddress
                == unmapInfo->addr) {
                List_remove ((List_Handle) &MemoryOS_state.mapTable,
                             (List_Elem *) info);
                MemoryOS_free (info, sizeof (MemoryOS_MapTableInfo), 0);
                break;
            }
        }

        OsalDrv_unmap (unmapInfo->addr, unmapInfo->size);

        IGateProvider_leave (MemoryOS_state.gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_unmap", status);

    /*! @retval MEMORYOS_SUCESS Operation completed successfully. */
    return status;
}


/*!
 *  @brief      Copies the data between memory areas.
 *
 *  @param      dst  destination address.
 *  @param      src  source address.
 *  @param      len  length of byte to be copied.
 */
Ptr
MemoryOS_copy (Ptr dst, Ptr src, UInt32 len)
{
    GT_3trace (curTrace, GT_ENTER, "MemoryOS_copy", dst, src, len);

    GT_assert (curTrace, ((NULL != dst) && (NULL != src)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval NULL Module was not initialized */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_copy",
                             MEMORYOS_E_INVALIDSTATE,
                             "Module was not initialized!");
    }
    else if ((dst == NULL) || (src == NULL)) {
        /*! @retval NULL Invalid argument */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_copy",
                             MEMORYOS_E_INVALIDARG,
                             "Invalid argument");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        dst = memcpy (dst, src, len);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_copy", dst);

    /*! @retval Pointer Success: Pointer to updated destination buffer */
    return dst;
}


/*!
 *  @brief      Set the specific value in the said memory area.
 *
 *  @param      buf  operating buffer.
 *  @param      value the value to be stored in each byte.
 *  @param      len  length of bytes to be set.
 */
Ptr
MemoryOS_set (Ptr buf, Int value, UInt32 len)
{
    GT_3trace (curTrace, GT_ENTER, "MemoryOS_set", buf, value, len);

    GT_assert (curTrace, (NULL != buf) && (len > 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval NULL Module was not initialized */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_set",
                             MEMORYOS_E_INVALIDSTATE,
                             "Module was not initialized!");
    }
    else if (buf == NULL) {
        /*! @retval NULL Invalid argument */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_set",
                             MEMORYOS_E_INVALIDARG,
                             "Invalid argument");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        buf = memset (buf, value, len);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_set", buf);

    /*! @retval Pointer Success: Pointer to updated destination buffer */
    return buf;
}


/*!
 *  @brief      Function to translate an address.
 *
 *  @param      srcAddr  source address.
 *  @param      flags    Tranlation flags.
 */
Ptr
MemoryOS_translate (Ptr srcAddr, Memory_XltFlags flags)
{
    Ptr                     buf    = NULL;
    MemoryOS_MapTableInfo * tinfo  = NULL;
    List_Elem *             info   = NULL;
    IArg                    key;
    UInt32                  frmAddr;
    UInt32                  toAddr;

    GT_2trace (curTrace, GT_ENTER, "MemoryOS_translate", srcAddr, flags);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (   Atomic_cmpmask_and_lt (&(MemoryOS_state.refCount),
                                  MEMORYOS_MAKE_MAGICSTAMP(0),
                                  MEMORYOS_MAKE_MAGICSTAMP(1))
        == TRUE) {
        /*! @retval NULL Module was not initialized */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MemoryOS_translate",
                             MEMORYOS_E_INVALIDSTATE,
                             "Module was not initialized!");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = IGateProvider_enter (MemoryOS_state.gateHandle);

        /* Traverse to the node in the map table */
        List_traverse (info, (List_Handle) &MemoryOS_state.mapTable) {
            tinfo = (MemoryOS_MapTableInfo *) info;
            frmAddr = (flags == Memory_XltFlags_Virt2Phys) ?
                                    tinfo->mappedAddress : tinfo->actualAddress;
            toAddr = (flags == Memory_XltFlags_Virt2Phys) ?
                                    tinfo->actualAddress : tinfo->mappedAddress;
            if (   (((UInt32) srcAddr) >= frmAddr)
                && (((UInt32) srcAddr) < (frmAddr + tinfo->size))) {
                buf = (Ptr) (toAddr + ((UInt32)srcAddr - frmAddr));
                break;
            }
        }

        IGateProvider_leave (MemoryOS_state.gateHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MemoryOS_translate", buf);

    /*! @retval Pointer Success: Pointer to updated destination buffer */
    return buf;
}


#if defined (__cplusplus)
}
#endif /* defined (_cplusplus)*/
