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
 *  @file   Bitops.h
 *
 *  @brief      Defines for bit operations macros.
 *  ============================================================================
 */


#ifndef UTILS_BITOPS_H_0X838E
#define UTILS_BITOPS_H_0X838E


#ifdef SYSLINK_BUILDOS_LINUX
#include <Atomic_Ops.h>
#endif

#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @brief  Sets bit at given position.
 *          This macro is independent of operand width. User must ensure
 *          correctness.
 */
#define SET_BIT(num,pos)            ((num) |= (1u << (pos)))

/*!
 *  @brief  Clears bit at given position.
 *          This macro is independent of operand width. User must ensure
 *          correctness.
 */
#define CLEAR_BIT(num,pos)          ((num) &= ~(1u << (pos)))

/*!
 *  @brief  Tests if bit at given position is set.
 *          This macro is independent of operand width. User must ensure
 *          correctness.
 */
#define TEST_BIT(num,pos)           ((((num) & (1u << (pos))) >> (pos)) & 0x01)

/*  ============================================================================
 *  @brief  This macro returns the minimum of two values.
 *  ============================================================================
 */
#define SYSLINK_min(a,b)            ((a)<(b)?(a):(b))

/*  ============================================================================
 *  @brief  This macro Round the value 'a' up by 'b', a power of two
 *  ============================================================================
 */
#define ROUND_UP(a, b) (SizeT)((((UInt32) a) + ((b) - 1)) & ~((b) - 1))


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* UTILS_BITOPS_H_0X838E */
