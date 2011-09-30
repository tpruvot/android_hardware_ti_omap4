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

/*
 * host_os.h
 *
 * IPC support functions for TI OMAP processors.
 *
 */

#ifndef _HOST_OS_H_
#define _HOST_OS_H_

#ifdef __KERNEL__

#include <linux/autoconf.h>
#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
//#include <asm/arch/bus.h>


#if defined (OMAP_2430) || defined (OMAP_3430)
#include <asm/arch/clock.h>
#ifdef OMAP_3430
#include <linux/clk.h>
//  #include <asm-arm/hardware/clock.h>
#endif
#endif

#include <linux/pagemap.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#else
// #include <asm/proc/cache.h>
#include <asm/pci.h>
#include <linux/pci.h>
#endif

/*  ----------------------------------- Macros */

#define SEEK_SET        0	/* Seek from beginning of file.  */
#define SEEK_CUR        1	/* Seek from current position.  */
#define SEEK_END        2	/* Seek from end of file.  */


/* TODO -- Remove, once BP defines them */
#ifdef OMAP_3430
#define INT_MAIL_MPU_IRQ        26
#define INT_DSP_MMU_IRQ        28
#endif


#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#ifdef DEBUG_BRIDGE_PERF
#include <sys/time.h>
#endif
#endif

//#include <dbtype.h>

#endif
