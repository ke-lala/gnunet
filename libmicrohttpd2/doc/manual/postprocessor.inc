@cindex POST
MHD provides the post processor API to make it easier for applications to
parse the data of a client's @code{POST} request that is encoded
in one of the typical formats for HTML form data.

POST data can be processed in two main ways with MHD:
@itemize
@item incrementally, as it is received over the network
@item from memory with random access
@end itemize

Incremental processing is necessary if the POSTed data does
not fit into main memory, while processing from memory with
random access is often more convenient and generally preferrable
for developers if the data fits into memory. MHD's API allows
the application to do both, even for the same request: the
application can specify how large of an in-memory buffer to
use with @var{buffer_size}, and then to provide a threshold
@var{max_nonstream_size} above which records will be processed
incementally.

@node libmicrohttpd-postprocessor-start
@section Starting POST processing

POST handling using this style is initiated by returning an action
created via @code{MHD_action_parse_post()} from
@ref{MHD_RequestCallback,,@code{MHD_RequestCallback}}.

@anchor{MHD_action_parse_post}
@deftypefun {const struct MHD_Action *} MHD_action_parse_post (struct MHD_Request *request, size_t buffer_size, size_t max_nonstream_size, enum MHD_HTTP_PostEncoding enc, MHD_PostDataReader stream_reader, void *reader_cls, MHD_PostDataFinished done_cb, void *done_cb_cls)

Creates an action to parse the POSTed content from the client.
The action starts parsing of the POST data. Any record that is larger
than @var{buffer_size} or @var{max_nonstream_size} is given to
the @var{stream_reader} (unless @var{stream_reader} is @code{NULL}).

If @var{buffer_size} is zero, then buffers will be limited to the
connection's memory pool. To force all POST records to be processed
incrementally via @var{stream_reader} set @var{max_nonstream_size} to
zero.

@table @var
@item request
the request to create a POST processing action for

@item buffer_size
the maximum size allowed for the buffers to parse this
request POST data. Within the set limit the buffer is
allocated automatically from the @ref{shared memory pool}
if necessary.

@item max_nonstream_size
the size of the field (in encoded form) above which
values are not buffered and provided for
the @var{steam_reader} automatically;
useful to have large data (like file uploads)
processed incrementally, while keeping buffer space
for small fields only;
ignored if @var{stream_reader} is @code{NULL}

@item enc
the data encoding to use,
@code{MHD_HTTP_POST_ENCODING_OTHER} to tell MHD
to detect the encoding automatically (based on HTTP headers);

@item stream_reader
a function to call for ``oversize'' records in
the stream; can be @code{NULL} if @var{max_nonstream_size}
is not zero;

@item reader_cls
the closure for the @var{stream_reader};

@item done_cb called once all data has been processed, returning
control back to the application to determine the next action; values
smaller than @var{max_nonstream_size} that fit into @var{buffer_size}
will be available during @var{done_cb} via
@ref{MHD_request_get_values_cb,,@code{MHD_request_get_values_cb()}}
and
@ref{MHD_request_get_values_list,,@code{MHD_request_get_values_list()}},
@ref{MHD_request_get_post_data_cb,,@code{MHD_request_get_post_data_cb()}}
and
@ref{MHD_request_get_post_data_list,,@code{MHD_request_get_post_data_list()}}.

@item done_cb_cls
the closure for the @var{done_cb}

@end table

Returns a pointer to the action, @code{NULL} if creating the action
failed (insufficient memory) or if any action has been already created
for the @var{request}.

@end deftypefun

@node libmicrohttpd-postprocessor-incremental
@section Incremental record processing


After returning the action created via @code{MHD_action_parse_post}
MHD will first call the given @code{MHD_PostDataReader} for incremental
processing of all records that exceed the length threshold.

@anchor{MHD_PostDataReader}
@deftypefn {Function Pointer} {enum MHD_Bool} (*MHD_PostDataReader) (void *cls, struct MHD_Request *req, const struct MHD_String *name, const struct MHD_StringNullable *filename, const struct MHD_StringNullable *content_type, const struct MHD_StringNullable *encoding, size_t size, const void *data, uint_fast64_t off, enum MHD_Bool final_data)

Stream reader for incremental processing of POST records.  This
callback is called to incrementally process parsed POST records sent
by the client.  The pointers to the @code{struct MHD_String} and
@code{struct MHD_StringNullable} are valid @emph{only} until the
application returns from this callback.

@table @var
@item cls
custom value provided by the application with the callback;

@item request
the request to get data for;

@item name
the name of the POST record;

@item filename
the name of the uploaded file, @code{NULL} if not provided;

@item content_type
the mime-type of the data, @code{NULL} if not provided;

@item encoding
the encoding of the data, @code{NULL} if not provided;

@item size
the number of bytes in @var{data}, may be zero if @var{final_data}
is @code{MHD_YES};

@item data
a pointer to @var{size} bytes of data at the specified
offset @var{off}, @strong{not} zero-terminated;

@item off
the offset of @var{data} in the overall value, always equal to
the sum of all @var{size} values of previous calls for the same
record; however, note that a client may provide more than one
record with the same @var{name} and the same @var{filename}! Thus
the next record (or file) may be indicated by a zero
value in @var{off} (and the end is indicated by @var{final_data});

@item final_data
set to @code{MHD_YES} when this is the last call for the given
record, set to @code{MHD_NO} if additional callbacks for the same
record will be made;

@end table

The incremental callback must specify how to continue processing
the upload. The choices are:

@itemize
@item @code{MHD_upload_action_continue()} if all is well,
@item @code{MHD_upload_action_suspend()} to stop reading from the client
  until the request is explicitly resumed by the application,
@item @code{MHD_upload_action_abort_request()} to close the socket, or
@item an action with a response to @emph{discard} the rest of the
  upload and transmit the response.
@end itemize

@end deftypefn

@node libmicrohttpd-postprocessor-final
@section Final in-memory processing of parsing status and records

After all large records have been provided to the
@code{MHD_PostDataReader} and MHD has concluded parsing the entire
body sent by the client, MHD will call the @var{done_cb} to give the
application a chance to inspect the records that were small enough to
fit fully into memory and to decide the next action.

@anchor{MHD_PostDataFinished}
@deftypefn {Function Pointer} {struct MHD_UploadAction} (*MHD_PostDataFinished) (void *cls, struct MHD_Request *req, const struct MHD_PostParseResult *parsing_result)

The callback to be called when MHD is finished with parsing
all of the body. The @var{stream_reader} will not be called
after this call. Implementations of this function can
use
@ref{MHD_request_get_values_cb,,@code{MHD_request_get_values_cb()}}
and
@ref{MHD_request_get_values_list,,@code{MHD_request_get_values_list()}},
@ref{MHD_request_get_post_data_cb,,@code{MHD_request_get_post_data_cb()}}
and
@ref{MHD_request_get_post_data_list,,@code{MHD_request_get_post_data_list()}}
to inspect the non-incremental POST data and must ultimately
return an action that determines how to continue handling the
request.

@table @var
@item cls
custom value provided by the application with the callback;

@item request
the request to get data for;

@item parsing_result
status of parsing the request body.
@end table
@end deftypefn

@deftp {Enumeration} MHD_PostParseResult

Represents possible outcomes of parsing a client's body.

@table @code
@item MHD_POST_PARSE_RES_OK
The POST data parsed successfully and completely.

@item MHD_POST_PARSE_RES_REQUEST_EMPTY
The request had no content or zero-length content.

@item MHD_POST_PARSE_RES_OK_BAD_TERMINATION
The POST data parsed successfully, but has missing or incorrect
termination. The last record parsed may thus have incorrect
or incomplete data.

@c FIXME: did I get the distinction right? It is a bit fuzzy. Do we need to distinguish this from the previous case?
@item MHD_POST_PARSE_RES_PARTIAL_INVALID_POST_FORMAT
Parsing of the POST data is definitively incomplete because the client
sent an incorrectly formatted body. The last record parsed has
incorrect or incomplete data.
Some POST data is available or has been provided via callback.

@item MHD_POST_PARSE_RES_FAILED_NO_POOL_MEM
The POST data cannot be parsed completely because the stream has
not enough free memory in the pool. Some POST data may have been parsed.

@item MHD_POST_PARSE_RES_FAILED_NO_LARGE_BUF_MEM
The POST data could not be parsed completely because not
enough large shared buffer space was available.
Some POST data may have been parsed.

@item MHD_POST_PARSE_RES_FAILED_UNKNOWN_CNTN_TYPE
The POST data could not be parsed because the
``Content-Type:'' specified is not understood by MHD.

@item MHD_POST_PARSE_RES_FAILED_NO_CNTN_TYPE
The POST data cannot be parsed because the
``Content-Type:'' header was not set by the clients.
Applications can sometimes work around this by explicitly
setting @var{enc} when calling @code{MHD_action_parse_post()}
if they see that the ``Content-Type'' was omitted and
they know what it is.

@item MHD_POST_PARSE_RES_FAILED_HEADER_NO_BOUNDARY
The POST data cannot be parsed because ``Content-Type:''
request header lacks the required ``boundary'' parameter for
``multipart/form-data''.

@item MHD_POST_PARSE_RES_FAILED_HEADER_MISFORMED
The POST data cannot be parsed because
``Content-Type: multipart/form-data''
request header is malformed.

@item MHD_POST_PARSE_RES_FAILED_HEADER_NOT_MPART
The application set POST encoding to ``multipart/form-data''
via @var{enc}, but the request has no ``Content-Type: multipart/form-data''
header which is required to find the ``boundary'' used in this encoding.

@item MHD_POST_PARSE_RES_FAILED_INVALID_POST_FORMAT
The POST data cannot be parsed because the client's upload
did not abide by the format specified for the POST encoding.

@end table
@end deftp

Once the @code{MHD_action_parse_post()} has called the @var{done_cb},
the application can use the following functions to access the records
that MHD could fit into memory.

@anchor{MHD_request_get_post_data_cb}
@deftypefun size_t MHD_request_get_post_data_cb (struct MHD_Request *request, struct MHD_PostDataIterator *iterator, void *iterator_cls)

Call @var{iterator} on all of the post data from the @var{request}.

@table @var
@item request
the request to get data for

@item iterator
callback to call on each header;
can be @code{NULL} (then this function will simply just count the records);

@item iterator_cls
extra argument to pass to @var{iterator}
@end table

Returns the number of entries iterated over, zero if no records were
available or a postprocessor was not used with @var{request}.
@end deftypefun

@c FIXME: "Field" is probably a bad name, I think we should change it to "Record" everywhere in the API
@anchor{MHD_PostDataIterator}
@deftypefn {Function Pointer} {enum MHD_Bool} (*MHD_PostDataIterator) (void *cls, const struct MHD_PostField *data)

Iterator over POST data.  The @var{data} pointer is valid only until
the application returns from this function.  However, the pointers to
the strings in @var{data} remain valid until any @code{struct
MHD_UploadAction} is provided. If any data is needed beyond this
point, it should be copied into an application-managed buffer.

@table @var
@item cls
custom value provided by the application with the callback;

@item data
one data record from the body
@end table

The callback must return @code{MHD_YES} to continue iterating
or @code{MHD_NO} to abort the iteration.
@end deftypefn


@deftp {C Struct} MHD_PostField name value filename content_type transfer_encoding

Post data record. If any member is not provided/set then pointer to C string is @code{NULL}. If any member is set to empty string then pointer to C string not @code{NULL}, but the @var{len} is zero.

@table @var
@item @code{struct MHD_String} name
The name of the record;

@item @code{struct MHD_StringNullable} value
The record's main value;

@item @code{struct MHD_StringNullable} filename
The filename if provided (only for ``multipart/form-data'');

@item @code{struct MHD_StringNullable} content_type
The Content-Type if provided (only for "multipart/form-data");

@item @code{struct MHD_StringNullable} transfer_encoding
The Transfer-Encoding if provided (only for ``multipart/form-data'').

@end table
@end deftp

Alternatively, the application can use
@code{MHD_request_get_post_data_cb} to just count the number of
records and obtain all records without iterating by providing
space for the @var{elements} itself and using
@code{MHD_request_get_post_data_list()}.


@anchor{MHD_request_get_post_data_list}
@deftypefun size_t MHD_request_get_post_data_list (struct MHD_Request *request, size_t num_elements, struct MHD_PostField elements[num_elements])

Stores all of the post records from the @var{request} in @var{elements}.

@table @var
@item request
the request to get data for

@item num_elements
the number of elements in the @var{elements} array

@item elements
an array of @var{num_elements} where to store the POST records
@end table

Returns the number of elements stored in @var{elements},
zero if no data was stored or a postprocessor was not
used with @var{request}.
@end deftypefun
