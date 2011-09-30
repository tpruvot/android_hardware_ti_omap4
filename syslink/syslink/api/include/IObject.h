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
 *  @file   IObject.h
 *
 *  @brief      Interface to provide object creation facilities.
 *
 *  ============================================================================
 */


#ifndef __IOBJECT_H__
#define __IOBJECT_H__


/* -------------------------------------------------- Utilities headers */
#include <Memory.h>


/* -------------------------------------------------- Module headers */
#include <Gate.h>

#if defined (__cplusplus)
extern "C" {
#endif

/* ObjType */
typedef enum Ipc_ObjType {
    Ipc_ObjType_CREATESTATIC         = 0x1,
    Ipc_ObjType_CREATESTATIC_REGION  = 0x2,
    Ipc_ObjType_CREATEDYNAMIC        = 0x4,
    Ipc_ObjType_CREATEDYNAMIC_REGION = 0x8,
    Ipc_ObjType_OPENDYNAMIC          = 0x10,
    Ipc_ObjType_LOCAL                = 0x20
} Ipc_ObjType;


/*!
 *      Object embedded in other module's object
 */
#define IOBJECT_SuperObject                                                    \
        Ptr next;                                                                  \
        Int status;


/*!
 *  Generic macro to define a create/delete function for a module
 */
#define IOBJECT_CREATE0(MNAME) \
static MNAME##_Object * MNAME##_firstObject = NULL;\
\
\
MNAME##_Handle MNAME##_create (const MNAME##_Params * params)\
{\
    IArg key;\
    MNAME##_Object * obj = (MNAME##_Object *) Memory_alloc (NULL,\
                                                    sizeof (MNAME##_Object),\
                                                    0,\
                                                    NULL);\
    if (!obj) return NULL;\
    Memory_set (obj, 0, sizeof (MNAME##_Object));\
    obj->status = MNAME##_Instance_init (obj, params);\
    if (obj->status == 0) {\
        key = Gate_enterSystem ();\
        if (MNAME##_firstObject == NULL) {\
            MNAME##_firstObject = obj;\
            obj->next = NULL;\
        }\
        else {\
            obj->next = MNAME##_firstObject;\
            MNAME##_firstObject = obj;\
        }\
        Gate_leaveSystem (key);\
    }\
    else {\
        Memory_free (NULL, obj, sizeof (MNAME##_Object));\
        obj = NULL;\
    }\
    return (MNAME##_Handle)obj;\
}\
\
\
Int MNAME##_delete (MNAME##_Handle * handle)\
{\
    IArg key;\
    MNAME##_Object * temp;\
    \
    if (handle == NULL) {\
        return MNAME##_E_INVALIDARG;\
    }\
    if (*handle == NULL) {\
        return MNAME##_E_INVALIDARG;\
    }\
    key = Gate_enterSystem ();\
    if (*handle == MNAME##_firstObject) {\
        MNAME##_firstObject = (*handle)->next;\
    }\
    else {\
        temp = MNAME##_firstObject;\
        while (temp) {\
            if (temp->next == (*handle)) {\
                temp->next = (*handle)->next;\
                break;\
            }\
            else { \
                temp = temp->next;\
            }\
        }\
        if (temp == NULL) {\
            Gate_leaveSystem (key);\
            return MNAME##_E_INVALIDARG;\
        }\
    }\
    Gate_leaveSystem (key);\
    MNAME##_Instance_finalize (*handle, (*handle)->status);\
    Memory_free (NULL, (*handle), sizeof (MNAME##_Object));\
    *handle = NULL;\
    return MNAME##_S_SUCCESS;\
}


#define IOBJECT_CREATE1(MNAME, ARG) \
static MNAME##_Object * MNAME##_firstObject = NULL;\
\
\
MNAME##_Handle MNAME##_create (ARG arg, const MNAME##_Params * params)\
{\
    IArg key;\
    MNAME##_Object * obj = (MNAME##_Object *) Memory_alloc (NULL,\
                                                    sizeof (MNAME##_Object),\
                                                    0,\
                                                    NULL);\
    if (!obj) return NULL;\
    Memory_set (obj, 0, sizeof (MNAME##_Object));\
    obj->status = MNAME##_Instance_init (obj, arg, params);\
    if (obj->status == 0) {\
        key = Gate_enterSystem ();\
        if (MNAME##_firstObject == NULL) {\
            MNAME##_firstObject = obj;\
            obj->next = NULL;\
        }\
        else {\
            obj->next = MNAME##_firstObject;\
            MNAME##_firstObject = obj;\
        }\
        Gate_leaveSystem (key);\
    }\
    else {\
        Memory_free (NULL, obj, sizeof (MNAME##_Object));\
        obj = NULL;\
    }\
    return (MNAME##_Handle)obj;\
}\
\
\
Int MNAME##_delete (MNAME##_Handle * handle)\
{\
    IArg key;\
    MNAME##_Object * temp;\
    \
    if (handle == NULL) {\
        return MNAME##_E_INVALIDARG;\
    }\
    if (*handle == NULL) {\
        return MNAME##_E_INVALIDARG;\
    }\
    key = Gate_enterSystem ();\
        if ((MNAME##_Object *)*handle == MNAME##_firstObject) {\
        MNAME##_firstObject = (*handle)->next;\
    }\
    else {\
        temp = MNAME##_firstObject;\
        while (temp) {\
            if (temp->next == (*handle)) {\
                temp->next = (*handle)->next;\
                break;\
            }\
            else { \
                temp = temp->next;\
            }\
        }\
        if (temp == NULL) {\
            Gate_leaveSystem (key);\
            return MNAME##_E_INVALIDARG;\
        }\
    }\
    Gate_leaveSystem (key);\
    MNAME##_Instance_finalize (*handle, (*handle)->status);\
    Memory_free (NULL, (*handle), sizeof (MNAME##_Object));\
    *handle = NULL;\
    return MNAME##_S_SUCCESS;\
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* ifndef __IOBJECT_H__ */
