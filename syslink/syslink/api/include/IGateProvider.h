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
 *  @file   IGateProvider.h
 *
 *  @brief      Interface implemented by all gate providers.
 *
 *  Gates are used serialize access to data structures that are used by more
 *  than one thread.
 *
 *  Gates are responsible for ensuring that only one out of multiple threads
 *  can access a data structure at a time.  There
 *  are important scheduling latency and performance considerations that
 *  affect the "type" of gate used to protect each data structure.  For
 *  example, the best way to protect a shared counter is to simply disable
 *  all interrupts before the update and restore the interrupt state after
 *  the update; disabling all interrupts prevents all thread switching, so
 *  the update is guaranteed to be "atomic".  Although highly efficient, this
 *  method of creating atomic sections causes serious system latencies when
 *  the time required to update the data structure can't be bounded.
 *
 *  For example, a memory manager's list of free blocks can grow indefinitely
 *  long during periods of high fragmentation.  Searching such a list with
 *  interrupts disabled would cause system latencies to also become unbounded.
 *  In this case, the best solution is to provide a gate that suspends the
 *  execution of  threads that try to enter a gate that has already been
 *  entered; i.e., the gate "blocks" the thread until the thread
 *  already in the gate leaves.  The time required to enter and leave the
 *  gate is greater than simply enabling and restoring interrupts, but since
 *  the time spent within the gate is relatively large, the overhead caused by
 *  entering and leaving gates will not become a significant percentage of
 *  overall system time.  More importantly, threads that do not need to
 *  access the shared data structure are completely unaffected by threads
 *  that do access it.
 *
 *  ============================================================================
 */


#ifndef __IGATEPROVIDER_H__
#define __IGATEPROVIDER_H__


#if defined (__cplusplus)
extern "C" {
#endif


/* -----------------------------------------------------------------------------
 *  Macros
 * -----------------------------------------------------------------------------
 */
/*! Invalid Igate */
#define IGateProvider_NULL      (IGateProvider_Handle)0xFFFFFFFF

/*!
 *  ======== IGateProvider_Q_BLOCKING ========
 *  Blocking quality
 *
 *  Gates with this "quality" may cause the calling thread to block;
 *  i.e., suspend execution until another thread leaves the gate.
 */
#define IGateProvider_Q_BLOCKING 1

/*!
 *  ======== IGateProvider_Q_PREEMPTING ========
 *  Preempting quality
 *
 *  Gates with this "quality" allow other threads to preempt the thread
 *  that has already entered the gate.
 */
#define IGateProvider_Q_PREEMPTING 2

/*!
 *  ======== IGateProvider_SuperObject ========
 *  Object embedded in other Gate modules. (Inheritance)
 */
#define IGateProvider_SuperObject                                              \
        IGateProvider_ENTER enter;                                                 \
        IGateProvider_LEAVE leave


/*!
 *
 */
#define IGateProvider_ObjectInitializer(x,y)                                   \
        ((IGateProvider_Handle)(x))->enter = (IGateProvider_ENTER)y##_enter;   \
        ((IGateProvider_Handle)(x))->leave = (IGateProvider_LEAVE)y##_leave;

/* -----------------------------------------------------------------------------
 *  Defines
 * -----------------------------------------------------------------------------
 */
/*! Prototype of enter function */
typedef IArg (*IGateProvider_ENTER) (Void *);

/*! Prototype of leave function */
typedef Void (*IGateProvider_LEAVE) (Void *, IArg);


/* -----------------------------------------------------------------------------
 *  Structs & Enums
 * -----------------------------------------------------------------------------
 */
/*!
 * Structure for generic gate instance
 */
typedef struct IGateProvider_Object {
        IGateProvider_SuperObject;
} IGateProvider_Object, *IGateProvider_Handle;


/* -----------------------------------------------------------------------------
 *  APIs
 * -----------------------------------------------------------------------------
 */
/*!
 *  Enter this gate
 *
 *  Each gate provider can implement mutual exclusion using different
 *  algorithms; e.g., disabling all scheduling, disabling the scheduling
 *  of all threads below a specified "priority level", suspending the
 *  caller when the gate has been entered by another thread and
 *  re-enabling it when the the other thread leaves the gate.  However,
 *  in all cases, after this method returns that caller has exclusive
 *  access to the data protected by this gate.
 *
 *  A thread may reenter a gate without blocking or failing.
 *
 *  @param handle Handle to the Gate.
 *
 *  @retval IArg Returns the instance specific return values.
 *
 *  @sa IGateProvider_leave
 *
 */
static inline IArg IGateProvider_enter (IGateProvider_Handle  handle)
{
    IArg key = 0;

    if (handle != IGateProvider_NULL) {
        key = (handle->enter) ((void *)handle);
    }

    return key;
}


/*!
 *  Leave this gate
 *
 *  This method is only called by threads that have previously entered
 *  this gate via `{@link #enter}`.  After this method returns, the
 *  caller must not access the data structure protected by this gate
 *  (unless the caller has entered the gate more than once and other
 *  calls to `leave` remain to balance the number of previous
 *  calls to `enter`).
 *
 *  @param handle Handle to the Gate.
 *  @param key    Instance specific argument.
 *
 *  @sa IGateProvider_enter
 *
 */
static inline Void IGateProvider_leave (IGateProvider_Handle  handle, IArg key)
{
    if (handle != IGateProvider_NULL)
        (handle->leave) ((void *)handle, key);
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* ifndef __IGATEPROVIDER_H__ */
