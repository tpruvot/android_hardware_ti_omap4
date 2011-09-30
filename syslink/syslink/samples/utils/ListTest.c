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
 *  @file   ListTest.c
 *
 *  @brief  Test for List operations
 *  ============================================================================
 */

#include <Std.h>

#include <OsalPrint.h>
#include <List.h>
#include <GateMutex.h>
#include <Memory.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

#define LIST_SIZE     32

typedef struct ListNode
{
    List_Elem elem;
    UInt32 value;
} ListNode;

Void ListTest(Void)
{
    List_Params listParams;
    List_Handle listHandle;
    List_Elem *elem;
    ListNode *node;
    UInt32 i, value;
    Bool failed = FALSE;

    IGateProvider_Handle gateHandle;

    List_Params_init(&listParams);

    gateHandle = (IGateProvider_Handle) GateMutex_create();
    if(gateHandle == NULL) {
        Osal_printf("ListTest: GateMutex_create failed.\n");
        goto exit;
    }

    listParams.gateHandle = gateHandle;
    listHandle = List_create(&listParams);
    if(listHandle == NULL) {
        Osal_printf("ListTest: List_create failed.\n");
        goto gateExit;
    }

    node = Memory_alloc(NULL, LIST_SIZE * sizeof(ListNode), 0);
    if(node == NULL) {
        Osal_printf("ListTest: Memory_alloc failed.\n");
        goto listExit;
    }

    // Put some nodes into the list
    for(i = 0; i < LIST_SIZE; i++) {
        node[i].value = i;
        List_put(listHandle, (List_Elem *)&node[i]);
    }

    // Traverse the list
    for(i = 0, elem = List_next(listHandle, NULL);
        elem != NULL && !failed;
        i++, elem = List_next(listHandle, elem)) {
        value = ((ListNode *)elem)->value;

        // Check against expected value
        if(value != i) {
            Osal_printf("ListTest: data mismatch, expected "
                "0x%x, actual 0x%x\n", i, i, value);
            failed = TRUE;
        }
    }

    // Remove nodes
    for(i = 0; i < LIST_SIZE && !List_empty(listHandle); i++) {
        // Get first element and put it back to test List_get and List_putHead
        elem = List_get(listHandle);
        List_putHead(listHandle, elem);

        // Now remove it permanently to test List_remove
        if(elem != NULL) {
            List_remove(listHandle, elem);
        }
    }
    // Did we remove the expected number of nodes?
    if(i != LIST_SIZE)
    {
        Osal_printf("ListTest: removed %d node(s), expected %d\n",
            i, LIST_SIZE);
        failed = TRUE;
    }

    if(!List_empty(listHandle))
    {
        Osal_printf("ListTest: list not empty!\n");
        failed = TRUE;
    }

    if(failed)
        Osal_printf("ListTest: FAILED!\n");
    else
        Osal_printf("ListTest: PASSED!\n");

listExit:
    List_delete(&listHandle);
gateExit:
    GateMutex_delete((GateMutex_Handle *)&gateHandle);
exit:
    return;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
