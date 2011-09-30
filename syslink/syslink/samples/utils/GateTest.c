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
 *  @file   GateTest.c
 *
 *  @brief  Test for GateMutex
 *  ============================================================================
 */

#include <Std.h>
#include <IGateProvider.h>
#include <OsalPrint.h>
#include <GateMutex.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_THREADS   32

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

UInt32 gateEntryCount;
IGateProvider_Handle gateHandle;
sem_t gateSemaphores[NUM_THREADS];

Void *GateTestThreadFxn(Void *arg)
{
    IGateProvider_Handle handle = gateHandle;
    UInt id = (UInt) arg;
    IArg key;
    Bool failed = FALSE;

    // Allow next thread to proceed
    if(id > 0)
        sem_post(&gateSemaphores[id]);

    key = IGateProvider_enter(handle);
    gateEntryCount += 1;

    // Check to make sure no other thread has breached the gate
    if(gateEntryCount > 1) {
        Osal_printf("GateTestThreadFxn(%d): gateEntryCount = %d\n", id, gateEntryCount);
        failed = TRUE;
    }

    // Yield, but no other thread should pass the gate
    if(id < NUM_THREADS - 1)
        sem_wait(&gateSemaphores[id + 1]);

    gateEntryCount -= 1;
    IGateProvider_leave(handle, key);

    return (Void *)failed;
}

Void GateTest(Void)
{
    UInt i;
    pthread_t threads[NUM_THREADS];
    UInt status;
    UInt failed;

    gateHandle = (IGateProvider_Handle) GateMutex_create();
    if(gateHandle == NULL) {
        Osal_printf("GateTest: Error creating GateMutex.\n");
        goto exit;
    }
    gateEntryCount = 0;     // Initialize counter

    for(i = 0; i < NUM_THREADS; i++) {
        sem_init(&gateSemaphores[i], 0, 0);

        // Create all threads
        status = pthread_create(&threads[i], NULL, GateTestThreadFxn, (Void*)i);

        if(status < 0) {
            Osal_printf("GateTest: pthread_create #%d returned status 0x%x\n", status);
            goto gateExit;
        }
    }

    // Wait for threads to complete
    for(i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], (Void *)&failed);

    if(failed)
        Osal_printf("GateTest: FAILED!\n");
    else
        Osal_printf("GateTest: PASSED!\n");

gateExit:
    GateMutex_delete((GateMutex_Handle *)&gateHandle);
exit:
    return;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
