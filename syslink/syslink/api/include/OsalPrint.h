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
 *  @file   OsalPrint.h
 *
 *  @brief      OS-specific utils Print interface definitions.
 *
 *              This will have the definitions for printf
 *              statements and also details of variable printfs
 *              supported in existing implementation.
 *
 *  ============================================================================
 */

#ifndef OSALPRINT_H_0xC431
#define OSALPRINT_H_0xC431

/* Standard headers */
#include <Std.h>

/* Linux OS-specific headers */
#include <stdio.h>


/*!
 *  @def    OSALPRINT_MODULEID
 *  @brief  Module ID for OsalPrint OSAL module.
 */
#define OSALPRINT_MODULEID                 (UInt16) 0xC431

#if defined (HAVE_ANDROID_OS) && defined (SYSLINK_DIRECT_LOGD)

#ifndef LOG_TAG
#define LOG_TAG "SYSLINK"
#endif

#include <utils/Log.h>

#define Osal_printf(...) LOGD( __VA_ARGS__ )

#else /* defined (HAVE_ANDROID_OS) && defined (SYSLINK_DIRECT_LOGD) */

#if defined (__cplusplus)
extern "C" {
#endif

#if defined (HAVE_ANDROID_OS)

/* Defines for configuring print */
typedef enum {
    Osal_PrintToConsole = 0,
    Osal_PrintToLogcat  = 1
} Osal_PrintConfig;

/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Configure print mechanism */
Void Osal_configPrint (Osal_PrintConfig printType);

#endif /*  defined (HAVE_ANDROID_OS) */

/*  printf abstraction at the kernel level. */
Void Osal_printf (Char* format, ...);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* defined (HAVE_ANDROID_OS) && defined (SYSLINK_DIRECT_LOGD) */

#endif /* ifndef OSALPRINT_H_0xC431 */
