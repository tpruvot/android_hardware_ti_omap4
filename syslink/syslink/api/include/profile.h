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
 *  @file   profile.h
 *
 *  @desc   Defines instrumentation structures for profiling DSP/BIOS LINK.
 *  ============================================================================
 */


#if !defined (PROFILE_H)
#define PROFILE_H

/*  ----------------------------------- DSP/BIOS Link               */
#include <gpptypes.h>
#if defined (CHNL_COMPONENT)
#include <chnldefs.h>
#endif /* #if defined (CHNL_COMPONENT) */
#if defined (POOL_COMPONENT)
#include <pooldefs.h>
#endif /* #if defined (POOL_COMPONENT) */


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


#if defined (DDSP_PROFILE)


/** ============================================================================
 *  @const  DATA_SIZE
 *
 *  @desc   Number of bytes stored per buffer for recording the history of
 *          data transfer on a channel.
 *  ============================================================================
 */
#define DATA_LENGTH   8u

/** ============================================================================
 *  @name   HIST_LENGTH
 *
 *  @desc   Number of buffers to be recorded as history.
 *  ============================================================================
 */
#define HIST_LENGTH   8u

/** ============================================================================
 *  @name   HistoryData
 *
 *  @desc   Array to hold history data for a channel.
 *  ============================================================================
 */
typedef Char8    HistoryData [DATA_LENGTH] ;

/** ============================================================================
 *  @name   NumberOfBytes
 *
 *  @desc   Number of bytes transferred.
 *  ============================================================================
 */
typedef Uint32   NumberOfBytes ;

/** ============================================================================
 *  @name   NumberOfInterrupts
 *
 *  @desc   Number of interrupts sent to/received from DSP.
 *  ============================================================================
 */
typedef Uint32   NumberOfInterrupts ;


#if defined (PROC_COMPONENT)
/** ============================================================================
 *  @name   PROC_Instrument
 *
 *  @desc   Instrumentation data for a DSP processor.
 *
 *  @field  procId
 *              Processor identifier.
 *  @field  active
 *              Indicates if the DSP is in use.
 *  @field  dataToDsp
 *              Number of bytes sent to DSP.
 *  @field  dataFromDsp
 *              Number of bytes received from DSP.
 *  @field  intsToDsp
 *              Number of interrupts sent to DSP.
 *  @field  intsFromDsp
 *              Number of interrupts received from DSP.
 *  @field  activeLinks
 *              Number of links currently active.
 *  ============================================================================
 */
typedef struct PROC_Instrument_tag {
    ProcessorId        procId      ;
    Bool               active      ;
    NumberOfBytes      dataToDsp   ;
    NumberOfBytes      dataFromDsp ;
    NumberOfInterrupts intsToDsp   ;
    NumberOfInterrupts intsFromDsp ;
    Uint32             activeLinks ;
} PROC_Instrument ;


/** ============================================================================
 *  @name   PROC_Stats
 *
 *  @desc   Instrumentation data for the DSPs.
 *
 *  @field  procData
 *              Instrumentation data per dsp processor.
 *  ============================================================================
 */
typedef struct PROC_Stats_tag {
    PROC_Instrument  procData [MAX_DSPS] ;
} PROC_Stats ;
#endif  /* if defined (PROC_COMPONENT) */


#if defined (CHNL_COMPONENT)
/** ============================================================================
 *  @name   CHNL_Shared
 *
 *  @desc   Place holder for instrumentation data common to multiple channels.
 *
 *  @field  nameLinkDriver
 *              Name of the link driver.
 *  ============================================================================
 */
typedef struct CHNL_Shared_tag {
    Pstr    nameLinkDriver ;
} CHNL_Shared ;


/** ============================================================================
 *  @name   CHNL_Instrument
 *
 *  @desc   Instrumentation data for a channel.
 *
 *  @field  procId
 *              Processor identifier.
 *  @field  chnlId
 *              Channel identifier.
 *  @field  active
 *              Indicates if the channel is in use.
 *  @field  mode
 *              Indicates if the channel is input or output.
 *  @field  chnlShared
 *              Pointer to the shared information across multiple channels.
 *  @field  transferred
 *              Number of bytes transferred on channel.
 *  @field  numBufsQueued
 *              Number of currently queued buffers.
 *  @field  archive
 *              History of data sent on channel.
 *  ============================================================================
 */
typedef struct CHNL_Instrument_tag {
    ProcessorId      procId        ;
    ChannelId        chnlId        ;
    Bool             active        ;
    ChannelMode      mode          ;
    CHNL_Shared *    chnlShared    ;
    NumberOfBytes    transferred   ;
    Uint32           numBufsQueued ;
#if defined (DDSP_PROFILE_DETAILED)
    Uint32           archIndex     ;
    HistoryData      archive [HIST_LENGTH] ;
#endif /* defined (DDSP_PROFILE_DETAILED) */
} CHNL_Instrument ;


/** ============================================================================
 *  @name   CHNL_Stats
 *
 *  @desc   Instrumentation data for channels.
 *
 *  @field  chnlData
 *              Instrumentation data per channel.
 *  ============================================================================
 */
typedef struct CHNL_Stats_tag {
    CHNL_Instrument  chnlData ;
} CHNL_Stats ;
#endif  /* if defined (CHNL_COMPONENT) */


/** ============================================================================
 *  @name   DSP_Stats_tag
 *
 *  @desc   Instrumentation data per DSP.
 *
 *  @field  dataGppToDsp
 *              Number of bytes transferred to DSP.
 *  @field  dataDspToGpp
 *              Number of bytes transferred from DSP.
 *  @field  intsGppToDsp
 *              Number of interrupts to DSP.
 *  @field  intsDspToGpp
 *              Number of interrupts from DSP.
 *  ============================================================================
 */
struct DSP_Stats_tag {
    NumberOfBytes       dataGppToDsp ;
    NumberOfBytes       dataDspToGpp ;
    NumberOfInterrupts  intsGppToDsp ;
    NumberOfInterrupts  intsDspToGpp ;
} ;


#if defined (MSGQ_COMPONENT)
/** ============================================================================
 *  @name   MSGQ_Instrument
 *
 *  @desc   This structure defines the instrumentation data for a message queue.
 *
 *  @field  transferred
 *              Number of messages transferred on this MSGQ.
 *  @field  queued
 *              Number of messages currently queued on this MSGQ, pending calls
 *              to get them.
 *  ============================================================================
 */
typedef struct MSGQ_Instrument_tag {
    Uint32      transferred ;
    Uint32      queued ;
} MSGQ_Instrument ;

/** ============================================================================
 *  @name   MSGQ_Stats
 *
 *  @desc   This structure defines the instrumentation data for MSGQs on the
 *          local processor.
 *
 *  @field  msgqData
 *              Instrumentation data for the Message Queue.
 *  ============================================================================
 */
typedef struct MSGQ_Stats_tag {
    MSGQ_Instrument msgqData ;
} MSGQ_Stats ;

#endif /* defined (MSGQ_COMPONENT) */


#if defined (POOL_COMPONENT)
/** ============================================================================
 *  @name   MPBUF_Stats
 *
 *  @desc   This structure defines instrumentation data for the buffer pools.
 *
 *  @field  size
 *              Size of the buffers in this pool.
 *  @field  totalBuffers
 *              Total number of buffers in pool.
 *  @field  freeBuffers
 *              Number of free buffers in pool.
 *  @field  maxUsed
 *              Maximum number of buffers that have been used at least once
 *              since creation of the buffer pool, this helps in case of
 *              multiple runs.
 *  ============================================================================
 */
typedef struct MPBUF_Stats_tag {
    Uint16        size         ;
    Uint16        totalBuffers ;
    Uint16        freeBuffers  ;
    Uint16        maxUsed      ;
} MPBUF_Stats ;

/** ============================================================================
 *  @name   SMAPOOL_Stats
 *
 *  @desc   This structure defines instrumentation data for SMA.
 *
 *  @field  mpBufStats
 *              Location to store SMA instrument data.
 *  @field  bufHandleCount
 *              Total number of buffer pools.
 *  @field  conflicts
 *              Total number of conflicts.
 *  @field  numCalls
 *              Total number of calls made to MPCS entry and leave functions.
 *  ============================================================================
 */
typedef struct SMAPOOL_Stats_tag {
    MPBUF_Stats mpBufStats [MAX_SMABUFENTRIES] ;
    Uint16      bufHandleCount ;
    Uint32      conflicts ;
    Uint32      numCalls ;
} SMAPOOL_Stats ;
#endif /* if defined (POOL_COMPONENT) */

/** ============================================================================
 *  @name   IPS_Instrument
 *
 *  @desc   This structure defines the instrumentation data for an IPS object.
 *
 *  @field  registerCount
 *              Number of listeners registered.
 *  @field  unregisterCount
 *              Number of listeners unregistered.
 *  @field  eventSentCount
 *              Number of events sent.
 *  @field  eventOccurredCount
 *              Number of events occurred.
 *  ============================================================================
 */
typedef struct IPS_Instrument_tag {
    Uint32 registerCount ;
    Uint32 unregisterCount ;
    Uint32 eventSentCount ;
    Uint32 eventOccurredCount ;
} IPS_Instrument ;


#if defined (PROC_COMPONENT)
/** ============================================================================
 *  @deprecated The deprecated data structure ProcInstrument has been replaced
 *              with PROC_Instrument
 *
 *  ============================================================================
 */
#define ProcInstrument             PROC_Instrument

/** ============================================================================
 *  @deprecated The deprecated data structure ProcStats has been replaced with
 *              PROC_Stats
 *
 *  ============================================================================
 */
#define ProcStats                  PROC_Stats
#endif  /* if defined (PROC_COMPONENT) */
#if defined (CHNL_COMPONENT)
/** ============================================================================
 *  @deprecated The deprecated data structure ChnlShared has been replaced with
 *              CHNL_Shared
 *
 *  ============================================================================
 */
#define ChnlShared                 CHNL_Shared

/** ============================================================================
 *  @deprecated The deprecated data structure ChnlInstrument has been replaced
 *              with CHNL_Instrument
 *
 *  ============================================================================
 */
#define ChnlInstrument             CHNL_Instrument

/** ============================================================================
 *  @deprecated The deprecated data structure ChnlStats has been replaced with
 *              CHNL_Stats
 *
 *  ============================================================================
 */
#define ChnlStats                  CHNL_Stats
#endif  /* if defined (CHNL_COMPONENT) */

/** ============================================================================
 *  @deprecated The deprecated data structure DspStats has been replaced with
 *              DSP_Stats
 *
 *  ============================================================================
 */
#define DspStats                   DSP_Stats

#if defined (MSGQ_COMPONENT)
/** ============================================================================
 *  @deprecated The deprecated data structure MsgqInstrument has been replaced
 *              with MSGQ_Instrument
 *
 *  ============================================================================
 */
#define MsgqInstrument             MSGQ_Instrument

/** ============================================================================
 *  @deprecated The deprecated data structure MsgqStats has been replaced
 *              with MSGQ_Stats
 *
 *  ============================================================================
 */
#define MsgqStats                  MSGQ_Stats
#endif  /* if defined (MSGQ_COMPONENT) */

#if defined (POOL_COMPONENT)
/** ============================================================================
 *  @deprecated The deprecated data structure MPBUF_Stats has been replaced
 *              with MPBUF_Stats
 *
 *  ============================================================================
 */
#define MpBufStats                 MPBUF_Stats
/** ============================================================================
 *  @deprecated The deprecated data structure SmaPoolStats has been replaced
 *              with SMAPOOL_Stats.
 *
 *  ============================================================================
 */
#define SmaPoolStats               SMAPOOL_Stats
#endif  /* if defined (POOL_COMPONENT) */

/** ============================================================================
 *  @deprecated The deprecated data structure IpsInstrument has been replaced
 *              with IPS_Instrument
 *
 *  ============================================================================
 */
#define IpsInstrument              IPS_Instrument


#endif  /* if defined (DDSP_PROFILE) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* defined (PROFILE_H) */
