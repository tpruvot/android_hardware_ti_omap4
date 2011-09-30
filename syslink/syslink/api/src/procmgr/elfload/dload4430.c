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
/* dload4430.c                                                               */
/*                                                                           */
/* Core Dynamic Loader wrapper interface.                                    */
/*                                                                           */
/* This implementation of the core dynamic loader is platform independent,   */
/* but it is object file format dependent.  In particular, this              */
/* implementation supports ELF object file format.                           */
/*****************************************************************************/

#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Memory.h>
#include <Trace.h>

/* Module level headers */
#include <ti/ipc/MultiProc.h>
#include <_MultiProc.h>
#include <ProcMgr.h>

#include "Queue.h"
#include "Stack.h"

#include "dload4430.h"
#include "dload_api.h"
#include "load.h"


/*!
 *  @brief  DLoad Module state object
 */
typedef struct DLoad4430_ModuleObject_tag {
    UInt32         setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
    DLoad_Config defCfg;
    /* Default module configuration */
    DLoad4430_Handle dLoadHandles [MultiProc_MAXPROCESSORS];
    /*!< Array of Handles of DLoad instances */
} DLoad4430_ModuleObject;



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
DLoad4430_ModuleObject DLoad_state =
{
    .setupRefCount = 0
};



/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Function to get the default configuration for the DLoad
 *              module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to DLoadr_setup filled in by the
 *              DLoad module with the default parameters. If the user does
 *              not wish to make any change in the default parameters, this API
 *              is not required to be called.
 *
 *  @param      cfg        Pointer to the DLoad module configuration structure
 *                         in which the default config is to be returned.
 *
 *  @sa         DLoad4430_setup
 */
Void
DLoad4430_getConfig (DLoad_Config * cfg)
{
    GT_1trace (curTrace, GT_ENTER, "DLoad4430_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_getConfig",
                             DLOAD_E_INVALIDARG,
                             "Argument of type (DLoad_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        memcpy(cfg, &DLoad_state.defCfg, sizeof(DLoad_Config));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "DLoad4430_getConfig");
}


/*!
 *  @brief      Function to setup the DLoad4430 module.
 *
 *              This function sets up the DLoad module. This function must
 *              be called before any other instance-level APIs can be invoked.
 *              Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then DLoad_getConfig can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. If the user
 *              does not wish to make any change in the default parameters, the
 *              application can simply call DLoad_setup with NULL parameters.
 *              The default parameters would get automatically used.
 *
 *  @param      cfg   Optional DLoad4430 module configuration. If provided as
 *                    NULL, default configuration is used.
 *
 *  @sa         DLoad4430_destroy
 */
Int
DLoad4430_setup (DLoad_Config * cfg)
{
    Int                     status = DLOAD_SUCCESS;
    Int                     i;

    GT_1trace (curTrace, GT_ENTER, "DLoad4430_setup", cfg);

    /* TBD: Protect from multiple threads. */
    DLoad_state.setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (DLoad_state.setupRefCount > 1) {
        /*! @retval DLOAD_S_ALREADYSETUP Success: DLoad module has been
                                         already setup in this process */
        status = DLOAD_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "    DLoad4430_setup: DLoad module has been already setup "
                   "in this process.\n"
                   "        RefCount: [%d]\n",
                   (DLoad_state.setupRefCount - 1));
    }
    else {
        /* Set all handles to NULL -- in order for destroy() to work */
        for (i = 0 ; i < MultiProc_MAXPROCESSORS ; i++) {
            DLoad_state.dLoadHandles [i] = NULL;
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_setup", status);

    /*! @retval DLOAD_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to destroy the DLoad4430 module.
 *
 *              Once this function is called, other DLoad4430 module APIs,
 *              except for the DLoad4430_getConfig API cannot be called anymore.
 *
 *  @sa         DLoad4430_setup
 */
Int
DLoad4430_destroy (Void)
{
    Int                     status = DLOAD_SUCCESS;
    UInt16                  i;

    GT_0trace (curTrace, GT_ENTER, "DLoad4430_destroy");

    /* TBD: Protect from multiple threads. */
    DLoad_state.setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (DLoad_state.setupRefCount >= 1) {
        /*! @retval PROCMGR_S_SETUP Success: ProcMgr module has been setup
                                             by other clients in this process */
        status = DLOAD_S_SETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "DLoad module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (DLoad_state.setupRefCount + 1));
    }
    else {
        /* Check if any ProcMgr instances have not been deleted so far. If not,
         * delete them.
         */
        for (i = 0 ; i < MultiProc_MAXPROCESSORS ; i++) {
            GT_assert (curTrace, (DLoad_state.procHandles [i] == NULL));
            if (DLoad_state.dLoadHandles [i] != NULL) {
                DLoad4430_delete (&(DLoad_state.dLoadHandles [i]));
            }
        }
    }

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_destroy", status);

    /*! @retval DLOAD_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Function to create a DLoad4430 object for a specific slave
 *              processor.
 *
 *              This function creates an instance of the DLoad module and
 *              returns an instance handle, which is used to access the
 *              specified slave processor. The processor ID specified here is
 *              the ID of the slave processor as configured with the MultiProc
 *              module.
 *              Instance-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then DLoad_Params_init can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. For this
 *              API, the params argument is not optional, since the user needs
 *              to provide some essential values such as loader, PwrMgr and
 *              Processor instances to be used with this ProcMgr instance.
 *
 *  @param      procId   Processor ID represented by this DLoad4430 instance
 *  @param      params   DLoad4430 instance configuration parameters.
 *
 *  @sa         DLoad4430_delete, Memory_calloc
 */
DLoad4430_Handle
DLoad4430_create (UInt16 procId, const DLoad_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                     status = DLOAD_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    DLoad4430_Object *        handle = NULL;
    /* TBD: UInt32                  key;*/

    GT_2trace (curTrace, GT_ENTER, "DLoad4430_create", procId, params);

    GT_assert (curTrace, MultiProc_isValidRemoteProc (procId));
    GT_assert (curTrace, (params != NULL));
    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (!MultiProc_isValidRemoteProc (procId)) {
        /*! @retval NULL Invalid procId specified */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_create",
                             status,
                             "Invalid procId specified");
    }
    else if (params == NULL) {
        /*! @retval NULL Invalid NULL params pointer specified */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_create",
                             status,
                             "Invalid NULL params pointer specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */
        if (DLoad_state.dLoadHandles [procId] != NULL) {
            /* If the object is already created/opened in this process, return
             * handle in the local array.
             */
            handle = (DLoad4430_Object *) DLoad_state.dLoadHandles [procId];
            handle->openRefCount++;
            GT_1trace (curTrace,
                       GT_2CLASS,
                       "    DLoad4430_create: Instance already exists in this"
                       " process space"
                       "        RefCount [%d]\n",
                       (handle->openRefCount - 1));
        }
        else {
            /* Allocate memory for the handle */
            handle = (DLoad4430_Object *) Memory_calloc (NULL,
                                                    sizeof (DLoad4430_Object),
                                                    0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (handle == NULL) {
                /*! @retval NULL Memory allocation failed for handle */
                status = DLOAD_E_MEMORY;
                GT_setFailureReason (curTrace,
                                    GT_4CLASS,
                                    "DLoad4430_create",
                                    status,
                                    "Memory allocation failed for handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                handle->loaderHandle = DLOAD_create((void*)handle);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (handle->loaderHandle == NULL) {
                    Memory_free(NULL, handle, sizeof (DLoad4430_Object));
                    /*! @retval NULL Memory allocation failed for handle */
                    status = DLOAD_E_MEMORY;
                    GT_setFailureReason (curTrace,
                              GT_4CLASS,
                              "DLoad4430_create",
                              status,
                              "Memory allocation failed for loaderhandle!");
                }
                else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                    /* Indicate that the object was created in this process. */
                    handle->created = TRUE;
                    handle->procId = procId;
                    handle->fileId = 0xFFFFFFFF;
                    mirror_debug_ptr_initialize_queue(&handle->mirror_debug_list);
                    dl_debug_initialize_stack(&handle->dl_debug_stk);

                    /* Store the ProcMgr handle in the local array. */
                    DLoad_state.dLoadHandles[procId] = (DLoad4430_Handle)handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        }
        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_create", handle);

    /*! @retval Valid Handle Operation successful */
    return (DLoad4430_Handle) handle;
}


/*!
 *  @brief      Function to delete a DLoad4430 object for a specific slave
 *              processor.
 *
 *              Once this function is called, other DLoad4430 instance level
 *              APIs that require the instance handle cannot be called.
 *
 *  @param      handlePtr   Pointer to Handle to the DLoad4430 object
 *
 *  @sa         DLoad4430_create, Memory_free
 */
Int
DLoad4430_delete (DLoad4430_Handle * handlePtr)
{
    Int                     status    = DLOAD_SUCCESS;
    DLoad4430_Object *        handle;
    /* TBD: UInt32          key;*/

    GT_1trace (curTrace, GT_ENTER, "DLoad4430_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handlePtr == NULL) {
        /*! @retval PROCMGR_E_INVALIDARG Invalid NULL handlePtr specified*/
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_delete",
                             status,
                             "Invalid NULL handlePtr specified");
    }
    else if (*handlePtr == NULL) {
        /*! @retval PROCMGR_E_HANDLE Invalid NULL *handlePtr specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_delete",
                             status,
                             "Invalid NULL *handlePtr specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        handle = (DLoad4430_Object *) (*handlePtr);
        /* TBD: Enter critical section protection. */
        /* key = Gate_enter (ProcMgr_state.gateHandle); */

        if (handle->openRefCount != 0) {
            /* There are still some open handles to this ProcMgr.
             * Give a warning, but still go ahead and delete the object.
             */
            status = DLOAD_S_OPENHANDLE;
            GT_assert (curTrace, (handle->openRefCount != 0));
            GT_1trace (curTrace,
                       GT_1CLASS,
                       "    DLoad4430_delete: Warning, some handles are"
                       " still open!\n"
                       "        RefCount: [%d]\n",
                       handle->openRefCount);
        }

        if (handle->created == FALSE) {
            /*! @retval PROCMGR_E_ACCESSDENIED The ProcMgr object was not
                   created in this process and access is denied to delete it. */
            status = DLOAD_E_ACCESSDENIED;
            GT_1trace (curTrace,
                       GT_1CLASS,
                       " DLoad4430_delete: Warning, the DLoad object was not"
                       " created in this process and access is denied to"
                       " delete it.\n",
                       status);
        }

        if (status >= 0) {
            /* Clear the ProcMgr handle in the local array. */
            GT_assert (curTrace,(handle->procId < MultiProc_MAXPROCESSORS));
            DLOAD_destroy(handle->loaderHandle);
            DLoad_state.dLoadHandles [handle->procId] = NULL;
            handle->loaderHandle = NULL;
            Memory_free (NULL, handle, sizeof (DLoad4430_Object));
            *handlePtr = NULL;
        }

        /* TBD: Leave critical section protection. */
        /* Gate_leave (ProcMgr_state.gateHandle, key); */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_delete", status);

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
 *  @param      handle      Handle to the DLoad4430 object
 *  @param      imagePath   Full file path
 *  @param      argc        Number of arguments
 *  @param      argv        String array of arguments
 *  @param      entry_point Return parameter: entry point of the loaded file
 *  @param      fileId      Return parameter: ID of the loaded file
 *
 *  @sa         DLoad4430_unload, DLOAD_load
 */
Int
DLoad4430_load (DLoad4430_Handle handle,
                String           imagePath,
                UInt32           argc,
                String *         argv,
                UInt32 *         entry_point,
                UInt32 *         fileId)
{
    Int                 status          = DLOAD_SUCCESS;
    Int                 prog_argc       = 0;
    Array_List          prog_argv;
    UInt32              proc_entry_point;
    UInt32              prog_handle;
    FILE*               fp;
    DLoad4430_Object *  handlePtr       = (DLoad4430_Object *) (handle);
    Bool                found = FALSE;

    GT_4trace (curTrace, GT_ENTER, "DLoad4430_load",
               handle, imagePath, argc, argv);
    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (fileId != NULL));
    /* imagePath may be NULL if a non-file based loader is used. In that case,
     * loader-specific params will contain the required information.
     */

    GT_assert (curTrace,
               (   ((argc == 0) && (argv == NULL))
                || ((argc != 0) && (argv != NULL))));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  DLOAD_E_HANDLE Invalid NULL handle specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_load",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (   ((argc == 0) && (argv != NULL))
             || ((argc != 0) && (argv == NULL))) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid argument */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_load",
                             status,
                             "Invalid argc/argv values specified");
    }
    else if (fileId == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid argument */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_load",
                             status,
                             "Invalid fileId pointer specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        *fileId = 0; /* Initialize return parameter. */
        *entry_point = 0;

        /*--------------------------------------------------------------------*/
        /* Open specified file from "load" command, load it, then close the   */
        /* file.                                                              */
        /*--------------------------------------------------------------------*/

        /*--------------------------------------------------------------------*/
        /* NOTE!! We require that any shared objects that we are trying to    */
        /* load be in the current directory.                                  */
        /*--------------------------------------------------------------------*/
        /*--------------------------------------------------------------------*/
        /* If there is registry available, we'll need to look up the given    */
        /* file_name in the registry to find the actual pathname to be used   */
        /* while opening the file.  Otherwise, we'll just use the given file  */
        /* name.                                                              */
        /*--------------------------------------------------------------------*/
        fp = fopen(imagePath, "rb");

        /*--------------------------------------------------------------------*/
        /* Were we able to open the file successfully?                        */
        /*--------------------------------------------------------------------*/
        if (!fp)
        {
            status = DLOAD_E_FAIL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "DLoad4430_load",
                                 status,
                                 "Failed to open file!");
        }
        else {
            /*----------------------------------------------------------------*/
            /* If we have a DLModules symbol available in the base image,     */
            /* then we need to create debug information for this file in      */
            /* target memory to support whatever DLL View plug-in or script   */
            /* is implemented in the debugger.                                */
            /*----------------------------------------------------------------*/
            if (handlePtr->DLL_debug)
                DLDBG_add_host_record(handlePtr, imagePath);

            //DLIF_mapTable(handlePtr);

            /*----------------------------------------------------------------*/
            /* Now, we are ready to start loading the specified file onto the */
            /* target.                                                        */
            /*----------------------------------------------------------------*/
            prog_handle = DLOAD_load(handlePtr->loaderHandle,
                                     fp, prog_argc, (char**)(prog_argv.buf));

            //DLIF_unMapTable(handlePtr);

            fclose(fp);

            /*----------------------------------------------------------------*/
            /* If the load was successful, then we'll need to write the debug */
            /* information for this file into target memory.                  */
            /*----------------------------------------------------------------*/
            if (prog_handle) {

                if (handlePtr->DLL_debug) {
                    /*--------------------------------------------------------*/
                    /* Allocate target memory for the module's debug record.  */
                    /* Use host version of the debug information to determine */
                    /* how much target memory we need and how it is to be     */
                    /* filled in.                                             */
                    /*--------------------------------------------------------*/
                    /* Note that we don't go through the normal API functions */
                    /* to get target memory and write the debug information   */
                    /* since we're not dealing with object file content here. */
                    /* The DLL View debug is supported entirely on the client */
                    /* side.                                                  */
                    /*--------------------------------------------------------*/
                    DLDBG_add_target_record(handlePtr, prog_handle);
                }

                /*------------------------------------------------------------*/
                /* Find entry point associated with loaded program's handle,  */
                /* set up entry point in "proc_entry_point".                  */
                /*------------------------------------------------------------*/
                DLOAD_get_entry_point(handlePtr->loaderHandle, prog_handle,
                                      (TARGET_ADDRESS)(&proc_entry_point));

                *entry_point = (UInt32)proc_entry_point;
                *fileId = prog_handle;

                if (handlePtr->fileId == 0xFFFFFFFF) {
                    handlePtr->fileId = prog_handle;

                    /*--------------------------------------------------------*/
                    /* Query base image for DLModules symbol to determine     */
                    /* whether debug support needs to be provided by the      */
                    /* client side of the loader.                             */
                    /*--------------------------------------------------------*/
                    /* Note that space for the DLL View debug list is already */
                    /* allocated as part of the base image, the header record */
                    /* is initialized to zero and needs to be filled in when  */
                    /* the first module record is written to target memory.   */
                    /*------------------------------------------------------ -*/
                    handlePtr->DLL_debug = DLOAD_query_symbol(
                                               handlePtr->loaderHandle,
                                               prog_handle,
                                               "_DLModules",
                                               &handlePtr->DLModules_loc);

                    found = DLOAD_query_symbol(handlePtr->loaderHandle,
                                               prog_handle, "_DYNEXT_BEG",
                                               &handlePtr->dynLoadMem);
                    if (!found) {
                        /*
                         * If we didn't find _DynLoaderMem, then we won't be
                         * able to perform dynamic loading.  Give a warning
                         */
                        handlePtr->dynLoadMem = 0;
                        GT_1trace (curTrace,
                                   GT_1CLASS,
                                   "DLoad4430_load: "
                                   "Did not find symbol _DYNEXT_BEG",
                                   found);
                    }
                    else {
                        void *memSize;
                        found = DLOAD_query_symbol(handlePtr->loaderHandle,
                                                   prog_handle, "_DYNEXT_SIZE",
                                                   &memSize);
                        if (!found) {
                            handlePtr->dynLoadMemSize = 0;
                            GT_1trace (curTrace,
                                       GT_1CLASS,
                                       "DLoad4430_load: "
                                       "Did not find symbol _DYNEXT_SIZE",
                                       found);
                        }
                        else {
                            handlePtr->dynLoadMemSize = (UInt32)memSize;
                            DLIF_initMem(handlePtr,
                                         (UInt32)handlePtr->dynLoadMem,
                                         handlePtr->dynLoadMemSize);
                        }
                    }
                }
            }
            /*----------------------------------------------------------------*/
            /* Report failure to load an object file.                         */
            /*----------------------------------------------------------------*/
            else {
                status = DLOAD_E_FAIL;
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "DLoad4430_load",
                                     status,
                                     "Failed load of file!");
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_load", status);

    /*! @retval DLOAD_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to unload the previously loaded file on the slave
 *              processor.
 *
 *              This function unloads the file that was previously loaded on the
 *              slave processor through the DLoad4430_load API. It frees up any
 *              resources that were allocated during DLoad4430_load for this
 *              file.  The fileId received from the load function must be passed
 *              to this function.
 *
 *  @param      handle     Handle to the DLoad4430 object
 *  @param      fileId     ID of the loaded file to be unloaded
 *
 *  @sa         DLoad4430_load, DLOAD_unload
 */
Int
DLoad4430_unload (DLoad4430_Handle handle, UInt32 fileId)
{
    Int                    status           = DLOAD_SUCCESS;
    DLoad4430_Object *       dloadHandle    = (DLoad4430_Object *) handle;
    Bool unloaded = FALSE;

    GT_2trace (curTrace, GT_ENTER, "DLoad4430_unload", handle, fileId);

    GT_assert (curTrace, (handle != NULL));
    /* Cannot check for fileId because it is loader dependent. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  DLOAD_E_HANDLE Invalid NULL handle specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_unload",
                             status,
                             "Invalid NULL handle specified");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        //DLIF_mapTable(dloadHandle);

        unloaded = DLOAD_unload(dloadHandle->loaderHandle, fileId);
        if (!unloaded) {
            status = DLOAD_E_FAIL;
        }

        //DLIF_unMapTable(dloadHandle);

        if (dloadHandle->DLL_debug)
            DLDBG_rm_target_record(dloadHandle, fileId);

        if (status >= 0 && dloadHandle->fileId == fileId) {
            /* Baseimage has been unloaded, reset the fileId to reflect this. */
            dloadHandle->fileId = 0xFFFFFFFF;
            DLIF_deinitMem(dloadHandle);
        }

#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "DLoad4430_unload",
                                 status,
                                 "Failed to unload file!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_unload", status);

    /*! @retval DLOAD_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the information about the entry point
 *              names to be used to allocate an array for calling
 *              DLoad4430_getEntryNames.
 *
 *  @param      handle                Handle to the DLoad4430 object
 *  @param      fileId                ID of the file received from the load
 *                                    function
 *  @param      entry_pt_cnt          Return parameter: Number of entry points
 *                                    in the specified file.
 *  @param      entry_pt_max_name_len Return parameter: Length in bytes of the
 *                                    longest entry point name.
 *
 *  @sa         DLOAD_get_entry_names_info, DLoad4430_getEntryNames
 */
Int
DLoad4430_getEntryNamesInfo (DLoad4430_Handle handle,
                             UInt32           fileId,
                             Int32 *          entry_pt_cnt,
                             Int32 *          entry_pt_max_name_len)
{
    Int                 status      = DLOAD_SUCCESS;
    DLoad4430_Object *  dloadHandle = (DLoad4430_Object *) handle;
    Bool                found       = FALSE;

    GT_4trace (curTrace, GT_ENTER, "DLoad4430_getEntryNamesInfo",
               handle, fileId, entry_pt_cnt, entry_pt_max_name_len);

    GT_assert (curTrace, (handle      != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (entry_pt_cnt  != NULL));
    GT_assert (curTrace, (entry_pt_max_name_len    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  DLOAD_E_HANDLE Invalid NULL handle specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_getEntryNamesInfo",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (entry_pt_cnt == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                       GT_4CLASS,
                       "DLoad4430_getEntryNamesInfo",
                       status,
                       "Invalid value NULL provided for argument entry_pt_cnt");
    }
    else if (entry_pt_max_name_len == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
              GT_4CLASS,
              "DLoad4430_getEntryNamesInfo",
              status,
              "Invalid value NULL provided for argument entry_pt_max_name_len");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        found = DLOAD_get_entry_names_info(dloadHandle->loaderHandle,
                                   fileId, entry_pt_cnt, entry_pt_max_name_len);
        /* Return symbol address. */
        if (!found) {
            *entry_pt_cnt = 0u;
            *entry_pt_max_name_len = 0u;
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_getEntryNamesInfo", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the entry point names for the specified
 *              file.
 *
 *  @param      handle         Handle to the DLoad4430 object
 *  @param      fileId         ID of the file received from the load function
 *  @param      entry_pt_cnt   Size of the array passed in entry_pt_names
 *  @param      entry_pt_names Return parameter: Pointer to an array to be
 *                             populated with the entry point names.
 *
 *  @sa         DLoad4430_getEntryNamesInfo, DLOAD_get_entry_names
 */
Int
DLoad4430_getEntryNames (DLoad4430_Handle handle,
                         UInt32           fileId,
                         Int32 *          entry_pt_cnt,
                         Char ***         entry_pt_names)
{
    Int                 status      = DLOAD_SUCCESS;
    DLoad4430_Object *  dloadHandle = (DLoad4430_Object *) handle;
    Bool                found       = FALSE;

    GT_4trace (curTrace, GT_ENTER, "DLoad4430_getEntryNames",
               handle, fileId, entry_pt_cnt, entry_pt_names);

    GT_assert (curTrace, (handle      != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (entry_pt_cnt  != NULL));
    GT_assert (curTrace, (entry_pt_names    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  DLOAD_E_HANDLE Invalid NULL handle specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_getEntryNames",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (entry_pt_cnt == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                       GT_4CLASS,
                       "DLoad4430_getEntryNames",
                       status,
                       "Invalid value NULL provided for argument entry_pt_cnt");
    }
    else if (entry_pt_names == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                     GT_4CLASS,
                     "DLoad4430_getEntryNames",
                     status,
                     "Invalid value NULL provided for argument entry_pt_names");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        found = DLOAD_get_entry_names(dloadHandle->loaderHandle,
                                      fileId, entry_pt_cnt, entry_pt_names);
        /* Return symbol address. */
        if (!found)
            *entry_pt_cnt = 0u;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_getEntryNames", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}


/*!
 *  @brief      Function to retrieve the target address of a symbol from the
 *              specified file.
 *
 *  @param      handle      Handle to the DLoad4430 object
 *  @param      fileId      ID of the file received from the load function
 *  @param      symbolName  Name of the symbol
 *  @param      symValue    Return parameter: Symbol address
 *
 *  @sa         DLOAD_query_symbol
 */
Int
DLoad4430_getSymbolAddress (DLoad4430_Handle handle,
                            UInt32           fileId,
                            String           symbolName,
                            UInt32 *         symValue)
{
    Int                 status      = DLOAD_SUCCESS;
    DLoad4430_Object *  dloadHandle = (DLoad4430_Object *) handle;
    Bool                found       = FALSE;

    GT_4trace (curTrace, GT_ENTER, "DLoad4430_getSymbolAddress",
               handle, fileId, symbolName, symValue);

    GT_assert (curTrace, (handle      != NULL));
    /* fileId may be 0, so no check for fileId. */
    GT_assert (curTrace, (symbolName  != NULL));
    GT_assert (curTrace, (symValue    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (handle == NULL) {
        /*! @retval  DLOAD_E_HANDLE Invalid NULL handle specified */
        status = DLOAD_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "DLoad4430_getSymbolAddress",
                             status,
                             "Invalid NULL handle specified");
    }
    else if (symbolName == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symbolName */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "DLoad4430_getSymbolAddress",
                         status,
                         "Invalid value NULL provided for argument symbolName");
    }
    else if (symValue == NULL) {
        /*! @retval  DLOAD_E_INVALIDARG Invalid value NULL provided for
                     argument symValue */
        status = DLOAD_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                         GT_4CLASS,
                         "DLoad4430_getSymbolAddress",
                         status,
                         "Invalid value NULL provided for argument symValue");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        found = DLOAD_query_symbol(dloadHandle->loaderHandle,
                                   fileId, symbolName, (void **)symValue);
        /* Return symbol address. */
        if (!found)
            *symValue = 0u;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "DLoad4430_getSymbolAddress", status);

    /*! @retval PROCMGR_SUCCESS Operation successful */
    return status;
}
