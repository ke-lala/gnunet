@node libmicrohttpd-daemon-workmode
@section Configuring the work mode

The work mode option determines how MHD should process connections
concurrently. The work mode is set by providing a @code{struct
MHD_WorkModeWithParam} that includes the various configuration
settings for the work mode. While @code{MHD_D_OPTION_WORK_MODE} can in
principle be used to set the work mode, the MHD API offers various
convenience APIs for each of the work modes.

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WORK_MODE (struct MHD_WorkModeWithParam wmp)

Set MHD work (threading and polling) mode.
Consider use of @code{MHD_D_OPTION_WM_EXTERNAL_PERIODIC()},
@code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL()},
@code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE()},
@code{MHD_D_OPTION_WM_EXTERNAL_SINGLE_FD_WATCH()},
@code{MHD_D_OPTION_WM_WORKER_THREADS()} or
@code{MHD_D_OPTION_WM_THREAD_PER_CONNECTION()}
instead of direct use of this option.

@table @var

@item wmp
the work mode setting, to be created by either @code{MHD_WM_OPTION_EXTERNAL_PERIODIC()}, @code{MHD_WM_OPTION_EXTERNAL_EVENT_LOOP_CB_LEVEL()}, @code{MHD_WM_OPTION_EXTERNAL_EVENT_LOOP_CB_EDGE()}, @code{MHD_WM_OPTION_EXTERNAL_SINGLE_FD_WATCH()}, @code{MHD_WM_OPTION_WORKER_THREADS()}, or @code{MHD_WM_OPTION_THREAD_PER_CONNECTION()}.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

As explained, the usual way to set the work mode is to use one of
the following convenience APIs instead of using
@code{MHD_D_OPTION_WORK_MODE()} directly. We will thus describe
each work mode together with the option used to set it.

@node libmicrohttpd-daemon-workmode-external-periodic
@subsection External periodic

In this mode, the application must periodical call
@code{MHD_daemon_process_blocking()}, where MHD internally checks all
sockets automatically.  This is the default mode. It is the simplest
to use, but also inefficient as it requires busy-waiting by the
application.

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_EXTERNAL_PERIODIC()

Create parameter for @code{MHD_daemon_set_options()} for work mode with
no internal threads and no support for event loops.
Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.

@end deftypefun

When using this mode, it is critical that the application
frequently calls @code{MHD_daemon_process_blocking()}.

@anchor{MHD_daemon_process_blocking}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_daemon_process_blocking (struct MHD_Daemon *daemon, uint_fast64_t microsec)

Run the websever operation with possible blocking.

Supported only in @code{MHD_WM_EXTERNAL_PERIODIC} mode.

This function does the following: waits for any network event not more
than specified number of microseconds, processes all incoming and
outgoing data, processes new connections, processes any timed-out
connection, and does other things required to run the webserver.  Once all
connections are processed, the function returns.

This function is useful for quick and simple (lazy) webserver
implementation if an application needs to run only a single thread and
does not even have an event loop.

@table @var

@item daemon
the daemon to run

@item microsec
the maximum time in microseconds to wait for network and other
events. Theere is no guarantee that function blocks for the specified
amount of time. The real processing time can be shorter (if some data
or connection timeout comes earlier) or longer (if data processing
requires more time, especially in user callbacks).  If set to '0' then
function does not block and processes only already available data (if
any).  If set to @code{MHD_WAIT_INDEFINITELY}, this function waits for
events indefinitely (blocks until next network activity or connection
timeout).

@end table

Returns @code{MHD_SC_OK} on success, otherwise an error code.
@end deftypefun




@node libmicrohttpd-daemon-workmode-external-event-loop
@subsection External event loop

In this mode, the application must provide a callback to
MHD where MHD will inform the application about network events
it cares about.  The application must then call
@ref{MHD_daemon_event_update,,@code{MHD_daemon_event_update()}}
and @ref{MHD_daemon_process_nonblocking,,@code{MHD_daemon_process_nonblocking()}}
to drive MHD's network interaction.

@anchor{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL(MHD_SocketRegistrationUpdateCallback cb_val, void *cb_cls_val)

Create parameter for @code{MHD_daemon_set_options()} for work mode with
an external event loop with level triggers.

@table @var

@item cb_val
callback implemented by the application to learn about MHD's needs;

@item cb_cls_val
closure argument for @var{cb_val};

@end table

Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.

@end deftypefun

@anchor{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE(MHD_SocketRegistrationUpdateCallback cb_val, void *cb_cls_val)

Create parameter for @code{MHD_daemon_set_options()} for work mode with
an external event loop with edge triggers.

@table @var

@item cb_val
callback implemented by the application to learn about MHD's needs;

@item cb_cls_val
closure argument for @var{cb_val};

@end table

Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.

@end deftypefun

How to use an external event loop is described in detail
in another chapter of this manual.
@xref{libmicrohttpd2-external,,Using external event loops}.


@node libmicrohttpd-daemon-workmode-external-single-fd
@subsection External single FD watch


@anchor{MHD_D_OPTION_WM_EXTERNAL_SINGLE_FD_WATCH}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_EXTERNAL_SINGLE_FD_WATCH()

Create parameter for @code{MHD_daemon_set_options()} to create a work
mode with no internal threads and an aggregate watch file descriptor (FD).
When using this setting, applications @emph{must} use
@ref{MHD_DAEMON_INFO_FIXED_AGGREGATE_FD,,@code{MHD_DAEMON_INFO_FIXED_AGGREGATE_FD}}
to get the single FD that gets triggered by any MHD event.  This FD can
be watched as an aggregate indicator for all MHD events.  This mode is
available only on selected platforms (currently GNU/Linux
only). @xref{MHD_LIB_INFO_FIXED_HAS_AGGREGATE_FD,,@code{MHD_LIB_INFO_FIXED_HAS_AGGREGATE_FD}}.  When the FD is
triggered, @code{MHD_daemon_process_nonblocking()} should be called as
soon as possible by the application.

Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.
@end deftypefun

@anchor{MHD_daemon_process_nonblocking}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_daemon_process_nonblocking (struct MHD_Daemon *daemon)

Run the websever operation without blocking.

Supported only in @code{MHD_WM_EXTERNAL_SINGLE_FD_WATCH} mode.

This function does the following: checks for any network events that
are ready, processes all incoming and outgoing data, processes new
connections, processes any timed-out connection, and does other things
required to run the webserver.  Once all network events have been
processed, the function immediately returns.

@table @var

@item daemon
the daemon to run;

@end table

Returns @code{MHD_SC_OK} on success, otherwise an error code.
@end deftypefun



@node libmicrohttpd-daemon-workmode-worker-threads
@subsection Worker threads


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_WORKER_THREADS(unsigned int num_workers)

Create parameter for @code{MHD_daemon_set_options()} for work mode
with @var{num_workers} internal worker threads that each listen for
incoming connections and handle multiple clients.  If number of
threads is one, then daemon starts with single worker thread that
handles all connections.  If number of threads is larger than one,
then handling of connections is distributed among the workers.


@table @var

@item num_workers
the number of worker threads, zero is treated as one;

@end table

Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.

@end deftypefun

@node libmicrohttpd-daemon-workmode-thread-per-connection
@subsection Thread per connection


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_WM_THREAD_PER_CONNECTION()

Create parameter for @code{MHD_daemon_set_options()} for work mode
one internal thread for listening, and additional internal threads per
connection.  Use this if handling requests is CPU-intensive or blocking,
our application is thread-safe and you have plenty of memory (per
connection) as each thread requires its own stack.
Applications using this mode should strongly consider using
@ref{MHD_D_OPTION_GLOBAL_CONNECTION_LIMIT,,@code{MHD_D_OPTION_GLOBAL_CONNECTION_LIMIT}}.

Returns an object of type @code{struct MHD_DaemonOptionAndValue}
with the requested settings.

@end deftypefun


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_POLL_SYSCALL (enum MHD_SockPollSyscall els)

Select a sockets watch system call to be used for internal polling.

@table @var

@item els
mode to use; @code{MHD_SPS_AUTO} is the default if this
option is not set. @xref{MHD_SockPollSyscall}

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@anchor{MHD_SockPollSyscall}
@deftp {Enumeration} MHD_SockPollSyscall

Possible ways for sockets polling for internal syscalls.

@table @code

@item MHD_SPS_AUTO
Automatic selection of the best-available method. This is also the
default.

@item MHD_SPS_SELECT
Force use of @code{select()}.

@item MHD_SPS_POLL
Force use of @code{poll()}.

@item MHD_SPS_EPOLL
Force use of @code{epoll()}.
@end table
@end deftp


@node libmicrohttpd-daemon-logging
@section Configuring logging and notifications

Options in this section allow an application to receive notifications
from MHD whenever the library's logic reaches a particular stage. This
includes general logging, but also daemons, connecitons or requests
reaching a particular stage of their respective lifecycle.

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_LOG_CALLBACK (MHD_LoggingCallback log_cb, void *log_cb_cls)

Set a callback to use for logging.

@table @var

@item log_cb
the callback to use for logging,
@code{NULL} to disable logging.
Logging to @code{stderr} is enabled by default;

@item log_cb_cls
the closure for the logging callback;

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_EARLY_URI_LOGGER (MHD_EarlyUriLogCallback cb, void *cb_cls)

Set a callback to be called first for every request when the request line is received (before any parsing of the header).
This callback is the only way to get raw (unmodified) request URI as URI is parsed and modified by MHD in-place.
Mandatory URI modification may apply before this call, like binary zero replacement, as required by RFCs.

@table @var

@item cb
the early URI callback;

@item cb_cls
the closure for the callback;

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@anchor{MHD_EarlyUriLogCallback}
@deftypefn {Function Pointer} void (*MHD_EarlyUriLogCallback) (void *cls, const struct MHD_EarlyUriCbData *req_data, void **request_app_context_ptr)

Function called by MHD to allow the application to log the full URI of any new request.
This is the only moment when unmodified URI is provided.
After this callback MHD parses the URI and modifies it by extracting
URL arguments in-place.

If this callback is set then it is the first application function called
for every new request.

If a @ref{MHD_RequestEndedCallback} is also set then it is guaranteed
that @ref{MHD_RequestEndedCallback} is called for the same request
eventually. Application may allocate request specific data in this
callback and de-allocate the data in @ref{MHD_RequestEndedCallback}.

@table @var
@item cls
custom value provided by the application at callback registration time;

@item req_data
data about the client's request;

@item request_app_contex_ptr
pointer to a location that can be set to
an application context for the request;
initially the location is guaranteed to
point to @code{NULL};

@end table
@end deftypefn

The @var{req_data} provides the following information
about the request:


@anchor{MHD_EarlyUriCbData}
@deftp {C Struct} struct MHD_EarlyUriCbData

Data for @ref{MHD_EarlyUriLogCallback}s.

@table @var

@item @code{struct MHD_Request *} request
The request handle.
Headers will not yet be available.

@item @code{struct MHD_String} full_uri
The full URI ("request target") from the HTTP request, including URI parameters (the part after '?').

@item @code{enum MHD_HTTP_Method} method
The HTTP method of the request;

@end table
@end deftp



@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DAEMON_READY_CALLBACK (MHD_DaemonReadyCallback cb, void *cb_cls)

Set a callback to be called for pre-start finalisation.

The specified callback will be called one time, after network initialisation, TLS pre-initialisation, but before the start of the internal threads (if allowed)

@table @var

@item cb
the pre-start callback;

@item cb_cls
the closure for the callback;

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@anchor{MHD_DaemonReadyCallback}
@deftypefn {Function Pointer} void (*MHD_DaemonReadyCallback) (void *cls)

The specified callback will be called one time,
after network initialisation, TLS pre-initialisation, but before
the start of the internal threads (if we have any).

This callback may use introspection call to retrieve and adjust some
of the daemon's aspects. For example, the TLS backend handle could be
obtained via introspection and used to configure some additional
TLS-backend specific settings.
 
@table @var
@item cls
custom value provided by the application at callback registration time;

@end table

@end deftypefn

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_NOTIFY_CONNECTION (MHD_NotifyConnectionCallback ncc, void *ncc_cls)

Set a function that should be called whenever a connection is started or closed.

@table @var

@item ncc
the callback for notifications;

@item cls
the closure for the callback;

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@anchor{MHD_NotifyConnectionCallback}
@deftypefn {Function Pointer} void (*MHD_NotifyConnectionCallback) (void *cls, struct MHD_ConnectionNotificationData *data)
@c FIXME: why is 'data' not const?
@c FIXME: non-uniform: for requests, we have the
@c application context in the argument, here we have it
@c inside of data; not nice!

 
@table @var
@item cls
custom value provided by the application at callback registration time;

@item data
details about the connection;

@end table

@end deftypefn

@c FIXME: elaborate on MHD_ConnectionNotificationData
@c FIXME: but only after we revised it!

@c FIXME: elaborate on MHD_ConnectionNotificationCode

@anchor{MHD_ConnectionNotificationDetails}
@deftp {C Union} struct MHD_ConnectionNotificationDetails

@var{code}-specific details on how a request ended.
So far not used, only a placeholder.

@table @var

@item @code{void *} reserved
Placeholder. Do not used.

@end table
@end deftp


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_NOTIFY_STREAM (MHD_NotifyStreamCallback nsc, void *nsc_cls)

Register a function that should be called whenever a stream is started or closed.
For HTTP/1.1 this callback is called one time for every connection.

@table @var

@item nsc
the callback for notifications;

@item nsc_cls
the closure for the callback;

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@anchor{MHD_NotifyStreamCallback}
@deftypefn {Function Pointer} void (*MHD_NotifyStreamCallback) (void *cls, const struct MHD_StreamNotificationData *data)
@c FIXME: non-uniform: for requests and connections, we have an
@c application context in the argument, here have none!

 
@table @var
@item cls
custom value provided by the application at callback registration time;

@item data
details about the stream;

@end table

@end deftypefn


@anchor{MHD_StreamNotificationData}
@deftp {C Struct} struct MHD_StreamNotificationData

Data for functions of type @ref{MHD_NotifyStreamCallback}.

@table @var

@item @code{struct MHD_Stream *} stream
The stream handle;

@item @code{enum MHD_StreamNotificationCode} code
The code of the event;

@item @code{union MHD_StreamNotificationDetail} details
Detailed information about the event the notification is about;

@end table
@end deftp


@anchor{MHD_StreamNotificationCode}
@deftp {Enumeration} MHD_StreamNotificationCode

Type of stream notifications.

@table @code

@c FIXME: rename to OPENED? more symmetric!
@item MHD_STREAM_NOTIFY_STARTED
A new stream has been started.

@item MHD_STREAM_NOTIFY_CLOSED
A stream is being closed.

@end table
@end deftp

@c FIXME: ealborate on MHD_StreamNotificationDetail
@c FIXME: strongly consider inlining the started structure...


@node libmicrohttpd-daemon-listen
@section Configuring the listen socket

The options in this section determine how MHD will listen
for incoming connections. MHD can either be told to
open its own listen socket (in which case the application
must specify the address family and port, or the complete
socket address), or MHD can be given a listen socket that
is already pre-configured (for example, if systemd socket
activation is used). It is even possible to run MHD without
a listen socket, and only give it the (already accepted)
connections from clients. Note that these are mutually
exclusive choices, as a single daemon only ever uses a
single listen socket. 

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_BIND_PORT (enum MHD_AddressFamily af, uint_least16_t port)

Bind to the given TCP port and address family.

Does not work with @code{MHD_D_OPTION_BIND_SA()} or
@code{MHD_D_OPTION_LISTEN_SOCKET()}.

If no listen socket options (@code{MHD_D_OPTION_BIND_PORT()}, @code{MHD_D_OPTION_BIND_SA()}, or @code{MHD_D_OPTION_LISTEN_SOCKET()}) are used, MHD does not listen for incoming connections.

@table @var

@item af
the address family to use,
Use the special @code{MHD_AF_NONE} to disable the listen socket (the same effect as if this option is not used).

@item port
port to use, 0 to let system assign any free port,
ignored if @var{af} is @code{MHD_AF_NONE}

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftp {Enumeration} MHD_AddressFamily

Address family to be used by MHD.

@table @code

@item MHD_AF_NONE
Option not given. Do not listen at all
(unless listen socket or address specified by
other means).

@item MHD_AF_AUTO
Pick "best" available method automatically.

@item MHD_AF_INET4
Use IPv4 only.

@item MHD_AF_INET6
Use IPv6 only.

@item MHD_AF_DUAL
Use dual stack (IPv4 and IPv6 on the same socket).

@item MHD_AF_DUAL_v4_OPTIONAL
Use dual stack (IPv4 and IPv6 on the same socket),
fallback to pure IPv6 if dual stack is not possible.

@item MHD_AF_DUAL_v6_OPTIONAL
Use dual stack (IPv4 and IPv6 on the same socket),
fallback to pure IPv4 if dual stack is not possible.

@end table
@end deftp


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_BIND_SA (size_t sa_len, const struct sockaddr *sa, enum MHD_Bool dual)

Bind to the given socket address.

Does not work in combination with @code{MHD_D_OPTION_BIND_PORT()} or @code{MHD_D_OPTION_LISTEN_SOCKET()}.

If no listen socket options (@code{MHD_D_OPTION_BIND_PORT()}, @code{MHD_D_OPTION_BIND_SA()}, or @code{MHD_D_OPTION_LISTEN_SOCKET()}) are used, MHD does not listen for incoming connections.

@table @var

@item sa_len
the size of the socket address pointed by @var{sa}.

@item sa
the address to bind to; can be IPv4 (AF_INET), IPv6 (AF_INET6) or even a UNIX domain socket (AF_UNIX)

@item dual
When a previous version of the protocol exists (like IPv4 when @var{v_sa} is IPv6) bind to both protocols (IPv6 and IPv4).

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_LISTEN_SOCKET (MHD_Socket listen_fd)

Accept connections from the given socket.  Socket
must be a TCP or UNIX domain (SOCK_STREAM) socket.

Does not work in combination with @code{MHD_D_OPTION_BIND_PORT()} or
@code{MHD_D_OPTION_BIND_SA()}.

If no listen socket options (@code{MHD_D_OPTION_BIND_PORT()}, @code{MHD_D_OPTION_BIND_SA()}, or @code{MHD_D_OPTION_LISTEN_SOCKET()}) are used, MHD does not listen for incoming connections.

@table @var

@item listen_fd
the listen socket to use, ignored if set to @code{MHD_INVALID_SOCKET}.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_LISTEN_ADDR_REUSE (enum MHD_DaemonOptionBindType reuse_type)

Select mode of reusing address:port listen address.

Works only when @code{MHD_D_OPTION_BIND_PORT()} or
@code{MHD_D_OPTION_BIND_SA()} are also used.

@table @var

@item reuse_type
address reuse options to use

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@anchor{MHD_DaemonOptionBindType}
@deftp {Enumeration} MHD_DaemonOptionBindType

Parameter for listen socket binding type. Determines what should
happen if multiple processes try to bind to the same port.

@table @code

@item MHD_D_OPTION_BIND_TYPE_SHARED
The listen socket binds to the networks address with sharing the address.
Several sockets can bind to the same address.

@item MHD_D_OPTION_BIND_TYPE_NOT_SHARED
The listen socket binds to the networks address without sharing the
address, except allowing binding to port/address which is only in
TIME_WAIT state (the state after closing connection).  On some
platforms it may also allow to bind to specific address if other
socket already bound to the same port of wildcard address (or bind to
wildcard address when other socket already bond to specific address
with the same port).  Typically achieved by enabling the
@code{SO_REUSEADDR} socket option.  This is the default.

@item MHD_D_OPTION_BIND_TYPE_NOT_SHARED_STRICTER
The listen socket binds to the networks address without sharing the
address.  The daemon way fail to start when any sockets still in
TIME_WAIT state on the same port, which effectively prevents quick
restart of the daemon on the same port.  On W32 systems this option
works like @code{MHD_D_OPTION_BIND_TYPE_NOT_SHARED} due to OS
limitations.

@item MHD_D_OPTION_BIND_TYPE_EXCLUSIVE
The list socket binds to the networks address in explicit exclusive
mode.  Works as @code{MHD_D_OPTION_BIND_TYPE_NOT_SHARED_STRICTER} on
platforms without support for the explicit exclusive socket use.

@end table
@end deftp


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TCP_FASTOPEN (enum MHD_TCPFastOpenType option, unsigned int queue_length)

Configure TCP_FASTOPEN option, including setting a
custom @var{queue_length}.

Note that having a larger queue size can cause resource exhaustion
attack as the TCP stack has to now allocate resources for the SYN
packet along with its DATA.

Works only in combination with @code{MHD_D_OPTION_BIND_PORT()} or @code{MHD_D_OPTION_BIND_SA()}.

@table @var

@item option
the type use of of TCP FastOpen; @xref{MHD_TCPFastOpenType}

@item queue_length
the length of the queue, zero to use system or MHD default,
silently ignored on platforms without support for custom queue size.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@anchor{MHD_TCPFastOpenType}
@deftp {Enumeration} MHD_TCPFastOpenType

Possible levels of support or enforcement for TCP_FASTOPEN.

@table @code
@item MHD_FOM_DISABLE
Disable use of TCP_FASTOPEN.

@item MHD_FOM_AUTO
Enable TCP_FASTOPEN where supported.
On GNU/Linux it works with a kernel >= 3.6.
This is the default.

@item MHD_FOM_REQUIRE
Require support for TCP_FASTOPEN.
This causes @code{MHD_daemon_start()} to fail
if TCP_FASTOPEN cannot be enabled!

@end table
@end deftp


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_LISTEN_BACKLOG (unsigned int backlog_size)

Use the given backlog for the @code{listen()} call.

Works only when @code{MHD_D_OPTION_BIND_PORT()}
or @code{MHD_D_OPTION_BIND_SA()} are used.

@table @var

@item backlog_size
Backlog to use. A value of zero implies to use MHD/system defaults.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun



@node libmicrohttpd-daemon-tls
@section Configuring TLS

The following options can be used to configure
MHD to use TLS, turning an HTTP server into an
HTTPS server. 

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TLS (enum MHD_TlsBackend backend)

Enable TLS (HTTPS) and select TLS backend

@table @var

@item backend
the TLS backend to use,
@code{MHD_TLS_BACKEND_NONE} for non-TLS (plain TCP) connections

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@c FIXME: const is commented out in C API!?
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TLS_CERT_KEY (const char *mem_cert, const char *mem_key, const char *mem_pass)

Provide TLS key and certificate data in-memory.
Works only if TLS mode is enabled.

@table @var

@item mem_cert
The X.509 certificates chain in PEM format loaded into memory (not a filename).
The first certificate must be the server certificate, following by the chain of signing
certificates up to (but not including) CA root certificate.

@item mem_key
the private key in PEM format loaded into memory (not a filename)

@item mem_pass
the option passphrase phrase to decrypt the private key,
could be NULL if private key does not need a password

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TLS_CLIENT_CA (const char *mem_client_ca)

Provide the certificate of the certificate authority (CA) to be used by the MHD daemon for client authentication.
Works only if TLS mode is enabled.

@table @var

@item mem_client_ca
the CA certificate in memory (not a filename)

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TLS_PSK_CALLBACK (MHD_PskServerCredentialsCallback psk_cb, void *psk_cb_cls)

Configure PSK to use for the TLS key exchange.

@table @var

@item psk_cb
the function to call to obtain pre-shared key

@item psk_cb_cls
the closure for @var{psk_cb}

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_NO_ALPN ()

Control ALPN for TLS connection.
If this option is set but TLS is not enabled overall,
it is silently ignored (does not cause a failure).
By default ALPN is automatically used for TLS connections.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@node libmicrohttpd-daemon-authentication
@section Configuring HTTP authentication

@c FIXME: const commented out in C API!?
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_RANDOM_ENTROPY (size_t buf_size, const void *buf)

Set strong random data to be used by MHD.
Currently the data is only used for digest authentication.
@c FIXME: is it NOT enabled if the option is not set? Unclear english!
Daemon support for digest authentication is enabled automatically when this option is used.
The recommended size is 32 bytes. 
Sizes larger then 32 bytes are unlikely to increase
security (assuming a strong random number generator was
used to create @var{buf}).

@table @var

@item buf_size
the size of the buffer

@item buf
the buffer with strong random data, the content will be copied by MHD

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_AUTH_DIGEST_MAP_SIZE (size_t size)

Specify the size of the internal hash map array that tracks generated digest nonces usage.
When the size of the map is too small then need to handle concurrent digest authentication requests, many false-positive stale nonce responses might be produced.
By default, the digest nonce hash map size is 1000 entries.

@table @var

@item size
the size of the map array

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_AUTH_DIGEST_NONCE_TIMEOUT (unsigned int timeout)

Configure nonce validity period for nonces used with HTTP digest
authentication.

@table @var

@item timeout
Validity period (in seconds) to apply for the nonces used in
digest authentication.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_AUTH_DIGEST_DEF_MAX_NC (uint_fast32_t max_nc)

Default maximum nonce counter (nc) value used ford digest authentication.
@xref{MHD_digest_auth_check_digest}.

@table @var

@item max_nc
Maximum nonce counter allowed. Zero is ignored.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@node libmicrohttpd-daemon-limits
@section Configuring resource limits

This section is about daemon configuration options that
enable applications to configure resource limits. Setting
resource limits can help ensure that MHD does not use
too many resources, but also ensure that MHD uses enough
resources for optimum performance.

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DEFAULT_TIMEOUT (unsigned int timeout)

Specify inactivity timeout for connection.
When no activity for specified time on connection, it is closed automatically.
Use zero for no timeout, which is also the (insecure!) default.  It is highly recommended to set a non-zero timeout to ensure connections are not kept open forever.

@table @var

@item timeout
the in seconds, zero for no timeout

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@anchor{MHD_D_OPTION_GLOBAL_CONNECTION_LIMIT}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_GLOBAL_CONNECTION_LIMIT (unsigned int glob_limit)

Sets the maximum number of (concurrent) network connections served by the daemon.

@table @var

@item glob_limit
Limit to apply. The real maximum number of network connections could
still be smaller than requested due to the system limitations,
like @code{FD_SETSIZE} when polling via @code{select()} is used.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_PER_IP_LIMIT (unsigned int limit)

Sets a limit on the number of (concurrent) network connections made to the server from the same IP address.
Can be used to prevent one IP from taking over all of the allowed connections. If the same IP tries to establish more than the specified number of connections, they will be immediately rejected.

@table @var

@item limit
Connection limit to use. 0 for no limit.
@c FIXME: 0 = no limit is correct?

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_ACCEPT_POLICY (MHD_AcceptPolicyCallback apc, void *apc_cls)

Set a policy callback that accepts/rejects connections based on the client's IP address.  The callbeck function will be called before servicing any new incoming connection.

@table @var

@item apc
the accept policy callback

@item apc_cls
the closure for the callback

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun


@anchor{MHD_AcceptPolicyCallback}
@deftypefn {Function Pointer} {enum MHD_Bool} (*MHD_AcceptPolicyCallback)(void *cls, size_t addr_len, const struct sockaddr *addr)

Allow or deny a client to connect.

@table @var

@item cls
closure value provided by the application when registering
the callback

@item addr_len
length of @var{addr}

@item addr
address information from the client

@end table
The function must return @code{MHD_YES} if
the connection should be allowed, and
@code{MHD_NO} if it should be denied.\
@end deftypefn


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_PROTOCOL_STRICT_LEVEL (enum MHD_ProtocolStrictLevel sl, enum MHD_UseStictLevel how)

Set how strictly MHD will enforce the HTTP protocol.

@table @var

@item sl
the level of strictness

@item how
the way how to use the requested level

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@c FIXME: define MHD_ProtocolStrictLevel
@c FIXME: define MHD_UseStrictLevel

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_CONN_MEMORY_LIMIT (size_t value)

The default limit is only 32kb per connection.  Applications should be
careful to make sure that the given limit is large enough to fit all
request headers (together with internal parsing information).  Half of
the memory will typically be used for I/O buffers.  On embedded
platforms, values above 128kb are unlikely to result in significant
performance benefits as TCP buffers are unlikely to support window
sizes above 64k on embedded systems.

@table @var

@item value
Maximum memory size per connection in bytes.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@anchor{shared memory pool}
@anchor{MHD_D_OPTION_LARGE_POOL_SIZE}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_LARGE_POOL_SIZE (size_t value)

The size of the shared memory pool for accumulated upload processing.
The same large pool is shared for all connections server by MHD and used when application requests avoiding of incremental upload processing to accumulate complete content upload before giving it to the application.
Default is 8Mb.
Can be set to zero to disable use of the shared pool.

@table @var

@item value
Size of the large memory pool in bytes.

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_STACK_SIZE ()

Desired size of the stack for the threads started by MHD.
Use 0 to use the operating system's default stack size, which is also the default value used by MHD.
Works only with
@code{MHD_D_OPTION_WM_WORKER_THREADS()} or
@code{MHD_D_OPTION_WM_THREAD_PER_CONNECTION()}.

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_FD_NUMBER_LIMIT (MHD_Socket max_fd)

The the maximum FD value.
The limit is applied to all sockets used by MHD.
If the listen socket FD is equal to or higher then the specified value, the daemon fail to start.
If a new connection's FD is equal to or higher then the specified value, the connection is rejected.
If an application uses @code{select()} for polling sockets,
the system's @code{FD_SETSIZE} is a good value for this option.
Silently ignored on W32 (WinSock sockets are not integers).

@table @var

@item max_fd
Maximum file descriptor to allow (exclusive).
@c FIXME: it is exclusive, right?

@end table

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun





@node libmicrohttpd-daemon-performance
@section Performance tuning options


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_TURBO ()

Enable `turbo`.
Disables certain calls to @code{shutdown()}, enables aggressive non-blocking optimistic reads and other potentially unsafe optimisations.
Most effects only happen with internal threads with @code{epoll()}.
The 'turbo' mode is disabled by default.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DISABLE_THREAD_SAFETY ()

Disable some internal thread safety.
Indicates that MHD daemon will be used by application in single-threaded mode only.  When this flag is set then application must call any MHD function only within a single thread.
This flag turns off some internal thread-safety and allows MHD making some of the internal optimisations suitable only for single-threaded environments.
Not compatible with any internal threads modes.
If MHD is compiled with custom configuration for embedded projects without threads support, this option is mandatory.
Thread safety is enabled by default.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DISALLOW_UPGRADE ()

You need to set this option if you want to disable use of HTTP Upgrade.
Upgrade may require usage of additional internal resources, which MHD can avoid allocating if they will not be used.
You should only use this option if you do not use upgrade functionality and need a generally minor boost in performance and resources savings.
HTTP upgrade is allowed by default.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DISALLOW_SUSPEND_RESUME ()

Disable @ref{MHD_action_suspend,,@code{MHD_action_suspend()}} functionality.

You should only use this function if you do not use suspend functionality and need a generally minor boost in performance.
Suspending requests is allowed by default.

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun

@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DISABLE_COOKIES ()

Disable cookies parsing.

Disable automatic cookies processing if cookies are not used.
Cookies are automatically parsed by default.

Returns the option to pass to @code{MHD_daemon_set_options()}
@end deftypefun




@node libmicrohttpd-daemon-other
@section Other daemon options


@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_SIGPIPE_SUPPRESSED ()

Inform that SIGPIPE is suppressed or handled by application.
If suppressed/handled, MHD uses network functions that could generate SIGPIPE, like @code{sendfile()}.
Silently ignored when MHD creates internal threads as for them SIGPIPE is suppressed automatically.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun



@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_DISABLE_URI_QUERY_PLUS_AS_SPACE ()

Disable converting plus ('+') character to space in URL arguments (URL part after '?').
Plus conversion is not required by HTTP RFCs, however it required by HTML specifications, see https://url.spec.whatwg.org/#application/x-www-form-urlencoded for details.
By default plus is converted to space in the query part of a URL.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@cindex real-time clock
@anchor{MHD_D_OPTION_SUPPRESS_DATE_HEADER}
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_SUPPRESS_DATE_HEADER ()

Suppresse use of 'Date:' header.
According to RFC should be suppressed only if the system has no real-time clock (RTC).
The 'Date:' is not suppressed (the header is enabled) by default.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun

@cindex Shoutcast
@deftypefun {struct MHD_DaemonOptionAndValue} MHD_D_OPTION_ENABLE_SHOUTCAST ()

Use SHOUTcast for responses.
This will cause @emph{all} responses to begin with the SHOUTcast 'ICY' line instead of 'HTTP'.

Returns the option to pass to @code{MHD_daemon_set_options()}.
@end deftypefun
