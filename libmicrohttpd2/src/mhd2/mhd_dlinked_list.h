/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024-2026 Evgeny Grin (Karlson2k)

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file src/mhd2/mhd_dlinked_list.h
 * @brief  Doubly-linked list macros and declarations
 * @author Karlson2k (Evgeny Grin)
 *
 * Doubly-linked list macros help create and manage the chain of objects
 * connected via inter-link pointers (named here @a links_name), while
 * the list is held by the owner in the helper struct (named here @a list_name).
 */

#ifndef MHD_DLINKED_LIST_H
#define MHD_DLINKED_LIST_H 1

#include "mhd_sys_options.h"

#include "sys_null_macro.h"
#include "mhd_assume.h"


/* This header defines macros for handling doubly-linked lists of objects
   (list elements). The pointers to the first and the last elements in the
   list are held in the list "owner".
   The list elements connect to each other via "next" and "prev" inter-links.
   Each element can be part of several lists at the same time, if referenced
   by differently named fields with inter-links. For example, connections are
   maintained in "all connections" and "need to be processed" lists
   simultaneously.
   A list element can be removed from the list (if it is already in the list)
   or inserted into the list (if it is NOT in the list) at any moment.
   Typically the name of the list (the field inside the "owner" object) is
   the same as the name of field with inter-links. However, it is possible to
   use different names. For example, connections can be removed from "all
   connections" list and moved to the "clean up" list using the same internal
   inter-links field "all connections".
   As this is a doubly-linked list, it can be walked from the beginning to
   the end and in the opposite direction.
   The list is designed to work with struct tags as contained and container
   objects.
 */

/* Helpers */

#define mhd_DLNKDL_LIST_TYPE_(base_name) struct base_name ## _list

#define mhd_DLNKDL_LINKS_TYPE_(base_name) struct base_name ## _links


/* Names */

/**
 * The name of the struct (struct tag) that holds the list in the owner object
 */
#define mhd_DLNKDL_LIST_TYPE(base_name) mhd_DLNKDL_LIST_TYPE_ (base_name)

/**
 * The name of the struct (struct tag) that holds the inter-links between list
 * elements
 */
#define mhd_DLNKDL_LINKS_TYPE(base_name) mhd_DLNKDL_LINKS_TYPE_ (base_name)


/* Definitions of the structures */

/**
 * Template for declaration of the list helper struct
 * @param l_type the struct tag name of elements that the list holds
 */
#define mhd_DLINKEDL_LIST_DEF(l_type) \
        mhd_DLNKDL_LIST_TYPE (l_type) { /* Holds the list in the owner */         \
          struct l_type *first; /* The pointer to the first element in the list */ \
          struct l_type *last; /* The pointer to the last element in the list */   \
        }

/**
 * Template for declaration of the inter-links helper struct
 * @param l_type the struct tag name of elements linked by the inter-links
 */
#define mhd_DLINKEDL_LINKS_DEF(l_type) \
        /* Holds the inter-links in the list element */ \
        mhd_DLNKDL_LINKS_TYPE (l_type) {                \
          struct l_type *prev; /* The pointer to the previous element in the list */ \
          struct l_type *next; /* The pointer to the next element in the list */     \
        }

/**
 * Template for declaration of the list helper structs
 * @param l_type the struct tag name of elements that the list holds
 */
#define mhd_DLINKEDL_STRUCTS_DEFS(l_type) \
        mhd_DLINKEDL_LIST_DEF (l_type); mhd_DLINKEDL_LINKS_DEF (l_type)


/* Declarations of the types for the list owners and the list elements */

/**
 * Declare the list field in the owner struct
 */
#define mhd_DLNKDL_LIST(l_type,list_name) \
        mhd_DLNKDL_LIST_TYPE (l_type) list_name

/**
 * Declare the inter-links field in the list element
 */
#define mhd_DLNKDL_LINKS(l_type,links_name) \
        mhd_DLNKDL_LINKS_TYPE (l_type) links_name

/* Direct work with the list */
/* These macros directly use the pointer to the list and allow using
 * names of the list field (within the owner object) different from the
 * names of the inter-links field (in the list elements). */

/**
 * Initialise the doubly linked list pointers in the list owner using
 * the direct pointer to the list
 * @warning arguments are evaluated multiple times
 * @param p_list the pointer to the list
 */
#define mhd_DLINKEDL_INIT_LIST_D(p_list) \
        do {(p_list)->first = NULL; (p_list)->last = NULL;} while (0)

/**
 * Insert new list element into the first position in the list using direct
 * pointer to the list
 * @warning arguments are evaluated multiple times
 * @param p_list the pointer to the list
 * @param p_obj the pointer to the new element to insert into the list,
 *              using @a links_name inter-links
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_INS_FIRST_D(p_list,p_obj,links_name) do { \
          mhd_ASSUME (NULL == (p_obj)->links_name.prev); \
          mhd_ASSUME (NULL == (p_obj)->links_name.next); \
          mhd_ASSUME ((p_obj) != (p_list)->first);       \
          mhd_ASSUME ((p_obj) != (p_list)->last);        \
          if (NULL != (p_list)->first)                             \
          { mhd_ASSUME (NULL == (p_list)->first->links_name.prev); \
            mhd_ASSUME (NULL == (p_list)->last->links_name.next);  \
            mhd_ASSUME ((p_obj) != (p_list)->first->links_name.next); \
            mhd_ASSUME (NULL != (p_list)->last);                   \
            ((p_obj)->links_name.next = (p_list)->first)           \
            ->links_name.prev = (p_obj); } else \
          { mhd_ASSUME (NULL == (p_list)->last);               \
            (p_list)->last = (p_obj); }                        \
          (p_list)->first = (p_obj);  } while (0)

/**
 * Insert new list element into the last position in the list using direct
 * pointer to the list
 * @warning arguments are evaluated multiple times
 * @param p_list the pointer to the list
 * @param p_obj the pointer to the new element to insert into the list,
 *              using @a links_name inter-links
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_INS_LAST_D(p_list,p_obj,links_name) do { \
          mhd_ASSUME (NULL == (p_obj)->links_name.prev); \
          mhd_ASSUME (NULL == (p_obj)->links_name.next); \
          mhd_ASSUME ((p_obj) != (p_list)->first);       \
          mhd_ASSUME ((p_obj) != (p_list)->last);        \
          if (NULL != (p_list)->last)                              \
          { mhd_ASSUME (NULL == (p_list)->last->links_name.next);  \
            mhd_ASSUME (NULL == (p_list)->first->links_name.prev); \
            mhd_ASSUME ((p_obj) != (p_list)->last->links_name.prev); \
            mhd_ASSUME (NULL != (p_list)->first);                 \
            ((p_obj)->links_name.prev = (p_list)->last)           \
            ->links_name.next = (p_obj); } else \
          { mhd_ASSUME (NULL == (p_list)->first);             \
            (p_list)->first = (p_obj); }                      \
          (p_list)->last = (p_obj);  } while (0)

/**
 * Remove list element from the list using direct pointer to the list
 * @warning arguments are evaluated multiple times
 * @param p_list the pointer to the list
 * @param p_obj the pointer to the existing list element to remove from the list
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_DEL_D(p_list,p_obj,links_name) do {    \
          mhd_ASSUME (NULL != (p_list)->first);             \
          mhd_ASSUME (NULL != (p_list)->last);              \
          mhd_ASSUME (NULL == (p_list)->first->links_name.prev); \
          mhd_ASSUME (NULL == (p_list)->last->links_name.next);  \
          mhd_ASSUME ((p_obj) != (p_obj)->links_name.prev);      \
          mhd_ASSUME ((p_list)->last != (p_obj)->links_name.prev);  \
          mhd_ASSUME ((p_obj) != (p_obj)->links_name.next);         \
          mhd_ASSUME ((p_list)->first != (p_obj)->links_name.next); \
          if (NULL != (p_obj)->links_name.next)             \
          { mhd_ASSUME ((p_obj) == (p_obj)->links_name.next->links_name.prev); \
            mhd_ASSUME ((p_obj) != (p_list)->last);       \
            mhd_ASSUME ((p_obj)->links_name.next !=       \
                        (p_obj)->links_name.prev);        \
            (p_obj)->links_name.next->links_name.prev =   \
              (p_obj)->links_name.prev; } else            \
          { mhd_ASSUME ((p_obj) == (p_list)->last);       \
            (p_list)->last = (p_obj)->links_name.prev; }  \
          if (NULL != (p_obj)->links_name.prev)           \
          { mhd_ASSUME ((p_obj) == (p_obj)->links_name.prev->links_name.next); \
            mhd_ASSUME ((p_obj) != (p_list)->first);      \
            mhd_ASSUME ((p_obj)->links_name.next !=       \
                        (p_obj)->links_name.prev);        \
            (p_obj)->links_name.prev->links_name.next =   \
              (p_obj)->links_name.next; } else            \
          { mhd_ASSUME ((p_obj) == (p_list)->first);      \
            (p_list)->first = (p_obj)->links_name.next; } \
          (p_obj)->links_name.prev = NULL;                \
          (p_obj)->links_name.next = NULL;  } while (0)

/**
 * Get the first element in the list using direct pointer to the list
 */
#define mhd_DLINKEDL_GET_FIRST_D(p_list) ((p_list)->first)

/**
 * Get the last element in the list using direct pointer to the list
 */
#define mhd_DLINKEDL_GET_LAST_D(p_list) ((p_list)->last)

/**
 * Move the list element within the list to the first position using a direct
 * pointer to the list
 * @warning arguments are evaluated multiple times
 * @param p_list the pointer to the list
 * @param p_obj the pointer to the existing list element to move to the
 *              first position
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_MOVE_TO_FIRST_D(p_list,p_obj,links_name) do { \
          mhd_ASSUME (NULL != (p_list)->first); \
          mhd_ASSUME (NULL != (p_list)->last);  \
          mhd_ASSUME ((p_obj) != (p_obj)->links_name.next); \
          mhd_ASSUME ((p_obj) != (p_obj)->links_name.prev); \
          if (NULL == (p_obj)->links_name.prev)             \
          { mhd_ASSUME ((p_obj) == (p_list)->first); } else \
          { mhd_ASSUME ((p_obj) != (p_list)->first);        \
            mhd_ASSUME ((p_obj) ==                          \
                        (p_obj)->links_name.prev->links_name.next); \
            (p_obj)->links_name.prev->links_name.next = \
              (p_obj)->links_name.next;                 \
            if (NULL == (p_obj)->links_name.next)       \
            { mhd_ASSUME ((p_obj) == (p_list)->last);   \
              (p_list)->last = (p_obj)->links_name.prev; } else \
            { mhd_ASSUME ((p_obj) != (p_list)->last);   \
              mhd_ASSUME ((p_obj) ==                    \
                          (p_obj)->links_name.next->links_name.prev); \
              (p_obj)->links_name.next->links_name.prev = \
                (p_obj)->links_name.prev; }               \
            (p_obj)->links_name.next = (p_list)->first;   \
            (p_obj)->links_name.prev = NULL;              \
            (p_list)->first->links_name.prev = (p_obj);   \
            (p_list)->first = (p_obj); } } while (0)


/* ** The main interface ** */
/* These macros use identical names for the list field (within the owner
 * object) and the inter-links field (within the list elements). */

/* Initialisers */

/**
 * Initialise the doubly linked list pointers in the owner object
 * @warning arguments are evaluated multiple times
 * @param p_own the pointer to the owner object with the @a list_name list
 * @param list_name the name of the list
 */
#define mhd_DLINKEDL_INIT_LIST(p_own,list_name) \
        mhd_DLINKEDL_INIT_LIST_D (&((p_own)->list_name))

/**
 * Initialise the doubly linked list pointers in the list element
 * @warning arguments are evaluated multiple times
 * @param p_obj the pointer to the future element of
 *              the @a links_name list
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_INIT_LINKS(p_obj,links_name) \
        do {(p_obj)->links_name.prev = NULL;      \
            (p_obj)->links_name.next = NULL;} while (0)

/* List manipulations */

/**
 * Insert new list element into the first position in the list
 * @warning arguments are evaluated multiple times
 * @param p_own the pointer to the owner object with the @a l_name list
 * @param p_obj the pointer to the new list element to insert into
 *              the @a l_name list
 * @param l_name the same name for the list field in the owner and
 *               the inter-links field in the list element
 */
#define mhd_DLINKEDL_INS_FIRST(p_own,p_obj,l_name) \
        mhd_DLINKEDL_INS_FIRST_D (&((p_own)->l_name),(p_obj),l_name)

/**
 * Insert new list element into the last position in the list
 * @warning arguments are evaluated multiple times
 * @param p_own the pointer to the owner object with the @a l_name list
 * @param p_obj the pointer to the new list element to insert into
 *              the @a l_name list
 * @param l_name the same name for the list field in the owner and
 *               the inter-links field in the list element
 */
#define mhd_DLINKEDL_INS_LAST(p_own,p_obj,l_name) \
        mhd_DLINKEDL_INS_LAST_D (&((p_own)->l_name),(p_obj),l_name)

/**
 * Remove list element from the list
 * @warning arguments are evaluated multiple times
 * @param p_own the pointer to the owner object with the @a l_name list
 * @param p_obj the pointer to the existing @a l_name list element
 *              to remove from the list
 * @param l_name the same name for the list field in the owner and
 *               the inter-links field in the list element
 */
#define mhd_DLINKEDL_DEL(p_own,p_obj,l_name) \
        mhd_DLINKEDL_DEL_D (&((p_own)->l_name),(p_obj),l_name)

/* List iterations */

/**
 * Get the first element in the list
 * @param p_own the pointer to the owner object with the @a list_name list
 * @param list_name the name of the list
 */
#define mhd_DLINKEDL_GET_FIRST(p_own,list_name) \
        mhd_DLINKEDL_GET_FIRST_D (&((p_own)->list_name))

/**
 * Get the last element in the list
 * @param p_own the pointer to the owner object with the @a list_name list
 * @param list_name the name of the list
 */
#define mhd_DLINKEDL_GET_LAST(p_own,list_name) \
        mhd_DLINKEDL_GET_LAST_D (&((p_own)->list_name))

/**
 * Get the next element in the list
 * @param p_obj the pointer to the existing @a links_name list element
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_GET_NEXT(p_obj,links_name) ((p_obj)->links_name.next)

/**
 * Get the previous element in the list
 * @param p_obj the pointer to the existing @a links_name list element
 * @param links_name the name of the inter-links field in the @a p_obj
 */
#define mhd_DLINKEDL_GET_PREV(p_obj,links_name) ((p_obj)->links_name.prev)


#endif /* ! MHD_DLINKED_LIST_H */
