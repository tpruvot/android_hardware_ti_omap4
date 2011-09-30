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
 *  @file   _signature.h
 *
 *  @desc   Defines the file and object signatures used in DSP/BIOS Link
 *          These signatures are used in object validation and error reporting.
 *
 *  ============================================================================
 */


#if !defined (_SIGNATURE_H)
#define _SIGNATURE_H


/*  ----------------------------------- DSP/BIOS Link               */
#include <gpptypes.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/*  ============================================================================
 *  @const  File identifiers
 *
 *  @desc   File identifiers used in TRC_SetReason ()
 *  ============================================================================
 */

/*  ============================================================================
 *  IPCBASE File identifiers
 *  ============================================================================
 */
#define FID_BASE_IPCBASE        0x0200u

 /*  GEN File identifiers */
#define FID_BASE_GEN            (FID_BASE_IPCBASE + 0x0000u)
#define FID_C_GEN_UTILS         (FID_BASE_GEN  + 0u)
#define FID_C_GEN_LIST          (FID_BASE_GEN  + 1u)

/*  OSAL File identifiers */
#define FID_BASE_OSAL           (FID_BASE_IPCBASE + 0x0020u)
#define FID_C_OSAL              (FID_BASE_OSAL + 0x0u)
#define FID_C_OSAL_DPC          (FID_BASE_OSAL + 0x1u)
#define FID_C_OSAL_ISR          (FID_BASE_OSAL + 0x2u)
#define FID_C_OSAL_MEM          (FID_BASE_OSAL + 0x4u)
#define FID_C_OSAL_SYNC         (FID_BASE_OSAL + 0x6u)
#define FID_C_OSAL_TRC          (FID_BASE_OSAL + 0x7u)
#define FID_C_OSAL_PRINT        (FID_BASE_OSAL + 0x9u)

/*  DRV File identifiers */
#define FID_BASE_IPCBASE_DRV    (FID_BASE_IPCBASE + 0x0040u)
#define FID_C_DRV_IPCBASE       (FID_BASE_IPCBASE_DRV  + 0u)


/*  ============================================================================
 *  Notify File identifiers
 *  ============================================================================
 */
#define FID_BASE_NOTIFY         0x0300u

 /*  Notify File identifiers */
#define FID_C_KNL_NOTIFY        (FID_BASE_NOTIFY  + 0u)
#define FID_C_KNL_NOTIFY_DRIVER (FID_BASE_NOTIFY  + 1u)
#define FID_C_DRV_IPCNOTIFY     (FID_BASE_NOTIFY  + 2u)


/*  ============================================================================
 *  Drivers File identifiers
 *  ============================================================================
 */
#define FID_BASE_DRIVERS        0x0400u

 /*  NotifyShmDrv File identifiers */
#define FID_C_BASE_NOTIFYSHMDRV    (FID_BASE_DRIVERS  + 0x0000u)
#define FID_C_KNL_NOTIFY_SHMDRIVER (FID_C_BASE_NOTIFYSHMDRV  + 0u)
#define FID_C_DRV_NOTIFYSHMDRV     (FID_C_BASE_NOTIFYSHMDRV  + 1u)

/* HAL */
#define FID_BASE_HAL               (FID_BASE_DRIVERS  + 0x0020u)
#define FID_C_HAL_INTERRUPT        (FID_BASE_HAL + 0xEu)


/*  ============================================================================
 *  Sample applications File identifiers
 *  ============================================================================
 */
#define FID_BASE_SAMPLES        0x0500u

 /*  Notify sample applications File identifiers */
#define FID_BASE_SAMPLES_NOTIFY (FID_BASE_SAMPLES + 0x0000u)
#define FID_C_KNL_NOTIFYXFER    (FID_BASE_SAMPLES_NOTIFY  + 0u)


/*  ============================================================================
 *  @const  Object signatures
 *
 *  @desc   Object signatures used to validate objects.
 *  ============================================================================
 */
#define SIGN_NULL                0x00000000u      /* NULL signature */

/*  OS Adaptation Layer          hex value       String (in reverse) */
#define SIGN_TRC                 0x5F435254u      /* TRC_ */
#define SIGN_DPC                 0x5F435044u      /* DPC_ */
#define SIGN_ISR                 0x5F525349u      /* ISR_ */
#define SIGN_KFILE               0x4C49464Bu      /* KFIL */
#define SIGN_MEM                 0x5F504550u      /* MEM_ */
#define SIGN_PRCS                0x53435250u      /* PRCS */
#define SIGN_SYNC                0x434E5953u      /* SYNC */
#define SIGN_DRV                 0x5F4B5244u      /* DRV_ */

/*  Generic components           hex value       String (in reverse) */
#define SIGN_GEN                 0x5F4E4547u      /* GEN_ */
#define SIGN_LIST                0x5453494Cu      /* LIST */

/* HAL                           hex value       String (in reverse) */
#define SIGN_HAL                 0x484C414Eu      /* HAL_ */


/*  ============================================================================
 *  @const  MAXIMUM_COMPONENTS
 *
 *  @desc   Maximum number of components
 *  ============================================================================
 */
#define MAXIMUM_COMPONENTS         16u

/*  ============================================================================
 *  @const  Component IDs
 *
 *  @desc   Component Identifiers. These must match corresponding bit
 *          position in component map
 *  ============================================================================
 */
#define ID_GEN_BASE             0x00010000u
#define ID_OSAL_BASE            0x00020000u
#define ID_IPCBASEDRV_BASE      0x00040000u
#define ID_KNL_NOTIFY_BASE      0x00080000u
#define ID_NOTIFYSHMDRV_BASE    0x00100000u
#define ID_HAL_BASE             0x00200000u
#define ID_SAMPLES              0x00400000u
#define ID_USR                  0x80000000u

#define MIN_COMPONENT  ID_GEN_BASE    >> 16u
#define MAX_COMPONENT  ID_SAMPLES     >> 16u


/*  ============================================================================
 *  GEN subcomponent map
 *  ============================================================================
 */
#define ID_GEN_UTILS        ID_GEN_BASE | 0x0001u
#define ID_GEN_LIST         ID_GEN_BASE | 0x0002u
#define ID_GEN_ALL          ID_GEN_UTILS | ID_GEN_LIST

/*  ============================================================================
 *  OSAL Subcomponent map
 *  ============================================================================
 */
#define ID_OSAL            ID_OSAL_BASE | 0x0001u
#define ID_OSAL_DPC        ID_OSAL_BASE | 0x0002u
#define ID_OSAL_ISR        ID_OSAL_BASE | 0x0004u
#define ID_OSAL_KFILE      ID_OSAL_BASE | 0x0008u
#define ID_OSAL_MEM        ID_OSAL_BASE | 0x0010u
#define ID_OSAL_PRCS       ID_OSAL_BASE | 0x0020u
#define ID_OSAL_SYNC       ID_OSAL_BASE | 0x0040u
#define ID_OSAL_TRC        ID_OSAL_BASE | 0x0080u
#define ID_OSAL_PRINT      ID_OSAL_BASE | 0x0100u
#define ID_OSAL_ALL        ID_OSAL      | ID_OSAL_DPC  | ID_OSAL_ISR      \
                         | ID_OSAL_KFILE| ID_OSAL_MEM  | ID_OSAL_PRCS     \
                         | ID_OSAL_SYNC | ID_OSAL_TRC  | ID_OSAL_PRINT

/*  ============================================================================
 *  IPCBASEDRV Subcomponent map
 *  ============================================================================
 */
#define ID_DRV_IPCBASE     ID_IPCBASEDRV_BASE | 0x0001u
#define ID_IPCBASEDRV_ALL  ID_DRV_IPCBASE

/*  ============================================================================
 *  Notify KNL Subcomponent map
 *  ============================================================================
 */
#define ID_KNL_NOTIFY        ID_KNL_NOTIFY_BASE | 0x0001u
#define ID_KNL_NOTIFY_DRIVER ID_KNL_NOTIFY_BASE | 0x0002u
#define ID_DRV_IPCNOTIFY     ID_KNL_NOTIFY_BASE | 0x0004u
#define ID_KNL_NOTIFY_ALL    ID_KNL_NOTIFY      | ID_KNL_NOTIFY_DRIVER    \
                           | ID_DRV_IPCNOTIFY

/*  ============================================================================
 *  NotifyShmDrv KNL Subcomponent map
 *  ============================================================================
 */
#define ID_KNL_NOTIFY_SHMDRIVER ID_NOTIFYSHMDRV_BASE | 0x0001u
#define ID_DRV_NOTIFYSHMDRV     ID_NOTIFYSHMDRV_BASE | 0x0002u
#define ID_NOTIFYSHMDRV_ALL     ID_KNL_NOTIFY_SHMDRIVER  | ID_DRV_NOTIFYSHMDRV

/*  ============================================================================
 *  HAL Subcomponent map
 *  ============================================================================
 */
#define ID_HAL_INTERRUPT    ID_HAL_BASE | 0x0001u
#define ID_HAL_ALL          ID_HAL_INTERRUPT

/*  ============================================================================
 *  Samples Subcomponent map
 *  ============================================================================
 */
#define ID_KNL_NOTIFYXFER   ID_SAMPLES | 0x0001u
#define ID_SAMPLES_ALL      ID_KNL_NOTIFYXFER

/*  ============================================================================
 *  Samples Subcomponent map
 *  ============================================================================
 */
#define ID_USR_NOTIFY       (ID_USR | 0x0001u)
#define ID_USR_DRVNOTIFY    (ID_USR | 0x0002u)
#define ID_USR__NOTIFY      (ID_USR | 0x0003u)

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (_SIGNATURE_H) */
