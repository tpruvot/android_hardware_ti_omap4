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
 *  @file   HeapMemMP.c
 *
 *  @brief  Defines HeapMemMP based memory allocator.
 *
 *  Multi-processor variable-size buffer heap implementation
 *
 *  Heap implementation that manages variable size buffers that can be used
 *  in a multiprocessor system with shared memory.
 *
 *  The HeapMemMP manager provides functions to allocate and free storage from a
 *  heap of type HeapMemMP.  The HeapMemMP manager is intended as a memory
 *  manager which can flexibly allocate blocks of various sizes. It is ideal for
 *  managing a heap that does not know beforehand the size of the blocks to be
 *  allocated or when there is expected to be large variation in alloc sizes.
 *
 *  The HeapMemMP module uses a NameServer instance to
 *  store instance information when an instance is created.  The name supplied
 *  must be unique for all HeapMemMP instances.
 *
 *  The create initializes the shared memory as needed. Once an
 *  instance is created, an open can be performed. The
 *  open is used to gain access to the same HeapMemMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor(s).
 *
 *  The open returns a HeapMemMP instance handle like the create,
 *  however the open does not modify the shared memory.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* Utilities headers */
#include <Trace.h>
#include <Memory.h>
#include <String.h>

/* Module level headers */
#include <_GateMP.h>
#include <ti/ipc/GateMP.h>
#include <ti/ipc/HeapMemMP.h>
#include <_HeapMemMP.h>
#include <HeapMemMPDrv.h>
#include <HeapMemMPDrvDefs.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/SharedRegion.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Macros
 * =============================================================================
 */
/*!
 *  @brief  Cache size
 */
#define HEAPMEMMP_CACHESIZE              128u


/*!
 *  @brief  Structure defining object for the HeapMemMP
 */
typedef struct HeapMemMP_Obj_tag {
    Ptr         knlObject;
    /*!< Pointer to the kernel-side HeapMemMP object. */
} HeapMemMP_Obj;

/*!
 *  @brief  Structure for HeapMemMP module state
 */
typedef struct HeapMemMP_ModuleObject_tag {
    UInt32              setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
     *   process.
     */
} HeapMemMP_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    HeapMemMP_state
 *
 *  @brief  Heap Buf state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
HeapMemMP_ModuleObject HeapMemMP_state =
{
    .setupRefCount = 0
};


/* =============================================================================
 *  Internal functions
 * =============================================================================
 */
Int32
_HeapMemMP_create (HeapMemMP_Handle       * heapHandle,
                   HeapMemMPDrv_CmdArgs     cmdArgs,
                   Bool                     createFlag);



/* =============================================================================
 * APIS
 * =============================================================================
 */
/*!
 *  @brief      Get the default configuration for the HeapMemMP module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to HeapMemMP_setup filled in by
 *              the HeapMemMP module with the default parameters. If the
 *              user does not wish to make any change in the default parameters,
 *              this API is not required to be called.
 *
 *  @param      cfgParams  Pointer to the HeapMemMP module configuration
 *                         structure in which the default config is to be
 *                         returned.
 *
 *  @sa         HeapMemMP_setup
 */
Void
HeapMemMP_getConfig (HeapMemMP_Config * cfgParams)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                status = HeapMemMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_getConfig", cfgParams);

    GT_assert (curTrace, (cfgParams != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfgParams == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getConfig",
                             status,
                             "Argument of type (HeapMemMP_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapMemMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.getConfig.config = cfgParams;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "HeapMemMP_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        HeapMemMPDrv_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_ENTER, "HeapMemMP_getConfig");
}


/*!
 *  @brief      Setup the HeapMemMP module.
 *
 *              This function sets up the HeapMemMP module. This function
 *              must be called before any other instance-level APIs can be
 *              invoked.
 *              Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then HeapMemMP_getConfig can be called to get
 *              the configuration filled with the default values. After this,
 *              only the required configuration values can be changed. If the
 *              user does not wish to make any change in the default parameters,
 *              the application can simply call HeapMemMP_setup with NULL
 *              parameters. The default parameters would get automatically used.
 *
 *  @sa        HeapMemMP_destroy
 */
Int
HeapMemMP_setup (const HeapMemMP_Config * config)
{
    Int                  status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_setup", config);

    /* TBD: Protect from multiple threads. */
    HeapMemMP_state.setupRefCount++;

    /* This is needed at runtime so should not be in
     * SYSLINK_BUILD_OPTIMIZE.
     */
    if (HeapMemMP_state.setupRefCount > 1) {
        status = HeapMemMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "HeapMemMP module has been already setup in this process.\n"
                   " RefCount: [%d]\n",
                   HeapMemMP_state.setupRefCount);
    }
    else {
        /* Open the driver handle. */
        status = HeapMemMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.setup.config = (HeapMemMP_Config *) config;
            status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason
                       (curTrace,
                        GT_4CLASS,
                        "HeapMemMP_setup",
                        status,
                        "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_setup", status);

    return status;
}


/*!
 *  @brief      Function to destroy the HeapMemMP module.
 *
 *  @param      None
 *
 *  @sa         HeapMemMP_create
 */
Int
HeapMemMP_destroy (void)
{
    Int                     status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "HeapMemMP_destroy");

    /* TBD: Protect from multiple threads. */
    HeapMemMP_state.setupRefCount--;

    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (HeapMemMP_state.setupRefCount >= 1) {
        status = HeapMemMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "HeapMemMP module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (HeapMemMP_state.setupRefCount + 1));
    }
    else {
        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        HeapMemMPDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_destroy", status);

    return status;
}


/*!
 *  @brief      Initialize this config-params structure with supplier-specified
 *              defaults before instance creation.
 *
 *  @param      handle  Handle to previously created HeapMemMP instance
 *  @param      params  Instance config-params structure.
 *
 *  @sa         HeapMemMP_create
 */
Void
HeapMemMP_Params_init (HeapMemMP_Params       * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                    status = HeapMemMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapMemMPDrv_CmdArgs   cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_Params_init",
                             HeapMemMP_E_INVALIDSTATE,
                             "Modules is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_Params_init",
                             HeapMemMP_E_INVALIDARG,
                             "Argument of type (HeapMemMP_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.ParamsInit.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapMemMP_Params_init");
}


/*!
 *  @brief      Creates a new instance of HeapMemMP module.
 *
 *  @param      params  Instance config-params structure.
 *
 *  @sa         HeapMemMP_delete, HeapMemMP_Params_init
 *              ListMP_sharedMemReq Gate_enter
 *              Gate_leave GateMutex_delete NameServer_addUInt32
 */
HeapMemMP_Handle
HeapMemMP_create (const HeapMemMP_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                  status = HeapMemMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapMemMP_Handle       handle = NULL;
    HeapMemMPDrv_CmdArgs   cmdArgs;
    UInt16                 index;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_create", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
         GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_create",
                             HeapMemMP_E_INVALIDSTATE,
                             "Modules is not initialized!");
    }
    else if (params == NULL) {
         GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_create",
                             HeapMemMP_E_INVALIDARG,
                             "Invalid NULL params pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.create.params = (HeapMemMP_Params *) params;

        /* Translate sharedAddr to SrPtr. */
        GT_1trace (curTrace,
                   GT_2CLASS,
                   "    HeapMemMP_create: Shared addr [0x%x]\n",
                   params->sharedAddr);
        if (params->sharedAddr == NULL) {
            cmdArgs.args.create.sharedAddrSrPtr = SharedRegion_INVALIDSRPTR;
        }
        else {
            index = SharedRegion_getId (params->sharedAddr);
            GT_assert (curTrace, (index != SharedRegion_INVALIDSRPTR));
            cmdArgs.args.create.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            params->sharedAddr,
                                                            index);
            GT_1trace (curTrace,
                       GT_2CLASS,
                       "    HeapMemMP_create: Shared addr SrPtr [0x%x]\n",
                       cmdArgs.args.create.sharedAddrSrPtr);
        }

        if (params->name != NULL) {
            cmdArgs.args.create.nameLen = (String_len (params->name) + 1);
        }
        else {
            cmdArgs.args.create.nameLen = 0;
        }

        /* Translate Gate handle to kernel-side gate handle. */
        if (params->gate != NULL) {
           cmdArgs.args.create.knlGate = (Ptr)
                                 GateMP_getKnlHandle (params->gate);
        }
        else {
            cmdArgs.args.create.knlGate = NULL;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_create",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            _HeapMemMP_create (&handle, cmdArgs, TRUE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMP_create",
                                     status,
                                     "Heap creation failed on user-side!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_create", handle);

    return (HeapMemMP_Handle) handle;
}


/*!
 *  @brief      Deletes an instance of HeapMemMP module.
 *
 *  @param      hpHandle  Handle to previously created instance.
 *
 *  @sa         HeapMemMP_create NameServer_remove Gate_enter
 *              Gate_leave GateMutex_delete ListMP_delete
 */
Int
HeapMemMP_delete (HeapMemMP_Handle * hpHandle)
{
    Int                  status = HeapMemMP_S_SUCCESS;
    HeapMemMP_Obj *      obj    = NULL;
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_delete", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, ((hpHandle != NULL) && (*hpHandle != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_delete",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_delete",
                             status,
                             "handle pointer passed is NULL!");
    }
    else if (*hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_delete",
                             status,
                             "*hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (HeapMemMP_Obj *) (((HeapMemMP_Object *) (*hpHandle))->obj);
        cmdArgs.args.deleteInstance.handle = obj->knlObject;
        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_DELETE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_delete",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            Memory_free (NULL, obj, sizeof (HeapMemMP_Obj));
            Memory_free (NULL, *hpHandle, sizeof (HeapMemMP_Object));
            *hpHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_delete", status);

    return status;
}


/*!
 *  @brief      Opens a created instance of HeapMemMP module.
 *
 *  @param      hpHandle    Return value: HeapMemMP handle
 *  @param      params      Parameters for opening the instance.
 *
 *  @sa         HeapMemMP_create, HeapMemMP_delete, HeapMemMP_close
 */
Int32
HeapMemMP_openByAddr (Ptr                sharedAddr,
                      HeapMemMP_Handle * hpHandle)
{
    Int32                status  = HeapMemMP_S_SUCCESS;
    UInt16               index;
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapMemMP_openByAddr", sharedAddr, hpHandle);

    GT_assert (curTrace, (sharedAddr != NULL));
    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_openByAddr",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (sharedAddr == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_openByAddr",
                             status,
                             "params passed is NULL!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_openByAddr",
                             status,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        index = SharedRegion_getId (sharedAddr);
        cmdArgs.args.openByAddr.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                      sharedAddr,
                                                      index);



        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_OPENBYADDR, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* HeapMemMP_E_NOTFOUND is an expected run-time failure. */
            if (status != HeapMemMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMP_openByAddr",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            status = _HeapMemMP_create (hpHandle, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMP_openByAddr",
                                     status,
                                     "_HeapMemMP_create from "
                                     "HeapMemMP_openByAddr failed!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_openByAddr", status);

    return status;
}

/*!
 *  @brief      Opens a created instance of HeapMemMP module.
 *
 *  @param      hpHandle    Return value: HeapMemMP handle
 *  @param      params      Parameters for opening the instance.
 *
 *  @sa         HeapMemMP_create, HeapMemMP_delete, HeapMemMP_close
 */
Int32
HeapMemMP_open (String             name,
                HeapMemMP_Handle * hpHandle)
{
    Int32                status  = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapMemMP_open", name, hpHandle );

    GT_assert (curTrace, (name != NULL));
    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_open",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (name == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_open",
                             status,
                             "name passed is NULL!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_open",
                             status,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.open.name   = name;

        if (name != NULL) {
            cmdArgs.args.open.nameLen = (String_len (name) + 1);
        }
        else {
            cmdArgs.args.open.nameLen = 0;
        }

        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* HeapMemMP_E_NOTFOUND is an expected run-time failure. */
            if (status != HeapMemMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMP_open",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            status = _HeapMemMP_create (hpHandle, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapMemMP_open",
                                     status,
                                     "_HeapMemMP_create from HeapMemMP_open "
                                     "failed!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_open", status);

    return status;
}

/*!
 *  @brief      Closes previously opened instance of HeapMemMP module.
 *
 *  @param      hpHandle  Pointer to handle to previously opened instance.
 *
 *  @sa         HeapMemMP_create, HeapMemMP_delete, HeapMemMP_open
 */
Int32
HeapMemMP_close (HeapMemMP_Handle * hpHandle)
{
    Int32                status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs cmdArgs;
    HeapMemMP_Obj *      obj;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_close", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, ((hpHandle != NULL)&& (*hpHandle != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_close",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_close",
                             status,
                             "handle pointer passed is NULL!");
    }
    else if (*hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_close",
                             status,
                             "*hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (HeapMemMP_Obj *) (((HeapMemMP_Object *) (*hpHandle))->obj);
        cmdArgs.args.close.handle = obj->knlObject;
        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_CLOSE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_close",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            Memory_free (NULL, obj, sizeof (HeapMemMP_Obj));
            Memory_free (NULL, *hpHandle, sizeof (HeapMemMP_Object));
            *hpHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_close", status);

    return status;
}


/*!
 *  @brief      Allocs a block
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @sa         HeapMemMP_alloc
 */
Void *
HeapMemMP_alloc (HeapMemMP_Handle   hpHandle,
                 UInt32             size,
                 UInt32             align)
{
    Int32                 status     = HeapMemMP_S_SUCCESS;
    Char                * block      = NULL;
    SharedRegion_SRPtr    blockSrPtr = SharedRegion_INVALIDSRPTR;
    HeapMemMPDrv_CmdArgs  cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "HeapMemMP_alloc", hpHandle, size, align);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_alloc",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
         GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_alloc",
                             HeapMemMP_E_INVALIDARG,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.alloc.handle =
            ((HeapMemMP_Obj *) ((HeapMemMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.alloc.size   = size;
        cmdArgs.args.alloc.align  = align;

        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_ALLOC, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_alloc",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            blockSrPtr = cmdArgs.args.alloc.blockSrPtr;
            if (blockSrPtr != SharedRegion_INVALIDSRPTR) {
                block = SharedRegion_getPtr (blockSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_alloc", block);

    return block;
}


/*!
 *  @brief      Frees a block
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         HeapMemMP_alloc ListMP_putTail
 */
Void
HeapMemMP_free (HeapMemMP_Handle   hpHandle,
                Ptr                block,
                UInt32             size)
{
    Int                   status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs  cmdArgs;
    UInt16                index;

    GT_3trace (curTrace, GT_ENTER, "HeapMemMP_free", hpHandle, block, size);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (block != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_free",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_free",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (block == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_free",
                             status,
                             "Invalid NULL block pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.free.handle =
            ((HeapMemMP_Obj *) ((HeapMemMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.free.size   = size;

        /* Translate to SrPtr. */
        index = SharedRegion_getId (block);
        cmdArgs.args.free.blockSrPtr = SharedRegion_getSRPtr (block, index);

        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_FREE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_free",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapMemMP_free");

     return;
}


/*!
 *  @brief      Get memory statistics
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @params     stats     Memory statistics
 *
 *  @sa         HeapMemMP_getExtendedStats
 */
Void
HeapMemMP_getStats (HeapMemMP_Handle  hpHandle,
                    Ptr               stats)
{
    Int                   status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapMemMP_getStats", hpHandle, stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        status = HeapMemMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getStats",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getStats",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (stats == NULL) {
        status = HeapMemMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getStats",
                             status,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getStats.handle =
            ((HeapMemMP_Obj *) ((HeapMemMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.getStats.stats  = (Memory_Stats*) stats;

        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_GETSTATS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "HeapMemMP_getStats",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapMemMP_getStats");

    return ;
}


/*!
 *  @brief      Get extended statistics
 *
 *  @param      hpHandle  Handle to previously created/opened instance.
 *  @params     stats     HeapMemMP statistics
 *
 *  @sa         HeapMemMP_getStats
 *
 */
Void
HeapMemMP_getExtendedStats (HeapMemMP_Handle          hpHandle,
                            HeapMemMP_ExtendedStats * stats)
{
    Int32                 status = HeapMemMP_S_SUCCESS;
    HeapMemMPDrv_CmdArgs  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapMemMP_getExtendedStats", hpHandle,
                stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getExtendedStats",
                             HeapMemMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getExtendedStats",
                             HeapMemMP_E_INVALIDARG,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (stats == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getExtendedStats",
                             HeapMemMP_E_INVALIDARG,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getExtendedStats.handle =
            ((HeapMemMP_Obj *) ((HeapMemMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.getExtendedStats.stats = (HeapMemMP_ExtendedStats *) stats;
        status = HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_GETEXTENDEDSTATS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_getExtendedStats",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapMemMP_getExtendedStats");

    return;
}


/*!
 *  @brief      Returns the shared memory size requirement for a single
 *              instance.
 *
 *  @param      params  Instance creation specific parameters
 *  @param      bufSize Return value: Size for heap buffers
 *
 *  @sa         None
 */
SizeT
HeapMemMP_sharedMemReq (const HeapMemMP_Params * params)
{
    Int                   status    = HeapMemMP_S_SUCCESS;
    SizeT                 stateSize = 0u;
    HeapMemMPDrv_CmdArgs  cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_sharedMemReq", params);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_sharedMemReq",
                             HeapMemMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_sharedMemReq",
                             HeapMemMP_E_INVALIDARG,
                             "Invalid NULL params pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.sharedMemReq.params  = (HeapMemMP_Params *)params;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_SHAREDMEMREQ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_getExtendedStats",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            stateSize = cmdArgs.args.sharedMemReq.bytes;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_sharedMemReq", stateSize);

    return (stateSize);
}


/*!
 *  @brief      Returns the HeapMemMP kernel object pointer.
 *
 *  @param      handle  Handle to previousely created/opened instance.
 *
 */
Void *
HeapMemMP_getKnlHandle (HeapMemMP_Handle hpHandle)
{
    Ptr     objPtr = NULL;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_getKnlHandle", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getKnlHandle",
                             HeapMemMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getKnlHandle",
                             HeapMemMP_E_INVALIDARG,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        objPtr = ((HeapMemMP_Obj *)
                            (((HeapMemMP_Object *) hpHandle)->obj))->knlObject;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (objPtr == NULL) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_getKnlHandle",
                                 HeapMemMP_E_INVALIDSTATE,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_getKnlHandle", objPtr);

    return (objPtr);
}


/*!
 *  @brief      Returns the HeapMemMP kernel object pointer.
 *
 *  @param      handle  Handle to kernel handle for which user handle is to be
 *                      provided.
 *
 */
Void *
HeapMemMP_getUsrHandle (HeapMemMP_Handle hpHandle)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int status = HeapMemMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Ptr objPtr = NULL;
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapMemMP_getUsrHandle", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapMemMP_state.setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getUsrHandle",
                             HeapMemMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_getUsrHandle",
                             HeapMemMP_E_INVALIDARG,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.open.handle = hpHandle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        _HeapMemMP_create ((HeapMemMP_Handle *) &objPtr,
                           cmdArgs,
                           FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_getUsrHandle",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapMemMP_getUsrHandle", objPtr);

    return (objPtr);
}


/* =============================================================================
 * Internal function
 * =============================================================================
 */
/*!
 *  @brief      Creates and populates handle and obj.
 *
 *  @param      heapHandle    Return value: Handle
 *  @param      cmdArgs       command areguments
 *  @param      createFlag    Indicates whether this is a create or open call.
 *
 *  @sa         ListMP_delete
 */
Int32
_HeapMemMP_create (HeapMemMP_Handle     * heapHandle,
                   HeapMemMPDrv_CmdArgs   cmdArgs,
                   Bool                   createFlag)
{
    Int32         status = HeapMemMP_S_SUCCESS;
    HeapMemMP_Obj * obj;

    /* No need for parameter checks, since this is an internal function. */

    /* Allocate memory for the handle */
    *heapHandle = (HeapMemMP_Handle) Memory_calloc (NULL,
                                                    sizeof (HeapMemMP_Object),
                                                    0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (*heapHandle == NULL) {
        status = HeapMemMP_E_MEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_create",
                             status,
                             "Memory allocation failed for handle!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Set pointer to kernel object into the user handle. */
        obj = (HeapMemMP_Obj *) Memory_calloc (NULL,
                                               sizeof (HeapMemMP_Obj),
                                               0);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (obj == NULL) {
            Memory_free (NULL, *heapHandle, sizeof (HeapMemMP_Object));
            *heapHandle = NULL;
            status = HeapMemMP_E_MEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_create",
                                 status,
                                 "Memory allocation failed for obj!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (createFlag == TRUE){
                obj->knlObject = cmdArgs.args.create.handle;
            }
            else{
                obj->knlObject = cmdArgs.args.open.handle;
            }

            ((HeapMemMP_Object *)(*heapHandle))->obj = obj;
            ((HeapMemMP_Object *)(*heapHandle))->alloc        = \
                                (IHeap_allocFxn) &HeapMemMP_alloc;
            ((HeapMemMP_Object *)(*heapHandle))->free         = \
                                (IHeap_freeFxn) &HeapMemMP_free;
            ((HeapMemMP_Object *)(*heapHandle))->getStats     = \
                                (IHeap_getStatsFxn) &HeapMemMP_getStats;
            ((HeapMemMP_Object *)(*heapHandle))->getKnlHandle = \
                                (IHeap_getKnlHandleFxn) &HeapMemMP_getKnlHandle;
            ((HeapMemMP_Object *)(*heapHandle))->isBlocking = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    return (status);
}


/*!
 *  @brief      Restore an instance to it's original created state.
 *
 *  @param      handle    Handle to previously created/opened instance.
 */
Void
HeapMemMP_restore (HeapMemMP_Handle handle)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int32                status  =  HeapMemMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapMemMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "Heap_restore", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapMemMP_restore",
                             HeapMemMP_E_INVALIDARG,
                             "handle passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.restore.handle = handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapMemMPDrv_ioctl (CMD_HEAPMEMMP_RESTORE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapMemMP_restore",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapMemMP_restore");

    return;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
