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
/*==============================================================================
 *  @file   Notify.c
 *
 *  @brief      User side Notify Manager
 *
 *  ============================================================================
 */


/* Standard headers*/
#include <Std.h>

/* TBD: this should be removed as getpid should made as osal */
#include <unistd.h>

/* Osal headers*/
#include <Trace.h>
#include <MemoryDefs.h>
#include <Memory.h>

/* Notify Headers */
#include <ti/ipc/Notify.h>
#include <_Notify.h>
#include <_NotifyDefs.h>
#include <NotifyDrvDefs.h>
#include <NotifyDrvUsr.h>
#include <ti/ipc/MultiProc.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/* =============================================================================
 *  Macros and types
 * =============================================================================
 */
/*!
 *  @brief  Notify Module state object
 */
typedef struct Notify_ModuleObject_tag {
    UInt32             setupRefCount;
    /*!< Reference count for number of times setup/destroy were called in this
         process. */
    Notify_Config      cfg;
    /*!< Notify configuration structure */
} Notify_ModuleObject;


/* =============================================================================
 *  Globals
 * =============================================================================
 */
/*!
 *  @var    Notify_state
 *
 *  @brief  Notify state object variable
 */
#if !defined(SYSLINK_BUILD_DEBUG)
static
#endif /* if !defined(SYSLINK_BUILD_DEBUG) */
Notify_ModuleObject Notify_state =
{
    .setupRefCount = 0
};


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Get the default configuration for the Notify module.
 *
 *              This function can be called by the application to get their
 *              configuration parameter to Notify_setup filled in by the
 *              Notify module with the default parameters. If the user
 *              does not wish to make any change in the default parameters, this
 *              API is not required to be called.
 *
 *  @param      cfg        Pointer to the Notify module configuration
 *                         structure in which the default config is to be
 *                         returned.
 *
 *  @sa         Notify_setup, NotifyDrvUsr_open, NotifyDrvUsr_ioctl,
 *              NotifyDrvUsr_close
 */
Void
Notify_getConfig (Notify_Config * cfg)
{
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    Int                         status = Notify_S_SUCCESS;
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    Notify_CmdArgsGetConfig    cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "Notify_getConfig", cfg);

    GT_assert (curTrace, (cfg != NULL));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (cfg == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_getConfig",
                             Notify_E_INVALIDARG,
                             "Argument of type (Notify_Config *) passed "
                             "is null!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        /* Temporarily open the handle to get the configuration. */
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        NotifyDrvUsr_open (FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_getConfig",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.cfg = cfg;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            status =
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            NotifyDrvUsr_ioctl (CMD_NOTIFY_GETCONFIG, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "Notify_getConfig",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        Memory_copy (&Notify_state.cfg,
                     cfg,
                     sizeof (Notify_Config));

        /* Close the driver handle. */
        NotifyDrvUsr_close (FALSE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "Notify_getConfig");
}


/*!
 *  @brief      Setup the Notify module.
 *
 *              This function sets up the Notify module. This function
 *              must be called before any other instance-level APIs can be
 *              invoked.
 *              Module-level configuration needs to be provided to this
 *              function. If the user wishes to change some specific config
 *              parameters, then Notify_getConfig can be called to get the
 *              configuration filled with the default values. After this, only
 *              the required configuration values can be changed. If the user
 *              does not wish to make any change in the default parameters, the
 *              application can simply call Notify_setup with NULL
 *              parameters. The default parameters would get automatically used.
 *
 *  @param      cfg   Optional Notify module configuration. If provided as
 *                    NULL, default configuration is used.
 *
 *  @sa         Notify_destroy, NotifyDrvUsr_open, NotifyDrvUsr_ioctl
 */
Int
Notify_setup (Notify_Config * cfg)
{
    Int                 status = Notify_S_SUCCESS;
    Notify_CmdArgsSetup cmdArgs;

    GT_1trace (curTrace, GT_ENTER, "Notify_setup", cfg);

    /* TBD: Protect from multiple threads. */
    Notify_state.setupRefCount++;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (Notify_state.setupRefCount > 1) {
        /*! @retval Notify_S_ALREADYSETUP Success: Notify module has been
                                           already setup in this process */
        status = Notify_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "    Notify_setup: Notify module has been already setup "
                   "in this process.\n"
                   "        RefCount: [%d]\n",
                   (Notify_state.setupRefCount - 1));
    }
    else {
        /* Open the driver handle. */
        status = NotifyDrvUsr_open (TRUE);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_setup",
                                 status,
                                 "Failed to open driver handle!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            cmdArgs.cfg = cfg;
            status = NotifyDrvUsr_ioctl (CMD_NOTIFY_SETUP, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
            if (status < 0) {
                GT_setFailureReason (curTrace,
                                  GT_4CLASS,
                                  "Notify_setup",
                                  status,
                                  "API (through IOCTL) failed on kernel-side!");
            }
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    }

    GT_1trace (curTrace, GT_LEAVE, "Notify_setup", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Destroy the Notify module.
 *
 *              Once this function is called, other Notify module APIs,
 *              except for the Notify_getConfig API cannot be called
 *              anymore.
 *
 *  @sa         Notify_setup, NotifyDrvUsr_ioctl, NotifyDrvUsr_close
 */
Int
Notify_destroy (Void)
{
    Int                    status = Notify_S_SUCCESS;
    Notify_CmdArgsDestroy  cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "Notify_destroy");

    /* TBD: Protect from multiple threads. */
    Notify_state.setupRefCount--;
    /* This is needed at runtime so should not be in SYSLINK_BUILD_OPTIMIZE. */
    if (Notify_state.setupRefCount >= 1) {
        /*! @retval Notify_S_ALREADYSETUP Success: Notify module has been setup
                                             by other clients in this process */
        status = Notify_S_ALREADYSETUP;
        GT_1trace (curTrace,
                   GT_1CLASS,
                   "Notify module has been setup by other clients in this"
                   " process.\n"
                   "    RefCount: [%d]\n",
                   (Notify_state.setupRefCount + 1));
    }
    else {
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_DESTROY, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_destroy",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

        /* Close the driver handle. */
        NotifyDrvUsr_close (TRUE);
    }

    GT_1trace (curTrace, GT_LEAVE, "Notify_destroy", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Creates notify drivers and registers them with Notify
 *
 *  @param      procId       Processor Id
 *  @param      sharedAddr   Shared address
 *
 *  @sa         Notify_setup, NotifyDrvUsr_ioctl, NotifyDrvUsr_close
 */
Int
Notify_attach (UInt16 procId, Ptr sharedAddr)
{
    Int                    status = Notify_S_SUCCESS;
    Notify_CmdArgsAttach   cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "Notify_attach", procId, sharedAddr);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors()));
    GT_assert (curTrace, (sharedAddr != NULL));

    cmdArgs.procId     = procId;
    /* using v2p mappping knl shoul map p2v again */
    cmdArgs.sharedAddr = Memory_translate (sharedAddr,
                                           Memory_XltFlags_Virt2Phys);
    status = NotifyDrvUsr_ioctl (CMD_NOTIFY_ATTACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_attach",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    GT_1trace (curTrace, GT_LEAVE, "Notify_attach", status);
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Destroy the Notify module.
 *
 *  @param      procId       Processor Id
 *  @param      sharedAddr   Shared address
 *
 *  @sa         Notify_setup, NotifyDrvUsr_ioctl, NotifyDrvUsr_close
 */
Int
Notify_detach (UInt16 procId)
{
    Int                    status = Notify_S_SUCCESS;
    Notify_CmdArgsDetach   cmdArgs;

    GT_0trace (curTrace, GT_ENTER, "Notify_detach");

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));

    cmdArgs.procId = procId;
    status = NotifyDrvUsr_ioctl (CMD_NOTIFY_DETACH, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_detach",
                             status,
                             "API (through IOCTL) failed on kernel-side!");
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_detach", status);

    return (status);
}


/*!
 *  @brief      Returns the amount of shared memory used by one Notify instance.
 *
 *  @param      procId       Processor Id
 *  @param      sharedAddr   Shared address
 *
 */
SizeT
Notify_sharedMemReq(UInt16 procId,
                    Ptr    sharedAddr)
{
    Int                          status = Notify_S_SUCCESS;
    Notify_CmdArgsSharedMemReq   cmdArgs;
    SizeT                        size;

    GT_2trace (curTrace, GT_ENTER, "Notify_sharedMemReq", procId, sharedAddr);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors()));
    GT_assert (curTrace, (sharedAddr != NULL));

    cmdArgs.procId     = procId;
    /* using v2p mappping knl shoul map p2v again */
    cmdArgs.sharedAddr = Memory_translate (sharedAddr,
                                           Memory_XltFlags_Virt2Phys);
    status = NotifyDrvUsr_ioctl (CMD_NOTIFY_SHAREDMEMREQ, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (status < 0) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_sharedMemReq",
                             status,
                             "API (through IOCTL) failed on kernel-side!");
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
    size = cmdArgs.sharedMemSize;

    GT_1trace (curTrace, GT_LEAVE, "Notify_sharedMemReq", size);
    /*! @retval Notify_S_SUCCESS Operation successful */
    return (size);
}


/*!
 *  @brief     Whether notification via interrupt line has been registered.
 *
 *  @param      procId       Processor Id
 *  @param      lineId       Interrupt line Id
 *
 */
Bool
Notify_isRegistered(UInt16 procId, UInt16 lineId)
{
    Int                          status = Notify_S_SUCCESS;
    Notify_CmdArgsIsRegistered   cmdArgs;
    Bool                         isRegistered = FALSE;

    GT_2trace (curTrace, GT_ENTER, "Notify_isRegistered", procId, lineId);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_intLineRegistered",
                             Notify_E_INVALIDARG,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_intLineRegistered",
                             Notify_E_INVALIDARG,
                             "Invalid lineId argument provided");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId = procId;
        cmdArgs.lineId = lineId;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_ISREGISTERED, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_isRegistered",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            isRegistered = cmdArgs.isRegistered;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_isRegistered", isRegistered);

    return (isRegistered);
}


/*!
 *  @brief      This function registers a callback to a specific event number,
 *              processor id and interrupt line. When the event is received by
 *              the specified processor, the callback is called.
 *
 *  @param      procId       Processor Id
 *  @param      lineId       Interrupt line Id
 *  @param      eventId      Event number to be unregistered
 *  @param      fnNotifyCbck Callback function to be registered
 *  @param      cbckArg      Argument to callback function
 *
 *  @sa         Notify_unregisterEventSingle
 */
Int
Notify_registerEventSingle(UInt16              procId,
                           UInt16              lineId,
                           UInt32              eventId,
                           Notify_FnNotifyCbck fnNotifyCbck,
                           UArg                cbckArg)

{
    Int32                       status          = Notify_S_SUCCESS;
    UInt32                      strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsRegisterEvent cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "Notify_registerEventSingle",
               procId, lineId, eventId, fnNotifyCbck, cbckArg);

    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (fnNotifyCbck != NULL));
    /* cbckArg is optional and may be NULL. */
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*! @retval  Notify_E_INVALIDARG Invalid procId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEventSingle",
                             status,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*! @retval  Notify_E_INVALIDARG Invalid lineId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEventSingle",
                             status,
                             "Invalid lineId argument provided");
    }
    else if (fnNotifyCbck == NULL) {
        /*! @retval  Notify_E_INVALIDARG Invalid NULL fnNotifyCbck argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEventSingle",
                             status,
                             "Invalid NULL fnNotifyCbck provided.");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*! @retval  Notify_E_EVTNOTREGISTERED Invalid eventId specified. */
        status = Notify_E_EVTNOTREGISTERED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEventSingle",
                             status,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*! @retval  Notify_E_EVTRESERVED Invalid usage of reserved event
                                            number. */
        status = Notify_E_EVTRESERVED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEventSingle",
                             status,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId       = procId;
        cmdArgs.lineId       = lineId;
        cmdArgs.eventId      = eventId;
        cmdArgs.fnNotifyCbck = fnNotifyCbck;
        cmdArgs.cbckArg      = cbckArg;
        cmdArgs.pid          = getpid ();
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_REGISTEREVENTSINGLE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_registerEventSingle",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_registerEventSingle", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Register a callback for a specific event with the Notify module.
 *
 *  @param      handle       Handle to the Notify Driver
 *  @param      procId       Processor Id
 *  @param      eventId      Event number to be registered
 *  @param      fnNotifyCbck Callback function to be registered
 *  @param      cbckArg      Optional call back argument
 *
 *  @sa         Notify_unregisterEvent
 */
Int
Notify_registerEvent (UInt16              procId,
                      UInt16              lineId,
                      UInt32              eventId,
                      Notify_FnNotifyCbck fnNotifyCbck,
                      UArg                cbckArg)
{
    Int32                       status          = Notify_S_SUCCESS;
    UInt32                      strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsRegisterEvent cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "Notify_registerEvent",
               procId, lineId, eventId, fnNotifyCbck, cbckArg);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (fnNotifyCbck != NULL));
    /* cbckArg is optional and may be NULL. */
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*! @retval  Notify_E_INVALIDARG Invalid procId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEvent",
                             status,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*! @retval  Notify_E_INVALIDARG Invalid lineId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEvent",
                             status,
                             "Invalid lineId argument provided");
    }
    else if (fnNotifyCbck == NULL) {
        /*! @retval  Notify_E_INVALIDARG Invalid NULL fnNotifyCbck argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEvent",
                             status,
                             "Invalid NULL fnNotifyCbck provided.");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*! @retval  Notify_E_EVTNOTREGISTERED Invalid eventId specified. */
        status = Notify_E_EVTNOTREGISTERED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEvent",
                             status,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*! @retval  Notify_E_EVTRESERVED Invalid usage of reserved event
                                            number. */
        status = Notify_E_EVTRESERVED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_registerEvent",
                             status,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId = procId;
        cmdArgs.lineId       = lineId;
        cmdArgs.eventId      = eventId;
        cmdArgs.fnNotifyCbck = fnNotifyCbck;
        cmdArgs.cbckArg = cbckArg;
        cmdArgs.pid = getpid ();
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_REGISTEREVENT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_registerEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_registerEvent", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      This function removes a previously registered callback
 *              with the driverHandle. The procId, lineId, and eventId must
 *              exactly match the registered one.
 *
 *  @param      procId       Processor Id
 *  @param      lineId       Interrupt line Id
 *  @param      eventId      Event number to be unregistered
 *
 *  @sa         Notify_registerEventSingle
 */
Int
Notify_unregisterEventSingle(UInt16 procId,
                             UInt16 lineId,
                             UInt32 eventId)
{
    Int32                         status          = Notify_S_SUCCESS;
    UInt32                      strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsUnregisterEvent cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "Notify_unregisterEventSingle",
               procId, lineId, eventId);

    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    /* cbckArg is optional and may be NULL. */
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*! @retval  Notify_E_INVALIDARG Invalid procId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEventSingle",
                             status,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*! @retval  Notify_E_INVALIDARG Invalid lineId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEventSingle",
                             status,
                             "Invalid lineId argument provided");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*! @retval  Notify_E_EVTNOTREGISTERED Invalid eventId specified. */
        status = Notify_E_EVTNOTREGISTERED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEventSingle",
                             status,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*! @retval  Notify_E_EVTRESERVED Invalid usage of reserved event
                                            number. */
        status = Notify_E_EVTRESERVED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEventSingle",
                             status,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId  = procId;
        cmdArgs.lineId  = lineId;
        cmdArgs.eventId = eventId;
        cmdArgs.pid     = getpid();
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_UNREGISTEREVENTSINGLE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_unregisterEventSingle",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_unregisterEventSingle", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Un-register the callback for the specific event with the Notify
 *              module.
 *
 *  @param      handle       Handle to the Notify Driver
 *  @param      procId       Processor Id
 *  @param      eventId      Event number to be unregistered
 *  @param      fnNotifyCbck Callback function to be unregistered
 *  @param      cbckArg      Optional call back argument
 *
 *  @sa         Notify_registerEvent
 */
Int
Notify_unregisterEvent (UInt16              procId,
                        UInt16              lineId,
                        UInt32              eventId,
                        Notify_FnNotifyCbck fnNotifyCbck,
                        UArg                cbckArg)
{
    Int32                         status          = Notify_S_SUCCESS;
    UInt32                      strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsUnregisterEvent cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "Notify_unregisterEvent",
               procId, lineId, eventId, fnNotifyCbck, cbckArg);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (fnNotifyCbck != NULL));
    /* cbckArg is optional and may be NULL. */
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*! @retval  Notify_E_INVALIDARG Invalid procId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEvent",
                             status,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*! @retval  Notify_E_INVALIDARG Invalid lineId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEvent",
                             status,
                             "Invalid lineId argument provided");
    }
    else if (fnNotifyCbck == NULL) {
        /*! @retval  Notify_E_INVALIDARG Invalid NULL fnNotifyCbck argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEvent",
                             status,
                             "Invalid NULL fnNotifyCbck provided.");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*! @retval  Notify_E_EVTNOTREGISTERED Invalid eventId specified. */
        status = Notify_E_EVTNOTREGISTERED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEvent",
                             status,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*! @retval  Notify_E_EVTRESERVED Invalid usage of reserved event
                                            number. */
        status = Notify_E_EVTRESERVED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_unregisterEvent",
                             status,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId       = procId;
        cmdArgs.lineId       = lineId;
        cmdArgs.eventId      = eventId;
        cmdArgs.fnNotifyCbck = fnNotifyCbck;
        cmdArgs.cbckArg      = cbckArg;
        cmdArgs.pid          = getpid ();
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_UNREGISTEREVENT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_unregisterEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_unregisterEvent", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Send a notification to the specified event.
 *
 *  @param      handle      Handle to the Notify Driver
 *  @param      procId      Processor Id
 *  @param      eventId     Event number to be sent.
 *  @param      payload     Payload to be sent alongwith the event.
 *  @param      waitClear   Indicates whether Notify driver will wait for
 *                          previous event to be cleared. If payload needs to
 *                          be sent across, this must be TRUE.
 *  @sa
 */
Int
Notify_sendEvent (UInt16              procId,
                  UInt16              lineId,
                  UInt32              eventId,
                  UInt32              payload,
                  Bool                waitClear)
{

    Int32                   status          = Notify_S_SUCCESS;
    UInt32                  strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsSendEvent cmdArgs;

    GT_5trace (curTrace, GT_ENTER, "Notify_sendEvent",
               procId, lineId, eventId, payload, waitClear);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*! @retval  Notify_E_INVALIDARG Invalid procId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_sendEvent",
                             status,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*! @retval  Notify_E_INVALIDARG Invalid lineId argument
                                         provided. */
        status = Notify_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_sendEvent",
                             status,
                             "Invalid lineId argument provided");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*! @retval  Notify_E_EVTNOTREGISTERED Invalid eventId specified. */
        status = Notify_E_EVTNOTREGISTERED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_sendEvent",
                             status,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*! @retval  Notify_E_EVTRESERVED Invalid usage of reserved event
                                            number. */
        status = Notify_E_EVTRESERVED;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_sendEvent",
                             status,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId = procId;
        cmdArgs.lineId    = lineId;
        cmdArgs.eventId   = eventId;
        cmdArgs.payload = payload;
        cmdArgs.waitClear = waitClear;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_SENDEVENT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        /* Notify_E_NOTINITIALIZED and Notify_E_EVTNOTREGISTERED are run-time
         * failures.
         */
        if (   (status < 0)
            && (status != Notify_E_EVTNOTREGISTERED)) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_sendEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_sendEvent", status);

    /*! @retval Notify_S_SUCCESS Operation successful */
    return (status);
}


/*!
 *  @brief      Disable all events for specified procId.
 *              This is equivalent to global interrupt disable for specified
 *              processor, however restricted within interrupts handled
 *              by the Notify module. All callbacks registered for all events
 *              are disabled with this API. It is not possible to disable a
 *              specific callback.
 *
 *  @param      procId       Processor Id
 *
 *  @sa         Notify_restore
 */
UInt
Notify_disable (UInt16 procId,
                UInt16 lineId)
{
    Int32                   status = Notify_S_SUCCESS;
    UInt32                  key    = 0;
    Notify_CmdArgsDisable   cmdArgs;

    GT_2trace (curTrace, GT_ENTER, "Notify_disable", procId, lineId);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /* No retVal since this function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disable",
                             Notify_E_INVALIDARG,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /* No retVal since this function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disable",
                             Notify_E_INVALIDARG,
                             "Invalid lineId argument provided");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId = procId;
        cmdArgs.lineId = lineId;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_DISABLE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_disable",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
        else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
            key = cmdArgs.flags;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "Notify_disable", key);

    /*! @retval flags Flags indicating disable status. */
    return (key);
}


/*!
 *  @brief      Restore the Notify module for specified procId to the state
 *              before the last Notify_disable() was called. This is equivalent
 *              to global interrupt restore, however restricted within
 *              interrupts handled by the Notify module.
 *              All callbacks registered for all events as specified in the
 *              flags are enabled with this API. It is not possible to enable a
 *              specific callback.
 *
 *  @param      key          Key received from Notify_disable
 *  @param      procId       Processor Id
 *
 *  @sa         Notify_disable
 */
Void
Notify_restore (UInt16 procId,
                UInt16 lineId,
                UInt   key)
{
    Int32                   status = Notify_S_SUCCESS;
    Notify_CmdArgsRestore   cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "Notify_restore", procId, lineId, key);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /* No retVal since this function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_restore",
                             Notify_E_INVALIDARG,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /* No retVal since this function does not return status. */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_restore",
                             Notify_E_INVALIDARG,
                             "Invalid lineId argument provided");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.key = key;
        cmdArgs.procId = procId;
        cmdArgs.lineId = lineId;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_RESTORE, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_restore",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "Notify_restore");
}


/*!
 *  @brief      Disable a specific event.
 *              All callbacks registered for the specific event are disabled
 *              with this API. It is not possible to disable a specific
 *              callback.
 *
 *  @param      handle    Handle to the Notify Driver
 *  @param      procId    Processor Id
 *  @param      eventId   Event number to be disabled
 *
 *  @sa         Notify_enableEvent
 */
Void
Notify_disableEvent (UInt16              procId,
                     UInt16              lineId,
                     UInt32              eventId)
{
    Int32                      status      = Notify_S_SUCCESS;
    UInt32                     strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsDisableEvent cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "Notify_disableEvent",
               procId, lineId, eventId);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disableEvent",
                             Notify_E_INVALIDARG,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disableEvent",
                             Notify_E_INVALIDARG,
                             "Invalid lineId argument provided");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disableEvent",
                             Notify_E_EVTNOTREGISTERED,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_disableEvent",
                             curTrace,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId  = procId;
        cmdArgs.lineId  = lineId;
        cmdArgs.eventId = eventId;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_DISABLEEVENT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_disableEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

   GT_0trace (curTrace, GT_LEAVE, "Notify_disableEvent");
}


/*!
 *  @brief      Enable a specific event.
 *              All callbacks registered for this specific event are enabled
 *              with this API. It is not possible to enable a specific callback.
 *
 *  @param      handle    Handle to the Notify Driver
 *  @param      procId    Processor Id
 *  @param      eventId   Event number to be enabled
 *
 *  @sa         Notify_disableEvent
 */
Void
Notify_enableEvent (UInt16              procId,
                    UInt16              lineId,
                    UInt32              eventId)
{
    Int32                     status          = Notify_S_SUCCESS;
    UInt32                    strippedEventId = (eventId & Notify_EVENT_MASK);
    Notify_CmdArgsEnableEvent cmdArgs;

    GT_3trace (curTrace, GT_ENTER, "Notify_enableEvent",
               procId, lineId, eventId);

    GT_assert (curTrace, (Notify_state.setupRefCount > 0));
    GT_assert (curTrace, (procId < MultiProc_getNumProcessors ()));
    GT_assert (curTrace, (lineId < Notify_MAX_INTLINES));
    GT_assert (curTrace, (strippedEventId < (Notify_state.cfg.numEvents)));
    GT_assert (curTrace, \
                        (ISRESERVED(eventId, Notify_state.cfg.reservedEvents)));

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (procId >= MultiProc_getNumProcessors ()) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_enableEvent",
                             Notify_E_INVALIDARG,
                             "Invalid procId argument provided");
    }
    else if (lineId >= Notify_MAX_INTLINES) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_enableEvent",
                             Notify_E_INVALIDARG,
                             "Invalid lineId argument provided");
    }
    else if (strippedEventId >= (Notify_state.cfg.numEvents)) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_enableEvent",
                             Notify_E_EVTNOTREGISTERED,
                             "Invalid eventId specified.");
    }
    else if (!ISRESERVED(eventId, Notify_state.cfg.reservedEvents)) {
        /*  No retVal since function is Void */
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "Notify_enableEvent",
                             curTrace,
                             "Invalid usage of reserved event number");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        cmdArgs.procId  = procId;
        cmdArgs.lineId  = lineId;
        cmdArgs.eventId = eventId;
        status = NotifyDrvUsr_ioctl (CMD_NOTIFY_ENABLEEVENT, &cmdArgs);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (status < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "Notify_enableEvent",
                                 status,
                                 "API (through IOCTL) failed on kernel-side!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

   GT_0trace (curTrace, GT_LEAVE, "Notify_enableEvent");
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
