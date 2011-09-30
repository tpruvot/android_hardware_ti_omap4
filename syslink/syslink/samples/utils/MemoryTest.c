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
 *  @file   MemoryTest.c
 *
 *  @brief  Test for Memory module
 *  ============================================================================
 */

 #include <Std.h>
#include <OsalPrint.h>

#include <Memory.h>

#define BUF_SIZE    1024

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */
Void MemoryTest(Void)
{
    UInt32 *buf;
    UInt i;

    buf = (UInt32 *) Memory_alloc(NULL, BUF_SIZE * sizeof(UInt32), 0);

    // Write to buffer
    for(i = 0; i < BUF_SIZE; i++)
        buf[i] = ((i * 3421) % 1335);

    // Readback test
    for(i = 0; i < BUF_SIZE; i++) {
        if(buf[i] != ((i * 3421) % 1335)) {
            Osal_printf("MemoryTest: Mismatch at 0x%x, expected 0x%x, actual 0x%x\n",
                &buf[i], (i * 3421) % 1335, buf[i]);
            break;
        }
    }

    // Free the buffer
    Memory_free(NULL, buf, BUF_SIZE);

    if(i == BUF_SIZE)
        Osal_printf("MemoryTest: PASSED!\n");
    else
        Osal_printf("MemoryTest: FAILED!\n");
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
