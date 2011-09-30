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
 *  @file   IpcCmdBase.h
 *
 *  @brief  Base file for all TI OMAP IPC ioctl's.
 *          Linux-OMAP IPC has allocated base 0xEE with a range of 0x00-0xFF.
 *          (need to get  the real one from open source maintainers)
 *  ============================================================================
 */


#ifndef _IPCCMDBASE_H
#define _IPCCMDBASE_H

#include <linux/ioctl.h>

#define IPC_IOC_MAGIC       0xE0
#define IPC_IOC_BASE        2

enum ipc_command_count {
    MULTIPROC_CMD_NOS = 4,
    NAMESERVER_CMD_NOS = 15,
    HEAPBUFMP_CMD_NOS = 14,
    SHAREDREGION_CMD_NOS = 13,
    GATEMP_CMD_NOS = 13,
    LISTMP_CMD_NOS = 19,
    MESSAGEQ_CMD_NOS = 18,
    IPC_CMD_NOS = 5,
    SYSMEMMGR_CMD_NOS = 6,
    HEAPMEMMP_CMD_NOS = 15,
    NOTIFY_CMD_NOS = 18
};

enum ipc_command_ranges {
    MULTIPROC_BASE_CMD              = IPC_IOC_BASE,
    MULTIPROC_END_CMD               = (MULTIPROC_BASE_CMD + \
                                      MULTIPROC_CMD_NOS - 1),

    NAMESERVER_BASE_CMD             = 10,
    NAMESERVER_END_CMD              = (NAMESERVER_BASE_CMD + \
                                      NAMESERVER_CMD_NOS - 1),

    HEAPBUFMP_BASE_CMD              = 30,
    HEAPBUFMP_END_CMD               = (HEAPBUFMP_BASE_CMD + \
                                      HEAPBUFMP_CMD_NOS - 1),

    SHAREDREGION_BASE_CMD           = 50,
    SHAREDREGION_END_CMD            = (SHAREDREGION_BASE_CMD + \
                                      SHAREDREGION_CMD_NOS - 1),

    GATEMP_BASE_CMD                 = 70,
    GATEMP_END_CMD                  = (GATEMP_BASE_CMD + \
                                      GATEMP_CMD_NOS - 1),

    LISTMP_BASE_CMD                 = 90,
    LISTMP_END_CMD                  = (LISTMP_BASE_CMD + \
                                      LISTMP_CMD_NOS - 1),

    MESSAGEQ_BASE_CMD               = 110,
    MESSAGEQ_END_CMD                = (MESSAGEQ_BASE_CMD + \
                                      MESSAGEQ_CMD_NOS - 1),

    IPC_BASE_CMD                    = 130,
    IPC_END_CMD                     = (IPC_BASE_CMD + \
                                      IPC_CMD_NOS - 1),

    SYSMEMMGR_BASE_CMD              = 140,
    SYSMEMMGR_END_CMD               = (SYSMEMMGR_BASE_CMD + \
                                      SYSMEMMGR_CMD_NOS - 1),

    HEAPMEMMP_BASE_CMD              = 150,
    HEAPMEMMP_END_CMD               = (HEAPMEMMP_BASE_CMD + \
                                      HEAPMEMMP_CMD_NOS - 1),

    NOTIFY_BASE_CMD                 = 170,
    NOTIFY_END_CMD                  = (NOTIFY_BASE_CMD + \
                                      NOTIFY_CMD_NOS - 1)
};

#endif /* _IPCCMDBASE_H */
