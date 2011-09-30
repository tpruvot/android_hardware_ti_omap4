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
 *  @file   GateMutex.h
 *
 *  @brief      Gate based on Mutex.
 *  ============================================================================
 */


#ifndef GATEMUTEX_H_0x72D0
#define GATEMUTEX_H_0x72D0


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Status codes
 * =============================================================================
 */
/*!
 *  @def    GateMutex_E_INVALIDARG
 *  @brief  Argument passed to a function is invalid.
 */
#define GateMutex_E_INVALIDARG       -1

/*!
 *  @def    GateMutex_E_MEMORY
 *  @brief  Memory allocation failed.
 */
#define GateMutex_E_MEMORY           -2

/*!
 *  @def    GateMutex_E_BUSY
 *  @brief  The name is already registered or not.
 */
#define GateMutex_E_BUSY             -3

/*!
 *  @def    GateMutex_E_FAIL
 *  @brief  Generic failure.
 */
#define GateMutex_E_FAIL             -4

/*!
 *  @def    GateMutex_E_NOTFOUND
 *  @brief  Name not found in the nameserver.
 */
#define GateMutex_E_NOTFOUND         -5

/*!
 *  @def    GateMutex_E_INVALIDSTATE
 *  @brief  Module is not initialized.
 */
#define GateMutex_E_INVALIDSTATE     -6

/*!
 *  @def    GateMutex_E_INUSE
 *  @brief  Indicates that the instance is in use.
 */
#define GateMutex_E_INUSE            -7

/*!
 *  @def    GateMutex_S_SUCCESS
 *  @brief  Operation successful.
 */
#define GateMutex_S_SUCCESS          0


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*! @brief  Object for Gate Mutex */
typedef struct GateMutex_Object GateMutex_Object;

/*! @brief  Handle for Gate Mutex */
typedef struct GateMutex_Object * GateMutex_Handle;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Function to create a Gate Mutex */
GateMutex_Handle GateMutex_create (Void);

/* Function to delete a Gate Mutex */
Int GateMutex_delete (GateMutex_Handle * gmHandle);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* GATEMUTEX_H_0x72D0 */
