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
/* prefix for the host listmp. */
#define LISTMPHOST_PREFIX          "LISTMPHOST"

/* prefix for the slave listmp. */
#define LISTMPSLAVE_PREFIX         "LISTMPSLAVE"

/* Length of ListMP Names */
#define  LISTMPAPP_NAMELENGTH       80u

/* Shared Region ID */
#define APP_SHAREDREGION_ENTRY_ID   0u

/* shared memory size */
#define SHAREDMEM               0xA0000000

/* Memory for the Notify Module */
#define NOTIFYMEM               (SHAREDMEM)
#define NOTIFYMEMSIZE           0x4000

/* Memory a GatePeterson instance */
#define GATEMPMEM               (NOTIFYMEM + NOTIFYMEMSIZE)
#define GATEMPMEMSIZE           0x1000

/* Memory a HeapBufMP instance */
#define HEAPBUFMP               (GATEMPMEM + GATEMPMEMSIZE)
#define HEAPBUFMPSIZE           0x4000

/* Memory a HeapBufMP instance */
#define HEAPBUFMP1              (HEAPBUFMP + HEAPBUFMPSIZE)
#define HEAPBUFMPSIZE1          0x4000

/* Memory for NameServerRemoteNotify */
#define NSRN_MEM                (HEAPBUFMP1 + HEAPBUFMPSIZE1)
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

#define GATEMPMEM1              (HEAPBUF_NS_MEM + HEAPBUF_NS_MEMSIZE)
#define GATEMPMEMSIZE1          0x1000

/* Memory for the Notify Module */
#define HEAPMEM                 (GATEMPMEM1 + GATEMPMEMSIZE1)
#define HEAPMEMSIZE             0x1000

#define HEAPMEM1                (HEAPMEM + HEAPMEMSIZE)
#define HEAPMEMSIZE1            0x1000

#define LOCAL_LIST              (HEAPMEM1 + HEAPMEMSIZE1)
#define LOCAL_LIST_SIZE         0x1000

#define REMOTE_LIST             (LOCAL_LIST + LOCAL_LIST_SIZE)
#define REMOTE_LIST_SIZE        0x1000

#define LOCAL_LIST_OFFSET       (LOCAL_LIST - SHAREDMEM)
#define REMOTE_LIST_OFFSET      (REMOTE_LIST - SHAREDMEM)

#define _HEAPBUFMP               (SHAREDMEM + 0x40000)
#define _HEAPBUFMPSIZE           0x4000

#define _HEAPBUFMP1              (_HEAPBUFMP + _HEAPBUFMPSIZE)
#define _HEAPBUFMPSIZE1          0x4000

#define LOCAL_HEAP              _HEAPBUFMP1
#define REMOTE_HEAP             _HEAPBUFMP
#define LOCAL_HEAP_OFFSET       (LOCAL_HEAP - SHAREDMEM)
#define REMOTE_HEAP_OFFSET      (REMOTE_HEAP - SHAREDMEM)

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* _HEAPBUFAPP_CONFIG_H_ */
