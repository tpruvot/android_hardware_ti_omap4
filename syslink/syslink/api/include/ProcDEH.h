/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2010, Texas Instruments Incorporated
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
/*============================================================================
 *  @file   ProcDEH.h
 *
 *  @brief  Declarations for Processor Device Error Handler module
 *
 *  ============================================================================
 */

#ifndef _PROCDEH_H_
#define _PROCDEH_H_

/* OSAL & Utils headers */
#include <Std.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @def    PROCDEH_MODULEID
 *  @brief  Module ID for ProcMgr.
 */
#define PROCDEH_MODULEID            (UInt16) 0xCA11

/* =============================================================================
 *  All success and failure codes for the module
 * =============================================================================
 */
/*!
 *  @def    PROCDEH_STATUSCODEBASE
 *  @brief  Error code base for ProcMgr.
 */
#define PROCDEH_STATUSCODEBASE      (PROCDEH_MODULEID << 12u)

/*!
 *  @def    PROCDEH_MAKE_FAILURE
 *  @brief  Macro to make failure code.
 */
#define PROCDEH_MAKE_FAILURE(x)     ((Int)(  0x80000000                        \
                                      | (PROCDEH_STATUSCODEBASE + (x))))

/*!
 *  @def    PROCDEH_MAKE_SUCCESS
 *  @brief  Macro to make success code.
 */
#define PROCDEH_MAKE_SUCCESS(x)     (PROCDEH_STATUSCODEBASE + (x))

/*!
 *  @def    ProcDEH_E_INVALIDARG
 *  @brief  Argument passed to a function is invalid.
 */
#define ProcDEH_E_INVALIDARG        PROCDEH_MAKE_FAILURE(1)

/*!
 *  @def    ProcDEH_E_MEMORY
 *  @brief  Memory allocation failed.
 */
#define ProcDEH_E_MEMORY            PROCDEH_MAKE_FAILURE(2)

/*!
 *  @def    ProcDEH_E_FAIL
 *  @brief  Generic failure.
 */
#define ProcDEH_E_FAIL              PROCDEH_MAKE_FAILURE(3)

/*!
 *  @def    ProcDEH_E_FAIL
 *  @brief  OS returned failure.
 */
#define ProcDEH_E_OSFAILURE         PROCDEH_MAKE_FAILURE(4)

/*!
 *  @def    PROCDEH_SUCCESS
 *  @brief  Operation successful.
 */
#define ProcDEH_S_SUCCESS           PROCDEH_MAKE_SUCCESS(0)

/*!
 *  @def    PROCDEH_S_ALREADYSETUP
 *  @brief  The ProcMgr module has already been setup in this process.
 */
#define ProcDEH_S_ALREADYSETUP      PROCDEH_MAKE_SUCCESS(1)

/*!
 *  @def    PROCDEH_S_OPENHANDLE
 *  @brief  Other ProcMgr clients have still setup the ProcMgr module.
 */
#define ProcDEH_S_SETUP             PROCDEH_MAKE_SUCCESS(2)

/*!
 *  @def    PROCDEH_S_OPENHANDLE
 *  @brief  Other ProcMgr handles are still open in this process.
 */
#define ProcDEH_S_OPENHANDLE        PROCDEH_MAKE_SUCCESS(3)


/* =============================================================================
 * Structures & Macros
 * =============================================================================
 */
/*!
 *  @brief  Command arguments for ProcDEH_registerEvent
 */
typedef struct ProcDEH_CmdArgsRegisterEvent_tag {
    Int32   fd;
    /*!< Eventfd descriptor id for notification */
    Int32   eventType;
    /*!< Type of event to be registered */
} ProcDEH_CmdArgsRegisterEvent;

/*!
 *  @brief  Device error types maintained by Device exception handler
 */
typedef enum ProcDEH_EventType_t {
    ProcDEH_SYSERROR = 1,
    ProcDEH_WATCHDOGERROR
} ProcDEH_EventType;


/*!
 *  @brief  DEVH driver IOCTL Magic number
 */
#define DEVH_IOC_MAGIC              'E'

/*!
 *  @brief  Command for ProcDEH event notification
 */
#define CMD_DEVH_IOCWAITONEVENTS    _IO(DEVH_IOC_MAGIC, 0)

/*!
 *  @brief  Command for ProcDEH event registration
 */
#define CMD_DEVH_IOCEVENTREG        _IOW(DEVH_IOC_MAGIC, 1,                    \
                                    ProcDEH_CmdArgsRegisterEvent)

/*!
 *  @brief  Command for ProcDEH event unregistration
 */
#define CMD_DEVH_IOCEVENTUNREG      _IOW(DEVH_IOC_MAGIC, 2,                    \
                                    ProcDEH_CmdArgsRegisterEvent)


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief  Function to close the ProcDEH driver
 *
 *  @param  procId   MultiProc id whose DEH driver needs to be opened.
 *
 *  @return ProcDEH success or failure status
 *
 *  @sa     ProcDEH_open
 */
Int32 ProcDEH_close (UInt16 procId);


/*!
 *  @brief  Function to open the ProcDEH driver
 *
 *  @param  procId   MultiProc id whose DEH driver needs to be closed.
 *
 *  @return ProcDEH success or failure status
 *
 *  @sa     ProcDEH_close
 */
Int32 ProcDEH_open (UInt16 procId);


/*!
 *  @brief  Function to Register for DEH faults
 *
 *  @param  procId      MultiProc id whose DEH driver needs to be closed
 *  @param  eventType   ProcDEH Event type
 *  @param  eventfd     Eventfd descriptor to be signalled
 *  @param  reg         Register or Unregister flag
 *
 *  @return ProcDEH success or failure status
 *
 *  @sa
 */
Int32 ProcDEH_registerEvent (UInt16     procId,
                             UInt32     eventType,
                             Int32      eventfd,
                             Bool       reg);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* _PROCDEH_H_*/
