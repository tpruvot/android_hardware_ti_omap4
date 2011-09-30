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
/*============================================================================
 *  @file   SyslinkTraceDaemon.c
 *
 *  @brief  Daemon for Syslink traces
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>
#include <UsrUtilsDrv.h>
#include <Memory.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/* Use the Ducati HEAP3 for trace buffer
 * SYSM3 VA address would be 0x81FE0000, APPM3 is 0x81FF0000
 */
#define SYSM3_TRACE_BUFFER_PHYS_ADDR    0x9FFE0000
#define APPM3_TRACE_BUFFER_PHYS_ADDR    0X9FFF0000
#define TESLA_TRACE_BUFFER_PHYS_ADDR    0x9CEF0000

#define TRACE_BUFFER_SIZE               0x10000

#define TIMEOUT_USECS                    5000000

#ifdef HAVE_ANDROID_OS
#undef LOG_TAG
#define LOG_TAG "TRACED"
#endif

static FILE *log;
/* Semaphore to allow only one thread to print at once */
sem_t        semPrint;
/* Scratch buffer with extra space for core names */
Char         tempBuffer [TRACE_BUFFER_SIZE + 128];


/* Thread (core) specific info */
typedef struct traceBufferParams {
    Char      * coreName;
    UInt32      bufferAddress;
} traceBufferParams;



Void printRemoteTraces (Void *args)
{
    Int                 status              = 0;
    Memory_MapInfo      traceinfo;
    UInt32              numOfBytesInBuffer  = 0;
    volatile UInt32   * readPointer;
    volatile UInt32   * writePointer;
    UInt32              writePos;
    Char              * traceBuffer;
    UInt32              numBytesToCopy;
    UInt32              printStart;
    UInt32              i;
    traceBufferParams * params = (traceBufferParams*)args;
    UInt32              coreNameSize = strlen(params->coreName);
    Char                saveChar;

    Osal_printf ("Creating trace thread for %s\n", params->coreName);

    /* Get the user virtual address of the buffer */
    traceinfo.src  = params->bufferAddress;
    traceinfo.size = TRACE_BUFFER_SIZE;
    status = Memory_map (&traceinfo);
    readPointer = (volatile UInt32 *)traceinfo.dst;
    writePointer = (volatile UInt32 *)(traceinfo.dst + 0x4);
    traceBuffer = (Char *)(traceinfo.dst + 0x8);

    /* Initialze read indexes to zero */
    *readPointer = 0;
    *writePointer = 0;
    do {
        do {
            usleep (TIMEOUT_USECS);
        } while (*readPointer == *writePointer);

        sem_wait (&semPrint);    /* Acquire exclusive access to printing */

        /* Copy the trace buffer contents to the scratch buffer. */
        memcpy (tempBuffer, params->coreName, coreNameSize);
        numOfBytesInBuffer = coreNameSize;
        writePos = *writePointer;
        if (*readPointer < writePos) {
            numBytesToCopy = writePos - (*readPointer);
            memcpy (&tempBuffer [numOfBytesInBuffer],
                    &traceBuffer [*readPointer], numBytesToCopy);
            numOfBytesInBuffer += numBytesToCopy;
        }
        else {
            numBytesToCopy = ((TRACE_BUFFER_SIZE - 8) - (*readPointer));
            memcpy (&tempBuffer [numOfBytesInBuffer],
                    &traceBuffer [*readPointer], numBytesToCopy);
            numOfBytesInBuffer += numBytesToCopy;
            numBytesToCopy = writePos;
            memcpy (&tempBuffer [numOfBytesInBuffer], traceBuffer,
                    numBytesToCopy);
            numOfBytesInBuffer += numBytesToCopy;
        }

        /* Update the read position in shared memory. */
        *readPointer = writePos;

        /* Print the traces one line at time. */
        printStart = 0;
        i = coreNameSize;
        while ( i < numOfBytesInBuffer ) {
            /* Search for a newline */
            while (tempBuffer [i] != '\n' && i < numOfBytesInBuffer ) {
                i++;
            }

            /* Pretty print truncated traces at the end of the buffer. */
            if (tempBuffer [i] != '\n') {
                tempBuffer [i] = '\n';
            }

            /* Temporarily replace the char after newline with '\0', */
            /* print trace, then prefix next trace with core name.   */
            saveChar = tempBuffer [i + 1];
            tempBuffer [i + 1] = 0;
            if (log == NULL) {
                Osal_printf ("%s", &tempBuffer [printStart]);
            }
            else {
                fprintf (log,"%s", &tempBuffer [printStart]);
            }
            tempBuffer [i + 1] = saveChar;
            i++;
            printStart = i - coreNameSize;
            memcpy (&tempBuffer [printStart], params->coreName, coreNameSize );
        }

        if (log != NULL ) {
            fflush (log);
        }

        sem_post (&semPrint);    /* Release exclusive access to printing */

    } while (1);

    Osal_printf ("Leaving %s thread function \n", params->coreName);

    return;
}


/** print usage and exit */
static Void printUsageExit (Char * app)
{
    Osal_printf ("%s: [-h] [-l logfile] [-f]\n", app);
    Osal_printf ("  -h   Show this help message.\n");
    Osal_printf ("  -l   Select log file to write. (\"stdout\" can be used for"
                 "terminal output.)\n");
    Osal_printf ("  -f   Run in foreground. (Do not fork daemon process.)\n");

    exit (EXIT_SUCCESS);
}


Int main (Int argc, Char * argv [])
{
    pthread_t           thread_sys; /* server thread object */
    pthread_t           thread_app; /* server thread object */
    pthread_t           thread_dsp; /* server thread object */
    Char              * log_file    = NULL;
    Bool                daemon      = TRUE;
    Int                 i;
    traceBufferParams   args_sys;
    traceBufferParams   args_app;
    traceBufferParams   args_dsp;

    /* parse cmd-line args */
    for (i = 1; i < argc; i++) {
        if (!strcmp ("-l", argv[i])) {
            if (++i >= argc) {
                printUsageExit (argv[0]);
            }
            log_file = argv[i];
        }
        else if (!strcmp ("-f", argv[i])) {
            daemon = FALSE;
        }
        else if (!strcmp ("-h", argv[i])) {
            printUsageExit (argv[0]);
        }
    }

    Osal_printf ("Spawning Ducati-Tesla Trace daemon...\n");

    if (daemon) {
        pid_t child_pid;
        pid_t child_sid;

        /* Fork off the parent process */
        child_pid = fork ();
        if (child_pid < 0) {
            Osal_printf ("Spawning Trace daemon failed!\n");
            exit (EXIT_FAILURE);     /* Failure */
        }

        /* If we got a good PID, then we can exit the parent process. */
        if (child_pid > 0) {
            exit (EXIT_SUCCESS);    /* Success */
        }

        /* Create a new SID for the child process */
        child_sid = setsid ();
        if (child_sid < 0) {
            Osal_printf ("setsid failed!\n");
            exit (EXIT_FAILURE);     /* Failure */
        }
    }

    if (log_file == NULL) {
        log = NULL;
    }
    else {
        if (strcmp (log_file, "stdout") == 0) {
            /* why do we need this?  It would be an issue when logging to file.. */
            /* Change file mode mask */
            umask (0);
            log = stdout;
        }
        else {
            log = fopen (log_file, "a+");
            if (log == NULL ) {
                Osal_printf("Failed to open file: %s\n", log_file);
                exit (EXIT_FAILURE);     /* Failure */
            }
        }
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        Osal_printf ("chdir failed!\n");
        exit (EXIT_FAILURE);     /* Failure */
    }

    sem_init (&semPrint, 0, 1);

    UsrUtilsDrv_setup ();

    args_sys.coreName = "[SYSM3]: ";
    args_sys.bufferAddress = SYSM3_TRACE_BUFFER_PHYS_ADDR;
    args_app.coreName = "[APPM3]: ";
    args_app.bufferAddress = APPM3_TRACE_BUFFER_PHYS_ADDR;
    args_dsp.coreName = "[DSP]: ";
    args_dsp.bufferAddress = TESLA_TRACE_BUFFER_PHYS_ADDR;

    pthread_create (&thread_sys, NULL, (Void *)&printRemoteTraces,
                    (Void*)&args_sys);
    pthread_create (&thread_app, NULL, (Void *)&printRemoteTraces,
                    (Void*)&args_app);
    //pthread_create (&thread_dsp, NULL, (Void *)&printRemoteTraces,
    //                (Void*)&args_dsp);

    pthread_join (thread_sys, NULL);
    Osal_printf ("SysM3 trace thread exited\n");
    pthread_join (thread_app, NULL);
    Osal_printf ("AppM3 trace thread exited\n");
    //pthread_join (thread_dsp, NULL);
    //Osal_printf ("Tesla trace thread exited\n");

    UsrUtilsDrv_destroy ();

    sem_destroy (&semPrint);

    if (log != NULL && log != stdout ) {
        fclose(log);
    }

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
