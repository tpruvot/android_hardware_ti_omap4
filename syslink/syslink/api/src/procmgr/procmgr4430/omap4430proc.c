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
 *  @file   omap4430proc.c
 *
 *  @brief      Processor implementation for OMAP4430.
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <Trace.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>
#include <omap4430proc.h>
#include <omap4430procDrvDefs.h>
#include <omap4430procDrvUsr.h>
#include <_ProcMgrDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  OMAP4430PROC Module state object
 */
typedef struct OMAP4430PROC_ModuleObject_tag {
    UInt32      setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
    Handle      procHandles [MultiProc_MAXPROCESSORS];
    /*!< Processor handle array. */
} OMAP4430PROC_ModuleObject;

/*!
 *  @brief  OMAP4430PROC instance object.
 */
typedef struct OMAP4430PROC_Object_tag {
    ProcMgr_CommonObject commonObj;
    /*!< Common object required to be the first field in the instance object
         structure. This is used by ProcMgr to get the handle to the kernel
         object. */
    UInt32               openRefCount;
    /*!< Reference count for number of times open/close were called in this
         process. */
    Bool                 created;
    /*!< Indicates whether the object was created in this process. */
    UInt16               procId;
    /*!< Processor ID */
} OMAP4430PROC_Object;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    OMAP4430PROC_state
 *
 *  @brief  OMAP4430PROC state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
OMAP4430PROC_ModuleObject OMAP4430PROC_state =
{
    .setupRefCount = 0
};


/* =============================================================================
 * APIs directly called by applications
 * =============================================================================
 */
/*!
 *  @brief      Function to get the default configuration for the OMAP4430PROC
 *              module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to OMAP4430PROC_setup filled in by the
 *              OMAP4430PROC module with the default parameters. If the user
 *              does not wish to make any change in the default parameters, this
 *              API is not required to be called.
 *
 *  @param      cfg        Pointer to the OMAP4430PROC module configuration
 *                         structure in which the default config is to be
 *                         returned.
 *
 *  @sa         OMAP4430PROC_setup, OMAP4430PROCDrvUsr_open,
 *              OMAP4430PROCDrvUsr_ioctl, OMAP4430PROCDrvUsr_close
 */
Void
OMAP4430PROC_getConfig (OMAP4430PROC_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                         status = OMAP4430PROC_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    OMAP4430PROC_CmdArgsGetConfig    cmdArgs;

	Osal_printf ("-> OMAP4430PROC_getConfig\n");

    GT_1trace (curTrace, GT_ENTER, "OMAP4430PROC_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_getConfig",
                             OMAP4430PROC_E_INVALIDARG,
                             "Argument of type (OMAP4430PROC_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        OMAP4430PROCDrvUsr_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROC_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.cfg = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        OMAP4430PROCDrvUsr_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_getConfig\n");
    GT_0trace (curTrace, GT_ENTER, "OMAP4430PROC_getConfig");
}


/*!
 *  @brief      Function to setup the OMAP4430PROC module.
 *
 *              This function sets up the OMAP4430PROC module. This function
 *              must be called before any other instance-level APIs can be
 *              invoked. Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then OMAP4430PROC_getConfig can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. If the user
 *              does not wish to make any change in the default parameters, the
 *              application can simply call OMAP4430PROC_setup with NULL
 *              parameters. The default parameters would get automatically used.
 *
 *  @param      cfg   Optional OMAP4430PROC module configuration. If provided as
 *                    NULL, default configuration is used.
 *
 *  @sa         OMAP4430PROC_destroy, OMAP4430PROCDrvUsr_open,
 *              OMAP4430PROCDrvUsr_ioctl
 */
Int
OMAP4430PROC_setup (OMAP4430PROC_Config * cfg)
{
    Int                     status = OMAP4430PROC_SUCCESS;
    OMAP4430PROC_CmdArgsSetup cmdArgs;

	Osal_printf ("-> OMAP4430PROC_setup\n");
    /* TBD: Protect from multiple threads. */
    OMAP4430PROC_state.setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (OMAP4430PROC_state.setupRefCount > 1) {
        /*! @retval OMAP4430PROC_S_ALREADYSETUP Success: OMAP4430PROC module has
                                           been already setup in this process */
        status = OMAP4430PROC_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "    OMAP4430PROC_setup: OMAP4430PROC module has been "
                   "already setup in this process.\n"
                   "        RefCount: [%d]\n",
                   (OMAP4430PROC_state.setupRefCount - 1));
    }
    else {
        /* Open the driver handle. */
        status = OMAP4430PROCDrvUsr_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROC_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.cfg = cfg;
            status = OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_SETUP,
                                               &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_setup",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

	Osal_printf ("<- OMAP4430PROC_setup\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_setup", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to destroy the OMAP4430PROC module.
 *
 *              Once this function is called, other OMAP4430PROC module APIs,
 *              except for the OMAP4430PROC_getConfig API cannot be called
 *              anymore.
 *
 *  @sa         OMAP4430PROC_setup, OMAP4430PROCDrvUsr_ioctl
 */
Int
OMAP4430PROC_destroy (Void)
{
    Int                         status = OMAP4430PROC_SUCCESS;
    OMAP4430PROC_CmdArgsDestroy   cmdArgs;
    UInt16                      i;
	Osal_printf ("-> OMAP4430PROC_destroy\n");
    GT_0trace (curTrace, GT_ENTER, "OMAP4430PROC_destroy");

    /* TBD: Protect from multiple threads. */
    OMAP4430PROC_state.setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (OMAP4430PROC_state.setupRefCount >= 1) {
        /*! @retval OMAP4430PROC_S_SETUP Success: OMAP4430PROC module has been
                                       setup by other clients in this process */
        status = OMAP4430PROC_S_SETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "OMAP4430PROC module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (OMAP4430PROC_state.setupRefCount + 1));
    }
    else {
        status = OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROC_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Check if any OMAP4430PROC instances have not been deleted so far. If
         * not, delete them.
         */
        for (i = 0 ; i < MultiProc_MAXPROCESSORS ; i++) {
            GT_assert (curTrace, (OMAP4430PROC_state.procHandles [i] == NULL));
            if (OMAP4430PROC_state.procHandles [i] != NULL) {
                OMAP4430PROC_delete (&(OMAP4430PROC_state.procHandles [i]));
            }
        }

        /* Close the driver handle. */
        OMAP4430PROCDrvUsr_close ();
    }

    Osal_printf ("<- OMAP4430PROC_destroy\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_destroy", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to initialize the parameters for the OMAP4430PROC
 *              instance.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to OMAP4430PROC_create filled in by the
 *              OMAP4430PROC module with the default parameters.
 *
 *  @param      handle   Handle to the OMAP4430PROC object. If specified as
 *                       NULL, the default global configuration values are
 *                       returned.
 *  @param      params   Pointer to the OMAP4430PROC instance params structure
 *                       in which the default params is to be returned.
 *
 *  @sa         OMAP4430PROC_create, OMAP4430PROCDrvUsr_ioctl
 */
Void
OMAP4430PROC_Params_init (Handle handle, OMAP4430PROC_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                             status = OMAP4430PROC_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    OMAP4430PROC_Object *           procHandle = (OMAP4430PROC_Object *) handle;
    OMAP4430PROC_CmdArgsParamsInit  cmdArgs;
	Osal_printf ("-> OMAP4430PROC_Params_init\n");

    GT_2trace (curTrace, GT_ENTER, "OMAP4430PROC_Params_init", handle, params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (params == NULL) {
        /* No retVal comment since this is a Void function. */
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_Params_init",
                             status,
                             "Argument of type (OMAP4430PROC_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Check if the handle is NULL and pass it in directly to kernel-side in
         * that case. Otherwise send the kernel object pointer.
         */
        if (procHandle == NULL) {
            cmdArgs.handle = handle;
        }
        else {
            cmdArgs.handle = procHandle->commonObj.knlObject;
        }
        cmdArgs.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROC_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_Params_init\n");
    GT_0trace (curTrace, GT_LEAVE, "OMAP4430PROC_Params_init");
}


/*!
 *  @brief      Function to create a OMAP4430PROC object for a specific slave
 *              processor.
 *
 *              This function creates an instance of the OMAP4430PROC module and
 *              returns an instance handle, which is used to access the
 *              specified slave processor. The processor ID specified here is
 *              the ID of the slave processor as configured with the MultiProc
 *              module.
 *              Instance-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then OMAP4430PROC_Params_init can be called to get
 *              the configuration filled with the default values. After this,
 *              only the required configuration values can be changed. For this
 *              API, the params argument is not optional, since the user needs
 *              to provide some essential values such as loader, PwrMgr and
 *              Processor instances to be used with this OMAP4430PROC instance.
 *
 *  @param      procId   Processor ID represented by this OMAP4430PROC instance
 *  @param      params   OMAP4430PROC instance configuration parameters.
 *
 *  @sa         OMAP4430PROC_delete, Memory_calloc, OMAP4430PROCDrvUsr_ioctl
 */
Handle
OMAP4430PROC_create (UInt16 procId, const OMAP4430PROC_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                          status = OMAP4430PROC_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    OMAP4430PROC_Object *        handle = NULL;
    /* TBD: UInt32                  key;*/
    OMAP4430PROC_CmdArgsCreate   cmdArgs;

	Osal_printf ("-> OMAP4430PROC_create\n");

    GT_2trace (curTrace, GT_ENTER, "OMAP4430PROC_create", procId, params);

    GT_assert (curTrace, MultiProc_isValidRemoteProc (procId));
    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!MultiProc_isValidRemoteProc (procId)) {
        /*! @retval NULL Invalid procId specified */
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_create",
                             status,
                             "Invalid procId specified");
    }
    else if (params == NULL) {
        /*! @retval NULL Invalid NULL params pointer specified */
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_create",
                             status,
                             "Invalid NULL params pointer specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (OMAP4430PROC_state.gateHandle); */
        if (OMAP4430PROC_state.procHandles [procId] != NULL) {
            /* If the object is already created/opened in this process, return
             * handle in the local array.
             */
            handle = OMAP4430PROC_state.procHandles [procId];
            handle->openRefCount++;
            GT_1trace (curTrace,
                       GT_2CLASS,
                       "    OMAP4430PROC_create: Instance already exists in"
                       " this process space"
                       "        RefCount [%d]\n",
                       (handle->openRefCount - 1));
        }
        else {
            cmdArgs.procId = procId;
            cmdArgs.params = (OMAP4430PROC_Params *) params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_create",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Allocate memory for the handle */
                handle = (OMAP4430PROC_Object *) Memory_calloc (NULL,
                                                   sizeof (OMAP4430PROC_Object),
                                                   0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (handle == NULL) {
                    /*! @retval NULL Memory allocation failed for handle */
                    status = OMAP4430PROC_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "OMAP4430PROC_create",
                                        status,
                                        "Memory allocation failed for handle!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    /* Set pointer to kernel object into the user handle. */
                    handle->commonObj.knlObject = cmdArgs.handle;
                    /* Indicate that the object was created in this process. */
                    handle->created = TRUE;
                    handle->procId = procId;
                    /* Store the OMAP4430PROC handle in the local array. */
                    OMAP4430PROC_state.procHandles [procId] = handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
        /* TBD: Leave critical section protection. */
        /* Gate_leave (OMAP4430PROC_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_create\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_create", handle);

    /*! @retval Valid Handle Operation successful */
    return (handle);
}


/*!
 *  @brief      Function to delete a OMAP4430PROC object for a specific slave
 *              processor.
 *
 *              Once this function is called, other OMAP4430PROC instance level
 *              APIs that require the instance handle cannot be called.
 *
 *  @param      handlePtr   Pointer to Handle to the OMAP4430PROC object
 *
 *  @sa         OMAP4430PROC_create, Memory_free, OMAP4430PROCDrvUsr_ioctl
 */
Int
OMAP4430PROC_delete (Handle * handlePtr)
{
    Int                         status    = OMAP4430PROC_SUCCESS;
    Int                         tmpStatus = OMAP4430PROC_SUCCESS;
    OMAP4430PROC_Object *       handle;
    /* TBD: UInt32          key;*/
    OMAP4430PROC_CmdArgsDelete  cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "OMAP4430PROC_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
	Osal_printf ("-> OMAP4430PROC_delete\n");
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval OMAP4430PROC_E_INVALIDARG Invalid NULL handle pointer
                                            specified*/
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_delete",
                             status,
                             "Invalid NULL handle pointer specified");
    }
    else if (*handlePtr == NULL) {
        /*! @retval OMAP4430PROC_E_HANDLE Invalid NULL handle specified */
        status = OMAP4430PROC_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_delete",
                             status,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        handle = (OMAP4430PROC_Object *) (*handlePtr);
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (OMAP4430PROC_state.gateHandle); */

        if (handle->openRefCount != 0) {
            /* There are still some open handles to this OMAP4430PROC.
             * Give a warning, but still go ahead and delete the object.
             */
            status = OMAP4430PROC_S_OPENHANDLE;
            GT_assert (curTrace, (handle->openRefCount != 0));
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    OMAP4430PROC_delete: Warning, some handles are"
                       " still open!\n"
                       "        RefCount: [%d]\n",
                       handle->openRefCount);
        }

        if (handle->created == FALSE) {
            /*! @retval OMAP4430PROC_E_ACCESSDENIED The OMAP4430PROC object was
                                not created in this process and access
                                is denied to delete it. */
            status = OMAP4430PROC_E_ACCESSDENIED;
            GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "OMAP4430PROC_delete",
                            status,
                            "The OMAP4430PROC object was not created in"
                            " this process and access is denied to delete it.");
        }

        if (status >= 0) {
            /* Only delete the object if it was created in this process. */
            cmdArgs.handle = handle->commonObj.knlObject;
            tmpStatus = OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_DELETE,
                                                  &cmdArgs);
            if (tmpStatus < 0) {
                /* Only override the status if kernel call failed. Otherwise
                 * we want the status from above to carry forward.
                 */
                status = tmpStatus;
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_delete",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
                Memory_free (NULL, handle, sizeof (OMAP4430PROC_Object));
                *handlePtr = NULL;

                /* Clear the OMAP4430PROC handle in the local array. */
                OMAP4430PROC_state.procHandles [handle->procId] = NULL;
            }
        }

        /* TBD: Leave critical section protection. */
        /* Gate_leave (OMAP4430PROC_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_delete\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_delete", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to open a handle to an existing OMAP4430PROC object
 *              handling the procId.
 *
 *              This function returns a handle to an existing OMAP4430PROC
 *              instance created for this procId. It enables other entities to
 *              access and use this OMAP4430PROC instance.
 *
 *  @param      handlePtr   Return Parameter: Handle to the OMAP4430PROC
 *                          instance
 *  @param      procId      Processor ID represented by this OMAP4430PROC
 *                          instance
 *
 *  @sa         OMAP4430PROC_close, Memory_calloc, OMAP4430PROCDrvUsr_ioctl
 */
Int
OMAP4430PROC_open (Handle * handlePtr, UInt16 procId)
{
    Int                          status = OMAP4430PROC_SUCCESS;
    OMAP4430PROC_Object *        handle = NULL;
    /* UInt32           key; */
    OMAP4430PROC_CmdArgsOpen     cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "OMAP4430PROC_open", handlePtr, procId);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, MultiProc_isValidRemoteProc (procId));
	Osal_printf ("-> OMAP4430PROC_open\n");
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval OMAP4430PROC_E_INVALIDARG Invalid NULL handle pointer
                                            specified*/
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_open",
                             status,
                             "Invalid NULL handle pointer specified");
    }
    else if (!MultiProc_isValidRemoteProc (procId)) {
        *handlePtr = NULL;
        /*! @retval OMAP4430PROC_E_INVALIDARG Invalid procId specified */
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_open",
                             status,
                             "Invalid procId specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (OMAP4430PROC_state.gateHandle); */
        if (OMAP4430PROC_state.procHandles [procId] != NULL) {
            /* If the object is already created/opened in this process, return
             * handle in the local array.
             */
            handle = OMAP4430PROC_state.procHandles [procId];
            handle->openRefCount++;
            status = OMAP4430PROC_S_ALREADYEXISTS;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    OMAP4430PROC_open: Instance already exists in this"
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
            OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_open",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Allocate memory for the handle */
                handle = (OMAP4430PROC_Object *) Memory_calloc (NULL,
                                                   sizeof (OMAP4430PROC_Object),
                                                   0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (handle == NULL) {
                    /*! @retval OMAP4430PROC_E_MEMORY Memory allocation failed
                                                 for handle */
                    status = OMAP4430PROC_E_MEMORY;
                    GT_setFailureReason (curTrace,
                                        GT_4CLASS,
                                        "OMAP4430PROC_open",
                                        status,
                                        "Memory allocation failed for handle!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    /* Set pointer to kernel object into the user handle. */
                    handle->commonObj.knlObject = cmdArgs.handle;
                    /* Store the OMAP4430PROC handle in the local array. */
                    OMAP4430PROC_state.procHandles [procId] = handle;
                    handle->openRefCount = 1;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                 }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }

        *handlePtr = handle;
        /* TBD: Leave critical section protection. */
        /* Gate_leave (OMAP4430PROC_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_open\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_open", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to close this handle to the OMAP4430PROC instance.
 *
 *              This function closes the handle to the OMAP4430PROC instance
 *              obtained through OMAP4430PROC_open call made earlier.
 *
 *  @param      handle     Handle to the OMAP4430PROC object
 *
 *  @sa         OMAP4430PROC_open, Memory_free, OMAP4430PROCDrvUsr_ioctl
 */
Int
OMAP4430PROC_close (OMAP4430PROC_Handle * handlePtr)
{
    Int                       status          = OMAP4430PROC_SUCCESS;
    OMAP4430PROC_Object *     procHandle;
    OMAP4430PROC_CmdArgsClose cmdArgs;

	Osal_printf ("-> OMAP4430PROC_close\n");
    GT_1trace (curTrace, GT_ENTER, "OMAP4430PROC_close", handlePtr);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval OMAP4430PROC_E_INVALIDARG Invalid NULL handlePtr pointer
                                         specified*/
        status = OMAP4430PROC_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_close",
                             status,
                             "Invalid NULL handlePtr pointer specified");
    }
    else if (*handlePtr == NULL) {
        /*! @retval OMAP4430PROC_E_HANDLE Invalid NULL handle specified */
        status = OMAP4430PROC_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OMAP4430PROC_close",
                             status,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (OMAP4430PROC_state.gateHandle); */
        procHandle = (OMAP4430PROC_Object *) (*handlePtr);
        if (procHandle->openRefCount == 0) {
            /*! @retval OMAP4430PROC_E_ACCESSDENIED All open handles to this
                                      OMAP4430PROC object are already closed. */
            status = OMAP4430PROC_E_ACCESSDENIED;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OMAP4430PROC_close",
                                 status,
                                 "All open handles to this OMAP4430PROC object"
                                 " are already closed.");

        }
        else if (procHandle->openRefCount > 1) {
            /* Simply reduce the reference count. There are other threads in
             * this process that have also opened handles to this OMAP4430PROC
             * instance.
             */
            procHandle->openRefCount--;
            status = OMAP4430PROC_S_OPENHANDLE;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    OMAP4430PROC_close: Other handles to this instance"
                       " are still open\n"
                       "        RefCount: [%d]\n",
                       (procHandle->openRefCount + 1));
        }
        else {
            /* The object can be closed now since all open handles are closed.*/
            cmdArgs.handle = procHandle->commonObj.knlObject;
            status = OMAP4430PROCDrvUsr_ioctl (CMD_PROC4430_CLOSE,
                                               &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OMAP4430PROC_close",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

            if (procHandle->created == FALSE) {
                /* Clear the OMAP4430PROC handle in the local array. */
                GT_assert (curTrace,
                           (procHandle->procId < MultiProc_MAXPROCESSORS));
                OMAP4430PROC_state.procHandles [procHandle->procId] = NULL;
                /* Free memory for the handle only if it was not created in
                 * this process.
                 */
                Memory_free (NULL, procHandle, sizeof (OMAP4430PROC_Object));
            }
            *handlePtr = NULL;
        }
        /* TBD: Leave critical section protection. */
        /* Gate_leave (OMAP4430PROC_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

	Osal_printf ("<- OMAP4430PROC_close\n");
    GT_1trace (curTrace, GT_LEAVE, "OMAP4430PROC_close", status);

    /*! @retval OMAP4430PROC_SUCCESS Operation successful */
    return (status);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
