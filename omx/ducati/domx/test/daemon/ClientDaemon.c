/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*============================================================================
 *  @file   SyslinkDaemon.c
 *
 *  @brief  Daemon for Syslink functions
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>

/* IPC headers */
#include <SysMgr.h>
#include <ProcMgr.h>
#include <RcmClient.h>


#define SYSM3_PROC_NAME             "SysM3"
#define APPM3_PROC_NAME             "AppM3"

/* Shared Memory Area for MPU - SysM3 */
#define SHAREDMEM                   0xA0000000
#define SHAREDMEMSIZE               0x55000
#define SHAREDMEMNUMBER             0

/* Shared Memory Area for MPU - AppM3 */
#define SHAREDMEM1                  0xA0055000
#define SHAREDMEMSIZE1              0x55000
#define SHAREDMEMNUMBER1            1


/*
 *  ======== MemMgrThreadFxn ========
 */
Void MemMgrThreadFxn();


#if defined (__cplusplus)
extern "C"
{
#endif				/* defined (__cplusplus) */


/*
 *  ======== ipc_setup ========
 */
	Int ipc_setup()
	{
		SysMgr_Config config;
		ProcMgr_StopParams stopParams;
		ProcMgr_StartParams start_params;
		ProcMgr_Handle procMgrHandle_server;
		UInt32 entry_point = 0;
		UInt16 remoteIdSysM3;
		UInt16 remoteIdAppM3;
		UInt32 shAddrBase;
		UInt32 shAddrBase1;
		UInt16 procId;
#if defined (SYSLINK_USE_LOADER)
		Char uProcId;
		UInt32 fileId;
#endif
		Int status = 0;
		Bool appM3Client = FALSE;

		  SysMgr_getConfig(&config);
		  status = SysMgr_setup(&config);
		if (status < 0)
		{
			printf("Error in SysMgr_setup [0x%x]\n", status);
			goto exit;
		}

		/* Get MultiProc IDs by name. */
		remoteIdSysM3 = MultiProc_getId(SYSM3_PROC_NAME);
		printf("MultiProc_getId remoteId: [0x%x]\n", remoteIdSysM3);
		remoteIdAppM3 = MultiProc_getId(APPM3_PROC_NAME);
		printf("MultiProc_getId remoteId: [0x%x]\n", remoteIdAppM3);
		procId = remoteIdSysM3;
		printf("MultiProc_getId procId: [0x%x]\n", procId);

		printf("RCM procId= %d\n", procId);
		/* Open a handle to the ProcMgr instance. */
		status = ProcMgr_open(&procMgrHandle_server, procId);
		if (status < 0)
		{
			printf("Error in ProcMgr_open [0x%x]\n", status);
			goto exit_sysmgr_destroy;
		} else
		{
			printf("ProcMgr_open status [0x%x]\n", status);
			/* Get the address of the shared region in kernel space. */
			status = ProcMgr_translateAddr(procMgrHandle_server,
			    (Ptr) & shAddrBase,
			    ProcMgr_AddrType_MasterUsrVirt,
			    (Ptr) SHAREDMEM, ProcMgr_AddrType_SlaveVirt);
			if (status < 0)
			{
				printf
				    ("Error in ProcMgr_translateAddr [0x%x]\n",
				    status);
				goto exit_procmgr_close;
			} else
			{
				printf
				    ("Virt address of shared address base #1:"
				    " [0x%x]\n", shAddrBase);
			}

			if (status >= 0)
			{
				/* Get the address of the shared region in kernel space. */
				status =
				    ProcMgr_translateAddr
				    (procMgrHandle_server,
				    (Ptr) & shAddrBase1,
				    ProcMgr_AddrType_MasterUsrVirt,
				    (Ptr) SHAREDMEM1,
				    ProcMgr_AddrType_SlaveVirt);
				if (status < 0)
				{
					printf
					    ("Error in ProcMgr_translateAddr [0x%x]\n",
					    status);
					goto exit_procmgr_close;
				} else
				{
					printf
					    ("Virt address of shared address base #2:"
					    " [0x%x]\n", shAddrBase1);
				}
			}
		}
		if (status >= 0)
		{
			/* Add the region to SharedRegion module. */
			status = SharedRegion_add(SHAREDMEMNUMBER,
			    (Ptr) shAddrBase, SHAREDMEMSIZE);
			if (status < 0)
			{
				printf("Error in SharedRegion_add [0x%x]\n",
				    status);
				goto exit_procmgr_close;
			}
		}

		if (status >= 0)
		{
			/* Add the region to SharedRegion module. */
			status = SharedRegion_add(SHAREDMEMNUMBER1,
			    (Ptr) shAddrBase1, SHAREDMEMSIZE1);
			if (status < 0)
			{
				printf("Error in SharedRegion_add1 [0x%x]\n",
				    status);
				goto exit_procmgr_close;
			}
		}

		printf("IPC setup completed successfully!\n");
		return 0;

	      exit_procmgr_close:
		status = ProcMgr_close(&procMgrHandle_server);
		if (status < 0)
		{
			printf("Error in ProcMgr_close: status = 0x%x\n",
			    status);
		}

	      exit_sysmgr_destroy:
		status = SysMgr_destroy();
		if (status < 0)
		{
			printf("Error in SysMgr_destroy: status = 0x%x\n",
			    status);
		}
	      exit:
		return (-1);
	}


	Int main(Int argc, Char * argv[])
	{
		pid_t child_pid, child_sid;
		int status;
		RcmClient_Config cfgParams;
		RcmClient_Params rcmParams;
		char cServerName[] = { "RSrv_Ducati1" };
		RcmClient_Handle rcmHndl = NULL;


		cfgParams.maxNameLen = 20;
		cfgParams.defaultHeapIdArrayLength = 4;

		printf("Spawning TILER server daemon...\n");

		/* Fork off the parent process */
		child_pid = fork();
		if (child_pid < 0)
		{
			printf("Spawn daemon failed!\n");
			exit(EXIT_FAILURE);	/* Failure */
		}
		/* If we got a good PID, then we can exit the parent process. */
		if (child_pid > 0)
		{
			exit(EXIT_SUCCESS);	/* Succeess */
		}

		/* Change file mode mask */
		umask(0);

		/* Create a new SID for the child process */
		child_sid = setsid();
		if (child_sid < 0)
			exit(EXIT_FAILURE);	/* Failure */

		/* Change the current working directory */
		if ((chdir("/")) < 0)
		{
			exit(EXIT_FAILURE);	/* Failure */
		}

		status = ipc_setup();
		if (status < 0)
		{
			printf("\nipc_setup failed\n");
			goto leave;
		}
		//Create default RCM client.
		RcmClient_getConfig(&cfgParams);
		printf("\nPrinting Config Parameters\n");
		printf("\nMaxNameLen %d\nHeapIdArrayLength %d\n",
		    cfgParams.maxNameLen, cfgParams.defaultHeapIdArrayLength);

		printf("\nRPC_InstanceInit: creating rcm instance\n");

		RcmClient_Params_init(NULL, &rcmParams);

		rcmParams.heapId = 1;
		printf("\n Heap ID configured : %d\n", rcmParams.heapId);

		printf("\nCalling client setup\n");
		status = RcmClient_setup(&cfgParams);
		printf("\nClient setup done\n");
		if (status < 0)
		{
			printf("Client  exist.Error Code:%d\n", status);
			goto leave;
		}

		while (rcmHndl == NULL)
		{
			printf
			    ("\nCalling client create with server name = %s\n",
			    cServerName);
			status =
			    RcmClient_create(cServerName, &rcmParams,
			    &rcmHndl);
			printf("\nClient create done\n");

			if (status < 0)
			{
				printf("\nCannot Establish the connection\n");
				goto leave;
			} else
			{
				printf("\nConnected to Server\n");
			}

		}
		//To ensure that it is always running
		while (1)
		{
		}
	      leave:
		return 0;
	}



#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
