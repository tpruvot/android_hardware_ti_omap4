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
 *  @file   gpptypes.h
 *
 *
 *  @desc   Defines the type system for DSP/BIOS Link
 *  ============================================================================
 */


#if !defined (GPPTYPES_H)
#define GPPTYPES_H


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/* Standard headers */
#include <Std.h>

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
 *  @macro  USES
 *
 *  @desc   Empty macro to indicate header file dependency
 *  ============================================================================
 */
#define USES(filename)


/** ============================================================================
 *  @macro  Data types
 *
 *  @desc   Basic data types
 *  ============================================================================
 */
typedef unsigned char       Uint8 ;     /*  8 bit value */
typedef unsigned short int  Uint16 ;    /* 16 bit value */
typedef unsigned long  int  Uint32 ;    /* 32 bit value */

typedef float               Real32 ;    /* 32 bit value */
typedef double              Real64 ;    /* 64 bit value */


typedef char                Char8 ;     /*  8 bit value */
typedef short               Char16 ;    /* 16 bit value */

typedef unsigned char       Uchar8 ;    /*  8 bit value */
typedef unsigned short      Uchar16 ;   /* 16 bit value */


//#define Void                void
typedef void *              Pvoid ;

typedef Char8 *             Pstr ;
typedef Uchar8 *            Pustr ;

/** ============================================================================
 *  @const  NULL
 *
 *  @desc   Definition is language specific
 *  ============================================================================
 */
#if !defined (NULL)

#if defined (__cplusplus)
#define NULL    0u
#else  /* defined (__cplusplus) */
#define NULL ((void *)0)
#endif /* defined (__cplusplus) */

#endif /* !defined (NULL) */


/** ============================================================================
 *  @const  NULL_CHAR
 *
 *  @desc   String terminator.
 *  ============================================================================
 */
#define NULL_CHAR '\0'


/** ============================================================================
 *  @macro  REG8/REG16/REG32
 *
 *  @desc   Macros to access register fields.
 *  ============================================================================
 */
#define REG8(A)         (*(volatile Char8  *) (A))
#define REG16(A)        (*(volatile Uint16 *) (A))
#define REG32(A)        (*(volatile Uint32 *) (A))


/** ============================================================================
 *  @macro  DSP/BIOS Link specific types
 *
 *  @desc   These types are used across DSP/BIOS Link.
 *  ============================================================================
 */
typedef UInt32     ProcessorId ;
typedef UInt32     ChannelId ;


/** ============================================================================
 *  @name   PoolId
 *
 *  @desc   This type is used for identifying the different pools used by
 *          DSPLINK.
 *  ============================================================================
 */
typedef UInt16     PoolId ;


/** ============================================================================
 *  @macro  OS Specific standard definitions
 *
 *  @desc   Free for OEMs to add their own generic stuff, if they so desire
 *  ============================================================================
 */
#if defined (OS_WINCE)

#endif  /* defined (OS_WINCE) */


#if defined (OS_NUCLEUS)

#endif  /* defined (OS_NUCLEUS) */


#if defined (OS_LINUX)

#endif  /* defined (OS_LINUX) */


/** ============================================================================
 *  @macro  Calling convention
 *
 *  @desc   Definition of CDECL, DLLIMPORT, DLLEXPORT can be defined by
 *          OEM for his compiler
 *  ============================================================================
 */
#define STATIC          static
#define EXTERN          extern


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

/* Derived calling conventions */
#define NORMAL_API      CDECL
#define IMPORT_API      DLLIMPORT
#define EXPORT_API      DLLEXPORT


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif  /* !defined (GPPTYPES_H) */
