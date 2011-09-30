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
 *  @file   HeapBufApp_config.h
 *
 *  @brief  HeapBuf application configuration file
 *  ============================================================================
 */


#ifndef _HEAPBUFAPP_CONFIG_H_
#define _HEAPBUFAPP_CONFIG_H_

#if defined (__cplusplus)
extern "C" {
#endif

/* App defines */
#define MSGSIZE                     256u
#define NSRN_NOTIFY_EVENTNO         7u
#define TRANSPORT_NOTIFY_EVENTNO    8u
#define HEAPID                      0u
#define HEAPNAME                    "myHeap"
#define DIEMESSAGE                  100

#define CORE0_MESSAGEQNAME          "Q0"
#define CORE1_MESSAGEQNAME          "Q1"

/*
 *  The shared memory is going to split between
 *  Notify:       0xA0000000 - 0xA0004000
 *  Gatepeterson: 0xA0004000 - 0xA0005000
 *  HeapMultiBuf: 0xA0005000 - 0xA0009000
 *  transport:    0xA0009000 - 0xA000B000
 *  NameServer:   0xA000B000 - 0xA000C000
 *  HeapBuf:      0xA000C000 - 0xA000E000
 *  List:         0xA000E000 - 0xA0010000
 */

#define SHAREDMEM               0xA0000000
#define SHAREDMEMSIZE           0x1B000

/* Memory for the Notify Module */
#define NOTIFYMEM               (SHAREDMEM)
#define NOTIFYMEMSIZE           0x4000

/* Memory a GatePeterson instance */
#define GATEPETERSONMEM         (NOTIFYMEM + NOTIFYMEMSIZE)
#define GATEPETERSONMEMSIZE     0x1000

/* Memory a HeapBuf instance MPU-DUCATI*/
#define HEAPBUFMEM              (GATEPETERSONMEM + GATEPETERSONMEMSIZE)
#define HEAPBUFMEMSIZE          0x4000

/* Memory for NameServerRemoteNotify */
#define NSRN_MEM                (HEAPBUFMEM + HEAPBUFMEMSIZE)
#define NSRN_MEMSIZE            0x1000

/* Memory a Transport instance */
#define TRANSPORTMEM            (NSRN_MEM + NSRN_MEMSIZE)
#define TRANSPORTMEMSIZE        0x2000

/* Memory for MessageQ's NameServer instance */
#define MESSAGEQ_NS_MEM         (TRANSPORTMEM + TRANSPORTMEMSIZE)
#define MESSAGEQ_NS_MEMSIZE     0x1000

/* Memory for HeapBuf's NameServer instance */
#define HEAPBUF_NS_MEM          (MESSAGEQ_NS_MEM + MESSAGEQ_NS_MEMSIZE)
#define HEAPBUF_NS_MEMSIZE      0x1000

#define GATEPETERSONMEM1        (HEAPBUF_NS_MEM + HEAPBUF_NS_MEMSIZE)
#define GATEPETERSONMEMSIZE1    0x1000

/* Memory for the Notify Module */
#define HEAPMEM                 (GATEPETERSONMEM1 + GATEPETERSONMEMSIZE1)
#define HEAPMEMSIZE             0x1000

#define HEAPMEM1                (HEAPMEM + HEAPMEMSIZE)
#define HEAPMEMSIZE1            0x1000

#define List                    (HEAPMEM1 + HEAPMEMSIZE1)
#define ListSIZE                0x1000

#define List1                   (List + ListSIZE)
#define ListSIZE1               0x1000

/* Memory for HeapMultiBuf instance */
#define HEAPMBMEM_CTRL          (List1 + ListSIZE1)
#define HEAPMBMEMSIZE_CTRL      0x1000

#define HEAPMBMEM_BUFS          (HEAPMBMEM_CTRL + HEAPMBMEMSIZE_CTRL)
#define HEAPMBMEMSIZE_BUFS      0x3000

#define HEAPMBMEM1_CTRL         (HEAPMBMEM_BUFS + HEAPMBMEMSIZE_BUFS)
#define HEAPMBMEMSIZE1_CTRL     0x1000

#define HEAPMBMEM1_BUFS         (HEAPMBMEM1_CTRL + HEAPMBMEMSIZE1_CTRL)
#define HEAPMBMEMSIZE1_BUFS     0x3000

#define LISTMP_OFFSET           (List - SHAREDMEM)
#define LISTMP1_OFFSET          (List1 - SHAREDMEM)
#define GATEPETERSON_OFFSET     (GATEPETERSONMEM - SHAREDMEM)

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _HEAPBUFAPP_CONFIG_H_ */
