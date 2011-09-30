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
 *  @file   IMessageQTransport.h
 *
 *  @brief      Defines types and functions that interface the specific
 *              implementations of MessageQ transports to the MessageQ module.
 *  ============================================================================
 */


#if !defined (IMessageQTransport_H)
#define IMessageQTransport_H


/* Utils & OSAL */
#include <Trace.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/* =============================================================================
 *  Forward declarations
 * =============================================================================
 */
/*! @brief Forward declaration of structure defining object for the
 *         Heap module
 */
typedef struct IMessageQTransport_Object_tag IMessageQTransport_Object;

/*!
 *  @brief  Handle for the Heap Buf.
 */
typedef struct IMessageQTransport_Object * IMessageQTransport_Handle;

/* =============================================================================
 *  Typedefs and structures
 * =============================================================================
 */
/*!
 *  @brief  Structure defining the reason for error function being called
 */
typedef enum  IMessageQTransport_Reason_tag {
    IMessageQTransport_Reason_FAILEDPUT,
    /*!< Failed to send the message. */
    IMessageQTransport_Reason_INTERNALERR,
    /*!< An internal error occurred in the transport */
    IMessageQTransport_Reason_PHYSICALERR,
    /*!<  An error occurred in the physical link in the transport */
    IMessageQTransport_Reason_FAILEDALLOC
    /*!<  Failed to allocate a message. */
} IMessageQTransport_Reason;


/*!
 *  @brief  Typedef for transport error callback function.
 *
 *  First parameter: Why the error function is being called.
 *
 *  Second parameter: Handle of transport that had the error. NULL denotes
 *  that it is a system error, not a specific transport.
 *
 *  Third parameter: Pointer to the message. This is only valid for
 *  #TransportShm_Reason_FAILEDPUT.
 *
 *  Fourth parameter: Transport specific information. Refer to individual
 *  transports for more details.
 */
typedef Void (*IMessageQTransport_ErrFxn) (IMessageQTransport_Reason reason,
                                           IMessageQTransport_Handle handle,
                                           Ptr                 msg,
                                           UArg                info);


/* =============================================================================
 *  Function pointer types for heap operations
 * =============================================================================
 */
/*! @brief Type for function pointer to put the message to the remote processor
 */
typedef Int (*IMessageQTransport_putFxn) (IMessageQTransport_Handle    handle,
                                          Ptr                          msg);

/*! @brief Type for function pointer to get the status of a Transport instance
 */
typedef Int (*IMessageQTransport_getStatusFxn)
                                        (IMessageQTransport_Handle handle);

/*! @brief Type for function pointer to Send a control command to the transport
 * instance
 */
typedef Int (*IMessageQTransport_controlFxn) (IMessageQTransport_Handle handle,
                                              UInt                      cmd,
                                              UArg                      cmdArg);


/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */
/*!
 *  @brief  Structure for the Handle for the transport.
 */
struct IMessageQTransport_Object_tag {
    IMessageQTransport_putFxn         put;
    /*!<  Put a message to remote processor */
    IMessageQTransport_getStatusFxn   getStatus;
    /*!<  Get the transport status */
    IMessageQTransport_controlFxn     controlFn;
    /*!<  Send a control command to the transport */
    Ptr                               obj;
    /*!<  Actual transport Handle */
};


/* =============================================================================
 *  APIs
 * =============================================================================
 */
/*!
 *  @brief      Allocate a block of memory of specified size.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      size      Size to be allocated (in bytes)
 *  @param      align     Alignment for allocation (power of 2)
 *
 *  @retval     buffer    Allocated buffer
 *
 *  @sa         IMessageQTransport_free
 */
static inline Int IMessageQTransport_put (IMessageQTransport_Handle    handle,
                                          Ptr                          msg)
{
    Int status;
    GT_assert (curTrace, (((IMessageQTransport_Object *) handle)->put != NULL));
    status = ((IMessageQTransport_Object *) handle)->put (handle, msg);
    return (status);
}


/*!
 *  @brief      Frees a block of memory.
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @param      block     Block of memory to be freed.
 *  @param      size      Size to be freed (in bytes)
 *
 *  @sa         Heap_alloc
 */
static inline Int
IMessageQTransport_getStatus (IMessageQTransport_Handle handle)
{
    Int status;
    GT_assert (curTrace,
               (((IMessageQTransport_Object *) handle)->getStatus != NULL));
    status = ((IMessageQTransport_Object *) handle)->getStatus (handle);
    return status;
}


/*!
 *  @brief      Get memory statistics
 *
 *  @param      handle    Handle to previously created/opened instance.
 *  @params     stats     Memory statistics structure
 *
 *  @sa
 */
static inline  Int IMessageQTransport_control (IMessageQTransport_Handle handle,
                                               UInt                      cmd,
                                               UArg                      cmdArg)
{
    Int status;
    GT_assert (curTrace,
               (((IMessageQTransport_Object *) handle)->controlFn != NULL));
    status = ((IMessageQTransport_Object *) handle)->controlFn (handle,
                                                                cmd,
                                                                cmdArg);
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (IMessageQTransport_H) */
