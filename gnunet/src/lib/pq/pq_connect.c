/*
   This file is part of GNUnet
   Copyright (C) 2017, 2019, 2020, 2021, 2023 GNUnet e.V.

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
 * @file pq/pq_connect.c
 * @brief functions to connect to libpq (PostGres)
 * @author Christian Grothoff
 * @author Özgür Kesim
 */
#include "platform.h"
#include "pq.h"
#include <pthread.h>


/**
 * Close connection to @a db and mark it as uninitialized.
 *
 * @param[in,out] db connection to close
 */
static void
reset_connection (struct GNUNET_PQ_Context *db)
{
  if (NULL == db->conn)
    return;
  PQfinish (db->conn);
  db->conn = NULL;
  db->prepared_check_patch = false;
  db->prepared_get_oid_by_name = false;
}


/**
 * Prepare the "gnunet_pq_check_patch" statement.
 *
 * @param[in,out] db database to prepare statement for
 * @return #GNUNET_OK on success,
 *         #GNUNET_SYSERR on failure
 */
static enum GNUNET_GenericReturnValue
prepare_check_patch (struct GNUNET_PQ_Context *db)
{
  PGresult *res;

  if (db->prepared_check_patch)
    return GNUNET_OK;
  res = PQprepare (db->conn,
                   "gnunet_pq_check_patch",
                   "SELECT"
                   " applied_by"
                   " FROM _v.patches"
                   " WHERE patch_name = $1"
                   " LIMIT 1",
                   1,
                   NULL);
  if (PGRES_COMMAND_OK !=
      PQresultStatus (res))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to run SQL logic to setup database versioning logic: %s/%s\n",
                PQresultErrorMessage (res),
                PQerrorMessage (db->conn));
    PQclear (res);
    reset_connection (db);
    return GNUNET_SYSERR;
  }
  PQclear (res);
  db->prepared_check_patch = true;
  return GNUNET_OK;
}


/**
 * Prepare the "gnunet_pq_get_oid_by_name" statement.
 *
 * @param[in,out] db database to prepare statement for
 * @return #GNUNET_OK on success,
 *         #GNUNET_SYSERR on failure
 */
static enum GNUNET_GenericReturnValue
prepare_get_oid_by_name (struct GNUNET_PQ_Context *db)
{
  PGresult *res;

  if (db->prepared_get_oid_by_name)
    return GNUNET_OK;
  res = PQprepare (db->conn,
                   "gnunet_pq_get_oid_by_name",
                   "SELECT typname, oid"
                   "  FROM pg_type"
                   " WHERE oid = to_regtype($1)",
                   1,
                   NULL);
  if (PGRES_COMMAND_OK != PQresultStatus (res))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to run SQL statement prepare OID lookups: %s/%s\n",
                PQresultErrorMessage (res),
                PQerrorMessage (db->conn));
    PQclear (res);
    reset_connection (db);
    return GNUNET_SYSERR;
  }
  PQclear (res);
  db->prepared_get_oid_by_name = true;
  return GNUNET_OK;
}


/**
 * Check if the "_v" versioning schema exists (and cache
 * the result in @e db).
 *
 * @param[in,out] db connection to check
 * @return #GNUNET_OK if versioning is OK
 *         #GNUNET_NO if it is not loaded
 *         #GNUNET_SYSERR if we encountered an error
 */
static enum GNUNET_GenericReturnValue
check_versioning_ok (struct GNUNET_PQ_Context *db)
{
  PGresult *res;
  ExecStatusType est;

  if (GNUNET_OK == db->versioning_ok)
    return GNUNET_OK;
  if (GNUNET_NO == db->versioning_ok)
    return GNUNET_NO;
  res = PQexec (db->conn,
                "SELECT"
                " schema_name"
                " FROM information_schema.schemata"
                " WHERE schema_name='_v';");
  est = PQresultStatus (res);
  if ( (PGRES_COMMAND_OK != est) &&
       (PGRES_TUPLES_OK != est) )
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to run statement to check versioning schema. Bad!\n");
    PQclear (res);
    return GNUNET_SYSERR;
  }
  if (0 == PQntuples (res))
  {
    PQclear (res);
    db->versioning_ok = GNUNET_NO;
    GNUNET_log (GNUNET_ERROR_TYPE_WARNING,
                "_v schema not found\n");
    return GNUNET_NO;
  }
  PQclear (res);
  db->versioning_ok = GNUNET_OK;
  return GNUNET_OK;
}


/**
 * Check if the patch with @a patch_number from the given
 * @a load_path was already applied on the @a db.
 *
 * @param[in] db database to check
 * @param load_path file system path to database setup files
 * @param patch_number number of the patch to check
 * @return #GNUNET_OK if patch is applied
 *         #GNUNET_NO if patch is not applied
 *         #GNUNET_SYSERR on internal error (DB failure)
 */
static enum GNUNET_GenericReturnValue
check_patch_applied (struct GNUNET_PQ_Context *db,
                     const char *load_path,
                     unsigned int patch_number)
{
  const char *load_path_suffix;
  size_t slen = strlen (load_path) + 10;
  char patch_name[slen];

  if (GNUNET_SYSERR ==
      check_versioning_ok (db))
  {
    GNUNET_break (0);
    return GNUNET_SYSERR; /* no versioning, cannot check */
  }
  load_path_suffix = strrchr (load_path,
                              '/');
  if (NULL == load_path_suffix)
    load_path_suffix = load_path;
  else
    load_path_suffix++; /* skip '/' */
  GNUNET_snprintf (patch_name,
                   sizeof (patch_name),
                   "%s%04u",
                   load_path_suffix,
                   patch_number);
  {
    struct GNUNET_PQ_QueryParam params[] = {
      GNUNET_PQ_query_param_string (patch_name),
      GNUNET_PQ_query_param_end
    };
    char *applied_by;
    struct GNUNET_PQ_ResultSpec rs[] = {
      GNUNET_PQ_result_spec_string ("applied_by",
                                    &applied_by),
      GNUNET_PQ_result_spec_end
    };
    enum GNUNET_DB_QueryStatus qs;

    if (GNUNET_OK !=
        prepare_check_patch (db))
    {
      GNUNET_break (0);
      return GNUNET_SYSERR;
    }
    qs = GNUNET_PQ_eval_prepared_singleton_select (db,
                                                   "gnunet_pq_check_patch",
                                                   params,
                                                   rs);
    switch (qs)
    {
    case GNUNET_DB_STATUS_SUCCESS_ONE_RESULT:
      GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
                  "Database version %s already applied by %s\n",
                  patch_name,
                  applied_by);
      GNUNET_PQ_cleanup_result (rs);
      return GNUNET_OK;
    case GNUNET_DB_STATUS_SUCCESS_NO_RESULTS:
      return GNUNET_NO;
    case GNUNET_DB_STATUS_SOFT_ERROR:
      GNUNET_break (0);
      return GNUNET_SYSERR;
    case GNUNET_DB_STATUS_HARD_ERROR:
      GNUNET_break (0);
      return GNUNET_SYSERR;
    }
    GNUNET_assert (0);
    return GNUNET_SYSERR;
  }
}


/**
 * Function called by libpq whenever it wants to log something.
 * We already log whenever we care, so this function does nothing
 * and merely exists to silence the libpq logging.
 *
 * @param arg the SQL connection that was used
 * @param res information about some libpq event
 */
static void
pq_notice_receiver_cb (void *arg,
                       const PGresult *res)
{
  /* do nothing, intentionally */
  (void) arg;
  (void) res;
}


/**
 * Function called by libpq whenever it wants to log something.
 * We log those using the GNUnet logger.
 *
 * @param arg the SQL connection that was used
 * @param message information about some libpq event
 */
static void
pq_notice_processor_cb (void *arg,
                        const char *message)
{
  (void) arg;
  GNUNET_log_from (GNUNET_ERROR_TYPE_INFO,
                   "pq",
                   "%s",
                   message);
}


enum GNUNET_GenericReturnValue
GNUNET_PQ_exec_sql (struct GNUNET_PQ_Context *db,
                    const char *buf)
{
  struct GNUNET_Process *psql;
  enum GNUNET_OS_ProcessStatusType type;
  unsigned long code;
  char *fn;

  GNUNET_asprintf (&fn,
                   "%s%s.sql",
                   db->load_path,
                   buf);
  if (GNUNET_YES !=
      GNUNET_DISK_file_test_read (fn))
  {
    GNUNET_free (fn);
    return GNUNET_NO;
  }
  GNUNET_log (GNUNET_ERROR_TYPE_INFO,
              "Applying SQL file `%s' on database %s\n",
              fn,
              db->config_str);
  psql = GNUNET_process_create (GNUNET_OS_INHERIT_STD_NONE);
  if (GNUNET_OK !=
      GNUNET_process_run_command_va (psql,
                                     "psql",
                                     "psql",
                                     db->config_str,
                                     "-f",
                                     fn,
                                     "-q",
                                     "--set",
                                     "ON_ERROR_STOP=1",
                                     NULL))
  {
    GNUNET_log_strerror_file (GNUNET_ERROR_TYPE_ERROR,
                              "exec",
                              "psql");
    GNUNET_process_destroy (psql);
    GNUNET_free (fn);
    return GNUNET_SYSERR;
  }
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_process_wait (psql,
                                      true,
                                      &type,
                                      &code));
  GNUNET_process_destroy (psql);
  if ( (GNUNET_OS_PROCESS_EXITED != type) ||
       (0 != code) )
  {
    GNUNET_log (GNUNET_ERROR_TYPE_WARNING,
                "Could not run PSQL on file %s: psql exit code was %d\n",
                fn,
                (int) code);
    GNUNET_free (fn);
    return GNUNET_SYSERR;
  }
  GNUNET_free (fn);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_PQ_check_current (struct GNUNET_PQ_Context *db,
                         const char *load_suffix)
{
  char *fn;

  GNUNET_log (GNUNET_ERROR_TYPE_INFO,
              "Loading SQL resources from `%s'\n",
              load_suffix);
  for (unsigned int i = 1; i<10000; i++)
  {
    enum GNUNET_GenericReturnValue ret;

    ret = check_patch_applied (db,
                               load_suffix,
                               i);
    if (GNUNET_SYSERR == ret)
    {
      GNUNET_break (0);
      return GNUNET_SYSERR;
    }
    if (GNUNET_OK  == ret)
      continue; /* patch already applied, skip it */
    /* patch not applied, check if it exists... */
    GNUNET_asprintf (&fn,
                     "%s%s%04u",
                     db->load_path,
                     load_suffix,
                     i);
    ret = GNUNET_DISK_file_test_read (fn);
    GNUNET_free (fn);
    if (GNUNET_YES == ret)
      return GNUNET_NO;
    return GNUNET_OK;
  }
  GNUNET_break (0); /* 10k patches applied!? */
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_PQ_run_sql (struct GNUNET_PQ_Context *db,
                   const char *load_suffix)
{
  size_t slen = strlen (load_suffix) + 10;
  char patch_name[slen];

  GNUNET_log (GNUNET_ERROR_TYPE_INFO,
              "Loading SQL resources from `%s'\n",
              load_suffix);
  for (unsigned int i = 1; i<10000; i++)
  {
    enum GNUNET_GenericReturnValue ret;

    ret = check_patch_applied (db,
                               load_suffix,
                               i);
    if (GNUNET_SYSERR == ret)
    {
      GNUNET_break (0);
      return GNUNET_SYSERR;
    }
    if (GNUNET_OK  == ret)
      continue; /* patch already applied, skip it */

    GNUNET_snprintf (patch_name,
                     sizeof (patch_name),
                     "%s%04u",
                     load_suffix,
                     i);
    ret = GNUNET_PQ_exec_sql (db,
                              patch_name);
    if (GNUNET_NO == ret)
      break; /* no such file, we are done */
    if (GNUNET_SYSERR == ret)
      return GNUNET_SYSERR;
  }
  return GNUNET_OK;
}


void
GNUNET_PQ_reconnect_if_down (struct GNUNET_PQ_Context *db)
{
  if (1 ==
      PQconsumeInput (db->conn))
    return;
  if (CONNECTION_BAD != PQstatus (db->conn))
    return;
  GNUNET_PQ_reconnect_ (db);
}


enum GNUNET_GenericReturnValue
GNUNET_PQ_get_oid_by_name (
  struct GNUNET_PQ_Context *db,
  const char *name,
  Oid *oid)
{
  /* Check if the entry is in the cache already */
  for (unsigned int i = 0; i < db->oids.num; i++)
  {
    /* Pointer comparison */
    if (name == db->oids.table[i].name)
    {
      *oid = db->oids.table[i].oid;
      return GNUNET_OK;
    }
  }

  /* No entry found in cache, ask database */
  {
    enum GNUNET_DB_QueryStatus qs;
    struct GNUNET_PQ_QueryParam params[] = {
      GNUNET_PQ_query_param_string (name),
      GNUNET_PQ_query_param_end
    };
    struct GNUNET_PQ_ResultSpec spec[] = {
      GNUNET_PQ_result_spec_uint32 ("oid",
                                    oid),
      GNUNET_PQ_result_spec_end
    };

    GNUNET_assert (NULL != db);

    qs = GNUNET_PQ_eval_prepared_singleton_select (db,
                                                   "gnunet_pq_get_oid_by_name",
                                                   params,
                                                   spec);
    if (GNUNET_DB_STATUS_SUCCESS_ONE_RESULT != qs)
      return GNUNET_SYSERR;
  }

  /* Add the entry to the cache */
  if (NULL == db->oids.table)
  {
    db->oids.table = GNUNET_new_array (8,
                                       typeof(*db->oids.table));
    db->oids.cap = 8;
    db->oids.num = 0;
  }

  if (db->oids.cap <= db->oids.num)
    GNUNET_array_grow (db->oids.table,
                       db->oids.cap,
                       db->oids.cap + 8);

  db->oids.table[db->oids.num].name = name;
  db->oids.table[db->oids.num].oid = *oid;
  db->oids.num++;

  return GNUNET_OK;
}


/**
 * Load the initial set of OIDs for the supported
 * array-datatypes
 *
 * @param db The database context
 * @return #GNUNET_OK on success,
 *         #GNUNET_SYSERR if any of the types couldn't be found
 */
static enum GNUNET_GenericReturnValue
load_initial_oids (struct GNUNET_PQ_Context *db)
{
  static const char *typnames[] = {
    "bool",
    "int2",
    "int4",
    "int8",
    "bytea",
    "varchar"
  };
  Oid oid;

  for (size_t i = 0; i< sizeof(typnames) / sizeof(*typnames); i++)
  {
    if (GNUNET_OK !=
        GNUNET_PQ_get_oid_by_name (db,
                                   typnames[i],
                                   &oid))
    {
      GNUNET_log_from (GNUNET_ERROR_TYPE_ERROR,
                       "pq",
                       "Couldn't retrieve OID for type %s\n",
                       typnames[i]);
      return GNUNET_SYSERR;
    }
  }
  return GNUNET_OK;
}


void
GNUNET_PQ_reconnect_ (struct GNUNET_PQ_Context *db)
{
  GNUNET_PQ_event_reconnect_ (db,
                              -1);
  reset_connection (db);
  db->versioning_ok = GNUNET_SYSERR; /* new connection, new game */
  db->conn = PQconnectdb (db->config_str);
  if ( (NULL == db->conn) ||
       (CONNECTION_OK != PQstatus (db->conn)) )
  {
    GNUNET_log_from (GNUNET_ERROR_TYPE_ERROR,
                     "pq",
                     "Database connection to '%s' failed: %s\n",
                     db->config_str,
                     (NULL != db->conn)
                     ? PQerrorMessage (db->conn)
                     : "PQconnectdb returned NULL");
    reset_connection (db);
    return;
  }
  PQsetNoticeReceiver (db->conn,
                       &pq_notice_receiver_cb,
                       db);
  PQsetNoticeProcessor (db->conn,
                        &pq_notice_processor_cb,
                        db);
  if (NULL != db->rc)
    db->rc (db->rc_cls,
            db);

  /* Prepare statement for OID lookup by name */
  if (GNUNET_OK !=
      prepare_get_oid_by_name (db))
    return;

  /* Reset the OID-cache and retrieve the OIDs for the supported Array types */
  db->oids.num = 0;
  if (GNUNET_SYSERR == load_initial_oids (db))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to retrieve OID information for array types!\n");
    reset_connection (db);
    return;
  }
  GNUNET_PQ_event_reconnect_ (db,
                              PQsocket (db->conn));
}


enum GNUNET_GenericReturnValue
GNUNET_PQ_load_versioning (struct GNUNET_PQ_Context *db)
{
  enum GNUNET_GenericReturnValue ret;

  ret = check_versioning_ok (db);
  if (GNUNET_SYSERR == ret)
    return GNUNET_SYSERR;
  if (GNUNET_YES == ret)
    return GNUNET_NO; /* already setup */
  ret = GNUNET_PQ_exec_sql (db,
                            "versioning");
  if (GNUNET_NO == ret)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to find SQL file to load database versioning logic\n");
    return GNUNET_SYSERR;
  }
  if (GNUNET_SYSERR == ret)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Failed to run SQL logic to setup database versioning logic\n");
    return GNUNET_SYSERR;
  }
  db->versioning_ok = GNUNET_YES;
  return GNUNET_OK;
}


struct GNUNET_PQ_Context *
GNUNET_PQ_init (const struct GNUNET_CONFIGURATION_Handle *cfg,
                const char *section,
                GNUNET_PQ_ReconnectCallback rc,
                GNUNET_PQ_RECONNECT_CALLBACK_CLOSURE *rc_cls)
{
  struct GNUNET_PQ_Context *db;
  char *conninfo;
  char *load_path;

  if (GNUNET_OK !=
      GNUNET_CONFIGURATION_get_value_string (cfg,
                                             section,
                                             "CONFIG",
                                             &conninfo))
    conninfo = GNUNET_strdup ("");
  load_path = NULL;
  if (GNUNET_OK !=
      GNUNET_CONFIGURATION_get_value_filename (cfg,
                                               section,
                                               "SQL_DIR",
                                               &load_path))
  {
    GNUNET_log_config_missing (GNUNET_ERROR_TYPE_INFO,
                               section,
                               "SQL_DIR");
  }
  db = GNUNET_new (struct GNUNET_PQ_Context);
  db->versioning_ok = GNUNET_SYSERR;
  db->config_str = conninfo;
  db->load_path = load_path;
  db->rc = rc;
  db->rc_cls = rc_cls;
  db->channel_map = GNUNET_CONTAINER_multishortmap_create (16,
                                                           true);
  GNUNET_PQ_reconnect_ (db);
  if (NULL == db->conn)
  {
    GNUNET_CONTAINER_multishortmap_destroy (db->channel_map);
    GNUNET_free (db->config_str);
    GNUNET_free (db);
    return NULL;
  }
  return db;
}


void
GNUNET_PQ_disconnect (struct GNUNET_PQ_Context *db)
{
  if (NULL == db)
    return;
  GNUNET_assert (0 ==
                 GNUNET_CONTAINER_multishortmap_size (db->channel_map));
  GNUNET_CONTAINER_multishortmap_destroy (db->channel_map);
  if (NULL != db->poller_task)
  {
    GNUNET_SCHEDULER_cancel (db->poller_task);
    db->poller_task = NULL;
  }
  GNUNET_free (db->load_path);
  GNUNET_free (db->config_str);
  GNUNET_free (db->oids.table);
  db->oids.num = 0;
  db->oids.cap = 0;
  PQfinish (db->conn);
  GNUNET_free (db);
}


/* end of pq/pq_connect.c */
