/*
 *  list_utils.h
 *
 *  Utility definitions for the Memory Interface for TI OMAP processors.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
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

#ifndef _LIST_UTILS_H_
#define _LIST_UTILS_H_

/*

    DLIST macros facilitate double-linked lists in a generic fashion.

    - Double-linked lists simplify list manipulations, so they are preferred to
      single-linked lists.

    - Having a double-linked list with a separate header allow accessing the
      last element of the list easily, so this is also preferred to a double
      linked list that uses NULL pointers at its ends.

    Therefore, the following is required:  a list info structure (separate from
    the element structure, although the info structure can be a member of the
    element structure).  The info structure must contain (at least) the
    following 3 members:

    *next, *last: as pointers to the info structure.  These are the next and
                  previous elements in the list.
    *me: as a pointer to the element structure.  This is how we get to the
         element itself, as next and last points to another info structure.

    The list element structure __MAY__ contain the info structure as a member,
    or a pointer to the info structure if it is desired to be kept separately.
    In such cases macros are provided to add the elements directly to the list,
    and automatically set up the info structure fields correctly.  You can also
    iterate through the elements of the list using a pointer to the elements
    itself, as the list structure can be obtained for each element.

    Otherwise, macros are provided to manipulate list info structures and
    link them in any shape or form into double linked lists.  This allows
    having NULL values as members of such lists.

   :NOTE: If you use a macro with a field argument, you must not have NULL
   elements because the field of any element must be/point to the list
   info structure

    DZLIST macros

    Having the list info structure separate from the element structure also
    allows to link elements into many separate lists (with separate info
    structure fields/pointers).  However, for cases where only a single list
    is desired, a set of easy macros are also provided.

    These macros combine the element and list structures.  These macros
    require the following 2 members in each element structure:

    *next, *last: as pointers to the element structure.  These are the next and
                  previous elements in the list.

    :NOTE: In case of the DZLIST-s, the head of the list must be an element
    structure, where only the next and last members are used. */

/*
  Usage:

  DLIST macros are designed for preallocating the list info structures, e.g. in
  an array.  This is why most macros take a list info structure, and not a
  pointer to a list info structure.

  Basic linked list consists of element structure and list info structure

    struct elem {
        int data;
    } *elA, *elB;
    struct list {
        struct elem *me;
        struct list *last, *next;
    } head, *inA, *inB, *in;

    DLIST_INIT(head);              // initialization -> ()
    DLIST_IS_EMPTY(head) == TRUE;  // emptiness check

    // add element at beginning of list -> (1)
    elA = NEW(struct elem);
    elA->data = 1;
    inA = NEW(struct list);
    DLIST_ADD_AFTER(head, elA, *inA);

    // add before an element -> (2, 1)
    elB = NEW(struct elem);
    elB->data = 2;
    inB = NEW(struct list);
    DLIST_ADD_BEFORE(*inA, elB, *inB);

    // move an element to another position or another list -> (1, 2)
    DLIST_MOVE_BEFORE(head, *inB);

    // works even if the position is the same -> (1, 2)
    DLIST_MOVE_BEFORE(head, *inB);

    // get first and last elements
    DLIST_FIRST(head) == elA;
    DLIST_LAST(head) == elB;

    // loop through elements
    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }

    // remove element -> (2)
    DLIST_REMOVE(*inA);
    FREE(elA);
    FREE(inA);

    // delete list
    DLIST_SAFE_LOOP(head, in, inA) {
        DLIST_REMOVE(*in);
        FREE(in->me);
        FREE(in);
    }

  You can combine the element and list info structures to create an easy list,
  but you still need to specify both element and info structure while adding
  elements.

    struct elem {
        int data;
        struct elem *me, *last, *next;
    } head, *el, *elA, *elB;

    DLIST_INIT(head);              // initialization -> ()
    DLIST_IS_EMPTY(head) == TRUE;  // emptiness check

    // add element at beginning of list -> (1)
    elA = NEW(struct elem);
    elA->data = 1;
    DLIST_ADD_AFTER(head, elA, *elA);

    // add before an element -> (2, 1)
    elB = NEW(struct elem);
    elB->data = 2;
    DLIST_ADD_BEFORE(*elA, elB, *elB);

    // move an element to another position or another list -> (1, 2)
    DLIST_MOVE_BEFORE(head, *elB);

    // works even if the position is the same -> (1, 2)
    DLIST_MOVE_BEFORE(head, *elB);

    // get first and last elements
    DLIST_FIRST(head) == elA;
    DLIST_LAST(head) == elB;

    // loop through elements
    DLIST_LOOP(head, el) {
        P("%d", el->data);
    }

    // remove element -> (2)
    DLIST_REMOVE(*elA);
    FREE(elA);

    // delete list
    DLIST_SAFE_LOOP(head, el, elA) {
        DLIST_REMOVE(*el);
        FREE(el);
    }

  Or, you can use a DZLIST.

    struct elem {
        int data;
        struct elem *last, *next;
    } head, *el, *elA, *elB;

    DZLIST_INIT(head);              // initialization -> ()
    DZLIST_IS_EMPTY(head) == TRUE;  // emptiness check

    // add element at beginning of list -> (1)
    elA = NEW(struct elem);
    elA->data = 1;
    DZLIST_ADD_AFTER(head, *elA);

    // add before an element -> (2, 1)
    elB = NEW(struct elem);
    elB->data = 2;
    DZLIST_ADD_BEFORE(*elA, *elB);

    // move an element to another position or another list -> (1, 2)
    DZLIST_MOVE_BEFORE(head, *elB);

    // works even if the position is the same -> (1, 2)
    DZLIST_MOVE_BEFORE(head, *elB);

    // get first and last elements
    DZLIST_FIRST(head) == elA;
    DZLIST_LAST(head) == elB;

    // loop through elements
    DZLIST_LOOP(head, el) {
        P("%d", el->data);
    }

    // remove element -> (2)
    DZLIST_REMOVE(*elA);
    FREE(elA);

    // delete list
    DZLIST_SAFE_LOOP(head, el, elA) {
        DZLIST_REMOVE(*el);
        FREE(el);
    }

  A better way to get to the list structure from the element structure is to
  enclose a pointer the list structure in the element structure.  This allows
  getting to the next/previous element from the element itself.

    struct elem;
    struct list {
        struct elem *me;
        struct list *last, *next;
    } head, *inA, *inB, *in;
    struct elem {
        int data;
        struct list *list_data;
    } *elA, *elB, *el;

    // or

    struct elem {
        int data;
        struct list {
            struct elem *me;
            struct list *last, *next;
        } *list_data;
    } *elA, *elB, *el;
    struct list head, *inA, *inB, *in;

    DLIST_INIT(head);              // initialization -> ()
    DLIST_IS_EMPTY(head) == TRUE;  // emptiness check

    // add element at beginning of list -> (1)
    elA = NEW(struct elem);
    elA->data = 1;
    inA = NEW(struct list);
    DLIST_PADD_AFTER(head, elA, inA, list_data);

    // add before an element -> (2, 1)
    elB = NEW(struct elem);
    elB->data = 2;
    inB = NEW(struct list);
    DLIST_PADD_BEFORE(*elA, elB, inB, list_data);

    // move an element to another position or another list -> (1, 2)
    DLIST_MOVE_BEFORE(head, *inB);

    // works even if the position is the same -> (1, 2)
    DLIST_MOVE_BEFORE(head, *inB);

    // get first and last elements
    DLIST_FIRST(head) == elA;
    DLIST_LAST(head) == elB;

    // loop through elements
    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }
    DLIST_PLOOP(head, el, list_data) {
        P("%d", el->data);
    }

    // remove element
    DLIST_REMOVE(*inA);
    FREE(inA);
    FREE(elA);

    // delete list
    DLIST_SAFE_PLOOP(head, el, elA, list_data) {
        DLIST_REMOVE(*el->list_data);
        FREE(el->list_data);
        FREE(el);
    }

  Lastly, you can include the list data in the element structure itself.

    struct elem {
        int data;
        struct list {
            struct list *last, *next;
            struct elem *me;
        } list_data;
    } *elA, *elB, *el;
    struct list head, *in;

    DLIST_INIT(head);              // initialization -> ()
    DLIST_IS_EMPTY(head) == TRUE;  // emptiness check

    // add element at beginning of list -> (1)
    elA = NEW(struct elem);
    elA->data = 1;
    DLIST_MADD_AFTER(head, elA, list_data);

    // add before an element -> (2, 1)
    elB = NEW(struct elem);
    elB->data = 2;
    DLIST_PADD_BEFORE(elA->list_data, elB, list_data);

    // move an element to another position or another list -> (1, 2)
    DLIST_MOVE_BEFORE(head, elB->list_data);

    // works even if the position is the same -> (1, 2)
    DLIST_MOVE_BEFORE(head, elB->list_data);

    // get first and last elements
    DLIST_FIRST(head) == elA;
    DLIST_LAST(head) == elB;

    // loop through elements
    DLIST_LOOP(head, in) {
        P("%d", in->me->data);
    }
    DLIST_MLOOP(head, el, list_data) {
        P("%d", el->data);
    }

    // remove element
    DLIST_REMOVE(elA->list_data);
    FREE(elA);

    // delete list
    DLIST_SAFE_MLOOP(head, el, elA, list_data) {
        DLIST_REMOVE(el->list_data);
        FREE(el);
    }

 */

/* -- internal (generic direction) macros -- */
#define DLIST_move__(base, info, next, last) \
    ((info).last = &base)->next = ((info).next = (base).next)->last = &(info)
#define DLIST_padd__(base, pInfo, pElem, pField, next, last) \
    ((pInfo)->last = &base)->next = ((pInfo)->next = (base).next)->last = \
    pElem->pField = pInfo
#define DLIST_loop__(head, pInfo, next) \
    for (pInfo=(head).next; pInfo != &(head); pInfo = (pInfo)->next)
#define DLIST_ploop__(head, pElem, pField, next) \
    for (pElem=(head).next->me; pElem; pElem = (pElem)->pField->next->me)
#define DLIST_mloop__(head, pElem, mField, next) \
    for (pElem=(head).next->me; pElem; pElem = (pElem)->mField.next->me)
#define DLIST_safe_loop__(head, pInfo, pInfo_safe, next) \
    for (pInfo=(head).next; pInfo != &(head); pInfo = pInfo_safe) \
        if ((pInfo_safe = (pInfo)->next) || 1)
#define DLIST_safe_ploop__(head, pElem, pElem_safe, pField, next) \
    for (pElem=(head).next->me; pElem; pElem = pElem_safe) \
        if ((pElem_safe = (pElem)->pField->next->me) || 1)
#define DLIST_safe_mloop__(head, pElem, pElem_safe, mField, next) \
    for (pElem=(head).next->me; pElem; pElem = pElem_safe) \
        if ((pElem_safe = (pElem)->mField.next->me) || 1)

#define DLIST_IS_EMPTY(head) ((head).next == &(head))

/* Adds the element (referred to by the info structure) before/or after another
   element (or list header) (base). */

#define DLIST_ADD_AFTER(base, pElem, info) \
    (DLIST_move__(base, info, next, last))->me = pElem
#define DLIST_ADD_BEFORE(base, pElem, info) \
    (DLIST_move__(base, info, last, next))->me = pElem

/* Adds the element (referred to by pElem pointer) along with its info
   structure (referred to by pInfo pointer) before/or after an element or
   list header (base).  It also sets up the list structure header to point to
   the element as well as the element's field to point back to the list info
   structure. */
#define DLIST_PADD_BEFORE(base, pElem, pInfo, pField) \
    (DLIST_padd__(base, pInfo, pElem, pField, last, next))->me = pElem
#define DLIST_PADD_AFTER(base, pElem, pInfo, pField) \
    (DLIST_padd__(base, pInfo, pElem, pField, next, last))->me = pElem

/* Adds the element (referred to by pElem pointer) before/or after an element or
   list header (base).  It also sets up the list structure header (which is a
   member of the element's structure) to point to the element. */
#define DLIST_MADD_BEFORE(base, pElem, mField) \
    (DLIST_move__(base, pElem->mField, last, next))->me = pElem
#define DLIST_MADD_AFTER(base, pElem, mField) \
    (DLIST_move__(base, pElem->mField, next, last))->me = pElem

/* Removes the element (referred to by the info structure) from its current
   list.  This requires that the element is a part of a list.

   :NOTE: the info structure will still think that it belongs to the list it
   used to belong to. However, the old list will not contain this element any
   longer. You want to discard the info/element after this call.  Otherwise,
   you can use one of the MOVE macros to also add the item to another list,
   or another place in the same list. */
#define DLIST_REMOVE(info) ((info).last->next = (info).next)->last = (info).last

/* Initializes the list header (to an empty list) */
#define DLIST_INIT(head) \
    S_ { (head).me = NULL; (head).next = (head).last = &(head); } _S

/* These functions move an element (referred to by the info structure) before
   or after another element (or the list head).
   :NOTE: This logic also works for moving an element after/before itself. */
#define DLIST_MOVE_AFTER(base, info) \
    S_ { DLIST_REMOVE(info); DLIST_move__(base, info, next, last); } _S
#define DLIST_MOVE_BEFORE(base, info) \
    S_ { DLIST_REMOVE(info); DLIST_move__(base, info, last, next); } _S

/* Loops behave syntactically as a for() statement.  They traverse the loop
   variable from the 1st to the last element (or in the opposite direction in
   case of RLOOP). There are 3 flavors of loops depending on the type of the
   loop variable.

   DLIST_LOOP's loop variable is a pointer to the list info structure.  You can
   get to the element by using the 'me' member.  Nonetheless, this loop
   construct allows having NULL elements in the list.

   DLIST_MLOOP's loop variable is a pointer to a list element.  mField is the
   field of the element containing the list info structure.  Naturally, this
   list cannot have NULL elements.

   DLIST_PLOOP's loop variable is also a pointer to a list element.  Use this
   construct if the element contains a pointer to the list info structure
   instead of embedding it directly into the element structure.

*/
#define DLIST_LOOP(head, pInfo)           DLIST_loop__(head, pInfo, next)
#define DLIST_RLOOP(head, pInfo)          DLIST_loop__(head, pInfo, last)
#define DLIST_MLOOP(head, pElem, mField) \
    DLIST_mloop__(head, pElem, mField, next)
#define DLIST_RMLOOP(head, pElem, mField) \
    DLIST_mloop__(head, pElem, mField, last)
#define DLIST_PLOOP(head, pElem, pField) \
    DLIST_ploop__(head, pElem, pField, next)
#define DLIST_RPLOOP(head, pElem, pField) \
    DLIST_ploop__(head, pElem, pField, last)

/* Safe loops are like ordinary loops, but they allow removal of the current
   element from the list. They require an extra loop variable that holds the
   value of the next element in case the current element is moved/removed. */
#define DLIST_SAFE_LOOP(head, pInfo, pInfo_safe) \
    DLIST_safe_loop__(head, pInfo, pInfo_safe, next)
#define DLIST_SAFE_RLOOP(head, pInfo, pInfo_safe) \
    DLIST_safe_loop__(head, pInfo, pInfo_safe, last)
#define DLIST_SAFE_MLOOP(head, pElem, pElem_safe, mField) \
    DLIST_safe_mloop__(head, pElem, pElem_safe, mField, next)
#define DLIST_SAFE_RMLOOP(head, pElem, pElem_safe, mField) \
    DLIST_safe_mloop__(head, pElem, pElem_safe, mField, last)
#define DLIST_SAFE_PLOOP(head, pElem, pElem_safe, pField) \
    DLIST_safe_ploop__(head, pElem, pElem_safe, pField, next)
#define DLIST_SAFE_RPLOOP(head, pElem, pElem_safe, pField) \
    DLIST_safe_ploop__(head, pElem, pElem_safe, pField, last)

/* returns the first element of a list */
#define DLIST_FIRST(head) (head).next->me
/* returns the last element of a list */
#define DLIST_LAST(head) (head).last->me


/* DZLIST equivalent API - provided so arguments are specified */
#define DZLIST_INIT(head)        S_ { (head).next = (head).last = &(head); } _S
#define DZLIST_IS_EMPTY(head)          DLIST_IS_EMPTY(head)
#define DZLIST_FIRST(head)             (head).next
#define DZLIST_LAST(head)              (head).last

#define DZLIST_ADD_AFTER(base, elem)   DLIST_move__(base, elem, next, last)
#define DZLIST_ADD_BEFORE(base, elem)  DLIST_move__(base, elem, last, next)

#define DZLIST_REMOVE(elem)            DLIST_REMOVE(elem)

#define DZLIST_MOVE_AFTER(base, elem)  DLIST_MOVE_AFTER(base, elem)
#define DZLIST_MOVE_BEFORE(base, elem) DLIST_MOVE_BEFORE(base, elem)

#define DZLIST_LOOP(head, pElem)       DLIST_LOOP(head, pElem)
#define DZLIST_RLOOP(head, pElem)      DLIST_RLOOP(head, pElem)
#define DZLIST_SAFE_LOOP(head, pElem, pElem_safe)   \
                                       DLIST_SAFE_LOOP(head, pElem, pElem_safe)
#define DZLIST_SAFE_RLOOP(head, pElem, pElem_safe)  \
                                       DLIST_SAFE_RLOOP(head, pElem, pElem_safe)

#endif
