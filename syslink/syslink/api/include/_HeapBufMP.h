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
 *  @file   _HeapBufMP.h
 *
 *  @brief  Internal definitions  for HeapBufMP based memory allocator
 *          internal structure definitions.
 *
 *  ============================================================================
 */


#ifndef HeapBufMP_H_0x4C57
#define HeapBufMP_H_0x4C57


/* Osal & Utils headers */
#include <ti/ipc/GateMP.h>
#include <ti/ipc/HeapBufMP.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 * Macros
 * =============================================================================
 */
/*!
 *  @def    HEAPBUFMP_MODULEID
 *  @brief  Unique module ID.
 */
#define HeapBufMP_MODULEID        (0x4CD7)

/*!
 *  @var    HeapBufMP_CREATED
 *
 *  @brief  HeapBufMP tag used in the attrs->status field
 */
#define HeapBufMP_CREATED     0x05251995

/*
 *  @brief  Object for the HeapBufMP Handle
 */
#define HeapBufMP_Object IHeap_Object


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure defining config parameters for the HeapBufMP module.
 */
typedef struct HeapBufMP_Config_tag {
    UInt32      maxRunTimeEntries;
    /*!< Maximum number of HeapBufMP instances that can be created */
    UInt32      maxNameLen;
    /*!< Maximum length of name */
    Bool        trackAllocs;
    /*!<
     *  Track the number of allocated blocks
     *
     *  This will enable/disable the tracking of the current and maximum number
     *  of allocations for a HeapBufMP instance.  This maximum refers to the
     *  "all time" maximum number of allocations for the history of a HeapBufMP
     *  instance.
     *
     *  Tracking allocations might adversely affect performance when allocating
     *  and/or freeing.  This is especially true if cache is enabled for the
     *  shared region.  If this feature is not needed, setting this to false
     *  avoids the performance penalty.
     */
} HeapBufMP_Config;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Get the default configuration for the HeapBufMP module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to HeapBufMP_setup filled in by
 *              the HeapBufMP module with the default parameters. If the
 *              user does not wish to make any change in the default parameters,
 *              this API is not required to be called.
 *
 *  @param      cfgParams  Pointer to the HeapBufMP module configuration
 *                         structure in which the default config is to be
 *                         returned.
 *
 *  @sa         HeapBufMP_setup
 */
Void HeapBufMP_getConfig (HeapBufMP_Config * cfgParams);

/*!
 *  @brief      Setup the HeapBufMP module.
 *
 *              This function sets up the HeapBufMP module. This function
 *              must be called before any other instance-level APIs can be
 *              invoked.
 *              Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then HeapBufMP_getConfig can be called to get
 *              the configuration filled with the default values. After this,
 *              only the required configuration values can be changed. If the
 *              user does not wish to make any change in the default parameters,
 *              the application can simply call HeapBufMP_setup with NULL
 *              parameters. The default parameters would get automatically used.
 *
 *  @param      cfg   Optional HeapBufMP module configuration. If provided
 *                    as NULL, default configuration is used.
 *
 *  @sa        HeapBufMP_destroy
 *             HeapBufMP_getConfig
 *             NameServer_create
 *             NameServer_delete
 *             GateMutex_delete
 */
Int HeapBufMP_setup (const HeapBufMP_Config * config);

/*!
 *  @brief      Function to destroy the HeapBufMP module.
 *
 *  @param      None
 *
 *  @sa         HeapBufMP_setup
 *              HeapBufMP_getConfig
 *              NameServer_create
 *              NameServer_delete
 *              GateMutex_delete
 */
Int HeapBufMP_destroy (void);

/*!
 *  @brief      Returns the HeapBufMP kernel object pointer.
 *
 *  @param      handle  Handle to previousely created/opened instance.
 *
 */
Ptr HeapBufMP_getKnlHandle (HeapBufMP_Handle hpHandle);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* HeapBufMP_H_0x4C57 */
