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
 *  @file   Trace.c
 *
 *  @brief      Trace implementation.
 *
 *              This abstracts and implements the definitions for
 *              user side traces statements and also details
 *              of variable traces supported in existing
 *              implementation.
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/* OSAL and kernel utils */
#include <Trace.h>
#include <TraceDrv.h>
#include <TraceDrvDefs.h>
#include <OsalPrint.h>


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @brief      Global trace flag.
 */
//Int curTrace = 0;
Int curTrace = GT_TraceState_Enable | GT_TraceEnter_Enable | GT_TraceSetFailure_Enable |
                            0x000F0000;

/*!
 *  @brief      Function to log the trace with zero parameters and just
 *              information string.
 *  @param      mask type of traces.
 *  @param      classtype One of three classes in Syslink where this trace need
 *              to be enabed.
 *  @param      The debug string.
 */
Void
_GT_0trace (UInt32 mask, GT_TraceClass classtype, Char * infoString)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with one additional parameter
 *  @param      mask type of traces
 *  @param      classtype One of three classes in Syslink where this trace
 *              need to be enabed.
 *  @param      The debug string.
 *  @param      param The additional parameter which needs to be logged.
 */
Void
_GT_1trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         infoString,
            UInt32         param)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString, param);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with two additional parameters
 *  @param      mask type of traces
 *  @param      classtype One of three classes in Syslink where this trace
 *              need to be enabed.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 */
Void
_GT_2trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         infoString,
            UInt32         param0,
            UInt32         param1)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString, param0, param1);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with three parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes in Syslink where this trace
 *              need to be enabed.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 */
Void
_GT_3trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1, param2);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString, param0, param1, param2);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with four parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes in Syslink where this trace
 *              need to be enabed.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 *  @param      param3 The fourth parameter which needs to be logged.
 */
Void
_GT_4trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2,
            UInt32         param3)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1, param2, param3);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString, param0, param1, param2, param3);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with five parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes in Syslink where this trace
 *              need to be enabed.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 *  @param      param3 The fourth parameter which needs to be logged.
 *  @param      param4 The fifth parameter which needs to be logged.
 */
Void
_GT_5trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2,
            UInt32         param3,
            UInt32         param4)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString,
                             param0,
                             param1,
                             param2,
                             param3,
                             param4);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                Osal_printf (infoString,
                             param0,
                             param1,
                             param2,
                             param3,
                             param4);
            }
        }
    }
}


/*!
 *  @brief      Function to report the syslink failure and log the trace. This
 *              is mostly the fatal error and system can not recover without
 *              module restart.
 *  @param      mask        Indicates whether SetFailure is enabled.
 *  @param      func        Name of the function where this oc.cured
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      status      What was the code we got/set for this failure
 *  @param      msg         Any additional information which can be useful for
 *                          deciphering the error condition.
 */
Void
_GT_setFailureReason (Int    mask,
                           Char * func,
                           Char * fileName,
                           UInt32 lineNo,
                           UInt32 status,
                           Char * msg)
{
    if (    ((mask & GT_TRACESETFAILURE_MASK) >> GT_TRACESETFAILURE_SHIFT)
        ==  GT_TraceState_Enable) {
        Osal_printf ("*** %s: %s\tError [0x%x] at Line no: %d in file %s\n",
                     func,
                     msg,
                     status,
                     lineNo,
                     fileName);
    }
}


/*!
 *  @brief      Function to change the trace mask setting.
 *
 *  @param      mask   Trace mask to be set
 *  @param      type   Type of trace to be set
 */
UInt32
_GT_setTrace (UInt32 mask, GT_TraceType type)
{
    TraceDrv_CmdArgs cmdArgs;

    GT_1trace (curTrace, GT_1CLASS, "Setting Trace to: [0x%x]\n", mask);

    cmdArgs.args.setTrace.mask = mask;
    cmdArgs.args.setTrace.type = type;
    TraceDrv_ioctl (CMD_TRACEDRV_SETTRACE, &cmdArgs);

    /*! @retval old-mask Operation successfully completed. */
    return cmdArgs.args.setTrace.oldMask;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

