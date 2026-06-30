/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff

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
 * @file test_incompatible.c
 * @brief tests server rejects incorrect or non-standard requests, either
 *   those that are:
 *   - incompatible to MUST requirements, or
 *   - non-standard and violate SHOULD requirements
 * @author Christian Grothoff
 */
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "microhttpd2.h"

#define LOG 0

/**
 * Defines a test.
 */
struct Test
{
  /**
   * Human-readable name of the test. NULL to end test array.
   */
  const char *name;

  /**
   * Request to send to the server.
   */
  const char *upload;

};


/**
 * Tests with HTTP requests that violate MUST constraints
 * of the HTTP specifications.
 *
 * For example, using a bare LF instead of CRLF is forbidden, and
 * requests that include both a "Transfer-Encoding:" and a
 * "Content-Length:" headers are rejected.
 */
static struct Test tests_must[] = {
  {
    .name = "HTTP 1.1 without Host",
    .upload = "GET / HTTP/1.1\r\n\r\n",
  },
  {
    .name = "HTTP 1.0 GET without CRLF",
    .upload = "GET / HTTP/1.0\n\n",
  },
  {
    .name = "POST with both Content-Length and Transfer-Encoding",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n",
  },
  {
    .name = "unsupported Ttransfer-Encoding",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: wild\r\n\r\n0\r\n",
  },
  {
    .name = "Invalid HTTP version format",
    .upload = "GET / HTTP/1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: HTTP-version must be "HTTP/" followed by two digits separated by "."
  },
  {
    .name = "Missing space after method",
    .upload = "GET/ HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3: Request-line requires SP between method and request-target
  },
  {
    .name = "Invalid request-target with space",
    .upload = "GET /path with space HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2: Request-target must not contain unencoded spaces
  },
  {
    .name = "Header field name with space",
    .upload = "GET / HTTP/1.1\r\nHost Name: example.com\r\n\r\n",
    // RFC 9110 Section 5.1: Field names must be tokens (no spaces allowed)
  },
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Header field name with colon",
    .upload = "GET / HTTP/1.1\r\nHost:Name: example.com\r\n\r\n",
    // RFC 9110 Section 5.1: Field names must be tokens (colons not allowed)
  },
#endif
  {
    .name = "Missing colon after header field name",
    .upload = "GET / HTTP/1.1\r\nHost example.com\r\n\r\n",
    // RFC 9112 Section 5: Header field must have name, colon, and value
  },
  {
    .name = "Header line ending with bare CR",
    .upload = "GET / HTTP/1.1\rHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.2: Lines must end with CRLF, not bare CR
  },
  {
    .name = "Request line ending with bare LF",
    .upload = "GET / HTTP/1.1\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.2: Request-line must end with CRLF
  },
  {
    .name = "Multiple Host headers",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nHost: other.com\r\n\r\n",
    // RFC 9112 Section 3.2: A sender MUST NOT generate multiple Host header fields
  },
  {
    .name = "Negative Content-Length",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: -5\r\n\r\n",
    // RFC 9110 Section 8.6: Content-Length value must be non-negative decimal integer
  },
  {
    .name = "Non-numeric Content-Length",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: abc\r\n\r\n",
    // RFC 9110 Section 8.6: Content-Length must be a decimal integer
  },
  {
    .name = "Multiple Content-Length with different values",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\n",
    // RFC 9110 Section 8.6: Multiple Content-Length values must be identical
  },
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Invalid method with control character",
    .upload = "GET\x01 / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9110 Section 9.1: Method token must not contain control characters
  },
#endif
  {
    .name = "Request-target starting with space",
    .upload = "GET  / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3: Only single SP allowed between method and request-target
  },
  {
    .name = "HTTP/0.9 simple request with headers",
    .upload = "GET /\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: HTTP/0.9 requests must not have headers
  },
  {
    .name = "Missing final CRLF after headers",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\n",
    // RFC 9112 Section 6.1: Empty line (CRLF) required after headers
  },
  {
    .name = "Whitespace before header field name",
    .upload = "GET / HTTP/1.1\r\n Host: example.com\r\n\r\n",
    // RFC 9112 Section 5: No whitespace allowed before field name
  },


  {
    .name = "Empty request line",
    .upload = "\r\n\r\n",
    // RFC 9112 Section 3: Request-line is required
  },
  {
    .name = "Request line with only method",
    .upload = "GET\r\n\r\n",
    // RFC 9112 Section 3: Request-line must have method, target, and version
  },
  {
    .name = "Request line with only method and target",
    .upload = "GET /\r\n\r\n",
    // RFC 9112 Section 3: HTTP-version is required in request-line
  },
  {
    .name = "Request with only CR as line ending",
    .upload = "GET / HTTP/1.1\rHost: example.com\r\r",
    // RFC 9112 Section 2.2: CRLF required, not bare CR
  },
  {
    .name = "Missing space before HTTP version",
    .upload = "GET /HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3: SP required between request-target and HTTP-version
  },
  {
    .name = "HTTP version with extra dot",
    .upload = "GET / HTTP/1.1.0\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: Version format is "HTTP/" DIGIT "." DIGIT
  },
  {
    .name = "HTTP version with letter",
    .upload = "GET / HTTP/1.A\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: Version numbers must be digits
  },
#ifdef NOT_A_BUG
  {
    .name = "Method with lowercase letters",
    .upload = "get / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9110 Section 9.1: Method is case-sensitive
    /* This a valid non-standard ("custom") method. */
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request-target with fragment identifier",
    .upload = "GET /path#fragment HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2.1: Fragment must not be sent in request-target
  },
#endif
#ifdef OTHER_USES
  {
    .name = "Absolute-form with userinfo in HTTP/1.1",
    .upload =
      "GET http://user:pass@example.com/ HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9110 Section 4.2.4: Userinfo (and its "@" delimiter) is now disallowed
    /* This is a valid string for HTTP proxy */
  },
#endif
  {
    .name = "Request-target with bare CR",
    .upload = "GET /path\r/file HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2: Request-target must not contain CR
  },
  {
    .name = "Request-target with LF",
    .upload = "GET /path\n/file HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2: Request-target must not contain LF
  },
  {
    .name = "Header colon without field name",
    .upload = "GET / HTTP/1.1\r\n: value\r\nHost: example.com\r\n\r\n",
    // RFC 9110 Section 5.1: Field name is required before colon
  },
  {
    .name = "Content-Length with plus sign",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: +10\r\n\r\n0123456789",
    // RFC 9110 Section 8.6: Content-Length must be 1*DIGIT (no sign allowed)
  },
  {
    .name = "Content-Length with whitespace",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 1 0\r\n\r\n0123456789",
    // RFC 9110 Section 8.6: Content-Length must be digits only
  },
  {
    .name = "Content-Length with decimal point",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 10.0\r\n\r\n0123456789",
    // RFC 9110 Section 8.6: Content-Length must be decimal integer, not floating point
  },
  {
    .name = "Content-Length overflow value",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 99999999999999999999999999999\r\n\r\n",
    // RFC 9110 Section 8.6: Content-Length must be valid decimal integer
  },
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with vertical tab in header",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Custom:\vvalue\r\n\r\n",
    // RFC 9110 Section 5.5: Only HTAB, SP, and VCHAR allowed in field values (VT is 0x0B)
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with form feed in header value",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Custom: val\fue\r\n\r\n",
    // RFC 9110 Section 5.5: Form feed (0x0C) not allowed in field values
  },
#endif
  {
    .name = "Transfer-Encoding with unknown coding",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: gzip\r\n\r\n",
    // RFC 9112 Section 6.1: Only 'chunked' and specific codings defined
  },
  {
    .name = "Transfer-Encoding chunked not last",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked, gzip\r\n\r\n",
    // RFC 9112 Section 6.1: 'chunked' must be last when present
  },
#ifdef SPECIAL_STRICT_PEER_CHECKING
  {
    .name = "HTTP/1.0 with Transfer-Encoding",
    .upload =
      "POST / HTTP/1.0\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n",
    // RFC 9112 Section 6.1: Transfer-Encoding must not be sent in HTTP/1.0
    /* RFC does not enforce the server to reject such requests. "Must not be sent" != "Must be rejected" */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "POST without Content-Length or Transfer-Encoding",
    .upload = "POST / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 6.3: Message with body must have Content-Length or Transfer-Encoding
    /* This is a perfectly valid request with an empty body. */
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with CTL character in header name",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Cu\x01stom: value\r\n\r\n",
    // RFC 9110 Section 5.1: Field names must be tokens, CTL chars not allowed
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with DEL character in header name",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Custom\x7F: value\r\n\r\n",
    // RFC 9110 Section 5.1: DEL (0x7F) not allowed in field names
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with non-ASCII in header name",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Cüstom: value\r\n\r\n",
    // RFC 9110 Section 5.1: Field names must be ASCII tokens
  },
#endif
#ifdef SPECIAL_STRICT_PEER_CHECKING
  {
    .name = "Request-target as asterisk for non-OPTIONS",
    .upload = "GET * HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2.4: Asterisk form only valid for OPTIONS
  },
#endif
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Authority-form for non-CONNECT",
    .upload = "GET example.com:80 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3.2.3: Authority-form only valid for CONNECT
  },
#endif
  {
    .name = "HTTP/1.1 GET with empty Host header",
    .upload = "GET / HTTP/1.1\r\nHost: \r\n\r\n",
    // RFC 9112 Section 3.2: Empty Host header is invalid for this target form
  },
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Request with quoted string in field name",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\n\"X-Custom\": value\r\n\r\n",
    // RFC 9110 Section 5.1: Field names must be tokens, not quoted strings
  },
#endif
  {
    .name = "HTTP/1.1 request with HTTP/1.2 version",
    .upload = "GET / HTTP/1.2\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: HTTP/1.2 is not defined
  },
  {
    .name = "HTTP version HTTP/2.0 on HTTP/1.1 connection",
    .upload = "GET / HTTP/2.0\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 2.3: HTTP/2 uses different framing, not text-based
  },
  {
    .name = "Empty method",
    .upload = " / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3: Method is required
  },
#ifdef MORE_PEER_VALIDATION
  {
    .name = "Method with special character",
    .upload = "GET/ / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    // RFC 9110 Section 9.1: Method must be a token (no '/' in method name)
  },
#endif
  {
    .name = "Triple Host headers",
    .upload =
      "GET / HTTP/1.1\r\nHost: a.com\r\nHost: b.com\r\nHost: c.com\r\n\r\n",
    // RFC 9112 Section 3.2: Multiple Host headers forbidden
  },
  {
    .name = "Content-Length with hexadecimal",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0x10\r\n\r\n0123456789012345",
    // RFC 9110 Section 8.6: Content-Length must be decimal, not hexadecimal
  },
  {
    .name = NULL,
  }
};


/**
 * Tests with HTTP requests that violate MUST constraints
 * of the HTTP specifications during the upload.
 */
static struct Test tests_must_upload[] = {
  {
    .name = "Chunked encoding with invalid hex",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\nGG\r\n\r\n",
    // RFC 9112 Section 7.1: Chunk size must be hexadecimal
  },
  {
    .name = "Chunked encoding with negative size",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n-5\r\n\r\n",
    // RFC 9112 Section 7.1: Chunk size must be non-negative hex
  },
  {
    .name = "Chunked encoding missing CRLF after size",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5hello\r\n0\r\n\r\n",
    // RFC 9112 Section 7.1: CRLF required after chunk-size
  },
  {
    .name = "Chunked encoding missing CRLF after data",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello0\r\n\r\n",
    // RFC 9112 Section 7.1: CRLF required after chunk-data
  },
  {
    .name = "Chunked with no final chunk",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n",
    // RFC 9112 Section 7.1: Last chunk (size 0) is required
  },
  {
    .name = "Chunk size with leading whitespace",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n 5\r\nhello\r\n0\r\n\r\n",
    // RFC 9112 Section 7.1: No whitespace before chunk-size
  },
  {
    .name = "Chunk size exceeding data provided",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\nA\r\nhello\r\n0\r\n\r\n",
    // RFC 9112 Section 7.1: Chunk-size must match chunk-data length (10 bytes expected, 5 provided)
  },
  {
    .name = NULL,
  }
};


/**
 * Tests with HTTP requests that violate SHOULD constraints
 * of the HTTP specifications.
 *
 * For example, for chunked encoding, this level (and more restrictive
 * ones) forbids whitespace in chunk extensions.  For cookie parsing,
 * this level (and more restrictive ones) rejects the entire cookie if
 * even a single value within it is incorrectly encoded.
 */
static struct Test tests_should[] = {
  {
    .name = "Obsolete line folding in header",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\n continuation\r\n\r\n",
    // RFC 9112 Section 5.2: Line folding is obsolete, recipients SHOULD reject or replace with SP
  },
#ifdef NOT_A_BUG
  {
    .name = "Multiple spaces after colon in header",
    .upload = "GET / HTTP/1.1\r\nHost:     example.com\r\n\r\n",
    // RFC 9110 Section 5.6.3: Senders SHOULD NOT generate optional whitespace except as SP
    /* RFC 9110 Section 5.6.3: "OWS and RWS have the same semantics as a single SP."
     * RFC 9112 Section 5.1: "A field line value might be preceded and/or followed by optional whitespace (OWS)"
     * The server must parse this value as a correct value. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Trailing whitespace in header value",
    .upload = "GET / HTTP/1.1\r\nHost: example.com   \r\n\r\n",
    // RFC 9110 Section 5.3: Trailing whitespace should be stripped
    /* RFC 9112 Section 5.1: "OWS occurring before the first non-whitespace octet of the field line value,
                              or after the last non-whitespace octet of the field line value, is excluded
                              by parsers when extracting the field line value from a field line."
     * This is a valid request. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Leading whitespace in header value",
    .upload = "GET / HTTP/1.1\r\nHost:    example.com\r\n\r\n",
    // RFC 9110 Section 5.3: Leading whitespace should be stripped
    /* RFC 9112 Section 5.1: "OWS occurring before the first non-whitespace octet of the field line value,
                              or after the last non-whitespace octet of the field line value, is excluded
                              by parsers when extracting the field line value from a field line."
     * This is a valid request. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Chunk extension with whitespace",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5 ; ext=val\r\nhello\r\n0\r\n\r\n",
    // RFC 9112 Section 7.1.1: Whitespace in chunk extensions should be minimal
    /* This is a valid request. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Chunked with uppercase hex digits",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
    // RFC 9112 Section 7.1: Senders SHOULD use lowercase for hex digits (though uppercase is valid)
    /* This is a valid request. It must be parsed. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Empty header field value",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Empty:\r\n\r\n",
    // RFC 9110 Section 5.3: Empty field values are valid but some implementations may reject
    /* This is a valid request. It must be parsed. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Header field name with uppercase and lowercase",
    .upload = "GET / HTTP/1.1\r\nHoSt: example.com\r\n\r\n",
    // RFC 9110 Section 5.1: Field names are case-insensitive, but conventional capitalization SHOULD be used
    /* This is a valid request. It must be parsed. */
  },
#endif
  {
    .name = "Excessive whitespace in request line",
    .upload = "GET / HTTP/1.1 \r\nHost: example.com\r\n\r\n",
    // RFC 9112 Section 3: Trailing whitespace in request-line should not be present
  },
#ifdef NOT_A_BUG
  {
    .name = "Content-Length with leading zeros",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0005\r\n\r\nhello",
    // RFC 9110 Section 8.6: Leading zeros should not be sent
    /* This is a valid request. It must be parsed. */
  },
#endif
#ifdef NOT_A_BUG
  {
    .name = "Cookie header with invalid encoding in value",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: name=val ue\r\n\r\n",
    // RFC 6265 Section 4.2: Cookie values should be properly encoded, spaces require encoding
    /* Rejected completely on stricter level. On default level valid part "name=val" is used. */
  },
#endif
  {
    .name = NULL,
  }
};


static struct Test *current;


/**
 * Our port.
 */
static uint16_t port;

/**
 * Set to true if a test failed.
 */
static volatile bool failed;


/**
 * Function to process data uploaded by a client.
 *
 * Given that we ONLY generate incorrect/malformed upload requests,
 * this function should never be called.
 *
 * @param upload_cls the argument given together with the function
 *                   pointer when the handler was registered with MHD
 * @param request the request is being processed
 * @param content_data_size the size of the @a content_data,
 *                          zero when all data have been processed
 * @param[in] content_data the uploaded content data,
 *                         may be modified in the callback,
 *                         valid only until return from the callback,
 *                         NULL when all data have been processed
 * @return action specifying how to proceed:
 *         #MHD_upload_action_continue() to continue upload (for incremental
 *         upload processing only),
 *         #MHD_upload_action_suspend() to stop reading the upload until
 *         the request is resumed,
 *         #MHD_upload_action_abort_request() to close the socket,
 *         or a response to discard the rest of the upload and transmit
 *         the response
 * @ingroup action
 */
static const struct MHD_UploadAction *
uc_fail (void *upload_cls,
         struct MHD_Request *request,
         size_t content_data_size,
         void *content_data)
{
  fprintf (stderr,
           "Test `%s' failed\n",
           current->name);
  failed = true;
  return NULL;
}


/**
 * A client has requested the given url using the given method
 * (#MHD_HTTP_METHOD_GET, #MHD_HTTP_METHOD_PUT,
 * #MHD_HTTP_METHOD_DELETE, #MHD_HTTP_METHOD_POST, etc).
 * If @a upload_size is not zero and response action is provided by this
 * callback, then upload will be discarded and the stream (the connection for
 * HTTP/1.1) will be closed after sending the response.
 *
 * This function is expected to be called, but when we try to
 * process the upload it should always fail on the MHD side.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param request the request object
 * @param path the requested uri (without arguments after "?")
 * @param method the HTTP method used (#MHD_HTTP_METHOD_GET,
 *        #MHD_HTTP_METHOD_PUT, etc.)
 * @param upload_size the size of the message upload content payload,
 *                    #MHD_SIZE_UNKNOWN for chunked uploads (if the
 *                    final chunk has not been processed yet)
 * @return action how to proceed, NULL
 *         if the request must be aborted due to a serious
 *         error while handling the request (implies closure
 *         of underling data stream, for HTTP/1.1 it means
 *         socket closure).
 */
static const struct MHD_Action *
server_upload_req_cb (void *cls,
                      struct MHD_Request *MHD_RESTRICT request,
                      const struct MHD_String *MHD_RESTRICT path,
                      enum MHD_HTTP_Method method,
                      uint_fast64_t upload_size)
{
  return MHD_action_process_upload (request,
                                    1024 * 1024,
                                    &uc_fail,
                                    NULL,
                                    &uc_fail,
                                    NULL);
}


/**
 * A client has requested the given url using the given method
 * (#MHD_HTTP_METHOD_GET, #MHD_HTTP_METHOD_PUT,
 * #MHD_HTTP_METHOD_DELETE, #MHD_HTTP_METHOD_POST, etc).
 * If @a upload_size is not zero and response action is provided by this
 * callback, then upload will be discarded and the stream (the connection for
 * HTTP/1.1) will be closed after sending the response.
 *
 * Given that we ONLY generate incorrect/malformed requests,
 * this function should never be called.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param request the request object
 * @param path the requested uri (without arguments after "?")
 * @param method the HTTP method used (#MHD_HTTP_METHOD_GET,
 *        #MHD_HTTP_METHOD_PUT, etc.)
 * @param upload_size the size of the message upload content payload,
 *                    #MHD_SIZE_UNKNOWN for chunked uploads (if the
 *                    final chunk has not been processed yet)
 * @return action how to proceed, NULL
 *         if the request must be aborted due to a serious
 *         error while handling the request (implies closure
 *         of underling data stream, for HTTP/1.1 it means
 *         socket closure).
 */
static const struct MHD_Action *
server_req_cb (void *cls,
               struct MHD_Request *MHD_RESTRICT request,
               const struct MHD_String *MHD_RESTRICT path,
               enum MHD_HTTP_Method method,
               uint_fast64_t upload_size)
{
  fprintf (stderr,
           "Test `%s' failed\n",
           current->name);
  failed = true;
  return NULL;
}


/**
 * Helper function to deal with partial writes.
 * Fails hard (calls exit() on failures)!
 *
 * @param fd where to write to
 * @param buf what to write
 * @param buf_size number of bytes in @a buf
 */
static void
write_all (int fd,
           const void *buf,
           size_t buf_size)
{
  const char *cbuf = buf;
  size_t off;

  off = 0;
  while (off < buf_size)
  {
    ssize_t ret;

    ret = write (fd,
                 &cbuf[off],
                 buf_size - off);
    if (ret <= 0)
    {
      fprintf (stderr,
               "Writing %u bytes to %d failed: %s\n",
               (unsigned int) (buf_size - off),
               fd,
               strerror (errno));
      exit (1);
    }
    off += ret;
  }
}


static int
run_test ()
{
  int s;
  struct sockaddr_in sa = {
    .sin_family = AF_INET,
    .sin_port = htons (port),
  };
  char dummy;

  s = socket (AF_INET, SOCK_STREAM, 0);
  if (-1 == s)
  {
    fprintf (stderr,
             "socket() failed: %s\n",
             strerror (errno));
    return 1;
  }
  inet_pton (AF_INET,
             "127.0.0.1",
             &sa.sin_addr);
  if (0 != connect (s,
                    (struct sockaddr *) &sa,
                    sizeof (sa)))
  {
    fprintf (stderr,
             "bind() failed: %s\n",
             strerror (errno));
    close (s);
    return 1;
  }
  write_all (s,
             current->upload,
             strlen (current->upload));
  shutdown (s,
            SHUT_WR);
  if (((ssize_t) sizeof (dummy)) !=
      read (s,
            &dummy,
            sizeof (dummy)))
  {
#if LOG
    fprintf (stderr,
             "Server closed connection\n");
#endif
  }
  close (s);
  if (failed)
    return 1;
  return 0;
}


static int
run_tests (struct Test *tests)
{
  for (unsigned int i = 0;
       NULL != tests[i].name;
       i++)
  {
    current = &tests[i];
#if LOG || 1
    fprintf (stderr,
             "Running test `%s'\n",
             current->name);
#endif
    if (0 != run_test ())
      return 1;
  }
  return 0;
}


static int
run (MHD_RequestCallback cb,
     struct Test *tests,
     enum MHD_ProtocolStrictLevel psl)
{
  struct MHD_Daemon *d;

  d = MHD_daemon_create (cb,
                         NULL);
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (
        d,
        MHD_D_OPTION_WM_WORKER_THREADS (2),
#if ! LOG
        MHD_D_OPTION_LOG_CALLBACK (NULL,
                                   NULL),
#endif
        MHD_D_OPTION_PROTOCOL_STRICT_LEVEL (psl,
                                            MHD_USL_PRECISE),
        MHD_D_OPTION_DEFAULT_TIMEOUT_MILSEC (1000),
        MHD_D_OPTION_BIND_PORT (MHD_AF_AUTO,
                                0)))
  {
    fprintf (stderr,
             "Failed to configure daemon!");
    return 1;
  }

  {
    enum MHD_StatusCode sc;

    sc = MHD_daemon_start (d);
    if (MHD_SC_OK != sc)
    {
#ifdef FIXME_STATUS_CODE_TO_STRING_NOT_IMPLEMENTED
      fprintf (stderr,
               "Failed to start server: %s\n",
               MHD_status_code_to_string_lazy (sc));
#else
      fprintf (stderr,
               "Failed to start server: %u\n",
               (unsigned int) sc);
#endif
      MHD_daemon_destroy (d);
      return 1;
    }
  }

  {
    union MHD_DaemonInfoFixedData info;
    enum MHD_StatusCode sc;

    sc = MHD_daemon_get_info_fixed (
      d,
      MHD_DAEMON_INFO_FIXED_BIND_PORT,
      &info);
    if (MHD_SC_OK != sc)
    {
      fprintf (stderr,
               "Failed to determine our port: %u\n",
               (unsigned int) sc);
      MHD_daemon_destroy (d);
      return 1;
    }
    port = info.v_bind_port_uint16;
  }

  {
    int result;

    result = run_tests (tests);
    MHD_daemon_destroy (d);
    return result;
  }
}


int
main (void)
{
  if (0 !=
      run (&server_req_cb,
           tests_must,
           MHD_PSL_STRICT))
    return 1;
  if (0 !=
      run (&server_upload_req_cb,
           tests_must_upload,
           MHD_PSL_STRICT))
    return 1;
  if (0 !=
      run (&server_req_cb,
           tests_should,
           MHD_PSL_VERY_STRICT))
    return 1;
  return 0;
}
