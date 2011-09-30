/*
 *  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/** ============================================================================
 *  @file   notifyxfer_os.c
 *
 *  @desc   OS specific implementation of functions used by the mpcsxfer
 *          sample application.
 *
 *  ============================================================================
 */


/*  ----------------------------------- OS Specific Headers           */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>

/* Standard headers */
#include <Std.h>

/*  ----------------------------------- DSP/BIOS Link                 */
#include <errbase.h>

/*  ----------------------------------- IPC                 */
#include <ipc.h>

/*  ----------------------------------- Application Header            */


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @name   NOTIFYXFER_SemObject
 *
 *  @desc   This object is used by various SEM API's.
 *
 *  @field  sem
 *              Linux semaphore.
 *
 *  @see    None
 *  ============================================================================
 */
typedef struct NOTIFYXFER_SemObject_tag {
    sem_t  sem ;
} NOTIFYXFER_SemObject ;


/** ============================================================================
 *  @func   NOTIFYXFER_0Print
 *
 *  @desc   Print a message without any arguments.
 *
 *  @modif  None
 *  ============================================================================
 */

Void
NOTIFYXFER_0Print (Char8 * str)
{
    printf (str) ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_1Print
 *
 *  @desc   Print a message with one arguments.
 *
 *  @modif  None
 *  ============================================================================
 */

Void
NOTIFYXFER_1Print (Char8 * str, Uint32 arg)
{
    printf (str, arg) ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_Sleep
 *
 *  @desc   Sleeps for the specified number of microseconds.
 *
 *  @modif  None
 *  ============================================================================
 */

Void
NOTIFYXFER_Sleep (Uint32 uSec)
{
    usleep (uSec) ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_CreateSem
 *
 *  @desc   This function creates a semaphore.
 *
 *  @modif  None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_CreateSem (OUT PVOID * semPtr)
{
    DSP_STATUS           status = DSP_SOK ;
    NOTIFYXFER_SemObject * semObj ;
    int                  osStatus ;

    semObj = malloc (sizeof (NOTIFYXFER_SemObject)) ;
    if (semObj != NULL) {
        osStatus = sem_init (&(semObj->sem), 0, 0) ;
        if (osStatus < 0) {
            status = DSP_EFAIL ;
        }
        else {
            *semPtr = (PVOID) semObj ;
        }
    }
    else {
        *semPtr = NULL ;
        status = DSP_EFAIL ;
    }

    return status ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_DeleteSem
 *
 *  @desc   This function deletes a semaphore.
 *
 *  @arg    semHandle
 *              Pointer to the semaphore object to be deleted.
 *
 *  @modif  None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_DeleteSem (IN PVOID semHandle)
{
    DSP_STATUS           status = DSP_SOK ;
    NOTIFYXFER_SemObject * semObj = semHandle ;
    int                  osStatus ;

    osStatus = sem_destroy (&(semObj->sem)) ;
    if (osStatus < 0) {
        status = DSP_EFAIL ;
    }

    free (semObj) ;

    return status ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_WaitSem
 *
 *  @desc   This function waits on a semaphore.
 *
 *  @modif  None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_WaitSem (IN PVOID semHandle)
{
    DSP_STATUS           status = DSP_SOK ;
    NOTIFYXFER_SemObject * semObj = semHandle ;
    int                  osStatus ;
	printf("NOTIFYXFER_WaitSem\n");
    osStatus = sem_wait (&(semObj->sem)) ;
    if (osStatus < 0) {
        status = DSP_EFAIL ;
    }

    return status ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_PostSem
 *
 *  @desc   This function posts a semaphore.
 *
 *  @modif  None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_PostSem (IN PVOID semHandle)
{
    DSP_STATUS           status = DSP_SOK ;
    NOTIFYXFER_SemObject * semObj = semHandle ;
    int                  osStatus ;

    osStatus = sem_post (&(semObj->sem)) ;
    if (osStatus < 0) {
        status = DSP_EFAIL ;
    }

    return status ;
}


/** ============================================================================
 *  @func   NOTIFYXFER_OS_init
 *
 *  @desc   This function initializes the OS specific component.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              General failure.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_OS_init (Void) ;


/** ============================================================================
 *  @func   NOTIFYXFER_OS_exit
 *
 *  @desc   This function finalizes the OS specific component.
 *
 *  @arg    None
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              General failure.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
NOTIFYXFER_OS_exit (Void) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
