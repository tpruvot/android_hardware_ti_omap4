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
 *  @file   mpcsxfer_os.h
 *
 *  @desc   OS specific definitions for the mpcsxfer sample.
 *
 *  ============================================================================
 */


#if !defined (NOTIFYXFER_OS_H)
#define NOTIFYXFER_OS_H


/*  ----------------------------------- DSP/BIOS Link                 */
#include <gpptypes.h>
#include <errbase.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @func   atoi
 *
 *  @desc   Extern declaration for function that converts a string into an
 *          integer value.
 *
 *  @arg    str
 *              String representation of the number.
 *
 *  @ret    <valid integer>
 *              If the 'initial' part of the string represents a valid integer
 *          0
 *              If the string passed does not represent an integer or is NULL.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
extern int atoi (const char * str) ;


/** ============================================================================
 *  @func   NOTIFYXFER_Atoi
 *
 *  @desc   This function converts a string into an integer value.
 *
 *  @arg    str
 *              String representation of the number.
 *
 *  @ret    <valid integer>
 *              If the 'initial' part of the string represents a valid integer
 *          0
 *              If the string passed does not represent an integer or is NULL.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */
#define NOTIFYXFER_Atoi atoi


/** ============================================================================
 *  @func   NOTIFYXFER_CreateSem
 *
 *  @desc   This function creates a semaphore.
 *
 *  @arg    semPtr
 *              Location to receive the semaphore object.
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
NOTIFYXFER_CreateSem (OUT PVOID * semPtr) ;


/** ============================================================================
 *  @func   NOTIFYXFER_DeleteSem
 *
 *  @desc   This function deletes a semaphore.
 *
 *  @arg    semHandle
 *              Pointer to the semaphore object to be deleted.
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
NOTIFYXFER_DeleteSem (IN PVOID semHandle) ;


/** ============================================================================
 *  @func   NOTIFYXFER_WaitSem
 *
 *  @desc   This function waits on a semaphore.
 *
 *  @arg    semHandle
 *              Pointer to the semaphore object to wait on.
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
NOTIFYXFER_WaitSem (IN PVOID semHandle) ;


/** ============================================================================
 *  @func   NOTIFYXFER_PostSem
 *
 *  @desc   This function posts a semaphore.
 *
 *  @arg    semHandle
 *              Pointer to the semaphore object to be posted.
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
NOTIFYXFER_PostSem (IN PVOID semHandle) ;


/** ============================================================================
 *  @func   NOTIFYXFER_0Print
 *
 *  @desc   Print a message without any arguments.
 *
 *  @modif  None
 *  ============================================================================
 */

void
NOTIFYXFER_0Print (Char8 * str) ;


/** ============================================================================
 *  @func   NOTIFYXFER_1Print
 *
 *  @desc   Print a message with one arguments.
 *
 *  @modif  None
 *  ============================================================================
 */

void
NOTIFYXFER_1Print (Char8 * str, Uint32 arg) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (NOTIFYXFER_OS_H) */
