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
 *  @file   HeapBufMP.c
 *
 *  @brief  Defines HeapBufMP based memory allocator.
 *
 *  Multi-processor fixed-size buffer heap implementation
 *
 *  Heap implementation that manages fixed size buffers that can be used
 *  in a multiprocessor system with shared memory.
 *
 *  The HeapBufMP manager provides functions to allocate and free storage from a
 *  heap of type HeapBufMP. HeapBufMP manages a single fixed-size buffer,
 *  split into equally sized allocatable blocks.
 *
 *  The HeapBufMP manager is intended as a very fast memory
 *  manager which can only allocate blocks of a single size. It is ideal for
 *  managing a heap that is only used for allocating a single type of object,
 *  or for objects that have very similar sizes.
 *
 *  The HeapBufMP module uses a NameServer instance to
 *  store instance information when an instance is created.  The name supplied
 *  must be unique for all HeapBufMP instances.
 *
 *  The create initializes the shared memory as needed. Once an
 *  instance is created, an open can be performed. The
 *  open is used to gain access to the same HeapBufMP instance.
 *  Generally an instance is created on one processor and opened on the
 *  other processor(s).
 *
 *  The open returns a HeapBufMP instance handle like the create,
 *  however the open does not modify the shared memory.
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* Utilities headers */
#include <Trace.h>
#include <Memory.h>
#include <String.h>

/* Module level headers */
#include <ti/ipc/HeapBufMP.h>
#include <_HeapBufMP.h>
#include <HeapBufMPDrv.h>
#include <HeapBufMPDrvDefs.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/SharedRegion.h>
#include <_GateMP.h>


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
#define HEAPBUFMP_CACHESIZE              128u

/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining object for the HeapBufMP
 */
typedef struct HeapBufMP_Obj_tag {
    Ptr         knlObject;
    /*!< Pointer to the kernel-side HeapBufMP object. */
} HeapBufMP_Obj;

/*!
 *  @brief  Structure for HeapBufMP module state
 */
typedef struct HeapBufMP_ModuleObject_tag {
    UInt32              setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
     *   process.
     */
} HeapBufMP_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    HeapBufMP_state
 *
 *  @brief  Heap Buf state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
HeapBufMP_ModuleObject HeapBufMP_state =
{
    .setupRefCount = 0
};


/*!
 *  @brief  Pointer to the HeapBufMP module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
HeapBufMP_ModuleObject * HeapBufMP_module = &HeapBufMP_state;


/* =============================================================================
 *  Internal functions
 * =============================================================================
 */
Int32
_HeapBufMP_create (HeapBufMP_Handle       * heapHandle,
                   HeapBufMPDrv_CmdArgs     cmdArgs,
                   Bool                     createFlag);



/* =============================================================================
 * APIS
 * =============================================================================
 */
/*
 *  Get the default configuration for the HeapBufMP module.
 *
 *  This function can be called by the application to get their
 *  configuration parameter to HeapBufMP_setup filled in by
 *  the HeapBufMP module with the default parameters. If the
 *  user does not wish to make any change in the default parameters,
 *  this API is not required to be called.
 *
 */
Void
HeapBufMP_getConfig (HeapBufMP_Config * cfgParams)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                status = HeapBufMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapBufMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_getConfig", cfgParams);

    GT_assert (curTrace, (cfgParams != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfgParams == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getConfig",
                             status,
                             "Argument of type (HeapBufMP_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapBufMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.getConfig.config = cfgParams;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "HeapBufMP_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        HeapBufMPDrv_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapBufMP_getConfig");
}


/*
 *  Setup the HeapBufMP module.
 *
 *  This function sets up the HeapBufMP module. This function
 *  must be called before any other instance-level APIs can be
 *  invoked.
 *  Module-level configuration needs to be provided to this
 *  function. If the user wishes to change some specific config
 *  parameters, then HeapBufMP_getConfig can be called to get
 *  the configuration filled with the default values. After this,
 *  only the required configuration values can be changed. If the
 *  user does not wish to make any change in the default parameters,
 *  the application can simply call HeapBufMP_setup with NULL
 *  parameters. The default parameters would get automatically used.
 *
 */
Int
HeapBufMP_setup (const HeapBufMP_Config * config)
{
    Int                status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_setup", config);

    /* TBD: Protect from multiple threads. */
    HeapBufMP_module->setupRefCount++;

    /* This is needed at runtime so should not be in
     * SYSLINK_BUILD_OPTIMIZE.
     */
    if (HeapBufMP_module->setupRefCount > 1) {
        status = HeapBufMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                GT_1CLASS,
                "HeapBufMP module has been already setup in this process.\n"
                " RefCount: [%d]\n",
                HeapBufMP_module->setupRefCount);
    }
    else {
        /* Open the driver handle. */
        status = HeapBufMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.setup.config = (HeapBufMP_Config *) config;
            status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason
                       (curTrace,
                        GT_4CLASS,
                        "HeapBufMP_setup",
                        status,
                        "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_setup", status);

    return status;
}


/*
 *  Function to destroy the HeapBufMP module.
 */
Int
HeapBufMP_destroy (void)
{
    Int                status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "HeapBufMP_destroy");

    /* TBD: Protect from multiple threads. */
    HeapBufMP_module->setupRefCount--;

    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (HeapBufMP_module->setupRefCount >= 1) {
        status = HeapBufMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "HeapBufMP module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (HeapBufMP_module->setupRefCount + 1));
    }
    else {
        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        HeapBufMPDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_destroy", status);

    return status;
}


/*
 *  Initialize this config-params structure with supplier-specified
 *  defaults before instance creation.
 */
Void
HeapBufMP_Params_init (HeapBufMP_Params       * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int               status = HeapBufMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapBufMPDrv_CmdArgs   cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_Params_init",
                             HeapBufMP_E_INVALIDSTATE,
                             "Modules is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_Params_init",
                             HeapBufMP_E_INVALIDARG,
                             "Argument of type (HeapBufMP_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.ParamsInit.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapBufMP_Params_init");
}


/*
 *  Creates a new instance of HeapBufMP module.
 */
HeapBufMP_Handle
HeapBufMP_create (const HeapBufMP_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                  status = 0;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    HeapBufMP_Handle       handle = NULL;
    HeapBufMPDrv_CmdArgs   cmdArgs;
    UInt16                 index;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_create", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_create",
                             HeapBufMP_E_INVALIDSTATE,
                             "Modules is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_create",
                             HeapBufMP_E_INVALIDARG,
                             "Invalid NULL params pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.create.params = (HeapBufMP_Params *) params;

        /* Translate sharedAddr to SrPtr. */
        GT_1trace (curTrace,
                   GT_2CLASS,
                   "    HeapBufMP_create: Shared addr [0x%x]\n",
                   params->sharedAddr);
        index = SharedRegion_getId (params->sharedAddr);
        cmdArgs.args.create.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            params->sharedAddr,
                                                            index);
        GT_1trace (curTrace,
                   GT_2CLASS,
                   "    HeapBufMP_create: Shared addr SrPtr [0x%x]\n",
                   cmdArgs.args.create.sharedAddrSrPtr);

        if (params->name != NULL) {
            cmdArgs.args.create.nameLen = (String_len (params->name) + 1);
        }
        else {
            cmdArgs.args.create.nameLen = 0;
        }

        /* Translate Gate handle to kernel-side gate handle. */
        if (params->gate != NULL) {
            cmdArgs.args.create.knlGate = GateMP_getKnlHandle (params->gate);
        }
        else {
            cmdArgs.args.create.knlGate = NULL;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_create",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            _HeapBufMP_create (&handle, cmdArgs, TRUE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapBufMP_create",
                                     status,
                                     "Heap creation failed on user-side!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_create", handle);

    return (HeapBufMP_Handle) handle;
}


/*
 *  Deletes an instance of HeapBufMP module.
 */
Int
HeapBufMP_delete (HeapBufMP_Handle * hpHandle)
{
    Int                status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs cmdArgs;
    HeapBufMP_Obj *      obj;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_delete", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, ((hpHandle != NULL) && (*hpHandle != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_delete",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_delete",
                             status,
                             "handle pointer passed is NULL!");
    }
    else if (*hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_delete",
                             status,
                             "*hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (HeapBufMP_Obj *) (((HeapBufMP_Object *) (*hpHandle))->obj);
        cmdArgs.args.deleteInstance.handle = obj->knlObject;
        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_DELETE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_delete",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            Memory_free (NULL, obj, sizeof (HeapBufMP_Obj));
            Memory_free (NULL, *hpHandle, sizeof (HeapBufMP_Object));
            *hpHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_delete", status);

    return status;
}


/*
 *  Opens a created instance of HeapBufMP module.
 */
Int32
HeapBufMP_openByAddr (Ptr                sharedAddr,
                      HeapBufMP_Handle * hpHandle)
{
    Int32                status  = HeapBufMP_S_SUCCESS;
    UInt16                index;
    HeapBufMPDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapBufMP_openByAddr", sharedAddr, hpHandle);

    GT_assert (curTrace, (sharedAddr != NULL));
    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_openByAddr",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (sharedAddr == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_openByAddr",
                             status,
                             "params passed is NULL!");
    }
    else if (hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_openByAddr",
                             status,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        index = SharedRegion_getId (sharedAddr);
        cmdArgs.args.openByAddr.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                      sharedAddr,
                                                      index);


        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_OPENBYADDR, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* HeapBufMP_E_NOTFOUND is an expected run-time failure. */
            if (status != HeapBufMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_openByAddr",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            status = _HeapBufMP_create (hpHandle, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapBufMP_openByAddr",
                                     status,
                                   "_HeapBufMP_create from HeapBufMP_openByAddr"
                                   " failed!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_openByAddr", status);

    return status;
}

/*
 *  Opens a created instance of HeapBufMP module.
 *
 */
Int32
HeapBufMP_open (String             name,
                HeapBufMP_Handle * hpHandle)
{
    Int32                status  = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapBufMP_open", name, hpHandle);

    GT_assert (curTrace, (name != NULL));
    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_open",
                             status,
                             "Modules is in an invalid state!");
    }
    else if (name == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_open",
                             status,
                             "name passed is NULL!");
    }
    else if (hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_open",
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

        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* HeapBufMP_E_NOTFOUND is an expected run-time failure. */
            if (status != HeapBufMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_open",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            status = _HeapBufMP_create (hpHandle, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "HeapBufMP_open",
                                     status,
                                   "_HeapBufMP_create from HeapBufMP_open "
                                   "failed!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_open", status);

    return status;
}

/*
 *  Closes previously opened instance of HeapBufMP module.
 */
Int32
HeapBufMP_close (HeapBufMP_Handle * hpHandle)
{
    Int32              status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs cmdArgs;
    HeapBufMP_Obj *      obj;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_close", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, ((hpHandle != NULL)&& (*hpHandle != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_close",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_close",
                             status,
                             "handle pointer passed is NULL!");
    }
    else if (*hpHandle == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_close",
                             status,
                             "*hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (HeapBufMP_Obj *) (((HeapBufMP_Object *) (*hpHandle))->obj);
        cmdArgs.args.close.handle = obj->knlObject;
        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_CLOSE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_close",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            Memory_free (NULL, obj, sizeof (HeapBufMP_Obj));
            Memory_free (NULL, *hpHandle, sizeof (HeapBufMP_Object));
            *hpHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_close", status);

    return status;
}


/*
 *  Allocs a block
 *
 */
Void *
HeapBufMP_alloc (HeapBufMP_Handle   hpHandle,
               UInt32           size,
               UInt32           align)
{
    Int32               status     = HeapBufMP_S_SUCCESS;
    Char           *    block      = NULL;
    SharedRegion_SRPtr  blockSrPtr = SharedRegion_INVALIDSRPTR;
    HeapBufMPDrv_CmdArgs  cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "HeapBufMP_alloc", hpHandle, size, align);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_alloc",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_alloc",
                             HeapBufMP_E_INVALIDARG,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.alloc.handle =
            ((HeapBufMP_Obj *) ((HeapBufMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.alloc.size   = size;
        cmdArgs.args.alloc.align  = align;

        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_ALLOC, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_alloc",
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

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_alloc", block);

     return block;
}


/*
 *  Frees a block
 */
Void
HeapBufMP_free (HeapBufMP_Handle   hpHandle,
                Ptr                block,
                UInt32             size)
{
    Int                   status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs  cmdArgs;
    UInt16                index;

    GT_3trace (curTrace, GT_ENTER, "HeapBufMP_free", hpHandle, block, size);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (block != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
         status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_free",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
         status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_free",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (block == NULL) {
         status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_free",
                             status,
                             "Invalid NULL block pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.free.handle =
            ((HeapBufMP_Obj *) ((HeapBufMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.free.size   = size;

        /* Translate to SrPtr. */
        index = SharedRegion_getId (block);
        cmdArgs.args.free.blockSrPtr = SharedRegion_getSRPtr (block, index);

        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_FREE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_free",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapBufMP_free");

     return;
}


/*
 *  Get memory statistics
 */
Void
HeapBufMP_getStats (HeapBufMP_Handle  hpHandle,
                    Ptr               stats)
{
    Int              status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapBufMP_getStats", hpHandle, stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
         status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getStats",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
         status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getStats",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (stats == NULL) {
         status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getStats",
                             status,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getStats.handle =
            ((HeapBufMP_Obj *) ((HeapBufMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.getStats.stats  = (Memory_Stats*) stats;

        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_GETSTATS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "HeapBufMP_getStats",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapBufMP_getStats");

     return;
}


/*
 *  Indicate whether the heap may block during an alloc or free call
 */
Bool
HeapBufMP_isBlocking (HeapBufMP_Handle handle)
{
    Bool isBlocking = FALSE;

    GT_1trace (curTrace, GT_ENTER, "Heap_isBlocking", handle);

    GT_assert (curTrace, (handle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_isBlocking",
                             HeapBufMP_E_INVALIDARG,
                             "handle passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Figure out how to determine whether the gate is blocking */
        isBlocking = TRUE;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_isBlocking", isBlocking);

     return isBlocking;
}


/*
 *  Get extended statistics
 */
Void
HeapBufMP_getExtendedStats (HeapBufMP_Handle       hpHandle,
                            HeapBufMP_ExtendedStats * stats)
{
    Int              status = HeapBufMP_S_SUCCESS;
    HeapBufMPDrv_CmdArgs  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "HeapBufMP_getExtendedStats", hpHandle,
                stats);

    GT_assert (curTrace, (hpHandle != NULL));
    GT_assert (curTrace, (stats != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
         status = HeapBufMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getExtendedStats",
                             status,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
         status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getExtendedStats",
                             status,
                             "Invalid NULL hpHandle pointer specified!");
    }
    else if (stats == NULL) {
        status = HeapBufMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getExtendedStats",
                             status,
                             "Invalid NULL stats pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getExtendedStats.handle =
            ((HeapBufMP_Obj *) ((HeapBufMP_Object *) hpHandle)->obj)->knlObject;
        cmdArgs.args.getExtendedStats.stats = (HeapBufMP_ExtendedStats *) stats;
        status = HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_GETEXTENDEDSTATS, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "HeapBufMP_getExtendedStats",
                    status,
                    "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "HeapBufMP_getExtendedStats");

    return;
}


/*
 *  Returns the shared memory size requirement for a single
 *              instance.
 */
SizeT
HeapBufMP_sharedMemReq (const HeapBufMP_Params * params)
{
    Int                 status = HeapBufMP_S_SUCCESS;
    SizeT               stateSize = 0u;
    HeapBufMPDrv_CmdArgs  cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_sharedMemReq", params);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_sharedMemReq",
                             HeapBufMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_sharedMemReq",
                             HeapBufMP_E_INVALIDARG,
                             "Invalid NULL params pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.sharedMemReq.params  = (HeapBufMP_Params *)params;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        HeapBufMPDrv_ioctl (CMD_HEAPBUFMP_SHAREDMEMREQ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                    GT_4CLASS,
                    "HeapBufMP_getExtendedStats",
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

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_sharedMemReq", stateSize);

     return (stateSize);
}


/*
 *  Returns the HeapBufMP kernel object pointer.
 */
Void *
HeapBufMP_getKnlHandle (HeapBufMP_Handle hpHandle)
{
    Ptr     objPtr = NULL;

    GT_1trace (curTrace, GT_ENTER, "HeapBufMP_getKnlHandle", hpHandle);

    GT_assert (curTrace, (hpHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (HeapBufMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getKnlHandle",
                             HeapBufMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (hpHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_getKnlHandle",
                             HeapBufMP_E_INVALIDARG,
                             "hpHandle passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        objPtr = ((HeapBufMP_Obj *)
                            (((HeapBufMP_Object *) hpHandle)->obj))->knlObject;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (objPtr == NULL) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_getKnlHandle",
                                 HeapBufMP_E_INVALIDSTATE,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "HeapBufMP_getKnlHandle", objPtr);

    return (objPtr);
}


/* =============================================================================
 * Internal function
 * =============================================================================
 */
/*
 *  Creates and populates handle and obj.
 *
 */
Int32
_HeapBufMP_create (HeapBufMP_Handle     * heapHandle,
                   HeapBufMPDrv_CmdArgs   cmdArgs,
                   Bool                   createFlag)
{
    Int32         status = HeapBufMP_S_SUCCESS;
    HeapBufMP_Obj * obj;

    /* No need for parameter checks, since this is an internal function. */

    /* Allocate memory for the handle */
    *heapHandle = (HeapBufMP_Handle) Memory_calloc (NULL,
                                                  sizeof (HeapBufMP_Object),
                                                  0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (*heapHandle == NULL) {
         status = HeapBufMP_E_MEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "HeapBufMP_create",
                             status,
                             "Memory allocation failed for handle!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Set pointer to kernel object into the user handle. */
        obj = (HeapBufMP_Obj *) Memory_calloc (NULL,
                                             sizeof (HeapBufMP_Obj),
                                             0);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (obj == NULL) {
            Memory_free (NULL, *heapHandle, sizeof (HeapBufMP_Object));
            *heapHandle = NULL;
             status = HeapBufMP_E_MEMORY;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "HeapBufMP_create",
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

            ((HeapBufMP_Object *)(*heapHandle))->obj = obj;
            ((HeapBufMP_Object *)(*heapHandle))->alloc        = \
                                (IHeap_allocFxn) &HeapBufMP_alloc;
            ((HeapBufMP_Object *)(*heapHandle))->free         = \
                                (IHeap_freeFxn) &HeapBufMP_free;
            ((HeapBufMP_Object *)(*heapHandle))->getStats     = \
                                (IHeap_getStatsFxn) &HeapBufMP_getStats;
            ((HeapBufMP_Object *)(*heapHandle))->getKnlHandle = \
                                (IHeap_getKnlHandleFxn) &HeapBufMP_getKnlHandle;
            ((HeapBufMP_Object *)(*heapHandle))->isBlocking   = \
                                (IHeap_isBlockingFxn)&HeapBufMP_isBlocking;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    return (status);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
