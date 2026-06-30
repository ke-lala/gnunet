/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/daemon_logger.h
 * @brief  The declaration of the logger function and relevant macros
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_DAEMON_LOGGER_H
#define MHD_DAEMON_LOGGER_H 1

#include "mhd_sys_options.h"

#ifdef MHD_SUPPORT_LOG_FUNCTIONALITY

#include "mhd_public_api.h" /* For enum MHD_StatusCode */

struct MHD_Daemon; /* Forward declaration */

/* Do not use this function directly, use wrapper macros below */
/**
 * The daemon logger function.
 * @param daemon the daemon handle
 * @param sc the status code of the event
 * @param fm the format string ('printf()'-style)
 * @ingroup logging
 */
MHD_INTERNAL void
mhd_logger (struct MHD_Daemon *daemon,
            enum MHD_StatusCode sc,
            const char *fm,
            ...);

/**
 * Log a single fixed message.
 *
 * The @a msg is a 'printf()' string, treated as format specifiers string.
 * Any '%' symbols should be doubled ('%%') to avoid interpretation as a format
 * specifier symbol.
 *
 * Note: no printf() parameters allowed (except the format string). To log
 *       message with variable parameters, use #mhd_LOG_PRINT with #mhd_LOG_FMT.
 */
#define mhd_LOG_MSG(daemon,sc,msg) mhd_logger (daemon,sc,msg)

/**
 * Define a message that will be used for logging.
 * This macro must be used for all log messages defined outside #mhd_LOG_MSG()
 * macro. For example, when defining a message that passed as a pointer
 * to the logging function.
 */
#define mhd_MSG4LOG(msg) msg

/**
 * Format message and log it.
 *
 * Always use with #mhd_LOG_FMT() for the format string.
 *
 * Example: mhd_LOG_PRINT(daemon, MHD_SC_VALUE, mhd_LOG_FMT("Number: %d"), i);
 */
#define mhd_LOG_PRINT mhd_logger

/**
 * The wrapper macro for the format string to be used for format parameter for
 * the #mhd_LOG_FMT() macro
 */
#define mhd_LOG_FMT(format_string) format_string

#else  /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */


#ifdef HAVE_MACRO_VARIADIC

/**
 * Log a single fixed message.
 *
 * The @a msg is a 'printf()' string, treated as format specifiers string.
 * Any '%' symbols should be doubled ('%%') to avoid interpretation as a format
 * specifier symbol.
 *
 * Note: no printf() parameters allowed (except the format string). To log
 *       message with variable parameters, use #mhd_LOG_PRINT with #mhd_LOG_FMT.
 */
#define mhd_LOG_MSG(daemon,sc,msg)  do { (void) daemon; } while (0)

/**
 * Format message and log it.
 *
 * Always use with #mhd_LOG_FMT() for the format string.
 *
 * Example: mhd_LOG_PRINT(daemon, MHD_SC_VALUE, mhd_LOG_FMT("Number: %d"), i);
 */
#define mhd_LOG_PRINT(daemon,sc,fm,...)  do { (void) daemon; } while (0)

#else  /* ! HAVE_MACRO_VARIADIC */

#include "sys_base_types.h" /* For NULL */

/**
 * Format message and log it
 *
 * Always use with #mhd_LOG_FMT() for the format string.
 */
mhd_static_inline void
mhd_LOG_PRINT (struct MHD_Daemon *daemon,
               enum MHD_StatusCode sc,
               const char *fm,
               ...)
{
  (void) daemon; (void) sc; (void) fm;
}


/**
 * Log a single message.
 *
 * The @a msg is a 'printf()' string, treated as format specifiers string.
 * Any '%' symbols should be doubled ('%%') to avoid interpretation as a format
 * specifier symbol.
 */
#define mhd_LOG_MSG(daemon,sc,msg) mhd_LOG_PRINT (daemon,sc,NULL)

#endif /* ! HAVE_MACRO_VARIADIC */

/**
 * Define a message that will be used for logging.
 * This macro must be used for all log messages defined outside #mhd_LOG_MSG()
 * macro. For example, when defining a message that passed as a pointer
 * to the logging function.
 */
#define mhd_MSG4LOG(msg) NULL

/**
 * The wrapper macro for the format string to be used for format parameter for
 * the #mhd_LOG_FMT() macro
 */
#define mhd_LOG_FMT(format_string) NULL

#endif /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */

#endif /* ! MHD_DAEMON_LOGGER_H */
