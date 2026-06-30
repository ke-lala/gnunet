When handling HTTP requests from clients in a
@code{MHD_RequestCallback} the main result of the handler is always a
@code{struct MHD_Action}.  The action returned by the handler
determines how the processing of the HTTP request should continue.

@anchor{MHD_Action}
@deftp {C Struct} MHD_Action
Actions are returned by the application to MHD
to drive the request handling of MHD.
@end deftp

It is important to remember that the
@ref{MHD_RequestCallback,,@code{MHD_RequestCallback}} is
invoked immediately after MHD has parsed the HTTP request
@emph{headers}.  This means, the client may not yet have uploaded the
request body (for example, if the request is using the POST
method). Depending on the HTTP protocol version and the size of the
upload, the client may actually wait for confirmation (``101
Continue'') from the HTTP server before proceeding with the upload.
Thus, when the @code{MHD_RequestCallback} is invoked, we generally
have several choices:

@itemize @bullet{}
@item Return an HTTP response immediately. If the client would have
proceeded to send a body, this may possibly preempt it from
uploading a body by denying the request.

@item Fail hard by closing the connection. This is usually the
least desirable choice and should be reserved to cases where the
server has no resources to generate even an response to indicate
a failure.

@item Suspend handling the request. This can be useful if generating
a response requires some other server-side process to make progress
or to rate-limit overly eager clients. Note that if an application
suspends handling a request, HTTP/1.1-compatible clients may start
the upload anyway (without first waiting for the
"101 Continue" response) as the may fall back to HTTP/1.0 compatibility
in this case. Thus, suspending request handling is not recommended
if an upload is to be prevented.

@item Explicitly allow the client to upload data, parse it and eventually
return a response (or fail hard by closing the connection).
@end itemize

@ref{libmicrohttpd2-responses} discusses generating responses in
detail. This chapter will focus on the @emph{other} three
cases.@footnote{There is a special fifth case which applies when an
HTTP/1.x client requests a ``protocol upgrade''.  This is most often
used to create a WebSocket. Creating an action for such protocol
upgrades is described in @ref{libmicrohttpd-upgrade}.}

@node libmicrohttpd-actions-fail
@section Failing hard by closing the connection

The simplest way to handle a request is to simply close the
connection, not returning any data to the client. This violates
the HTTP protocol, but is of course the ultimate fallback when
regular error handling is not possible.

@anchor{MHD_action_abort_request}
@deftypefun {const struct MHD_Action *} MHD_action_abort_request (struct MHD_Request *request)

Creates an action to tell MHD to close the connection hard
(kind-of breaking HTTP specification). This
is actually simply a macro that is equivalent to just @code{NULL},
except that it also suggests to the compiler that it it should
not emit a warning for not using @var{request}.
@end deftypefun



@node libmicrohttpd-actions-suspend
@section Suspend handling the request

@code{MHD_action_suspend()} is used to suspend a client's
request. MHD will keep the network connection open until the
application explicitly tells it to resume processing.

@anchor{MHD_action_suspend}
@deftypefun {const struct MHD_Action *} MHD_action_suspend (struct MHD_Request *request)

Suspend handling of network data for a given request.  This can
be used to dequeue a request from MHD's event loop for a while.

Suspended requests continue to count against the total number of
requests allowed (per daemon, as well as per IP, if such limits
are set).  Suspended requests will NOT time out; timeouts will
restart when the request handling is resumed.  While a
request is suspended, MHD may not detect disconnects by the
client.

@table @var
@item ctx request
the request for which the action is generated
@end table

The function returns an action to cause a request to be suspended.

@code{NULL} is returned if any action has been already created for the @var{request}.
@end deftypefun

After suspending a request, the application must eventually resume
request handling via @code{MHD_request_resume()}.  MHD will then call
the @code{MHD_RequestCallback} again for the @var{request}, allowing
the application to again inspect the request and return a new action.

@cindex long-polling
@anchor{MHD_request_resume}
@deftypefun void MHD_request_resume (struct MHD_Request *request)

Resume handling of network data for suspended request.  It is safe to
resume a suspended request at any time.  Calling this function on a
request that was not previously suspended will result in undefined
behaviour. @c FIXME: or a call to the panic function!?

@table @var
@item ctx request
the request to resume
@end table
@end deftypefun

If you are using this function in @emph{external} select mode, you
must make sure to run @code{MHD_daemon_process_blocking()} afterwards
(as otherwise the change may not be reflected in the set returned to
your @code{MHD_SocketRegistrationUpdateCallback} and you may end up
with a request that is stuck until the next network activity).


@node libmicrohttpd-actions-upload
@section Processing a request body

There are various ways to create an action that tells MHD that the
application wants to process the body being uploaded by the client. If
the client sent an ``Expect: 100-Continue'' header with its request,
returning any of these will automatically cause MHD to respond with a
``100 Continue'' status and initiate receiving the response from the
client.  Thus, applications should never attempt to explicitly return
``100 Continue'' themselves.

The main function to call to handle uploads discussed in this
section is @code{MHD_action_process_upload()}. While this function
in principle works for any kind of upload, it is not the most
convenient when handling structured data from an HTML form that
is encoded as ``application/x-www-form-urlencoded'' or
``multipart/form-data'' or the (less common) ``text/plain''
HTML5 form.  When handling uploads of these formats, it is
usually better to use @ref{MHD_action_parse_post}.

@anchor{MHD_action_process_upload}
@deftypefun {const struct MHD_Action *} MHD_action_process_upload (struct MHD_Request *request, size_t large_buffer_size, MHD_UploadCallback uc_full, void *uc_full_cls, MHD_UploadCallback uc_inc, void *uc_inc_cls)

Creates an action that handles an upload from a client.

@c FIXME: this isn't exactly elegant. We should see if we can
@c provide a way to generate a TOO_LARGE status response properly instead!
If @var{uc_inc} is @code{NULL} and the body does not fit
into the allocated buffer, the request is aborted without
any response by closing the connection.

@table @var
@item request
the request to create action for;

@item large_buffer_size
how large should the upload buffer be.
May allocate memory from the shared "large"
memory pool if necessary and non-zero is given.
Must be zero if @var{uc_full} is @code{NULL};

@item uc_full
function to call when complete body is
received (only if it fits into @var{upload_buffer_size} bytes;
can be @code{NULL} if @var{uc_inc} is not @code{NULL};
must be @code{NULL} is @var{upload_buffer_size} is zero;

@item uc_full_cls
closure for @var{uc_full};

@item uc_inc
function to call to incrementally process the uploaded data
if the upload if larger than @var{upload_buffer_size}
or if @var{upload_buffer_size} cannot be allocated or
if @var{uc_full} is @code{NULL};
can be @code{NULL} if @var{uc_full} is not @code{NULL};

@item uc_inc_cls
closure for @var{uc_inc}.
@end table

The function returns @code{NULL} on error (out of memory, invalid parameters)
or if any action has been already created for the @var{request},
and otherwise a pointer to the action.
@end deftypefun

@code{MHD_action_process_upload()} is a bit overly complicated
as it conflates the two main use-cases:

@itemize
@item handling of the entire data body in one buffer in memory, and
@item incremental parsing of the upload over time.
@end itemize

In practice, applications are expected to use the two APIs
that separate these use-cases, specifically
@code{MHD_action_process_upload_full()} and
@code{MHD_action_process_upload_inc()}.

@anchor{MHD_action_process_upload_full}
@deftypefun {const struct MHD_Action *} MHD_action_process_upload_full (struct MHD_Request *request, size_t large_buffer_size, MHD_UploadCallback uc_full, void *uc_full_cls)

Creates an action that handles an upload from a client
as a full upload data where MHD places all uploaded data
into a buffer and then gives the result to the application
all at once.

@c FIXME: this isn't exactly elegant. We should see if we can
@c provide a way to generate a TOO_LARGE status response properly instead!
If @var{uc_inc} is @code{NULL} and the body does not fit
into the allocated buffer, the request is aborted without
any response by closing the connection.

@table @var
@item request
the request to create action for

@item large_buffer_size
how large should the upload buffer be.
May allocate memory from the shared "large"
memory pool if necessary. Usually the value
given here will be the size of the upload as
provided by the HTTP request header.

@item uc_full
function to call when complete body is
received (only if it fits into @var{upload_buffer_size} bytes;

@item uc_full_cls
closure for @var{uc_full}.
@end table

The function returns @code{NULL} on error (out of memory, invalid parameters)
or if any action has been already created for the @var{request},
and otherwise a pointer to the action.
@end deftypefun

@anchor{MHD_action_process_upload_inc}
@deftypefun {const struct MHD_Action *} MHD_action_process_upload_inc (struct MHD_Request *request, MHD_UploadCallback uc_inc, void *uc_inc_cls)

Creates an action that handles an upload from a client incrementally.

@table @var
@item request
the request to create action for;

@item uc_inc
function to call to incrementally process the uploaded data;

@item uc_inc_cls
closure for @var{uc_inc}.
@end table
@end deftypefun

In all three cases, the application must provide callbacks to handle
the uploaded data. These callbacks have the type @code{MHD_UploadCallback}.
The application is provided with the uploaded data (either all of it at
once or just the latest increment) and must return a
@code{struct MHD_UploadAction} to determine how to continue handling
the @var{request}.


@anchor{MHD_UploadCallback}
@deftypefn {Function Pointer} {struct MHD_UploadAction *} (*MHD_UploadCallback) (void *cls, struct MHD_Request *request, size_t content_data_size, void *content_data)

Functions of this type are invoked by MHD whenever an application
should process the HTTP body of a client's request.

@table @var
@item upload_cls
custom closure provided by the application together with the
pointer to the function

@item request
the request that is being processed

@item content_data_size
number of bytes in @var{content_data},
zero when all data have been processed;

@item content_data
@var{content_data_size} bytes of the HTTP request body;
can be safely modified in the callback;
the buffer is valid only until the application
returns from the callback;
@code{NULL} when all data of the body have been processed;

@end table
The returned @code{struct MHD_UploadAction}
informs MHD how to proceed further with the
@var{request}.  Returning @code{NULL} is
equivalent to @code{MHD_upload_action_abort_request()}.
@end deftypefn

The @code{struct MHD_UploadAction} is quite similar to
the @ref{MHD_Action,,@code{struct MHD_Action}} except that it is only
applicable when processing client uploads.

@anchor{MHD_UploadAction}
@deftp {C Struct} MHD_UploadAction
Actions are returned by the application to MHD
to drive the request handling of MHD.
@end deftp

There are four basic @code{struct MHD_UploadAction}s that
applications can use:

@c FIXME: the fact that there are upload actions that only
@c work for incremental upload processing suggests to me
@c again that we should just have MHD_Action and not Upload/DCC actions.
@itemize
@item continue processing the upload (for incremental upload processing only)
@item suspend processing the upload (say to slow down the client to allow
  the application to keep up)
@item abort handling the request (closing the connection), and
@item returning an HTTP response
@end itemize

The following functions generate the respective upload actions.


@anchor{MHD_upload_action_continue}
@deftypefun {const struct MHD_UploadAction *} MHD_upload_action_continue (struct MHD_Request *request)

Action telling MHD to continue processing the upload.
Valid only for incremental upload processing.

@table @var
@item request
the request to create the upload action for;
@end table

Fails and returns @code{NULL} when used in combination with a
full upload callback, for the final (with zero bytes of data)
call to an incremental callback or on a request that is not
handling an upload.
@c FIXME: the fact that we fail here if an upload action
@c is created for a request that isn't doing an upload suggests to me
@c again that we should just have MHD_Action and not Upload/DCC actions.

@end deftypefun


@anchor{MHD_upload_action_abort_request}
@deftypefun {const struct MHD_UploadAction *} MHD_upload_action_abort_request (struct MHD_Request *request)

Aborts handling the request by closing the connection. This
is actually simply a macro that is equivalent to just @code{NULL},
except that it also suggests to the compiler that it it should
not emit a warning for not using @var{request}.

@table @var
@item request
the request to create the upload action for;
@end table
@end deftypefun


@anchor{MHD_upload_action_from_response}
@deftypefun {const struct MHD_UploadAction *} MHD_upload_action_from_response (struct MHD_Request *request, struct MHD_Response *response)

Converts a @var{response} to an upload action.  If
@code{MHD_R_O_REUSABLE} is not set, the reference to the
@var{response} is consumed by the conversion. If
@code{MHD_R_O_REUSABLE} is @code{MHD_YES}, then the @var{response} can
be used again to create other actions in the future.  However, the
@var{response} is frozen by this step and must no longer be modified
(i.e. by setting headers).

@table @var
@item request
the request to create the upload action for;

@item response
the response to return to the client.
@end table

The function returns a pointer to the upload action; the upload action
must be consumed otherwise the response object may leak; The function
returns @code{NULL} if it failed (no memory) or if any other upload
action has been already created for the @var{request} (or if the
request is currently not in a state of handling an upload).  When the
function fails, the response object is nevertheless consumed and need
not be "destroyed" by the application.

@end deftypefun

@anchor{MHD_upload_action_suspend}
@deftypefun {const struct MHD_UploadAction *} MHD_upload_action_suspend (struct MHD_Request *request)

Suspend handling of network data for the given @var{request}.  This can
be used to dequeue a request from MHD's event loop for a while.

Suspended requests continue to count against the total number of
requests allowed (per daemon, as well as per IP, if such limits
are set).  Suspended requests will NOT time out; timeouts will
restart when the request handling is resumed.  While a
request is suspended, MHD may not detect disconnects by the
client.

@table @var
@item request
the request to create the upload action for;
@end table
@end deftypefun

After suspending an upload action, applications must
use @ref{MHD_request_resume,,@code{MHD_request_resume()}}
to resume processing the upload.