@node libmicrohttpd-response-performance
@subsection Response options for performance optimization

@anchor{MHD_R_OPTION_REUSABLE}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_REUSABLE ()

Make the response object re-usable.
The response will not be consumed by
@ref{MHD_action_from_response,,@code{MHD_action_from_response()}}
and must be destroyed by
@ref{MHD_response_destroy,,@code{MHD_response_destroy()}}
Useful if the same response is repeatedly used.
@table @var
@end table

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun

@anchor{MHD_R_OPTION_HEAD_ONLY_RESPONSE}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_HEAD_ONLY_RESPONSE ()

Enable special processing of the response as body-less (with undefined
body size). No automatic "Content-Length" or "Transfer-Encoding:
chunked" headers are added when the response is used with
@code{MHD_HTTP_STATUS_NOT_MODIFIED} code or to respond to HEAD
requests.

The flag also allows applications to set an arbitrary "Content-Length"
header using
@ref{MHD_response_add_header,,@code{MHD_response_add_header()}}.
This flag value can be used only with responses created with a body of
zero length.

Responses with this flag enabled cannot be used in situations where a
response body must be sent to the client.  This flag is primarily
intended to be used when automatic "Content-Length" header computation
by MHD is undesirable in response to HEAD requests.

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun



@node libmicrohttpd-response-compatibility
@subsection Response options to help with compatibility


@anchor{MHD_R_OPTION_CONN_CLOSE}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_CONN_CLOSE ()

Force MHD to close the connection after sending the response,
prevents keeping connections alive and adds a "Connection: close" header.

Returns the option to pass to @code{MHD_response_set_options()}
@end deftypefun

@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_HTTP_1_0_COMPATIBLE_STRICT ()

Only respond in conservative (dumb) HTTP/1.0-compatible mode.
Response still use HTTP/1.1 version in header, but always close the
connection after sending the response and do not use chunked encoding
for the response.  You can also set the #MHD_R_O_HTTP_1_0_SERVER flag
to force HTTP/1.0 version in the response.  Responses are still
compatible with HTTP/1.1.  This option thus ensures:

@itemize
@item declared reply version: HTTP/1.1
@item keep-alive: no
@item chunked encoding: no
@end itemize

This option can be used to communicate with some broken client
which does not implement HTTP/1.1 features but (falsely) advertises
support for HTTP/1.1.
@table @var
@end table

Returns the option to pass to @code{MHD_response_set_options()}
@end deftypefun


@anchor{MHD_R_OPTION_HTTP_1_0_SERVER}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_HTTP_1_0_SERVER ()

Only respond in HTTP/1.0-mode.
Contrary to the
@code{MHD_R_O_HTTP_1_0_COMPATIBLE_STRICT} flag, the
response's HTTP version will always be set to 1.0
and keep-alive connections will be used only if explicitly requested by the client.
The "Connection:" header will be added for both "close" and "keep-alive" connections.
Chunked encoding will not be used for the response.
Due to backward compatibility, HTTP/1.0-responses still can be used with HTTP/1.1 clients.
This option can be used to emulate an HTTP/1.0 server for a specific response
(for the response part only, as chunked encoding in requests (if any) is processed by
MHD before the application selects a response).
This option thus ensures:

@itemize
@item declared reply version: HTTP/1.0
@item keep-alive: possible
@item chunked encoding: no
@end itemize

With this option an HTTP/1.0 server is emulated (with support for "keep-alive" connections).
@table @var
@end table

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun


@node libmicrohttpd-response-observability
@subsection Response options for observability

@c FIXME: this should be a *request* option!

@anchor{MHD_R_OPTION_TERMINATION_CALLBACK}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_TERMINATION_CALLBACK (MHD_RequestEndedCallback ended_cb, void *ended_cb_cls)

Set a function to be called once MHD is finished with the request.

@table @var

@item ended_cb
the function to call,
@code{NULL} to not use the callback

@item ended_cb_cls
the closure for the callback
@end table

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun


@anchor{MHD_RequestEndedCallback}
@deftypefn {Function Pointer} void (*MHD_RequestEndedCallback) (void *cls, const struct MHD_RequestEndedData *req_data, void *request_app_context)

Signature of the callback used by MHD to notify the application about completed requests.

This is the last callback called for any request (if the
respective option is set by the application). The main
use-case is to clean up the application context.
 
@table @var
@c FIXME: we want to get rid of this argument...
@item cls
custom value provided by the application at callback registration time;

@item data
final data about the client's request;

@item request_app_contex
value from the location where the application
stored information about the request; @code{NULL}
if the application never stored any data there.

@end table
@end deftypefn

The @var{data} provides the following information
about the request:

@anchor{MHD_RequestEndedData}
@deftp {C Struct} struct MHD_RequestEndedData

Data for @ref{MHD_RequestEndedCallback}s.

@table @var

@item @code{struct MHD_Request *} request
The request handle.
Most of the request data may already be unavailable.

@item @code{enum MHD_RequestEndedCode} code
Code explaining how the request ended.

@item @code{union MHD_RequestEndedDetail} details
Further details about how the request ended, specifics
depend on @var{code}.

@end table
@end deftp


@anchor{MHD_RequestEndedCode}
@deftp {Enumeration} MHD_RequestEndedCode

The @code{enum MHD_RequestEndedCode} specifies reasons
why or how a request has been ended.

@table @code

@item MHD_REQUEST_ENDED_COMPLETED_OK
The response was successfully sent.

@item MHD_REQUEST_ENDED_COMPLETED_OK_UPGRADE
The response was successfully sent and connection is being switched to another protocol.

@item MHD_REQUEST_ENDED_TIMEOUT_REACHED
No activity on the connection for the number of seconds exceeding the timeout value.
@c FIXME: specified using @ref{MHD_C_OPTION_TIMEOUT}.

@item MHD_REQUEST_ENDED_CONNECTION_ERROR
The connection was broken or we had a TLS protocol error.

@item MHD_REQUEST_ENDED_CLIENT_ABORT
The client terminated the connection by closing the socket either completely or for writing (TCP half-closed) before sending the complete request.

@item MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR
The request was not valid according to HTTP specifications.

@item MHD_REQUEST_ENDED_BY_APP_ABORT
The application aborted handling the request without
specifying a response.

@item MHD_REQUEST_ENDED_BY_APP_ERROR
The request was aborted because the application failed to provide a valid response.

@item MHD_REQUEST_ENDED_NO_RESOURCES
MHD failed to handle the connection due to resource exhaustion.

@item MHD_REQUEST_ENDED_FILE_ERROR
The request was aborted due to an error reading the file (for responses backed by file descriptors).

@item MHD_REQUEST_ENDED_NONCE_ERROR
The request was aborted due to MHD failing to generate a valid nonce for digest authentication.

@item MHD_REQUEST_ENDED_DAEMON_SHUTDOWN
The request was terminated because MHD is closing the
underlying session as MHD is being shut down.

@end table
@end deftp


@anchor{MHD_RequestEndedDetail}
@deftp {C Union} struct MHD_RequestEndedDetail

@var{code}-specific details on how a request ended.
So far not used, only a placeholder.

@table @var

@item @code{void *} reserved
Placeholder. Do not used.

@end table
@end deftp



@node libmicrohttpd-response-testing
@subsection Response options for testing


@anchor{MHD_R_OPTION_CHUNKED_ENC}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_CHUNKED_ENC ()

Force use of chunked encoding even if the response content size is known.
Ignored when the response does not have a body.

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun


@anchor{MHD_R_OPTION_INSANITY_HEADER_CONTENT_LENGTH}
@deftypefun {struct MHD_ResponseOptionAndValue} MHD_R_OPTION_INSANITY_HEADER_CONTENT_LENGTH ()

Disable sanity checks that generally prevent clients from manually
setting the HTTP content length option.  Also allows applications to
set several "Content-Length" headers.  These headers will be returned
to the client even with replies without a body.

This option is useful when using MHD to test HTTP clients, but should
have no place in production.

Returns the option to pass to @code{MHD_response_set_options()}.
@end deftypefun
