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
 *  @file   ProcMgr.c
 *
 *  @brief      The Processor Manager on a master processor provides control
 *              functionality for a slave device.
 *              The ProcMgr module provides the following services for the
 *              slave processor:
 *              - Slave processor boot-loading
 *              - Read from or write to slave processor memory
 *              - Slave processor power management
 *              - Slave processor error handling
 *              - Dynamic Memory Mapping
 *              The Device Manager (Processor module) shall have interfaces for:
 *              - Loader: There may be multiple implementations of the Loader
 *                        interface within a single Processor instance.
 *                        For example, COFF, ELF, dynamic loader, custom types
 *                        of loaders may be written and plugged in.
 *              - Power Manager: The Power Manager implementation can be a
 *                        separate module that is plugged into the Processor
 *                        module. This allows the Processor code to remain
 *                        generic, and the Power Manager may be written and
 *                        maintained either by a separate team, or by customer.
 *              - Processor: The implementation of this interface provides all
 *                        other functionality for the slave processor, including
 *                        setup and initialization of the Processor module,
 *                        management of slave processor MMU (if available),
 *                        functions to write to and read from slave memory etc.
 *              All processors in the system shall be identified by unique
 *              processor ID. The management of this processor ID is done by the
 *              MultiProc module.
 *
 *              Processor state machine:
 *
 *                      attach
 *              Unknown -------> Powered
 *                 ^  ^           |
 *                 |   \          |
 *           slave |    \ detach  V
 *           error \     ------- Reset <-----
 *                 ^              |          \
 *                 |              | load      \
 *           slave |              V           |stop
 *           error \             Loaded       |
 *            or    \             |           |
 *            crash  \            | start     /
 *                    \           V          /
 *                     -------- Running ----
 *                               /   ^
 *                              /     \
 *                              |     |
 *                    Pwr state |     | Pwr state
 *                     change   |     |  change
 *                              \     /
 *                              V    /
 *                            Unavailable
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
#include <asm/unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/eventfd.h>
/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <Trace.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>
#include <ProcMgr.h>
#include <ProcMgrDrvDefs.h>
#include <ProcMgrDrvUsr.h>
#include <_ProcMgrDefs.h>
#include <dload4430.h>
#include <ArrayList.h>
#include <dload_api.h>
#include <load.h>
#include <_Ipc.h>
#include <ProcMMU.h>
#include <ProcMgr.h>
#include <ProcDEH.h>

#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  Checks if a value lies in given range.
 */
#define IS_RANGE_VALID(x,min,max) (((x) < (max)) && ((x) >= (min)))

#define __PROCMGR_INV_MEMORY    (__ARM_NR_BASE+0x0007fd)
#define __PROCMGR_CLEAN_MEMORY  (__ARM_NR_BASE+0x0007fe)
#define __PROCMGR_FLUSH_MEMORY  (__ARM_NR_BASE+0x0007ff)

/*!
 *  @brief  Symbol name for Ipc Synchronization section
 */
#define RESETVECTOR_SYMBOL          "_Ipc_ResetVector"

/*!
 *  @brief  ProcMgr Module state object
 */
typedef struct ProcMgr_ModuleObject_tag {
    Int32          setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
    ProcMgr_Handle procHandles [MultiProc_MAXPROCESSORS];
    /*!< Array of Handles of ProcMgr instances */
} ProcMgr_ModuleObject;

/*!
 *  @brief  ProcMgr instance object
 */
typedef struct ProcMgr_Object_tag {
    Ptr              knlObject;
    /*!< Pointer to the kernel-side ProcMgr object. */
    UInt32           openRefCount;
    /*!< Reference count for number of times open/close were called in this
         process. */
    Bool             created;
    /*!< Indicates whether the object was created in this process. */
    UInt16           procId;
    /*!< Processor ID */
    UInt16           numMemEntries;
    /*!< Number of valid memory entries */
    UInt16           numOpenMemEntries;
    /*!< Number of memory entries retrieved during open call */
    UInt16           numAttachMemEntries;
    /*!< Number of memory entries retrieved during attach call */
    ProcMgr_AddrInfo memEntries [PROCMGR_MAX_MEMORY_REGIONS];
    /*!< Configuration of memory regions */
    DLoad4430_Handle loaderHandle;
} ProcMgr_Object;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    ProcMgr_state
 *
 *  @brief  ProcMgr state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
ProcMgr_ModuleObject ProcMgr_state =
{
    .setupRefCount = 0
};

/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Function to get the default configuration for the ProcMgr
 *              module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to ProcMgr_setup filled in by the
 *              ProcMgr module with the default parameters. If the user does
 *              not wish to make any change in the default parameters, this API
 *              is not required to be called.
 *
 *  @param      cfg        Pointer to the ProcMgr module configuration structure
 *                         in which the default config is to be returned.
 *
 *  @sa         ProcMgr_setup, ProcMgrDrvUsr_open, ProcMgrDrvUsr_ioctl,
 *              ProcMgrDrvUsr_close
 */
Void
ProcMgr_getConfig (ProcMgr_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                         status = PROCMGR_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ProcMgr_CmdArgsGetConfig    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getConfig",
                             PROCMGR_E_INVALIDARG,
                             "Argument of type (ProcMgr_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ProcMgrDrvUsr_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.cfg = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        ProcMgrDrvUsr_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ProcMgr_getConfig");
}


/*!
 *  @brief      Function to setup the ProcMgr module.
 *
 *              This function sets up the ProcMgr module. This function must
 *              be called before any other instance-level APIs can be invoked.
 *              Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then ProcMgr_getConfig can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. If the user
 *              does not wish to make any change in the default parameters, the
 *              application can simply call ProcMgr_setup with NULL parameters.
 *              The default parameters would get automatically used.
 *
 *  @param      cfg   Optional ProcMgr module configuration. If provided as
 *                    NULL, default configuration is used.
 *
 *  @sa         ProcMgr_destroy, ProcMgrDrvUsr_open, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_setup (ProcMgr_Config * cfg)
{
    Int                     status = PROCMGR_SUCCESS;
    ProcMgr_CmdArgsSetup    cmdArgs;
    Int                     i;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_setup", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_setup",
                             status,
                             "Argument of type (ProcMgr_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Protect from multiple threads. */
        ProcMgr_state.setupRefCount++;
        /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
        if (ProcMgr_state.setupRefCount > 1) {
            /*! @retval PROCMGR_S_ALREADYSETUP Success: ProcMgr module has been
              already setup in this process */
            status = PROCMGR_S_ALREADYSETUP;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    ProcMgr_setup: ProcMgr module has been already setup "
                       "in this process.\n"
                       "        RefCount: [%d]\n",
                       (ProcMgr_state.setupRefCount - 1));
        }
        else {
            /* Set all handles to NULL -- in order for destroy() to work */
            for (i = 0 ; i < MultiProc_MAXPROCESSORS ; i++) {
                ProcMgr_state.procHandles [i] = NULL;
            }

            /* Open the driver handle. */
            status = ProcMgrDrvUsr_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_setup",
                                     status,
                                     "Failed to open driver handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                cmdArgs.cfg = cfg;
                status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                         GT_4CLASS,
                                         "ProcMgr_setup",
                                         status,
                                         "API (through IOCTL) failed on kernel-side!");
                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_setup", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to destroy the ProcMgr module.
 *
 *              Once this function is called, other ProcMgr module APIs, except
 *              for the ProcMgr_getConfig API cannot be called anymore.
 *
 *  @sa         ProcMgr_setup, ProcMgrDrvUsr_ioctl, ProcMgrDrvUsr_close
 */
Int
ProcMgr_destroy (Void)
{
    Int                     status = PROCMGR_SUCCESS;
    ProcMgr_CmdArgsDestroy  cmdArgs;
    UInt16                  i;

    GT_0trace (curTrace, GT_ENTER, "ProcMgr_destroy");

    /* TBD: Protect from multiple threads. */
    ProcMgr_state.setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (ProcMgr_state.setupRefCount < 0) {
        status = PROCMGR_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_destroy",
                             status,
                             "ProcMgr module not setup");

    }
    else if (ProcMgr_state.setupRefCount >= 1) {
        /*! @retval PROCMGR_S_SETUP Success: ProcMgr module has been setup
                                             by other clients in this process */
        status = PROCMGR_S_SETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "ProcMgr module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (ProcMgr_state.setupRefCount + 1));
    }
    else {
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Check if any ProcMgr instances have not been deleted so far. If not,
         * delete them.
         */
        for (i = 0 ; i < MultiProc_MAXPROCESSORS ; i++) {
            GT_assert (curTrace, (ProcMgr_state.procHandles [i] == NULL));
            if (ProcMgr_state.procHandles [i] != NULL) {
                ProcMgr_delete (&(ProcMgr_state.procHandles [i]));
            }
        }

        /* Close the driver handle. */
        ProcMgrDrvUsr_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_destroy", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to initialize the parameters for the ProcMgr instance.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to ProcMgr_create filled in by the
 *              ProcMgr module with the default parameters.
 *
 *  @param      handle   Handle to the ProcMgr object. If specified as NULL,
 *                       the default global configuration values are returned.
 *  @param      params   Pointer to the ProcMgr instance params structure in
 *                       which the default params is to be returned.
 *
 *  @sa         ProcMgr_create, ProcMgrDrvUsr_ioctl
 */
Void
ProcMgr_Params_init (ProcMgr_Handle handle, ProcMgr_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                         status          = PROCMGR_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ProcMgr_Object *            procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsParamsInit   cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_Params_init", handle, params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (params == NULL) {
        /* No retVal comment since this is a Void function. */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_Params_init",
                             status,
                             "Argument of type (ProcMgr_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Check if the handle is NULL and pass it in directly to kernel-side in
         * that case. Otherwise send the kernel object pointer.
         */
        if (procMgrHandle == NULL) {
            cmdArgs.handle = handle;
        }
        else {
            cmdArgs.handle = procMgrHandle->knlObject;
        }
        cmdArgs.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ProcMgrDrvUsr_ioctl (CMD_PROCMGR_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ProcMgr_Params_init");
}


/*!
 *  @brief      Function to create a ProcMgr object for a specific slave
 *              processor.
 *
 *              This function creates an instance of the ProcMgr module and
 *              returns an instance handle, which is used to access the
 *              specified slave processor. The processor ID specified here is
 *              the ID of the slave processor as configured with the MultiProc
 *              module.
 *              Instance-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then ProcMgr_Params_init can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. For this
 *              API, the params argument is not optional, since the user needs
 *              to provide some essential values such as loader, PwrMgr and
 *              Processor instances to be used with this ProcMgr instance.
 *
 *  @param      procId   Processor ID represented by this ProcMgr instance
 *  @param      params   ProcMgr instance configuration parameters.
 *
 *  @sa         ProcMgr_delete, Memory_calloc, ProcMgrDrvUsr_ioctl
 */
ProcMgr_Handle
ProcMgr_create (UInt16 procId, const ProcMgr_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                     status = PROCMGR_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ProcMgr_Object *        handle = NULL;
    /* TBD: UInt32                  key;*/
    ProcMgr_CmdArgsCreate   cmdArgs;
    DLoad_Config            loadCfg;
    DLoad_Params            loadParams;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_create", procId, params);

    GT_assert (curTrace, MultiProc_isValidRemoteProc (procId));
    GT_assert (curTrace, (params != NULL));
    GT_assert (curTrace, ((params != NULL)) && (params->procHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!MultiProc_isValidRemoteProc (procId)) {
        /*! @retval NULL Invalid procId specified */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_create",
                             status,
                             "Invalid procId specified");
    }
    else if (params == NULL) {
        /*! @retval NULL Invalid NULL params pointer specified */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_create",
                             status,
                             "Invalid NULL params pointer specified");
    }
    else if (params->procHandle == NULL) {
        /*! @retval NULL Invalid NULL procHandle specified in params */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_create",
                             status,
                             "Invalid NULL procHandle specified in params");
    }

    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */
        if (ProcMgr_state.procHandles [procId] != NULL) {
            /* If the object is already created/opened in this process, return
             * handle in the local array.
             */
            handle = (ProcMgr_Object *) ProcMgr_state.procHandles [procId];
            handle->openRefCount++;
            GT_1trace (curTrace,
                       GT_2CLASS,
                       "    ProcMgr_create: Instance already exists in this"
                       " process space"
                       "        RefCount [%d]\n",
                       (handle->openRefCount - 1));
        }
        else {
            cmdArgs.procId = procId;
            /* Get the kernel objects of Processor, Loader and PwrMgr modules,
             * and pass them to the kernel-side.
             */
            cmdArgs.params.procHandle = ((ProcMgr_CommonObject *)
                                             (params->procHandle))->knlObject;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            ProcMgrDrvUsr_ioctl (CMD_PROCMGR_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_create",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Allocate memory for the handle */
                handle = (ProcMgr_Object *) Memory_calloc (NULL,
                                                        sizeof (ProcMgr_Object),
                                                        0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (handle == NULL) {
                    /*! @retval NULL Memory allocation failed for handle */
                    status = PROCMGR_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "ProcMgr_create",
                                        status,
                                        "Memory allocation failed for handle!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    /* Set pointer to kernel object into the user handle. */
                    handle->knlObject = cmdArgs.handle;
                    /* Indicate that the object was created in this process. */
                    handle->created = TRUE;
                    handle->procId = procId;

                    /* Setup Loader handle for this instance. */
                    DLoad4430_getConfig (&loadCfg);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                    status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    DLoad4430_setup (&loadCfg);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                    if (status < 0) {
                        status = PROCMGR_E_FAIL;
                        GT_setFailureReason (curTrace,
                                    GT_4CLASS,
                                    "ProcMgr_create",
                                    status,
                                    "DLoad4430_setup failed");
                    }
                    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                        handle->loaderHandle = DLoad4430_create (procId,
                                                                 &loadParams);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                        if (handle->loaderHandle == NULL) {
                            DLoad4430_destroy ();
                            status = PROCMGR_E_FAIL;
                            GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "ProcMgr_create",
                                        status,
                                        "DLoad4430_create failed");
                        }
                        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                            /* Store the ProcMgr handle in the local
                             * array.
                             */
                            ProcMgr_state.procHandles [procId] =
                                                     (ProcMgr_Handle)handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                        }
                    }
                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_create", handle);

    /*! @retval Valid Handle Operation successful */
    return (ProcMgr_Handle) handle;
}


/*!
 *  @brief      Function to delete a ProcMgr object for a specific slave
 *              processor.
 *
 *              Once this function is called, other ProcMgr instance level APIs
 *              that require the instance handle cannot be called.
 *
 *  @param      handlePtr   Pointer to Handle to the ProcMgr object
 *
 *  @sa         ProcMgr_create, Memory_free, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_delete (ProcMgr_Handle * handlePtr)
{
    Int                     status    = PROCMGR_SUCCESS;
    Int                     tmpStatus = PROCMGR_SUCCESS;
    ProcMgr_Object *        handle;
    /* TBD: UInt32          key;*/
    ProcMgr_CmdArgsDelete   cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval PROCMGR_E_INVALIDARG Invalid NULL handlePtr specified*/
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_delete",
                             status,
                             "Invalid NULL handlePtr specified");
    }
    else if (*handlePtr == NULL) {
        /*! @retval PROCMGR_E_HANDLE Invalid NULL *handlePtr specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_delete",
                             status,
                             "Invalid NULL *handlePtr specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        handle = (ProcMgr_Object *) (*handlePtr);
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */

        if (handle->openRefCount != 0) {
            /* There are still some open handles to this ProcMgr.
             * Give a warning, but still go ahead and delete the object.
             */
            status = PROCMGR_S_OPENHANDLE;
            GT_assert (curTrace, (handle->openRefCount != 0));
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    ProcMgr_delete: Warning, some handles are"
                       " still open!\n"
                       "        RefCount: [%d]\n",
                       handle->openRefCount);
        }

        if (handle->created == FALSE) {
            /*! @retval PROCMGR_E_ACCESSDENIED The ProcMgr object was not
                   created in this process and access is denied to delete it. */
            status = PROCMGR_E_ACCESSDENIED;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       " ProcMgr_delete: Warning, the ProcMgr object was not"
                       " created in this process and access is denied to"
                       " delete it.\n",
                       status);
        }

        if (status >= 0) {
            status = DLoad4430_delete (&handle->loaderHandle);
            status = DLoad4430_destroy ();

            /* Only delete the object if it was created in this process. */
            cmdArgs.handle = handle->knlObject;
            tmpStatus = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_DELETE, &cmdArgs);
            if (tmpStatus < 0) {
                /* Only override the status if kernel call failed. Otherwise
                 * we want the status from above to carry forward.
                 */
                status = tmpStatus;
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_delete",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
                /* Clear the ProcMgr handle in the local array. */
                GT_assert (curTrace,(handle->procId < MultiProc_MAXPROCESSORS));
                ProcMgr_state.procHandles [handle->procId] = NULL;
                Memory_free (NULL, handle, sizeof (ProcMgr_Object));
                *handlePtr = NULL;
            }
        }

        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_delete", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to open a handle to an existing ProcMgr object handling
 *              the procId.
 *
 *              This function returns a handle to an existing ProcMgr instance
 *              created for this procId. It enables other entities to access
 *              and use this ProcMgr instance.
 *
 *  @param      handlePtr   Return Parameter: Handle to the ProcMgr instance
 *  @param      procId      Processor ID represented by this ProcMgr instance
 *
 *  @sa         ProcMgr_close, Memory_calloc, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_open (ProcMgr_Handle * handlePtr, UInt16 procId)
{
    Int                     status = PROCMGR_SUCCESS;
    ProcMgr_Object *        handle = NULL;
    /* UInt32           key; */
    ProcMgr_CmdArgsOpen     cmdArgs;
    DLoad_Config            loadCfg;
    DLoad_Params            loadParams;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_open", handlePtr, procId);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, MultiProc_isValidRemoteProc (procId));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval PROCMGR_E_INVALIDARG Invalid NULL handle pointer specified*/
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_open",
                             status,
                             "Invalid NULL handle pointer specified");
    }
    else if (!MultiProc_isValidRemoteProc (procId)) {
        *handlePtr = NULL;
        /*! @retval PROCMGR_E_INVALIDARG Invalid procId specified */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_open",
                             status,
                             "Invalid procId specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */
        if (ProcMgr_state.procHandles [procId] != NULL) {
            /* If the object is already created/opened in this process, return
             * handle in the local array.
             */
            handle = (ProcMgr_Object *) ProcMgr_state.procHandles [procId];
            handle->openRefCount++;
            status = PROCMGR_S_ALREADYEXISTS;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    ProcMgr_open: Instance already exists in this"
                       " process space"
                       "        RefCount [%d]\n",
                       (handle->openRefCount - 1));
        }
        else {
            /* The object was not created/opened in this process. Need to drop
             * down to the kernel to get the object instance.
             */
            cmdArgs.procId = procId;
            cmdArgs.handle = NULL; /* Return parameter */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            ProcMgrDrvUsr_ioctl (CMD_PROCMGR_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_open",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Allocate memory for the handle */
                handle = (ProcMgr_Object *) Memory_calloc (NULL,
                                                        sizeof (ProcMgr_Object),
                                                        0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (handle == NULL) {
                    /*! @retval PROCMGR_E_MEMORY Memory allocation failed for
                                                 handle */
                    status = PROCMGR_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "ProcMgr_open",
                                        status,
                                        "Memory allocation failed for handle!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    /* Set pointer to kernel object into the user handle. */
                    handle->knlObject = cmdArgs.handle;
                    /* Store the ProcMgr handle in the local array. */
                    ProcMgr_state.procHandles [procId] = (ProcMgr_Handle)handle;
                    handle->openRefCount = 1;
                    handle->procId = procId;
                    handle->created = FALSE;

                    /* Store the memory information received, only if the Proc
                     * has been attached-to already, which will create the
                     * mappings on kernel-side.
                     */
                    if (cmdArgs.procInfo.numMemEntries != 0) {
                        handle->numMemEntries = cmdArgs.procInfo.numMemEntries;
                        handle->numOpenMemEntries =
                                                cmdArgs.procInfo.numMemEntries;
                        Memory_copy (&(handle->memEntries),
                                     &(cmdArgs.procInfo.memEntries),
                                     sizeof (handle->memEntries));
                    }

                    /* Setup Loader handle for this instance. */
                    DLoad4430_getConfig (&loadCfg);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                    status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    DLoad4430_setup (&loadCfg);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                    if (status < 0) {
                        status = PROCMGR_E_FAIL;
                        GT_setFailureReason (curTrace,
                                    GT_4CLASS,
                                    "ProcMgr_create",
                                    status,
                                    "DLoad4430_setup failed");
                    }
                    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                        handle->loaderHandle = DLoad4430_create (procId,
                                                                 &loadParams);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                        if (handle->loaderHandle == NULL) {
                            DLoad4430_destroy ();
                            status = PROCMGR_E_FAIL;
                            GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "ProcMgr_create",
                                        status,
                                        "DLoad4430_create failed");
                        }
                    }
                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }

        *handlePtr = (ProcMgr_Handle) handle;
        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */

        /* Open handle to MMU */
        status = ProcMMU_open (procId);
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_open",
                                 status,
                                 "ProcMMU_open failed!");
        }

        /* Open handle to DEH */
        status = ProcDEH_open (procId);
        if (status < 0) {
             GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_open",
                                  status,
                                  "ProcDEH_open failed!");
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_open", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to close this handle to the ProcMgr instance.
 *
 *              This function closes the handle to the ProcMgr instance
 *              obtained through ProcMgr_open call made earlier.
 *
 *  @param      handle     Handle to the ProcMgr object
 *
 *  @sa         ProcMgr_open, Memory_free, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_close (ProcMgr_Handle * handlePtr)
{
    Int                     status = PROCMGR_SUCCESS;
    UInt16                  procId = MultiProc_INVALIDID;
    ProcMgr_Object *        procMgrHandle;
    ProcMgr_CmdArgsClose    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_close", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval PROCMGR_E_INVALIDARG Invalid NULL handlePtr specified */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_delete",
                             status,
                             "Invalid NULL handlePtr specified");
    }
    else if (*handlePtr == NULL) {
        /*! @retval PROCMGR_E_HANDLE Invalid NULL *handlePtr specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_delete",
                             status,
                             "Invalid NULL *handlePtr specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        procMgrHandle = (ProcMgr_Object *) (*handlePtr);
        procId = procMgrHandle->procId;
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */
        if (procMgrHandle->openRefCount == 0) {
            /*! @retval PROCMGR_E_ACCESSDENIED All open handles to this ProcMgr
                                               object are already closed. */
            status = PROCMGR_E_ACCESSDENIED;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_close",
                                 status,
                                 "All open handles to this ProcMgr object are"
                                 " already closed.");

        }
        else if (procMgrHandle->openRefCount > 1) {
            /* Simply reduce the reference count. There are other threads in
             * this process that have also opened handles to this ProcMgr
             * instance.
             */
            procMgrHandle->openRefCount--;
            status = PROCMGR_S_OPENHANDLE;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    ProcMgr_close: Other handles to this instance"
                       " are still open\n"
                       "        RefCount: [%d]\n",
                       (procMgrHandle->openRefCount + 1));
        }
        else {
            /* The object can be closed now since all open handles are closed.*/
            cmdArgs.handle = procMgrHandle->knlObject;
            /* Copy memory information to command arguments. */
            cmdArgs.procInfo.numMemEntries = procMgrHandle->numOpenMemEntries;
            if (cmdArgs.procInfo.numMemEntries != 0) {
                Memory_copy (&(cmdArgs.procInfo.memEntries),
                             &(procMgrHandle->memEntries),
                             sizeof (procMgrHandle->memEntries));
            }
            status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_CLOSE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ProcMgr_close",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Update memory information back. */
            if (cmdArgs.procInfo.numMemEntries != 0) {
                Memory_copy (&(procMgrHandle->memEntries),
                             &(cmdArgs.procInfo.memEntries),
                             sizeof (procMgrHandle->memEntries));
                procMgrHandle->numMemEntries = 0;
            }

            status = DLoad4430_delete (&procMgrHandle->loaderHandle);
            status = DLoad4430_destroy ();

            if (procMgrHandle->created == FALSE) {
                /* Clear the ProcMgr handle in the local array. */
                GT_assert (curTrace,
                           (procMgrHandle->procId < MultiProc_MAXPROCESSORS));
                ProcMgr_state.procHandles [procMgrHandle->procId] = NULL;
                /* Free memory for the handle only if it was not created in
                 * this process.
                 */
                Memory_free (NULL, procMgrHandle, sizeof (ProcMgr_Object));
            }
            *handlePtr = NULL;
        }
        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */

        if (procId != MultiProc_INVALIDID) {
            status = ProcMMU_close (procId);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_close",
                                     status,
                                     "ProcMMU_close failed!");
            }
        }

        if (procId != MultiProc_INVALIDID) {
            status = ProcDEH_close (procId);
             if (status < 0) {
                 GT_setFailureReason (curTrace,
                                      GT_4CLASS,
                                      "ProcMgr_close",
                                      status,
                                      "ProcDEH_close failed!");
            }
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_close", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to initialize the parameters for the ProcMgr attach
 *              function.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to ProcMgr_attach filled in by the
 *              ProcMgr module with the default parameters. If the user does
 *              not wish to make any change in the default parameters, this API
 *              is not required to be called.
 *
 *  @param      handle   Handle to the ProcMgr object. If specified as NULL,
 *                       the default global configuration values are returned.
 *  @param      params   Pointer to the ProcMgr attach params structure in
 *                       which the default params is to be returned.
 *
 *  @sa         ProcMgr_attach, ProcMgrDrvUsr_ioctl
 */
Void
ProcMgr_getAttachParams (ProcMgr_Handle handle, ProcMgr_AttachParams * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                             status          = PROCMGR_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsGetAttachParams  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_getAttachParams", handle, params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getAttachParams",
                             PROCMGR_E_INVALIDARG,
                             "Argument of type (ProcMgr_AttachParams *) passed "
                             "is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* If NULL, send the same to kernel-side. Otherwise translate and send
         * the kernel handle.
         */
        if (procMgrHandle == NULL) {
            cmdArgs.handle = handle;
        }
        else {
            cmdArgs.handle = procMgrHandle->knlObject;
        }
        cmdArgs.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETATTACHPARAMS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getAttachParams",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ProcMgr_getAttachParams");
}


/*!
 *  @brief      Function to attach the client to the specified slave and also
 *              initialize the slave (if required).
 *
 *              This function attaches to an instance of the ProcMgr module and
 *              performs any hardware initialization required to power up the
 *              slave device. This function also performs the required state
 *              transitions for this ProcMgr instance to ensure that the local
 *              object representing the slave device correctly indicates the
 *              state of the slave device. Depending on the slave boot mode
 *              being used, the slave may be powered up, in reset, or even
 *              running state.
 *              Configuration parameters need to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then ProcMgr_getAttachParams can be called to get
 *              the configuration filled with the default values. After this,
 *              only the required configuration values can be changed. If the
 *              user does not wish to make any change in the default parameters,
 *              the application can simply call ProcMgr_attach with NULL
 *              parameters.
 *              The default parameters would get automatically used.
 *
 *  @param      handle   Handle to the ProcMgr object.
 *  @param      params   Optional ProcMgr attach parameters. If provided as
 *                       NULL, default configuration is used.
 *
 *  @sa         ProcMgr_detach, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_attach (ProcMgr_Handle handle, ProcMgr_AttachParams * params)
{
    Int                    status           = PROCMGR_SUCCESS;
    ProcMgr_Object *       procMgrHandle    = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsAttach  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_attach", handle, params);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_attach",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (params == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_attach",
                             status,
                             "Invalid attach params specified");

    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.params = params;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_ATTACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_attach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Store the memory information received. */
            if (cmdArgs.procInfo.numMemEntries != 0) {
                GT_assert (curTrace, (procMgrHandle->numOpenEntries == 0));
                procMgrHandle->numMemEntries = cmdArgs.procInfo.numMemEntries;
                procMgrHandle->numAttachMemEntries =
                                                cmdArgs.procInfo.numMemEntries;
                Memory_copy (&(procMgrHandle->memEntries),
                             &(cmdArgs.procInfo.memEntries),
                             sizeof (procMgrHandle->memEntries));
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_attach", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to detach the client from the specified slave and also
 *              finalze the slave (if required).
 *
 *              This function detaches from an instance of the ProcMgr module
 *              and performs any hardware finalization required to power down
 *              the slave device. This function also performs the required state
 *              transitions for this ProcMgr instance to ensure that the local
 *              object representing the slave device correctly indicates the
 *              state of the slave device. Depending on the slave boot mode
 *              being used, the slave may be powered down, in reset, or left in
 *              its original state.
 *
 *  @param      handle     Handle to the ProcMgr object
 *
 *  @sa         ProcMgr_attach, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_detach (ProcMgr_Handle handle)
{
    Int                    status           = PROCMGR_SUCCESS;
    ProcMgr_Object *       procMgrHandle    = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsDetach  cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_detach", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_detach",
                             status,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        /* Copy memory information. */
        cmdArgs.procInfo.numMemEntries = procMgrHandle->numAttachMemEntries;
        if (cmdArgs.procInfo.numMemEntries != 0) {
            Memory_copy (&(cmdArgs.procInfo.memEntries),
                         &(procMgrHandle->memEntries),
                         sizeof (procMgrHandle->memEntries));
        }
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_DETACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_detach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Update memory information back. */
            if (cmdArgs.procInfo.numMemEntries != 0) {
                Memory_copy (&(procMgrHandle->memEntries),
                             &(cmdArgs.procInfo.memEntries),
                             sizeof (procMgrHandle->memEntries));
                procMgrHandle->numMemEntries = 0;
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_detach", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to load the specified slave executable on the slave
 *              Processor.
 *
 *              This function allows usage of different types of loaders. The
 *              loader specified when creating this instance of the ProcMgr
 *              is used for loading the slave executable. Depending on the type
 *              of loader, the imagePath parameter may point to the path of the
 *              file in the host file system, or it may be NULL. Some loaders
 *              may require specific parameters to be passed. This function
 *              returns a fileId, which can be used for further function calls
 *              that reference a specific file that has been loaded on the
 *              slave processor.
 *
 *  @param      handle     Handle to the ProcMgr object
 *  @param      imagePath  Full file path
 *  @param      argc       Number of arguments
 *  @param      argv       String array of arguments
 *  @param      params     Loader specific parameters
 *  @param      fileId     Return parameter: ID of the loaded file
 *
 *  @sa         ProcMgr_unload, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_load (ProcMgr_Handle handle,
              String         imagePath,
              UInt32         argc,
              String *       argv,
              UInt32 *       entry_point,
              UInt32 *       fileId,
              ProcMgr_ProcId procID)
{
    Int                 status          = PROCMGR_SUCCESS;
    ProcMgr_Object    * procMgrHandle   = (ProcMgr_Object *) handle;

    GT_5trace (curTrace, GT_ENTER, "ProcMgr_load",
               handle, imagePath, argc, argv, procID);
    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (fileId != NULL));
    /* imagePath may be NULL if a non-file based loader is used. In that case,
     * loader-specific params will contain the required information.
     */

    GT_assert (curTrace,
               (   ((argc == 0) && (argv == NULL))
                || ((argc != 0) && (argv != NULL))));

    /*FIXME: (KW) Remove field ID if not used. */
    //cmdArgs.fileId = 0;
    if (procID == MultiProc_getId ("SysM3") ||
            procID == MultiProc_getId ("Tesla")) {
        status = ProcMMU_init (DUCATI_BASEIMAGE_PHYSICAL_ADDRESS, procID);
        if (status < 0) {
            Osal_printf ("Error in ProcMMU_init [0x%x]\n", status);
            goto error_exit;
        }
    }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_load",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (   ((argc == 0) && (argv != NULL))
             || ((argc != 0) && (argv == NULL))) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid argument */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_load",
                             status,
                             "Invalid argc/argv values specified");
    }
    else if (fileId == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid argument */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_load",
                             status,
                             "Invalid fileId pointer specified");
    }
    else if (!MultiProc_isValidRemoteProc (procID)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_load",
                             status,
                             "Invalid procID specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        *fileId = 0; /* Initialize return parameter. */

        status = ProcMgr_setState (ProcMgr_State_Loading);
        if (status != PROCMGR_SUCCESS) {
            status = PROCMGR_E_INVALIDSTATE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_load",
                                 status,
                                 "Not Able to set the state");
        }

        if (status >= 0) {
            status = DLoad4430_load (procMgrHandle->loaderHandle, imagePath,
                                     argc, (char**)(argv),
                                     entry_point, fileId);

            /*---------------------------------------------------------------*/
            /* Did we get a valid program handle back from the loader?       */
            /*---------------------------------------------------------------*/
            if (status != DLOAD_SUCCESS || !(*fileId))
            {
                status = PROCMGR_E_FAIL;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_load",
                                     status,
                                     "DLoad4430_load failed.");
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

error_exit:
    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_load", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to unload the previously loaded file on the slave
 *              processor.
 *
 *              This function unloads the file that was previously loaded on the
 *              slave processor through the ProcMgr_load API. It frees up any
 *              resources that were allocated during ProcMgr_load for this file.
 *              The fileId received from the load function must be passed to
 *              this function.
 *
 *  @param      handle     Handle to the ProcMgr object
 *  @param      fileId     ID of the loaded file to be unloaded
 *
 *  @sa         ProcMgr_load, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_unload (ProcMgr_Handle handle, UInt32 fileId)
{
    Int                    status           = PROCMGR_SUCCESS;
    ProcMgr_Object *       procMgrHandle    = (ProcMgr_Object *) handle;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_unload", handle, fileId);

    GT_assert (curTrace, (handle != NULL));
    /* Cannot check for fileId because it is loader dependent. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_unload",
                             status,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = DLoad4430_unload (procMgrHandle->loaderHandle, fileId);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = PROCMGR_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_unload",
                                 status,
                                 "DLoad4430_unload failed");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_unload", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to initialize the parameters for the ProcMgr start
 *              function.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to ProcMgr_start filled in by the
 *              ProcMgr module with the default parameters. If the user does
 *              not wish to make any change in the default parameters, this API
 *              is not required to be called.
 *
 *  @param      handle   Handle to the ProcMgr object. If specified as NULL,
 *                       the default global configuration values are returned.
 *  @param      params   Pointer to the ProcMgr start params structure in
 *                       which the default params is to be returned.
 *
 *  @sa         ProcMgr_start, ProcMgrDrvUsr_ioctl
 */
Void
ProcMgr_getStartParams (ProcMgr_Handle handle, ProcMgr_StartParams * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                             status          = PROCMGR_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsGetStartParams   cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_getStartParams", handle, params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getStartParams",
                             PROCMGR_E_INVALIDARG,
                             "Argument of type (ProcMgr_StartParams *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETSTARTPARAMS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getStartParams",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ProcMgr_getStartParams");
}


/*!
 *  @brief      Function to start the slave processor running.
 *
 *              Function to start execution of the loaded code on the slave
 *              from the entry point specified in the slave executable loaded
 *              earlier by call to ProcMgr_load ().
 *              After successful completion of this function, the ProcMgr
 *              instance is expected to be in the ProcMgr_State_Running state.
 *
 *  @param      handle   Handle to the ProcMgr object
 *  @param      params   Optional ProcMgr start parameters. If provided as NULL,
 *                       default parameters are used.
 *
 *  @sa         ProcMgr_stop, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_start (ProcMgr_Handle        handle,
               UInt32                entry_point,
               ProcMgr_StartParams * params)
{
    Int                     status          = PROCMGR_SUCCESS;
    ProcMgr_Object *        procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsStart    cmdArgs;
#ifdef SYSLINK_USE_SYSMGR
    UInt32                  start;
#endif
    Memory_MapInfo          sysCtrlMapInfo;
    Memory_UnmapInfo        sysCtrlUnmapInfo;
    UInt32                  numBytes;
    UInt32                  entryPt         = entry_point;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_start", handle, params);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_start",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (params == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_start",
                             status,
                             "Invalid NULL params specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* FIXME: move sysmgr calls from Proc user space */
#ifdef SYSLINK_USE_SYSMGR
        /* read the symbol from slave binary */
        status = ProcMgr_getSymbolAddress (handle,
                                      ((DLoad4430_Object *)
                                      (procMgrHandle->loaderHandle))->fileId,
                                      RESETVECTOR_SYMBOL,
                                      &start);

        if (status >= 0 && procMgrHandle->procId == MultiProc_getId ("Tesla")) {
            numBytes = 4;
            status = ProcMgr_read (handle, start + 4, &numBytes, &entryPt);
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_start",
                                     status,
                                     "ProcMgr_read failed");
            }
            else {
                /* Get the user virtual address of the PRM base */
                sysCtrlMapInfo.src  = 0x4A002000;
                sysCtrlMapInfo.size = 0x1000;

                status = Memory_map (&sysCtrlMapInfo);
                if (status < 0) {
                    status = PROCMGR_E_FAIL;
                    GT_setFailureReason (curTrace,
                                      GT_4CLASS,
                                      "ProcMgr_load",
                                      status,
                                      "Memory_map failed");
                }
                else {
                    Osal_printf ("DSP load address [0x%x]\n",
                              start + entryPt);
                    *(UInt32 *)(sysCtrlMapInfo.dst + 0x304) = start + entryPt;

                    sysCtrlUnmapInfo.addr = sysCtrlMapInfo.dst;
                    sysCtrlUnmapInfo.size = sysCtrlMapInfo.size;
                    status = Memory_unmap (&sysCtrlUnmapInfo);
                }
            }
        }
        else if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_start",
                                 status,
                                 "ProcMgr_getSymbolAddress failed");
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = ProcMgr_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_start",
                                 status,
                                 "ProcMgr_getSymbolAddress failed");
        }
        else {
#endif
            status = Ipc_control (params->proc_id, Ipc_CONTROLCMD_LOADCALLBACK,
                                  (Ptr)start);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                 GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_start",
                                     status,
                                     "Ipc API failed on kernel-side!");
            }
            else {
#endif
#endif
                cmdArgs.handle = procMgrHandle->knlObject;
                cmdArgs.params = params;
                cmdArgs.entry_point = entryPt;
                status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_START, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ProcMgr_start",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
                }
#endif
#ifdef SYSLINK_USE_SYSMGR
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
#endif //#ifdef SYSLINK_USE_SYSMGR
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif

#ifdef SYSLINK_USE_SYSMGR
    if (status >= 0) {
        status = Ipc_control (params->proc_id, Ipc_CONTROLCMD_STARTCALLBACK,
                              NULL);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_start",
                                 status,
                                 "SYSMGR API failed on kernel-side!");
        }
#endif
    }
#endif //#ifdef SYSLINK_USE_SYSMGR

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_start", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to stop the slave processor.
 *
 *              Function to stop execution of the slave processor.
 *              Depending on the boot mode, after successful completion of this
 *              function, the ProcMgr instance may be in the ProcMgr_State_Reset
 *              state.
 *
 *  @param      handle   Handle to the ProcMgr object
 *
 *  @sa         ProcMgr_start, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_stop (ProcMgr_Handle handle, ProcMgr_StopParams * params)
{
    Int                 status          = PROCMGR_SUCCESS;
    ProcMgr_Object *    procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsStop cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_stop", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_stop",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (params == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_stop",
                             status,
                             "Invalid NULL params specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

#ifdef SYSLINK_USE_SYSMGR
        status = Ipc_control (params->proc_id, Ipc_CONTROLCMD_STOPCALLBACK,
                              NULL);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_stop",
                                 status,
                                 "Ipc API failed on kernel-side!");
        }
#endif
#endif //#ifdef SYSLINK_USE_SYSMGR
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.params = params;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_STOP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_stop",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_stop", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to get the current state of the slave Processor.
 *
 *              This function gets the state of the slave processor as
 *              maintained on the master Processor state machine. It does not
 *              go to the slave processor to get its actual state at the time
 *              when this API is called.
 *
 *  @param      handle   Handle to the ProcMgr object
 *
 *  @sa         Processor_getState, ProcMgrDrvUsr_ioctl
 */
ProcMgr_State
ProcMgr_getState (ProcMgr_Handle handle)
{
    Int                     status          = PROCMGR_SUCCESS;
    ProcMgr_State           state           = ProcMgr_State_Unknown;
    ProcMgr_Object *        procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsGetState cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_getState", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /* No status set here since this function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getState",
                             PROCMGR_E_HANDLE,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETSTATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getState",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            state = cmdArgs.procMgrState;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_getState", state);

    /*! @retval Processor state */
    return state;
}

Int ProcMgr_setState (ProcMgr_State  state)
{
    return PROCMGR_SUCCESS;
}


/*!
 *  @brief      Function to read from the slave processor's memory.
 *
 *              This function reads from the specified address in the
 *              processor's address space and copies the required number of
 *              bytes into the specified buffer.
 *              It returns the number of bytes actually read in the numBytes
 *              parameter.
 *
 *  @param      handle     Handle to the ProcMgr object
 *  @param      procAddr   Address in space processor's address space of the
 *                         memory region to read from.
 *  @param      numBytes   IN/OUT parameter. As an IN-parameter, it takes in the
 *                         number of bytes to be read. When the function
 *                         returns, this parameter contains the number of bytes
 *                         actually read.
 *  @param      buffer     User-provided buffer in which the slave processor's
 *                         memory contents are to be copied.
 *
 *  @sa         ProcMgr_write, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_read (ProcMgr_Handle handle,
              UInt32         procAddr,
              UInt32 *       numBytes,
              Ptr            buffer)
{
    Int                  status         = PROCMGR_SUCCESS;
    ProcMgr_Object *     procMgrHandle  = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsRead  cmdArgs;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_read",
               handle, procAddr, numBytes, buffer);

    GT_assert (curTrace, (handle   != NULL));
    GT_assert (curTrace, (procAddr != 0));
    GT_assert (curTrace, (numBytes != NULL));
    GT_assert (curTrace, (buffer   != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_read",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (procAddr == 0u) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value 0 provided for
                     argument procAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_read",
                             status,
                             "Invalid value 0 provided for argument procAddr");
    }
    else if (numBytes == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument numBytes */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_read",
                           status,
                           "Invalid value NULL provided for argument numBytes");
    }
    else if (buffer == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument buffer */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_read",
                             status,
                             "Invalid value NULL provided for argument buffer");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.procAddr = procAddr;
        cmdArgs.numBytes = *numBytes;
        cmdArgs.buffer = buffer;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_READ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_read",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Return number of bytes actually read. */
            *numBytes = cmdArgs.numBytes;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_read", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to write into the slave processor's memory.
 *
 *              This function writes into the specified address in the
 *              processor's address space and copies the required number of
 *              bytes from the specified buffer.
 *              It returns the number of bytes actually written in the numBytes
 *              parameter.
 *
 *  @param      handle     Handle to the ProcMgr object
 *  @param      procAddr   Address in space processor's address space of the
 *                         memory region to write into.
 *  @param      numBytes   IN/OUT parameter. As an IN-parameter, it takes in the
 *                         number of bytes to be written. When the function
 *                         returns, this parameter contains the number of bytes
 *                         actually written.
 *  @param      buffer     User-provided buffer from which the data is to be
 *                         written into the slave processor's memory.
 *
 *  @sa         ProcMgr_read, ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_write (ProcMgr_Handle handle,
               UInt32         procAddr,
               UInt32 *       numBytes,
               Ptr            buffer)
{
    Int                     status          = PROCMGR_SUCCESS;
    ProcMgr_Object *        procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsWrite    cmdArgs;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_write",
               handle, procAddr, numBytes, buffer);

    GT_assert (curTrace, (handle   != NULL));
    GT_assert (curTrace, (procAddr != 0));
    GT_assert (curTrace, (numBytes != NULL));
    GT_assert (curTrace, (buffer   != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_write",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (procAddr == 0u) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value 0 provided for
                     argument procAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_write",
                             status,
                             "Invalid value 0 provided for argument procAddr");
    }
    else if (numBytes == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument numBytes */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_write",
                           status,
                           "Invalid value NULL provided for argument numBytes");
    }
    else if (buffer == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument buffer */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_write",
                             status,
                             "Invalid value NULL provided for argument buffer");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.procAddr = procAddr;
        cmdArgs.numBytes = *numBytes;
        cmdArgs.buffer = buffer;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_WRITE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_write",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Return number of bytes actually written. */
            *numBytes = cmdArgs.numBytes;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_write", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to perform device-dependent operations.
 *
 *              This function performs control operations supported by the
 *              as exposed directly by the specific implementation of the
 *              Processor interface. These commands and their specific argument
 *              types are used with this function.
 *
 *  @param      handle     Handle to the ProcMgr object
 *  @param      cmd        Device specific processor command
 *  @param      arg        Arguments specific to the type of command.
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_control (ProcMgr_Handle handle, Int32 cmd, Ptr arg)
{
    Int                     status          = PROCMGR_SUCCESS;
    ProcMgr_Object *        procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsControl  cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "ProcMgr_control", handle, cmd, arg);

    GT_assert (curTrace, (handle != NULL));
    /* cmd and arg can be 0/NULL, so cannot check for validity. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_control",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (arg == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_control",
                             status,
                             "Invalid NULL argument specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.cmd = cmd;
        cmdArgs.arg = arg;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_CONTROL, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_control",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_control", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to translate between two types of address spaces.
 *
 *              This function translates addresses between two types of address
 *              spaces. The destination and source address types are indicated
 *              through parameters specified in this function.
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      dstAddr     Return parameter: Pointer to receive the translated
 *                          address.
 *  @param      dstAddrType Destination address type requested
 *  @param      srcAddr     Source address in the source address space
 *  @param      srcAddrType Source address type
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_translateAddr (ProcMgr_Handle   handle,
                       Ptr *            dstAddr,
                       ProcMgr_AddrType dstAddrType,
                       Ptr              srcAddr,
                       ProcMgr_AddrType srcAddrType)
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;
    UInt32                          fmAddrBase      = (UInt32) NULL;
    UInt32                          toAddrBase      = (UInt32) NULL;
    Bool                            found           = FALSE;
    ProcMgr_AddrInfo *              memEntry;
    UInt16                          i;

    GT_5trace (curTrace, GT_ENTER, "ProcMgr_translateAddr",
               handle, dstAddr, dstAddrType, srcAddr, srcAddrType);

    GT_assert (curTrace, (handle        != NULL));
    GT_assert (curTrace, (dstAddr       != NULL));
    GT_assert (curTrace, (dstAddrType   < ProcMgr_AddrType_EndValue));
    GT_assert (curTrace, (srcAddr       != NULL));
    GT_assert (curTrace, (srcAddrType   < ProcMgr_AddrType_EndValue));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_translateAddr",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (dstAddr == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument dstAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "ProcMgr_translateAddr",
                            status,
                            "Invalid value NULL provided for argument dstAddr");
    }
    else if (dstAddrType >= ProcMgr_AddrType_EndValue) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value provided for
                     argument dstAddrType */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_translateAddr",
                             status,
                             "Invalid value provided for argument dstAddrType");
    }
    else if (srcAddr == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument srcAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "ProcMgr_translateAddr",
                            status,
                            "Invalid value NULL provided for argument srcAddr");
    }
    else if (srcAddrType >= ProcMgr_AddrType_EndValue) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value provided for
                     argument srcAddrType */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_translateAddr",
                             status,
                             "Invalid value provided for argument srcAddrType");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        *dstAddr = NULL;
        for (i = 0 ; i < procMgrHandle->numMemEntries ; i++) {
            GT_assert (curTrace, (i < PROCMGR_MAX_MEMORY_REGIONS));
            memEntry = &(procMgrHandle->memEntries [i]);
            /* Determine which way to convert */
            fmAddrBase = memEntry->addr [srcAddrType];
            toAddrBase = memEntry->addr [dstAddrType];
            GT_3trace (curTrace,
                       GT_3CLASS,
                       "    ProcMgr_translateAddr: Entry %d\n"
                       "        Source address base [0x%x]\n"
                       "        Dest   address base [0x%x]\n",
                       i,
                       fmAddrBase,
                       toAddrBase);
            if (IS_RANGE_VALID ((UInt32) srcAddr,
                                fmAddrBase,
                                (fmAddrBase + memEntry->size))) {
                GT_2trace (curTrace,
                           GT_2CLASS,
                           "    ProcMgr_translateAddr: Found entry!\n"
                           "        Region address base [0x%x]\n"
                           "        Region size         [0x%x]\n",
                           fmAddrBase,
                           memEntry->size);
                found = TRUE;
                *dstAddr = (Ptr) (((UInt32) srcAddr - fmAddrBase) + toAddrBase);
                break;
            }
        }

        /* This check must not be removed even with build optimize. */
        if (found == FALSE) {
            /*! @retval PROCMGR_E_TRANSLATE Failed to translate address. */
            status = PROCMGR_E_TRANSLATE;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_translateAddr",
                                 status,
                                 "Failed to translate address");
        }
        else {
            GT_2trace (curTrace,
                       GT_1CLASS,
                       "    ProcMgr_translateAddr: srcAddr [0x%x] "
                       "dstAddr [0x%x]\n",
                       srcAddr,
                      *dstAddr);
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_translateAddr", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the target address of a symbol from the
 *              specified file.
 *
 *  @param      handle   Handle to the ProcMgr object
 *  @param      fileId   ID of the file received from the load function
 *  @param      symName  Name of the symbol
 *  @param      symValue Return parameter: Symbol address
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_getEntryNamesInfo (ProcMgr_Handle handle,
                           UInt32         fileId,
                           Int32 *        entryPtCnt,
                           Int32 *        entryPtMaxNameLen)
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_getEntryNamesInfo",
               handle, fileId, entryPtCnt, entryPtMaxNameLen);

    GT_assert (curTrace, (handle       != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (entryPtCnt   != NULL));
    GT_assert (curTrace, (entryPtNames != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getEntryNamesInfo",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (entryPtCnt == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getEntryNamesInfo",
                         status,
                         "Invalid value NULL provided for argument entryPtCnt");
    }
    else if (entryPtMaxNameLen == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getEntryNamesInfo",
                         status,
                         "Invalid value NULL provided for argument "
                         "entryPtMaxNameLen");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = DLoad4430_getEntryNamesInfo (procMgrHandle->loaderHandle,
                                              fileId, entryPtCnt,
                                              entryPtMaxNameLen);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = PROCMGR_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getEntryNamesInfo",
                                 status,
                                 "DLoad4430_getEntryNamesInfo failed");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_getEntryNamesInfo", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the target address of a symbol from the
 *              specified file.
 *
 *  @param      handle   Handle to the ProcMgr object
 *  @param      fileId   ID of the file received from the load function
 *  @param      symName  Name of the symbol
 *  @param      symValue Return parameter: Symbol address
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_getEntryNames (ProcMgr_Handle handle,
                       UInt32         fileId,
                       Int32 *        entryPtCnt,
                       Char ***       entryPtNames)
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_getEntryNames",
               handle, fileId, entryPtCnt, entryPtNames);

    GT_assert (curTrace, (handle       != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (entryPtCnt   != NULL));
    GT_assert (curTrace, (entryPtNames != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getEntryNames",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (entryPtCnt == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getEntryNames",
                         status,
                         "Invalid value NULL provided for argument entryPtCnt");
    }
    else if (entryPtNames == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getEntryNames",
                         status,
                         "Invalid value NULL provided for argument "
                         "entryPtNames");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = DLoad4430_getEntryNames (procMgrHandle->loaderHandle,
                                          fileId, entryPtCnt, entryPtNames);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = PROCMGR_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getEntryNames",
                                 status,
                                 "DLoad4430_getEntryNames failed");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_getEntryNames", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the target address of a symbol from the
 *              specified file.
 *
 *  @param      handle   Handle to the ProcMgr object
 *  @param      fileId   ID of the file received from the load function
 *  @param      symName  Name of the symbol
 *  @param      symValue Return parameter: Symbol address
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_getSymbolAddress (ProcMgr_Handle handle,
                          UInt32         fileId,
                          String         symbolName,
                          UInt32 *       symValue)
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_getSymbolAddress",
               handle, fileId, symbolName, symValue);

    GT_assert (curTrace, (handle      != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (symbolName  != NULL));
    GT_assert (curTrace, (symValue    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getSymbolAddress",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (symbolName == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getSymbolAddress",
                         status,
                         "Invalid value NULL provided for argument symbolName");
    }
    else if (symValue == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_getSymbolAddress",
                         status,
                         "Invalid value NULL provided for argument symValue");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = DLoad4430_getSymbolAddress (procMgrHandle->loaderHandle,
                                             fileId, symbolName, symValue);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            status = PROCMGR_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getSymbolAddress",
                                 status,
                                 "DLoad4430_getSymbolAddress failed");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_2trace (curTrace, GT_LEAVE, "ProcMgr_getSymbolAddress", status,
                *symValue);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to map address to slave address space.
 *
 *              This function maps the provided slave address to a host address
 *              and returns the mapped address and size.
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      procAddr    Slave address to be mapped
 *  @param      size        Size (in bytes) of region to be mapped
 *  @param      mappedAddr  Return parameter: Mapped address in host address
 *                          space
 *  @param      mappedSize  Return parameter: Mapped size
 *  @param      type        Type of mapping.
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_map (ProcMgr_Handle     handle,
             UInt32             procAddr,
             UInt32             size,
             UInt32 *           mappedAddr,
             UInt32 *           mappedSize,
             ProcMgr_MapType    type,
             ProcMgr_ProcId     procID)
{
    Int                 status          = PROCMGR_SUCCESS;
    Int                 flags           = 0;
    Int                 numOfBuffers           = 1;
    ProcMgr_MemPoolType poolId;

    GT_5trace (curTrace, GT_ENTER, "ProcMgr_map",
               handle, procAddr, size, mappedAddr, mappedSize);

    GT_assert (curTrace, (handle        != NULL));
    GT_assert (curTrace, (procAddr      != 0));
    GT_assert (curTrace, (size          != 0));
    GT_assert (curTrace, (mappedAddr    != NULL));
    GT_assert (curTrace, (mappedSize    != NULL));
    GT_assert (curTrace, (type < ProcMgr_MapType_EndValue));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_map",
                             status,
                             "Invalid NULL handle specified");
    }
    else if  (!MultiProc_isValidRemoteProc (procID)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_map",
                             status,
                             "Invalid remote proc specified");
   }
    else if (procAddr == 0u) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value 0 provided for
                     argument procAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_map",
                             status,
                             "Invalid value 0 provided for argument procAddr");
    }
    else if (size == 0u) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value 0 provided for
                     argument size */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_map",
                             status,
                             "Invalid value 0 provided for argument size");
    }
    else if (mappedAddr == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument mappedAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_map",
                         status,
                         "Invalid value NULL provided for argument mappedAddr");
    }
    else if (mappedSize == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument mappedSize */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_map",
                         status,
                         "Invalid value NULL provided for argument mappedSize");
    }
    else if (type >= ProcMgr_MapType_EndValue) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value provided for
                     argument type */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_map",
                             status,
                             "Invalid value provided for argument type");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        switch (type) {
        case ProcMgr_MapType_Phys:
            flags |= DMM_DA_PHYS;
            poolId = ProcMgr_NONE_MemPool;
            break;
        case ProcMgr_MapType_Tiler:
            flags |= DMM_DA_PHYS;
            poolId = ProcMgr_NONE_MemPool;
            break;
        case ProcMgr_MapType_Virt:
            flags |= DMM_DA_USER;
            poolId = ProcMgr_DMM_MemPool;
            break;
        default:
            status = PROCMGR_E_INVALIDARG;
            break;
        }
        GT_5trace (curTrace, GT_ENTER, "ProcMgr_map",
               type, procAddr, size, mappedAddr, mappedSize);

        if (status < 0)
            goto error_exit;

        status = ProcMMU_Map (procAddr, mappedAddr, numOfBuffers, size, poolId,
                                                                flags, procID);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_map",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_map", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
error_exit:
    return status;
}


/*!
 *  @brief      Function to unmap address from the slave address space.
 *
 *              This function unmaps the already mapped slave address.
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      mappedAddr  Mapped address in host address space
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_unmap (ProcMgr_Handle   handle,
               UInt32           mappedAddr,
               ProcMgr_ProcId   procID)
{
    Int                   status          = PROCMGR_SUCCESS;

    GT_3trace (curTrace, GT_ENTER, "ProcMgr_unmap", handle, mappedAddr, procID);

#if defined(SYSLINK_BUILD_DEBUG)
    GT_assert (curTrace, (handle        != NULL));
    GT_assert (curTrace, (mappedAddr    != NULL));
#endif

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_unmap",
                             status,
                             "Invalid NULL handle specified");
    }
    else if  (!MultiProc_isValidRemoteProc (procID)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_unmap",
                             status,
                             "Invalid remote proc specified");
   }
    else if (mappedAddr == (UInt32)NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument mappedAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "ProcMgr_unmap",
                         status,
                         "Invalid value NULL provided for argument mappedAddr");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = ProcMMU_UnMap (mappedAddr, procID);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_unmap", status);

    return status;
}

/*!
 *  @brief      Function to get the target revision
 *
 *              This function get the ES version of the OMAPBoard
 *
 *  @param      cpuRev    Return parameter
  *
 *  @sa         none
 */


Int
ProcMgr_getCpuRev (UInt32 *cpuRev)
{
    Int                         status;
    ProcMgr_CmdArgsGetCpuRev    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_getCpuRev", cpuRev);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cpuRev == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getCpuRev",
                             status,
                             "Invalid pointer specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.cpuRev = cpuRev;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETBOARDREV, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "ProcMgr_getCpuRev",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_getCpuRev", status);

    return status;
}

/*!
 *  @brief      Function that registers for notification when the slave
 *              processor transitions to any of the states specified.
 *
 *              This function allows the user application to register for
 *              changes in processor state and take actions accordingly.
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      fxn         Handling function to be registered.
 *  @param      args        Optional arguments associated with the handler fxn.
 *  @param      state       Array of target states for which registration is
 *                          required.
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_registerNotify (ProcMgr_Handle      handle,
                        ProcMgr_CallbackFxn fxn,
                        Ptr                 args,
                        ProcMgr_State       state [])
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsRegisterNotify   cmdArgs;

    GT_4trace (curTrace, GT_ENTER, "ProcMgr_registerNotify",
               handle, fxn, args, state);

    GT_assert (curTrace, (handle        != NULL));
    GT_assert (curTrace, (fxn           != 0));
    /* args is optional and may be NULL. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_registerNotify",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (fxn == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument fxn */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_registerNotify",
                             status,
                             "Invalid value NULL provided for argument fxn");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.fxn = fxn;
        cmdArgs.args = args;
        /* State TBD. */
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_REGISTERNOTIFY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_registerNotify",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_registerNotify", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function that returns information about the characteristics of
 *              the slave processor.
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      procInfo    Pointer to the ProcInfo object to be populated.
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_getProcInfo (ProcMgr_Handle     handle,
                     ProcMgr_ProcInfo * procInfo)
{
    Int                         status          = PROCMGR_SUCCESS;
    ProcMgr_Object *            procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsGetProcInfo  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_getProcInfo", handle, procInfo);

    GT_assert (curTrace, (handle    != NULL));
    GT_assert (curTrace, (procInfo  != 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_getProcInfo",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (procInfo == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument procInfo */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_getProcInfo",
                           status,
                           "Invalid value NULL provided for argument procInfo");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.procInfo = procInfo;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETPROCINFO, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_getProcInfo",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Update user virtual address in memory configuration. */
            procInfo->numMemEntries = procMgrHandle->numMemEntries;
            Memory_copy (&(procInfo->memEntries),
                         &(procMgrHandle->memEntries),
                         sizeof (procMgrHandle->memEntries));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_getProcInfo", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}

/*!
 *  @brief      Function that returns virtual address to phsyical
 *              address translations
 *
 *  @param      handle      Handle to the ProcMgr object
 *  @param      remoteAddr  Remote Processor Address
 *  @param      numOfPages  Number of entries to translate
 *  @param      physEntries Buffer that has translated entries
 *  @param      procId      Remote Processor ID
 *
 *  @sa         ProcMgrDrvUsr_ioctl
 */
Int
ProcMgr_virtToPhysPages (ProcMgr_Handle handle,
                         UInt32         remoteAddr,
                         UInt32         numOfPages,
                         UInt32 *       physEntries,
                         ProcMgr_ProcId procId)
{
    Int                             status          = PROCMGR_SUCCESS;
    ProcMgr_Object *                procMgrHandle   = (ProcMgr_Object *) handle;
    ProcMgr_CmdArgsVirtToPhysPages  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "ProcMgr_virtToPhysPages", handle,
                physEntries);

    GT_assert (curTrace, (handle    != NULL));
    GT_assert (curTrace, (physEntries  != 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  PROCMGR_E_HANDLE Invalid NULL handle specified */
        status = PROCMGR_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_virtToPhysPages",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (physEntries == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument physEntries */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_virtToPhysPages",
                           status,
                           "Invalid value provided for argument physEntries");
    }
    else if (numOfPages == 0) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_virtToPhysPages",
                           status,
                           "Invalid value provided for argument numEntries");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.handle = procMgrHandle->knlObject;
        cmdArgs.da = remoteAddr;
        cmdArgs.memEntries = physEntries;
        cmdArgs.numEntries = numOfPages;
        status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_GETVIRTTOPHYS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_virtToPhysPages",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_virtToPhysPages", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}

/*!
 *  @brief      Function to Invalidate user space buffers
 *
 *  @param      bufAddr     Userspace Virtual Address
 *  @param      bufSize     size of the buffer
 *
 *  @sa         ProcMgr_flushMemory
 */
Int
ProcMgr_invalidateMemory(PVOID bufAddr, UInt32 bufSize,
                        ProcMgr_ProcId procID)
{
    Int                             status          = PROCMGR_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_invalidateMemory", bufAddr);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (bufAddr == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument bufAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_invalidateMemory",
                           status,
                           "Invalid value provided for argument bufAddr");
    }
    else if (bufSize == 0) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_invalidateMemory",
                           status,
                           "Invalid value provided for argument bufSize");
    }
    else if (!MultiProc_isValidRemoteProc (procID)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_invalidateMemory",
                           status,
                           "Invalid value provided for argument procID");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = ProcMMU_InvMemory(bufAddr, bufSize, procID);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_invalidateMemory",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_invalidateMemory", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to flush user space buffers
 *
 *  @param      bufAddr    Userspace Virtual Address
 *  @param      bufSize      size of the buffer
 *
 *  @sa         ProcMgr_invalidateMemory
 */
Int
ProcMgr_flushMemory(PVOID bufAddr, UInt32 bufSize, ProcMgr_ProcId procID) {

    Int                             status          = PROCMGR_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_flushMemory", bufAddr);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (bufAddr == NULL) {
        /*! @retval  PROCMGR_E_INVALIDARG Invalid value NULL provided for
                     argument bufAddr */
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_flushMemory",
                           status,
                           "Invalid value provided for argument bufAddr");
    }
    else if (bufSize == 0) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_flushMemory",
                           status,
                           "Invalid value provided for argument bufSize");
    }
    else if (!MultiProc_isValidRemoteProc (procID)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_flushMemory",
                           status,
                           "Invalid value provided for argument procID");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = ProcMMU_FlushMemory(bufAddr, bufSize, procID);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_flushMemory",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_flushMemory", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}

/*!
 *  @brief      Function to register for Mutiple Events types
 *
 *  @param      procId       Processor ID
 *  @param      eventType    Array of events
 *  @param      size         Number of events
 *  @param      timeout      Timeout
 *  @param      index        Index of event received
 *
 *  @sa         ProcMgr_waitForEvent
 */
Int
ProcMgr_waitForMultipleEvents (ProcMgr_ProcId      procId,
                               ProcMgr_EventType * eventType,
                               UInt32              size,
                               Int                 timeout,
                               UInt *              index)
{
    Int                   * efd;
    fd_set                  fds;
    struct timeval          tv;
    struct timeval       *  to = NULL;
    Int                     status = PROCMGR_SUCCESS;
    Int                     i;
    Int                     max = 0;
    Bool                    mmuEventUsed = FALSE;
    Bool                    dehEventUsed = FALSE;
    ProcMgr_CmdArgsRegEvent cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "ProcMgr_waitForMultipleEvents", procId,
               eventType, size, timeout, index);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!size) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_waitForMultipleEvents",
                             status,
                             "Invalid value provided for argument size");
    }
    else if (index == NULL) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_waitForMultipleEvents",
                             status,
                             "Invalid value provided for argument index");
    }
    else {
#endif
        efd = Memory_alloc (NULL, sizeof(Int) * size, 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (!efd) {
            status = PROCMGR_E_MEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_waitForMultipleEvents",
                                 status,
                                 "Unable to allocate memory for efd");
        }
        else {
#endif
            if (timeout != -1) {
                tv.tv_sec = timeout / 1000;
                tv.tv_usec = (timeout % 1000) * 1000;
                to = &tv;
            }
            FD_ZERO (&fds);
            for (i = 0; i < size; i++) {
                if (eventType [i] == PROC_MMU_FAULT && !mmuEventUsed) {
                    status = ProcMMU_open (procId);
                    if (status < 0) {
                        status = PROCMGR_E_FAIL;
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "ProcMgr_waitForMultipleEvents",
                                             status,
                                             "Error in ProcMMU_open");
                        break;
                    }
                    mmuEventUsed = TRUE;
                }
                if ((eventType [i] == PROC_ERROR ||
                        eventType [i] == PROC_WATCHDOG) && !dehEventUsed) {
                    status = ProcDEH_open (procId);
                    if (status < 0) {
                        status = PROCMGR_E_FAIL;
                        GT_setFailureReason (curTrace,
                                             GT_4CLASS,
                                             "ProcMgr_waitForMultipleEvents",
                                             status,
                                             "Error in ProcDEH_open");
                        break;
                    }
                    dehEventUsed = TRUE;
                }
                efd [i] = eventfd (0, 0);
                if (efd [i] == -1) {
                    status = PROCMGR_E_FAIL;
                    break;
                }

                switch (eventType [i]) {
                case PROC_MMU_FAULT:
                    status = ProcMMU_registerEvent (procId, efd [i], TRUE);
                    if (status == ProcMMU_S_SUCCESS) {
                        status = PROCMGR_SUCCESS;
                    }
                    else {
                        status = PROCMGR_E_FAIL;
                    }
                    break;

                case PROC_STOP:
                case PROC_START:
                    cmdArgs.procId = procId;
                    cmdArgs.event = eventType [i];
                    cmdArgs.fd = efd [i];
                    status = ProcMgrDrvUsr_ioctl (CMD_PROCMGR_REGEVENT, &cmdArgs);
                    break;

                case PROC_ERROR:
                    status = ProcDEH_registerEvent (procId, ProcDEH_SYSERROR,
                                                        efd [i], TRUE);
                    if (status == ProcDEH_S_SUCCESS) {
                        status = PROCMGR_SUCCESS;
                    }
                    else {
                        status = PROCMGR_E_FAIL;
                    }
                    break;

                case PROC_WATCHDOG:
                    status = ProcDEH_registerEvent (procId,
                                                    ProcDEH_WATCHDOGERROR,
                                                    efd [i], TRUE);
                    if (status == ProcDEH_S_SUCCESS) {
                        status = PROCMGR_SUCCESS;
                    }
                    else {
                        status = PROCMGR_E_FAIL;
                    }
                    break;

                default:
                    status = PROCMGR_E_INVALIDARG;
                    break;
                }
                if (status != PROCMGR_SUCCESS) {
                    close (efd [i]);
                    break;
                }

                FD_SET (efd [i], &fds);
                if (efd [i] > max) {
                    max = efd [i];
                }
            }

            if (status == PROCMGR_SUCCESS) {
                status = select (max + 1, &fds, NULL, NULL, to);
                if (!status) {
                    status = PROCMGR_E_TIMEOUT;
                }
                else if (status < 0) {
                    status = PROCMGR_E_FAIL;
                }
                else {
                    status = PROCMGR_SUCCESS;
                }
            }

            while (i--) {
                if (FD_ISSET (efd [i], &fds)) {
                    *index = i;
                }
                if (eventType [i] == PROC_MMU_FAULT) {
                    ProcMMU_registerEvent (procId, efd [i], FALSE);
                }
                else if (eventType [i] == PROC_ERROR) {
                    ProcDEH_registerEvent (procId, ProcDEH_SYSERROR,
                                            efd [i], FALSE);
                }
                else if (eventType [i] == PROC_WATCHDOG) {
                    ProcDEH_registerEvent (procId, ProcDEH_WATCHDOGERROR,
                                            efd [i], FALSE);
                }
                else {
                    cmdArgs.event = eventType [i];
                    cmdArgs.fd = efd [i];
                    ProcMgrDrvUsr_ioctl (CMD_PROCMGR_UNREGEVENT, &cmdArgs);
                }
                close (efd [i]);
            }

            if (dehEventUsed) {
                ProcDEH_close (procId);
            }

            if (mmuEventUsed) {
                ProcMMU_close (procId);
            }

            Memory_free (NULL, efd, sizeof(Int) * size);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_waitForMultipleEvents", status);

    return status;
}


/*!
 *  @brief      Function to register for an event type
 *
 *  @param      procId       Processor ID
 *  @param      eventType    Event Type
 *  @param      timeout      Timeout
 *
 *  @sa         ProcMgr_waitForMultipleEvents
 */
Int
ProcMgr_waitForEvent (ProcMgr_ProcId    procId,
                      ProcMgr_EventType eventType,
                      Int               timeout)
{
    UInt    index;
    Int     status;

    GT_3trace (curTrace, GT_ENTER, "ProcMgr_waitForEvent", procId, eventType,
               timeout);

    status = ProcMgr_waitForMultipleEvents (procId, &eventType, 1,
                                            timeout, &index);

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_waitForEvent", status);

    return status;
}


/*!
 *  @brief      Function to create DMM pool
 *
 *  @param      poolId    Pool ID that is assigned to this pool
 *  @param      daBegin   Virtual Address beginning address
 *  @param      size      size of the buffer
 *  @param      proc      The destination Processor ID
 *
 *  @sa         ProcMgr_deleteDMMPool
 */
Int
ProcMgr_createDMMPool (UInt32 poolId, UInt32 daBegin, UInt32 size, Int proc)
{
    Int status = PROCMGR_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_createDMMPool", proc);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (size == 0) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_createDMMPool",
                           status,
                           "Invalid value provided for argument size");
    }
    else if (!MultiProc_isValidRemoteProc (proc)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                           GT_4CLASS,
                           "ProcMgr_createDMMPool",
                           status,
                           "Invalid value provided for argument proc");
    }
    else {
#endif
        status = ProcMMU_CreateVMPool (poolId, size, daBegin, (daBegin + size),
                                                                0, proc);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_createDMMPool",
                                 status,
                                 "API (through ProcMMU) failed!");
        }
    }
#endif

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_createDMMPool", status);

    return status;
}


/*!
 *  @brief      Function to delete DMM pool
 *
 *  @param      poolId    Pool ID that is assigned to this pool
 *  @param      proc      Processor to which the pool is assigned
 *
 *  @sa         ProcMgr_createDMMPool
 */
Int
ProcMgr_deleteDMMPool (UInt32 poolId, Int proc)
{
    Int status = PROCMGR_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "ProcMgr_deleteDMMPool", proc);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!MultiProc_isValidRemoteProc (proc)) {
        status = PROCMGR_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ProcMgr_deleteDMMPool",
                             status,
                             "Invalid value provided for argument proc");
    }
    else {
#endif
        status = ProcMMU_DeleteVMPool (poolId, proc);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ProcMgr_deleteDMMPool",
                                 status,
                                 "API (through ProcMMU) failed!");
        }
    }
#endif

    GT_1trace (curTrace, GT_LEAVE, "ProcMgr_deleteDMMPool", status);

    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
