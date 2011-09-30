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
 *  @file   _safe.h
 *
 *  @desc   Contains safe programming macros
 *
 *  ============================================================================
 */


#if !defined (_SAFE_H)
#define _SAFE_H


/*  ----------------------------------- IPC headers */
#include <ipc.h>

/*  ----------------------------------- OSAL headers */
#include <mem.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @macro  IS_OBJECT_VALID
 *
 *  @desc   Checks validity of object by comparing against it's signature.
 *  ============================================================================
 */
#define IS_OBJECT_VALID(obj, sign)                                  \
    (((obj != NULL) && ((obj)->signature == sign)) ? TRUE : FALSE)


/** ============================================================================
 *  @macro  IS_RANGE_VALID
 *
 *  @desc   Checks if a value lies in given range.
 *  ============================================================================
 */
#define IS_RANGE_VALID(x,min,max) (((x) < (max)) && ((x) >= (min)))

/** ============================================================================
 *  @macro  MIN
 *
 *  @desc   Returns minumum of the two arguments
 *  ============================================================================
 */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


/** ============================================================================
 *  @macro  FREE_PTR
 *
 *  @desc   Frees memory pointed to by ptr and sets it to NULL. Also returns
 *          status of MEM_Free function call
 *  ============================================================================
 */
#define FREE_PTR(ptr)  MEM_Free ((Pvoid *) ((Pvoid) &ptr), MEM_DEFAULT)


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (_SAFE_H) */
