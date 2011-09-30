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
 *  @file   SharedRegion.c
 *
 *  @brief      Shared Region Manager
 *
 *              The SharedRegion module is designed to be used in a
 *              multi-processor environment where there are memory regions that
 *              are shared and accessed across different processors. This module
 *              creates a shared memory region lookup table. This lookup table
 *              contains the processor's view for every shared region in the
 *              system. Each processor must have its own lookup table. Each
 *              processor's view of a particular shared memory region can be
 *              determined by the same table index across all lookup tables.
 *              Each table entry is a base and length pair. During runtime,
 *              this table along with the shared region pointer is used to do a
 *              quick address translation.<br>
 *              The shared region pointer (#SharedRegion_SRPtr) is a 32-bit
 *              portable pointer composed of an index and offset. The most
 *              significant bits of a #SharedRegion_SRPtr are used for the index.
 *              The index corresponds to the index of the entry in the lookup
 *              table. The offset is the offset from the base of the shared
 *              memory region. The maximum number of table entries in the lookup
 *              table will determine the number of bits to be used for the index.
 *              An increase in the index means the range of the offset would
 *              decrease. Here is sample code for getting a #SharedRegion_SRPtr
 *              and then getting a the real address pointer back.
 *              @Example
 *              @code
 *                  SharedRegion_SRPtr srptr;
 *                  UInt16  index;
 *
 *                  // to get the index of the address
 *                  index = SharedRegion_getId(addr);
 *
 *                  // to get the shared region pointer for the address
 *                  srptr = SharedRegion_getSRPtr(addr, index);
 *
 *                  // to get the address back from the shared region pointer
 *                  addr = SharedRegion_getPtr(srptr);
 *              @endcode
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <String.h>
#include <Trace.h>
#include <GateMutex.h>
#include <Gate.h>
#include <Bitops.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>
#include <ProcMgr.h>
#include <ti/ipc/SharedRegion.h>
#include <_SharedRegion.h>
#include <_HeapMemMP.h>
#include <SharedRegionDrv.h>
#include <SharedRegionDrvDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Internal functions
 * =============================================================================
 */

/*
 *  @brief      Return the number of offsetBits bits
 *
 *  @param      None
 *
 *  @sa         None
 */
UInt32 SharedRegion_getNumOffsetBits();

/*!
 *  @brief      Checks to make sure overlap does not exists.
 *              Return error if overlap found.
 *
 *  @param      base      Base of Shared Region
 *  @param      len       Length of Shared Region
 *
 *  @sa         None
 */
Int SharedRegion_checkOverlap (Ptr    base, UInt32 len);


/* =============================================================================
 * Macros and types
 * =============================================================================
 */

/*!
 *  @def    IS_RANGE_VALID
 *  @brief  checks if addrress is within the range.
 */
#define IS_RANGE_VALID(x,min,max) (((x) < (max)) && ((x) >= (min)))


/* =============================================================================
 * Structure & Enums
 * =============================================================================
 */
/*!
 *  @brief  SharedRegion Module state object
 */
typedef struct SharedRegion_ModuleObject_tag {
    UInt32                setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
     *   process.
     */
    IGateProvider_Handle  localLock;      /*!< Handle to a gate instance */
    SharedRegion_Region * regions;        /*!< Pointer to the regions */
    UInt32                numOffsetBits;  /*!< Index bit offset */
    UInt32                offsetMask;
    UInt32              * bCreatedInKnlSpace; /* Flags to denote if regions are
                                               * created in user space or created
                                               * in knl space
                                               */
    SharedRegion_Config   cfg;        /*!< Current config values */
} SharedRegion_ModuleObject;


/* =============================================================================
 * Globals
 * =============================================================================
 */
/*!
 *  @var    SharedRegion_state
 *
 *  @brief  ProcMgr state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
SharedRegion_ModuleObject SharedRegion_state =
{
    .setupRefCount        = 0,
    .numOffsetBits        = 0,
    .regions              = NULL,
    .localLock            = NULL,
    .offsetMask           = 0,
};

/*!
 *  @var    SharedRegion_module
 *
 *  @brief  Pointer to the SharedRegion module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
SharedRegion_ModuleObject * SharedRegion_module = &SharedRegion_state;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/* Function to get the configuration */
Void
SharedRegion_getConfig (SharedRegion_Config * config)
{
    Int                     status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getConfig", config);

    GT_assert (curTrace, (config != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (config == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getConfig",
                             SharedRegion_E_INVALIDARG,
                             "Argument of type (SharedRegion_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        SharedRegionDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.getConfig.config = config;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            SharedRegionDrv_ioctl (CMD_SHAREDREGION_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_getConfig",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");

            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        SharedRegionDrv_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_getConfig");
}


/* Function to setup the SharedRegion module. */
Int
SharedRegion_setup (SharedRegion_Config * cfg)
{
    Int                     status  = SharedRegion_S_SUCCESS;
    SharedRegion_Config *   ptCfg   = NULL;
    SharedRegion_Config     tCfg;
    UInt16                  i;
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_setup", cfg);

    /* TBD: Protect from multiple threads. */
    SharedRegion_module->setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (SharedRegion_module->setupRefCount > 1) {
        /*! @retval SharedRegion_S_ALREADYSETUP Success: SharedRegion module has
                                           been already setup in this process */
        status = SharedRegion_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "SharedRegion module has been already setup in this process."
                   "\n  RefCount: [%d]\n",
                   SharedRegion_module->setupRefCount);
    }
    else {
        /* Open the driver handle. */
        status = SharedRegionDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (cfg == NULL) {
                SharedRegion_getConfig (&tCfg);
                ptCfg = &tCfg;
            }
            else {
                ptCfg = cfg;
            }

            if ((ptCfg->numEntries > 0u) && (status >= 0)) {
                /* copy the user provided values into the state object */
                Memory_copy ((Ptr) &(SharedRegion_module->cfg),
                             (Ptr) ptCfg,
                             sizeof (SharedRegion_Config));
            }
            else {
                /*! @retval SharedRegion_E_INVALIDARG cfg->numEntries is 0 */
                status = SharedRegion_E_INVALIDARG;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_setup",
                                     status,
                                     "cfg->numEntries is 0!");
            }

            if (status >= 0) {
                /* Allocate memory for the regions */
                SharedRegion_module->regions = (SharedRegion_Region *)
                                              Memory_alloc (NULL,
                                              (  sizeof (SharedRegion_Region)
                                               * ptCfg->numEntries),
                                              0);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (SharedRegion_module->regions == NULL) {
                    status = SharedRegion_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_setup",
                                     status,
                                     "Failed to allocate memory for regions!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    SharedRegion_module->bCreatedInKnlSpace =
                                              Memory_calloc (NULL,
                                              (  sizeof (Int) * ptCfg->numEntries),
                                              0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                    if (SharedRegion_module->bCreatedInKnlSpace == NULL) {
                        status = SharedRegion_E_MEMORY;
                        GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "SharedRegion_setup",
                                         status,
                                         "Failed to allocate memory for"
                                         "bCreatedInKnlSpace!");
                    }
                    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                        cmdArgs.args.setup.regions  = SharedRegion_module->regions;
                        cmdArgs.args.setup.config   = (SharedRegion_Config *) ptCfg;
                        for (i = 0; i < SharedRegion_module->cfg.numEntries; i++) {
                            SharedRegion_module->regions[i].entry.base = NULL;
                            SharedRegion_module->regions[i].entry.len = 0;
                            SharedRegion_module->regions[i].entry.ownerProcId = 0;
                            SharedRegion_module->regions[i].entry.isValid = FALSE;
                            SharedRegion_module->regions[i].entry.cacheEnable = TRUE;
                            SharedRegion_module->regions[i].entry.cacheLineSize =
                                            SharedRegion_module->cfg.cacheLineSize;
                            SharedRegion_module->regions[i].entry.createHeap   = FALSE;
                            SharedRegion_module->regions[i].reservedSize = 0;
                            SharedRegion_module->regions[i].heap = NULL;
                            SharedRegion_module->regions[i].entry.name = NULL;
                        }

                        /* set the defaults for region 0  */
                        SharedRegion_module->regions[0].entry.createHeap  = TRUE;
                        SharedRegion_module->regions[0].entry.ownerProcId = MultiProc_self();

                        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_SETUP,
                                                        &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                        if (status < 0) {
                            status = SharedRegion_E_OSFAILURE;
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "SharedRegion_setup",
                                                 status,
                                                 "API (through IOCTL) failed on "
                                                 "kernel-side!");
                        }
                    }
                }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            }

            if (status >= 0) {
                SharedRegion_module->numOffsetBits =
                                            SharedRegion_getNumOffsetBits ();
                SharedRegion_module->offsetMask =
                                 (1 << SharedRegion_module->numOffsetBits) - 1;

                /* Create a lock for protecting list object */
                SharedRegion_module->localLock = (IGateProvider_Handle)
                                   GateMutex_create ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (SharedRegion_module->localLock == NULL) {
                    status = SharedRegion_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "SharedRegion_setup",
                                         SharedRegion_E_MEMORY,
                                         "Failed to create the localLock!");
                }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        if (status < 0) {
            SharedRegion_destroy ();
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_setup", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successful */
    return (status);
}


/* Function to destroy the SharedRegion module. */
Int
SharedRegion_destroy (Void)
{
    Int                        status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs    cmdArgs;
    SharedRegion_Config        tCfg;
    IArg                       key;

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_destroy");

    /* TBD: Protect from multiple threads. */
    SharedRegion_module->setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (SharedRegion_module->setupRefCount == 0) {
        SharedRegion_getConfig (&tCfg);
        cmdArgs.args.setup.regions  = SharedRegion_module->regions;
        cmdArgs.args.setup.config   = (SharedRegion_Config *) &tCfg;
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = SharedRegion_E_OSFAILURE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Enter the gate */
        if (SharedRegion_module->localLock != NULL) {
            key = IGateProvider_enter (SharedRegion_module->localLock);
        }

        if (SharedRegion_module->bCreatedInKnlSpace != NULL) {
            Memory_free (NULL,
                         SharedRegion_module->bCreatedInKnlSpace,
                         (  sizeof (UInt32) *
                         SharedRegion_module->cfg.numEntries));
        }
        if (SharedRegion_module->regions != NULL) {
            Memory_free (NULL,
                         SharedRegion_module->regions,
                         (  sizeof (SharedRegion_Region)
                          * SharedRegion_module->cfg.numEntries));
            SharedRegion_module->regions = NULL;
        }

        Memory_set (&SharedRegion_module->cfg,
                    0,
                    sizeof (SharedRegion_Config));

        SharedRegion_module->numOffsetBits = 0;
        SharedRegion_module->offsetMask    = 0;

        if (SharedRegion_module->localLock != NULL) {
            /* Leave the gate */
            IGateProvider_leave (SharedRegion_module->localLock, key);

            /* Delete the local lock */
            status = GateMutex_delete ((GateMutex_Handle *)
                                       &SharedRegion_module->localLock);
            GT_assert (curTrace, (status >= 0));
            if (status < 0) {
                status = SharedRegion_E_FAIL;
            }
        }

        /* Close the driver handle. */
        SharedRegionDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_destroy", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successful */
    return status;
}


/* Creates a heap by owner of region for each SharedRegion.
 * Function is called by Ipc_start(). Requires that SharedRegion 0
 * be valid before calling start().
 */
Int
SharedRegion_start (Void)
{
    Int                        status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_start");

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        /*! @retval  SharedRegion_E_INVALIDSTATE Module is in invalid state */
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_start",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_START, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_start",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_start", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successful */
    return status;
}


/* Function to stop the SharedRegion module.  */
Int
SharedRegion_stop (Void)
{
    Int                        status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_stop");

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        /*! @retval  SharedRegion_E_INVALIDSTATE Module is in invalid state */
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_stop",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_STOP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_stop",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_stop", status);

    return status;
}


/* Opens a heap, for non-owner processors, for each SharedRegion. */
Int
SharedRegion_attach (UInt16 remoteProcId)
{
    Int                        status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_attach", remoteProcId);

    GT_assert (curTrace, (remoteProcId < MultiProc_MAXPROCESSORS));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        /*! @retval  SharedRegion_E_INVALIDSTATE Module is in invalid state */
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_attach",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.attach.remoteProcId  = remoteProcId;
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_ATTACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_attach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_attach", status);

    /*! @retval SharedRegion_S_SUCCESS Operation successful */
    return status;
}


/* Closes a heap, for non-owner processors, for each SharedRegion. */
Int
SharedRegion_detach (UInt16 remoteProcId)
{
    Int                        status = SharedRegion_S_SUCCESS;
    SharedRegionDrv_CmdArgs    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_detach", remoteProcId);

    GT_assert (curTrace, (remoteProcId < MultiProc_MAXPROCESSORS));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        /*! @retval  SharedRegion_E_INVALIDSTATE Module is in invalid state */
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_detach",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.detach.remoteProcId  = remoteProcId;
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_DETACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_detach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_detach", status);

    return status;
}


/* Sets the table information entry in the table. */
Int
SharedRegion_setEntry (UInt16               id,
                       SharedRegion_Entry * entry)
{
    Int                     status = SharedRegion_S_SUCCESS;
    SharedRegion_Region  *  region = NULL;
    UInt32                  physAddress;
    SharedRegionDrv_CmdArgs cmdArgs;
    IArg                    key;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_setEntry", id, entry);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));
    GT_assert (curTrace, (entry != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_setEntry",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (entry == NULL) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_setEntry",
                             status,
                             "Table entry cannot be NULL!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_setEntry",
                             status,
                             "index is invalid");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        physAddress = (UInt32) Memory_translate (entry->base,
                                                 Memory_XltFlags_Virt2Phys);
        Memory_copy (&cmdArgs.args.setEntry.entry,
                     entry,
                     sizeof (SharedRegion_Entry));
        cmdArgs.args.setEntry.entry.base  = (Ptr) physAddress;
        cmdArgs.args.setEntry.id          = id;
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_SETENTRY, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status >= 0) {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Enter the gate */
            key = IGateProvider_enter (SharedRegion_module->localLock);

            region = &(SharedRegion_module->regions [id]);

            /* set specified region id to entry values */
            Memory_copy ((Ptr) &(region->entry),
                         (Ptr) entry,
                         sizeof (SharedRegion_Entry));

            /* Leave the gate */
            IGateProvider_leave (SharedRegion_module->localLock, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */


    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_setEntry", status);

    return status;
}


/* Returns the id of shared region in which the pointer resides.
 * Returns 0 if SharedRegion_module->cfg.translate set to TRUE.
 * It returns SharedRegion_INVALIDREGIONID if no entry is found.
 */
UInt16
SharedRegion_getId (Ptr addr)
{
    SharedRegion_Region  * region = NULL;
    UInt16                 id = SharedRegion_INVALIDREGIONID;
    UInt32                 i;
    IArg                   key;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getId", addr);

    /* addr can be NULL. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getId",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    /* Return invalid for NULL addr */
    if (addr != NULL) {
        /* Enter the gate */
        key = IGateProvider_enter (SharedRegion_module->localLock);

        for (i = 0; i < SharedRegion_module->cfg.numEntries; i++) {
            region = &(SharedRegion_module->regions [i]);
            if (   (region->entry.isValid)
                && (addr >= region->entry.base)
                && (  addr
                    < (Ptr)((UInt32) region->entry.base + region->entry.len))) {

                id = i;
                break;
            }
        }

        /* Leave the gate */
        IGateProvider_leave (SharedRegion_module->localLock, key);
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getId", id);

    return id;

}


/* Returns the address pointer associated with the shared region pointer. */
Ptr
SharedRegion_getPtr (SharedRegion_SRPtr srPtr)
{

    SharedRegion_Region * region    = NULL;
    Ptr                   returnPtr = NULL;
    IArg                  key    = 0;
    UInt16                regionId;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getPtr", srPtr);

    /* srPtr can be invalid. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getPtr",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    /* Return NULL for invalid SrPTR */
    else
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    if (srPtr != SharedRegion_INVALIDSRPTR) {
        if (SharedRegion_module->cfg.translate == FALSE) {
            returnPtr = (Ptr) srPtr;
        }
        else {
            /* Enter the gate */
            key = IGateProvider_enter (SharedRegion_module->localLock);

            regionId = (UInt32) (srPtr >> SharedRegion_module->numOffsetBits);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (regionId >= SharedRegion_module->cfg.numEntries) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_getPtr",
                                     SharedRegion_E_INVALIDARG,
                                     "Id cannot be larger than numEntries!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                region = &(SharedRegion_module->regions [regionId]);

                returnPtr = (Ptr)(  (srPtr & SharedRegion_module->offsetMask)
                                  + (UInt32) region->entry.base);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Leave the gate */
            IGateProvider_leave (SharedRegion_module->localLock, key);
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getPtr", returnPtr);

    return returnPtr;

}


/* Returns the address pointer associated with the shared region pointer. */
SharedRegion_SRPtr
SharedRegion_getSRPtr (Ptr addr, UInt16 id)
{
    SharedRegion_Region * region  = NULL;
    SharedRegion_SRPtr    retPtr = SharedRegion_INVALIDSRPTR ;
    IArg                  key    = 0;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_getSRPtr", addr, id);

    /* addr can be NULL. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getSRPtr",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    /* Return invalid for NULL addr */
    if (addr != NULL) {
        GT_assert (curTrace, (id != SharedRegion_INVALIDREGIONID));
        GT_assert (curTrace,
                   (    (id != SharedRegion_INVALIDREGIONID)
                    &&  (id < SharedRegion_module->cfg.numEntries)));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (id == SharedRegion_INVALIDREGIONID) {
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getSRPtr",
                             SharedRegion_E_INVALIDARG,
                            "Invalid id SharedRegion_INVALIDREGIONID passed!");
        }
        else if (id >= SharedRegion_module->cfg.numEntries) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_getSRPtr",
                                 SharedRegion_E_INVALIDARG,
                                 "id cannot be larger than numEntries!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Enter the gate */
            /* if no shared region configured, set SRPtr to addr */
            if (SharedRegion_module->cfg.translate == FALSE) {
                retPtr = (SharedRegion_SRPtr) addr;
            }
            else {
                /* Enter the gate */
                key = IGateProvider_enter (SharedRegion_module->localLock);

                region = &(SharedRegion_module->regions[id]);

                /*
                 *  Note: The very last byte on the very last id cannot be
                 *        mapped because SharedRegion_INVALIDSRPTR which is ~0
                 *        denotes an error. Since pointers should be word
                 *        aligned, we don't expect this to be a problem.
                 *
                 *        ie: numEntries = 4,
                 *            id = 3, base = 0x00000000, len = 0x40000000
                 *            ==> address 0x3fffffff would be invalid because
                 *                the SRPtr for this address is 0xffffffff
                 */
                if (    ((UInt32) addr >= (UInt32) region->entry.base)
                    &&  (  (UInt32) addr
                         < ((UInt32) region->entry.base + region->entry.len))) {
                    retPtr = (SharedRegion_SRPtr)
                              (  (id << SharedRegion_module->numOffsetBits)
                               | ((UInt32) addr - (UInt32) region->entry.base));
                }
                else {
                    retPtr = SharedRegion_INVALIDSRPTR;
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "SharedRegion_getSRPtr",
                                         SharedRegion_E_INVALIDARG,
                                         "Provided addr is not in correct range"
                                         " for the specified id!");
                }
                /* Leave the gate */
                IGateProvider_leave (SharedRegion_module->localLock, key);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getSRPtr", retPtr);

    return retPtr;
}


/* Clears the region in the table. */
Int
SharedRegion_clearEntry (UInt16 id)
{
    Int                     status = SharedRegion_S_SUCCESS;
    SharedRegion_Region *   region = NULL;
    IArg                    key    = 0;
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_clearEntry", id);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));
    /* Need to make sure not trying to clear Region 0 */
    GT_assert (curTrace, (id != 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        status = SharedRegion_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_clearEntry",
                             status,
                             "Module is in invalid state!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_clearEntry",
                             status,
                             "index is outside range of configured numEntries");
    }
    else if (id == 0u) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_clearEntry",
                             status,
                             "Clearing Shared Region 0 is not permitted");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.clearEntry.id = id;
        status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_CLEARENTRY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_clearEntry",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Enter the gate */
            key = IGateProvider_enter (SharedRegion_module->localLock);

            region = &(SharedRegion_module->regions [id]);

            /* Clear region to their defaults */
            region->entry.isValid       = FALSE;
            region->entry.base          = NULL;
            region->entry.len           = 0u;
            region->entry.ownerProcId   = SharedRegion_DEFAULTOWNERID;
            region->entry.cacheEnable   = TRUE;
            region->entry.cacheLineSize = SharedRegion_module->cfg.cacheLineSize;
            region->entry.createHeap    = FALSE;
            region->entry.name          = NULL;
            region->reservedSize        = 0u;
            region->heap                = NULL;

            /* Leave the gate */
            IGateProvider_leave (SharedRegion_module->localLock, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_clearEntry", status);

    return status;
}


/* Returns Heap Handle of associated id */
Ptr SharedRegion_getHeap (UInt16 id)
{
    Int                     status = SharedRegion_S_SUCCESS;
    IHeap_Handle            heap   = NULL;
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getHeap", id);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        /*! @retval  NULL Module is in invalid state! */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getHeap",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getHeap",
                             status,
                             "index is outside range of configured numEntries");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /*
         *  If translate == TRUE or translate == FALSE
         *  and 'id' is not INVALIDREGIONID, then assert id is valid.
         *  Return the heap associated with the region id.
         *
         *  If those conditions are not met, the id is from
         *  an addres in local memory so return NULL.
         */
        if (    (SharedRegion_module->cfg.translate)
            ||  (   (SharedRegion_module->cfg.translate == FALSE)
                 && (id != SharedRegion_INVALIDREGIONID))) {
            if (id >= SharedRegion_module->cfg.numEntries) {
                /* Need to make sure id is smaller than numEntries */
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_getHeap",
                                     SharedRegion_E_INVALIDARG,
                                     "id cannot be larger than numEntries!");
            }
            else {
                cmdArgs.args.getHeap.id = id;

                status = SharedRegionDrv_ioctl (CMD_SHAREDREGION_GETHEAP,
                                                &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "SharedRegion_getHeap",
                                         status,
                                         "API (through IOCTL) failed on "
                                         "kernel-side!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    heap = HeapMemMP_getUsrHandle ((HeapMemMP_Handle)
                                            cmdArgs.args.getHeap.heapHandle);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getHeap", heap);

    return heap;
}


/* Returns the id of shared region that matches name.
 * Returns SharedRegion_INVALIDREGIONID if no region is found.
 */
UInt16
SharedRegion_getIdByName (String name)
{
    SharedRegion_Region * region = NULL;
    UInt16                regionId = SharedRegion_INVALIDREGIONID;
    UInt16                i;
    IArg                  key   = 0;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getIdByName", name);

    GT_assert (curTrace, (name != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getHeap",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (name == NULL){
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getIdByName",
                             SharedRegion_E_INVALIDARG,
                             "Index is outside range of configured numEntries");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* loop through entries to find matching name */
        key = IGateProvider_enter (SharedRegion_module->localLock);

        for (i = 0; i < SharedRegion_module->cfg.numEntries; i++) {
            region = &(SharedRegion_module->regions [i]);

            if (region->entry.isValid) {
                if (String_cmp (region->entry.name, name) == 0) {
                    regionId = i;
                    break;
                }
            }
        }

        /* Leave the gate */
        IGateProvider_leave (SharedRegion_module->localLock, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getIdByName", regionId);

    return (regionId);
}


/* Gets the entry information for the specified region id */
Int
SharedRegion_getEntry (UInt16               id,
                       SharedRegion_Entry * entry)
{
    Int                   status = SharedRegion_S_SUCCESS;
    SharedRegion_Region * region = NULL;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_getEntry", id, entry);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));
    GT_assert (curTrace, (entry != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getHeap",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (entry == NULL) {
        status = SharedRegion_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getEntry",
                             status,
                             "Table entry cannot be NULL!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        /*! @retval None
         *          Id cannot be larger than numEntries
         */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getEntry",
                             SharedRegion_E_INVALIDARG,
                             "Id cannot be larger than numEntries!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        region = &(SharedRegion_module->regions [id]);

        Memory_copy ((Ptr) entry,
                     (Ptr) &(region->entry),
                     sizeof (SharedRegion_Entry));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getEntry", status);

    return status;
}


/* Get cache line size */
SizeT
SharedRegion_getCacheLineSize (UInt16 id)
{
    SizeT cacheLineSize = sizeof (Int);

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_getCacheLineSize", id);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getCacheLineSize",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getCacheLineSize",
                             SharedRegion_E_INVALIDARG,
                             "Id cannot be larger than numEntries!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /*
         *  If translate == TRUE or translate == FALSE
         *  and 'id' is not INVALIDREGIONID, then assert id is valid.
         *  Return the heap associated with the region id.
         *
         *  If those conditions are not met, the id is from
         *  an addres in local memory so return NULL.
         */
        if (    (SharedRegion_module->cfg.translate)
            ||  (   (SharedRegion_module->cfg.translate == FALSE)
                 && (id != SharedRegion_INVALIDREGIONID))) {
            if (id >= SharedRegion_module->cfg.numEntries) {
                /* Need to make sure id is smaller than numEntries */
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_getCacheLineSize",
                                     SharedRegion_E_INVALIDARG,
                                     "id cannot be larger than numEntries!");
            }
            else {
                cacheLineSize =
                        SharedRegion_module->regions [id].entry.cacheLineSize;
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getCacheLineSize",
               cacheLineSize);

    return cacheLineSize;
}


/* Is cache enabled */
Bool
SharedRegion_isCacheEnabled (UInt16 id)
{
    Bool cacheEnable = FALSE;

    GT_1trace (curTrace, GT_ENTER, "SharedRegion_isCacheEnabled", id);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_isCacheEnabled",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_isCacheEnabled",
                             SharedRegion_E_INVALIDARG,
                             "Id cannot be larger than numEntries!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /*
         *  If translate == TRUE or translate == FALSE
         *  and 'id' is not INVALIDREGIONID, then assert id is valid.
         *  Return the heap associated with the region id.
         *
         *  If those conditions are not met, the id is from
         *  an addres in local memory so return NULL.
         */
        if (    (SharedRegion_module->cfg.translate)
            ||  (   (SharedRegion_module->cfg.translate == FALSE)
                 && (id != SharedRegion_INVALIDREGIONID))) {
            if (id >= SharedRegion_module->cfg.numEntries) {
                /* Need to make sure id is smaller than numEntries */
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_isCacheEnabled",
                                     SharedRegion_E_INVALIDARG,
                                     "id cannot be larger than numEntries!");
            }
            else {
                cacheEnable =
                            SharedRegion_module->regions[id].entry.cacheEnable;
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_isCacheEnabled", cacheEnable);

    return (cacheEnable);
}


/* Whether address translation is enabled */
Bool
SharedRegion_translateEnabled (Void)
{
    GT_0trace (curTrace, GT_ENTER, "SharedRegion_translateEnabled");
    GT_1trace (curTrace,
               GT_LEAVE,
               "SharedRegion_translateEnabled",
               SharedRegion_module->cfg.translate);

    return (SharedRegion_module->cfg.translate);
}


/* Gets the number of regions */
UInt16
SharedRegion_getNumRegions (Void)
{
    GT_0trace (curTrace, GT_ENTER, "SharedRegion_getNumRegions");
    GT_1trace (curTrace,
               GT_LEAVE,
               "SharedRegion_getNumRegions",
               SharedRegion_module->cfg.numEntries);
    return (SharedRegion_module->cfg.numEntries);
}


/* Initializes the entry fields */
Void
SharedRegion_entryInit (SharedRegion_Entry * entry)
{
    GT_1trace (curTrace, GT_ENTER, "SharedRegion_entryInit", entry);

    GT_assert (curTrace, (entry != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_entryInit",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (entry == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_clearEntry",
                             SharedRegion_E_INVALIDARG,
                             "Invalid NULL entry provided!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* init the entry to default values */
        entry->base          = NULL;
        entry->len           = 0;
        entry->ownerProcId   = SharedRegion_DEFAULTOWNERID;
        entry->cacheEnable   = TRUE;
        entry->cacheLineSize = SharedRegion_module->cfg.cacheLineSize;
        entry->createHeap    = FALSE;
        entry->name          = NULL;
        entry->isValid       = FALSE;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "SharedRegion_entryInit");
}


/* Returns invalid SRPtr value. */
SharedRegion_SRPtr SharedRegion_invalidSRPtr (Void)
{
    return (SharedRegion_INVALIDSRPTR);
}


/* =============================================================================
 *  Internal Functions
 * =============================================================================
 */
/* Reserves the specified amount of memory from the specified region id. */
Ptr
SharedRegion_reserveMemory (UInt16 id, SizeT size)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = SharedRegion_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Ptr                   retPtr = NULL;
    SharedRegion_Region * region = NULL;
    UInt32                minAlign;
    SizeT                 newSize;
    SizeT                 curSize;
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_reserveMemory", id, size);

    GT_assert (curTrace, (id < SharedRegion_module->cfg.numEntries));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_reserveMemory",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (id >= SharedRegion_module->cfg.numEntries) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_reserveMemory",
                             SharedRegion_E_INVALIDARG,
                             "Id cannot be larger than numEntries!");
    }
    else if (SharedRegion_module->regions[id].entry.isValid == FALSE) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_reserveMemory",
                             SharedRegion_E_INVALIDARG,
                             "Specified region ID is not valid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        minAlign = Memory_getMaxDefaultTypeAlign ();
        if (SharedRegion_getCacheLineSize (id) > minAlign) {
            minAlign = SharedRegion_getCacheLineSize (id);
        }

        region = &(SharedRegion_module->regions [id]);

        /* Set the current size to the reservedSize */
        curSize = region->reservedSize;

        /* No need to round here since curSize is already aligned */
        retPtr = (Ptr)((UInt32) region->entry.base + curSize);

        /*  Round the new size to the min alignment since */
        newSize = ROUND_UP (size, minAlign);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        /* Need to make sure (curSize + newSize) is smaller than region len */
        if (region->entry.len < (curSize + newSize)) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_reserveMemory",
                                 SharedRegion_E_INVALIDARG,
                                 "Too large size is requested to be reserved!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Add the new size to current size */
            region->reservedSize = curSize + newSize;

            /* Now do the same on kernel-side. */
            cmdArgs.args.reserveMemory.id = id;
            cmdArgs.args.reserveMemory.size = size;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            SharedRegionDrv_ioctl (CMD_SHAREDREGION_RESERVEMEMORY,
                                            &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "SharedRegion_reserveMemory",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_reserveMemory", retPtr);

    return (retPtr);
}


/* Return the number of offsetBits bits */
UInt32 SharedRegion_getNumOffsetBits (Void)
{
    UInt32    numEntries = SharedRegion_module->cfg.numEntries;
    UInt32    indexBits  = 0;
    UInt32    numOffsetBits = 0;

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_getNumOffsetBits");

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getNumOffsetBits",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (numEntries == 0) {
            indexBits = 0;
        }
        else if (numEntries == 1) {
            indexBits = 1;
        }
        else {
            numEntries = numEntries - 1;

            /* determine the number of bits for the index */
            while (numEntries) {
                indexBits++;
                numEntries = numEntries >> 1;
            }
        }

        numOffsetBits = 32 - indexBits;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_getNumOffsetBits",
               numOffsetBits);

    return (numOffsetBits);
}


/* Checks to make sure overlap does not exists. */
Int
SharedRegion_checkOverlap (Ptr base, UInt32 len)
{
    Int                   status = SharedRegion_S_SUCCESS;
    SharedRegion_Region * region = NULL;
    IArg                  key    = 0;
    UInt32                i;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_checkOverlap", base, len);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_checkOverlap",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = IGateProvider_enter (SharedRegion_module->localLock);

        /* check whether new region overlaps existing ones */
        for (i = 0; i < SharedRegion_module->cfg.numEntries; i++) {
            region = &(SharedRegion_module->regions [i]);
            if (region->entry.isValid) {
                if (base >= region->entry.base) {
                    if (  base
                        < (Ptr)(    (UInt32) region->entry.base
                                +   region->entry.len)) {
                        status = SharedRegion_E_FAIL;
                        GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "SharedRegion_checkOverlap",
                                         status,
                                         "Specified region falls within another"
                                         " region!");
                        /* Break on failure. */
                        break;
                    }
                }
                else {
                    if ((Ptr) ((UInt32) base + len) > region->entry.base) {
                        status = SharedRegion_E_FAIL;
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "SharedRegion_checkOverlap",
                                             status,
                                             "Specified region spans across"
                                             " multiple regions!");
                        /* Break on failure. */
                        break;
                    }
                }
            }
        }

        IGateProvider_leave (SharedRegion_module->localLock, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "SharedRegion_checkOverlap", status);

    return (status);
}


/* Clears the reserve memory for each region in the table. */
Void
SharedRegion_clearReservedMemory (Void)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                     status = SharedRegion_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    SharedRegionDrv_CmdArgs cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "SharedRegion_clearReservedMemory");

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_clearReservedMemory",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        SharedRegionDrv_ioctl (CMD_SHAREDREGION_CLEARRESERVEDMEMORY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "SharedRegion_clearReservedMemory",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "SharedRegion_clearReservedMemory");
}


/* Return the region info */
Void
SharedRegion_getRegionInfo (UInt16                i,
                            SharedRegion_Region * region)
{
    SharedRegion_Region * regions = NULL;

    GT_2trace (curTrace, GT_ENTER, "SharedRegion_getRegionInfo", i, region);

    GT_assert (curTrace, (i < SharedRegion_module->cfg.numEntries));
    GT_assert (curTrace, (region != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (SharedRegion_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getRegionInfo",
                             SharedRegion_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (i >= SharedRegion_module->cfg.numEntries) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getRegionInfo",
                             SharedRegion_E_INVALIDARG,
                             "Id cannot be larger than numEntries!");
    }
    else if (region == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "SharedRegion_getRegionInfo",
                             SharedRegion_E_INVALIDARG,
                             "Invalid NULL region provided!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        regions = &(SharedRegion_module->regions[i]);

        Memory_copy ((Ptr) region,
                     (Ptr) &regions->entry,
                     sizeof (SharedRegion_Region));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "SharedRegion_getRegionInfo");
}

/* Sets the regions in user space that are created in knl space and
 * not on user space
 */
Int
_SharedRegion_setRegions (Void)
{
    Int                 status = SharedRegion_S_SUCCESS;
    UInt32              i;
    SharedRegion_Region *   regions = NULL;
    SharedRegionDrv_CmdArgs cmdArgs;
    Memory_MapInfo          mapInfo;

    cmdArgs.args.getRegionInfo.regions = (SharedRegion_Region *)
                                      Memory_alloc (NULL,
                                      (  sizeof (SharedRegion_Region)
                                       * SharedRegion_module->cfg.numEntries),
                                      0);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cmdArgs.args.getRegionInfo.regions == NULL) {
        status = SharedRegion_E_MEMORY;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "_SharedRegion_setRegions",
                         status,
                         "Failed to allocate memory for regions!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        SharedRegionDrv_ioctl (CMD_SHAREDREGION_GETREGIONINFO, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "_SharedRegion_setRegionsInfo",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            for (i = 0u;
                (   (i < SharedRegion_module->cfg.numEntries) && (status >= 0));
                i++) {
                regions = &(cmdArgs.args.getRegionInfo.regions [i]);
                if (regions->entry.isValid == TRUE) {
                    if (SharedRegion_module->regions[i].entry.isValid != TRUE) {
                        mapInfo.src  = (UInt32) regions->entry.base;
                        mapInfo.size = regions->entry.len;
                        mapInfo.isCached = FALSE;
                        status = Memory_map (&mapInfo);
                        if (status < 0) {
                            GT_setFailureReason (curTrace,
                                                 GT_4CLASS,
                                                 "_SharedRegion_setRegions",
                                                 status,
                                                 "Memory_map failed!");
                        }
                        else {
                            Memory_copy(&SharedRegion_module->regions[i],
                                        &cmdArgs.args.getRegionInfo.regions [i],
                                        sizeof (SharedRegion_Region));
                            SharedRegion_module->regions[i].entry.base =
                                       (Ptr) mapInfo.dst;
                            SharedRegion_module->regions[i].entry.isValid = TRUE;
                            SharedRegion_module->bCreatedInKnlSpace[i] = TRUE;

                            /* Not Opening heapMem  instances for the regions
                             * that  have createHeap flag set to TRUE.
                             * SharedRegion_getHeap will actually open the heap.
                             */
                        }
                    }
                    else {
                        /* TBD: Cross check the alreay existing entry  contens
                         * with the entry contens that are obtained from knl
                         * space . Flash assert if there is discrapency.
                         */
                    }
                }
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        Memory_free ( NULL,
                      cmdArgs.args.getRegionInfo.regions,
                      ( sizeof (SharedRegion_Region)
                       * SharedRegion_module->cfg.numEntries));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "_SharedRegion_setRegions");

    return status;
}

/* Clears the regions in user space that are created in knl space and
 * not on user space.
 */
Int
_SharedRegion_clearRegions ()
{
    Int                 status = SharedRegion_S_SUCCESS;
    UInt32              i;
    Memory_UnmapInfo    unmapInfo;
    SharedRegion_Region *regions;

    for (i = 0;
        (   (i < SharedRegion_module->cfg.numEntries) && (status >= 0));
        i++) {
        regions = &(SharedRegion_module->regions[i]);
        if (   (regions->entry.isValid == TRUE)
            && (SharedRegion_module->bCreatedInKnlSpace[i] == TRUE)) {
//            Gate_enterSystem();
            SharedRegion_module->regions[i].entry.isValid = FALSE;
            SharedRegion_module->bCreatedInKnlSpace[i] = FALSE;

//            Gate_leaveSystem();

            unmapInfo.addr  = (UInt32) regions->entry.base;
            unmapInfo.size = regions->entry.len;
            unmapInfo.isCached = FALSE;
            status = Memory_unmap (&unmapInfo);
            if (status < 0) {
                status = SharedRegion_E_FAIL;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "_SharedRegion_clearRegions",
                                     status,
                                     "Memory_map failed!");
            }
            else {
                    regions->entry.base = NULL;
            }
        }

    }
    GT_0trace (curTrace, GT_LEAVE, "_SharedRegion_clearRegions");

    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
