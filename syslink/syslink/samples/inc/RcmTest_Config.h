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
 *  @file   RCMTest_config.h
 *
 *  @brief  RCM sample applications' configuration file
 *  ============================================================================
 */

#ifndef _RCMTEST_CONFIG_H_
#define _RCMTEST_CONFIG_H_

#if defined (__cplusplus)
extern "C" {
#endif

/* App defines */
#define MAX_CREATE_ATTEMPTS         0xFFFF
#define LOOP_COUNT                  4
#define JOB_COUNT                   4
#define MAX_NAME_LENGTH             32
#define MSGSIZE                     256
#define RCM_HEAP_SR                 1
#define RCM_MSGQ_HEAPID             0
#define RCM_MSGQ_HEAPNAME           "Heap0"
#define RCMSERVER_NAME              "RcmSvr_Mpu:0"
#define MSGQ_NAME                   "MsgQ_Mpu:0"
#define SYSM3_SERVER_NAME           "RcmSvr_SysM3:0"
#define APPM3_SERVER_NAME           "RcmSvr_AppM3:0"
#define DSP_SERVER_NAME             "RcmSvr_Dsp:0"
#define MPU_PROC_NAME               "MPU"
#define SYSM3_PROC_NAME             "SysM3"
#define APPM3_PROC_NAME             "AppM3"
#define DSP_PROC_NAME               "Tesla"

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _RCMTEST_CONFIG_H_ */
