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
 *  @file   Gate.c
 *
 *  @brief      gate wrapper implementation
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <pthread.h>

/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <IGateProvider.h>
#include <Gate.h>


pthread_mutex_t staticMutex = PTHREAD_MUTEX_INITIALIZER;

#if defined (__cplusplus)
extern "C" {
#endif

/* Structure defining internal object for the Gate Peterson.*/
struct Gate_Object {
    IGateProvider_SuperObject; /* For inheritance from IGateProvider */
};

/* Function to enter a Gate */
IArg  Gate_enterSystem (void)
{
    unsigned long flags = 0u;

    pthread_mutex_lock  (&staticMutex);

    return (IArg)flags;
}


/* Function to leave a Gate */
Void Gate_leaveSystem (IArg key)
{
    pthread_mutex_unlock  (&staticMutex);
}

/* Match with IGateProvider */
static inline IArg _Gate_enterSystem (struct Gate_Object * obj)
{
    (Void) obj;
    return Gate_enterSystem ();
}

/* Match with IGateProvider */
static inline Void _Gate_leaveSystem (struct Gate_Object * obj, IArg key)
{
    (Void) obj;
    Gate_leaveSystem (key);
}

struct Gate_Object Gate_systemObject =
{
    .enter = (IGateProvider_ENTER)_Gate_enterSystem,
    .leave = (IGateProvider_LEAVE)_Gate_leaveSystem,
};

IGateProvider_Handle Gate_systemHandle = (IGateProvider_Handle)&Gate_systemObject;

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
