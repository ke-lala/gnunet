A modern HTTP server should handle multiple concurrent connections
and an application embedding an HTTP server into its process may
possibly need do other work not directly related to handling HTTP
requests.

There are several approches for how an HTTP server can handle
concurrent requests:

@itemize
@item start a new process per request, or use a pool of processes
@item run a (new) thread per request, or use a pool of threads
@item use an event loop to learn when data can be received from or
  sent to the client (based on system calls like @code{select()}
  or @code{epoll()})
@end itemize

Finally, these different approaches are not mutually exclusive,
it is possible to use multiple processes with multiple threads
each using an event loop.  The good news is that MHD supports
all of these.

The @emph{initially} simplest approach is usually to simply tell MHD
to start one or more new threads to handle connections, allowing the
main application to continue doing its work in its own separate
threats.  The disadvantage of this approach is that concurrent
accesses between the main application's threads and MHD's threads
require the use of synchronization primitives (such as mutexes) to
avoid data races. Furthermore, threads are not available on all
platforms and require additional stack space for each thread.

An alternative to using threads is to integrate MHD's request
processing into the main (event) loop of the application. This is
particularly suitable for applications that already have an event loop
and that avoid using threads for performance, portability,
maintainability or security reasons.  We call this approach the
use of MHD with an @emph{external} event loop, in contrast to
MHD creating its own threads with an @emph{internal} event loop.

This chapter describes MHD's API for integrating a daemon with
an external event loop.  Any external event loop needs to ask
MHD for three pieces of information:

@itemize
@item The set of sockets to wait on for status changes;
@item for each socket the types of status changes to
  wait for (ready-to-read, ready-to-write, failed);
@item a timeout for when to wake up MHD again even if nothing
  else happened.
@end itemize

Depending on the operating system and the event loop style, the
representation of these bits of information may differ; in particular,
with @code{epoll()} MHD might even give the application a single
handle that represents the entire set.  Finally, we need to
distinguish between event loops that support edge triggers (only
notifying when the status changed to ready) and level triggers
(notifying about the socket being ready, even if it simply remained
ready).


@anchor{external-registration}
@node libmicrohttpd-external-registration
@section The @code{MHD_SocketRegistrationUpdateCallback}

External event loops are enabled using either
@ref{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL} or
@ref{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE} depending
on whether edge or level triggers should be used. In both
cases, the application must provide a function of type
@code{MHD_SocketRegistrationUpdateCallback} when enabling
the option.

@anchor{MHD_SocketRegistrationUpdateCallback}
@deftypefn {Function Pointer} {MHD_APP_SOCKET_CNTX_TYPE *} (*MHD_SocketRegistrationUpdateCallback) (void *cls, MHD_Socket fd, enum MHD_FdState watch_for, MHD_APP_SOCKET_CNTX_TYPE *app_cntx_old, struct MHD_EventUpdateContext *ecb_cntx)

Type of a function for registration or de-registration of the sockets to
watch in the external event loop.

Implementations of this callback must not call
@ref{MHD_daemon_destroy,,@code{MHD_daemon_destroy()}},
@ref{MHD_daemon_quiesce,,@code{MHD_daemon_quiesce()}} or
@ref{MHD_daemon_add_connection,,@code{MHD_daemon_add_connection()}}.

@table @var
@item cls
custom value provided by the application at callback registration time;

@item fd
the socket to watch;

@item watch_for
the states of the @var{fd} to watch, if set to
@code{MHD_FD_STATE_NONE} the socket must be de-registred
and the resources from the application state in the @var{app_cntx_old} should
be released;

@item app_cntx_old
the old application-defined context for the socket,
@code{NULL} if the @var{fd} socket was not registered before;

@item ecb_cntx
the context handle to be used
with @code{MHD_daemon_event_update()};
@end table

Implementations must return the new socket context to be used with
this @var{fd}.  Implementations must return @code{NULL} on de-registration
and on error (in which case the connection to the client will be
closed).

@end deftypefn

The registration update callback is provided with @var{fd} which
is the socket for which the external event loop should become
active. The use of @ref{MHD_Socket,,@code{MHD_Socket}} is for
portability.  The @var{watch_for} determines the types of
triggers the external event loop should use as triggers.


@deftp {Enumeration} MHD_FdState

The network status of the socket or the type of trigger to wait for.
When set by MHD (for @code{MHD_SocketRegistrationUpdateCallback} and
similar APIs) it indicates a request by MHD to watch for specific
socket state: readiness for receiving the data, readiness for
sending the data and/or an exception state of the socket.
When set by application (and provided to
@code{MHD_daemon_event_update()} and similar APIs)
it must indicate the actual status of the socket.

In both cases, the actual value may be a bitwise OR
combination of @code{MHD_FD_STATE_RECV},
@code{MHD_FD_STATE_SEND} and @code{MHD_FD_STATE_EXCEPT}.

@table @code
@item MHD_FD_STATE_NONE
The socket is not ready for receiving or sending and
does not have any exceptional state.
The state is used for de-registration of sockets in
a @code{MHD_SocketRegistrationUpdateCallback}.

@item MHD_FD_STATE_RECV
Indicates that socket should be watched for incoming data
(when set for @code{MHD_SocketRegistrationUpdateCallback})
or that a socket has incoming data ready to read (when used for
@code{MHD_daemon_event_update()});

@item MHD_FD_STATE_SEND
Indicates that socket should be watched for availability for sending
(when set for @code{MHD_SocketRegistrationUpdateCallback})
or that a socket is ready for transmission of data (when used
for @code{MHD_daemon_event_update()});

@item MHD_FD_STATE_EXCEPT
Indicates that socket should be watched for disconnect, out-of-band
data available or high priority data available (when set by
@code{MHD_SocketRegistrationUpdateCallback})
or that a socket has been disconnected, has out-of-band data available or
has high priority data available (when used for
@code{MHD_daemon_event_update()}). This status must not include
signalling the ``remote peer shut down writing'' (FIN) status.

@item MHD_FD_STATE_RECV_SEND
Combination of @code{MHD_FD_STATE_RECV} and @code{MHD_FD_STATE_SEND};

@item MHD_FD_STATE_RECV_EXCEPT
Combination of @code{MHD_FD_STATE_RECV} and @code{MHD_FD_STATE_EXCEPT};

@item MHD_FD_STATE_SEND_EXCEPT
Combination of @code{MHD_FD_STATE_SEND} and @code{MHD_FD_STATE_EXCEPT};

@item MHD_FD_STATE_RECV_SEND_EXCEPT
Combination of @code{MHD_FD_STATE_RECV}, @code{MHD_FD_STATE_SEND}
and @code{MHD_FD_STATE_EXCEPT}.

@end table
@end deftp

@anchor{MHD_APP_SOCKET_CNTX_TYPE}
The @var{app_cntx_old} argument is a bit unusual:
@code{MHD_APP_SOCKET_CNTX_TYPE} is actually a macro that @emph{should}
be defined by the application @emph{before} including
``microhttpd2.h''. If the application does not define this type, it
will be defined as @code{void}.  The use of this argument is to give
the application the opportunity to associate its own data structure
with the external event loop trigger responsibility given to it by
MHD.  When called to register a @var{fd} for the first time, MHD will
always pass @code{NULL}. Subsequently, MHD will pass whatever value
was returned by the @code{MHD_SocketRegistrationUpdateCallback}. By
giving MHD a pointer to whatever data structure the event loop of the
application wants to use, applications can avoid iterating over their
internal data structures to find an entry for @var{fd}.  The use of
an application-defined macro for this type improves type-safety.
When MHD calls the callback to de-register a socket, the application
must free any of its resources associated with @var{app_cntx_old}.
The application must at least store the @var{ecb_cntx}. A typical
application will furthemore store the @var{fd} and the @var{watch_for}
value in the application context.

Finally, @var{ecb_cntx} is provided because after registration, the
application is expected to call
@ref{MHD_daemon_event_update,,@code{MHD_daemon_event_update()}}
whenever the specified type of event is triggered, and
@code{MHD_daemon_event_update()} requires the @var{ecb_cntx} to allow
MHD to understand which socket changed state.


@anchor{external-trigger}
@node libmicrohttpd-external-trigger
@section Notifying MHD that a socket is ready

When the application's external event loop learns that a socket
is ready for an event MHD registered for, it must call
@code{MHD_daemon_event_update()}.


@anchor{MHD_daemon_event_update}
@deftypefun void MHD_daemon_event_update (struct MHD_Daemon *daemon, struct MHD_EventUpdateContext *ecb_cntx, enum MHD_FdState fd_current_state)

Update the socket's state for MHD.
Must be called for every socket for which the status changed.
In @code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL()} mode
should be called for each registered socket.
In @code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE()} mode
should be called only when a socket becomes ready for the specified
event.

Available only for daemons stated in
@code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL} or
@code{MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_EDGE} mode.

@table @var
@item ecb_cntx
context handle provided during the call
to the @code{MHD_SocketRegistrationUpdateCallback}

@item fd_current_state
the current state of the socket

@end table
@end deftypefun

Note that calling @code{MHD_daemon_event_update()} does not
cause MHD to actually work on the events: MHD will merely
update its internal state. To actually do work, applications
must still call @code{MHD_daemon_process_reg_events()}.


@anchor{external-runner}
@node libmicrohttpd-external-runner
@section Processing externally triggered events

@anchor{MHD_daemon_process_reg_events}
@deftypefun {enum MHD_StatusCode} MHD_daemon_process_reg_events (struct MHD_Daemon *daemon, uint_fast64_t *next_max_wait)

Process registered network events and updates sockets registration.
This function first processes @emph{all}
previously registered (via @code{MHD_daemon_event_update()})
network events (if any) and then calls the
@code{MHD_SocketRegistrationUpdateCallback}
callback for every socket that needs to be added/updated/removed
from the external event loop.

This function is only available only for daemons stated in
@code{MHD_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL} or
@code{MHD_WM_EXTERNAL_EVENT_LOOP_CB_EDGE} mode.

@table @var
@item daemon
daemon to process connections of

@item next_max_wait
address where MHD will store the next maximum wait time in
microseconds to be used for the external event loop as an upper limit;
can be @code{NULL} (but that is uncommon as usually the external event
loop needs to learn the timeout).

@end table

Returns @code{MHD_SC_OK} on success, otherwise an
error code.
@end deftypefun


@node libmicrohttpd-external-example
@section External event loop example: select

The following is a simple example for how to implement
an external event loop using @code{select()} using
a level trigger.

@example c
@verbatiminclude examples/external-select.c
@end example

@c FIXME: add one for epoll() with edge trigger!
