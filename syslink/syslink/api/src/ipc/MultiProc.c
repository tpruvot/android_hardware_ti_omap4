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
 *  @file   MultiProc.c
 *
 *  @brief  Handles processor id management in multi processor systems.Used
 *          by all modules which need processor ids for their oprations.
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <String.h>
#include <Memory.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>
#include <MultiProcDrvDefs.h>
#include <MultiProcDrv.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  MultiProc Module state object
 */
typedef struct MultiProc_ModuleObject_tag {
    MultiProc_Config cfg;
    /*!< Notify configuration structure */
    MultiProc_Config defCfg;
    /*!< Default module configuration */
    UInt32           refCount;
    /* Reference count */
    UInt16           id;
    /* Local Processor ID */
} MultiProc_ModuleObject;


/* =============================================================================
 *  Extern declarations
 * =============================================================================
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
MultiProc_ModuleObject MultiProc_state = {
    .refCount = 0
};

/*!
 *  @var    MultiProc_module
 *
 *  @brief  Pointer to the MultiProc module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
MultiProc_ModuleObject * MultiProc_module = &MultiProc_state;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Function to get the default configuration for the MultiProc module. */
Void
MultiProc_getConfig (MultiProc_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                  status = MultiProc_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    MultiProcDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MultiProc_getConfig",
                             MultiProc_E_FAIL,
                             "Argument of type (MultiProc_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (MultiProc_module->refCount == 0) {
            /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            MultiProcDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MultiProc_getConfig",
                                     status,
                                     "Failed to open driver handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                cmdArgs.args.getConfig.config = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                MultiProcDrv_ioctl (CMD_MULTIPROC_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "MultiProc_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Close the driver handle. */
            MultiProcDrv_close ();
        }
        else {
            Memory_copy (cfg,
                         &MultiProc_module->cfg,
                         sizeof (MultiProc_Config));
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MultiProc_getConfig");
}


/* Function to setup the MultiProc Module */
Int
MultiProc_setup (MultiProc_Config * cfg)
{
    Int                  status = MultiProc_S_SUCCESS;
    MultiProcDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_setup", cfg);

    if (MultiProc_module->refCount == 0) {
        /* Open the driver handle. */
        status = MultiProcDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MultiProc_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.setup.config = (MultiProc_Config *) cfg;
            status = MultiProcDrv_ioctl (CMD_MULTIPROC_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MultiProc_setup",
                                     status,
                                     "API (through IOCTL) failed"
                                     " on kernel-side!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                Memory_copy (&MultiProc_module->cfg,
                             cfg,
                             sizeof (MultiProc_Config));
                MultiProc_module->id = cfg->id;
                MultiProc_module->refCount++;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }
    else {
        MultiProc_module->refCount++;
    }

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_setup", status);

    /*! @retval MultiProc_S_SUCCESS Operation successful */
    return (status);
}


/* Function to destroy the MultiProc module */
Int
MultiProc_destroy (Void)
{
    Int                  status = MultiProc_S_SUCCESS;
    MultiProcDrv_CmdArgs cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "MultiProc_destroy");

    /* TBD: Protect from multiple threads. */
    MultiProc_module->refCount--;
    if (MultiProc_module->refCount == 0) {
        status = MultiProcDrv_ioctl (CMD_MULTIPROC_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MultiProc_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        MultiProcDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_destroy", status);

    /*! @retval MultiProc_S_SUCCESS Operation successful */
    return (status);
}


/* Function to set Local processor Id */
Int
MultiProc_setLocalId (UInt16 id)
{
    Int                   status = MultiProc_S_SUCCESS;
    MultiProcDrv_CmdArgs  cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_setLocalId", id);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MultiProc_module->refCount < 1) {
        status = MultiProc_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MultiProc_setLocalId",
                             status,
                             "Module is in invalidstate!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.setLocalId.id = id;
        status = MultiProcDrv_ioctl (CMD_MULTIPROC_SETLOCALID, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MultiProc_setLocalId",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else if (id >= MultiProc_module->cfg.numProcessors) {
            status = MultiProc_E_INVALIDARG;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MultiProc_setLocalId",
                                 MultiProc_E_INVALIDARG,
                                 "ID cannot be greater than or equal to"
                                 " numProcessors");
        }
        else if (MultiProc_module->id != MultiProc_INVALIDID) {
            status = MultiProc_E_INVALIDARG;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "MultiProc_setLocalId",
                                 MultiProc_E_INVALIDARG,
                                 "Cannot change the ID after module"
                                 " startup");
        }
        else{
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            MultiProc_module->id= id;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_setLocalId", status);

    return (status);
}


/* Function to get proccesor Id from proccessor name. */
UInt16
MultiProc_getId (String name)
{
    Bool   found = FALSE;
    UInt32 id    = MultiProc_INVALIDID;
    Int    i;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_getId", name);

    GT_assert(curTrace, (MultiProc_module->refCount > 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MultiProc_module->refCount < 1) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MultiProc_getId",
                             MultiProc_E_INVALIDSTATE,
                             "Module is in invalidstate!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* If the name is NULL, simply return the local id */
        if (name == NULL) {
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (MultiProc_module->id== MultiProc_INVALIDID) {
                GT_setFailureReason (curTrace,
                                   GT_4CLASS,
                                   "MultiProc_getId",
                                   MultiProc_E_FAIL,
                                   "MultiProc_localId not set to proper value");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                GT_assert (curTrace,
                           (MultiProc_module->id!= MultiProc_INVALIDID));
                id = MultiProc_module->id;
                found = TRUE;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
        else {
            for (i = 0; i < MultiProc_module->cfg.numProcessors ; i++) {
                if (   String_cmp (name, &MultiProc_module->cfg.nameList [i][0])
                    == 0) {
                    id = i;
                    found = TRUE;
                    break;
                }
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (!found) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "MultiProc_getId",
                                     MultiProc_E_NOTFOUND,
                                     "Processor name not found");
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_getId", id);

    return (id);
}

/* Function to get name from processor id. */
String
MultiProc_getName (UInt16 id)
{
    String name = NULL;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_getName", id);

    GT_assert (curTrace, (id < MultiProc_module->cfg.numProcessors));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MultiProc_module->refCount < 1) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MultiProc_getName",
                             MultiProc_E_INVALIDSTATE,
                             "Module is in invalidstate!");
    }
    else if (id >= MultiProc_module->cfg.numProcessors) {
        GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "MultiProc_getName",
                    MultiProc_E_FAIL,
                    "Processor id is not less than MultiProc_MAXPROCESSORS");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        name = MultiProc_module->cfg.nameList [id];
        GT_1trace (curTrace, GT_1CLASS, "    MultiProc_getName [%s]", name);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "MultiProc_getName");

    return (name);
}


/* Function to get maximum proc Id in the system. */
inline UInt16
MultiProc_getNumProcessors (Void)
{
    GT_0trace (curTrace, GT_ENTER, "MultiProc_getNumProcessors");
    GT_1trace (curTrace,
               GT_LEAVE,
               "MultiProc_getNumProcessors",
               MultiProc_module->cfg.numProcessors);

    /* Don't put any checks here, since this needs to be very fast. */

    return (MultiProc_module->cfg.numProcessors);
}


/* Return Id of current processor */
inline UInt16
MultiProc_self (Void)
{
    GT_0trace (curTrace, GT_ENTER, "MultiProc_self");

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_self", MultiProc_module->id);

    /* Don't put any checks here, since this needs to be very fast. */

    return (MultiProc_module->id);
}


/* Determines the offset for any two processors. */
UInt
MultiProc_getSlot (UInt16 remoteProcId)
{
    UInt slot = 0u;
    UInt i;
    UInt j;
    UInt smallId;
    UInt largeId;

    GT_1trace (curTrace, GT_ENTER, "MultiProc_getSlot", remoteProcId);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (MultiProc_module->refCount < 1) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "MultiProc_getSlot",
                             MultiProc_E_INVALIDSTATE,
                             "Module is in invalidstate!");
    }
    else if (remoteProcId >= MultiProc_module->cfg.numProcessors) {
        GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "MultiProc_getSlot",
                    MultiProc_E_INVALIDARG,
                    "Processor id is not less than numProcessors");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (remoteProcId > MultiProc_self ()) {
            smallId = MultiProc_self ();
            largeId = remoteProcId;
        }
        else {
            largeId = MultiProc_self ();
            smallId = remoteProcId;
        }

        /* determine what offset to create for the remote Proc Id */
        for (i = 0; i < MultiProc_module->cfg.numProcessors; i++) {
            for (j = i + 1; j < MultiProc_module->cfg.numProcessors; j++) {
                if ((smallId == i) && (largeId == j)) {
                    break;
                }
                slot++;
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "MultiProc_getSlot", slot);

    return (slot);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
