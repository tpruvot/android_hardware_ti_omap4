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
 *  @file   mem_os.h
 *
 *  @desc   Defines the OS dependent attributes & structures for the
 *          sub-component MEM.
 *  ============================================================================
 */


#if !defined (MEM_OS_H)
#define MEM_OS_H


/*  ----------------------------------- IPC Headers */
#include <ipc.h>
#include <_ipc.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  MEM_KERNEL
 *
 *  @desc   Example memory type. One has to handle it in MEM_Alloc if one wants
 *          to implement it.
 *  ============================================================================
 */
#define MEM_KERNEL      GFP_KERNEL


/** ============================================================================
 *  @type   MemAllocAttrs
 *
 *  @desc   OS dependent attributes for allocating memory.
 *
 *  @field  physicalAddress
 *              Physical address of the allocated memory.
 *  ============================================================================
 */
typedef struct MemAllocAttrs_tag {
    Uint32 *    physicalAddress ;
} MemAllocAttrs ;

/** ============================================================================
 *  @type   MemFreeAttrs
 *
 *  @desc   OS dependent attributes for freeing memory.
 *
 *  @field  physicalAddress
 *              Physical address of the memory to be freed.
 *  @field  size
 *              Size of the memory to be freed.
 *  ============================================================================
 */
typedef struct MemFreeAttrs_tag {
    Uint32 *    physicalAddress ;
    Uint32      size ;
} MemFreeAttrs ;


/** ============================================================================
 *  @type   MemMapInfo_tag
 *
 *  @desc   OS dependent definition of the information required for mapping a
 *          memory region.
 *
 *  @field  src
 *              Address to be mapped.
 *  @field  size
 *              Size of memory region to be mapped.
 *  @field  dst
 *              Mapped address.
 *  ============================================================================
 */
struct MemMapInfo_tag {
    Uint32   src  ;
    Uint32   size ;
    Uint32   dst  ;
} ;


/** ============================================================================
 *  @type   MemUnmapInfo_tag
 *
 *  @desc   OS dependent definition of the information required for unmapping
 *          a previously mapped memory region.
 *
 *  @field  addr
 *              Address to be unmapped. This is the address returned as 'dst'
 *              address from a previous call to MEM_Map () in the MemMapInfo
 *              structure.
 *  @field  size
 *              Size of memory region to be unmapped.
 *  ============================================================================
 */
struct MemUnmapInfo_tag {
    Uint32  addr ;
    Uint32  size ;
} ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (MEM_OS_H) */
