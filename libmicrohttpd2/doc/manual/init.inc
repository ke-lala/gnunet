This chapter explains the key functions of MHD to create, start,
quiesce and destroy an HTTP server, as well as how to configure
global logging to receive more detailed messages when something
goes really wrong inside of MHD.

@node libmicrohttpd-init-panic
@section Setting up a panic handler
@cindex logging

@code{MHD_PanicCallback} is the type of a function that MHD
calls when it encounters an error that it cannot handle. This
should always be indicative of a serious bug, be it a hardware
failure (such as a bit-flip in RAM or a broken CPU) or a bug
in the application (for example, memory corruption or a
failed assertion). MHD will not cause a panic in case of
normal operation or when there would be a safe way to continue
the execution.

@anchor{MHD_PanicCallback}
@deftypefn {Function Pointer} {enum MHD_Result} (*MHD_PanicCallback) (void *cls, const char *file, const char *func, unsigned int line, const char *message)

Invoked in the context of a serious error condition. The callback should not return. Some parameters could be empty strings (with zero-termination at zero position),
especially if MHD is built without support for log messages.
@table @var
@item cls
custom value provided by the application at callback registration time;

@item file
name of the source file where the error occured;

@item func
function in the source where the error occured;

@item line
line in the source file where the error occured;

@item message
message describing the error;
@end table
@end deftypefn

MHD comes with a default panic action, which is to print an error
message and call @code{abort()} to terminate the process.
Applications can use @code{MHD_lib_set_panic_func()} to change this
behavior. However, applications must always terminate the process
inside of the panic callback (but they could use @code{exit()} or
@code{_exit()} or some other way to terminate the process instead
of calling @code{abort()}). Note that
the panic function is @emph{expected} to be called in a situation
where the process is in an @emph{unsafe} state, thus even the default
behavior of calling @code{fprintf()} could be dangerous as it may
enable an attacker to further exploit whatever bug caused the panic in
the first place.

@deftypefun {void} MHD_lib_set_panic_func (MHD_PanicCallback cb, void *cb_cls)
Set a handler for fatal errors.

@table @var
@item cb
function to call if MHD encounters a fatal internal error.  If no handler was set explicitly, MHD will log an error and call @code{abort()}.

@item cb_cls
closure argument for @var{cb}; the other arguments are the name of the source file, line number and a string describing the nature of the fatal error (which can be @code{NULL})
@end table
@end deftypefun

@node libmicrohttpd-init-create
@section Creating an HTTP daemon

@cindex daemon

@anchor{MHD_Daemon}
@deftp {C Struct} MHD_Daemon

Handle for an HTTP daemon. A daemon contains the MHD configuration
and state to listening on a socket for HTTP clients and handle
their requests.
@end deftp

An application can in principle create multiple daemons, for example
to listen on both port 80 and port 443. Each daemon handles requests
from at most one @code{listen()} socket.
The @code{MHD_daemon_create()} function is used by applications to
create a @code{struct MHD_Daemon}.

@anchor{MHD_daemon_create}
@deftypefun {struct MHD_Daemon *} MHD_daemon_create (MHD_RequestCallback req_cb, void *req_cb_cls)
Create a new HTTP daemon object. Does not actually start
the HTTP daemon.
@table @var
@item req_cb
Function to call to handle each HTTP request;

@item req_cb_cls
closure to pass to the @var{req_cb};
@end table

Returns @code{NULL} on error (usually out-of-memory),
handle to daemon on success.
@end deftypefun

When creating an HTTP daemon the application must pass the address of
a function of type @code{MHD_RequestCallback} which MHD will call for
each HTTP request that daemon receives from the network.

@cindex anchor
@anchor{MHD_RequestCallback}
@deftypefn {Function Pointer} {struct MHD_Action *} (*MHD_RequestCallback) (void *cls, struct MHD_Request *request, const struct MHD_String *path, enum MHD_HTTP_Method method, uint_fast64_t upload_size)

Functions of this type are invoked by MHD whenever it received an HTTP
request and needs to handle it.

@table @var
@item cls
custom value provided by the application to @code{MHD_daemon_create()};

@item path
the PATH component of the HTTP URL requested by the client. Empty
if the request was just for ``http://domain``, otherwise
starting with ``/``;

@item method
the HTTP method used by the client;

@item upload_size
number of bytes of data that the client intends to upload
in the request body; a special value of @code{MHD_SIZE_UNKNOWN}
is provided if the client indicated the use of chunked encoding
and the final upload size is not yet known;
@end table

The returned @code{struct MHD_Action} determines how MHD will continue
handling the request. Implementations must call MHD
functions to create a @code{struct MHD_Action}, which then determines
the next steps.

@ref{libmicrohttpd2-actions,,Using actions} explains in more detail how to create
various possible actions. It is also possible to return @code{NULL},
in which case MHD will close the HTTP connection without returning
anything (not even an error message).  This can be used if the socket
must be closed due to a serious error while handling the request (such
as being out-of-memory).
@end deftypefn

The HTTP @var{method} is not provided as a string as comparing
strings is generally inefficient for the application. Furthermore,
by providing an enumeration value applications often can avoid
implementing case-insensitive ASCII string comparissons. However,
the enumeration only works for HTTP methods specified in RFC 9110.

@cindex method
@anchor{MHD_HTTP_Method}
@deftp {Enumeration} MHD_HTTP_Method

Represents canonical HTTP methods as per RFC 9110.

@table @code
@item MHD_HTTP_METHOD_GET
 "GET". Safe. Idempotent.  RFC9110, Section 9.3.1.

@item MHD_HTTP_METHOD_HEAD
"HEAD". Safe. Idempotent.  RFC9110, Section 9.3.2.

@item MHD_HTTP_METHOD_POST
"POST". Not safe. Not idempotent. RFC9110, Section 9.3.3.

@item MHD_HTTP_METHOD_PUT
"PUT". Not safe. Idempotent. RFC9110, Section 9.3.4.

@item MHD_HTTP_METHOD_DELETE
"DELETE". Not safe. Idempotent. RFC9110, Section 9.3.5.

@item MHD_HTTP_METHOD_CONNECT
"CONNECT". Not safe. Not idempotent. RFC9110, Section 9.3.6.

@item MHD_HTTP_METHOD_OPTIONS
"OPTIONS". Safe. Idempotent. RFC9110, Section 9.3.7.

@item MHD_HTTP_METHOD_TRACE
"TRACE". Safe.  Idempotent. RFC9110, Section 9.3.8.

@item MHD_HTTP_METHOD_ASTERISK
 "*". Not safe. Not idempotent. RFC9110, Section 18.2.

@item MHD_HTTP_METHOD_OTHER
Non-canonical HTTP method. If an application wants to
support non-canonical HTTP methods, the application
must request the method's value by passing
@code{MHD_REQUEST_INFO_DYNAMIC_HTTP_METHOD_STR}
to @code{MHD_request_get_info_dynamic()}.
@xref{libmicrohttpd2-info request,,request introspection}.

@end table
@end deftp

The convenience function @code{MHD_http_method_to_string()}
can be used to convert members of this enumeration to
a string value.

@anchor{MHD_http_method_to_string}
@deftypefun {const struct MHD_String *} MHD_http_method_to_string (enum MHD_HTTP_Method method)
Get text version of the method name.

Returns a pointer (!) to the HTTP method as a string.
@code{NULL} is returned if @var{method} is @code{MHD_HTTP_METHOD_OTHER}.

@table @var
@item method
the method to get the text version for

@end table
@end deftypefun

Various additional string constants for HTTP methods are defined in
the @code{microhttpd2.h} header for HTTP methods from a wide range of
RFCs. Check the header for constants with the prefix
@code{MHD_HTTP_METHOD_STR_}.

@node libmicrohttpd-init-start
@section Starting an HTTP daemon

After creating an MHD daemon, applications will typically configure
various optional features, such as support for TLS.
@ref{libmicrohttpd2-doptions,,Daemon options} explains how to set the various options.
Once the daemon is correctly configured, applications can start
processing client requests using @code{MHD_daemon_start()}.

@anchor{MHD_daemon_start}
@deftypefun {enum MHD_StatusCode} MHD_daemon_start (struct MHD_Daemon *daemon)

Starts an HTTP daemon. Checks that options are consistent, initializes
the TLS library (if enabled), creates the listen socket (if not
disabled) and launches the internal threads (if configured to do so).

@table @var
@item daemon
Handle to the HTTP daemon to start. Must be configured
but not have been started previously.
@end table

Returns @code{MHD_SC_OK} on success, otherwise an error code indicative of the problem.
@end deftypefun

Note that if you configured the daemon to use an
@emph{external} event loop, calling @code{MHD_daemon_start()}
is not sufficient to actually start processing client requests.
In this case, you must also integrate MHD into your event loop.
@xref{libmicrohttpd2-external,,Using external event loops}.

@node libmicrohttpd-status-codes
@section Interpreting status codes
@cindex status code

MHD uses the @code{enum MHD_StatusCode} to inform the application
about errors.

@anchor{MHD_StatusCode}
@deftp {Enumeration} MHD_StatusCode
Status codes are returned to indicate to the
application the success or failure of an
operation.  The value ranges have a meaning:

@itemize
@item from 0 and 10000
  must be handled explicitly by the app.
@item from 10000-19999
  are informational.
@item from 20000-29999
  indicate successful operations.
@item from 30000-39999
 indicate unsuccessful (normal) operations.
@item from 40000-49999
 indicate client errors.
@item from 50000-59999
 indicate MHD server errors.
@item from 60000-65535
 indicate application errors.
@end itemize

The most common value is @code{MHD_SC_OK} (0)
which indicates a successful operation.
@end deftp

To obtain a human-readable message explaining the status code,
applications can use @code{MHD_status_code_to_string()}.

@anchor{MHD_status_code_to_string}
@deftypefun {const struct MHD_String *} MHD_status_code_to_string (struct MHD_StatusCode code)

Get text description for the MHD status code.
This function works for MHD status codes, not for HTTP status codes!

@table @var
@item code
the MHD code to get a description for
@end table

The function returns a pointer to the text description for the
status code, or @code{NULL} if @var{code} is not known.

@end deftypefun

The convenience macro @code{MHD_status_code_to_string_lazy()} converts
an @code{enum MHD_StatusCode} directly to a @code{const char *}. It
also ensures that the result is never @code{NULL} by returning
@code{"[No code]"} instead of @code{NULL}.


@node libmicrohttpd-init-quiesce
@section Quiescing an HTTP daemon
@cindex quiesce

Quiescing an HTTP daemon prevents it from further accepting new
connections, but allows it to finish handling existing clients.  This
is useful to minimize the disruptions when gracefully shutting down,
restarting or reloading an HTTP service.

@anchor{MHD_daemon_quiesce}
@deftypefun MHD_Socket MHD_daemon_quiesce (struct MHD_Daemon *daemon)
Stop accepting connections from the listening socket.  Allows clients
to continue processing, but stops accepting new connections.

@table @var
@item daemon
Handle to the HTTP daemon to quiesce. The daemon should have been
started before.
@end table

The returned socket is the listen socket of the daemon.  This value
can be useful in the special case that a listen socket is to be
migrated to another process (i.e. a newer version of the HTTP server).
The caller is made responsible for ultimately closing the returned
socket; however, if MHD is run using threads (anything but external
select mode), it @strong{must not} be closed until @emph{after}
@code{MHD_daemon_destroy()} has been called (as it is theoretically
possible that an existing thread is still using it).

A value of @code{-1} is returned if either the daemon was not
started or was not listening on a socket.
@end deftypefun

@cindex socket
@anchor{MHD_Socket}
@deftp {Typedef} MHD_Socket

The type @code{MHD_Socket} is platform-dependent:
@code{MHD_Socket} is simply an @code{int} on UNIX systems, while on
W32 it is a @code{SOCKET}.
@end deftp


@node libmicrohttpd-init-destroy
@section Destroying an HTTP daemon

Destroying an HTTP daemon using @code{MHD_daemon_destroy()}
immediately stops all MHD processing of associated requests and frees
all associated resources. Applications do @emph{not} have to call
@code{MHD_daemon_quiesce()} before destroying a daemon: if the daemon
was not quiesced first, the listen socket will still be closed and all
ongoing requests will simply be terminated hard by closing the
connections.

@anchor{MHD_daemon_destroy}
@deftypefun void MHD_daemon_destroy (struct MHD_Daemon *daemon)

Shutdown and destroy an HTTP daemon. Frees all
resources still associated with the daemon.

@table @var
@item daemon
Handle to the HTTP daemon to destroy.
@end table
@end deftypefun


@node libmicrohttpd-init-example
@section Example: Starting with GNU libmicrohttpd

The following is a minimal starting point for implementing
an application that uses MHD to add an HTTP server:

@example c
@verbatiminclude examples/init-example.c
@end example