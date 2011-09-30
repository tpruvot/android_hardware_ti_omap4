/*
 *  Copyright 2008-2011 Texas Instruments - http://www.ti.com/
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
 *  @file   TilerSysLinkApp.h
 *
 *
 *
 *  ============================================================================
 */


#ifndef TILERSYSLINKTESTAPP_0xf2ba
#define TILERSYSLINKTESTAPP_0xf2ba



#if defined (__cplusplus)
extern "C" {
#endif

/* =============================================================================
 * Structures & Enums
 * =============================================================================
 */


/* =============================================================================
 *  APIs
 * =============================================================================
 */

/*!
 *  @brief  Function to Test use buffer functionality using Tiler and Syslink
 *          IPC
 */
Int SyslinkUseBufferTest(Int, Bool, UInt);

/*!
 *  @brief  Function to Test Physical to Virtual address translation for tiler
 *          address
 */
Int SyslinkVirtToPhysTest(void);

/*!
 *  @brief  Function to Test Physical to Virtual pages address translation for
  *         tiler address
 */
Int SyslinkVirtToPhysPagesTest(void);

/*!
 *  @brief  Function to Test repeated mapping and unmapping of addresses to
 *          Ducati virtual space
 */
Int SyslinkMapUnMapTest(UInt);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif
