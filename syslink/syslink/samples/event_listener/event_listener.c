/*
 *  Copyright 2001-2010 Texas Instruments - http://www.ti.com/
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

/*============================================================================
 *  @file   event_listener.c
 *
 *  @brief  Demonstrate on how to register to mmu fault notification
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>

/* RCM headers */
#include <IpcUsr.h>
#include <ProcMgr.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

static pthread_t                fault_handle;
static sem_t                    sem_fault_wait;


struct listener_args
{
	ProcMgr_ProcId proc;
};

static void *listener_handler(void *data)
{
	int status;
	struct listener_args *args = data;
	ProcMgr_Handle proc;
	char *events_name[] = {"MMUFault", "PROC_ERROR", "PROC_STOP",
							 "PROC_WATCHDOG"};
	char *procs[] = {"Tesla", "AppM3", "SysM3"};
	ProcMgr_EventType events[] = {PROC_MMU_FAULT, PROC_ERROR, PROC_STOP,
								PROC_WATCHDOG};
	UInt i;
	UInt size;

	size = (sizeof (events)) / (sizeof (ProcMgr_EventType));

	while (1) {
		status = Ipc_setup(NULL);
		if (status < 0) {
				Osal_printf ("Error in MultiProc_setup [0x%x]\n", status);
				break;
		}
		Osal_printf("Waiting daemon to start\n");
		status = ProcMgr_waitForEvent(args->proc, PROC_START, -1);
		if (status != PROCMGR_SUCCESS) {
			Osal_printf ("Error ProcMgr_waitForEvent %d\n"
								, status);
			break;
		}
		Osal_printf("Daemon is running\n");
		Osal_printf("Open handle to remote proc module\n");
		status = ProcMgr_open(&proc, args->proc);
		if (status < 0) {
			Osal_printf ("Error ProcMgr_open %d\n"
								, status);
			break;
		}
		Osal_printf("Waiting fatal erros from %s\n",
							procs[args->proc]);
		status = ProcMgr_waitForMultipleEvents(args->proc,
							events, size, -1, &i);

		if (status != PROCMGR_SUCCESS) {
			Osal_printf ("Error ProcMgr_waitForMultipleEvents %d\n"
								, status);
			break;
		}
		Osal_printf("Received %s notification from %s\n"
				, events_name[i], procs[args->proc]);
		Osal_printf("Close the opened handles so that recovery can "
				"restart remote processor\n");
		usleep(100);
		ProcMgr_close(&proc);
		usleep(100);
		Ipc_destroy();
	}
	/* Initiate cleanup */
	sem_post(&sem_fault_wait);
	return NULL;
}


void print_usage()
{
	Osal_printf("./fault_listener <proc> \n"
		"\tProcs:\n"
		"\t\t 0: Tesla\n"
		"\t\t 1: AppM3\n"
		"\t\t 2: SysM3\n\n");
	exit(1);
}

/*
 *  ======== main ========
 */
int main (int argc, char * argv [])
{
	short proc;
	struct listener_args args;

	if (argc != 2)
		print_usage();

	proc = atoi(argv[1]);

	if (proc < 0 || proc > PROC_SYSM3)
		print_usage();

	args.proc = proc;
	/* Create the fault handler thread */
	Osal_printf ("Create listener handler thread.\n");
	sem_init(&sem_fault_wait, 0, 0);
	pthread_create (&fault_handle, NULL,
	                &listener_handler, &args);

	sem_wait(&sem_fault_wait);

	sem_destroy(&sem_fault_wait);

	Osal_printf ("Exiting listener application application.\n");

	return 0;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
