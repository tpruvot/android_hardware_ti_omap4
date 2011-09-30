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
 *  @file   trc.h
 *
 *  @desc   Defines the interfaces and data structures for the
 *          sub-component TRC.
 *  ============================================================================
 */


#if !defined (TRC_H)
#define TRC_H


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  MAXIMUM_COMPONENTS
 *
 *  @desc   maximum number of components supported.
 *  ============================================================================
 */
#define MAXIMUM_COMPONENTS         16u

/** ============================================================================
 *  @const  TRC_ENTER/TRC_LEVELn/TRC_LEAVE
 *
 *  @desc   Severity levels for debug printing.
 *  ============================================================================
 */
#define TRC_ENTER           0x01u       /*  Lowest level of severity */
#define TRC_LEVEL1          0x02u
#define TRC_LEVEL2          0x03u
#define TRC_LEVEL3          0x04u
#define TRC_LEVEL4          0x05u
#define TRC_LEVEL5          0x06u
#define TRC_LEVEL6          0x07u
#define TRC_LEVEL7          0x08u      /*  Highest level of severity */
#define TRC_LEAVE           TRC_ENTER


/** ============================================================================
 *  @name   ErrorInfo
 *
 *  @desc   Structure for storing error reason.
 *
 *  @field  IsSet
 *              Flag to indicate error is set.
 *  @field  ErrCode
 *              Error Code.
 *  @field  OsMajor
 *              OS  Version Major version number.
 *  @field  OsMinor
 *              OS  Version Minor version number.
 *  @field  OsBuild
 *              OS  Version Build number.
 *  @field  PddMajor
 *              PDD Version Major version number.
 *  @field  PddMinor
 *              PDD Version Minor version number.
 *  @field  PddBuild
 *              PDD Version Build number.
 *  @field  FileId
 *              ID of the file where failure occured.
 *  @field  LineNum
 *              Line number where failure occured.
 *  ============================================================================
 */
typedef struct ErrorInfo_tag {
    Bool       IsSet    ;

    DSP_STATUS ErrCode  ;

    Int32      OsMajor  ;
    Int32      OsMinor  ;
    Int32      OsBuild  ;

    Int32      PddMajor ;
    Int32      PddMinor ;
    Int32      PddBuild ;

    Int32      FileId   ;
    Int32      LineNum  ;
} ErrorInfo ;


/** ============================================================================
 *  @func   TRC_SetReason
 *
 *  @desc   This function logs failure if no previous failure has been logged.
 *
 *  @arg    status
 *              Error status to be logged.
 *  @arg    FileId
 *              File identifier.
 *  @arg    Line
 *              Line number where error occurred.
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ============================================================================
 */

void
TRC_SetReason (DSP_STATUS status, Int32 FileId, Int32 Line) ;

#if defined (TRACE_ENABLE)

#if defined (TRACE_KERNEL)

/** ============================================================================
 *  @macro  TRC_ENABLE
 *
 *  @desc   Wrapper for function TRC_Enable ().
 *  ============================================================================
 */
#define TRC_ENABLE(map)             TRC_enable (map)

/** ============================================================================
 *  @macro  TRC_DISABLE
 *
 *  @desc   Wrapper for function TRC_Disable ().
 *  ============================================================================
 */
#define TRC_DISABLE(map)            TRC_disable (map)

/** ============================================================================
 *  @macro  TRC_SET_SEVERITY
 *
 *  @desc   Wrapper for function TRC_SetSeverity ().
 *  ============================================================================
 */
#define TRC_SET_SEVERITY(level)     TRC_setSeverity (level)

/** ============================================================================
 *  @macro  TRC_nPRINT
 *
 *  @desc   Uses corresponding TRC_nPrint function to print debug strings and
 *          optional arguments.
 *  ============================================================================
 */

#define TRC_0PRINT(a,b)                                           \
    TRC_0print (COMPONENT_ID, (a), (Char8 *)(b))

#define TRC_1PRINT(a,b,c)                                         \
    TRC_1print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c))

#define TRC_2PRINT(a,b,c,d)                                       \
    TRC_2print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c),   \
                                              (Uint32) (d))

#define TRC_3PRINT(a,b,c,d,e)                                     \
    TRC_3print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c),   \
                                              (Uint32) (d),       \
                                              (Uint32) (e))

#define TRC_4PRINT(a,b,c,d,e,f)                                   \
    TRC_4print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c),   \
                                              (Uint32) (d),       \
                                              (Uint32) (e),       \
                                              (Uint32) (f))

#define TRC_5PRINT(a,b,c,d,e,f,g)                                 \
    TRC_5print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c),   \
                                              (Uint32) (d),       \
                                              (Uint32) (e),       \
                                              (Uint32) (f),       \
                                              (Uint32) (g))

#define TRC_6PRINT(a,b,c,d,e,f,g,h)                               \
    TRC_6print (COMPONENT_ID, (a), (Char8 *) (b), (Uint32) (c),   \
                                              (Uint32) (d),       \
                                              (Uint32) (e),       \
                                              (Uint32) (f),       \
                                              (Uint32) (g),       \
                                              (Uint32) (h))

/** ============================================================================
 *  @name   TrcObject
 *
 *  @desc   TRC Object that stores the severity and component and
 *          subcomponent maps on a global level.
 *
 *  @field  components
 *              component map
 *  @field  level
 *              severity level
 *  @field  subcomponents
 *              subcomponent map
 *  ============================================================================
 */
typedef struct TrcObject_tag {
    Uint16 components ;
    Uint16 level      ;
    Uint16 subcomponents [MAXIMUM_COMPONENTS] ;
} TrcObject ;

/** ============================================================================
 *  @func   TRC_enable
 *
 *  @desc   Enables debug prints on a component and subcomponent level.
 *
 *  @arg    componentMap
 *             The component & subcomponent map
 *
 *  @ret    DSP_SOK
 *              Operation successful
 *          DSP_EINVALIDARG
 *              Invalid argument to function call
 *          DSP_EFAIL
 *              Operation not successful
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    TRC_Disable, TRC_SetSeverity
 *  ============================================================================
 */

DSP_STATUS
TRC_enable (IN Uint32 componentMap);


/** ============================================================================
 *  @func   TRC_disable
 *
 *  @desc   Disables debug prints on a component and subcomponent level.
 *
 *  @arg    componentMap
 *             The component & subcomponent map
 *
 *  @ret    DSP_SOK
 *              Operation successful
 *          DSP_EINVALIDARG
 *              Invalid argument to function call
 *          DSP_EFAIL
 *              Operation not successful
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    TRC_Enable, TRC_SetSeverity
 *  ============================================================================
 */

DSP_STATUS
TRC_disable (IN Uint32 componentMap);


/** ============================================================================
 *  @func   TRC_setSeverity
 *
 *  @desc   set the severity of the required debug prints.
 *
 *  @arg    level
 *             The severity level of the debug prints required
 *
 *  @ret    DSP_SOK
 *              Operation successful
 *          DSP_EINVALIDARG
 *              Invalid argument to function call
 *          DSP_EFAIL
 *              Operation not successful
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    TRC_Enable, TRC_Disable
 *  ============================================================================
 */

DSP_STATUS
TRC_setSeverity (IN Uint16   level) ;

/** ============================================================================
 *  @func   TRC_0print
 *
 *  @desc   Prints a null terminated character string based on its severity,
 *          the subcomponent and component it is associated with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_1Print, TRC_2Print, TRC_3Print, TRC_4Print, TRC_5Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_0print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString) ;


/** ============================================================================
 *  @func   TRC_1print
 *
 *  @desc   Prints a null terminated character string and an integer argument
 *          based on its severity, the subcomponent and component it is
 *          associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_2Print, TRC_3Print, TRC_4Print, TRC_5Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_1print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1) ;


/** ============================================================================
 *  @func   TRC_2print
 *
 *  @desc   Prints a null terminated character string and two integer arguments
 *          based on its severity, the subcomponent and component it is
 *          associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The first integer argument to be printed
 *  @arg    argument2
 *             The second integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_1Print, TRC_3Print, TRC_4Print, TRC_5Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_2print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1,
            IN  Uint32   argument2) ;


/** ============================================================================
 *  @func   TRC_3print
 *
 *  @desc   Prints a null terminated character string and three integer
 *          arguments based on its severity, the subcomponent and component it
 *          is associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The first integer argument to be printed
 *  @arg    argument2
 *             The second integer argument to be printed
 *  @arg    argument3
 *             The third integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_1Print, TRC_2Print, TRC_4Print, TRC_5Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_3print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1,
            IN  Uint32   argument2,
            IN  Uint32   argument3) ;


/** ============================================================================
 *  @func   TRC_4print
 *
 *  @desc   Prints a null terminated character string and four integer
 *          arguments based on its severity, the subcomponent and component it
 *          is associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The first integer argument to be printed
 *  @arg    argument2
 *             The second integer argument to be printed
 *  @arg    argument3
 *             The third integer argument to be printed
 *  @arg    argument4
 *             The fourth integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_1Print, TRC_2Print, TRC_3Print, TRC_5Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_4print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1,
            IN  Uint32   argument2,
            IN  Uint32   argument3,
            IN  Uint32   argument4) ;


/** ============================================================================
 *  @func   TRC_5print
 *
 *  @desc   Prints a null terminated character string and five integer
 *          arguments based on its severity, the subcomponent and component it
 *          is associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The first integer argument to be printed
 *  @arg    argument2
 *             The second integer argument to be printed
 *  @arg    argument3
 *             The third integer argument to be printed
 *  @arg    argument4
 *             The fourth integer argument to be printed
 *  @arg    argument5
 *             The fifth integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_1Print, TRC_2Print, TRC_3Print, TRC_4Print,
 *          TRC_6Print
 *  ============================================================================
 */

void
TRC_5print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1,
            IN  Uint32   argument2,
            IN  Uint32   argument3,
            IN  Uint32   argument4,
            IN  Uint32   argument5) ;


/** ============================================================================
 *  @func   TRC_6print
 *
 *  @desc   Prints a null terminated character string and six integer
 *          arguments based on its severity, the subcomponent and component it
 *          is associated  with.
 *
 *  @arg    componentMap
 *             The component & subcomponent to which this print belongs
 *  @arg    severity
 *             The severity associated with the print
 *  @arg    debugString
 *             The null terminated character string to be printed
 *  @arg    argument1
 *             The first integer argument to be printed
 *  @arg    argument2
 *             The second integer argument to be printed
 *  @arg    argument3
 *             The third integer argument to be printed
 *  @arg    argument4
 *             The fourth integer argument to be printed
 *  @arg    argument5
 *             The fifth integer argument to be printed
 *  @arg    argument6
 *             The sixth integer argument to be printed
 *
 *  @ret    None
 *
 *  @enter  The character string is valid
 *
 *  @leave  None
 *
 *  @see    TRC_0Print, TRC_1Print, TRC_2Print, TRC_3Print, TRC_4Print,
 *          TRC_5Print.
 *  ============================================================================
 */

void
TRC_6print (IN  Uint32   componentMap,
            IN  Uint16   severity,
            IN  Char8 *  debugString,
            IN  Uint32   argument1,
            IN  Uint32   argument2,
            IN  Uint32   argument3,
            IN  Uint32   argument4,
            IN  Uint32   argument5,
            IN  Uint32   argument6) ;

#else /* defined (TRACE_KERNEL) */

#define TRC_ENABLE(map)
#define TRC_DISABLE(map)
#define TRC_SET_SEVERITY(level)

#define TRC_0PRINT(a,b)                 \
    PRINT_Printf (b)

#define TRC_1PRINT(a,b,c)               \
    PRINT_Printf ((b), (int)(c))

#define TRC_2PRINT(a,b,c,d)             \
    PRINT_Printf ((b), (int)(c),        \
                       (int)(d))

#define TRC_3PRINT(a,b,c,d,e)           \
    PRINT_Printf ((b),(int)(c),         \
                      (int)(d),         \
                       (int)(e))

#define TRC_4PRINT(a,b,c,d,e,f)         \
    PRINT_Printf ((b), (int) (c),       \
                       (int) (d),       \
                       (int) (e),       \
                       (int) (f))

#define TRC_5PRINT(a,b,c,d,e,f,g)       \
    PRINT_Printf ((b), (int) (c),       \
                       (int) (d),       \
                       (int) (e),       \
                       (int) (f),       \
                       (int) (g))

#define TRC_6PRINT(a,b,c,d,e,f,g,h)     \
    PRINT_Printf ((b), (int) (c),       \
                       (int) (d),       \
                       (int) (e),       \
                       (int) (f),       \
                       (int) (g),       \
                       (int) (h))
#endif /* defined(TRACE_KERNEL) */

#define TRC_0ENTER(str)                     \
    TRC_0PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n")

#define TRC_1ENTER(str,a)                   \
    TRC_1PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n",         \
                a)

#define TRC_2ENTER(str,a,b)                 \
    TRC_2PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n"          \
                "\t"#b"\t[0x%x]\n",         \
                a,b)

#define TRC_3ENTER(str,a,b,c)               \
    TRC_3PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n"          \
                "\t"#b"\t[0x%x]\n"          \
                "\t"#c"\t[0x%x]\n",         \
                a,b,c)

#define TRC_4ENTER(str,a,b,c,d)             \
    TRC_4PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n"          \
                "\t"#b"\t[0x%x]\n"          \
                "\t"#c"\t[0x%x]\n"          \
                "\t"#d"\t[0x%x]\n",         \
                a,b,c,d)

#define TRC_5ENTER(str,a,b,c,d,e)           \
    TRC_5PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n"          \
                "\t"#b"\t[0x%x]\n"          \
                "\t"#c"\t[0x%x]\n"          \
                "\t"#d"\t[0x%x]\n"          \
                "\t"#e"\t[0x%x]\n",         \
                a,b,c,d,e)

#define TRC_6ENTER(str,a,b,c,d,e,f)         \
    TRC_6PRINT (TRC_ENTER,                  \
                "Entered " str " ()\n"    \
                "\t"#a"\t[0x%x]\n"          \
                "\t"#b"\t[0x%x]\n"          \
                "\t"#c"\t[0x%x]\n"          \
                "\t"#d"\t[0x%x]\n"          \
                "\t"#e"\t[0x%x]\n"          \
                "\t"#f"\t[0x%x]\n",         \
                a,b,c,d,e,f)

#define TRC_0LEAVE(str)                     \
    TRC_0PRINT (TRC_LEAVE,                  \
                "Leaving " str " ()\n")

#define TRC_1LEAVE(str,status)                                  \
    TRC_1PRINT (TRC_LEAVE,                                      \
                "Leaving " str " () \t"#status" [0x%x]\n",    \
                status)


#else  /* defined (TRACE_ENABLE) */

#define TRC_ENABLE(map)
#define TRC_DISABLE(map)
#define TRC_SET_SEVERITY(level)

#define TRC_0PRINT(a,b)
#define TRC_1PRINT(a,b,c)
#define TRC_2PRINT(a,b,c,d)
#define TRC_3PRINT(a,b,c,d,e)
#define TRC_4PRINT(a,b,c,d,e,f)
#define TRC_5PRINT(a,b,c,d,e,f,g)
#define TRC_6PRINT(a,b,c,d,e,f,g,h)

#define TRC_0ENTER(str)
#define TRC_1ENTER(str,a)
#define TRC_2ENTER(str,a,b)
#define TRC_3ENTER(str,a,b,c)
#define TRC_4ENTER(str,a,b,c,d)
#define TRC_5ENTER(str,a,b,c,d,e)
#define TRC_6ENTER(str,a,b,c,d,e,f)

#define TRC_0LEAVE(str)
#define TRC_1LEAVE(str,status)


#endif  /* defined (TRACE_ENABLE) */

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* !defined (TRC_H) */
