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
 *  @file   ipctypes.h
 *
 *
 *  @desc   Defines data types for IPC
 *
 *  ============================================================================
 */


#if !defined (IPCTYPES_H)
#define IPCTYPES_H


/*  ----------------------------------- Common */
/* TBD: Include common types header file. */


#if defined (__cplusplus)
EXTERN "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @macro  IN/OUT/OPTIONAL/CONST
 *
 *  @desc   Argument specification syntax
 *  ============================================================================
 */
#define IN                              /* The argument is INPUT  only */
#define OUT                             /* The argument is OUTPUT only */
#define OPT                             /* The argument is OPTIONAL    */
#define CONST   const


/** ============================================================================
 *  @name   Notify_Handle
 *
 *  @desc   This typedef defines the type for the handle to the Notify driver.
 *  ============================================================================
 */
typedef Uint32     Processor_Id ;


/** ============================================================================
 *  @macro  Calling convention
 *
 *  @desc   Definition of CDECL, DLLIMPORT, DLLEXPORT can be defined by
 *          OEM for his compiler
 *  ============================================================================
 */
#if defined (OS_WINCE)
/*  ------------------------------------------- WINCE               */
#define CDECL           __cdecl
#define DLLIMPORT       __declspec (dllexport)
#define DLLEXPORT       __declspec (dllexport)
/*  ------------------------------------------- WINCE               */
#endif  /* defined (OS_WINCE) */


#if defined (OS_NUCLEUS)
/*  ------------------------------------------- NUCLEUS             */
#define CDECL
#define DLLIMPORT
#define DLLEXPORT
/*  ------------------------------------------- NUCLEUS             */
#endif  /* defined (OS_NUCLEUS) */

#if defined (OS_LINUX)
/*  ------------------------------------------- LINUX               */
#define CDECL
#define DLLIMPORT
#define DLLEXPORT
/*  ------------------------------------------- LINUX               */
#endif  /* defined (OS_LINUX) */


#if defined (OS_PROS)
/*  ------------------------------------------- PROS                */
#define CDECL
#define DLLIMPORT
#define DLLEXPORT
/*  ------------------------------------------- PROS                */
#endif  /* defined (OS_PROS) */


#if defined (OS_DSPBIOS)
/*  ------------------------------------------- PROS                */
#define CDECL
#define DLLIMPORT
#define DLLEXPORT
/*  ------------------------------------------- PROS                */
#endif  /* defined (OS_DSPBIOS) */

/* Derived calling conventions */
#define NORMAL_API      CDECL
#define IMPORT_API      DLLIMPORT
#define EXPORT_API      DLLEXPORT


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (IPCTYPES_H) */
