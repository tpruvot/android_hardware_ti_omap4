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
 *  @file   GateMutex.c
 *
 *  @brief      Gate based on Mutex
 *
 *  ============================================================================
 */



/* Standard headers */
#include <Std.h>

/* Osal & Utility headers */
#include <OsalMutex.h>
#include <Memory.h>
#include <Trace.h>

/* Module level headers */
#include <IObject.h>
#include <IGateProvider.h>
#include <GateMutex.h>


#if defined (__cplusplus)
extern "C" {
#endif


/* -----------------------------------------------------------------------------
 *  Structs & Enums
 * -----------------------------------------------------------------------------
 */
/*! @brief  Object for Gate Mutex */
struct GateMutex_Object {
        IGateProvider_SuperObject; /* For inheritance from IGateProvider */
        IOBJECT_SuperObject;       /* For inheritance for IObject */
    OsalMutex_Handle          mHandle;
    /*!< Handle to OSAL Mutex object */
};


/* -----------------------------------------------------------------------------
 *  Forward declarations
 * -----------------------------------------------------------------------------
 */
/* Forward declaration of function */
IArg   GateMutex_enter (GateMutex_Handle gmHandle);

/* Forward declaration of function */
Void GateMutex_leave (GateMutex_Handle gmHandle, IArg key);


/* -----------------------------------------------------------------------------
 *  APIs
 * -----------------------------------------------------------------------------
 */

/*!
 *  ======== GateMutex_Instance_init ========
 */
GateMutex_Handle
GateMutex_create (Void)
{
    GateMutex_Handle gmHandle = NULL;

    GT_0trace (curTrace, GT_ENTER, "GateMutex_create");

    /* Allocate memory for the gate object */
    gmHandle = (GateMutex_Handle) Memory_alloc (NULL,
                                              sizeof (GateMutex_Object),
                                              0);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (gmHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMutex_create",
                             GateMutex_E_MEMORY,
                             "Unable to allocate memory for the gmHandle!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        IGateProvider_ObjectInitializer (gmHandle, GateMutex);
        gmHandle->mHandle = OsalMutex_create (OsalMutex_Type_Interruptible);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
        if (gmHandle->mHandle == NULL) {
            Memory_free (NULL, gmHandle, sizeof (GateMutex_Object));
            gmHandle = NULL;
            GT_setFailureReason (curTrace,
                                 GT_4CLASS,
                                 "GateMutex_create",
                                 GateMutex_E_FAIL,
                                 "Unable to create Osal mutex object!");
        }
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "GateMutex_create", gmHandle);

    return gmHandle;
}


/*!
 *  @brief      Function to delete a Gate based on Mutex.
 *
 *  @param      gmHandle  Handle to previously created gate mutex instance.
 *
 *  @sa         GateMutex_create
 */
Int
GateMutex_delete (GateMutex_Handle * gmHandle)
{
    Int              status = GateMutex_S_SUCCESS;

    GT_1trace (curTrace, GT_ENTER, "GateMutex_delete", gmHandle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (gmHandle == NULL) {
        status = GateMutex_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMutex_delete",
                             status,
                             "gmHandle passed is invalid!");
    }
    else if (*gmHandle == NULL) {
        status = GateMutex_E_INVALIDARG;
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMutex_delete",
                             status,
                             "*gmHandle passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        OsalMutex_delete (&(*gmHandle)->mHandle);
        GT_assert (curTrace, (status >= 0));

        Memory_free (NULL, (*gmHandle), sizeof (GateMutex_Object));
        (*gmHandle) = NULL;
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "GateMutex_delete", status);

    return status;
}


/*!
 *  @brief      Function to enter a Gate Mutex.
 *
 *  @param      gmHandle  Handle to previously created gate mutex instance.
 *
 *  @sa         GateMutex_leave
 */
IArg
GateMutex_enter (GateMutex_Handle gmHandle)
{
    IArg   key = 0x0;

    GT_1trace (curTrace, GT_ENTER, "GateMutex_enter", gmHandle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (gmHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMutex_enter",
                             GateMutex_E_INVALIDARG,
                             "Handle passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        key = OsalMutex_enter ((OsalMutex_Handle) gmHandle->mHandle);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_1trace (curTrace, GT_LEAVE, "GateMutex_enter", key);

    /*! @retval Flags Operation successful */
    return key;
}


/*!
 *  @brief      Function to leave a Gate Mutex.
 *
 *  @param      gmHandle    Handle to previously created gate mutex instance.
 *  @param      key       Flags.
 *
 *  @sa         GateMutex_enter
 */
Void
GateMutex_leave (GateMutex_Handle gmHandle, IArg   key)
{
    GT_1trace (curTrace, GT_ENTER, "GateMutex_leave", gmHandle);

#if !defined(SYSLINK_BUILD_OPTIMIZE)
    if (gmHandle == NULL) {
        GT_setFailureReason (curTrace,
                             GT_4CLASS,
                             "GateMutex_enter",
                             GateMutex_E_INVALIDARG,
                             "Handle passed is invalid!");
    }
    else {
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */
        OsalMutex_leave ((OsalMutex_Handle) gmHandle->mHandle, key);
#if !defined(SYSLINK_BUILD_OPTIMIZE)
    }
#endif /* if !defined(SYSLINK_BUILD_OPTIMIZE) */

    GT_0trace (curTrace, GT_LEAVE, "GateMutex_leave");
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
