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
 *  @file   OsalSemaphore.c
 *
 *  @brief  Semaphore interface implementation.
 *
 *  ============================================================================
 */

/* Standard headers */
#include <Std.h>

/* OSAL and utils headers */
#include <OsalSemaphore.h>
#include <Trace.h>
#include <Memory.h>

/* Linux headers */
#include <semaphore.h>
#include <errno.h>
#if defined(OSALSEMAPHORE_USE_TIMEDWAIT)
#include <time.h>
#endif

#if defined (__cplusplus)
extern "C" {
#endif


#define MAX_SCHEDULE_TIMEOUT 10000

/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief   Defines object to encapsulate the Semaphore.
 *           The definition is OS/platform specific.
 */
typedef struct OsalSemaphore_Object_tag {
    OsalSemaphore_Type  semType;
    /*!< Indicates the type of the semaphore (binary or counting). */
    UInt32 value;
    /*!< Current status of semaphore (0,1) - binary and (0-n) counting. */
    sem_t lock;
    /*!< lock on which this Semaphore is based. */
} OsalSemaphore_Object;


/* =============================================================================
 * APIs
 * =============================================================================
 */
/*!
 *  @brief      Creates an instance of Mutex object.
 *
 *  @param      semType Type of semaphore. This parameter is a mask of semaphore
 *                      type and interruptability type.
 *
 *  @sa         OsalSemaphore_delete
 */
OsalSemaphore_Handle OsalSemaphore_create(UInt32 semType, UInt32 semValue)
{
    Int status = OSALSEMAPHORE_SUCCESS;
    OsalSemaphore_Object * semObj = NULL;
    int osStatus = 0;

    GT_1trace (curTrace, GT_ENTER, "OsalSemaphore_create", semType);

    /* Check for semaphore type (binary/counting) */
    GT_assert (curTrace,
               ((OSALSEMAPHORE_TYPE_VALUE(semType))
                <   OsalSemaphore_Type_EndValue));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (OSALSEMAPHORE_TYPE_VALUE(semType) >= OsalSemaphore_Type_EndValue) {
        GT_setFailureReason (curTrace,
                        GT_4CLASS,
                        "OsalSemaphore_create",
                        (unsigned int)OSALSEMAPHORE_E_INVALIDARG,
                        "Invalid semaphore type (OsalSemaphore_Type) provided");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        semObj = Memory_calloc (NULL, sizeof (OsalSemaphore_Object), 0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (semObj == NULL) {
            GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_create",
                             (unsigned int)OSALSEMAPHORE_E_MEMORY,
                             "Failed to allocate memory for semaphore object.");
        }
        else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
            semObj->semType = semType;
            semObj->value = semValue;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if ((OSALSEMAPHORE_TYPE_VALUE (semObj->semType)
                ==  OsalSemaphore_Type_Binary) && (semValue > 1)){
                /*! @retVal NULL Invalid semaphore value. */
                status = OSALSEMAPHORE_E_INVALIDARG;
                GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "OsalSemaphore_create",
                            status,
                            "Invalid semaphore value");
            }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
            osStatus = sem_init(&(semObj->lock), 0, semValue);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (osStatus < 0) {
                /*! @retVal NULL Failed to initialize semaphore. */
                status = OSALSEMAPHORE_E_RESOURCE;
                GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_create",
                             status,
                             "Failed to initialize semaphore");
            }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalSemaphore_create", semObj);

    return (OsalSemaphore_Handle) semObj;
}


/*!
 *  @brief      Deletes an instance of Semaphore object.
 *
 *  @param      mutexHandle   Semaphore object handle which needs to be deleted.
 *
 *  @sa         OsalSemaphore_create
 */
Int
OsalSemaphore_delete(OsalSemaphore_Handle *semHandle)
{
    Int status = OSALSEMAPHORE_SUCCESS;
    OsalSemaphore_Object **semObj = (OsalSemaphore_Object **)semHandle;
    int osStatus = 0;

    GT_1trace (curTrace, GT_ENTER, "OsalSemaphore_delete", semHandle);

    GT_assert (curTrace, (semHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (semHandle == NULL) {
        status = OSALSEMAPHORE_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_delete",
                             status,
                             "NULL provided for argument semHandle");
    }
    else if (*semHandle == NULL) {
        status = OSALSEMAPHORE_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_delete",
                             status,
                             "NULL Semaphore handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
            osStatus = sem_destroy(&((*semObj)->lock));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (osStatus < 0) {
                   status = OSALSEMAPHORE_E_HANDLE;
                GT_setFailureReason (curTrace,
                            GT_4CLASS,
                            "OsalSemaphore_delete",
                            status,
                            "Failed to destroy semaphore");
               }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        Memory_free(NULL, *semHandle, sizeof (OsalSemaphore_Object));
        *semHandle = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalSemaphore_delete", status);

    return status;
}


/*!
 *  @brief      Wait on the Semaphore in the kernel thread context
 *
 *  @param      semHandle   Semaphore object handle
 *  @param      timeout     Timeout (in msec). Special values are provided for
 *                          no-wait and infinite-wait.
 *
 *  @sa         OsalSemaphore_post
 */
Int
OsalSemaphore_pend(OsalSemaphore_Handle semHandle, UInt32 timeout)
{
    Int                     status      = OSALSEMAPHORE_SUCCESS;
    OsalSemaphore_Object *  semObj      = (OsalSemaphore_Object *) semHandle;
    int                     osStatus    = 0;
#if defined(OSALSEMAPHORE_USE_TIMEDWAIT)
    struct timespec         absTimeout;
#endif /* #if defined(OSALSEMAPHORE_USE_TIMEDWAIT) */

    GT_2trace (curTrace, GT_ENTER, "OsalSemaphore_pend", semHandle, timeout);

    GT_assert (curTrace, (semHandle != NULL));
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (semHandle == NULL) {
        status = OSALSEMAPHORE_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_pend",
                             status,
                             "NULL Semaphore handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Different handling for no-timeout case. */
        if (timeout == OSALSEMAPHORE_WAIT_NONE) {
            osStatus = sem_trywait(&(semObj->lock));
            GT_assert (curTrace, (osStatus == 0));
            if (osStatus != 0) {
                if (errno == EAGAIN) {
                    status = OSALSEMAPHORE_E_WAITNONE;
                    GT_1trace (curTrace,
                               GT_4CLASS,
                               "OsalSemaphore_pend"
                               "WAIT_NONE timeout value was provided,"
                               " semaphore was not available.",
                               status);
                }
                else {
                    status = OSALSEMAPHORE_E_RESOURCE;
                    GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OsalSemaphore_pend",
                                  status,
                                  "Failure in sem_trywait()");
                }
            }
            else if(semObj->value > 0) {
                if (OSALSEMAPHORE_TYPE_VALUE (semObj->semType)
                        ==  OsalSemaphore_Type_Binary) {
                    semObj->value = 0;
                }
                else {
                    semObj->value--;
                }
            }
        }
        /* Finite and infinite timeout cases */
        else {
#if defined(OSALSEMAPHORE_USE_TIMEDWAIT)
            /* Temporarily disabled as sem_timedwait is reflecting issues */
            /* Get timeout value in OS-recognizable format. */
            if (timeout == OSALSEMAPHORE_WAIT_FOREVER) {
                clock_gettime(CLOCK_MONOTONIC, &absTimeout);
                absTimeout.tv_sec += (MAX_SCHEDULE_TIMEOUT/1000);
                absTimeout.tv_nsec += (MAX_SCHEDULE_TIMEOUT % 1000) * 1000000;
            }
            else {
                clock_gettime(CLOCK_MONOTONIC, &absTimeout);
                absTimeout.tv_sec += (timeout/1000);
                absTimeout.tv_nsec += (timeout % 1000) * 1000000;
            }

            osStatus = sem_timedwait(&(semObj->lock), &absTimeout);
            GT_assert (curTrace, (osStatus == 0));
            if (osStatus != 0) {
                if (errno == ETIMEDOUT) {
                    status = OSALSEMAPHORE_E_WAITNONE;
                    GT_1trace (curTrace,
                               GT_LEAVE,
                               "sem_timedwait() timed out",
                               osStatus);
                }
                else {
                    status = OSALSEMAPHORE_E_RESOURCE;
                    GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "OsalSemaphore_pend",
                                  status,
                                  "Failure in sem_timedwait()");
                }
            }
#else
            osStatus = sem_wait(&(semObj->lock));
            GT_assert (curTrace, (osStatus == 0));
            if (osStatus != 0) {
                status = OSALSEMAPHORE_E_RESOURCE;
                GT_setFailureReason (curTrace,
                              GT_4CLASS,
                              "OsalSemaphore_pend",
                              status,
                              "Failure in sem_wait()");
            }
#endif /* #if defined(OSALSEMAPHORE_USE_TIMEDWAIT) */
            else if(semObj->value > 0) {
                if (OSALSEMAPHORE_TYPE_VALUE (semObj->semType)
                        ==  OsalSemaphore_Type_Binary) {
                    semObj->value = 0;
                }
                else {
                    semObj->value--;
                }
            }
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalSemaphore_pend", status);

    return status;
}

/*!
 *  @brief      Signals the semaphore and makes it available for other
 *              threads.
 *
 *  @param      semHandle Semaphore object handle
 *
 *  @sa         OsalSemaphore_pend
 */
Int
OsalSemaphore_post (OsalSemaphore_Handle semHandle)
{
    Int                     status      = OSALSEMAPHORE_SUCCESS;
    OsalSemaphore_Object *  semObj      = (OsalSemaphore_Object *) semHandle;
    int                     osStatus    = 0;

    GT_1trace (curTrace, GT_ENTER, "OsalSemaphore_post", semHandle);

    GT_assert (curTrace, (semHandle != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (semHandle == NULL) {
        status = OSALSEMAPHORE_E_HANDLE;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "OsalSemaphore_post",
                             status,
                             "NULL Semaphore handle provided.");
    }
    else {
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */
        if (OSALSEMAPHORE_TYPE_VALUE (semObj->semType)
                ==  OsalSemaphore_Type_Binary) {
            semObj->value = 1;
        }
        else {
            semObj->value++;
        }
        osStatus = sem_post(&(semObj->lock));
        GT_assert (curTrace, (osStatus == 0));
        if (osStatus != 0) {
            status = OSALSEMAPHORE_E_RESOURCE;
            GT_1trace (curTrace,
                       GT_LEAVE,
                       "Error in sem_post",
                       osStatus);
        }
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* #if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "OsalSemaphore_post", status);

    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (_cplusplus)*/
