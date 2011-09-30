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
 *  @file   HeapBufApp.h
 *
 *  @brief      Sample application for HeapBuf module
 *
 *  ============================================================================
 */


/* Standard headers */
#include <Std.h>

/*!
 *  @brief  Function to execute the startup for HeapBufApp sample application
 *
 *  @sa
 */
Int HeapBufApp_startup (Void);
/*!
 *  @brief  Function to execute the HeapBufApp sample application
 *
 *  @sa     HeapBufApp_callback
 */
Int HeapBufApp_execute (Void);

/*!
 *  @brief  Function to execute the shutdown for HeapBufApp sample application
 *
 *  @sa     HeapBufApp_callback
 */
Int HeapBufApp_shutdown (Void);


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
