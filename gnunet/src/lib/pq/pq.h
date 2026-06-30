/*
   This file is part of GNUnet
   Copyright (C) 2017, 2019 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

     SPDX-License-Identifier: AGPL3.0-or-later
 */
/**
 * @file pq/pq.h
 * @brief shared internal data structures of libgnunetpq
 * @author Christian Grothoff
 */
#ifndef PQ_H
#define PQ_H

#include "gnunet_util_lib.h"
#include "gnunet_pq_lib.h"


/**
 * Handle to Postgres database.
 */
struct GNUNET_PQ_Context
{
  /**
   * Actual connection.
   */
  PGconn *conn;

  /**
   * Function to call whenever we needed to reconnect @e conn.
   */
  GNUNET_PQ_ReconnectCallback rc;

  /**
   * Closure for @e rc.
   */
  GNUNET_PQ_RECONNECT_CALLBACK_CLOSURE *rc_cls;

  /**
   * Configuration to use to connect to the DB.
   */
  char *config_str;

  /**
   * Path to load SQL files from.
   */
  char *load_path;

  /**
   * Map managing event subscriptions.
   */
  struct GNUNET_CONTAINER_MultiShortmap *channel_map;

  /**
   * Task responsible for polling events directly after
   * having posted an event.
   */
  struct GNUNET_SCHEDULER_Task *poller_task;

  /**
   * Task responsible for processing events.
   */
  struct GNUNET_SCHEDULER_Task *event_task;

  /**
   * File descriptor wrapper for @e event_task.
   */
  struct GNUNET_NETWORK_Handle *rfd;

  /**
   * How fast should we resubscribe again?
   */
  struct GNUNET_TIME_Relative resubscribe_backoff;

  /**
   * Set to #GNUNET_OK if versioning schema is properly setup,
   * #GNUNET_NO if it is definitively not setup,
   * #GNUNET_SYSERR if we do not know.
   */
  enum GNUNET_GenericReturnValue versioning_ok;

  /**
   * Did we prepare the gnunet_pq_check_patch statement?
   */
  bool prepared_check_patch;

  /**
   * Did we prepare the gnunet_pq_get_oid_by_name statement?
   */
  bool prepared_get_oid_by_name;

  /**
   * Mapping between array types and Oid's, pre-filled at reconnect.
   * More entries are captured in via GNUNET_PQ_get_oid_by_name.
   */
  struct
  {
    /* Allocated number of elements array the table */
    unsigned int cap;

    /* Number of entries in the table */
    unsigned int num;

    /* The table of (name, oid) pairs.
     * Note that the names are 'const char *' and the pointers should be point
     * to the same string throughout the lifetime of the program.*/
    struct name2oid
    {
      const char *name;
      Oid oid;
    } *table;

  } oids;
};


/**
 * Internal types that are supported as array types.
 */

enum array_types
{
  array_of_bool,
  array_of_uint16,
  array_of_uint32,
  array_of_uint64,
  array_of_byte,      /* buffers of (char *), (void *), ... */
  array_of_string,    /* NULL-terminated (char *) */
  array_of_abs_time,
  array_of_rel_time,
  array_of_timestamp,
  array_of_MAX,       /* must be last */
};

/**
 * The header for a postgresql array in binary format. Note that this a
 * simplified special case of the general structure (which contains pointers),
 * as we only support one-dimensional arrays.
 */
struct pq_array_header
{
  uint32_t ndim;       /* number of dimensions. we only support ndim = 1 */
  uint32_t has_nulls; /* not zero if array contains NULL elements */
  uint32_t oid;
  uint32_t dim;        /* size of the array */
  uint32_t lbound;     /* index value of first element in the db (default: 1). */
} __attribute__((packed));


/**
 * Reinitialize the database @a db.
 *
 * @param db database connection to reinitialize
 * @deprecated
 */
void
GNUNET_PQ_reconnect_ (struct GNUNET_PQ_Context *db);


/**
 * Internal API. Reconnect should re-register notifications
 * after a disconnect.
 *
 * @param db the DB handle
 * @param fd socket to listen on
 */
void
GNUNET_PQ_event_reconnect_ (struct GNUNET_PQ_Context *db,
                            int fd);


#endif
