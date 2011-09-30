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
 *  @file   MemAllocTest_config.h
 *
 *  @brief  MemAllocTest sample configuration file
 *  ============================================================================
 */

#ifndef _MEMALLOCTEST_CONFIG_H_
#define _MEMALLOCTEST_CONFIG_H_

#if defined (__cplusplus)
extern "C" {
#endif

/* App defines */
#define MAX_CREATE_ATTEMPTS         0xFFFF
#define MSGSIZE                     256
#define RCM_MSGQ_TILER_HEAPID       0
#define RCM_MSGQ_DOMX_HEAPID        1

#define RCM_SERVER_NAME_SYSM3       "MemAllocServerSysM3"
#define RCM_SERVER_NAME_APPM3       "MemAllocServerAppM3"
#define SYSM3_PROC_NAME             "SysM3"
#define APPM3_PROC_NAME             "AppM3"

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _MEMALLOCTEST_CONFIG_H_ */
