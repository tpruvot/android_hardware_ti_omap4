/*
 *  utils.h
 *
 *  Utility definitions for the Memory Interface for TI OMAP processors.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
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

#ifndef _UTILS_H_
#define _UTILS_H_

/* ---------- Generic Macros Used in Macros ---------- */

/* statement begin */
#define S_ do
/* statement end */
#define _S while (0)
/* expression begin */
#define E_ (
/* expression end */
#define _E )

/* allocation macro */
#define NEW(type)    (type*)calloc(1, sizeof(type))
#define NEWN(type,n) (type*)calloc(n, sizeof(type))
#define ALLOC(var)    var = calloc(1, sizeof(*var))
#define ALLOCN(var,n) var = calloc(n, sizeof(*var))


/* free variable and set it to NULL */
#define FREE(var)    S_ { free(var); var = NULL; } _S

/* clear variable */
#define ZERO(var)    memset(&(var), 0, sizeof(var))

/* binary round methods */
#define ROUND_DOWN_TO2POW(x, N) ((x) & ~((N)-1))
#define ROUND_UP_TO2POW(x, N) ROUND_DOWN_TO2POW((x) + (N) - 1, N)

/* regulare round methods */
#define ROUND_DOWN_TO(x, N) ((x) / (N) * (N))
#define ROUND_UP_TO(x, N) ROUND_DOWN_TO((x) + (N) - 1, N)

#endif
