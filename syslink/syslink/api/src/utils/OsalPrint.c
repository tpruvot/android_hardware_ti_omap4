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
 *  @file   OsalPrint.c
 *
 *  @brief      Linux user Semaphore interface implementations.
 *
 *              This will have the definitions for user side printf.
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL and kernel utils */
#undef SYSLINK_DIRECT_LOGD
#include <OsalPrint.h>

/* Linux specifc header files */
#include <stdarg.h>
#include <stdio.h>


#ifdef HAVE_ANDROID_OS
#include <utils/Log.h>
#undef LOG_TAG
#define LOG_TAG "SYSLINK"
#endif

#if defined (__cplusplus)
extern "C" {
#endif


#if defined (HAVE_ANDROID_OS)
Bool printToLogCat = FALSE;

/*!
 *  @brief      Configure printf for Android systems.
 *
 *  @param      print config option.
 */
Void Osal_configPrint (Osal_PrintConfig printType)
{
    switch (printType) {
    case 1:
        printToLogCat = TRUE;
        break;
    case 0:
    default:
        printToLogCat = FALSE;
        break;
   }
}
#endif


/*!
 *  @brief      printf abstraction at the kernel level.
 *
 *  @param      string which needs to be printed.
 *  @param      Variable number of parameters for printf call
 */
Void Osal_printf (Char* format, ...)
{
    va_list args;
    Char    buffer [512];

    va_start (args, format);
    vsnprintf (buffer, sizeof(buffer), format, args);
    va_end   (args);

#if defined (HAVE_ANDROID_OS)
    if (printToLogCat == TRUE) {
        LOGD ("%s", buffer);
    } else {
        printf ("%s", buffer);
    }
#else
    printf ("%s", buffer);
#endif
}

/*!
 *  @brief      printf in Log Cat buffer abstraction at the kernel level.
 *
 *  @param      string which needs to be printed.
 *  @param      Variable number of parameters for printf call
 */
Void Osal_printfLOGD (char* format, ...)
{
    va_list args;
    char    buffer [512 ];

    va_start (args, format);
    vsprintf (buffer, format, args);
    va_end   (args);
    LOGD("%s", buffer);
}


#if defined (__cplusplus)
}
#endif /* defined (_cplusplus)*/
