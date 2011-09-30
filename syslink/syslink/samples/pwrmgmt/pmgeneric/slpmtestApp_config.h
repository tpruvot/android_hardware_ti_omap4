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
 *  @file   SLPMTESTApp_config.h
 *
 *  @brief  slpm application configuration file
 *
 *  ============================================================================
 */


#ifndef _SLPMTESTAPP_CONFIG_H_
#define _SLPMTESTAPP_CONFIG_H_


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


#define NOVALID                                   -1

#define NOCMD                                      0
#define REQUEST                                  0x1
#define REGISTER                                 0x2
#define REQUEST_REGISTER                         0x3
#define VALIDATE                                 0x4
#define REQUEST_VALIDATE                         0x5
#define REGISTER_VALIDATE                        0x6
#define REQUEST_REGISTER_VALIDATE                0x7
#define DUMPRCB                                  0x8
#define SUSPEND                                 0x10
#define RESUME                                  0x20
#define UNREGISTER                              0x40
#define RELEASE                                 0x80
#define UNREGISTER_RELEASE                      0xC0

#define MULTITHREADS                           0x100
#define LIST                                   0x200
#define ENTER_I2C_SPINLOCK                     0x400
#define LEAVING_I2C_SPINLOCK                   0x800

#define MULTITHREADS_I2C                      0x1000
#define MULTI_TASK_I2C                        0x2000
#define NEWFEATURE                            0x4000
/* Paul's Tests */
#define IDLEWFION                             0x8000
#define IDLEWFIOFF                           0x10000
#define COUNTIDLES                           0x20000
#define PAUL_01                              0x40000
#define PAUL_02                              0x80000
#define PAUL_03                             0x100000
#define PAUL_04                             0x200000
#define TIMER                               0x400000
#define PWR_SUSPEND                         0x800000

#define NOTIFY_SYSM3                       0x1000000
#define ENABLE_HIBERNATION                 0x2000000
#define SET_CONSTRAINTS                    0x4000000

#define EXIT                              0x80000000

#define REQUIRES_MESSAGEQ (REQUEST | REGISTER | VALIDATE | DUMPRCB | UNREGISTER \
    | RELEASE | MULTITHREADS | LIST | NEWFEATURE |ENTER_I2C_SPINLOCK \
    | LEAVING_I2C_SPINLOCK | MULTITHREADS_I2C | MULTI_TASK_I2C \
    | IDLEWFION | IDLEWFIOFF | COUNTIDLES | PAUL_01 | PAUL_02 | PAUL_03 \
    | PAUL_04 | TIMER | PWR_SUSPEND | ENABLE_HIBERNATION | SET_CONSTRAINTS)

#define EXIT_OR_NOVALID (~(SUSPEND | RESUME | REQUIRES_MESSAGEQ))

#define REQUIRES_NOTIFY_SYSM3 NOTIFY_SYSM3


#define FDIF            0
#define IPU             1
#define SYSM3           2
#define APPM3           3
#define ISS             4
#define IVA_HD          5
#define IVASEQ0         6
#define IVASEQ1         7
#define L3_BUS          8
#define MPU             9
#define SDMA            10
#define GP_TIMER        11
#define GP_IO           12
#define I2C             13
#define REGULATOR       14
#define AUX_CLK         15

#define NORESOURCE      -1

#define FIRSTRESOURCE   FDIF
#define LASTRESOURCE    AUX_CLK

#define MAX_NUM_ARGS    10

#define LAST_CONSTR_RES MPU


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* _SLPMTESTAPP_CONFIG_H_ */
