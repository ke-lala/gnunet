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
 * @file test_raw.c
 * @brief tests streams against server
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

  /**
   * Expected HTTP method.
   */
  enum MHD_HTTP_Method expect_method;

  /**
   * Expected path.
   */
  const char *expect_path;

  /**
   * Expected upload size.
   */
  uint_fast64_t expect_upload_size;

  /**
   * Special string indicating what the MHD parser should give us.
   * Can be used to encode expected HTTP headers using
   * "H-$KEY:$VALUE" or HTTP cookies using "C-$KEY:$VALUE".
   * Multiple entries are separated via "\n".
   */
  const char *expect_parser;
};


static struct Test tests[] = {
  {
    .name = "Basic GET",
    .upload = "GET / HTTP/1.0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
  },
  {
    .name = "Basic HTTP/1.1 GET",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com",
  },
  {
    .name = "Multi-header HTTP/1.1 GET with query parameters",
    .upload = "GET /?k=v&a HTTP/1.1\r\nHost: example.com\r\nKey: value\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Key:value\nQ-k:v\nQ-a",
  },
  {
    .name = "Empty header value",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Empty:\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Empty:",
  },
  {
    .name = "Header with leading/trailing whitespace",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Space:  value  \r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Space:value",
  },
  {
    .name = "Multiple headers with same name",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Dup: first\r\nX-Dup: second\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Dup:first",
  },
  {
    .name = "Case insensitive header names",
    .upload =
      "GET / HTTP/1.1\r\nhost: example.com\r\nContent-TYPE: text/plain\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Content-Type:text/plain",
  },
  {
    .name = "Query parameter with empty value",
    .upload = "GET /?key= HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-key:",
  },
  {
    .name = "Query parameter without value",
    .upload = "GET /?flag HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-flag",
  },
  {
    .name = "Multiple query parameters",
    .upload = "GET /?a=1&b=2&c=3 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-a:1\nQ-b:2\nQ-c:3",
  },
  {
    .name = "URL encoded query parameter",
    .upload =
      "GET /?name=hello%20world&special=%21%40%23 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-name:hello world\nQ-special:!@#",
  },
  {
    .name = "Simple cookie",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: session=abc123\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-session:abc123",
  },
  {
    .name = "Multiple cookies in one header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: a=1; b=2; c=3\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-a:1\nC-b:2\nC-c:3",
  },
#if COMPATIBILITY_QUESTION
  {
    .name = "Cookie with spaces",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: key = value\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-key:value",
  },
#endif
  {
    .name = "POST with Content-Length",
    .upload =
      "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello",
    .expect_method = MHD_HTTP_METHOD_POST,
    .expect_path = "/submit",
    .expect_upload_size = 5,
    .expect_parser = "H-Host:example.com\nH-Content-Length:5",
  },
#if VERY_BAD_BUG
  {
    .name = "Path with special characters",
    .upload =
      "GET /path/to/resource%20file.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/path/to/resource file.html",
    .expect_parser = "H-Host:example.com",
  },
#endif
  {
    .name = "Long header value",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Long: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser =
      "H-Host:example.com\nH-X-Long:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  },
  {
    .name = "Query string with equals sign in value",
    .upload = "GET /?expr=a=b HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-expr:a=b",
  },
  {
    .name = "PATCH request",
    .upload =
      "PATCH /resource HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_OTHER,
    .expect_path = "/resource",
    .expect_upload_size = 0,
    .expect_parser = "H-Host:example.com\nH-Content-Length:0",
  },
  {
    .name = "Mixed query parameters with and without values",
    .upload =
      "GET /?debug&level=5&verbose HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-debug\nQ-level:5\nQ-verbose",
  },
#if PARSER_BUG
  {
    .name = "Continuation header (folded)",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Multi: line one\r\n line two\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Multi:line one line two",
  },
#endif
  {
    .name = "Empty query string",
    .upload = "GET /? HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com",
  },
#if UNDERSPECIFIED_LIKELY_BAD
  {
    .name = "Fragment in URI (should probably be stripped!)",
    .upload = "GET /page#section HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/page",
    .expect_parser = "H-Host:example.com",
  },
#endif
  {
    .name = "PUT request with headers",
    .upload =
      "PUT /resource HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nContent-Length: 0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_PUT,
    .expect_path = "/resource",
    .expect_upload_size = 0,
    .expect_parser =
      "H-Host:example.com\nH-Content-Type:application/json\nH-Content-Length:0",
  },
  {
    .name = "DELETE request",
    .upload = "DELETE /resource/123 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_DELETE,
    .expect_path = "/resource/123",
    .expect_parser = "H-Host:example.com",
  },

  {
    .name = "Header with colon in value",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Time: 12:34:56\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Time:12:34:56",
  },
  {
    .name = "Cookie with empty value",
    .upload = "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: empty=\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-empty:",
  },
  {
    .name = "Query parameter with plus sign encoding",
    .upload = "GET /?text=hello+world HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-text:hello world",
  },
  {
    .name = "Multiple Cookie headers",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: a=1\r\nCookie: b=2\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-a:1\nC-b:2",
  },
  {
    .name = "HEAD request",
    .upload = "HEAD /resource HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_HEAD,
    .expect_path = "/resource",
    .expect_parser = "H-Host:example.com",
  },
  {
    .name = "OPTIONS request",
    .upload = "OPTIONS * HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_OPTIONS,
    .expect_path = "*",
    .expect_parser = "H-Host:example.com",
  },
  {
    .name = "Query with ampersand in value",
    .upload = "GET /?code=a%26b HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-code:a&b",
  },
  {
    .name = "Query with percent sign in value",
    .upload = "GET /?percent=100%25 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-percent:100%",
  },
  {
    .name = "Root path with trailing slash",
    .upload = "GET // HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "//",
    .expect_parser = "H-Host:example.com",
  },
  {
    .name = "Deep path nesting",
    .upload = "GET /a/b/c/d/e/f/g HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/a/b/c/d/e/f/g",
    .expect_parser = "H-Host:example.com",
  },
  {
    .name = "Authorization header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nAuthorization: Bearer token123\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Authorization:Bearer token123",
  },
  {
    .name = "User-Agent header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nUser-Agent: Mozilla/5.0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-User-Agent:Mozilla/5.0",
  },
  {
    .name = "Accept header with multiple values",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nAccept: text/html, application/json\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Accept:text/html, application/json",
  },
  {
    .name = "Connection keep-alive",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: keep-alive\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Connection:keep-alive",
  },
  {
    .name = "Query with duplicate parameters",
    .upload = "GET /?id=1&id=2&id=3 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-id:1",
  },
  {
    .name = "Cookie with quoted value",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: session=\"abc123\"\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-session:abc123",
  },
  {
    .name = "Query with UTF-8 encoding",
    .upload = "GET /?name=%E2%9C%93 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-name:✓",
  },
  {
    .name = "Referer header",
    .upload =
      "GET /page HTTP/1.1\r\nHost: example.com\r\nReferer: https://google.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/page",
    .expect_parser = "H-Host:example.com\nH-Referer:https://google.com",
  },
  {
    .name = "Content-Type with charset",
    .upload =
      "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_POST,
    .expect_path = "/",
    .expect_upload_size = 0,
    .expect_parser =
      "H-Host:example.com\nH-Content-Type:text/html; charset=utf-8\nH-Content-Length:0",
  },
  {
    .name = "Cache-Control header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCache-Control: no-cache\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-Cache-Control:no-cache",
  },
  {
    .name = "If-None-Match header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nIf-None-Match: \"etag123\"\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-If-None-Match:\"etag123\"",
  },
  {
    .name = "Range header",
    .upload =
      "GET /file HTTP/1.1\r\nHost: example.com\r\nRange: bytes=0-1023\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/file",
    .expect_parser = "H-Host:example.com\nH-Range:bytes=0-1023",
  },
  {
    .name = "X-Forwarded-For header",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nX-Forwarded-For: 192.168.1.1\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nH-X-Forwarded-For:192.168.1.1",
  },
  {
    .name = "Query with empty parameter name",
    .upload = "GET /?=value HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-:value",
  },
#if QUESTIONABLE_TEST
  {
    .name = "Absolute URI in request line",
    .upload = "GET http://example.com/ HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com",
  },
#endif
  {
    .name = "Query with semicolon separator",
    .upload = "GET /?a=1;b=2 HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-a:1;b=2",
  },
  {
    .name = "Cookie with special characters",
    .upload =
      "GET / HTTP/1.1\r\nHost: example.com\r\nCookie: data=value_with-special.chars\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nC-data:value_with-special.chars",
  },
  {
    .name = "Minimal HTTP/1.0 request with path only",
    .upload = "GET /path HTTP/1.0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/path",
  },
  {
    .name = "POST with form data Content-Type",
    .upload =
      "POST /form HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 0\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_POST,
    .expect_path = "/form",
    .expect_upload_size = 0,
    .expect_parser =
      "H-Host:example.com\nH-Content-Type:application/x-www-form-urlencoded\nH-Content-Length:0",
  },
  {
    .name = "Query with trailing ampersand",
    .upload = "GET /?a=1& HTTP/1.1\r\nHost: example.com\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com\nQ-a:1",
  },
#if BUG
  {
    .name = "Absolute form",
    // RFC 9112 Section 3.2.2
    .upload = "GET http://example.com/ HTTP/1.1\r\nHost: example.bad\r\n\r\n",
    .expect_method = MHD_HTTP_METHOD_GET,
    .expect_path = "/",
    .expect_parser = "H-Host:example.com",
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


static bool
check_headers (struct MHD_Request *MHD_RESTRICT request,
               const char *spec)
{
  bool ok = true;
  char *hdr = strdup (spec);
  char *tok;
  char key[strlen (spec)];
  char value[strlen (spec)];

  for (tok = strtok (hdr,
                     "\n");
       NULL != tok;
       tok = strtok (NULL,
                     "\n"))
  {
    char dummy;

    if (2 ==
        sscanf (tok,
                "H-%[^:]:%[^\n]s",
                key,
                value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_HEADER,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected header `%s' missing\n",
                 key);
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              value)) )
      {
        fprintf (stderr,
                 "Wrong header value `%s' under key `%s', expected `%s'\n",
                 have.cstr,
                 key,
                 value);
        ok = false;
      }
    }
    else if (2 ==
             sscanf (tok,
                     "C-%[^:]:%[^\n]",
                     key,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_COOKIE,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected cookie `%s' missing\n",
                 key);
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              value)) )
      {
        fprintf (stderr,
                 "Wrong cookie value `%s' under key `%s', expected `%s'\n",
                 have.cstr,
                 key,
                 value);
        ok = false;
      }
    }
    else if (2 ==
             sscanf (tok,
                     "Q-%[^:]:%[^\n]",
                     key,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_URI_QUERY_PARAM,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected URI query parameter `%s' missing\n",
                 key);
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              value)) )
      {
        fprintf (stderr,
                 "Wrong URI query parameter value `%s' under key `%s', expected `%s'\n",
                 have.cstr,
                 key,
                 value);
        ok = false;
      }
    }
    else if (1 ==
             sscanf (tok,
                     "H-%[^:]:%[^\n]",
                     key,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_HEADER,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected header `%s' missing\n",
                 key);
        ok = false;
      } /* HTTP header can never be "NULL", only empty string */
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              "")) )
      {
        fprintf (stderr,
                 "Unexpected non-empty header value `%s' under key `%s'\n",
                 have.cstr,
                 key);
        ok = false;
      }
    }
    else if (2 ==
             sscanf (tok,
                     "C-%[^:]%c%[^\n]",
                     key,
                     &dummy,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_COOKIE,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected cookie `%s' missing\n",
                 key);
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              "")) )
      {
        fprintf (stderr,
                 "Unexpected non-empty cookie value `%s' under key `%s'\n",
                 have.cstr,
                 key);
        ok = false;
      }
    }
    else if (1 ==
             sscanf (tok,
                     "C-%[^:]:%[^\n]",
                     key,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_COOKIE,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected cookie `%s' missing\n",
                 key);
        ok = false;
      }
      else if (NULL != have.cstr)
      {
        fprintf (stderr,
                 "Unexpected non-NULL cookie value `%s' under key `%s'\n",
                 have.cstr,
                 key);
        ok = false;
      }
    }
    else if (2 ==
             sscanf (tok,
                     "Q-%[^:]%c%[^\n]",
                     key,
                     &dummy,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_URI_QUERY_PARAM,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected URI query parameter `%s' missing\n",
                 key);
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              "")) )
      {
        fprintf (stderr,
                 "Unexpected non-empty URI query parameter value `%s' under key `%s'\n",
                 have.cstr,
                 key);
        ok = false;
      }
    }
    else if (1 ==
             sscanf (tok,
                     "Q-%[^:]:%[^\n]",
                     key,
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_URI_QUERY_PARAM,
                                   key,
                                   &have))
      {
        fprintf (stderr,
                 "Expected URI query parameter `%s' missing\n",
                 key);
        ok = false;
      }
      else if (NULL != have.cstr)
      {
        fprintf (stderr,
                 "Unexpected non-NULL URI query parameter value `%s' under key `%s'\n",
                 have.cstr,
                 key);
        ok = false;
      }
    }
    else if (1 ==
             sscanf (tok,
                     "Q-:%[^\n]",
                     value))
    {
      struct MHD_StringNullable have;

      if (! MHD_request_get_value (request,
                                   MHD_VK_URI_QUERY_PARAM,
                                   "",
                                   &have))
      {
        fprintf (stderr,
                 "Expected URI query parameter without key missing\n");
        ok = false;
      }
      else if ( (NULL == have.cstr) ||
                (0 != strcmp (have.cstr,
                              value)) )
      {
        fprintf (stderr,
                 "Wrong URI query parameter value `%s' under missing key, expected `%s'\n",
                 have.cstr,
                 value);
        ok = false;
      }
    }
    else
    {
      fprintf (stderr,
               "Invalid token `%s' in test specification\n",
               tok);
      ok = false;
    }
  }
  free (hdr);
  return ok;
}


/**
 * A client has requested the given url using the given method
 * (#MHD_HTTP_METHOD_GET, #MHD_HTTP_METHOD_PUT,
 * #MHD_HTTP_METHOD_DELETE, #MHD_HTTP_METHOD_POST, etc).
 * If @a upload_size is not zero and response action is provided by this
 * callback, then upload will be discarded and the stream (the connection for
 * HTTP/1.1) will be closed after sending the response.
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
  bool failed = false;

  if (current->expect_method != method)
  {
    fprintf (stderr,
             "Wrong HTTP method\n");
    failed = true;
  }
  if (0 != strcmp (current->expect_path,
                   path->cstr))
  {
    fprintf (stderr,
             "Wrong HTTP path %s\n",
             path->cstr);
    failed = true;
  }
  if (current->expect_upload_size !=
      upload_size)
  {
    fprintf (stderr,
             "Wrong HTTP path %s\n",
             path->cstr);
    failed = true;
  }
  if ( (NULL != current->expect_parser) &&
       (! check_headers (request,
                         current->expect_parser)) )
    failed = true;
  if (failed)
    return NULL;
  return MHD_action_from_response (
    request,
    MHD_response_from_empty (MHD_HTTP_STATUS_NO_CONTENT));
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
  if (sizeof (dummy) !=
      read (s,
            &dummy,
            sizeof (dummy)))
  {
    fprintf (stderr,
             "Server closed connection due to error!\n");
    close (s);
    return 1;
  }
  close (s);
  return 0;
}


static int
run_tests (void)
{
  for (unsigned int i = 0;
       NULL != tests[i].name;
       i++)
  {
    current = &tests[i];
    if (0 != run_test ())
    {
      fprintf (stderr,
               "Test `%s' failed\n",
               current->name);
      return 1;
    }
  }
  return 0;
}


int
main (void)
{
  struct MHD_Daemon *d;

  d = MHD_daemon_create (&server_req_cb,
                         NULL);
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (
        d,
        MHD_D_OPTION_WM_WORKER_THREADS (2),
        MHD_D_OPTION_DEFAULT_TIMEOUT_MILSEC (1500),
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

    result = run_tests ();
    MHD_daemon_destroy (d);
    return result;
  }
}
