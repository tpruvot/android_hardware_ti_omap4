/*
 *  types.h
 *
 *  Type definitions for the Memory Interface for TI OMAP processors.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
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

#ifndef _MEM_TYPES_H_
#define _MEM_TYPES_H_

/* for bool definition */
#include <stdbool.h>
#include <stdint.h>

/** ---------------------------------------------------------------------------
 * Type definitions
 */

/**
 * Buffer length in bytes
 */
typedef uint32_t bytes_t;

/**
 * Length in pixels
 */
typedef uint16_t pixels_t;


/**
 * Pixel format
 *
 * Page mode is encoded in the pixel format to handle different
 * set of buffers uniformly
 */
enum pixel_fmt_t {
    PIXEL_FMT_MIN   = 0,
    PIXEL_FMT_8BIT  = 0,
    PIXEL_FMT_16BIT = 1,
    PIXEL_FMT_32BIT = 2,
    PIXEL_FMT_PAGE  = 3,
    PIXEL_FMT_MAX   = 3
};

typedef enum pixel_fmt_t pixel_fmt_t;

/**
 * Ducati Space Virtual Address Pointer
 *
 * This is handled as a unsigned long so that no dereferencing
 * is allowed by user space components.
 */
typedef uint32_t DSPtr;

/**
 * System Space Pointer
 *
 * This is handled as a unsigned long so that no dereferencing
 * is allowed by user space components.
 */
typedef uint32_t SSPtr;

/**
 * Error values
 *
 * Page mode is encoded in the pixel format to handle different
 * set of buffers uniformly
 */
#define MEMMGR_ERR_NONE    0
#define MEMMGR_ERR_GENERIC 1

#endif

