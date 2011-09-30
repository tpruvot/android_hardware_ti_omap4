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
 *  @file   CrashInfo.h
 *
 *  @brief  Crash Dump utility structures file
 *  ============================================================================
 */

#include <stdlib.h>

/* ThreadType */
typedef enum {
    BIOS_ThreadType_Hwi,
    BIOS_ThreadType_Swi,
    BIOS_ThreadType_Task,
    BIOS_ThreadType_Main
} BIOS_ThreadType;

/*!
 *  Exception Context - Register contents at the time of an exception.
 */
typedef struct {
    /* Thread Context */
    BIOS_ThreadType threadType;     /* Type of thread executing at */
                                    /* the time the exception occurred */
    Ptr             threadHandle;   /* Handle to thread executing at */
                                    /* the time the exception occurred */
    Ptr             threadStack;    /* Address of stack contents of thread */
                                    /* executing at the time the exception */
                                    /* occurred */
    size_t          threadStackSize;/* size of thread stack */

    /* Internal Registers */
    Ptr     r0;
    Ptr     r1;
    Ptr     r2;
    Ptr     r3;
    Ptr     r4;
    Ptr     r5;
    Ptr     r6;
    Ptr     r7;
    Ptr     r8;
    Ptr     r9;
    Ptr     r10;
    Ptr     r11;
    Ptr     r12;
    Ptr     sp;
    Ptr     lr;
    Ptr     pc;
    Ptr     psr;

    /* NVIC registers */
    Ptr     ICSR;
    Ptr     MMFSR;
    Ptr     BFSR;
    Ptr     UFSR;
    Ptr     HFSR;
    Ptr     DFSR;
    Ptr     MMAR;
    Ptr     BFAR;
    Ptr     AFSR;
} ExcContext;
