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
 *  @file   OsalMutex.h
 *
 *  @brief      Kernel mutex interface.
 *
 *              Mutex control provided at the kernel
 *              level with the help of Mutex objects. Interface do not
 *              contain much of the state informations and are independent.
 *
 *  ============================================================================
 */

#ifndef OSALMUTEX_H_0xE4AF
#define OSALMUTEX_H_0xE4AF


/* OSAL and utils */


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @def    OSALMUTEX_MODULEID
 *  @brief  Module ID for OsalMutex OSAL module.
 */
#define OSALMUTEX_MODULEID                 (UInt16) 0xE4AF

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
* @def   OSALMUTEX_STATUSCODEBASE
* @brief Stauts code base for OsalMutex module.
*/
#define OSALMUTEX_STATUSCODEBASE      (OSALMUTEX_MODULEID << 12u)

/*!
* @def   OSALMUTEX_MAKE_FAILURE
* @brief Convert to failure code.
*/
#define OSALMUTEX_MAKE_FAILURE(x)    ((Int) (0x80000000  \
                                      + (OSALMUTEX_STATUSCODEBASE + (x))))
/*!
* @def   OSALMUTEX_MAKE_SUCCESS
* @brief Convert to success code.
*/
#define OSALMUTEX_MAKE_SUCCESS(x)      (OSALMUTEX_STATUSCODEBASE + (x))

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */

/*!
* @def   OSALMUTEX_E_MEMORY
* @brief Indicates Memory alloc/free failure.
*/
#define OSALMUTEX_E_MEMORY             OSALMUTEX_MAKE_FAILURE(1)

/*!
* @def   OSALMUTEX_E_INVALIDARG
* @brief Invalid argument provided
*/
#define OSALMUTEX_E_INVALIDARG         OSALMUTEX_MAKE_FAILURE(2)

/*!
* @def   OSALMUTEX_E_FAIL
* @brief Generic failure
*/
#define OSALMUTEX_E_FAIL               OSALMUTEX_MAKE_FAILURE(3)

/*!
* @def   OSALMUTEX_E_TIMEOUT
* @brief A timeout occurred
*/
#define OSALMUTEX_E_TIMEOUT            OSALMUTEX_MAKE_FAILURE(4)

/*!
 *  @def    OSALMUTEX_E_HANDLE
 *  @brief  Invalid handle provided
 */
#define OSALMUTEX_E_HANDLE             OSALMUTEX_MAKE_FAILURE(5)

/*!
* @def   OSALMUTEX_SUCCESS
* @brief Operation successfully completed
*/
#define OSALMUTEX_SUCCESS              OSALMUTEX_MAKE_SUCCESS(0)


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  Declaration for the OsalSpinlock object handle.
 *          Definition of OsalMutex_Object is not exposed.
 */
typedef struct OsalMutex_Object * OsalMutex_Handle;

/*!
 *  @brief   Enumerates the types of spinlocks
 */
typedef enum {
    OsalMutex_Type_Interruptible    = 0u,
    /*!< Waits on this mutex are interruptible */
    OsalMutex_Type_Noninterruptible = 1u,
    /*!< Waits on this mutex are non-interruptible */
    OsalMutex_Type_EndValue         = 2u
    /*!< End delimiter indicating start of invalid values for this enum */
} OsalMutex_Type;


/* =============================================================================
 *  APIs
 * =============================================================================
 */

/* Creates a new instance of Mutex object. */
OsalMutex_Handle OsalMutex_create (OsalMutex_Type type);

/* Deletes an instance of Mutex object. */
Int OsalMutex_delete (OsalMutex_Handle * mutexHandle);

/* Enters the critical section indicated by this Mutex object. Returns key. */
IArg OsalMutex_enter (OsalMutex_Handle mutexHandle);

/* Leaves the critical section indicated by this Mutex object.
 * Takes in key received from enter.
 */
Void OsalMutex_leave (OsalMutex_Handle mutexHandle, IArg   key);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* ifndef OSALMUTEX_H_0x5E93 */
