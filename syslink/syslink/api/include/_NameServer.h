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
/** ============================================================================
 *  @file   _NameServer.h
 *
 *  @brief      HLOS-specific NameServer header
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


#ifndef NameServer_H_0XF414
#define NameServer_H_0XF414

/* Utilities headers */
#include <ti/ipc/NameServer.h>
#include <NameServerRemote.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Macros & Defines
 * =============================================================================
 */
/*!
 *  @def    NameServer_MODULEID
 *  @brief  Unique module ID.
 */
#define NameServer_MODULEID      (0xF414)

/* =============================================================================
 * Struct & Enums
 * =============================================================================
 */
/*!
 *  @brief  Module configuration structure.
 */
typedef struct NameServer_Config_tag {
    UInt32 reserved;
    /*!< Reserved value. */
} NameServer_Config;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief      Get the default configuration for the NameServer module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to NameServer_setup filled in by the
 *              NameServer module with the default parameters. If the user
 *              does not wish to make any change in the default parameters, this
 *              API is not required to be called.
 *
 *  @param      cfg        Pointer to the NameServer module configuration
 *                         structure in which the default config is to be
 *                         returned.
 *
 *  @sa         NameServer_setup
 */
Void NameServer_getConfig (NameServer_Config * cfg);

/*!
 *  @brief      Function to setup the nameserver module.
 *
 *  @sa         NameServer_destroy
 */
Int NameServer_setup (Void);

/*!
 *  @brief      Function to destroy the nameserver module.
 *
 *  @sa         NameServer_setup
 */
Int NameServer_destroy (Void);

/*!
 *  @brief      Function to construct a name server.
 *
 *  @sa         NameServer_destruct
 */
Void
NameServer_construct (      NameServer_Handle   object,
                            String              name,
                      const NameServer_Params * params);

/*!
 *  @brief      Function to destruct a name server
 *
 *  @sa         NameServer_construct
 */
Void
NameServer_destruct (NameServer_Handle object);

/*!
 *  @brief      Function to register a remote driver for a processor.
 *
 *  @param      handle  Handle to the nameserver remote driver.
 *  @param      procId  Processor identifier.
 *
 *  @sa         NameServer_unregisterRemoteDriver
 */
Int NameServer_registerRemoteDriver (NameServerRemote_Handle handle,
                                     UInt16                  procId);

/*!
 *  @brief      Function to unregister a remote driver for a processor.
 *
 *  @param      procId  Processor identifier.
 *
 *  @sa         NameServer_unregisterRemoteDriver
 */
Int NameServer_unregisterRemoteDriver (UInt16 procId);

/*!
 *  @brief     Determines if a remote driver is registered for the specified id.
 *
 *  @param     procId  The remote processor id.
 */
Bool NameServer_isRegistered (UInt16 procId);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* NameServer_H_0X5B4D */
