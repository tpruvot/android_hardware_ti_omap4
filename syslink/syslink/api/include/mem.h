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
 *  @file   mem.h
 *
 *  @desc   Defines the interfaces and data structures for the
 *          sub-component MEM.
 *  ============================================================================
 */


#if !defined (MEM_H)
#define MEM_H


/*  ----------------------------------- IPC headers */
#include <ipc.h>
#include <_ipc.h>
#include <gpptypes.h>

/*  ----------------------------------- Trace & Debug */
#include <_trace.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  MEM_DEFAULT
 *
 *  @desc   Default attributes for OS independent operations for memory
 *          allocation & de-allocation.
 *          OS dependent attributes shall be defined in file 'mem_os.h'.
 *  ============================================================================
 */
#define MEM_DEFAULT    NULL


/** ============================================================================
 *  @name   MemMapInfo
 *
 *  @desc   Forward declaration for the OS specific structure containing memory
 *          mapping information.
 *  ============================================================================
 */
typedef struct MemMapInfo_tag MemMapInfo ;

/** ============================================================================
 *  @name   MemUnmapInfo
 *
 *  @desc   Forward declaration for the OS specific structure containing memory
 *          unmapping information.
 *  ============================================================================
 */
typedef struct MemUnmapInfo_tag MemUnmapInfo ;


/** ============================================================================
 *  @func   MEM_init
 *
 *  @desc   Initializes the MEM sub-component.
 *
 *  @arg    None.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Memory error occurred.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None.
 *  ============================================================================
 */

DSP_STATUS
MEM_init (void) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Initialize has been replaced
 *              with MEM_init.
 *  ============================================================================
 */
#define MEM_Initialize MEM_init


/** ============================================================================
 *  @func   MEM_exit
 *
 *  @desc   Releases all resources used by this sub-component.
 *
 *  @arg    None.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Memory error occurred.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None.
 *  ============================================================================
 */

DSP_STATUS
MEM_exit (void) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Finalize has been replaced
 *              with MEM_exit.
 *  ============================================================================
 */
#define MEM_Finalize MEM_exit


/** ============================================================================
 *  @func   MEM_alloc
 *
 *  @desc   Allocates the specified number of bytes.
 *
 *  @arg    ptr
 *              Location where pointer to allocated memory will be kept .
 *  @arg    cBytes
 *              Number of bytes to allocate.
 *  @arg    arg
 *              Type of memory to allocate. MEM_DEFAULT should be used if there
 *              is no need of special memory.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory error.
 *          DSP_EINVALIDARG
 *              Invalid argument.
 *
 *  @enter  MEM must be initialized.
 *          ptr must be a valid pointer.
 *          cBytes must be greater than 0.
 *
 *  @leave  *ptr must be a valid pointer upon successful completion otherwise
 *          it must be NULL.
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
MEM_alloc (OUT void ** ptr, IN Uint32 cBytes, IN OUT Pvoid arg) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Alloc has been replaced
 *              with MEM_alloc.
 *  ============================================================================
 */
#define MEM_Alloc MEM_alloc


/** ============================================================================
 *  @func   MEM_calloc
 *
 *  @desc   Allocates the specified number of bytes and clears them by filling
 *          it with 0s.
 *
 *  @arg    ptr
 *              Location where pointer to allocated memory will be kept
 *  @arg    cBytes
 *              Number of bytes to allocate.
 *  @arg    arg
 *              Type of memory to allocate. MEM_DEFAULT should be used if there
 *              is no need of special memory.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Out of memory error.
 *          DSP_EINVALIDARG
 *              Invalid argument.
 *
 *  @enter  MEM must be initialized.
 *          ptr must be a valid pointer.
 *          cBytes must be greater than 0.
 *
 *  @leave  *ptr must be a valid pointer upon successful completion otherwise
 *          it must be NULL.
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
MEM_calloc (OUT void ** ptr, IN Uint32 cBytes, IN OUT Pvoid arg) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Calloc has been replaced
 *              with MEM_calloc.
 *  ============================================================================
 */
#define MEM_Calloc MEM_calloc


/** ============================================================================
 *  @func   MEM_free
 *
 *  @desc   Frees up the allocated chunk of memory.
 *
 *  @arg    ptr
 *              Pointer to pointer to start of memory to be freed.
 *  @arg    arg
 *              Type of memory allocated.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EMEMORY
 *              Memory error.
 *          DSP_EINVALIDARG
 *              Invalid argument.
 *
 *  @enter  MEM must be initialized.
 *          memBuf must be a valid pointer.
 *
 *  @leave  None
 *
 *  @see
 *  ============================================================================
 */

DSP_STATUS
MEM_free (IN Pvoid * ptr, IN Pvoid arg) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Free has been replaced
 *              with MEM_free.
 *  ============================================================================
 */
#define MEM_Free MEM_free


/** ============================================================================
 *  @func   MEM_map
 *
 *  @desc   Maps a specified memory area into the GPP virtual space.
 *
 *  @arg    mapInfo
 *              Data required for creating the mapping.
 *
 *  @ret    DSP_SOK
 *              Operation completed successfully.
 *          DSP_EMEMORY
 *              Could not map the given memory address.
 *
 *  @enter  mapInfo pointer must be valid.
 *
 *  @leave  None
 *
 *  @see    MEM_Unmap
 *  ============================================================================
 */

DSP_STATUS
MEM_map (IN OUT MemMapInfo * mapInfo) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Map has been replaced
 *              with MEM_map.
 *  ============================================================================
 */
#define MEM_Map MEM_map


/** ============================================================================
 *  @func   MEM_unmap
 *
 *  @desc   Unmaps the specified memory area.
 *
 *  @arg    unmapInfo
 *              Information required for unmapping a memory area.
 *
 *  @ret    DSP_SOK
 *              Operation completed successfully.
 *
 *  @enter  unmapInfo pointer must be valid.
 *
 *  @leave  None.
 *
 *  @see    MEM_map
 *  ============================================================================
 */

DSP_STATUS
MEM_unmap (IN MemUnmapInfo * unmapInfo) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Unmap has been replaced
 *              with MEM_unmap.
 *  ============================================================================
 */
#define MEM_Unmap MEM_unmap


/** ============================================================================
 *  @func   MEM_Copy
 *
 *  @desc   Copies data between the specified memory areas.
 *
 *  @arg    dst
 *              Destination address
 *  @arg    src
 *              Source address
 *  @arg    len
 *              length of data to be coiped.
 *  @arg    endian
 *              Endianism
 *
 *  @ret    DSP_SOK
 *              Operation completed successfully.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None
 *  ============================================================================
 */

DSP_STATUS
MEM_Copy (IN void * dst, OUT Void * src, IN Uint32 len, IN Endianism endian) ;


/** ============================================================================
 *  @deprecated The deprecated function MEM_Copy has been replaced
 *              with MEM_copy.
 *  ============================================================================
 */
#define MEM_copy(dst,src,len) MEM_Copy(dst, src, len, Endianism_Default)


#if defined (DDSP_DEBUG)
/** ============================================================================
 *  @func   MEM_debug
 *
 *  @desc   Prints debug information for MEM.
 *
 *  @arg    None
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

void
MEM_debug (void) ;

/** ============================================================================
 *  @deprecated The deprecated function MEM_Debug has been replaced
 *              with MEM_debug.
 *  ============================================================================
 */
#define MEM_Debug MEM_debug
#endif /* defined (DDSP_DEBUG) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (MEM_H) */
