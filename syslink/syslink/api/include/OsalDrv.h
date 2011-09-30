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
 *  @file   OsalDrv.h
 *
 *  @brief      Declarations of OS-specific functionality for Osal
 *
 *              This file contains declarations of OS-specific functions for
 *              Osal.
 *
 *  ============================================================================
 */


#ifndef OsalDrv_H_0xf2ba
#define OsalDrv_H_0xf2ba


#if defined (__cplusplus)
extern "C" {
#endif


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @def    OSALDRV_MODULEID
 *  @brief  Module ID for Memory OSAL module.
 */
#define OSALDRV_MODULEID                 (UInt16) 0x97D3

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
 * @def   OSALDRV_STATUSCODEBASE
 * @brief Stauts code base for MEMORY module.
 */
#define OSALDRV_STATUSCODEBASE            (OSALDRV_MODULEID << 12u)

/*!
 * @def   OSALDRV_MAKE_FAILURE
 * @brief Convert to failure code.
 */
#define OSALDRV_MAKE_FAILURE(x)          ((Int) (0x80000000  \
                                           + (OSALDRV_STATUSCODEBASE + (x))))
/*!
 * @def   OSALDRV_MAKE_SUCCESS
 * @brief Convert to success code.
 */
#define OSALDRV_MAKE_SUCCESS(x)            (OSALDRV_STATUSCODEBASE + (x))

/*!
 * @def   OSALDRV_E_MAP
 * @brief Failed to map to host address space.
 */
#define OSALDRV_E_MAP                      OSALDRV_MAKE_FAILURE(0)

/*!
 * @def   OSALDRV_E_UNMAP
 * @brief Failed to unmap from host address space.
 */
#define OSALDRV_E_UNMAP                    OSALDRV_MAKE_FAILURE(1)

/*!
 * @def   OSALDRV_E_OSFAILURE
 * @brief Failure in OS calls.
 */
#define OSALDRV_E_OSFAILURE                OSALDRV_MAKE_FAILURE(2)

/*!
 * @def   OSALDRV_SUCCESS
 * @brief Operation successfully completed
 */
#define OSALDRV_SUCCESS                    OSALDRV_MAKE_SUCCESS(0)


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Function to open the ProcMgr driver. */
Int OsalDrv_open (Void);

/* Function to close the ProcMgr driver. */
Int OsalDrv_close (Void);

/* Function to map a memory region specific to the driver. */
UInt32 OsalDrv_map (UInt32 addr, UInt32 size);

/* Function to unmap a memory region specific to the driver. */
Void OsalDrv_unmap (UInt32 addr, UInt32 size);

/* Function to invoke the APIs through ioctl. */
Int OsalDrv_ioctl (UInt32 cmd, Ptr args);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* OsalDrv_H_0xf2ba */
