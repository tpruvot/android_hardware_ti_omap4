/*
 *  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/** ============================================================================
 *  @file   errbase.h
 *
 *  @desc   Central repository for error and status code bases and ranges for
 *          DSP/BIOS LINK and any Algorithm Framework built on top of LINK.
 *
 *  ============================================================================
 */


#if !defined (ERRBASE_H)
#define ERRBASE_H


/*  ----------------------------------- IPC headers */
#include <gpptypes.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/* Define the return code type */
typedef Int32  DSP_STATUS ;

/* Success & Failure macros*/
#define DSP_SUCCEEDED(status)   (    ((Int32) (status) >= (DSP_SBASE))   \
                                 &&  ((Int32) (status) <= (DSP_SLAST)))

#define DSP_FAILED(status)      (!DSP_SUCCEEDED (status))

/* Success & Failure macros*/
#define RINGIO_SUCCEEDED(status) (    ((Int32) (status) >= (RINGIO_SBASE))  \
                                  &&  ((Int32) (status) <= (RINGIO_SLAST)))

#define RINGIO_FAILED(status)    (    (!RINGIO_SUCCEEDED (status))          \
                                  &&  (DSP_FAILED (status)))


/* Base and range of generic success codes. */
#define DSP_SBASE               (DSP_STATUS)0x00008000l
#define DSP_SLAST               (DSP_STATUS)0x00008500l

/* Base and range of component success codes */
#define RINGIO_SBASE            (DSP_STATUS)0x00008100l
#define RINGIO_SLAST            (DSP_STATUS)0x000081FFl

/* Base and range of generic error codes */
#define DSP_EBASE               (DSP_STATUS)0x80008000l
#define DSP_ELAST               (DSP_STATUS)0x800081FFl

/* Base and range of component error codes */
#define DSP_COMP_EBASE          (DSP_STATUS)0x80040200l
#define DSP_COMP_ELAST          (DSP_STATUS)0x80047fffl


/*  ============================================================================
 *  SUCCESS Codes
 *  ============================================================================
 */

/*  ----------------------------------------------------------------------------
 *  SUCCESS codes: Generic
 *  ----------------------------------------------------------------------------
 */
/* Generic success code */
#define DSP_SOK                     (DSP_SBASE + 0x0l)


/* GPP is already attached to this DSP processor */
#define DSP_SALREADYATTACHED        (DSP_SBASE + 0x1l)

/* This is the last object available for enumeration. */
#define DSP_SENUMCOMPLETE           (DSP_SBASE + 0x2l)

/* Object has been finalized successfully. */
#define DSP_SFINALIZED              (DSP_SBASE + 0x3l)

/* The MSGQ transport/pool is already opened. */
#define DSP_SALREADYOPENED          (DSP_SBASE + 0x4l)

/* The resource already exists. */
#define DSP_SEXISTS                 (DSP_SBASE + 0x5l)

/* The resource has been freed. */
#define DSP_SFREE                   (DSP_SBASE + 0x6l)

/* The DSPLINK driver has already been setup by some other
 * application/process.
 */
#define DSP_SALREADYSETUP           (DSP_SBASE + 0x7l)

/* The DSPLINK driver has been finalized. */
#define DSP_SDESTROYED              (DSP_SBASE + 0x8l)

/* The final process has detached from the specific processor. */
#define DSP_SDETACHED               (DSP_SBASE + 0x9l)

/* The DSP has already been loaded with an executable. */
#define DSP_SALREADYLOADED          (DSP_SBASE + 0xAl)

/* The DSP has already been started with an executable. */
#define DSP_SALREADYSTARTED         (DSP_SBASE + 0xBl)

/* The final process has stopped the DSP execution. */
#define DSP_SSTOPPED                (DSP_SBASE + 0xCl)

/* The final process has closed the resource (e.g. MSGQ transport) */
#define DSP_SCLOSED                 (DSP_SBASE + 0xDl)

/*  ----------------------------------------------------------------------------
 *  SUCCESS codes: RINGIO
 *  ----------------------------------------------------------------------------
 */

/* Success code for RingIO component */
#define RINGIO_SUCCESS              (RINGIO_SBASE + 0x0l)

/* Indicates that either: 1) The amount of data requested could not be
                             serviced due to the presence of an attribute
                          2) During an attribute read if another is also present
                             at the same offset  */
#define RINGIO_SPENDINGATTRIBUTE    (RINGIO_SBASE + 0x1l)


/*  ============================================================================
 *  FAILURE Codes
 *  ============================================================================
 */
/*  ----------------------------------------------------------------------------
 *  FAILURE codes: Generic
 *  ----------------------------------------------------------------------------
 */
/* The caller does not have access privileges to call this function */
#define DSP_EACCESSDENIED           (DSP_EBASE + 0x0l)

/* The Specified Connection already exists */
#define DSP_EALREADYCONNECTED       (DSP_EBASE + 0x1l)

/* The GPP process is not attached to the specific processor. */
#define DSP_EATTACHED               (DSP_EBASE + 0x2l)

/* During enumeration a change in the number or properties of the objects
 * has occurred.
 */
#define DSP_ECHANGEDURINGENUM       (DSP_EBASE + 0x3l)

/* An error occurred while parsing the DSP executable file */
#define DSP_ECORRUPTFILE            (DSP_EBASE + 0x4l)

/* A failure occurred during a delete operation */
#define DSP_EDELETE                 (DSP_EBASE + 0x5l)

/* The specified direction is invalid */
#define DSP_EDIRECTION              (DSP_EBASE + 0x6l)

/* A stream has been issued the maximum number of buffers allowed in the
 * stream at once.  buffers must be reclaimed from the stream before any
 * more can be issued.
 */
#define DSP_ESTREAMFULL             (DSP_EBASE + 0x7l)

/* A general failure occurred */
#define DSP_EFAIL                   (DSP_EBASE + 0x8l)

/* The specified executable file could not be found. */
#define DSP_EFILE                   (DSP_EBASE + 0x9l)

/* The specified handle is invalid. */
#define DSP_EHANDLE                 (DSP_EBASE + 0xAl)

/* An invalid argument was specified. */
#define DSP_EINVALIDARG             (DSP_EBASE + 0xBl)

/* A memory allocation failure occurred. */
#define DSP_EMEMORY                 (DSP_EBASE + 0xCl)

/* The DSPLINK driver has not been set up. */
#define DSP_ESETUP                  (DSP_EBASE + 0xDl)

/* The DSP has not been started. */
#define DSP_ESTARTED                (DSP_EBASE + 0xEl)

/* The module has not been initialized */
#define DSP_EINIT                   (DSP_EBASE + 0xFl)

/* The indicated operation is not supported. */
#define DSP_ENOTIMPL                (DSP_EBASE + 0x10l)

/* I/O is currently pending. */
#define DSP_EPENDING                (DSP_EBASE + 0x11l)

/* An invalid pointer was specified. */
#define DSP_EPOINTER                (DSP_EBASE + 0x12l)

/* A parameter is specified outside its valid range. */
#define DSP_ERANGE                  (DSP_EBASE + 0x13l)

/* An invalid size parameter was specified. */
#define DSP_ESIZE                   (DSP_EBASE + 0x14l)

/* A stream creation failure occurred on the DSP. */
#define DSP_ESTREAM                 (DSP_EBASE + 0x15l)

/* A task creation failure occurred on the DSP. */
#define DSP_ETASK                   (DSP_EBASE + 0x16l)

/* A timeout occurred before the requested operation could complete. */
#define DSP_ETIMEOUT                (DSP_EBASE + 0x17l)

/* A data truncation occurred, e.g., when requesting a descriptive error
 * string, not enough space was allocated for the complete error message.
 */
#define DSP_ETRUNCATED              (DSP_EBASE + 0x18l)

/* The resource (e.g. MSGQ transport) is not opened. */
#define DSP_EOPENED                 (DSP_EBASE + 0x19l)

/* A parameter is invalid. */
#define DSP_EVALUE                  (DSP_EBASE + 0x1Al)

/* The state of the specified object is incorrect for the requested
 * operation.
 */
#define DSP_EWRONGSTATE             (DSP_EBASE + 0x1Bl)

/* The DSPLINK driver is already setup in this process. */
#define DSP_EALREADYSETUP           (DSP_EBASE + 0x1Cl)

/* The operation was interrupted. */
#define DSP_EINTR                   (DSP_EBASE + 0x1Dl)

/* The specific DSP is already started in this process. */
#define DSP_EALREADYSTARTED         (DSP_EBASE + 0x1El)

/* The MSGQ transport/pool is already opened in this process. */
#define DSP_EALREADYOPENED          (DSP_EBASE + 0x1Fl)

/* Reserved error codes */
#define DSP_ERESERVED_06            (DSP_EBASE + 0x20l)
#define DSP_ERESERVED_07            (DSP_EBASE + 0x21l)
#define DSP_ERESERVED_08            (DSP_EBASE + 0x22l)
#define DSP_ERESERVED_09            (DSP_EBASE + 0x23l)
#define DSP_ERESERVED_0A            (DSP_EBASE + 0x24l)
#define DSP_ERESERVED_0B            (DSP_EBASE + 0x25l)
#define DSP_ERESERVED_0C            (DSP_EBASE + 0x26l)
#define DSP_ERESERVED_0D            (DSP_EBASE + 0x27l)

/* A requested resource is not available. */
#define DSP_ERESOURCE               (DSP_EBASE + 0x28l)

/* A critical error has occurred, and the DSP is being re-started. */
#define DSP_ERESTART                (DSP_EBASE + 0x29l)

/* A DSP memory free operation failed. */
#define DSP_EFREE                   (DSP_EBASE + 0x2Al)

/* A DSP I/O free operation failed. */
#define DSP_EIOFREE                 (DSP_EBASE + 0x2Bl)

/* Multiple instances are not allowed. */
#define DSP_EMULINST                (DSP_EBASE + 0x2Cl)

/* A specified entity was not found. */
#define DSP_ENOTFOUND               (DSP_EBASE + 0x2Dl)

/* A DSP I/O resource is not available. */
#define DSP_EOUTOFIO                (DSP_EBASE + 0x2El)

/* Address Translation between ARM and DSP has failed */
#define DSP_ETRANSLATE              (DSP_EBASE + 0x2fl)

/* Version mismatch between the GPP and DSP-sides of DSPLINK. */
#define DSP_EVERSION                (DSP_EBASE + 0x30l)

/* File or section load write function failed to write to DSP */
#define DSP_EFWRITE                 (DSP_EBASE + 0x31l)

/* Unable to find a named section in DSP executable */
#define DSP_ENOSECT                 (DSP_EBASE + 0x32l)

/* Reserved error code */
#define DSP_ERESERVED_0F            (DSP_EBASE + 0x33l)
#define DSP_ERESERVED_10            (DSP_EBASE + 0x34l)
#define DSP_ERESERVED_11            (DSP_EBASE + 0x35l)
#define DSP_ERESERVED_12            (DSP_EBASE + 0x36l)
#define DSP_ERESERVED_13            (DSP_EBASE + 0x37l)
#define DSP_ERESERVED_14            (DSP_EBASE + 0x38l)
#define DSP_ERESERVED_15            (DSP_EBASE + 0x39l)
#define DSP_ERESERVED_16            (DSP_EBASE + 0x3Al)
#define DSP_ERESERVED_17            (DSP_EBASE + 0x3Bl)
#define DSP_ERESERVED_18            (DSP_EBASE + 0x3Cl)
#define DSP_ERESERVED_19            (DSP_EBASE + 0x3Dl)
#define DSP_ERESERVED_1A            (DSP_EBASE + 0x3El)
#define DSP_ERESERVED_1B            (DSP_EBASE + 0x3Fl)
#define DSP_ERESERVED_1C            (DSP_EBASE + 0x40l)
#define DSP_ERESERVED_1D            (DSP_EBASE + 0x41l)
#define DSP_ERESERVED_1E            (DSP_EBASE + 0x42l)
#define DSP_ERESERVED_1F            (DSP_EBASE + 0x43l)
#define DSP_ERESERVED_20            (DSP_EBASE + 0x44l)
#define DSP_ERESERVED_21            (DSP_EBASE + 0x45l)
#define DSP_ERESERVED_22            (DSP_EBASE + 0x46l)
#define DSP_ERESERVED_23            (DSP_EBASE + 0x47l)
#define DSP_ERESERVED_24            (DSP_EBASE + 0x48l)
#define DSP_ERESERVED_25            (DSP_EBASE + 0x49l)
#define DSP_ERESERVED_26            (DSP_EBASE + 0x4al)
#define DSP_ERESERVED_27            (DSP_EBASE + 0x4bl)
#define DSP_ERESERVED_28            (DSP_EBASE + 0x4cl)
#define DSP_ERESERVED_29            (DSP_EBASE + 0x4dl)
#define DSP_ERESERVED_2A            (DSP_EBASE + 0x4el)
#define DSP_ERESERVED_2B            (DSP_EBASE + 0x4fl)

/* The connection requested by the client already exists */
#define DSP_EALREADYEXISTS          (DSP_EBASE + 0x50l)

/* Timeout parameter was "NO_WAIT", yet the operation was
 * not completed.
 */
#define DSP_ENOTCOMPLETE            (DSP_EBASE + 0x51l)

/* Invalid configuration. This indicates that configuration information
 * provided is incorrect, or the DSP configuration does not match the
 * configuration expected by the GPP-side.
 */
#define DSP_ECONFIG                 (DSP_EBASE + 0x52l)

/* Feature is not supported */
#define DSP_ENOTSUPPORTED           (DSP_EBASE + 0x53l)

/* DSP is not ready to respond to requested command */
#define DSP_ENOTREADY               (DSP_EBASE + 0x54l)


/*  ----------------------------------------------------------------------------
 *  FAILURE codes: RINGIO
 *  ----------------------------------------------------------------------------
 */
#define RINGIO_EBASE                (DSP_EBASE + 0x55l)

/* Generic RingIO error code */
#define RINGIO_EFAILURE             (RINGIO_EBASE + 0x00l)

/* Indicates that the amount of data requested could not be serviced due to the
   ring buffer getting wrapped */
#define RINGIO_EBUFWRAP             (RINGIO_EBASE + 0x01l)

/* Indicates that there is no data in the buffer for reading */
#define RINGIO_EBUFEMPTY            (RINGIO_EBASE + 0x02l)

/* Indicates that the buffer is full */
#define RINGIO_EBUFFULL             (RINGIO_EBASE + 0x03l)

/* Indicates that there is no attribute at the current, but attributes are
   present at a future offset */
#define RINGIO_EPENDINGDATA         (RINGIO_EBASE + 0x04l)

/* Indicates that attibute get() failed, need to extract variable length message
   getv() */
#define RINGIO_EVARIABLEATTRIBUTE   (RINGIO_EBASE + 0x05l)

/* Indicates that the RingIO being created already exists */
#define RINGIO_EALREADYEXISTS       (RINGIO_EBASE + 0x06l)

/* Indicates that the valid data is present in the RingIO
 * but it is not contiguous.
 */
#define RINGIO_ENOTCONTIGUOUSDATA   (RINGIO_EBASE + 0x07l)

/* Indicates that the RingIO is in a wrong state */
#define RINGIO_EWRONGSTATE          (RINGIO_EBASE + 0x08l)


/* Reserved error code */
#define DSP_ERESERVED_BASE_1        (DSP_COMP_EBASE + 0x000l)
#define DSP_ERESERVED_BASE_2        (DSP_COMP_EBASE + 0x100l)
#define DSP_ERESERVED_BASE_3        (DSP_COMP_EBASE + 0x200l)
#define DSP_ERESERVED_BASE_4        (DSP_COMP_EBASE + 0x300l)
#define DSP_ERESERVED_BASE_5        (DSP_COMP_EBASE + 0x400l)


/*  ----------------------------------------------------------------------------
 *  FAILURE codes: CHNL
 *  ----------------------------------------------------------------------------
 */
#define CHNL_EBASE                  (DSP_COMP_EBASE + 0x500l)

/* Attempt to create too many channels. */
#define CHNL_E_MAXCHANNELS          (CHNL_EBASE + 0x00l)

/* Reserved error code */
#define CHNL_E_RESERVED_1           (CHNL_EBASE + 0x01l)

/* No free channels are available. */
#define CHNL_E_OUTOFSTREAMS         (CHNL_EBASE + 0x02l)

/* Channel ID is out of range. */
#define CHNL_E_BADCHANID            (CHNL_EBASE + 0x03l)

/* Channel is already in use. */
#define CHNL_E_CHANBUSY             (CHNL_EBASE + 0x04l)

/* Invalid channel mode argument. */
#define CHNL_E_BADMODE              (CHNL_EBASE + 0x05l)

/* Timeout parameter was "NO_WAIT", yet no I/O completions
 * were queued.
 */
#define CHNL_E_NOIOC                (CHNL_EBASE + 0x06l)

/* I/O has been cancelled on this channel. */
#define CHNL_E_CANCELLED            (CHNL_EBASE + 0x07l)

/* End of stream was already requested on this output channel. */
#define CHNL_E_EOS                  (CHNL_EBASE + 0x09l)

/* Unable to create the channel event object. */
#define CHNL_E_CREATEEVENT          (CHNL_EBASE + 0x0Al)

/* Reserved error code */
#define CHNL_E_RESERVED_2           (CHNL_EBASE + 0x0Bl)

/* Reserved error code */
#define CHNL_E_RESERVED_3           (CHNL_EBASE + 0x0Cl)

/* DSP word size of zero configured for this device. */
#define CHNL_E_INVALIDWORDSIZE      (CHNL_EBASE + 0x0Dl)

/* Reserved error code */
#define CHNL_E_RESERVED_4           (CHNL_EBASE + 0x0El)

/* Reserved error code */
#define CHNL_E_RESERVED_5           (CHNL_EBASE + 0x0Fl)

/* Reserved error code */
#define CHNL_E_RESERVED_6           (CHNL_EBASE + 0x10l)

/* Unable to plug channel ISR for configured IRQ. */
#define CHNL_E_ISR                  (CHNL_EBASE + 0x11l)

/* No free I/O request packets are available. */
#define CHNL_E_NOIORPS              (CHNL_EBASE + 0x12l)

/* Buffer size is larger than the size of physical channel. */
#define CHNL_E_BUFSIZE              (CHNL_EBASE + 0x13l)

/* User cannot mark end of stream on an input channel. */
#define CHNL_E_NOEOS                (CHNL_EBASE + 0x14l)

/* Wait for flush operation on an output channel timed out. */
#define CHNL_E_WAITTIMEOUT          (CHNL_EBASE + 0x15l)

/* Reserved error code */
#define CHNL_E_RESERVED_7           (CHNL_EBASE + 0x16l)

/* Reserved error code */
#define CHNL_E_RESERVED_8           (CHNL_EBASE + 0x17l)

/* Unable to prepare buffer specified */
#define CHNL_E_RESERVED_9           (CHNL_EBASE + 0x18l)

/* Unable to Unprepare buffer specified */
#define CHNL_E_RESERVED_10          (CHNL_EBASE + 0x19l)

/* The operation failed because it was started from a wrong state */
#define CHNL_E_WRONGSTATE           (CHNL_EBASE + 0x1Al)


/*  ----------------------------------------------------------------------------
 *  FAILURE codes: SYNC
 *  ----------------------------------------------------------------------------
 */
#define SYNC_EBASE                  (DSP_COMP_EBASE + 0x600l)

/* Wait on a kernel event failed. */
#define SYNC_E_FAIL                 (SYNC_EBASE + 0x00l)

/* Timeout expired while waiting for event to be signalled. */
#define SYNC_E_TIMEOUT              (SYNC_EBASE + 0x01l)


/* Reserved error code */
#define DSP_ERESERVED_BASE_6        (DSP_COMP_EBASE + 0x700l)
#define DSP_ERESERVED_BASE_7        (DSP_COMP_EBASE + 0x800l)


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (ERRBASE_H) */
