/*
 *  utils_test.c
 *
 *  Memory Allocator Utility tests.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __DEBUG__
#define __DEBUG_ASSERT__
#define __DEBUG_ENTRY__

#include <utils.h>
#include <list_utils.h>
#include <debug_utils.h>
#include "testlib.h"

#define TESTS\
    T(test_new())\
    T(test_list())\
    T(test_ezlist())\
    T(test_dzlist())\
    T(test_plist())\
    T(test_mlist())\
    T(test_math())

#define F() in = head.next;
#define N(a) res |= NOT_P(in,!=,&head); \
    res |= NOT_P(in->me,!=,NULL) || NOT_I(in->me->data,==,a); in = in->next;
#define Z(a) res |= NOT_P(in,!=,&head); \
    res |= NOT_I(in->data,==,a); in = in->next;
#define L() res |= NOT_P(in,==,&head);

int all_zero(int *p, int len)
{
    IN;
    int ix = 0;
    for (ix = 0; ix < len; ix ++)
    {
        if (p[ix])
        {
            P("[%d]=%d\n", ix, p[ix]);
            return R_I(1);
        }
    }
    return R_I(0);
}

int test_new() {
    IN;
    int *p;
    p = NEW(int);
    int res = NOT_I(all_zero(p, 1),==,0);
    FREE(p);
    res |= NOT_I(p,==,NULL);
    p = NEWN(int, 8000);
    res |= NOT_I(all_zero(p, 8000),==,0);
    FREE(p);
    p = NEWN(int, 1000000);
    res |= NOT_I(all_zero(p, 1000000),==,0);
    FREE(p);
    return R_I(res);
}

int test_list() {
    IN;

    struct elem {
        int data;
    } *elA, *elB;
    struct list {
        struct elem *me;
        struct list *last, *next;
    } head, *inA, *inB, *in, *in_safe;

    /* initialization */
    DLIST_INIT(head);
    int res = NOT_I(DLIST_IS_EMPTY(head),!=,0);

    /* add element at beginning of list */
    elA = NEW(struct elem);
    elA->data = 1;
    inA = NEW(struct list);
    DLIST_ADD_AFTER(head, elA, *inA);
    F()N(1)L();

    /* add element after an element */
    elB = NEW(struct elem);
    elB->data = 2;
    inB = NEW(struct list);
    DLIST_ADD_AFTER(*inA, elB, *inB);
    F()N(1)N(2)L();

    /* add element at the end of the list */
    elB = NEW(struct elem);
    inB = NEW(struct list);
    (DLIST_ADD_BEFORE(head, elB, *inB))->data = 3;
    F()N(1)N(2)N(3)L();

    /* move an element to another position or another list */
    DLIST_MOVE_AFTER(head, *inB);
    F()N(3)N(1)N(2)L();

    DLIST_MOVE_BEFORE(head, *inB);
    F()N(1)N(2)N(3)L();

    /* works even if the position is the same */
    DLIST_MOVE_BEFORE(head, *inB);
    F()N(1)N(2)N(3)L();

    res |= NOT_I(DLIST_FIRST(head)->data,==,1);
    res |= NOT_I(DLIST_LAST(head)->data,==,3);

    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }
    P(".");

    /* remove elements */
    DLIST_SAFE_LOOP(head, in, in_safe) {
        if (in->me->data > 1)
        {
            DLIST_REMOVE(*in);
            FREE(in->me);
            FREE(in);
        }
    }
    F()N(1)L();

    /* delete list */
    DLIST_SAFE_LOOP(head, in, in_safe) {
        DLIST_REMOVE(*in);
        FREE(in->me);
        FREE(in);
    }
    F()L();

    return R_I(res);
}

int test_ezlist() {
    IN;

    struct elem {
        int data;
        struct elem *me, *last, *next;
    } *elA, *elB, head, *el, *el_safe, *in;

    /* initialization */
    DLIST_INIT(head);
    int res = NOT_I(DLIST_IS_EMPTY(head),!=,0);

    /* add element at beginning of list */
    elA = NEW(struct elem);
    elA->data = 1;
    DLIST_ADD_AFTER(head, elA, *elA);
    F()N(1)L();

    /* add element after an element */
    elB = NEW(struct elem);
    elB->data = 2;
    DLIST_ADD_AFTER(*elA, elB, *elB);
    F()N(1)N(2)L();

    /* add element at the end of the list */
    elB = NEW(struct elem);
    (DLIST_ADD_BEFORE(head, elB, *elB))->data = 3;
    F()N(1)N(2)N(3)L();

    /* move an element to another position or another list */
    DLIST_MOVE_AFTER(head, *elB);
    F()N(3)N(1)N(2)L();

    DLIST_MOVE_BEFORE(head, *elB);
    F()N(1)N(2)N(3)L();

    /* works even if the position is the same */
    DLIST_MOVE_BEFORE(head, *elB);
    F()N(1)N(2)N(3)L();

    res |= NOT_I(DLIST_FIRST(head)->data,==,1);
    res |= NOT_I(DLIST_LAST(head)->data,==,3);

    DLIST_LOOP(head, el) {
        P("%d", el->data);
    }
    P(".");

    /* remove elements */
    DLIST_SAFE_RLOOP(head, el, el_safe) {
        if (el->me->data == 1)
        {
            DLIST_REMOVE(*el);
            FREE(el);
        }
    }
    F()N(2)N(3)L();

    /* delete list */
    DLIST_SAFE_LOOP(head, el, el_safe) {
        DLIST_REMOVE(*el);
        FREE(el);
    }
    F()L();

    return R_I(res);
}

int test_dzlist() {
    IN;

    struct elem {
        int data;
        struct elem *last, *next;
    } *elA, *elB, head, *el, *el_safe, *in;

    /* initialization */
    DZLIST_INIT(head);
    int res = NOT_I(DZLIST_IS_EMPTY(head),!=,0);

    /* add element at beginning of list */
    elA = NEW(struct elem);
    elA->data = 1;
    DZLIST_ADD_AFTER(head, *elA);
    F()Z(1)L();

    /* add element after an element */
    elB = NEW(struct elem);
    elB->data = 2;
    DZLIST_ADD_AFTER(*elA, *elB);
    F()Z(1)Z(2)L();

    /* add element at the end of the list */
    elB = NEW(struct elem);
    (DZLIST_ADD_BEFORE(head, *elB))->data = 3;
    F()Z(1)Z(2)Z(3)L();

    /* move an element to another position or another list */
    DZLIST_MOVE_AFTER(head, *elB);
    F()Z(3)Z(1)Z(2)L();

    DZLIST_MOVE_BEFORE(head, *elB);
    F()Z(1)Z(2)Z(3)L();

    /* works even if the position is the same */
    DZLIST_MOVE_BEFORE(head, *elB);
    F()Z(1)Z(2)Z(3)L();

    res |= NOT_I(DZLIST_FIRST(head)->data,==,1);
    res |= NOT_I(DZLIST_LAST(head)->data,==,3);

    DZLIST_LOOP(head, el) {
        P("%d", el->data);
    }
    P(".");

    /* remove elements */
    DZLIST_SAFE_RLOOP(head, el, el_safe) {
        if (el->data == 1)
        {
            DZLIST_REMOVE(*el);
            FREE(el);
        }
    }
    F()Z(2)Z(3)L();

    /* delete list */
    DZLIST_SAFE_LOOP(head, el, el_safe) {
        DZLIST_REMOVE(*el);
        FREE(el);
    }
    F()L();

    return R_I(res);
}

int test_plist() {
    IN;

    struct elem;
    struct list {
        struct elem *me;
        struct list *last, *next;
    } head, *inA, *inB, *in;
    struct elem {
        int data;
        struct list *list_data;
    } *elA, *elB, *el, *el_safe;

    /* initialization */
    DLIST_INIT(head);
    int res = NOT_I(DLIST_IS_EMPTY(head),!=,0);

    /* add element at beginning of list */
    elA = NEW(struct elem);
    elA->data = 1;
    inA = NEW(struct list);
    DLIST_PADD_AFTER(head, elA, inA, list_data);
    F()N(1)L();

    /* add element after an element */
    elB = NEW(struct elem);
    elB->data = 2;
    inB = NEW(struct list);
    DLIST_PADD_AFTER(*inA, elB, inB, list_data);
    F()N(1)N(2)L();

    /* add element at the end of the list */
    elB = NEW(struct elem);
    inB = NEW(struct list);
    (DLIST_PADD_BEFORE(head, elB, inB, list_data))->data = 3;
    F()N(1)N(2)N(3)L();

    /* move an element to another position or another list */
    DLIST_MOVE_AFTER(head, *inB);
    F()N(3)N(1)N(2)L();

    DLIST_MOVE_BEFORE(head, *inB);
    F()N(1)N(2)N(3)L();

    /* works even if the position is the same */
    DLIST_MOVE_BEFORE(head, *inB);
    F()N(1)N(2)N(3)L();

    res |= NOT_I(DLIST_FIRST(head)->data,==,1);
    res |= NOT_I(DLIST_LAST(head)->data,==,3);

    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }
    P(".");
    DLIST_PLOOP(head, el, list_data) {
        P("%d", el->data);
    }
    P(".");

    /* remove elements */
    DLIST_SAFE_PLOOP(head, el, el_safe, list_data) {
        if (el->data == 2)
        {
            DLIST_REMOVE(*el->list_data);
            FREE(el->list_data);
            FREE(el);
        }
    }
    F()N(1)N(3)L();

    /* delete list */
    DLIST_SAFE_PLOOP(head, el, el_safe, list_data) {
        DLIST_REMOVE(*el->list_data);
        FREE(el->list_data);
        FREE(el);
    }
    F()L();

    return R_I(res);
}

int test_mlist() {
    IN;

    struct elem {
        int data;
        struct list {
            struct list *last, *next;
            struct elem *me;
        } list_data;
    } *elA, *elB, *el, *el_safe;
    struct list head, *in;

    /* initialization */
    DLIST_INIT(head);
    int res = NOT_I(DLIST_IS_EMPTY(head),!=,0);

    /* add element at beginning of list */
    elA = NEW(struct elem);
    elA->data = 1;
    DLIST_MADD_AFTER(head, elA, list_data);
    F()N(1)L();

    /* add element after an element */
    elB = NEW(struct elem);
    elB->data = 2;
    DLIST_MADD_AFTER(elA->list_data, elB, list_data);
    F()N(1)N(2)L();

    /* add element at the end of the list */
    elB = NEW(struct elem);
    (DLIST_MADD_BEFORE(head, elB, list_data))->data = 3;
    F()N(1)N(2)N(3)L();

    /* move an element to another position or another list */
    DLIST_MOVE_AFTER(head, elB->list_data);
    F()N(3)N(1)N(2)L();

    DLIST_MOVE_BEFORE(head, elB->list_data);
    F()N(1)N(2)N(3)L();

    /* works even if the position is the same */
    DLIST_MOVE_BEFORE(head, elB->list_data);
    F()N(1)N(2)N(3)L();

    res |= NOT_I(DLIST_FIRST(head)->data,==,1);
    res |= NOT_I(DLIST_LAST(head)->data,==,3);

    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }
    P(".");
    DLIST_MLOOP(head, el, list_data) {
        P("%d", el->data);
    }
    P(".");

    /* remove elements */
    DLIST_SAFE_MLOOP(head, el, el_safe, list_data) {
        if (el->data != 2)
        {
            DLIST_REMOVE(el->list_data);
            FREE(el);
        }
    }
    F()N(2)L();

    /* delete list */
    DLIST_SAFE_MLOOP(head, el, el_safe, list_data) {
        DLIST_REMOVE(el->list_data);
        FREE(el);
    }
    F()L();

    return R_I(res);
}

int test_math()
{
    IN;
    int res = 0;
    res |= NOT_I(ROUND_UP_TO2POW(0, 4096),==,0);
    res |= NOT_I(ROUND_UP_TO2POW(1, 4096),==,4096);
    res |= NOT_I(ROUND_UP_TO2POW(4095, 4096),==,4096);
    res |= NOT_I(ROUND_UP_TO2POW(4096, 4096),==,4096);
    res |= NOT_I(ROUND_UP_TO2POW(4097, 4096),==,8192);
    res |= NOT_I(ROUND_DOWN_TO2POW(0, 4096),==,0);
    res |= NOT_I(ROUND_DOWN_TO2POW(1, 4096),==,0);
    res |= NOT_I(ROUND_DOWN_TO2POW(4095, 4096),==,0);
    res |= NOT_I(ROUND_DOWN_TO2POW(4096, 4096),==,4096);
    res |= NOT_I(ROUND_DOWN_TO2POW(4097, 4096),==,4096);
    return R_I(res);
}

DEFINE_TESTS(TESTS)

int main(int argc, char **argv)
{
    return TestLib_Run(argc, argv, nullfn, nullfn, NULL);
}

