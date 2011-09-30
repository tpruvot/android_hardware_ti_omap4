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
 * ipcioctl.h
 *
 *      Contains structures and commands that are used for interaction
 *      between the IPC API layer and IPC kernel layer.
*/


#ifndef IPC_IOCTL_
#define IPC_IOCTL_

#include <Std.h>
#include <NameServer.h>

typedef union {

	/* MultiProc Module */
	struct {
		UInt16 proc_id;
	} ARGS_MPROC_SET_ID;

	struct {
		UInt16 *proc_id;		
		char *name;
	} ARGS_MPROC_GET_ID;

	struct {
		UInt16 proc_id;
		char *name;
	} ARGS_MPROC_GET_NAME;

	struct {
		UInt16 *max_id;
	} ARGS_MPROC_GET_MAX_ID;

	/* Nameserver Module */

	struct {
		NameServer_Handle handle;
		NameServer_CreateParams *params;
	} ARGS_NMSVR_GET_PARAM;

	struct {
		String name;
		NameServer_CreateParams *params;
		NameServer_Handle *handle;
	} ARGS_NMSVR_CREATE;

	struct {
		NameServer_Handle *handle;
	} ARGS_NMSVR_DELETE;

	struct {
		NameServer_Handle handle;
		String name;
		Ptr buf;
		UInt len;
	} ARGS_NMSVR_ADD;

	struct {
		NameServer_Handle handle;
		String name;
		UInt32 value;
	} ARGS_NMSVR_ADD_U32;

	struct {
		NameServer_Handle handle;
		String name;
	} ARGS_NMSVR_REMOVE;

	struct {
		NameServer_Handle handle;
		String name;
		Ptr buf;
		UInt32 len;
		UInt16 *procId;		
	} ARGS_NMSVR_GET;

	struct {
		NameServer_Handle handle;
		String name;
		Ptr buf;
		UInt32 len;
	
	} ARGS_GET_LOCAL;

	struct {
		String name;
		NameServer_Handle *handle;
	} ARGS_NMSVR_GET_HANDLE;

	struct {
		NameServerRemote_Handle handle;
		UInt16 proc_id;
	} ARGS_NMSVR_REG_REMOTE_DRV;

	struct {
		UInt16 proc_id;
	} ARGS_NMSVR_UNREG_REMOTE_DRV;
} IPC_Trapped_Args;

#define IPC_CMD_BASE                      0

/* multiproc module offsets */
#define CMD_MPROC_BASE_OFFSET             IPC_CMD_BASE
#define CMD_MPROC_SET_ID_OFFSET           (CMD_MPROC_BASE_OFFSET + 0)
#define CMD_MPROC_GET_ID_OFFSET           (CMD_MPROC_BASE_OFFSET + 1)
#define CMD_MPROC_GET_NAME_OFFSET         (CMD_MPROC_BASE_OFFSET + 2)
#define CMD_MPROC_GET_MAX_ID_OFFSET       (CMD_MPROC_BASE_OFFSET + 3)
#define CMD_MPROC_END_OFFSET              CMD_MPROC_GET_MAX_ID_OFFSET

#define CMD_NMSVR_BASE_OFFSET             (CMD_MPROC_END_OFFSET + 1)
#define CMD_NMSVR_GET_PARAM_OFFSET        (CMD_NMSVR_BASE_OFFSET + 0)
#define CMD_NMSVR_GET_HANDLE_OFFSET       (CMD_NMSVR_BASE_OFFSET + 1)
#define CMD_NMSVR_CREATE_OFFSET           (CMD_NMSVR_BASE_OFFSET + 2)
#define CMD_NMSVR_DELETE_OFFSET           (CMD_NMSVR_BASE_OFFSET + 3)
#define CMD_NMSVR_ADD_OFFSET              (CMD_NMSVR_BASE_OFFSET + 4)
#define CMD_NMSVR_ADD_U32_OFFSET          (CMD_NMSVR_BASE_OFFSET + 5)
#define CMD_NMSVR_REMOVE_OFFSET           (CMD_NMSVR_BASE_OFFSET + 6)
#define CMD_NMSVR_GET_OFFSET              (CMD_NMSVR_BASE_OFFSET + 7)
#define CMD_NMSVR_GET_LOCAL_OFFSET        (CMD_NMSVR_BASE_OFFSET + 8)
#define CMD_NMSVR_REG_REMOTE_DRV_OFFSET   (CMD_NMSVR_BASE_OFFSET + 9)
#define CMD_NMSVR_UNREG_REMOTE_DRV_OFFSET (CMD_NMSVR_BASE_OFFSET + 9)
#define CMD_NMSVR_END_OFFSET              CMD_NMSVR_UNREG_REMOTE_DRV_OFFSET

#endif /* IPC_IOCTL_ */

