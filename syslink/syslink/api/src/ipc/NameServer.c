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
 *  @file   NameServer.c
 *
 *  @brief      NameServer Manager
 *
 *              The NameServer module manages local name/value pairs that
 *              enables an application and other modules to store and retrieve
 *              values based on a name. The module supports different lengths of
 *              values. The add/get functions are for variable length values.
 *              The addUInt32 function is optimized for UInt32 variables and
 *              constants. Each NameServer instance manages a different
 *              name/value table. This allows each table to be customized to
 *              meet the requirements of user:
 *              @li Size differences: one table could allow long values
 *              (e.g. > 32 bits) while another table could be used to store
 *              integers. This customization enables better memory usage.
 *              @li Performance: improves search time when retrieving a
 *              name/value pair.
 *              @li Relax name uniqueness: names in a specific table must be
 *              unique, but the same name can be used in different tables.
 *              @li Critical region management: potentially different tables are
 *              used by different types of threads. The user can specify the
 *              type of critical region manager (i.e. xdc.runtime.IGateProvider)
 *              to be used for each instance.
 *              When adding a name/value pair, the name and value are copied
 *              into internal buffers in NameServer. To minimize runtime memory
 *              allocation these buffers can be allocated at creation time.
 *              The NameServer module can be used in a multiprocessor system.
 *              The module communicates to other processors via the RemoteProxy.
 *              The way the communication to the other processors is dependent
 *              on the RemoteProxy implementation.
 *              The NameServer module uses the MultiProc module for identifying
 *              the different processors. Which remote processors and the order
 *              they are quered is determined by the procId array in the get
 *              function.
 *              Currently there is no endian or wordsize conversion performed by
 *              the NameServer module.<br>
 *              Transport management:<br>
 *              #NameServer_setup API creates two NameServers internally. These
 *              NameServer are used for holding handles and names of other
 *              nameservers created by application or modules. This helps, when
 *              a remote processors wants to get data from a nameserver on this
 *              processor. In all modules, which can have instances, all created
 *              instances can be kept in a module specific nameserver. This
 *              reduces search operation if a single nameserver is used for all
 *              modules. Thus a module level nameserver helps.<br>
 *              When a module requires some information from a remote nameserver
 *              it passes a name in the following format:<br>
 *              "<module_name>:<instance_name>or<instance_info_name>"<br>
 *              For example: "GatePeterson:notifygate"<br>
 *              When transport gets this name it searches for <module_name> in
 *              the module nameserver (created by NameServer_setup). So, it gets
 *              the module specific NameServer handle, then it searchs for the
 *              <instance_name> or <instance_info_name> in the NameServer.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* Utilities & OSAL headers */
#include <Gate.h>
#include <Memory.h>
#include <Trace.h>
#include <String.h>

/* Module level headers */
#include <ti/ipc/NameServer.h>
#include <ti/ipc/MultiProc.h>
#include <_NameServer.h>
#include <NameServerDrv.h>
#include <NameServerDrvDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */

/* Structure defining object for the Gate Peterson */
typedef struct NameServer_Object {
    Ptr         knlObject;
    /*!< Pointer to the kernel-side ProcMgr object. */
} NameServer_Object;

/*!
 *  @brief  ProcMgr Module state object
 */
typedef struct NameServer_ModuleObject_tag {
    NameServer_Config   cfg;
    /*!< NameServer configuration structure */
    NameServer_Config   defCfg;
    /*!< Default module configuration */
    UInt32              refCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
} NameServer_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    NameServer_state
 *
 *  @brief  ProcMgr state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
NameServer_ModuleObject NameServer_state =
{
    .refCount = 0
};

/*!
 *  @var    NameServer_module
 *
 *  @brief  Pointer to the NameServer module state.
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
NameServer_ModuleObject * NameServer_module = &NameServer_state;


/* =============================================================================
 * APIS
 * =============================================================================
 */
/* Function to get the default configuration for the NameServer module. */
Void
NameServer_getConfig (NameServer_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    NameServerDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "NameServer_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getConfig",
                             NameServer_E_FAIL,
                             "Argument of type (NameServer_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (NameServer_module->refCount == 0) {
            /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            NameServerDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NameServer_getConfig",
                                     status,
                                     "Failed to open driver handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                cmdArgs.args.getConfig.config = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                NameServerDrv_ioctl (CMD_NAMESERVER_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
                if (status < 0) {
                    GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "NameServer_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");

                }
            }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Close the driver handle. */
            NameServerDrv_close ();
        }
        else {
            Memory_copy (cfg,
                         &NameServer_module->cfg,
                         sizeof (NameServer_Config));
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "NameServer_getConfig");
}


/* Function to setup the nameserver module. */
Int
NameServer_setup (Void)
{
    Int                   status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "NameServer_setup");

    /* TBD: Protect from multiple threads. */
    NameServer_module->refCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (NameServer_module->refCount > 1) {
        status = NameServer_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "NameServer module has been already setup in this process.\n"
                   "    RefCount: [%d]\n",
                   NameServer_module->refCount);
    }
    else {
        /* Open the driver handle. */
        status = NameServerDrv_open ();
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            status = NameServerDrv_ioctl (CMD_NAMESERVER_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NameServer_setup",
                                     status,
                                     "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "NameServer_setup", status);

    return (status);
}


/* Function to destroy the nameserver module. */
Int
NameServer_destroy (Void)
{
    Int                     status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs   cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "NameServer_destroy");

    /* TBD: Protect from multiple threads. */
    NameServer_module->refCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (NameServer_module->refCount >= 1) {
        status = NameServer_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "NameServer module has been already setup in this process.\n"
                   "    RefCount: [%d]\n",
                   NameServer_module->refCount);
    }
    else {
        status = NameServerDrv_ioctl (CMD_NAMESERVER_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        NameServerDrv_close ();
    }

    GT_1trace (curTrace, GT_LEAVE, "NameServer_destroy", status);

    return status;
}


/* Initialize this config-params structure with supplier-specified
 * defaults before instance creation.
 */
Void
NameServer_Params_init (NameServer_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    NameServerDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "NameServer_Params_init", params);

    GT_assert (curTrace, (params != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_create",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (params == NULL) {
        /* No retVal comment since this is a Void function. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_Params_init",
                             NameServer_E_INVALIDARG,
                             "Argument of type (NameServer_Params *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.ParamsInit.params = params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_PARAMS_INIT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "NameServer_Params_init");
}


/* Function to create a name server. */
NameServer_Handle
NameServer_create (String name, const NameServer_Params * params)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = 0;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    NameServer_Handle     handle = NULL;
    NameServerDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "NameServer_create", name, params);

    GT_assert (curTrace, (params != NULL));
    GT_assert (curTrace, (name != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_create",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (params == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_create",
                             NameServer_E_INVALIDARG,
                             "params passed is NULL!");
    }
    else if (name == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_create",
                             NameServer_E_INVALIDARG,
                             "name passed is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.create.name    = name;
        cmdArgs.args.create.nameLen = String_len (name) + 1;
        cmdArgs.args.create.params  = (NameServer_Params *) params;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_CREATE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_create",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Allocate memory for the handle */
            handle = (NameServer_Handle) Memory_calloc (NULL,
                                                   sizeof (NameServer_Object),
                                                   0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (handle == NULL) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NameServer_create",
                                     NameServer_E_MEMORY,
                                     "Memory allocation failed for handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Set pointer to kernel object into the user handle. */
                handle->knlObject = cmdArgs.args.create.handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_create", handle);

    return handle;
}


/* Function to delete a name server. */
Int
NameServer_delete (NameServer_Handle * handlePtr)
{
    Int                   status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "NameServer_delete", handlePtr);

    GT_assert (curTrace, (handlePtr != NULL));
    GT_assert (curTrace, ((handlePtr != NULL) && (*handlePtr != NULL)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_delete",
                             status,
                             "Module is in invalid state!");
    }
    else if (handlePtr == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_delete",
                             status,
                             "handlePtr passed is NULL!");
    }
    else if (*handlePtr == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_delete",
                             status,
                             "*handlePtr passed is NULL!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.delete.handle = (*handlePtr)->knlObject;
        status = NameServerDrv_ioctl (CMD_NAMESERVER_DELETE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_Params_init",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_delete", status);

    return status;
}


/* Adds a variable length value into the local NameServer table */
Ptr
NameServer_add (NameServer_Handle handle, String name, Ptr buf, UInt len)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Ptr                   new_node = NULL;
    NameServerDrv_CmdArgs cmdArgs;

    GT_4trace (curTrace, GT_ENTER, "NameServer_add",
               handle, name, buf, len);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name     != NULL));
    GT_assert (curTrace, (buf      != NULL));
    GT_assert (curTrace, (len      != 0));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_add",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_add",
                             NameServer_E_INVALIDARG,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_add",
                             NameServer_E_INVALIDARG,
                             "name: name is null!");
    }
    else if (buf == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_add",
                             NameServer_E_INVALIDARG,
                             "buf: buf is null!");
    }
    else if (len == 0u) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_add",
                             NameServer_E_INVALIDARG,
                             "len: Length is zero!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.add.handle  = handle->knlObject;
        cmdArgs.args.add.name    = name;
        cmdArgs.args.add.nameLen = String_len (name) + 1;
        cmdArgs.args.add.buf     = buf;
        cmdArgs.args.add.len     = len;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_ADD, &cmdArgs);
        new_node = cmdArgs.args.add.node;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_add",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_add", new_node);

    return new_node;
}


/* Function to add a UInt32 value into a name server. */
Ptr
NameServer_addUInt32 (NameServer_Handle handle, String name, UInt32 value)
{
    Ptr entry = NULL;

    GT_3trace (curTrace,
               GT_ENTER,
               "NameServer_addUInt32",
               handle,
               name,
               value);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name     != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_addUInt32",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_addUInt32",
                             NameServer_E_INVALIDARG,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_addUInt32",
                             NameServer_E_INVALIDARG,
                             "name: name is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        entry = NameServer_add (handle, name, &value, sizeof (UInt32));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_addUInt32", entry);

    return entry;
}


/* Function to remove a name/value pair from a name server. */
Int
NameServer_remove (NameServer_Handle handle, String name)
{
    Int                   status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "NameServer_remove",
               handle, name);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name     != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_remove",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_remove",
                             status,
                             "Handle is null!");
    }
    else if (name == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_remove",
                             status,
                             "name: name is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.remove.handle  = handle->knlObject;
        cmdArgs.args.remove.name    = name;
        cmdArgs.args.remove.nameLen = String_len (name) + 1;
        status = NameServerDrv_ioctl (CMD_NAMESERVER_REMOVE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_remove",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_remove", status);

    return (status);
}


/* Function to remove a name/value pair from a name server. */
Int
NameServer_removeEntry (NameServer_Handle handle, Ptr entry)
{
    Int                    status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs  cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "NameServer_removeEntry",
               handle, entry);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (entry  != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_removeEntry",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_removeEntry",
                             status,
                             "Handle is null!");
    }
    else if (entry == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_removeEntry",
                             status,
                             "entry: entry is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.removeEntry.handle = handle->knlObject;
        cmdArgs.args.removeEntry.entry  = entry;
        status = NameServerDrv_ioctl (CMD_NAMESERVER_REMOVEENTRY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_removeEntry",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_removeEntry", status);

    return (status);
}


/* Function to retrieve the value portion of a name/value pair from
 * local table.
 */
Int
NameServer_get (NameServer_Handle handle,
                String            name,
                Ptr               value,
                UInt32          * len,
                UInt16            procId[])
{
    Int                   status  = NameServer_S_SUCCESS;
    UInt32                procLen = 0;
    UInt32                i       = 0u;
    NameServerDrv_CmdArgs cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "NameServer_get",
               handle, name, value, len, procId);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name   != NULL));
    GT_assert (curTrace, (value  != NULL));
    GT_assert (curTrace, (len    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "name: name is null!");
    }
    else if (value == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "value: value is null!");
    }
    else if (len == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "len: length is null!");
    }
    else if (*len == 0u) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_get",
                             status,
                             "*len: length is zero!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.get.handle  = handle->knlObject;
        cmdArgs.args.get.name    = name;
        cmdArgs.args.get.nameLen = String_len (name) + 1;
        cmdArgs.args.get.value   = value;
        cmdArgs.args.get.len     = *len;
        cmdArgs.args.get.procId  = procId;
        if (procId != NULL) {
            while (procId[i] != 0xFFFF) { /* TBD */
                procLen++;
                i++;
            }
        }
        cmdArgs.args.get.procLen = procLen;
        status = NameServerDrv_ioctl (CMD_NAMESERVER_GET, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        /* NameServer_E_NOTFOUND is a valid run-time failure. */
        if ((status < 0) && (status != NameServer_E_NOTFOUND)) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_get",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Return updated len */
            *len = cmdArgs.args.get.len;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_get", status);

    return status;
}


/*!
 *  @brief      Function to retrieve the value portion of a name/value pair
 *              from local table.
 *
 *  @param      handle    Handle to the nameserver.
 *  @param      name      Entry name.
 *  @param      value     UInt32 value.
 *  @param      len       Length of the buffer. Returns the length matched.
 *
 *  @sa         NameServer_create
 */
Int
NameServer_getLocal (NameServer_Handle handle,
                     String            name,
                     Ptr               value,
                     UInt32          * len)
{
    Int                   status = NameServer_S_SUCCESS;
    NameServerDrv_CmdArgs cmdArgs;

    GT_4trace (curTrace, GT_ENTER, "NameServer_getLocal",
               handle, name, value, len);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name   != NULL));
    GT_assert (curTrace, (value  != NULL));
    GT_assert (curTrace, (len    != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "name: name is null!");
    }
    else if (value == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "value: value is null!");
    }
    else if (len == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "len: length is null!");
    }
    else if (*len == 0u) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocal",
                             status,
                             "*len: length is zero!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getLocal.handle  = handle->knlObject;
        cmdArgs.args.getLocal.name    = name;
        cmdArgs.args.getLocal.nameLen = String_len (name) + 1;
        cmdArgs.args.getLocal.value   = value;
        cmdArgs.args.getLocal.len     = *len;
        status = NameServerDrv_ioctl (CMD_NAMESERVER_GETLOCAL, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_getLocal",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Return updated len */
            *len = cmdArgs.args.get.len;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_getLocal", status);

    return status;
}


/* Gets a 32-bit value by name */
Int
NameServer_getUInt32 (NameServer_Handle handle,
                      String            name,
                      Ptr               value,
                      UInt16            procId[])
{
    Int    status = NameServer_S_SUCCESS;
    UInt32 len    = sizeof (UInt32);

    GT_4trace (curTrace, GT_ENTER, "NameServer_getUInt32",
               handle, name, value, procId);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name   != NULL));
    GT_assert (curTrace, (value  != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getUInt32",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getUInt32",
                             status,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getUInt32",
                             status,
                             "name: name is null!");
    }
    else if (value == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getUInt32",
                             status,
                             "value: value is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = NameServer_get (handle, name, value, &len, procId);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if ((status < 0) && (status != NameServer_E_NOTFOUND)) {
            /* NameServer_E_NOTFOUND is a valid run-time failure. */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_getUInt32",
                                 status,
                                 "NameServer_get failed!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_getUInt32", status);

    return status;
}


/* Gets a 32-bit value by name from the local table */
Int
NameServer_getLocalUInt32 (NameServer_Handle handle,
                           String            name,
                           Ptr               value)
{
    Int    status = NameServer_S_SUCCESS;
    UInt32 len    = sizeof (UInt32);

    GT_3trace (curTrace, GT_ENTER, "NameServer_getLocalUInt32",
               handle, name, value);

    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (name   != NULL));
    GT_assert (curTrace, (value  != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        status = NameServer_E_INVALIDSTATE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocalUInt32",
                             status,
                             "Module is in invalid state!");
    }
    else if (handle == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocalUInt32",
                             status,
                             "handle: Handle is null!");
    }
    else if (name == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocalUInt32",
                             status,
                             "name: name is null!");
    }
    else if (value == NULL) {
        status = NameServer_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getLocalUInt32",
                             status,
                             "value: value is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        status = NameServer_getLocal (handle, name, value, &len);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if ((status < 0) && (status != NameServer_E_NOTFOUND)) {
            /* NameServer_E_NOTFOUND is a valid run-time failure. */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_getLocalUInt32",
                                 status,
                                 "NameServer_getLocal failed!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_getLocalUInt32", status);

    return status;
}


/*  Function to retrieve the value portion of a name/value pair from
 *  local table.
 */
Int
NameServer_match (NameServer_Handle handle,
                  String            name,
                  UInt32 *          value)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int         status = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    UInt32      foundLen = 0;
    NameServerDrv_CmdArgs cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "NameServer_match", handle, name, value);

    GT_assert (curTrace, (name != NULL));
    GT_assert (curTrace, (handle != NULL));
    GT_assert (curTrace, (value != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_match",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (name == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_match",
                             NameServer_E_INVALIDARG,
                             "name is null!");
    }
    else if (handle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_match",
                             NameServer_E_INVALIDARG,
                             "handle is null!");
    }
    else if (value == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_match",
                             NameServer_E_INVALIDARG,
                             "value is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.match.handle  = handle->knlObject;
        cmdArgs.args.match.name    = name;
        cmdArgs.args.match.nameLen = String_len (name) + 1;
        cmdArgs.args.match.value   = *value;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_MATCH, &cmdArgs);
        foundLen = cmdArgs.args.match.count;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_match",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_match", foundLen);

    return foundLen;
}


/* Function to retrive a NameServer handle from name. */
NameServer_Handle
NameServer_getHandle (String name)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int         status = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    NameServer_Handle handle = NULL;
    NameServerDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "NameServer_getHandle", name);

    GT_assert (curTrace, (name != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getHandle",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (name == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_getHandle",
                             NameServer_E_INVALIDARG,
                             "name is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.getHandle.name    = name;
        cmdArgs.args.getHandle.nameLen = String_len (name) + 1;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_GETHANDLE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_getHandle",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            /* Allocate memory for the handle */
            handle = (NameServer_Handle) Memory_calloc (NULL,
                                                   sizeof (NameServer_Object),
                                                   0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (handle == NULL) {
                GT_setFailureReason (curTrace,
                                     GT_4CLASS,
                                     "NameServer_getHandle",
                                     NameServer_E_MEMORY,
                                     "Memory allocation failed for handle!");
            }
            else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
                /* Set pointer to kernel object into the user handle. */
                handle->knlObject = cmdArgs.args.getHandle.handle;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            }
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_getHandle", handle);

    return handle;
}


/* =============================================================================
 * Internal functions
 * =============================================================================
 */
/* Determines if a remote driver is registered for the specified id. */
Bool
NameServer_isRegistered (UInt16 procId)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                   status     = NameServer_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Bool                  registered = FALSE;
    NameServerDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "NameServer_isRegistered", procId);

    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (NameServer_module->refCount == 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_isRegistered",
                             NameServer_E_INVALIDSTATE,
                             "Module is in invalid state!");
    }
    else if (procId >= MultiProc_getNumProcessors()) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "NameServer_isRegistered",
                             NameServer_E_INVALIDARG,
                             "Invalid procid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.args.isRegistered.procId  = procId;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NameServerDrv_ioctl (CMD_NAMESERVER_ISREGISTERED, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "NameServer_isRegistered",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            registered = cmdArgs.args.isRegistered.check;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "NameServer_isRegistered", registered);

    return registered;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
