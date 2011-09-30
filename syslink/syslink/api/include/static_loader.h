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
 *  @file   static_loader.h
 *
 *  @desc   Defines the Static Loader interface.
 *          This loader is used when the DSP executable is statically linked
 *          into the ARM-side executable. This is possible when the ARM and DSP
 *          memory space is unified, as in the Davinci platform.
 *          In this loader, no separate load of the DSP executable is required,
 *          since it gets loaded automatically when the ARM executable is
 *          loaded.
 *  ============================================================================
 */


#if !defined (STATIC_LOADER_H)
#define STATIC_LOADER_H


/*  ----------------------------------- DSP/BIOS Link               */
#include <dsplink.h>
#include <_loaderdefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @func   STATICLOADER_init
 *
 *  @desc   Initializes the context object for loading a base image file or
 *          a section. This function is required to be called before any other
 *          function is called from this sub component.
 *
 *  @arg    procId
 *              Processor Identifier for the target DSP.
 *  @arg    baseImage
 *              Identifier for the file.
 *  @arg    args
 *              Arguments for initialization of the loader.
 *  @arg    objCtx
 *              Location to receive the context object for the loader.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFILE
 *              File not found.
 *          DSP_EMEMORY
 *              Out of memory error.
 *
 *  @enter  procId must be valid.
 *          baseImage must not be NULL.
 *          contextObj must not be NULL.
 *
 *  @leave  None
 *
 *  @see    STATICLOADER_exit ()
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
STATICLOADER_init (IN  ProcessorId      procId,
                   IN  Pstr             baseImage,
                   IN  LoaderInitArgs * args,
                   OUT Pvoid *          objCtx) ;


/** ============================================================================
 *  @func   STATICLOADER_exit
 *
 *  @desc   Deallocates the object(s) created by STATICLOADER_init function
 *          and releases the context.
 *
 *  @arg    objCtx
 *              The context object obtained through STATICLOADER_init.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFILE
 *              File is not open.
 *          DSP_EPOINTER
 *              Invalid file object.
 *
 *  @enter  objCtx must be valid.
 *
 *  @leave  None
 *
 *  @see    STATICLOADER_init ()
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
STATICLOADER_exit (IN  Pvoid objCtx) ;


/*  ============================================================================
 *  @func   STATICLOADER_load
 *
 *  @desc   Loads the DSP executable that has been statically linked into the
 *          ARM executable. This is called by PMGR_PROC_load through the
 *          function pointer table.
 *
 *  @arg    procId
 *              Target DSP identifier where the base image is to be loaded.
 *  @arg    loaderObj
 *              This object is used to receive arguments from PMGR.
 *  @arg    argc
 *              Number of arguments to be passed to the base image upon start.
 *  @arg    argv
 *              Arguments to be passed to DSP main application.
 *  @arg    entryPt
 *              OUT argument for returning entry address for the executable.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EINVALIDARG
 *              Invalid ProcId argument.
 *          DSP_EMEMORY
 *              Memory Error.
 *          DSP_EFILE
 *              Invalid base image.
 *          DSP_EFAIL
 *              General Failure, unable to load image on DSP
 *
 *  @enter  procId must be valid.
 *          baseImage must be a valid file identifier.
 *          entryAddress must be a valid pointer.
 *          If argc is 0 then argv must be NULL pointer.
 *          If argc is non-zero then argv must be a valid pointer.
 *
 *  @leave  None
 *
 *  @see    PMGR_PROC_load
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
STATICLOADER_load (IN  ProcessorId     procId,
                   IN  LoaderObject *  loaderObj,
                   IN  Uint32          argc,
                   IN  Char8 **        argv,
                   OUT Uint32 *        entryPt) ;


/*  ============================================================================
 *  @func   STATICLOADER_loadSection
 *
 *  @desc   Loads a section from the DSP executable onto the DSP. PMGR_PROC_load
 *          calls this through the function pointer table.
 *
 *  @arg    procId
 *              DSP identifier.
 *  @arg    loaderObj
 *              This object is used to receive arguments from PMGR.
 *  @arg    sectId
 *              Identifier for section to load.
 *
 *  @ret    DSP_ENOTIMPL
 *              Function not implemented.
 *
 *  @enter  procId must be valid.
 *          baseImage must be a valid file identifier.
 *          sectId must be a valid section identifier.
 *
 *  @leave  None
 *
 *  @see    PMGR_PROC_load
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
STATICLOADER_loadSection (IN  ProcessorId     procId,
                          IN  LoaderObject *  loaderObj,
                          IN  Uint32          sectId) ;


/** ============================================================================
 *  @func   STATICLOADER_getSymbolAddress
 *
 *  @desc   Gets the DSP address corresponding to a symbol within a DSP
 *          executable currently loaded on the DSP.
 *
 *  @arg    procId
 *              DSP identifier.
 *  @arg    objCtx
 *              Context of the loader object.
 *  @arg    symbolName
 *              Name of the symbol to be looked up.
 *  @arg    dspAddr
 *              Location to receive the DSP address for the specified symbol.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EINVALIDARG
 *              Invalid argument.
 *          DSP_ENOTSUPPORTED
 *              Symbol lookup feature is not supported by the loader.
 *          DSP_ENOTFOUND
 *              Symbol not found.
 *          DSP_EFAIL
 *              General failure.
 *
 *  @enter  procId must be valid.
 *          objCtx must be valid pointer.
 *          symbolName must be valid pointer.
 *          dspAddr must be valid pointer.
 *
 *  @leave  On success, dspAddr contains a valid DSP address.
 *          On failure, dspAddr is NULL.
 *
 *  @see    None.
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
STATICLOADER_getSymbolAddress (IN   ProcessorId     procId,
                               IN   Void *          objCtx,
                               IN   Char8 *         symbolName,
                               OUT  Uint32 *        dspAddr) ;


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (STATIC_LOADER_H) */
