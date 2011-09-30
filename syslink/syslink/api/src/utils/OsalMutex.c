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
/*==============================================================================
 *  @file   OsalMutex.c
 *
 *  @brief      Linux user (pthread) Mutex interface implementation.
 *
 *              pthread Mutex control provided at the user
 *              level with the help of Mutex objects. Interface do not
 *              contain much of the state informations and are independent.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL and utils headers */
#include <OsalMutex.h>
#include <Trace.h>
#include <Memory.h>

/* Linux specific header files */
#include <pthread.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief   Defines object to encapsulate the Mutex.
 *           The definition is OS/platform specific.
 */
typedef struct OsalMutex_Object_tag {
    pthread_mutex_t lock; /*!< OS specific mutex lock */
    OsalMutex_Type  type; /*!< Type of mutex. */
} OsalMutex_Object;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief      Creates an instance of Mutex object.
 *
 *  @param      type Type of mutex.
 *
 *  @sa         OsalMutex_delete, Memory_alloc
 */
OsalMutex_Handle
OsalMutex_create (OsalMutex_Type type)
{
    OsalMutex_Object *  mutexObj = NULL;

    GT_1trace (curTrace, GT_ENTER, "OsalMutex_create", type);

    GT_assert (curTrace, (type < OsalMutex_Type_EndValue));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (type >= OsalMutex_Type_EndValue) {
        /*! @retval NULL Invalid value provided for argument type. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalMutex_create",
                             OSALMUTEX_E_INVALIDARG,
                             "Invalid value provided for argument type.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        mutexObj = Memory_calloc (NULL, sizeof (OsalMutex_Object), 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (mutexObj == NULL) {
            /*! @retval NULL Failed to allocate memory for Mutex object. */
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "OsalIsr_create",
                                 OSALMUTEX_E_MEMORY,
                                 "Failed to allocate memory for Mutex object.");
        }
        else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
            pthread_mutex_init (&(mutexObj->lock), NULL);
            mutexObj->type = type;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalMutex_create", mutexObj);

    /*! @retval Mutex-handle Operation successfully completed. */
    return (OsalMutex_Handle) mutexObj;
}


/*!
 *  @brief      Deletes an instance of Mutex object.
 *
 *  @param      mutexHandle   Mutex object handle which needs to be deleted.
 *
 *  @sa         OsalMutex_create, Memory_free
 */
Int
OsalMutex_delete (OsalMutex_Handle * mutexHandle)
{
    Int status = OSALMUTEX_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "OsalMutex_delete", mutexHandle);

    GT_assert (curTrace, (mutexHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (mutexHandle == NULL) {
        /*! @retval OSALMUTEX_E_INVALIDARG NULL provided for argument
                                           mutexHandle.*/
        status = OSALMUTEX_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalMutex_delete",
                             status,
                             "NULL provided for argument mutexHandle");
    }
    else if (*mutexHandle == NULL) {
        /*! @retval OSALMUTEX_E_HANDLE NULL Mutex handle provided. */
        status = OSALMUTEX_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalMutex_delete",
                             status,
                             "NULL Mutex handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        Memory_free (NULL, *mutexHandle, sizeof (OsalMutex_Object));
        *mutexHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalMutex_delete", status);

    /*! @retval OSALMUTEX_SUCCESS Operation successfully completed. */
    return status;
}


/*!
 *  @brief      Acquire/lock the Mutex object.
 *
 *              This function acquires the critical section identified by this
 *              mutex handle. Once this function is successfully entered,
 *              further calls to OsalMutex_enter shall block till the critical
 *              section has been released by the caller.
 *
 *  @param      mutexHandle   Mutex object handle to be acquired.
 *
 *  @sa         OsalMutex_leave
 */
IArg
OsalMutex_enter (OsalMutex_Handle mutexHandle)
{
    IArg                retVal      = 0;
    OsalMutex_Object *  mutexObj    = (OsalMutex_Object*) mutexHandle;

    GT_1trace (curTrace, GT_ENTER, "OsalMutex_enter", mutexHandle);

    GT_assert (curTrace, (mutexHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (mutexHandle == NULL) {
        /* Function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalMutex_enter",
                             OSALMUTEX_E_HANDLE,
                             "NULL Mutex handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        pthread_mutex_lock (&(mutexObj->lock));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalMutex_enter", retVal);

    /*! @retval 0 Operation successfully completed. */
    return retVal;
}

/*!
 *  @brief      Leave/unlock the Mutex object.
 *
 *              This function releases the critical section identified by this
 *              mutex handle. The completion of this function shall make the
 *              critical section available to any other threads that may be
 *              waiting to acquire it.
 *
 *  @param      mutexHandle   Mutex object handle to be released.
 *  @param      key           Key received from the corresponding enter call.
 *
 *  @sa         OsalMutex_enter
 */
Void
OsalMutex_leave (OsalMutex_Handle mutexHandle, IArg   key)
{
    OsalMutex_Object * mutexObj = (OsalMutex_Object*) mutexHandle;

    GT_2trace (curTrace, GT_ENTER, "OsalMutex_leave", mutexHandle, key);

    GT_assert (curTrace, (mutexHandle != NULL));
    /* key can be 0, so not checked. */

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (mutexHandle == NULL) {
        /* Void function, so not setting status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalMutex_leave",
                             OSALMUTEX_E_HANDLE,
                             "NULL Mutex handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        pthread_mutex_unlock (&(mutexObj->lock));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "OsalMutex_leave");
}


#if defined (__cplusplus)
}
#endif /* defined (_cplusplus)*/
