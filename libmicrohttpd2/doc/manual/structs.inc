

@deftp {C Struct} MHD_Connection
Connection to an HTTP client. Since HTTP2, multiple
sessions can be multiplexed over the same connection.
@end deftp

@deftp {C Struct} MHD_Session
Since HTTP2, multiple sessions can run over the
same connection.  In HTTP/1.x, there is a 1:1
relationship between sessions and connections.
Since HTTP/1.1, multiple requests can be run
over the same session.
@end deftp

@deftp {C Struct} MHD_Request
Handle for an HTTP request.
MHD will only show one request per session
to the application at any given time.
@end deftp

@deftp {C Struct} MHD_Action
Next action to perform when handling a request.
@end deftp

@deftp {C Struct} MHD_UploadAction
Action that processes data uploaded by a client.
@end deftp

@deftp {C Struct} MHD_DynamicContentCreatorAction
Action that is used to dynamically create content.
@end deftp

@deftp {C Struct} MHD_Response
Handle for a response.
@end deftp

@deftp {C Struct} MHD_UpgradedHandle
Handle for response that has been "upgraded"
(say to a WebSocket).
@end deftp

@deftp {C Struct} MHD_PostProcessor
@cindex POST method
Handle for @code{POST} processing.
@end deftp

@deftp {C Struct} MHD_AuthDigestInfo
@cindex Authentication
Represents information about a digest authorization
client header. Includes the cryptographic algorithm,
information about the user name, nonce and other
optional data from the digest authentication request.
@end deftp

@deftp {C Struct} MHD_AuthDigestUsernameInfo
@cindex Authentication
Represents information about a digest authorization
client header. Includes the cryptographic algorithm,
information about the user name.
@end deftp

@deftp {C Struct} MHD_AuthBasicCreds
@cindex Authentication
Represents information from a basic authorization
client header. Includes the username and password.
@end deftp


@deftp {C Struct} MHD_NameAndValueCStr
Represents key-value pair, consisting of a
to vanilla 0-terminated C-strings without
explicit length field.
@end deftp

@deftp {C Struct} MHD_NameAndValue
Represents key-value pair, consisting of a
(non-NULL) string key and a (nullable) string
value.  Used commonly to represent HTTP headers.
@end deftp

@deftp {C Struct} MHD_NameAndValueKind
Adds a "kind" field to the @code{MHD_NameAndValue}
to specify the origin of the key-value pair.
@end deftp

@deftp {C Struct} MHD_PostField
Information about data uploaded as part of an
HTTP "POST" action. Includes the name of the
field, value of the field (possibly NULL), the
filename (possibly NULL), the Mime content-type
(possibly NULL) and the transfer encoding
(possibly NULL).
@end deftp

@deftp {C Struct} MHD_IoVec
An element of an array of memory buffers.
@end deftp

@deftp {C Union} MHD_DynContentZCIoVec
Data structure with an array of
@code{MHD_IoVec} IO vectors and a callback
to free associated resources when the
IO vectors are no longer needed.
@end deftp

@deftp {C Union} MHD_DaemonInfoFixedData
Information about an MHD daemon that will not change.
@end deftp

@deftp {C Union} MHD_DaemonInfoDynamicData
Information about an MHD daemon that may change.
@end deftp

@deftp {C Union} MHD_ConnectionInfoFixedData
Information about an MHD connection that will not change.
@end deftp

@deftp {C Union} MHD_ConnectionInfoDynamicData
Information about an MHD connection that may change.
@end deftp

@deftp {C Union} MHD_StreamInfoFixedData
Information about an MHD stream that will not change.
@end deftp

@deftp {C Union} MHD_StreamInfoDynamicData
Information about an MHD stream that may change.
@end deftp

@deftp {C Union} MHD_RequestInfoFixedData
Information about an MHD request that will not change.
@end deftp

@deftp {C Union} MHD_RequestInfoDynamicData
Information about an MHD request that may change.
@end deftp
