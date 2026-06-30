
@deftypefn {Function Pointer} enum MHD_Result {*MHD_AcceptPolicyCallback} (void *cls, const struct sockaddr * addr, socklen_t addrlen)
Invoked in the context of a connection to allow or deny a client to
connect.  This callback return @code{MHD_YES} if connection is allowed,
@code{MHD_NO} if not.

@table @var
@item cls
custom value selected at callback registration time;
@item addr
address information from the client;
@item addrlen
length of the address information.
@end table
@end deftypefn


@anchor{MHD_RequestCompletedCallback}
@deftypefn {Function Pointer} void {*MHD_RequestCompletedCallback} (void *cls, struct MHD_Connection *connection, void **req_cls, enum MHD_RequestTerminationCode toe)
Signature of the callback used by MHD to notify the application about
completed requests.

@table @var
@item cls
custom value selected at callback registration time;

@item connection
connection handle;

@item req_cls
value as set by the last call to the
@code{MHD_AccessHandlerCallback};

@item toe
reason for request termination see @code{MHD_OPTION_NOTIFY_COMPLETED}.
@end table
@end deftypefn


@deftypefn {Function Pointer} enum MHD_Result {*MHD_KeyValueIterator} (void *cls, enum MHD_ValueKind kind, const char *key, const char *value, size_t value_size)
Iterator over key-value pairs.  This iterator can be used to iterate
over all of the cookies, headers, or @code{POST}-data fields of a
request, and also to iterate over the headers that have been added to a
response.

@table @var
@item cls
custom value specified when iteration was triggered;

@item kind
kind of the header we are looking at

@item key
key for the value, can be an empty string

@item value
value corresponding value, can be NULL

@item value_size
number of bytes in @code{value}. This argument was introduced in
@code{MHD_VERSION} 0x00096301 to allow applications to use binary
zeros in values.  Applications using this argument must ensure that
they are using a sufficiently recent version of MHD, i.e. by testing
@code{MHD_get_version()} for values above or equal to 0.9.64.
Applications that do not need zeros in values and that want to compile
without warnings against newer versions of MHD should not declare this
argument and cast the function pointer argument to
@code{MHD_KeyValueIterator}.

@end table

Return @code{MHD_YES} to continue iterating, @code{MHD_NO} to abort the
iteration.
@end deftypefn


@deftypefn {Function Pointer} ssize_t {*MHD_ContentReaderCallback} (void *cls, uint64_t pos, char *buf, size_t max)
Callback used by MHD in order to obtain content.  The callback has to
copy at most @var{max} bytes of content into @var{buf}.  The total
number of bytes that has been placed into @var{buf} should be returned.

Note that returning zero will cause MHD to try again.
Thus, returning zero should only be used in conjunction
with @code{MHD_suspend_connection()} to avoid busy waiting.

While usually the callback simply returns the number of bytes written
into @var{buf}, there are two special return value:

@code{MHD_CONTENT_READER_END_OF_STREAM} (-1) should be returned
for the regular end of transmission (with chunked encoding, MHD will then
terminate the chunk and send any HTTP footers that might be
present; without chunked encoding and given an unknown
response size, MHD will simply close the connection; note
that while returning @code{MHD_CONTENT_READER_END_OF_STREAM} is not technically
legal if a response size was specified, MHD accepts this
and treats it just as @code{MHD_CONTENT_READER_END_WITH_ERROR}.

@code{MHD_CONTENT_READER_END_WITH_ERROR} (-2) is used to indicate a server
error generating the response; this will cause MHD to simply
close the connection immediately.  If a response size was
given or if chunked encoding is in use, this will indicate
an error to the client.  Note, however, that if the client
does not know a response size and chunked encoding is not in
use, then clients will not be able to tell the difference between
@code{MHD_CONTENT_READER_END_WITH_ERROR} and
@code{MHD_CONTENT_READER_END_OF_STREAM}.
This is not a limitation of MHD but rather of the HTTP protocol.

@table @var
@item cls
custom value selected at callback registration time;

@item pos
position in the datastream to access; note that if an
@code{MHD_Response} object is re-used, it is possible for the same
content reader to be queried multiple times for the same data; however,
if an @code{MHD_Response} is not re-used, MHD guarantees that
@var{pos} will be the sum of all non-negative return values obtained
from the content reader so far.
@end table

Return @code{-1} on error (MHD will no longer try to read content and
instead close the connection with the client).
@end deftypefn


@deftypefn {Function Pointer} void {*MHD_ContentReaderFreeCallback} (void *cls)
This method is called by MHD if we are done with a content reader.
It should be used to free resources associated with the content reader.
@end deftypefn


@deftypefn {Function Pointer} enum MHD_Result {*MHD_PostDataIterator} (void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size)
Iterator over key-value pairs where the value maybe made available in
increments and/or may not be zero-terminated.  Used for processing
@code{POST} data.

@table @var
@item cls
custom value selected at callback registration time;

@item kind
type of the value;

@item key
zero-terminated key for the value;

@item filename
name of the uploaded file, @code{NULL} if not known;

@item content_type
mime-type of the data, @code{NULL} if not known;

@item transfer_encoding
encoding of the data, @code{NULL} if not known;

@item data
pointer to size bytes of data at the specified offset;

@item off
offset of data in the overall value;

@item size
number of bytes in data available.
@end table

Return @code{MHD_YES} to continue iterating, @code{MHD_NO} to abort the
iteration.
@end deftypefn


@deftypefn {Function Pointer} void* {*MHD_WebSocketMallocCallback} (size_t buf_len)
@cindex websocket
This callback function is used internally by many websocket functions
for allocating data.
By default @code{malloc} is used.
You can use your own allocation function with @code{MHD_websocket_stream_init2}
if you wish to.
This can be useful for operating systems like Windows
where @code{malloc}, @code{realloc} and @code{free} are compiler-dependent.
You can call the associated @code{malloc} callback of
a websocket stream with @code{MHD_websocket_malloc}.

@table @var
@item buf_len
size of the buffer to allocate in bytes.
@end table

Return the pointer of the allocated buffer or @code{NULL} on failure.
@end deftypefn


@deftypefn {Function Pointer} void* {*MHD_WebSocketReallocCallback} (void *buf, size_t new_buf_len)
@cindex websocket
This callback function is used internally by many websocket
functions for reallocating data.
By default @code{realloc} is used.
You can use your own reallocation function with
@code{MHD_websocket_stream_init2} if you wish to.
This can be useful for operating systems like Windows
where @code{malloc}, @code{realloc} and @code{free} are compiler-dependent.
You can call the associated @code{realloc} callback of
a websocket stream with @code{MHD_websocket_realloc}.

@table @var
@item buf
current buffer, may be @code{NULL};

@item new_buf_len
new size of the buffer in bytes.
@end table

Return the pointer of the reallocated buffer or @code{NULL} on failure.
On failure the old pointer must remain valid.
@end deftypefn


@deftypefn {Function Pointer} void {*MHD_WebSocketFreeCallback} (void *buf)
@cindex websocket
This callback function is used internally by many websocket
functions for freeing data.
By default @code{free} is used.
You can use your own free function with
@code{MHD_websocket_stream_init2} if you wish to.
This can be useful for operating systems like Windows
where @code{malloc}, @code{realloc} and @code{free} are compiler-dependent.
You can call the associated @code{free} callback of
a websocket stream with @code{MHD_websocket_free}.

@table @var
@item cls
current buffer to free, this may be @code{NULL} then nothing happens.
@end table
@end deftypefn


@deftypefn {Function Pointer} size_t {*MHD_WebSocketRandomNumberGenerator} (void *cls, void* buf, size_t buf_len)
@cindex websocket
This callback function is used for generating random numbers
for masking payload data in client mode.
If you use websockets in server mode with @emph{libmicrohttpd} then
you don't need a random number generator, because
the server doesn't mask its outgoing messages.
However if you wish to use a websocket stream in client mode,
you must pass this callback function to @code{MHD_websocket_stream_init2}.

@table @var
@item cls
closure specified in @code{MHD_websocket_stream_init2};
@item buf
buffer to fill with random values;
@item buf_len
size of buffer in bytes.
@end table

Return the number of generated random bytes.
The return value should usually equal to buf_len.
@end deftypefn
