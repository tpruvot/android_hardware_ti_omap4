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

#if !defined(GTTRACE_H)
#define GTTRACE_H

#include <Std.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>



#define PRINT   printf


#define GT_config( config )
#define GT_configInit( config )
#define GT_seterror( fxn )

#define GT_ENTER        ((UInt8)0x01)
#define GT_1CLASS       ((UInt8)0x02)   /**< User mask 1. */
#define GT_2CLASS       ((UInt8)0x04)   /**< User mask 2. */
#define GT_3CLASS       ((UInt8)0x08)   /**< User mask 3. */
#define GT_4CLASS       ((UInt8)0x10)   /**< User mask 4. */
#define GT_5CLASS       ((UInt8)0x20)
#define GT_6CLASS       ((UInt8)0x40)
#define GT_7CLASS       ((UInt8)0x80)
#define GT_LEAVE        ((UInt8)0x82)

#define GT_0trace(mask, classId, format)
#define GT_1trace(mask, classId, format, arg1)
#define GT_2trace(mask, classId, format, arg1, arg2)
#define GT_3trace(mask, classId, format, arg1, arg2, arg3)
#define GT_4trace(mask, classId, format, arg1, arg2, arg3, arg4)
#define GT_5trace(mask, classId, format, arg1, arg2, arg3, arg4, arg5)
#define GT_6trace(mask, classId, format, arg1, arg2, arg3, arg4, arg5, arg6)

typedef struct GT_Mask_tag {
    String      modName;    /**< Name of this module instance. */
    UInt8      *flags;      /**< The current state of this instance */
} GT_Mask;


extern Int    errno;
extern Int    line;
extern String file;

#ifndef GT_setFailureReason
#define GT_setFailureReason(mask, classId, func, status, msg)      \
{                                                                              \
    PRINT ("%s: Error at Line no: %d in %s: 0x%x: %s\n", func, __LINE__, __FILE__, status, msg); \
    if (errno == 0){file =__FILE__; line=__LINE__;errno = status;}\
}
#endif

#define GT_assert(x, y) \
{ \
    if (!(y)) {\
        PRINT ("Assertion at Line no: %d in %s: %s : failed\n", __LINE__, __FILE__, #y); \
    }\
}

#define GT_print    PRINT
#define Print_printf printf

//#ifndef Memory_set
//#define Memory_set     memset
//#endif
#ifndef Memory_cpy
#define Memory_cpy     memcpy
#endif

#define SET_BIT(x,y)    ((x) |= 0x1 << (y))
#define RESET_BIT(x,y)    ((x) &= ~(0x1 << (y)))
#define TEST_BIT(x,y)    ((x) & (0x1 << (y)))

#define IO_ADDRESS(x)   x

#define PADDING(x)  (((128 + (x) - 1) / 128) * 128)

#define IPC_Get_statusCode(a, b) 0


#endif
