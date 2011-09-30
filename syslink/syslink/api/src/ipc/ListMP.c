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
 *  @file   ListMP.c
 *
 *  @brief      Defines for shared memory doubly linked list.
 *
 *              This module implements the ListMP.
 *              ListMP is a linked-list based module designed to be
 *              used in a multi-processor environment.  It is designed to
 *              provide a means of communication between different processors.
 *              processors.
 *              This module is instance based. Each instance requires a small
 *              piece of shared memory. This is specified via the #sharedAddr
 *              parameter to the create. The proper #sharedAddrSize parameter
 *              can be determined via the #sharedMemReq call. Note: the
 *              parameters to this function must be the same that will used to
 *              create or open the instance.
 *              The ListMP module uses a #NameServer instance
 *              to store instance information when an instance is created and
 *              the name parameter is non-NULL. If a name is supplied, it must
 *              be unique for all ListMP instances.
 *              The #create also initializes the shared memory as needed. The
 *              shared memory must be initialized to 0 or all ones
 *              (e.g. 0xFFFFFFFFF) before the ListMP instance
 *              is created.
 *              Once an instance is created, an open can be performed. The #open
 *              is used to gain access to the same ListMP instance.
 *              Generally an instance is created on one processor and opened
 *              on the other processor.
 *              The open returns a ListMP instance handle like the
 *              create, however the open does not modify the shared memory.
 *              Generally an instance is created on one processor and opened
 *              on the other processor.
 *              There are two options when opening the instance:
 *              @li Supply the same name as specified in the create. The
 *              ListMP module queries the NameServer to get the
 *              needed information.
 *              @li Supply the same #sharedAddr value as specified in the
 *              create.
 *              If the open is called before the instance is created, open
 *              returns NULL.
 *              There is currently a list of restrictions for the module:
 *              @li Both processors must have the same endianness. Endianness
 *              conversion may supported in a future version of
 *              ListMP.
 *              @li The module will be made a gated module
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* Osal And Utils  headers */
#include <String.h>
#include <Trace.h>
#include <Memory.h>
#include <String.h>
#include <_GateMP.h>
#include <ti/ipc/GateMP.h>
#include <_SharedRegion.h>

/* Module level headers */
#include <_ListMP.h>
#include <ti/ipc/ListMP.h>
#include <ListMPDrv.h>
#include <ListMPDrvDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 * Globals
 * =============================================================================
 */
/*!
 *  @brief  Cache size
 */
#define ListMP_CACHESIZE   128u
/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/* Structure defining object for the Gate */
typedef struct ListMP_Object_tag {
    Ptr         knlObject;
    /*!< Pointer to the kernel-side ListMP object. */
} ListMP_Object;

/*!
 *  @brief  ListMP Module state object
 */
typedef struct ListMP_ModuleObject_tag {
    UInt32                    setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
} ListMP_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    ListMP_state
 *
 *  @brief  ListMP state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
ListMP_ModuleObject ListMP_state =
{
    .setupRefCount = 0
};

/*!
 *  @var    ListMP_module
 *
 *  @brief  Pointer to the ListMP module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
ListMP_ModuleObject * ListMP_module = &ListMP_state;


/* =============================================================================
 *  Internal functions
 * =============================================================================
 */
Int32
 _ListMP_create(ListMP_Handle       * listMPHandle,
                ListMPDrv_CmdArgs     cmdArgs,
                UInt16                createFlag);


/* =============================================================================
 * APIS
 * =============================================================================
 */
/*
 *  Function to get configuration parameters to setup
 *  the ListMP module.
 */
Void
ListMP_getConfig (ListMP_Config * cfgParams)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int               status = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ListMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ListMP_getConfig", cfgParams);

    GT_assert (curTrace, (cfgParams != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfgParams == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_getConfig",
                             ListMP_E_INVALIDARG,
                             "Argument of type (ListMP_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.getConfig.config = cfgParams;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            ListMPDrv_ioctl (CMD_LISTMP_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "ListMP_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Close the driver handle. */
        ListMPDrv_close ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ListMP_getConfig");
}


/*
 *  Function to setup the ListMP module.
 */
Int
ListMP_setup (ListMP_Config * config)
{
    Int               status = ListMP_S_SUCCESS;
    ListMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ListMP_setup", config);

    /* TBD: Protect from multiple threads. */
    ListMP_module->setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (ListMP_module->setupRefCount > 1) {
        status = ListMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "ListMP module has been already setup in this"
                   " process.\n RefCount: [%d]\n",
                   ListMP_module->setupRefCount);
    }
    else {
        /* Open the driver handle. */
        status = ListMPDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.args.setup.config = (ListMP_Config *) config;
            status = ListMPDrv_ioctl (CMD_LISTMP_SETUP,
                                      &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ListMP_setup",
                                     status,
                                     "API (through IOCTL) failed"
                                     " on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "ListMP_setup", status);

    return status;
}


/*
 *  Function to destroy the ListMP module.
 */
Int
ListMP_destroy (void)
{
    Int                           status = ListMP_S_SUCCESS;
    ListMPDrv_CmdArgs    cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "ListMP_destroy");

    /* TBD: Protect from multiple threads. */
    ListMP_module->setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (ListMP_module->setupRefCount >= 1) {
        status = ListMP_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "ListMP module has been setup by other clients"
                   " in this process.\n"
                   "    RefCount: [%d]\n",
                   ListMP_module->setupRefCount);
    }
    else {
        status = ListMPDrv_ioctl (CMD_LISTMP_DESTROY,
                                  &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        ListMPDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "ListMP_destroy", status);

    return status;
}


/*
 *  Initialize this config-params structure with supplier-specified
 *  defaults before instance creation.
 */
Void
ListMP_Params_init (ListMP_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                  status = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ListMPDrv_CmdArgs    cmdArgs;

    GT_1trace (curTrace,
               GT_ENTER,
               "ListMP_Params_init",
               params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_Params_init",
                             ListMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_Params_init",
                             ListMP_E_INVALIDARG,
                             "Argument of type (ListMP_Params *) "
                             " passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.ParamsInit.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_PARAMS_INIT,
                         &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "ListMP_Params_init");

    return;
}


/*
 *  Creates a new instance of ListMP module.
 */
ListMP_Handle
ListMP_create (const ListMP_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int               status = 0;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ListMP_Handle     handle = NULL;
    ListMPDrv_CmdArgs cmdArgs;
    UInt16            index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_create", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_create",
                             ListMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_create",
                             ListMP_E_INVALIDARG,
                             "Params passed is NULL!");
    }
    else if ((params->sharedAddr == (UInt32)NULL)
             &&(params->regionId == SharedRegion_INVALIDREGIONID)) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_create",
                             ListMP_E_INVALIDARG,
                             "Please provide a valid shared region "
                             "address or Region Id!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.create.params = (ListMP_Params *) params;

        /* Translate sharedAddr to SrPtr. */
        GT_1trace (curTrace,
                   GT_2CLASS,
                   "    ListMP_create: Shared addr [0x%x]\n",
                   params->sharedAddr);
        if  (params->sharedAddr != NULL) {
            index = SharedRegion_getId (params->sharedAddr);
            cmdArgs.args.create.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            params->sharedAddr,
                                                            index);
        }
        else {
            cmdArgs.args.create.sharedAddrSrPtr = SharedRegion_INVALIDSRPTR;
        }
        GT_1trace (curTrace,
                   GT_2CLASS,
                   "    ListMP_create: "
                   "Shared buffer addr SrPtr [0x%x]\n",
                   cmdArgs.args.create.sharedAddrSrPtr);

        /* Translate Gate handle to kernel-side gate handle. */
        if (params->gate != NULL) {
            cmdArgs.args.create.knlGate = GateMP_getKnlHandle (params->gate);
        }
        else {
            cmdArgs.args.create.knlGate = NULL;
        }
        if (params->name != NULL) {
            cmdArgs.args.create.nameLen = (String_len (params->name)+ 1u);
        }
        else {
            cmdArgs.args.create.nameLen = 0;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_CREATE,
                                     &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_create",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            _ListMP_create (&handle, cmdArgs, TRUE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "ListMP_create",
                            status,
                            "ListMP creation failed on user-side!");
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_create", handle);

    return handle;
}


/*
 *  Deletes a instance of ListMP module.
 */
Int
ListMP_delete (ListMP_Handle * handlePtr)
{
    Int               status = ListMP_S_SUCCESS;
    ListMP_Object   * obj    = NULL;
    ListMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ListMP_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_delete",
                             status,
                             "Module is not initialized!");
    }
    else if (handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_delete",
                             status,
                             "handlePtr passed is NULL!");
    }
    else if (*handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_delete",
                             status,
                             "*handlePtr passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) (*handlePtr);
        cmdArgs.args.deleteInstance.handle = obj->knlObject;

        status = ListMPDrv_ioctl (CMD_LISTMP_DELETE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_delete",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        Memory_free (NULL,
                     (*handlePtr),
                     sizeof (ListMP_Object));
        *handlePtr = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_delete", status);

    return status;
}


/*
 *  Amount of shared memory required for creation of each instance.
 */
SizeT
ListMP_sharedMemReq (const ListMP_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int32                 status = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    SizeT                 retVal = 0u;
    UInt16                index;
    ListMPDrv_CmdArgs     cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ListMP_sharedMemReq", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
   if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_sharedMemReq",
                             ListMP_E_INVALIDARG,
                             "params passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        index = SharedRegion_getId (params->sharedAddr);
        cmdArgs.args.sharedMemReq.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            params->sharedAddr,
                                                            index);
        if (params->gate != NULL) {
            cmdArgs.args.sharedMemReq.knlGate =
                                        GateMP_getKnlHandle (params->gate);
        }

        if (params->name != NULL) {
            cmdArgs.args.sharedMemReq.nameLen = (String_len (params->name)+ 1u);
        }
        else {
            cmdArgs.args.sharedMemReq.nameLen = 0;
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_SHAREDMEMREQ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_sharedMemReq",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        retVal = cmdArgs.args.sharedMemReq.bytes;

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_sharedMemReq", retVal);

    return retVal;
}


/*
 *  Function to open an ListMP instance
 */
Int ListMP_open (String            name,
                 ListMP_Handle   * handlePtr)
{
    Int               status = ListMP_S_SUCCESS;
    ListMPDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace,
               GT_ENTER,
               "ListMP_open",
               name,
               handlePtr);

    GT_assert (curTrace, (name != NULL));
    GT_assert (curTrace, (handlePtr != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_open",
                             status,
                             "Module is not initialized!");
    }
    else if (name == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_open",
                             status,
                             "name passed is NULL!");
    }
    else if (handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_open",
                             status,
                             "Invalid NULL handlePtr specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (name != NULL) {
            cmdArgs.args.open.nameLen = (String_len (name)+ 1u);
        }
        else {
            cmdArgs.args.open.nameLen = 0;
        }
        cmdArgs.args.open.name = name;
        status = ListMPDrv_ioctl (CMD_LISTMP_OPEN, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* ListMP_E_NOTFOUND is an expected run-time failure. */
            if (status != ListMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ListMP_open",
                                     status,
                                     "API (through IOCTL) failed on "
                                     "kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
           status = _ListMP_create (handlePtr, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_open", status);

    return status;
}


/*
 *  Function to open an ListMP instance
 */
Int ListMP_openByAddr (Ptr               sharedAddr,
                       ListMP_Handle   * handlePtr)
{
    Int               status = ListMP_S_SUCCESS;
    ListMPDrv_CmdArgs cmdArgs;
    UInt16            index;

    GT_2trace (curTrace,
               GT_ENTER,
               "ListMP_openByAddr",
               sharedAddr,
               handlePtr);

    GT_assert (curTrace, (sharedAddr != NULL));
    GT_assert (curTrace, (handlePtr  != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_openByAddr",
                             status,
                             "Module is not initialized!");
    }
    else if (handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_openByAddr",
                             status,
                             "Invalid NULL handlePtr specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        index = SharedRegion_getId (sharedAddr);
        cmdArgs.args.openByAddr.sharedAddrSrPtr = SharedRegion_getSRPtr (
                                                            sharedAddr,
                                                            index);
        status = ListMPDrv_ioctl (CMD_LISTMP_OPENBYADDR,
                                  &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            /* ListMP_E_NOTFOUND is an expected run-time failure. */
            if (status != ListMP_E_NOTFOUND) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "ListMP_openByAddr",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
           status = _ListMP_create (handlePtr, cmdArgs, FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_openByAddr", status);

    return status;
}


/*
 *  Function to close a previously opened instance
 */
Int ListMP_close (ListMP_Handle * handlePtr)
{
    Int               status = ListMP_S_SUCCESS;
    ListMP_Object   * obj = NULL;
    ListMPDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "ListMP_close", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_close",
                             status,
                             "Module is not initialized!");
    }
    else if (handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_close",
                             status,
                             "handlePtr passed is NULL!");
    }
    else if (*handlePtr == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_close",
                             status,
                             "*handlePtr passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) *handlePtr;
        cmdArgs.args.close.handle = obj->knlObject;

        status = ListMPDrv_ioctl (CMD_LISTMP_CLOSE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_close",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        Memory_free (NULL,
                     (*handlePtr),
                     sizeof (ListMP_Object));

        *handlePtr = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_close", status);

    return status;
}


/*
 *  Function to check if list is empty
 */
Bool
ListMP_empty (ListMP_Handle  listMPHandle)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                status   = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Bool               isEmpty  = FALSE;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;

    GT_1trace (curTrace, GT_ENTER, "ListMP_empty", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_create",
                             ListMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_empty",
                             ListMP_E_INVALIDARG,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.isEmpty.handle = obj->knlObject;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_ISEMPTY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_empty",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            isEmpty = cmdArgs.args.isEmpty.isEmpty ;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_empty", isEmpty);

     return (isEmpty);
}


/*
 *  Function to get head element from list
 */
Ptr ListMP_getHead (ListMP_Handle listMPHandle)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                status = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ListMP_Elem      * elem   = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;

    GT_1trace (curTrace, GT_ENTER, "ListMP_getHead", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_getHead",
                             ListMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_getHead",
                             ListMP_E_INVALIDARG,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *)listMPHandle;
        cmdArgs.args.getHead.handle = obj->knlObject;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_GETHEAD, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_getHead",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (cmdArgs.args.getHead.elemSrPtr != SharedRegion_INVALIDSRPTR) {
                elem = (ListMP_Elem *) SharedRegion_getPtr(
                                                cmdArgs.args.getHead.elemSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_getHead", elem);

    return elem;
}


/*
 *  Function to get tail element from list
 */
Ptr ListMP_getTail (ListMP_Handle listMPHandle)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                status = ListMP_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    ListMP_Elem      * elem   = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;

    GT_1trace (curTrace, GT_ENTER, "ListMP_getTail", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_getTail",
                             ListMP_E_INVALIDSTATE,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_getTail",
                             ListMP_E_INVALIDARG,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *)listMPHandle;
        cmdArgs.args.getTail.handle = obj->knlObject;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        ListMPDrv_ioctl (CMD_LISTMP_GETTAIL, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_getTail",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (cmdArgs.args.getTail.elemSrPtr != SharedRegion_INVALIDSRPTR) {
                elem = (ListMP_Elem *) SharedRegion_getPtr(
                                                cmdArgs.args.getTail.elemSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_getTail", elem);

    return elem;
}


/*!
 *  Function to put head element into list
 */
Int ListMP_putHead (ListMP_Handle  listMPHandle,
                    ListMP_Elem  * elem)
{
    Int                status = ListMP_S_SUCCESS;
    ListMP_Object *    obj = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    UInt16             index;


    GT_1trace (curTrace, GT_ENTER, "ListMP_putHead", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    GT_assert (curTrace, (elem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putHead",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putHead",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else if (elem == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putHead",
                             status,
                             "elem passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.putHead.handle = obj->knlObject;

        index = SharedRegion_getId (elem);
        cmdArgs.args.putHead.elemSrPtr =  SharedRegion_getSRPtr (elem,index);
        status = ListMPDrv_ioctl (CMD_LISTMP_PUTHEAD, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_putHead",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_putHead", status);

    return status;
}


/*
 *  Function to put tail element into list
 */
Int ListMP_putTail (ListMP_Handle    listMPHandle,
                    ListMP_Elem    * elem)
{
    Int                status = ListMP_S_SUCCESS;
    ListMP_Object *    obj = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    UInt16             index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_putTail", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    GT_assert (curTrace, (elem         != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putTail",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putTail",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else if (elem == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_putTail",
                             status,
                             "elem passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.putTail.handle = obj->knlObject;
        index = SharedRegion_getId (elem);
        cmdArgs.args.putTail.elemSrPtr =  SharedRegion_getSRPtr (elem,index);

        status = ListMPDrv_ioctl (CMD_LISTMP_PUTTAIL, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_putTail",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_putTail", status);

    return status;
}


/*
 * Function to insert element into list
 */
Int ListMP_insert (ListMP_Handle  listMPHandle,
                   ListMP_Elem  * newElem,
                   ListMP_Elem  * curElem)
{
    Int                status = ListMP_S_SUCCESS;
    ListMP_Object *    obj = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    UInt16             index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_insert", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    GT_assert (curTrace, (newElem != NULL));
    GT_assert (curTrace, (curElem != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_insert",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_insert",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else if (newElem == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_insert",
                             status,
                             "newElem passed is NULL!");
    }
    else if (curElem == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_insert",
                             status,
                             "curElem passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.insert.handle = obj->knlObject;

        index = SharedRegion_getId (newElem);
        cmdArgs.args.insert.newElemSrPtr =SharedRegion_getSRPtr (newElem,index);
        index = SharedRegion_getId (curElem);
        cmdArgs.args.insert.curElemSrPtr =SharedRegion_getSRPtr (curElem,index);
        status = ListMPDrv_ioctl (CMD_LISTMP_INSERT,
                                              &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_insert",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    return status;
}


/*
 * Function to remove element from list
 */
Int ListMP_remove (ListMP_Handle   listMPHandle,
                   ListMP_Elem   * elem)
{
    Int                status = ListMP_S_SUCCESS;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;
    UInt16             index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_remove", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    GT_assert (curTrace, (elem         != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_remove",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_remove",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else if (elem == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_remove",
                             status,
                             "elem pointer passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.remove.handle = obj->knlObject;

        index = SharedRegion_getId (elem);
        cmdArgs.args.remove.elemSrPtr = SharedRegion_getSRPtr (elem,index);

        status = ListMPDrv_ioctl (CMD_LISTMP_REMOVE, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_remove",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_remove", status);

    return status;
}


/*
 *  Function to traverse to next element in list
 */
Ptr ListMP_next (ListMP_Handle   listMPHandle,
                 ListMP_Elem   * elem)
{
    Int                status = ListMP_S_SUCCESS;
    Ptr                next   = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;
    UInt16             index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_next", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    /* elem can be NULL to return the first element in the list. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_next",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_next",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;
        cmdArgs.args.next.handle = obj->knlObject;

        if (elem != NULL){
            index = SharedRegion_getId (elem);
            cmdArgs.args.next.elemSrPtr = SharedRegion_getSRPtr (elem,index);
        }
        else{
            cmdArgs.args.next.elemSrPtr = SharedRegion_INVALIDSRPTR;
        }

        status = ListMPDrv_ioctl (CMD_LISTMP_NEXT,
                                  &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_next",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (cmdArgs.args.next.nextElemSrPtr != SharedRegion_INVALIDSRPTR) {
                next = (ListMP_Elem *)SharedRegion_getPtr(
                                               cmdArgs.args.next.nextElemSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_next", next);

    return next;
}


/*
 *  Function to traverse to prev element in list
 */
Ptr ListMP_prev (ListMP_Handle    listMPHandle,
                 ListMP_Elem    * elem)
{
    Int                status = ListMP_S_SUCCESS;
    Ptr                prev   = NULL;
    ListMPDrv_CmdArgs  cmdArgs;
    ListMP_Object *    obj = NULL;
    UInt16             index;

    GT_1trace (curTrace, GT_ENTER, "ListMP_prev", listMPHandle);

    GT_assert (curTrace, (listMPHandle != NULL));
    /* elem can be NULL to return the first element in the list. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (ListMP_module->setupRefCount == 0) {
        status = ListMP_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_prev",
                             status,
                             "Module is not initialized!");
    }
    else if (listMPHandle == NULL) {
        status = ListMP_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_prev",
                             status,
                             "Invalid NULL listMPHandle pointer specified!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        obj = (ListMP_Object *) listMPHandle;

        cmdArgs.args.prev.handle = obj->knlObject;
        if(elem != NULL){
            index = SharedRegion_getId (elem);
            cmdArgs.args.prev.elemSrPtr = SharedRegion_getSRPtr (elem,index);
        }
        else{
            cmdArgs.args.prev.elemSrPtr = SharedRegion_INVALIDSRPTR;
        }

        status = ListMPDrv_ioctl (CMD_LISTMP_PREV, &cmdArgs);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "ListMP_prev",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            if (cmdArgs.args.prev.prevElemSrPtr != SharedRegion_INVALIDSRPTR) {
                prev = (ListMP_Elem *)SharedRegion_getPtr(
                                               cmdArgs.args.prev.prevElemSrPtr);
            }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "ListMP_prev", prev);

    return prev;
}


/*=============================================================================
    Internal functions
  =============================================================================
*/
/*
 * Create and populates handle and obj.
 */
Int32
_ListMP_create(ListMP_Handle     * handlePtr,
               ListMPDrv_CmdArgs   cmdArgs,
               UInt16              createFlag)
{
    Int32 status = ListMP_S_SUCCESS;

    GT_assert (curTrace, (handlePtr != NULL));

    /* Allocate memory for the handle */
    *handlePtr = (ListMP_Handle) Memory_calloc (NULL,
                                                sizeof (ListMP_Object),
                                                0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (*handlePtr == NULL) {
        status = ListMP_E_MEMORY;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "ListMP_create",
                             status,
                             "Memory allocation failed for handle!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Set pointer to kernel object into the user handle. */
        if (createFlag == TRUE) {
            ((ListMP_Object *)(*handlePtr))->knlObject =
                                                    cmdArgs.args.create.handle;
        }
        else{
            ((ListMP_Object *)(*handlePtr))->knlObject =
                                                     cmdArgs.args.open.handle;
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    return(status);
}

/*
 *  Retrieves the GateMP handle associated with the ListMP instance.
 */
GateMP_Handle ListMP_getGate (ListMP_Handle handle)
{
    return NULL; /* TBD */
}



#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
