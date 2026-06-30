@cindex response

@noindent
Response handling by MHD is asynchronous with respect to the
application execution flow. Instances of type @code{struct MHD_Response}
are not tightly associated with a request or a daemon;
they are managed with reference counting.

@deftp {C Struct} MHD_Response
Handle for a response to be returned to an HTTP client.
A response includes the HTTP status code, HTTP headers,
a body and possibly HTTP footers.
@end deftp

@cindex action

In the simplest case we allocate a new @code{struct MHD_Response}
for each request, use it to create an action and it is (automatically)
destroyed:

@example
@verbatiminclude examples/simple_reply_fragment.c
@end example

The above code will cause MHD to @code{close()} a connection in
case the memory allocation failed (and log nothing), but unless
an application really needs to handle even such an error more
gracefully, the above code is already fine even in terms of
error handling and leaks no memory (assuming the action is
returned to MHD).

However, MHD also allows responses to be reused and the same response
structure to be used multiple times. This is explained in more detail
under @ref{MHD_R_OPTION_REUSABLE} in the section on
@ref{libmicrohttpd-response-performance,,Response options for
performance optimization}.

@menu
* libmicrohttpd-response enqueue::  Returning a response.
* libmicrohttpd-response create::   Creating responses.
* libmicrohttpd-response headers::  Adding headers to a response.
* libmicrohttpd2-response options:: Setting response options.
* libmicrohttpd-upgrade::           Creating a response for protocol upgrades.
@end menu

@c ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

@c ------------------------------------------------------------
@node libmicrohttpd-response enqueue
@section Enqueuing a response

As we have discussed in the previous chapters and shown in the example
above, handling a request requires applications to return a
@code{struct MHD_Action} that tells MHD how to continue. The most
common way to create an action is from a @code{struct MHD_Response}
via @code{MHD_action_from_response}.

@anchor{MHD_action_from_response}
@deftypefun {struct MHD_Action *} MHD_action_from_response (struct MHD_Request *request, struct MHD_Response *response)

This instructs MHD to transmit the given @var{response} to
the client. The @var{request} must match the request handle
given to the call that is returning the action.

@table @var
@item request
the request to create an action for;

@item response
the response to convert. If this argument is NULL then this
function will behave equivalent to @code{MHD_action_abort_connection()}.
@end table

Returns an action on success. The action returned must be returned to
MHD, otherwise there @emph{may} be a memory leak.  This function
checks that no other action was already created for the given
@var{request} and returns @code{NULL} on API usage errors or possibly
in out-of-memory situations.  The application @emph{may} try to create
a different action in this case, but most likely should just give MHD
the @code{NULL} value for the action to close the connection.

@end deftypefun

Note that calling @code{MHD_action_from_response()} will typically
consume a response and cause its resources to be released
by MHD once the response has been sent. Thus, after converting
a response to an action applications do not have to call
@code{MHD_response_destroy()} @emph{unless} they used
@ref{MHD_R_OPTION_REUSABLE} as explained below.


@node libmicrohttpd-response create
@section Creating responses

@cindex lifetime
@subsection Destroying responses

There are various ways for applications to create a @code{struct
MHD_Response}.  Almost all of these functions will have to allocate
memory and may fail, returning @code{NULL} to indicate that the
response generation failed. To avoid leaking memory, any response that
was successfull created must @emph{either} be converted into an action
via @code{MHD_action_from_response()} or explicitly destroyed via
@code{MHD_response_destroy()}.

@anchor{MHD_response_destroy}
@deftypefun void MHD_response_destroy (struct MHD_Response *response)
Destroys a response and associated resources (decrement the
reference counter).  Note that MHD may keep some of the resources
around if the response is still in the queue for some clients, so the
memory may not necessarily be freed immediately.

@table @var
@item response
the response to destroy.
@end table
@end deftypefun

@subsection HTTP status codes
@cindex status code

When creating a response, applications must always specify the HTTP
status code that should be returned with the response.  MHD provides
@code{enum MHD_HTTP_StatusCode} with all supported HTTP status
codes. Using other numeric values is possible but not advised: in such
cases MHD will be unable to provide the human-readable status code
description and clients are likely to be unhappy about the use of
non-standard codes.

@anchor{MHD_HTTP_StatusCode}
@deftp {Enumeration} MHD_HTTP_StatusCode

Represents canonical HTTP status codes.

@table @code
@include manual/http-status-texi.gen
@end table
@end deftp


@subsection Responses from memory buffers

You can use any of the following functions (or convenience macros)
to create responses from static data in memory:

@anchor{MHD_response_from_buffer}
@deftypefun {struct MHD_Response *} MHD_response_from_buffer (enum MHD_HTTP_StatusCode sc, size_t buffer_size, const char *buffer, MHD_FreeCallback free_cb, void *free_cb_cls)

Create a response object with a callback that can be used to free resources after the response has been sent to the client.

@table @var
@item sc
HTTP status code for the response;

@item size
size of the data portion of the response;

@item buffer
the body to return, must contain at least @var{size} bytes of data;
the application must ensure that data in @var{buffer} remains valid
at least until the response was sent;

@item free_cb
function to call to release application data associated with
the response (usually @var{buffer}) after transmitting the
response is complete. For example, if @var{buffer} is the
result of @code{mmap()} the callback may invoke
@code{munmap()}. Similarly, if @var{buffer} was part of an
allocation on the heap, the callback may simply call
@code{free()}. You may pass @code{NULL} to indicate that
no cleanup function is needed; however, in that case it
might be simpler to use @code{MHD_response_from_buffer_static()}.

@item free_cb_cls
additional argument to pass to the @var{free_cb}.
A common value to pass is @var{buffer}.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

@anchor{MHD_FreeCallback}
@deftypefn {Function Pointer} void (*MHD_FreeCallback) (void *cls)

Functions of this type are invoked by MHD whenever it wants to give
the application a chance to free resources.

@table @var
@item cls
custom value provided by the application together with the
actual function pointer; should identify the resources to be released
@end table
@end deftypefn

When releasing resources is unnecessary, applications can instead
use the simplified @code{MHD_response_from_buffer_static()} API
that simply omits the @var{free_cb} and @var{free_cb_cls} arguments.

@anchor{MHD_response_from_buffer_static}
@deftypefun {struct MHD_Response *} MHD_response_from_buffer_static (enum MHD_HTTP_StatusCode sc, size_t buffer_size, const char *buffer)
Create a response object without the need to free resources after the response has been sent to the client.

@table @var
@item sc
HTTP status code for the response.

@item size
size of the data portion of the response;

@item buffer
the body to return, must contain at least @var{size} bytes of data;
the application must ensure that data in @var{buffer} remains valid
at least until the response was sent; can be NULL if @var{size} is zero,
but in this case it might be simpler to use @code{MHD_response_from_empty()}.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

@anchor{MHD_response_from_buffer_copy}
@deftypefun {struct MHD_Response *} MHD_response_from_buffer_copy (enum MHD_HTTP_StatusCode sc, size_t buffer_size, const char buffer[buffer_size])

Create a response object from an ephemeral @var{buffer}. The data
in the @var{buffer} will be copied into internal storage of MHD.

@table @var
@item sc
HTTP status code for the response.

@item size
size of the data portion of the response;

@item buffer
the body to return, must contain at least @var{size} bytes of data;
the @var{buffer} can become invalid immediately after the
call to @code{MHD_response_from_buffer_copy()} returns as MHD
will make a copy of the data contained in it.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

@anchor{MHD_response_from_empty}
@deftypefun {struct MHD_Response *} MHD_response_from_empty (enum MHD_HTTP_StatusCode sc)
Create an response with an empty body.

@table @var
@item sc
HTTP status code for the response. A common value would be @code{MHD_HTTP_NO_CONTENT}.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

Finally, in rare cases, the response data may not be available in a
continuous block in memory and it might be more efficient to not copy
the data into a continous area just for the network transfer (or
encryption).  In this case, applications can initialize an array of
@code{struct MHD_IoVec} data structures and use
@code{MHD_response_from_iovec()} to point MHD to the various fragments
of the body and enable (if supported) the kernel (or NIC) to assemble
the final response dynamcially from the fragments in
memory.@footnote{See the @code{writev()} POSIX call for an example of
the type of scatter input operationg system interface that MHD might
use to implement this.}

@cindex iovec
@cindex scatter write
@cindex writev
@anchor{MHD_response_from_iovec}
@deftypefun {struct MHD_Response *} MHD_response_from_iovec (enum MHD_HTTP_StatusCode sc, unsigned int iov_count, const struct MHD_IoVec iov[iov_count], MHD_FreeCallback free_cb, void *free_cb_cls)

Create a response object from an array of memory buffers.
@table @var
@item sc
HTTP status code for the response.

@item iov_count
the number of elements in @var{iov};

@item iov
the array for response data buffers of length @var{iov_count}, an internal copy of the @var{iov} will be made; however, note that the data pointed to by the @var{iov} is @strong{not} copied and must be preserved unchanged at the given locations until the response is no longer in use and the @var{free_cb} is called;

@item free_cb
the callback to call to free resources associated with @var{iov};
You may pass @code{NULL} to indicate that no cleanup function is needed.

@item free_cb_cls
additional argument to pass to the @var{free_cb}.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

@anchor{MHD_IoVec}
@deftp {C Struct} MHD_IoVec
Input/output vector type. Informations MHD about a fragment of
data in memory for assembly as part of an HTTP response body.
@table @var
@item @code{const void *} iov_base
Pointer to the memory region for I/O.
@item @code{size_t} iov_len
The size (in bytes) of the memory region pointed to by @var{iov_base}.
@end table
@end deftp


@subsection Responses from file descriptors
@cindex sendfile

@anchor{MHD_response_from_fd}
@deftypefun {struct MHD_Response *} MHD_response_from_fd (enum MHD_HTTP_StatusCode sc, int fd, uint_fast64_t offset, uint_fast64_t size)
Create a response object based on a file descriptor @var{fd} from
which the body is supposed to be read.

@table @var
@item sc
HTTP status code for the response.

@item fd
file descriptor referring to a file on disk with the
data; MHD will @code{close()} the file descriptor when
the response is destroyed; @var{fd} should be in @emph{blocking} mode;
@var{fd} must be an actual file descriptor (not a pipe or socket)
since MHD might use @code{sendfile()} or @code{seek()} on it;

@item offset
offset to start reading from in the file;
readings file beyond 2 GiB may be not supported by some
operationg systems or MHD builds.
@xref{MHD_LIB_INFO_FIXED_HAS_LARGE_FILE}.

@item size
size size of the data portion of the response;
sizes larger than 2 GiB may be not supported by
some operating systems or MHD builds.
@xref{MHD_LIB_INFO_FIXED_HAS_LARGE_FILE}.
@end table

Returns @code{NULL} on error (i.e. invalid arguments, out of memory).
The @var{fd} is closed on error.
@end deftypefun

@cindex splice
@anchor{MHD_response_from_pipe}
@deftypefun {struct MHD_Response *} MHD_response_from_pipe (enum MHD_HTTP_StatusCode sc, int fd)

Create a response object where the response body is created by reading from
the provided pipe.
@c FIXME: will it also work for sockets? Is this using splice()? Clarify!

@table @var
@item sc
HTTP status code for the response.

@item fd
file descriptor of the read-end of a pipe; will be
closed when response is destroyed.
The descriptor should be in blocking-IO mode.
@end table

Returns @code{NULL} on error (i.e. out of memory).
@c FIXME: closes fd on error?
@end deftypefun


@subsection Responses from dynamic generators

This API is useful for applications that dynamically create the
response body, possibly even with significant delays between
chunks.

@c FIXME: this comment seems wrong, see above for from_pipe()!
@c This is also the only API that applications can use to
@c return responses for which they do not know the length up-front.

@cindex dynamic responses
@cindex generation

@anchor{MHD_response_from_callback}
@deftypefun {struct MHD_Response *} MHD_response_from_callback (enum MHD_HTTP_StatusCode sc, uint_fast64_t size, MHD_DynamicContentCreator dyn_cont, void *dyn_cont_cls, MHD_FreeCallback dyn_cont_fc)

Create a response object with a body that will be yielded over
time by the @code{dyn_cont} callback.

@table @var
@item sc
HTTP status code for the response.

@item size
size of the body of the response, applications should pass
@code{MHD_SIZE_UNKNOWN} if the size of the body is not yet known;

@item dyn_cont
callback that MHD will use to obtain the response body

@item dyn_cont_cls
extra argument that MHD will pass to @var{dyn_cont} and
@var{dyn_cont_fc}

@item dyn_cont_fc
callback that MHD will call to allow the application to
free resources associated with @var{dyn_cont_cls}.
@end table

Return @code{NULL} on error (i.e. invalid arguments, out of memory).
@end deftypefun

The interesting work for dynamic response generation happens in the
dynamic content creation callback, which must be a function of
type @code{MHD_DynamicContentCreator}.

@anchor{MHD_DynamicContentCreator}
@deftypefn {Function Pointer} {struct MHD_DynamicContentCreatorAction *} (*MHD_DynamicContentCreator) (void *dyn_cont_cls, struct MHD_DynamicContentCreatorContext *ctx, uint_fast64_t pos, void *buf, size_t max)

@table @var
@item dyn_cont_cls
closure argument that was associated with the callback

@item ctx
the context to produce the action to return;
the pointer is only valid until the callback returns;
equivalent to the @var{request} argument given to
functions of type @code{MHD_RequestCallback};

@item pos
position in the datastream to access;
note that if a @code{struct MHD_Response} is re-used,
it is possible for the same dynamic content creator to
be queried multiple times for the same data;
however, if a @code{struct MHD_Response} is not re-used,
libmicrohttpd guarantees that @var{pos} will be
the sum of all data sizes previously yielded by this callback

@item buf
buffer where the application may write data that should be
included in the body at offset @var{pos}; @var{buf} has
@var{max} bytes available;

@item max
maximum number of bytes the application may copy to @var{buf};
if the size of the content of the response is known then the
size of the buffer is never larger than amount of data left
@end table

Most importantly, the callback must also return a
@code{struct MHD_DynamicContentCreatorAction} which determines
the next step in creating the body. Applications may
return @code{NULL} in which case response generation will
be aborted and MHD will @code{close()} the connection.
@end deftypefn

@cindex dynamic content creator action

@anchor{MHD_DynamicContentCreatorAction}
@deftp {C Struct} MHD_DynamicContentCreatorAction
Special type of action that must be returned by a
@code{MHD_DynamicContentCreator}.
@end deftp

Various possibilities exist for applications to create
an @code{struct MHD_DynamicContentCreatorAction} (DCC action). Note
that at most one @code{struct MHD_DynamicContentCreatorAction}
can be created per @var{ctx}.

We begin with DCC actions that end the process of
dynamic response generation:

@anchor{MHD_DCC_action_abort}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_abort (struct MHD_DynamicContentCreatorContext *ctx)

Aborts handling the request by closing the connection. This
is actually simply a macro that is equivalent to just @code{NULL},
except that it also suggests to the compiler that it it should
not emit a warning for not using @var{ctx}.

@end deftypefun

@anchor{MHD_DCC_action_finish}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_finish (struct MHD_DynamicContentCreatorContext *ctx)

Create a "finished" action that signals MHD that the dynamic content creation
is finished.  Note that this will cause MHD to @code{close()} the connection
if the amount of data yielded by the content creation logic is less than
the @var{size} promised to @code{MHD_response_from_callback()}.

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated.
@end table

@code{NULL} is returned in case of any error.
@end deftypefun


HTTP has not only headers, but also the lesser-known concept of
@emph{footers}. Applications can specify footers (for example,
to append a checksum after dynamic content creation) using
@code{MHD_DCC_action_finish_with_footer()}.

@anchor{MHD_DCC_action_finish_with_footer}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_finish_with_footer (struct MHD_DynamicContentCreatorContext *ctx, size_t num_footers, const struct MHD_NameValueCStr *footers)

Create a "finished" action that signals MHD that the dynamic content creation
is finished.  Note that this will cause MHD to @code{close()} the connection
if the amount of data yielded by the content creation logic is less than
the @var{size} promised to @code{MHD_response_from_callback()}.

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated;

@item num_footers
length of the @var{footers} array;

@item footers
array of @var{num_footers} HTTP footers to return.
@end table

@code{NULL} is returned in case of any error.
@end deftypefun

The @var{footers} are provided as key-value pairs of 0-terminated C strings
in a @code{struct MHD_NameValueCStr}. In this context, both members must
not be @code{NULL}.

@anchor{MHD_NameValueCStr}
@deftp {C Struct} MHD_NameValueCStr name value
Name with value pair as C strings.
@table @var
@item @code{const char *} name
The name or key of the field. Must never be @code{NULL}.
@item @code{const char *} value
The value to be returned under the name or key.
In some cases, @var{value} is allowed to be absent,
which is indicated by a @code{NULL} pointer.
@end table
@end deftp

@cindex long-polling

Applications may not want to finish generating a response, but also
may not (yet) have more data to return to the client. In this case,
applications must @emph{suspend} response generation. This is done
using @code{MHD_DCC_action_suspend()}.

@anchor{MHD_DCC_action_suspend}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_suspend (struct MHD_DynamicContentCreatorContext *ctx)

Creates a DCC action that suspends response generation. The connection
is not closed! MHD merely waits for the application to resume response
generation via a call to @code{MHD_request_resume()}. This is
especially useful for long-polling where response generation is not
computationally bound. This is common in cases where client and server
need to wait (possibly for an extended period of time) on some
external event to happen before the server can generate a response.

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated;
@end table

@code{NULL} is returned in case of any error.
@end deftypefun

After dynamic content creation was suspended with
@code{MHD_DCC_action_suspend()}, applications @strong{must}
(eventually) call @code{MHD_request_resume()} to continue
processing the client's request.

Finally, we consider DCC actions that can be used to signal MHD data to be
returned in the MHD body.

@anchor{MHD_DCC_action_continue}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_continue (struct MHD_DynamicContentCreatorContext *ctx, size_t data_size)

Creates an action that indicates that a chunk of @var{data_size} bytes
was provided in the @var{buf} that MHD provided to the
@code{MHD_DynamicContentCreator} callback
and that MHD should continue to call the content generator for more
(as soon as MHD has more buffer space available).

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated;

@item data_size
number of bytes that were written by the application into @var{buf};
must be non-zero;
@end table

@code{NULL} is returned in case of any error.
@end deftypefun

A small variation of the above API can be used to associate
a chunk-extension (see RFC 9112, 7.1) with the chunk:

@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_continue_ce (struct MHD_DynamicContentCreatorContext *ctx, size_t data_size, const char *chunk_ext)

Creates an action that indicates that a chunk of @var{data_size} bytes
was provided in the @var{buf} that MHD provided to the
@code{MHD_DynamicContentCreator} callback
and that MHD should continue to call the content generator for more
(as soon as MHD has more buffer space available).
Furthermore, the chunk of data that was returned is optionally
associated with a chunk-extension (@var{ce}) that MHD should
include in the transmission if possible.

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated;

@item data_size
number of bytes that were written by the application into @var{buf};
must be non-zero;

@item chunk_ext
optional pointer to a chunk extension string;
can be @code{NULL} to not use chunk extension,
ignored if chunked encoding is not used by MHD
(say because the client is using HTTP/1.0 and
the connection is only used for a single request).
@end table

@code{NULL} is returned in case of any error.
@end deftypefun

While writing new data that an application may dynamically generate
directly into MHD's @var{buf} is usually ideal, copying existing
data over into the buffer provided by MHD may not be ideal.
Applications can also instruct MHD to include data from
locations in the application's memory, and in fact it is
possible to @emph{mix} both strategies using
@code{MHD_DCC_action_continue_zc()}:

@anchor{MHD_DCC_action_continue_zc}
@deftypefun {struct MHD_DynamicContentCreatorAction *} MHD_DCC_action_continue_zc (struct MHD_DynamicContentCreatorContext *ctx, size_t data_size, const struct MHD_DynContentZCIoVec *iov_data, const char *chunk_ext)

Yields a chunk of body data to MHD with (optional) chunk-extension.
The data is provided in the MHD buffer and/or in the zero-copy @var{iov_data}.

If data is provided both in the MHD buffer and @var{ivo_data} then
data in the MHD buffer is sent first, followed by the @var{iov_data}.
The total size of the data in the buffer and in @var{iov_data} @strong{must}
be non-zero.

@table @var
@item ctx
context that was given to the @code{MHD_DynamicContentCreator} for
which a @code{struct MHD_DynamicContentCreatorAction} should be generated;

@item data_size
the amount of the data placed to the provided MHD buffer (@var{buf}),
cannot be larger than provided buffer size,
must be non-zero if @var{iov_data} is @code{NULL} or has no data;

@item iov_data
optional pointer to a vector of data fragments to send,
must not be NULL and have non-zero bytes of data if @var{data_size}
is zero;

@item chunk_ext
optional pointer to a chunk extension string;
can be @code{NULL} to not use chunk extension,
ignored if chunked encoding is not used by MHD
(say because the client is using HTTP/1.0 and
the connection is only used for a single request).
@end table

It is an error if the total response body size is known and the
amount of data yielded in total by the dynamic content creation logic
exceeds that amount. In this case, this function will fail and
return @code{NULL}.

@end deftypefun

The @code{struct MHD_DynContentZCIoVec} adds an application
cleanup callback to an array of @code{struct MHD_IoVec}s:

@cindex iovec
@cindex writev
@cindex scatter write

@anchor{MHD_DynContentZCIoVec}
@deftp {C Struct} MHD_DynContentZCIoVec
Structure to pass various memory buffers to MHD together with
a callback for cleaning up after MHD is done using the data.
@table @var
@item @code{size_t} iov_count
The length of the @var{iov} array;

@item @code{const struct MHD_ioVec *} iov
Pointer to an array of length @var{iov_count} identifying data chunks
to return as part of the body;

@item @code{MHD_FreeCallback} iov_fcb
Function to call to free resources associated with @var{iov}.
Can be @code{NULL} if releasing these resources is not necessary
after finishing the response generation;

@item @code{void *} iov_fcb_cls
Parameter to pass to @var{iov_fcb}.
@end table

@end deftp


@node libmicrohttpd-response headers
@section Adding headers to a response
@cindex header
@cindex Connection
@cindex Date
@cindex Content-Length
@cindex Transfer-Encoding

After creating a response, but @emph{before} using it to create an
@code{struct MHD_Action}, applications may add custom HTTP headers
to a response. Note that MHD will automatically set some common
HTTP headers, in particular ``Content-Length'', ``Transfer-Encoding'',
``Date`` and ``Connection''.

@code{MHD_response_add_header()} prevents applications from setting a
``Transfer-Encoding'' header to values other than ``identity'' or
``chunked'' as other transfer encodings are not supported by MHD. Note
that usually MHD will pick the correct transfer encoding
automatically; however, applications can use the header to force a
particular behavior.

@code{MHD_response_add_header()} prevents applications from setting a
``Connection'' header to values other than ``close'' or
``keep-alive''. Note that usually MHD will pick a sane value
automatically; however, applications can use the header to force a
particular behavior.

@code{MHD_response_add_header()} also prevents applications from setting a
``Content-Length'' header unless special options are set for the
response (see @ref{libmicrohttpd2-response options,,response options}) as
MHD will automatically set a correct ``Content-Length'' header if it
is possible and allowed by the HTTP protocol.

@anchor{MHD_response_add_header}
@deftypefun {enum MHD_StatusCode} MHD_response_add_header (struct MHD_Response *response, const char *name, const char *value)

Add a header line to the response.

@table @var
@item response
which response to add a header for, @code{NULL} is tolerated;

@item name
the name of the header to add;
an internal copy of the string will be made;
must not contain newlines, carriage returns or
tabulator characters;

@item value
the value of the header to add;
an internal copy of the string will be made;
must not contain newlines, carriage returns or
tabulator characters.
@end table

On success @code{MHD_SC_OK} is returned.
@end deftypefun

@cindex HTTP2
@cindex predefined HTTP header
@cindex header

HTTP2 introduced @emph{predefined} (standard) HTTP headers. For such
headers, it is (slightly) more efficient to set the headers using
values from the @code{enum MHD_PredefinedHeader} instead of specifying
the name as a string.@footnote{The main reason being that HTTP2 can
encode these headers using a number instead of a string, and by having
the application directly provide the numeric value MHD does not have
to map the string to the number.}

@deftypefun {enum MHD_StatusCode} MHD_response_add_predef_header (struct MHD_Response *response, enum MHD_PredefinedHeader stk, const char *content)
Adds a header line to the response.

@table @var
@item response
which response to add a header for, @code{NULL} is tolerated;

@item stk
code of the predefined header;

@item content
the value of the header to add
 an internal copy of the string will be made.
@end table

Notice that the content must not contain newlines, carriage returns or
tabulator characters. On success @code{MHD_SC_OK} is returned.
@end deftypefun

@anchor{MHD_PredefinedHeader}
@deftp {Enumeration} MHD_PredefinedHeader

Predefined list of canonical HTTP headers, useful for HTTP2
header packing (HPACK).

@c FIXME: the table seems rather incomplete (also in libmicrohttpd2.h),
@c and we should probably generate it via GANA (like HTTP status codes)
@table @code
@item MHD_PREDEF_ACCEPT_CHARSET
15: ``Accept-Charset``
@item MHD_PREDEF_ACCEPT_LANGUAGE
17: ``Accept-Language``
@end table
@end deftp

@node libmicrohttpd2-response options
@section Setting response options
@cindex option

This section is about setting response options. In principle, setting
response options works like setting other options in
MHD. @xref{libmicrohttpd2-doptions,,Configuring your HTTP daemon}.

MHD response options are represented using a @code{struct
MHD_ResponseOptionAndValue}.  However, applications should never be
concerned with the internals of this data structure, and only use the
provided API functions and macros explained in this chapter to
construct these options.

MHD offers three main ways to set options for a @code{struct MHD_Response}:

@itemize
@item @code{MHD_response_set_options()} sets an array of options,
@item @code{MHD_response_set_option()} sets an individual option, and
@item @code{MHD_RESPONSE_SET_OPTIONS()} sets a list of options.
@end itemize

All three approaches can be combined for the same response
to set various options. We will now look at each of them.

@code{MHD_response_set_options()} is useful if the application needs
complex code to first initialize a set of options, or if it has a
static data buffer with all of the options (say in ROM). It is also
the function use to implement the other styles.

@deftypefun {struct MHD_StatusCode} MHD_response_set_options (struct MHD_Response *response, const struct MHD_ResponseOptionAndValue *options, size_t options_max_num)

@table @var
@item response
the response to configure

@item options
array of options to set, either @code{MHD_D_OPTION_TERIMINATE()}-terminated or
limited by @var{options_max_num} (whatever comes first).

@item options_max_num
maximum number of options to process from the @var{options} array,
or @code{MHD_OPTIONS_ARRAY_MAX_SIZE} to process all options from
@var{options} until the @code{MHD_D_OPTION_TERMINATE()}-terminator.

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code of
the first option that could not be set.
@end deftypefun

To set only a single option, MHD defines a simple convenience API with
@code{MHD_response_set_option()}. This is useful if there is only one
option to be set, or if error handling for that specific option is
somehow special (like not failing when setting this particular option
failed).

@deftypefun {struct MHD_StatusCode} MHD_response_set_option (struct MHD_Response *response, const struct MHD_ResponseOptionAndValue *option)

@table @var
@item response
the response to configure

@item option
the option to set

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code from
trying to set the @var{option}.
@end deftypefun


An easy way to set a bunch of options without having to worry
about explicitly allocating an array or checking each return
value is to use @code{MHD_RESPONSE_SET_OPTIONS()}.

@anchor{MHD_RESPONSE_SET_OPTIONS}
@deftypefun {struct MHD_StatusCode} MHD_RESPONSE_SET_OPTIONS (struct MHD_Response *response, ...)

@table @var
@item response
the response to configure

@item ...
variable-length list of @code{MHD_ResponseOptionAndValue} options
to set; does @emph{not} need to be terminated via
@code{MHD_D_OPTION_TERMINATE()} as such a terminator is added
automatically.

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code of
the first option that could not be set.
@end deftypefun

The remaining sections of this section discuss the various
response options available to applications today.

@include manual/response_options.inc


@node libmicrohttpd-upgrade
@section Creating a response for protocol upgrades
@cindex WebSockets
@cindex HTTP Upgrade
@cindex HTTP2
@cindex RFC2817

With RFC 2817 a mechanism to switch protocols within HTTP was
introduced.  Here, a client sends a request with a ``Connection:
Upgrade'' header.  The server responds with a ``101 Switching
Protocols'' response header, after which the two parties begin to
speak a different (non-HTTP) protocol over the TCP connection.

This mechanism is used for upgrading HTTP 1.1 connections to HTTP2 or
HTTPS, as well as for implementing WebSockets.  Which protocol
upgrade is performed is negotiated between server and client in
additional headers, in particular the ``Upgrade'' header.

MHD supports switching protocols using this mechanism
by returning a special @code{MHD_action_upgrade()} action
from the @ref{MHD_RequestCallback,,@code{MHD_RequestCallback}}.

Note that support for upgrading connections is an @emph{optional}
feature that could be left out especially in embedded builds.  Use
@ref{MHD_LIB_INFO_FIXED_HAS_UPGRADE,,@code{MHD_LIB_INFO_FIXED_HAS_UPGRADE}}
to check if the library supports it. Furthermore, connection upgrades
can be disabled for a daemon using
@code{MHD_D_OPTION_DISALLOW_UPGRADE()} for some potential minor
performance benefit. Thus, you may want to make sure the feature is
actually properly enabled.

@anchor{MHD_action_upgrade}
@deftypefun {const struct MHD_Action *} MHD_action_upgrade (struct MHD_Request *request, const char *upgrade_hdr_value, MHD_UpgradeHandler upgrade_handler, void *upgrade_handler_cls, size_t num_headers, const struct MHD_NameValueCStr *headers)

Create a action object that can be used for 101 Upgrade
responses, for example to implement WebSockets.  After sending the
response, control over the data stream is given to the callback (which
can then, for example, start some bi-directional communication).
The callback will @emph{only} be called after the response header
was successfully passed to the OS; if there are communication errors
before, the usual MHD connection error handling code will be performed.

When creating this type of response, the "Connection: Upgrade"
header will be set automatically for you.


@table @var
@item request
the request to create action for;

@item upgrade_hdr_value
the value of the (mandatory) "Upgrade:" header, must not be @code{NULL};

@item upgrade_handler
function to call with the "upgraded" socket;

@item upgrade_handler_cls
closure for @var{upgrade_handler};

@item num_headers
number of elements in the @var{headers} array,
must be zero if @var{headers} is @code{NULL};

@item headers
optional pointer to an array of additional HTTP headers to return in
the response (the strings will be copied and do not need to be
preserved by the application after this function returns);
can be @code{NULL} if @var{num_headers} is zero;

@end table

@code{NULL} is returned on error (i.e. invalid arguments, out of memory).
@end deftypefun

The @var{upgrade_handler} argument has the following type:

@anchor{MHD_UpgradeHandler}
@deftypefn {Function Pointer} void (*MHD_UpgradeHandler) (void *cls, struct MHD_Request *request, struct MHD_UpgradeHandle *urh)

Applications must implement functions of this type which be called
once MHD has transmitted the header of the response to the connection
that is being upgraded.  At this point, the application is expected to
take over the connection represented by @var{urh} and use @var{urh} to speak
the non-HTTP protocol to which the connection was upgraded with the client.

The application must call @code{MHD_upgraded_send()},
@code{MHD_upgraded_recv()} and eventually @code{MHD_upgraded_close()}
to interact with the client. @code{MHD_upgraded_close()} must be
called before destroying the daemon.

"Upgraded" connection will not time out, but are still counted for daemon
global connections limit and for per-IP limit (if set).

Except when in "thread-per-connection" mode, implementations of this
function should never block (as it will be called from within the main
event loop).

@table @var
@item cls
matches the @code{upgrade_handler_cls} that was given to @code{MHD_action_upgrade()};

@item request
identifies the request of the connection that is being upgraded;

@item urh
handle to interact with the client.
@c FIXME: C comment talks about MHD_upgrade_operation(), seems outdated!
@end table

@end deftypefn

@c FIXME: Evgeny: this is correct, right?
When upgrading a connection, applications using an external event loop may
use the connection @ref{libmicrohttpd-request-status-info,,request
introspection API} to obtain the socket underlying the HTTP request
and then add that socket to their own event loop to decide when to
call @code{MHD_upgraded_recv()} and @code{MHD_upgraded_send()}.

@anchor{MHD_upgraded_recv}
@deftypefun {enum MHD_StatusCode} MHD_upgraded_recv (struct MHD_UpgradeHandle *urh, size_t recv_buf_size, void *recv_buf, size_t *received_size, uint_fast64_t max_wait_millisec)

Receive data on the HTTP-Upgraded connection.

The function will return if one of the following happens:
@itemize
@item Any amount of data has been received,
@item The specified timeout was reached, or
@item a network error occured.
@end itemize

@table @var
@item urh
identifies the HTTP-upgraded connection handle

@item recv_buf_size
size of the @var{recv_buf}

@item recv_buf
application buffer where MHD should place received data

@item received_size
pointer where to return the number of bytes received
@c FIXME: header lacks [out], English not so good

@item max_wait_millisec
the maximum wait time for the data, non-blocking operation if set to zero,
wait indefinitely if set larger or equal to @code{MHD_WAIT_INDEFINITELY};
the function may return earlier if waiting is interrupted for some reason.
@end table
Returns:
@table @code
@item MHD_SC_OK
if any data was received (check the @var{received_size}) or
remote shut down send side (indicated by @var{received_size}
set to zero);

@item MHD_SC_UPGRADED_NET_TIMEOUT
if no data was received but and timeout was reached;

@item MHD_SC_UPGRADED_NET_CONN_CLOSED
if the network connection has been closed,
@c FIXME: should the application still call MHD_upgraded_close()? If so, we should say so explicitly!

@item MHD_SC_UPGRADED_NET_CONN_BROKEN
if a broken network connection has been detected,

@item MHD_SC_UPGRADED_TLS_ERROR
if a TLS error occured (only possible with TLS connections),

@item MHD_SC_UPGRADED_NET_HARD_ERROR
if any other network or sockets unrecoverable error occured,

@item MHD_SC_UPGRADED_HANDLE_INVALID
if @var{urh} is invalid,

@item MHD_SC_UPGRADED_WAITING_NOT_SUPPORTED
if timed wait is not supported by this MHD build or platform
@end table
@end deftypefun

@cindex upgraded handle

After the upgrade, the application must use the
@code{struct MHD_UpgradedHandle} to continue the
interaction with the client.

@anchor{MHD_UpgradedHandle}
@deftp {C Struct} MHD_UpgradedHandle
Handle for response that has been "upgraded" (say to a WebSocket).
@end deftp

Using the handle, the application primarily has
three ways to interact with the client:
@itemize
@item send data,
@item receive data, or
@item close the connection.
@end itemize

@anchor{MHD_upgraded_send}
@deftypefun {enum MHD_StatusCode} MHD_upgraded_send (struct MHD_UpgradeHandle *urh, size_t send_buf_size, const void *send_buf, size_t *sent_size, uint_fast64_t max_wait_millisec, enum MHD_Bool more_data_to_come)

Sends data on an HTTP-Upgraded connection.

The function will return if one of the following happens:
@itemize
@item All provided data has been sent,
@c FIXME: Truly *all*? So always blocking? Not ideal! Discuss!
@item The timeout was reached, or
@item a network error occurs
@end itemize


@table @var
@item urh
identifies the upgraded connection to transmit data on

@item send_buf_size
the number of bytes of data in the @var{send_buf}

@item send_buf
the buffer with the data to send

@item sent_size
the pointer where MHD will return the number of bytes of data actually sent

@item max_wait_millisec
the maximum wait time for the data, non-blocking operation if set to zero,
wait indefinitely if larger or equal to @code{MHD_WAIT_INDEFINITELY}

@item more_data_to_come
should be set to @code{MHD_YES} if the provided data in the
@var{send_buf} is part of a larger data package, like an incomplete
message or part of a data stream (and not the final part), and thus
more data is expected to be sent soon over the same connection; set to
@code{MHD_NO} if the data in the @var{send_buf} is the complete
message or the final part of the message (or file) and it should be
pushed to the network (and to the client) as soon as possible;
basically controls network buffering (see Nagle's algorithm and
TCP PUSH).
@end table

Returns:
@table @code
@item MHD_SC_OK
if any data was sent (check the @var{sent_size});

@item MHD_SC_UPGRADED_NET_TIMEOUT
if no data was sent and the timeout was reached;

@item MHD_SC_UPGRADED_NET_CONN_CLOSED
if the network connection has been closed,
@c FIXME: should the application still call MHD_upgraded_close()? If so, we should say so explicitly!

@item MHD_SC_UPGRADED_NET_CONN_BROKEN
if a broken network connection has been detected,

@item MHD_SC_UPGRADED_TLS_ERROR
if a TLS error occured (only possible with TLS connections),

@item MHD_SC_UPGRADED_NET_HARD_ERROR
if any other network or sockets unrecoverable error occured,

@item MHD_SC_UPGRADED_HANDLE_INVALID
if @var{urh} is invalid,

@item MHD_SC_UPGRADED_WAITING_NOT_SUPPORTED
if timed wait is not supported by this MHD build or platform
@end table

@end deftypefun


@anchor{MHD_upgraded_close}
@deftypefun {enum MHD_StatusCode} MHD_upgraded_close (struct MHD_UpgradeHandle *urh)
Close HTTP-Upgraded connection handle.

The handle cannot be used after successful return from this function.

The function cannot fail if called correctly (the daemon is not destroyed
and the upgraded connection has not been closed previously).

@table @var
@item urh
identifies the upgraded connection to close
@end table
Returns @code{MHD_SC_OK} on success, otherwise an error code.
@end deftypefun
