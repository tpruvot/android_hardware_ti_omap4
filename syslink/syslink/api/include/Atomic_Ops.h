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
 *  @file   Atomic_Ops.h
 *
 *  @brief      Atomic operations abstraction
 *
 *
 *  @ver        2.00.00.06
 *
 *  ============================================================================
 */


#ifndef  ATOMIC_OPS_H
#define  ATOMIC_OPS_H

/* Standard libc headers */
#include <stdio.h>
#include <pthread.h>


/* =============================================================================
 * Typedef
 * =============================================================================
 */
/*! @brief Typedef for atomic variable */
typedef UInt32 Atomic;


/* =============================================================================
 * Global
 * =============================================================================
 */
/* Lock used by atomic operations to create psuedo atomicity */
static pthread_mutex_t Atomic_lock = PTHREAD_MUTEX_INITIALIZER;


/* =============================================================================
 * APIs & Macros
 * =============================================================================
 */
/*!
 *   @brief Function to read an variable atomically
 *
 *   @param var Pointer to atomic variable
 */
static inline UInt32 Atomic_read (Atomic * var)
{
    UInt32 ret;

    pthread_mutex_lock (&Atomic_lock);
    ret = *var;
    pthread_mutex_unlock (&Atomic_lock);

    /*! @retval value   Current value of the atomic variable */
    return ret;
}


/*!
 *   @brief Function to set an variable atomically
 *
 *   @param var Pointer to atomic variable
 *   @param val Value to be set
 */
static inline void Atomic_set (Atomic * var, UInt32 val)
{
    pthread_mutex_lock (&Atomic_lock);
    *var = val;
    pthread_mutex_unlock (&Atomic_lock);
}


/*!
 *   @brief Function to increment an variable atomically
 *
 *   @param var Pointer to atomic variable
 *   @param val Value to be set
 */
static inline UInt32 Atomic_inc_return (Atomic * var)
{
    UInt32 ret;

    pthread_mutex_lock (&Atomic_lock);
    *var = *var + 1u;
    ret = *var;
    pthread_mutex_unlock (&Atomic_lock);

    /*! @retval value   Current value of the atomic variable */
    return ret;
}


/*!
 *   @brief Function to decrement an variable atomically
 *
 *   @param var Pointer to atomic variable
 *   @param val Value to be set
 */
static inline UInt32 Atomic_dec_return (Atomic * var)
{
    UInt32 ret;

    pthread_mutex_lock (&Atomic_lock);
    *var = *var - 1u;
    ret = *var;
    pthread_mutex_unlock (&Atomic_lock);

    /*! @retval value   Current value of the atomic variable */
    return ret;
}


/*!
 * @brief Function to compare a mask and set if not equal
 *
 * @params v    Pointer to atomic variable
 * @params mask Mask to compare with
 * @params val  Value to be set if mask does not match.
 */
static inline void Atomic_cmpmask_and_set(Atomic * var, UInt32 mask, UInt32 val)
{
    UInt32  ret;

    pthread_mutex_lock (&Atomic_lock);
    ret = *var;
    if ((ret & mask) != mask) {
        *var = val;
    }
    pthread_mutex_unlock (&Atomic_lock);
}

/*!
 * @brief Function to compare a mask and then check current value less than
 *        provided value.
 *
 * @params v    Pointer to atomic variable
 * @params mask Mask to compare with
 * @params val  Value to be set if mask does not match.
 */
static inline Bool Atomic_cmpmask_and_lt(Atomic * var, UInt32 mask, UInt32 val)
{
    Bool   ret = TRUE;
    UInt32 cur = 0;

    pthread_mutex_lock (&Atomic_lock);
    cur = *var;
    if ((cur & mask) == mask) {
        if (cur >= val) {
            ret = FALSE;
        }
    }
    pthread_mutex_unlock (&Atomic_lock);

    /*! @retval TRUE  if mask matches and current value is less than given
     *  value */
    /*! @retval FALSE either mask doesnot matches or current value is not less
     *  than given value */
    return ret;
}

/*!
 * @brief Function to compare a mask and then check current value greater than
 *        provided value.
 *
 * @params v    Pointer to atomic variable
 * @params mask Mask to compare with
 * @params val  Value to be set if mask does not match.
 */
static inline Bool Atomic_cmpmask_and_gt(Atomic * var, UInt32 mask, UInt32 val)
{
    Bool   ret = TRUE;
    UInt32 cur = 0;

    pthread_mutex_lock (&Atomic_lock);
    cur = *var;
    if ((cur & mask) == mask) {
        if (cur < val) {
            ret = FALSE;
        }
    }
    pthread_mutex_unlock (&Atomic_lock);

    /*! @retval TRUE  if mask matches and current value is less than given
     *  value */
    /*! @retval FALSE either mask doesnot matches or current value is not
     *  greater than given value */
    return ret;
}

#endif /* if !defined(ATOMIC_OPS_H) */
