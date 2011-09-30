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
 *  @file   _Notify.h
 *
 *  @brief      HLOS-specific Notify header
 *
 *              The Notify component provides APIs to the user for sending and
 *              receiving events including short fixed-size event data between
 *              the processors. It abstracts the physical interrupts and
 *              provides multiple prioritized events over a single interrupt.
 *              The Notify component is responsible for notifying an event to
 *              its peer on the remote processor. This component shall use the
 *              services provided on the hardware platform. It provides, which
 *              are used by upper layers to establish communication amongst
 *              peers at that level.
 *
 *  ============================================================================
 */


#if !defined (_Notify_H_0x5f84)
#define _Notify_H_0x5f84


/* Module headers */
#include <ti/ipc/Notify.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/* =============================================================================
 *  Macros and types
 * =============================================================================
 */

/*!
 *  @def    Notify_MODULEID
 *  @brief  Module ID for Notify.
 */
#define Notify_MODULEID           (UInt16) 0x5f84

/*!
 *  @def    Notify_EVENT_MASK
 *  @brief  Mask to check for event ID.
 */
#define Notify_EVENT_MASK      (UInt16) 0xFFFF

/*!
 *  @def    ISRESERVED
 *  @brief  Macro to check if event is a reserved Event.
 */
#define ISRESERVED(eventId, reservedEvent) ( ( (eventId & Notify_EVENT_MASK) \
                                            >= reservedEvent) \
                             || (eventId >> sizeof(UInt16) == Notify_SYSTEMKEY))


/*!
 *  @brief   Module configuration structure.
 *           This structure defines attributes for initialization of the Notify
 *           module.
 */
typedef struct Notify_Config_tag {
    UInt32   numEvents;
    /*!< Number of events to be supported */
    UInt32   sendEventPollCount;
    /*!< Poll for specified amount before sendEvent times out */
    UInt32   numLines;
    /*!< Maximum number of interrupt lines between a single pair of processors */
    UInt32   reservedEvents;
    /*!< Number of reserved events to be supported */
} Notify_Config ;

/*!
 *  @brief   This structure defines the configuration structure for
 *           initialization of the Notify object.
 */
typedef struct Notify_Params_tag {
    UInt32    reserved;
    /*!< Reserved field */
} Notify_Params;

/* Forward declaration of Notify_Object */
typedef struct Notify_Object_tag Notify_Object;

/*!
 *  @brief  Defines the type for the handle to the Notify driver.
 */
typedef Notify_Object * Notify_Handle;


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/* Function to get the default configuration for the Notify module. */
Void Notify_getConfig (Notify_Config * cfg);

/* Function to setup the Notify Module */
Int Notify_setup (Notify_Config * cfg);

/* Function to destroy the Notify module */
Int Notify_destroy (Void);

/* Function to create an instance of Notify driver */
Notify_Handle Notify_create (      Ptr                 driverHandle,
                                   UInt16              remoteProcId,
                                   UInt16              lineId,
                             const Notify_Params *     params);

/* Function to delete an instance of Notify driver */
Int Notify_delete (Notify_Handle * handlePtr);

/* Function to call device specific the Notify module setup */
Int Notify_attach (UInt16 procId, Ptr sharedAddr);

/* Function to destroy the device specific Notify module */
Int Notify_detach (UInt16 procId);

/* Function registered as callback with the Notify driver */
Void Notify_exec (Notify_Object * obj, UInt32 eventId, UInt32 payload);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (_Notify_H_0x5f84) */
