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
 *  @file   _HeapMemMP.h
 *
 *  @brief  Defines HeapMemMP based memory allocator.
 *  ============================================================================
 */


#ifndef HeapMemMP_H_0x4C56
#define HeapMemMP_H_0x4C56


/* Osal & Utils headers */
#include <IHeap.h>
#include <ti/ipc/GateMP.h>
#include <ti/ipc/HeapMemMP.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    HeapMemMP_MODULEID
 *  @brief  Unique module ID.
 */
#define HeapMemMP_MODULEID        (0x4CD7)

/*!
 *  @var    HeapMemMP_CREATED
 *
 *  @brief  HeapMemMp tag used in the attrs->status field
 */
#define HeapMemMP_CREATED     0x07041776

/*
 *  @brief  Object for the HeapMemMP Handle
 */
#define HeapMemMP_Object IHeap_Object


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining config parameters for the HeapMemMP module.
 */
typedef struct HeapMemMP_Config_tag {
    UInt32      maxNameLen;
    /*!< Maximum length of name */
    UInt32      maxRunTimeEntries;
    /*!< Maximum number of entries */
} HeapMemMP_Config;


/* =============================================================================
 *  APIs
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
Void HeapMemMP_getConfig (HeapMemMP_Config * cfgParams);

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
 *  @param      cfg   Optional HeapMemMP module configuration. If provided
 *                    as NULL, default configuration is used.
 *
 *  @sa        HeapMemMP_destroy
 *             HeapMemMP_getConfig
 *             NameServer_create
 *             NameServer_delete
 *             GateMutex_delete
 */
Int HeapMemMP_setup (const HeapMemMP_Config * config);

/*!
 *  @brief      Function to destroy the HeapMemMP module.
 *
 *  @param      None
 *
 *  @sa         HeapMemMP_setup
 *              HeapMemMP_getConfig
 *              NameServer_create
 *              NameServer_delete
 *              GateMutex_delete
 */
Int HeapMemMP_destroy (Void);

/*!
 *  @brief      Returns the HeapMemMP kernel object pointer.
 *
 *  @param      handle  Handle to previousely created/opened instance.
 *
 */
Ptr HeapMemMP_getKnlHandle (HeapMemMP_Handle hpHandle);

/*!
 *  @brief      Returns a HeapMemMP user object pointer.
 *
 *  @param      handle  Handle to kernel handle for which user handle is to be
 *                      provided.
 *
 */
Ptr HeapMemMP_getUsrHandle (HeapMemMP_Handle hpHandle);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* HeapMemMP_H_0x4C56 */
