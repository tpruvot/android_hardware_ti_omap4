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
 *  @file   dehApp_config.h
 *
 *  @brief  DEH application configuration file
 *
 *  ============================================================================
 */


#ifndef _DEHAPP_CONFIG_H_
#define _DEHAPP_CONFIG_H_


#if defined (__cplusplus)
extern "C" {
#endif

/* App defines */
#define MSGSIZE                     256u
#define HEAPID                      0u
#define HEAPNAME                    "Heap0"
#define DIEMESSAGE                  0xFFFF

#define DUCATI_CORE0_MESSAGEQNAME   "MsgQ0"
#define DUCATI_CORE1_MESSAGEQNAME   "MsgQ1"
#define DSP_MESSAGEQNAME            "MsgQ2"
#define ARM_MESSAGEQNAME            "MsgQ3"

#define MAX_NUM_ARGS                4

/* Cmd */
#define DIV_BY_ZERO                 1
#define AMMU_FAULT_BY_READ_OP       2
#define IOMMU_FAULT_BY_READ_OP      3
#define AMMU_FAULT_BY_WRITE_OP      4
#define IOMMU_FAULT_BY_WRITE_OP     5
#define READ                        6
#define WRITE                       7
#define INFINITE_LOOP               8


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* _DEHAPP_CONFIG_H_ */
