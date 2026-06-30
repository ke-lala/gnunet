A @code{struct MHD_Request} is given by MHD to the application
to refer to an individual HTTP request by a client.  Applications
primarily are given a request handle as part of the arguments given
to the central @ref{MHD_RequestCallback,,@code{MHD_RequestCallback}}
of each @code{struct MHD_Daemon}.

@deftp {C Struct} MHD_Request
Handle representing an HTTP request.

With HTTP/1.1, multiple requests can be run over the same
stream.  However, MHD will only show one request per data
stream to the application at any given time.
@end deftp

This chapter discusses how to inspect the information
from the request @emph{header}.
@xref{libmicrohttpd-actions-upload} for handling data uploaded
by HTTP clients in the body of a request.


@node libmicrohttpd-requests-name-value-info
@section Inspecting request name-value pairs

When inspecting requests, MHD classifies values from a client's
request into various @emph{kinds} of values. The different kinds
are represented by the @code{enum MHD_ValueKind}.

@anchor{MHD_ValueKind}
@deftp {Enumeration} MHD_ValueKind
@cindex header
@cindex kind

Specifies the source of the name-value pairs in the HTTP protocol.

@table @code
@item MHD_VK_HEADER
HTTP header (single line in the header).

@item MHD_VK_COOKIE
Cookies.  Note that the original HTTP header containing
all the cookie(s) is also available via @code{MHD_VK_HEADER}.

@item MHD_VK_URL_ARGUMENT
URL arguments (often called "GET" parameters, but not actually limited to the HTTP GET method).

@item MHD_VK_POSTDATA
POST data.
This is available only if @ref{MHD_action_parse_post,,@code{MHD_action_parse_post()}} is used,
the body's content encoding is supported by MHD, and only if the posted content
fits within the specified memory buffers.

@cindex Transfer-Encoding
Warning: The encoding ``multipart/form-data'' has more fields than just
``name'' and ``value'' which are not available via the APIs discussed
in this chapter. @xref{MHD_request_get_post_data_cb} and
@ref{MHD_request_get_post_data_list,,@code{MHD_request_get_post_data_list()}} for more information.
In particular it could be important
to check the ``Transfer-Encoding''.

@item MHD_VK_FOOTER
Data from client request HTTP footer (only for HTTP 1.1 chunked encodings).

@item MHD_VK_HEADER_OR_FOOTER
Values from both the HTTP header or footer.

@item MHD_VK_URL_OR_POST
Values from URL arguments or post data.

@end table
@end deftp

Given a value @var{kind} (or possibly a bitmask combining different
kinds), MHD offers various ways to access values from of that kind
for a given @var{request}.

@c FIXME: why int? what would negative values be used for?
@anchor{MHD_request_get_values_cb}
@deftypefun int MHD_request_get_values_cb (struct MHD_Request *request, enum MHD_ValueKind kind, MHD_NameValueIterator iterator, void *iterator_cls)
Get all the headers matching @var{kind} from the request.

@table @var
@item request
the HTTP request to inspect;

@item kind
kinds of name-value pairs of the request to pass to @var{iterator},
can be a bitmask, ORing the various header kinds that are requested
(@xref{MHD_ValueKind});

@item iterator
This callback function is invoked once for each header, with
@var{iterator_cls} as first argument.  The
headers are iterated in the same order as they were received from
the network.
@var{iterator} can be @code{NULL}: in this case this function just counts
and returns the number of headers;

@item iterator_cls
closure argument for @var{iterator}.
@end table

The function returns the number of entries
iterated over; this can be less than the number of headers if, while
iterating, @var{iterator} returns @code{MHD_NO}.
@end deftypefun

@var{iterator}s implementing the @code{MHD_NameValueIterator}
are simply given the specific @var{kind} and name-and-value pair.

@c FIXME: why don't we use NameValueKind here?
@anchor{MHD_NameValueIterator}
@deftypefn {Function Pointer} {enum MHD_Bool} (*MHD_NameValueIterator) (void *cls, enum MHD_ValueKind kind, const struct MHD_NameAndValue *nv)

Functions of this type are invoked by MHD for each matching
name-value pair.

@table @var
@item cls
custom closure provided by the application to @code{MHD_request_get_values_cb()};

@item kind
the type (kind) of the element; will only have one bit set
(@xref{MHD_ValueKind});

@item nv
the name and the value of the element,
the referenced data is valid only until the
application returns from this function.
@end table
The function should return @code{MHD_YES} to continue to
iterate, and @code{MHD_NO} to abort the iteration.
@end deftypefn


@anchor{MHD_NameAndValue}
@deftp {C Struct} MHD_NameAndValue name value
Represents a pair of a name and a value.
@table @code
@item @code{struct MHD_String} name
Name under which the value is stored, some kinds allow empty strings;

@item @code{struct MHD_StringNullable} value
The value of the field.  Some kinds allow absence of the value;
the absence is indicated by a @code{NULL} pointer to the C string.
@c FIXME: We need to be clearer if value is NULL or cstr is NULL!
@end table
@end deftp

@c FIXME: rename GET -> URL?
If @var{kind} is @code{MHD_VK_GET_ARGUMENT}, the @var{value} argument
given to the @var{iterator} callback will be @code{NULL} if the URL
contained a key without an equals operator.
For example, for a HTTP request to the URL ``http://foo/bar?key'', the
@var{value} argument is @code{NULL}; in contrast, a HTTP request to the URL
``http://foo/bar?key='', the @var{value} argument is the empty string.
The normal case is that the URL contains ``http://foo/bar?key=value''
in which case @var{value} would be the string ``value'' and @var{key}
would contain the string ``key''.


@c FIXME: why int? what would negative values be used for?
@anchor{MHD_request_get_values_list}
@deftypefun int MHD_request_get_values_list (struct MHD_Request *request, enum MHD_ValueKind kind, size_t num_elements, struct MHD_NameValueKind elements[num_elements])
Get all the headers matching @var{kind} from the request.

The pointers to the strings returned in @var{elements} are valid until
any @code{struct MHD_Action} or @code{struct MHD_UploadAction} is
provided for the @var{request}. If the data is needed beyond this
point, it should be copied to an application buffer before returning
an action.

@table @var
@item request
the HTTP request to inspect;

@item kind
kinds of name-value pairs of the request to pass to @var{iterator},
can be a bitmask, ORing the various header kinds that are requested;

@item num_elements
length of the @var{elements} array provided

@item elements
array of length @var{num_elements} to be filled with
the key-value pairs; if @var{request} has more matching elements
than @var{num_elements} than any @var{num_elements} are stored.
@end table

The function returns the number of entries
stored in @var{elements}, the number cannot be larger
than @var{num_elements}, zero if there were no matching
elements or on any error.
@end deftypefun

@anchor{MHD_NameValueKind}
@deftp {C Struct} MHD_NameValueKind nv kind
Represents a triplet of a name, value and kind.
@table @code
@item @code{struct MHD_NameAndValue} nv
The name and value of the field
(@xref{MHD_NameAndValue});

@item @code{struct MHD_ValueKind} kind
The kind of the field
(@xref{MHD_ValueKind}).
@end table
@end deftp

@anchor{MHD_request_get_value}
@deftypefun {const struct MHD_StringNullable *} MHD_request_get_value (struct MHD_Connection *connection, enum MHD_ValueKind kind, const char *key)

Returns a particular data value from a @var{request}.

@table @var
@item request
the HTTP request to inspect;

@item kind
kinds of name-value pairs of the request to pass to @var{iterator},
can be a bitmask, ORing the various header kinds that are requested
(@xref{MHD_ValueKind});

@item key
the name of the value to look for (used for case-insensitive
match), use an empty string to lookup 'trailing' values without a key;
@var{key} must reference a zero-terminated ASCII-coded string
representing the value to look for.
@end table
The function returns @code{NULL} if no matching item was found.
If multiple values match the
@var{kind} and @var{key}, returns the first of them.
@end deftypefun


The following code is a simple example to determine the
value of a ``Host:'' header.

@example
@verbatiminclude examples/host-example.c
@end example



@node libmicrohttpd-request-status-info
@section Obtaining request status information

This section describes the request introspection API.  It follows the
same patterns as the other introspection APIs described in
@ref{libmicrohttpd2-introspection,,Introspecting MHD objects}.  You
may want to refer to the introduction of that chapter for an overview
on MHD introspection.

@anchor{MHD_request_get_info_fixed_sz}
@deftypefun {enum MHD_StatusCode} MHD_request_get_info_fixed_sz (struct MHD_Request *request, enum MHD_RequestInfoFixedType info_type, union MHD_RequestInfoFixedData *output_buf, size_t output_buf_size)

Obtain fixed information about the given request.
This information is not changed for the lifetime of the request.

@table @var
@item request
the request to get information about;

@item info_type
the type of information requested;

@item output_buf
output_buf the pointer to union to be set to the requested
information;

@item output_buf_size
size of @var{output_buf} in bytes
@end table

The function returns:
@table @code
@item MHD_SC_OK
on success,
@c FIXME: can we really have TOO_EARLY/TOO_LATE for *fixed* data?

@item MHD_SC_TOO_EARLY
if the information is not yet available,
usually because the respective stage in processing was not yet reached,

@item MHD_SC_TOO_LATE
if the information is no longer available,
usually because processing is past the stage where the information is kept,

@item MHD_SC_INFO_GET_TYPE_UNKNOWN
if @var{info_type} is unknown,

@item MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE
if the requested information is not available for this specific
object due to how that object was configured,

@item MHD_SC_INFO_GET_TYPE_UNOBTAINABLE
if the requested information should be available
but cannot be provided due to some error or other reason,

@item MHD_SC_INFO_GET_BUFF_TOO_SMALL
if @var{output_buf} is too small for this request;
@end table
specific @var{info_type} values may yield additional
type-specific error codes.
@end deftypefun

Using the above function directly is not recommended.
Instead, applications should use @code{MHD_request_get_info_fixed()}
which is more convenient as it uses a macro to automatically
provide the @var{output_buf_size}.

@anchor{MHD_request_get_info_fixed}
@deftypefun {enum MHD_StatusCode} MHD_request_get_info_fixed (struct MHD_Request *request, enum MHD_RequestInfoFixedType info_type, union MHD_RequestInfoFixedData *output_buf)

Obtain fixed information about the given request.
This information is not changed for the lifetime of the request.

@table @var
@item request
the request to get information about;

@item info_type
the type of information requested;

@item output_buf
output_buf the pointer to union to be set to the requested
information;
@end table

The macro returns the same values as
@code{MHD_request_get_info_fixed_sz()}
@end deftypefun


@deftp {Enumeration} MHD_RequestInfoFixedType
Selects which fixed information about the request is desired.

@table @code
@anchor{MHD_REQUEST_INFO_FIXED_HTTP_VER}
@item MHD_REQUEST_INFO_FIXED_HTTP_VER
Get the version of HTTP protocol used for the request.
If the request line has not been fully received yet then @code{MHD_SC_TOO_EARLY}
is returned.
The result is placed in the @var{v_http_ver} member of @var{output_buf}.

@anchor{MHD_REQUEST_INFO_FIXED_HTTP_METHOD}
@item MHD_REQUEST_INFO_FIXED_HTTP_METHOD
Get the HTTP method used for the
request (as an @ref{MHD_HTTP_Method}).
The result is placed in @var{v_http_method} member.
@xref{MHD_REQUEST_INFO_DYNAMIC_HTTP_METHOD_STR}.

@item MHD_REQUEST_INFO_FIXED_DAEMON
Return the daemon to which the request belongs to.
The result is placed in the @var{v_daemon} member of @var{output_buf}.

@item MHD_REQUEST_INFO_FIXED_CONNECTION
Return which connection is associated with the stream which is associated
with the request. The result is placed in the @var{v_connection} member of @var{output_buf}.

@item MHD_REQUEST_INFO_FIXED_STREAM
Return which stream the request is associated with.
The result is placed in the @var{v_stream} member of @var{output_buf}.

@anchor{MHD_REQUEST_INFO_FIXED_APP_CONTEXT}
@item MHD_REQUEST_INFO_FIXED_APP_CONTEXT
Returns the pointer to a variable pointing to request-specific
application context data. The same data is provided for
@ref{MHD_EarlyUriLogCallback,,@code{MHD_EarlyUriLogCallback}}.
@c FIXME and @ref{MHD_RequestCompletedCallback,,@code{MHD_RequestCompletedCallback}}.
By using provided pointer application may get or set the pointer to
any data specific for the particular request.
The result is placed in the @var{v_ppvoid} member of @var{output_buf}.

@end table
@end deftp

@c FIXME: define MHD_RequestTerminationCallback!

@anchor{MHD_RequestInfoFixedData}
@deftp {C Union} MHD_RequestInfoFixedData

Fixed information about a request.
This information will not change during the lifetime of the request.

@table @var
@item @code{struct MHD_Stream *} v_stream
The stream of the request.

@item @code{struct MHD_Connection *} v_connection
The connection of the request.

@item @code{struct MHD_Daemon *} v_daemon
The daemon handling the request.

@item @code{enum MHD_HTTP_ProtocolVersion} v_http_ver
The HTTP protocol version used for the request.

@item @code{enum MHD_HTTP_Method} v_http_method
The HTTP method of the request.

@item @code{void **} v_ppvoid
The pointer to a pointer where the application can store
contextual data for the request.
@end table
@end deftp


@anchor{MHD_request_get_info_dynamic_sz}
@deftypefun {enum MHD_StatusCode} MHD_request_get_info_dynamic_sz (struct MHD_Request *request, enum MHD_RequestInfoDynamicType info_type, union MHD_RequestInfoDynamicData *output_buf, size_t output_buf_size)

Obtain dynamic information about the given request.
This information may be changed during the lifetime of the request.
Most of the data provided is available only after the request line or even the complete
request headers have been processed. Data may also be no longer available once MHD
has started to return the response.

Any pointers in the returned data are only valid until any
@code{struct MHD_Action} or @code{struct MHD_UploadAction} is
returned to MHD. If the returned data is needed beyond this point,
it should be copied into an application buffer before returning an
action.

@table @var
@item request
the request to get information about;

@item info_type
the type of information requested;

@item output_buf
output_buf the pointer to union to be set to the requested
information;

@item output_buf_size
size of @var{output_buf} in bytes;
@end table

The function returns
@table @code
@item MHD_SC_OK
on success,
@c FIXME: can we really have TOO_EARLY/TOO_LATE for *fixed* data?

@item MHD_SC_TOO_EARLY
if the information is not yet available,
usually because the respective stage in processing was not yet reached,

@item MHD_SC_TOO_LATE
if the information is no longer available,
usually because processing is past the stage where the information is kept,

@item MHD_SC_INFO_GET_TYPE_UNKNOWN
if @var{info_type} is unknown,

@item MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE
if the requested information is not available for this specific
object due to how that object was configured,

@item MHD_SC_INFO_GET_TYPE_UNOBTAINABLE
if the requested information should be available
but cannot be provided due to some error or other reason,

@item MHD_SC_INFO_GET_BUFF_TOO_SMALL
if @var{output_buf} is too small for this request;
@end table
specific @var{info_type} values may yield additional
type-specific error codes.
@end deftypefun

Using the above function directly is not recommended.
Instead, applications should probably use
@code{MHD_request_get_info_dynamic()}
which is more convenient as it uses a macro to automatically
provide the @var{output_buf_size}.

@anchor{MHD_request_get_info_dynamic}
@deftypefun {enum MHD_StatusCode} MHD_request_get_info_dynamic (struct MHD_Request *request, enum MHD_RequestInfoDynamicType info_type, union MHD_RequestInfoDynamicData *output_buf)

Obtain fixed information about the given request.
This information is not changed for the lifetime of the request.

@table @var
@item request
the request to get information about;

@item info_type
the type of information requested;

@item output_buf
output_buf the pointer to union to be set to the requested
information;
@end table

The macro returns the same values as
@code{MHD_request_get_info_dynamic_sz()}
@end deftypefun


@anchor{MHD_RequestInfoDynamicType}
@deftp {Enumeration} MHD_RequestInfoDynamicType

Selects what dynamic information about request is desired.
This information may be changed during the lifetime of the request.
Any returned string pointers are valid only until a response is
provided for the request by the application!

@c FIXME: fix member names in C code, use semantic names, not type-names!
@table @code

@anchor{MHD_REQUEST_INFO_DYNAMIC_HTTP_METHOD_STR}
@item MHD_REQUEST_INFO_DYNAMIC_HTTP_METHOD_STR
Get the HTTP method used for the request (as a MHD_String).
The result is placed in @var{v_str} member.
The resulting string pointer is valid only until a response is provided.
@xref{MHD_REQUEST_INFO_FIXED_HTTP_METHOD}

@item MHD_REQUEST_INFO_DYNAMIC_URI
Get the URI used for the request (as a MHD_String), excluding
the parameter part (anything after '?').
The result is placed in the @var{v_str} member.
The resulting string pointer in valid only until a response is provided.

@item MHD_REQUEST_INFO_DYNAMIC_NUMBER_GET_PARAMS
Get the number of GET parameters (the decoded part of the original
URI string after '?')
The result is placed in the @var{v_sizet} member.

@item MHD_REQUEST_INFO_DYNAMIC_NUMBER_COOKIES
Get the number of cookies in the request.
The result is placed in the @var{v_sizet} member.

@item MHD_REQUEST_INFO_DYNAMIC_HEADER_SIZE
Return length of the client's HTTP request header.
This is a total raw size of the plaintext header. 
The result is placed in the @var{v_sizet} member.

@item MHD_REQUEST_INFO_DYNAMIC_NUMBER_POST_PARAMS
Get the number of decoded POST entries in the request.
The result is placed in the @var{v_sizet} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_PRESENT
Get whether the upload content is present in the request.
The result is @code{MHD_YES} if any upload content is present, even
if the upload content size is zero.
The result is placed in the @var{v_bool} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_CHUNKED
Get whether the chunked upload content is present in the request.
The result is @code{MHD_YES} if chunked upload content is present.
The result is placed in the @var{v_bool} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_SIZE_TOTAL
Get the total request body size.
Results in zero if no body is expected to be uploaded,
@code{MHD_SIZE_UNKNOWN} if the size is not known
(for example, in the case of a chunked upload).
The result is placed in the @var{v_uint64} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_SIZE_RECIEVED
Get the total size of the content upload already received from the client.
This is the total size received, it is possible that some of it has
not yet been seen or fully processed by the application.
The result is placed in the @var{v_uint64} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_SIZE_TO_RECIEVE
Get the total size of the content upload left to be received from
the client.
Results in @code{MHD_SIZE_UNKNOWN} if total size is not known
(for example, in case of a chunked upload).
The result is placed in the @var{v_uint64} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_SIZE_PROCESSED
Get the total size of the content upload already processed (upload callback
called and completed (if any)).
If the value is requested from @code{MHD_UploadCallback}, then result does @emph{not}
include the data currently being processed by the callback.
The result is placed in the @var{v_uint64} member.

@item MHD_REQUEST_INFO_DYNAMIC_UPLOAD_SIZE_TO_PROCESS
Get the total size of the content upload left to be processed.
The resulting value includes the size of the data not yet received from
the client.
If the value is requested from @code{MHD_UploadCallback}, then result includes
the current data being processed by the callback.
Resulted in @code{MHD_SIZE_UNKNOWN} if total size is not
known (for example, in the case of a chunked upload).
The result is placed in the @var{v_uint64} member.

@c FIXME: should this not be removed?
@item MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_USERNAME
Returns pointer to information about username in client's digest auth
request.
The resulting pointer is @code{NULL} if no digest auth header is set by
the client, the format of the digest auth header is broken, no
username is provided or the format of the username parameter is broken.
Pointers in the returned structure (if any) are valid until response
is provided for the request.
The result is placed in the @var{v_auth_digest_uname} member.

@anchor{MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_INFO}
@item MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_INFO
Returns pointer to information about digest auth in client request.
The resulting pointer is @code{NULL} if no digest auth header is set by
the client or the format of the digest auth header is broken.
Pointers in the returned structure (if any) are valid until response
is provided for the request.
The result is placed in the @var{v_auth_digest_info} member.

@anchor{MHD_REQUEST_INFO_DYNAMIC_AUTH_BASIC_CREDS}
@item MHD_REQUEST_INFO_DYNAMIC_AUTH_BASIC_CREDS
Returns information about Basic Authentication credentials in the request.
Pointers in the returned structure (if any) are valid until
any @code{struct MHD_Action} or @code{struct MHD_UploadAction}
is provided. If the data is needed beyond this point,
it must be copied to an application buffer before providing the action.
If @code{MHD_request_get_info_dynamic_sz()} returns @code{MHD_SC_OK} then
@var{v_auth_basic_creds} is not @code{NULL} and at least the username 
is provided.
The result is placed in the @var{v_auth_basic_creds} member.
@end table
@end deftp


@c FIXME: again, we should use semantic names, not type-based names!
@anchor{MHD_RequestInfoDynamicData}
@deftp {C Union} MHD_RequestInfoDynamicData
Stores dynamic information about a request.

@table @var
@item @code{struct MHD_String} v_str
Used when the requested information is a string.

@item @code{size_t} v_sizet
Used when the requested information is a size.

@item @code{enum MHD_Bool} v_bool
Used when the requested information is a boolean.

@item @code{uint_fast64_t} v_uint64
Used when the requested information is an unsigned 64-bit integer.

@item @code{const struct MHD_AuthDigestUsernameInfo *} v_auth_digest_uname
Information about client-provided username for digest authentication.

@item @code{const struct MHD_AuthDigestInfo *} v_auth_digest_info
Information about client's digest authentication.

@item @code{const struct MHD_AuthBasicCreds *} v_auth_basic_creds
The username and password provided by the client's basic authentication header.
If @code{MHD_request_get_info_dynamic_sz()} returns
@code{MHD_SC_OK} then this pointer is not @code{NULL} and
at least the @var{username} is provided.
@end table
@end deftp
