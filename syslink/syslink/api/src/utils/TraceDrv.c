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
 *  @file   TraceDrv.c
 *
 *  @brief      User-side OS-specific implementation of TraceDrv driver for Linux
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <TraceDrv.h>
#include <OsalDrv.h>
#include <TraceDrvDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */


/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/* Extern declaration to Global trace flag. */
extern Int curTrace;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to invoke the APIs through ioctl.
 *
 *  @param  cmd     Command for driver ioctl
 *  @param  args    Arguments for the ioctl command
 *
 *  @sa
 */
Void
TraceDrv_ioctl (UInt32 cmd, Ptr args)
{
    int osStatus = 0;
    Bool done = FALSE;
    TraceDrv_CmdArgs * cmdArgs = (TraceDrv_CmdArgs *) args;

    GT_2trace (curTrace, GT_ENTER, "TraceDrv_ioctl", cmd, args);

    if (cmd == CMD_TRACEDRV_SETTRACE) {
        if (cmdArgs->args.setTrace.type == GT_TraceType_User) {
            cmdArgs->args.setTrace.oldMask = curTrace;
            curTrace = cmdArgs->args.setTrace.mask;
            done = TRUE;
        }
    }

    if (done == FALSE) {
        osStatus = OsalDrv_ioctl (cmd, args);
        GT_assert (curTrace, (osStatus >= 0));
        if (osStatus < 0) {
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "TraceDrv_ioctl",
                                 OSALDRV_E_OSFAILURE,
                                 "Driver ioctl failed!");
        }
    }

    GT_0trace (curTrace, GT_LEAVE, "TraceDrv_ioctl");
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
