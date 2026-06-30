@cindex authentication
@noindent
MHD supports three types of client authentication:

@itemize

@item Basic authentication uses a simple authentication method that
transmits username and password in simple base64 encoding. Username
and password are exchanged in cleartext between the client and the
server, so this method must only be used for non-sensitive content or
when the session is protected with TLS.  When using basic
authentication MHD will have access to the clear password, possibly
allowing the application to create a chained authentication toward an
external authentication server.

@item Digest authentication uses a one-way authentication method based on
cryptographic hash algorithms. Only the hash is transmitted over the
network, hence protecting the user's password. A nonce is used to
prevent replay attacks. This method is appropriate for general use,
especially when TLS is not used to encrypt the transmission.

@item Client certificate authentication uses a X.509 certificate from the
client. This is the strongest authentication mechanism and also
requires the use of TLS. Client certificate authentication can be used
simultaneously with Basic or Digest Authentication in order to provide
a two levels authentication (like, for instance, separate machine and
user authentication).

@end itemize

@menu
* libmicrohttpd-bauth basic::      Using Basic Authentication.
* libmicrohttpd-bauth example::    Example for Basic Authentication.
* libmicrohttpd-dauth digest::     Using Digest Authentication.
@c * libmicrohttpd-dauth example::    Example for Digest Authentication.
@c * libmicrohttpd-dauth token::     Using Token Authentication.
@c FIXME * libmicrohttpd-dauth tls::     Using TLS Client Authentication.
@end menu


@node libmicrohttpd-bauth basic
@section Using Basic Authentication
@cindex basic authentication

@ref{MHD_LIB_INFO_FIXED_HAS_AUTH_BASIC,,@code{MHD_LIB_INFO_FIXED_HAS_AUTH_BASIC}}
can be used to detect if the MHD build includes support for HTTP basic
authorization.

To use HTTP basic authentication, the HTTP server must first return a
response with status @code{MHD_HTTP_STATUS_UNAUTHORIZED} asking the
client to authenticate.  This is usually done @emph{after} checking
whether the client did already authenticate, which we will look at
later in this section.  To challenge a client to authenticate using
HTTP basic authentication, MHD offers the
@code{MHD_response_add_auth_basic_challenge()} function which
@emph{modifies} an existing response, adding HTTP headers asking the
client to authenticate.

@deftypefun {enum MHD_StatusCode} MHD_response_add_auth_basic_challenge (struct MHD_Response *response, const char *realm, enum MHD_Bool prefer_utf8)

Adds a basic authentication "challenge" to the response.  The
@var{response} must use @code{MHD_HTTP_STATUS_UNAUTHORIZED} for its
HTTP status code.

If access to any resource should be limited to specific users,
authenticated by basic authentication mechanism, and the request for
this resource does not have basic authentication information
(see @ref{MHD_AuthBasicCreds,,@code{MHD_AuthBasicCreds}}), then a
response with a basic authentication "challenge" should be sent by
extending an error response using this function. This works as an
indication that basic authentication should be used for the access.

See RFC 7617, section 2 for details.

@table @var
@item response
the response to update; should contain the "access denied" body; note: this
  function sets the "WWW Authenticate" header and thus the caller
  should not set this header; the response must have
  @code{MHD_HTTP_STATUS_UNAUTHORIZED} HTTP status code; passing
  @code{NULL} is tolerated, in which case the result will be
  @code{MHD_SC_RESP_POINTER_NULL}

@item realm
the realm presented to the client

@item prefer_utf8
if not set to @code{MHD_NO} the parameter 'charset="UTF-8"'
will be added, indicating for client that UTF-8 encoding is preferred

@end table

The function returns
@itemize
@item @code{MHD_SC_OK} if it succeeded
@item @code{MHD_SC_TOO_LATE} if the response has been already "frozen" (used to create an action),
@item @code{MHD_SC_RESP_HEADERS_CONFLICT} if a Basic Authentication "challenge" was already added,
@item @code{MHD_SC_RESP_POINTER_NULL} if @var{response} was set to @code{NULL},
@item @code{MHD_SC_RESP_HTTP_CODE_NOT_SUITABLE} is response status code is wrong,
@item @code{MHD_SC_RESP_HEADER_VALUE_INVALID} if realm is zero-length or has CR or LF characters,
@item @code{MHD_SC_RESPONSE_HEADER_MEM_ALLOC_FAILED} if memory allocation failed,
@item or possibly other error codes (that could be defined in the future) on failure.
@end itemize

@end deftypefun

@c FIXME: current header ONLY exports this API (and the _p/_a variants) if static inline is supported; that's not nice, API changes based on compiler.
@c FIXME: also not convinced this API is even all that useful. Maybe remove?
MHD provides a convenience API that adds the basic authentication challenge
to response and turns it into the action to be returned
from the @ref{MHD_RequestCallback,,@code{MHD_RequestCallback}}.

@deftypefun {const struct MHD_Action *} MHD_action_basic_auth_challenge (struct MHD_Request *request, const char *realm, enum MHD_Bool prefer_utf8, struct MHD_Response *response, enum MHD_Bool abort_if_failed)

Creates an action to reply with a basic authentication "challenge".
The response must use @code{MHD_HTTP_STATUS_UNAUTHORIZED} for its HTTP
status code.

If access to any resource should be limited to specific users,
authenticated by basic authentication mechanism, and the request for
this resource does not have basic authentication information
(see @ref{MHD_AuthBasicCreds,,@code{MHD_AuthBasicCreds}}), then a response with a basic
authentication "challenge" should be sent by extending an error
response using this function. This works as an indication that basic
authentication should be used for the access.

See RFC 7617, section-2 for details.

@table @var
@item request
the client request to generate the action for

@item realm
the realm presented to the client

@item response
the reply to send; should contain the "access denied" body; note: this
  function sets the "WWW Authenticate" header and thus the caller
  should not set this header; the response must have
  @code{MHD_HTTP_STATUS_UNAUTHORIZED} HTTP status code; passing
  @code{NULL} is tolerated, in which case the result will be
  @code{MHD_SC_RESP_POINTER_NULL}

@item prefer_utf8
if not set to @code{MHD_NO} the parameter 'charset="UTF-8"'
will be added, indicating for client that UTF-8 encoding is preferred

@item abort_if_failed
if set to @code{MHD_NO} the response will be returned even if
MHD failed to add Basic Authentication "challenge",
if set to @code{MHD_YES} the HTTP request will be aborted
if the "challenge" could not be added.
@end table
@end deftypefun

If the client has supplied an HTTP basic authorization header, the
application can obtain the credentials using request introspection.
@ref{MHD_REQUEST_INFO_DYNAMIC_AUTH_BASIC_CREDS,,@code{MHD_REQUEST_INFO_DYNAMIC_AUTH_BASIC_CREDS}}
can be used to obtain the credentials as a @code{struct
MHD_AuthBasicCreds}.

@anchor{MHD_AuthBasicCreds}
@deftp {C Struct} MHD_AuthBasicCreds username password

Information decoded from a client's header with basic authentication data.
@table @var
@item @code{struct MHD_String} username
The username;

@item @code{struct MHD_StringNullable} password
The password, string pointer may be @code{NULL} if
the password is not properly encoded in the
transmission from the client.
@end table
@end deftp

@node libmicrohttpd-bauth example
@section Example for Basic Authentication
@cindex basic authentication


@example
@verbatiminclude examples/basic-authentication.c
@end example



@node libmicrohttpd-dauth digest
@section Using Digest Authentication
@cindex digest authentication

@ref{MHD_LIB_INFO_FIXED_HAS_AUTH_DIGEST,,@code{MHD_LIB_INFO_FIXED_HAS_AUTH_DIGEST}}
can be used to detect if the MHD build includes support for HTTP digest
authorization.


To use HTTP digest authentication, the HTTP server must first return a
response with status @code{MHD_HTTP_STATUS_UNAUTHORIZED} asking the
client to authenticate.  This is usually done @emph{after} checking
whether the client did already authenticate, which we will look at
later in this section.  To challenge a client to authenticate using
HTTP digest authentication, MHD offers the
@code{MHD_response_add_auth_digest_challenge()} function which
@emph{modifies} an existing response, adding HTTP headers asking the
client to authenticate.
MHD supports various features of HTTP digest authentication, including
various hash algorithms and levels of quality of protection (qop) which
must be specified by the application when creating the challenge.

@anchor{MHD_response_add_auth_digest_challenge}
@deftypefun {enum MHD_StatusCode} MHD_response_add_auth_digest_challenge (struct MHD_Response *response, const char *realm, const char *opaque, const char *domain, enum MHD_Bool indicate_stale, enum MHD_DigestAuthMultiQOP mqop, enum MHD_DigestAuthMultiAlgo malgo, enum MHD_Bool userhash_support, enum MHD_Bool prefer_utf8)

Adds a digest authentication "challenge" to the response.
The @var{response} must use @code{MHD_HTTP_STATUS_UNAUTHORIZED} for its HTTP status code.

@cindex qop
If @var{mqop} allows both RFC 2069 (@code{MHD_DIGEST_AUTH_QOP_NONE})
and other QOP values, then the "challenge" is formed like if
@code{MHD_DIGEST_AUTH_QOP_NONE} bit was not set, because such
a "challenge" should be backwards-compatible with RFC 2069.

If @var{mqop} allows only @code{MHD_DIGEST_AUTH_MULT_QOP_NONE},
then the response is formed in strict accordance with RFC 2069
(no 'qop', no 'userhash', no 'charset').
For better compatibility with clients, it is recommended (but
not required) to set @var{domain} to @code{NULL} in this mode.

New nonces are generated each time when the resulting response is
used.

See RFC 7616, section 3.3 for details.

@table @var

@item response
the response to update; should contain the "access denied"
body; note: this function sets the "WWW Authenticate" header and
the caller should not set this header;
the response must have a @code{MHD_HTTP_STATUS_UNAUTHORIZED} HTTP status
code; a value of @code{NULL} is tolerated (the result will then be
@code{MHD_SC_RESP_POINTER_NULL});

@item realm
the realm presented to the client;

@item opaque
the string for the opaque value, can be @code{NULL}, but @code{NULL} is
not recommended for better compatibility with clients;
the recommended format are string using hex or Base64 encoding;

@item domain
an optional space-separated list of URIs for which the
same authorisation could be used, URIs can be in form
"path-absolute" (the path for the same host with initial slash)
or in form "absolute-URI" (the full path with protocol), in
any case client may assume that URI is in the same "protection
space" if it starts with any of values specified here;
could be @code{NULL} (clients typically assume that the same
credentials could be used for any URI on the same host);
this list provides information for the client only and does
not actually restrict anything on the server side;

@item indicate_stale
if set to @code{MHD_YES} then an indication of a stale nonce
having been used in the client's request is indicated by adding
``stale=true'' to the authentication header; this
instructs the client to retry immediately with the new
nonce and the same credentials, without asking the user
for the new password;

@item mqop
the quality of protection to use (see @ref{MHD_DigestAuthMultiQOP,,@code{MHD_DigestAuthMultiQOP}});

@item algo
digest algorithm to use; if several algorithms are allowed
then one challenge for each allowed algorithm is added to the
response (@xref{MHD_DigestAuthMultiAlgo});

@item userhash_support
if set to @code{MHD_YES} then support of the ``userhash'' variant is
indicated, allowing the client to provide @code{hash("username:realm")}
instead of the username in clear text;
note that clients are @emph{allowed} to provide the username
in cleartext even if this parameter set to @code{MHD_YES};
when userhash is used, the application must be ready to
identify users by provided userhash value instead of
the plaintext username; see @ref{MHD_digest_auth_calc_userhash}
@c and @ref{MHD_digest_auth_calc_userhash_hex}

@item prefer_utf8
if set to @code{MHD_YES}, the parameter ``charset=UTF-8'' is
added, indicating to the client that UTF-8 encoding for
the username is preferred
@end table

The function returns
@itemize
@item @code{MHD_SC_OK} if it succeeded
@item @code{MHD_SC_TOO_LATE} if the response has been already "frozen" (used to create an action),
@item @code{MHD_SC_RESP_HEADERS_CONFLICT} if a Basic Authentication "challenge" was already added,
@item @code{MHD_SC_RESP_POINTER_NULL} if @var{response} was set to @code{NULL},
@item @code{MHD_SC_RESP_HTTP_CODE_NOT_SUITABLE} is response status code is wrong,
@item @code{MHD_SC_RESP_HEADER_VALUE_INVALID} if realm is zero-length or has CR or LF characters,
@item @code{MHD_SC_RESPONSE_HEADER_MEM_ALLOC_FAILED} if memory allocation failed,
@item or possibly other error codes (that could be defined in the future) on failure.
@end itemize
@end deftypefun


@cindex qop
@anchor{MHD_DigestAuthMultiQOP}
@deftp {Enumeration} MHD_DigestAuthMultiQOP

Quality of protection (QOP) levels supported by the server. Values of
this enumeration can be combined with OR to create a bitmask, allowing
the server to offer multiple protection levels to the client.

@table @code

@item MHD_DIGEST_AUTH_MULT_QOP_INVALID
Invalid/unknown QOP.

@item MHD_DIGEST_AUTH_MULT_QOP_NONE
No QOP parameter.
The digest authentication is to proceed as described in
the old RFC 2069 original specification.
This mode is not allowed by latest RFCs and should be used only to
communicate with clients that do not support more modern modes (with QOP
parameter).
This mode is less secure than other modes and inefficient.

@item MHD_DIGEST_AUTH_MULT_QOP_AUTH
The 'auth' QOP type.

@item MHD_DIGEST_AUTH_MULT_QOP_AUTH_INT
The 'auth-int' QOP type. This value is @emph{not} actually supported by MHD.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_QOP_ANY_NON_INT
The 'auth' QOP type OR the old RFC2069 (no QOP) type.
In other words: any types except 'auth-int'.
RFC2069-compatible mode is allowed, thus this value should be used only
when it is really necessary.

@item MHD_DIGEST_AUTH_MULT_QOP_AUTH_ANY
Any 'auth' QOP type ('auth' or 'auth-int').
Currently supported by MHD as 'auth' QOP type only.

@end table
@end deftp


@cindex hash
@cindex digest
@anchor{MHD_DigestAuthMultiAlgo}
@deftp {Enumeration} MHD_DigestAuthMultiAlgo
Which digest algorithm should be used. Values of
this enumeration can be combined with OR to create a bitmask, allowing
the server to offer multiple hash functions to the client.

@table @code

@item MHD_DIGEST_AUTH_MULT_ALGO_INVALID
Unknown or invalid algorithm type.

@item MHD_DIGEST_AUTH_MULT_ALGO_MD5
The 'MD5' algorithm, non-session version. Deprecated, only use if you
have legacy clients that require it.

@item MHD_DIGEST_AUTH_MULT_ALGO_MD5_SESSION
The 'MD5-sess' algorithm.
Not actually supported by MHD for authentication.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA256
The 'SHA-256' algorithm, non-session version.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA256_SESSION
The 'SHA-256-sess' algorithm.
Not supported by MHD for authentication.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA512_256
The 'SHA-512-256' (SHA-512/256) algorithm, non-session version.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA512_256_SESSION
The 'SHA-512-256-sess' (SHA-512/256 session) algorithm.
Not supported by MHD for authentication.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA_ANY_NON_SESSION
SHA-256 or SHA-512/256 non-session algorithm, MHD will choose
the preferred or the matching one.

@item MHD_DIGEST_AUTH_MULT_ALGO_ANY_NON_SESSION
Any non-session algorithm, MHD will choose the preferred or
the matching one.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA_ANY_SESSION
The SHA-256 or SHA-512/256 session algorithm.
Not supported by MHD.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_ALGO_ANY_SESSION
Any session algorithm.
Not supported by MHD.
Reserved value.

@item MHD_DIGEST_AUTH_MULT_ALGO_MD5_ANY
The MD5 algorithm, session or non-session.
Currently supported as non-session only.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA256_ANY
The SHA-256 algorithm, session or non-session.
Currently supported as non-session only.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA512_256_ANY,
The SHA-512/256 algorithm, session or non-session.
Currently supported as non-session only.

@item MHD_DIGEST_AUTH_MULT_ALGO_SHA_ANY_ANY
The SHA-256 or SHA-512/256 algorithm, session or non-session.
Currently supported as non-session only.

@item MHD_DIGEST_AUTH_MULT_ALGO_ANY
Any algorithm. When sending a challenge to the client,
MHD will send challenges for each supported algorithms. When checking
client requests with responses to challenges, MHD will allow
the client to answer with any algorithm it supports.
@end table
@end deftp


@c FIXME: did not document MHD_action_digest_auth_challenge (and _p/_a variants), again not always available (depends on static inline!)
@c FIXME: not convinced we want to keep that, just looks like bloat,

When using digest authentication with @var{userhash_support}, applications
have to compute the @var{userhash} from the @var{username} and the
@var{realm}. This can be done using @code{MHD_digest_auth_calc_userhash()}.



@cindex userhash

@anchor{MHD_digest_auth_calc_userhash}
@deftypefun {enum MHD_StatusCode} MHD_digest_auth_calc_userhash (enum MHD_DigestAuthAlgo algo, const char *username, const char *realm, size_t bin_buf_size, void *userhash_bin)

Calculates the @var{userhash_bin} and returns it as binary data.

The "userhash" is the hash of the string "username:realm".

The "userhash" can be used to avoid sending username in cleartext in
a header when a client is using digest authorization.

The userhash is not designed to hide the username in a local database or files, as the username in cleartext is still required for
the @code{MHD_digest_auth_check_digest()} function to check the response,
it can only can be used to hide the plaintext username in HTTP headers.

This function could be used when the new username is added to a username
database to save the "userhash" alongside with the username (preferably) or
when loading list of the usernames to generate the userhash for every loaded
username (this will cause delays at the start with longer lists).

Once "userhash" is generated it could be used to identify users by clients
with "userhash" support.

The result of the computation may be cached and reused across requests.
@table @var

@item algo
the hash algorithm to use for userhash calculations, see @ref{MHD_DigestAuthAlgorithm};

@item username
the username;

@item realm
the realm;

@item bin_buf_size
the size of the @var{userhash_bin} buffer, must be at least
@code{#MHD_digest_get_hash_size()} bytes long;

@item userhash_bin
the output buffer for userhash as binary data;
if this function succeeds, then this buffer will be set to
@code{MHD_digest_get_hash_size()} bytes of userhash upon return;

@end table

Possible return values include:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_OUT_BUFF_TOO_SMALL} if @var{bin_buf_size} is too small,
@item @code{MHD_SC_HASH_FAILED} if hashing failed, or
@item @code{MHD_SC_AUTH_DIGEST_ALGO_NOT_SUPPORTED} if
   the requested @var{algo} is unknown or unsupported.
@end itemize

@end deftypefun

Different hash algorithms can be specified for the
@var{algo} when computing the userhash.

@cindex hash
@cindex digest
@anchor{MHD_DigestAuthAlgorithm}
@deftp {Enumeration} MHD_DigestAuthAlgorithm
Which digest algorithm should be used.

@table @code
@item MHD_DIGEST_AUTH_ALGO_INVALID
Unknown or invalid algorithm type. Using this value will cause MHD to fail.

@item MHD_DIGEST_AUTH_ALGO_MD5
Use (deprecated, ancient, insecure) MD5.

@item MHD_DIGEST_AUTH_ALGO_MD5_SESSION
Use (deprecated, ancient, insecure) MD5-sess. Not supported by MHD.

@item MHD_DIGEST_ALG_SHA256
Use SHA-256.

@item MHD_DIGEST_ALG_SHA256_SESSION
Use SHA-256-sess. Not supported by MHD.

@item MHD_DIGEST_ALG_SHA512_256
Use SHA-512_256.

@item MHD_DIGEST_ALG_SHA512_256_SESSION
Use SHA-512_256-sess. Not supported by MHD.

@end table
@end deftp

Given a @ref{MHD_DigestAuthAlgorithm,,@code{MHD_DigestAuthAlgorithm}},
@code{MHD_digest_get_hash_size()} can be used to determine the length
of the corresponding binary hash value.


@deftypefun {size_t} MHD_digest_get_hash_size (enum MHD_DigestAuthAlgo algo)

Return the digest size in bytes for the specified hash algorithm @var{algo}.

@table @var

@item algo
the hash algorithm to use for userhash calculations, see @ref{MHD_DigestAuthAlgorithm};
@end table
The function returns the number of bytes of a hash value of the
given @var{algo}. It returns zero if @var{algo} is invalid or
unknown.

@end deftypefun


@c FIXME: MHD_digest_auth_calc_userhash_hex not yet documented, why is this one useful in a MICRO-API that is supposed to be minimal? Do we want to keep it!?

One key advantage of using digest authentication is not only that
the password does not have to be transmitted in the clear, but
also that the application does not need to store the user's password
in cleartext. Instead, applications @emph{should} use
@code{MHD_digest_auth_calc_userdigest()} to compute a hash of
the user's credentials and only store the resulting digest.

@cindex userdigest

@anchor{MHD_digest_auth_calc_userdigest}
@deftypefun {enum MHD_StatusCode} MHD_digest_auth_calc_userdigest (enum MHD_DigestAuthAlgo algo, const char *username, const char *realm, const char *password, size_t bin_buf_size, void *userhash_bin)

Calculates a userdigest and returns it as a binary data.
The "userdigest" is the hash of the "username:realm:password" string.
The "userdigest" can be used to avoid storing the password in clear text
in database/files of the application.

This function is designed to improve security of stored credentials,
using "userdigest" does not improve security of the authentication process.

The result can be used to store @var{username} and @var{userdigest_bin} pairs instead of
@var{username} and @var{password} pairs.
To further improve privacy, application may
store @var{username}, @var{userhash} and @var{userdigest} triplets
and enable @var{userhash_support}.


@table @var

@item algo
the hash algorithm to use for userhash calculations, see @ref{MHD_DigestAuthAlgorithm};

@item username
the username

@item realm
the realm

@item password
the password belonging to @var{username}

@item bin_buf_size
the size of the @var{userdigest_bin} buffer, must be at least
@code{#MHD_digest_get_hash_size()} bytes long

@item userdigest_bin
the output buffer for userdigest as binary data;
if this function succeeds, then this buffer will be set to
@code{MHD_digest_get_hash_size()} bytes of userhash upon return

@end table

Possible return values include:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_OUT_BUFF_TOO_SMALL} if @var{bin_buf_size} is too small,
@item @code{MHD_SC_HASH_FAILED} if hashing failed,
@item @code{MHD_SC_AUTH_DIGEST_ALGO_NOT_SUPPORTED} if
   the requested @var{algo} is unknown or unsupported.
@end itemize

@end deftypefun

When a client provides a digest authentication header, the application
@emph{must} first use introspection using
@ref{MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_INFO,,@code{MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_INFO}}
to determine the client's identity and algorithmic choices.
Introspection will return a @code{struct MHD_AuthDigestInfo}, or
@code{NULL} if the client did not provide a digest
authorization header.

@c FIXME: MHD_REQUEST_INFO_DYNAMIC_AUTH_DIGEST_USERNAME is not discussed. When exactly is this necessary!? => will be removed.

@anchor{MHD_AuthDigestInfo}
@deftp {C Struct} MHD_AuthDigestInfo

Information decoded from a client's header with digest authentication data.
@table @var

@item @code{enum MHD_DigestAuthAlgo} algo
The algorithm as selected by the client.
Set automatically to MD5 if not explicitly specified by client.

@item @code{enum MHD_DigestAuthUsernameType} uname_type
The type of username used by client.
@xref{MHD_DigestAuthUsernameType}.

@item @code{struct MHD_StringNullable} username
The username as a string.
Used only if @var{uname_type} is standard or extended, always
@code{NULL} otherwise.
If extended notation is used, this string is a pct-decoded string
with charset and language tag removed (i.e. it is original username
extracted from the extended notation).
When userhash is used by the client, the string pointer is @code{NULL} and
@var{userhash_hex} and @var{userhash_bin} will be set instead.

@item @code{struct MHD_StringNullable} userhash_hex
The userhash string.
Valid only if @var{uname_type} is userhash.
This is unqoted string without decoding of the hexadecimal
digits (as provided by the client).
@c FIXME: why do we return both the _hex and the _bin variants? Seems redundant!

@item @code{const uint8_t *} userhash_bin
The userhash decoded to binary form.
Used only if @var{uname_type} is userhash, always @code{NULL} otherwise.
When not @code{NULL}, this points to binary sequence of @var{userhash_bin_size} bytes.
The valid size should be @code{MHD_digest_get_hash_size()} bytes.
@emph{Warning:} This buffer points to binary data, there is no zero termination!
@emph{Warning:} To avoid buffer overruns, always check the size of the data before
 use, because @var{userhash_bin} can point even to zero bytes of data.

@item @code{size_t} userhash_bin_size
The size of the data pointed by @var{userhash_bin}.
Always zero when @var{userhash_bin} is @code{NULL}.

@item @code{struct MHD_StringNullable} opaque
The 'opaque' parameter value, as specified by client.
If not specified by client then string pointer is @code{NULL}.

@item @code{struct MHD_StringNullable} realm
The 'realm' parameter value, as specified by client.
If not specified by client then string pointer is @code{NULL}.

@item @code{enum MHD_DigestAuthQOP} qop
The quality of protection (qop) parameter value.

@item @code{size_t} cnonce_len
Length of the @var{cnonce} parameter, including possible
backslash-escape characters. @var{cnonce} is used in hash
calculation.
An application may want to reject too large @var{cnonce}
values to limit CPU load.
A few kilobytes is a reasonable limit, typical values for
@var{cnonce} should just use 32-160 characters.

@item @code{enum MHD_DigestAuthNC} nc_type
The type of the nonce count (nc) value provided in the request.
@xref{MHD_DigestAuthNC}.

@item @code{uint_fast32_t} nc
The nonce count (nc) parameter value.
Can be used by application to limit the number of nonce re-uses. If @var{nc}
is higher than application wants to allow, then "auth required" response
with ``stale=true'' could be used to force client to retry with the fresh
@var{nonce}.
Set to zero when @var{nc_type} is not set to
@code{MHD_DIGEST_AUTH_NC_NUMBER}.

@end table
@end deftp

The type of client nonce provided by the client is represented
by an @code{enum MHD_DigestAuthNC}.

@anchor{MHD_DigestAuthNC}
@deftp {Enumeration} MHD_DigestAuthNC

The type of nonce count (nc) value provided in the request.

@table @code

@item MHD_DIGEST_AUTH_NC_NUMBER
Readable hexdecimal non-zero number.
The decoded value is placed in the @var{nc} member of
@ref{MHD_AuthDigestInfo,,@code{MHD_AuthDigestInfo}}.

@item MHD_DIGEST_AUTH_NC_ZERO
Readable zero number.
Compliant clients should not use such values.
Should probably be treated as an invalid request.

@item MHD_DIGEST_AUTH_NC_NONE
No @var{nc} value was provided by the client.
Unless old RFC 2069 mode is allowed, this should be treated as an invalid
request.

@item MHD_DIGEST_AUTH_NC_TOO_LONG
The provided ``nc'' value was too long to be decoded.
Compliant clients should not use such values.
Should be treated as an invalid request.

@item MHD_DIGEST_AUTH_NC_TOO_LARGE
The provided ``nc'' value was too large to fit into a @code{uint32_t}.
Compliant clients should not use such values.
Can be treated as request with a stale nonce or as an invalid request.

@end table
@end deftp

@c FIXME: MHD_DigestAuthUsernameType not defined

@anchor{MHD_DigestAuthUsernameType}
@deftp {enum} MHD_DigestAuthUsernameType 

Type of username encoded by the client in a digest authorization
header.

@table @var
@item @code{MHD_DIGEST_AUTH_UNAME_TYPE_MISSING} 
  No username parameter is in Digest Authorization header.
  Not used currently. Value @code{MHD_SC_REQ_AUTH_DATA_BROKEN} is returned
  by @ref{MHD_request_get_info_dynamic_sz,,@code{MHD_request_get_info_dynamic_sz()}}
  if the request has no username.

@item @code{MHD_DIGEST_AUTH_UNAME_TYPE_STANDARD}
  The 'username' parameter is used to specify the username.

@item @code{MHD_DIGEST_AUTH_UNAME_TYPE_EXTENDED}
  The username is specified by 'username*' parameter with
  the extended notation (see RFC 5987, section-3.2.1).
  The only difference between standard and extended types is
  the way how username value is encoded in the header.

@item @code{MHD_DIGEST_AUTH_UNAME_TYPE_USERHASH}
  The username provided in form of 'userhash' as
  specified by RFC 7616, section-3.4.4.
  @xref{MHD_digest_auth_calc_userhash,,@code{MHD_digest_auth_calc_userhash()}}.

@item @code{MHD_DIGEST_AUTH_UNAME_TYPE_INVALID}
  The invalid combination of username parameters are used by client.
  Either:
  @itemize @bullet
  @item both 'username' and 'username*' are used
  @item 'username*' is used with 'userhash=true'
  @item 'username*' used with invalid extended notation
  @item 'username' is not hexadecimal string, while 'userhash' set to 'true'
  @end itemize
  Not used currently. Value @code{MHD_SC_REQ_AUTH_DATA_BROKEN} is returned by
  @ref{MHD_request_get_info_dynamic_sz,,@code{MHD_request_get_info_dynamic_sz()}}
  if the request has broken username.
@end table
@end deftp



Finally, given the client's identity (username or userhash) the
application must possibly determine the username from the userhash,
and always determine the userdigest.  Given the @var{username} and
@var{userdigest} and the set of algorithms allowed by the application,
it can can use @code{MHD_digest_auth_check_digest()} to actually check
the digest authentication provided by the client's request.

@anchor{MHD_digest_auth_check_digest}
@deftypefun {enum MHD_DigestAuthResult} MHD_digest_auth_check_digest (struct MHD_Request *request, const char *realm, const char *username, size_t userdigest_size, const void *userdigest, uint_fast32_t max_nc, enum MHD_DigestAuthMultiQOP mqop, enum MHD_DigestAuthMultiAlgo malgo)

@cindex userdigest
Authenticates the authorization header sent by the client by using
hash of "username:realm:password".

If RFC2069 mode is allowed by setting bit @code{MHD_DIGEST_AUTH_QOP_NONE} in
@var{mqop} and the client uses this mode, then server generated nonces are
used as one-time nonces because nonce-count is not supported in this old RFC.
Communication in this mode is very inefficient, especially if the client
requests several resources one-by-one as for every request a new nonce must
be generated and client repeats all requests twice (first time to get a new
nonce and second time to perform an authorised request).

@table @var
@item request
the request;

@item realm
the realm for authorization of the client;

@item username
the username to be authenticated, must be in clear text
even if userhash is used by the client;

@item userdigest_size
the size of the @var{userdigest} in bytes, must match the
hashing algorithm;

@item userdigest
the precalculated binary hash of the string
"username:realm:password";
see @ref{MHD_digest_auth_calc_userdigest,,@code{MHD_digest_auth_calc_userdigest}};

@item max_nc
the maximum allowed nonce counter (nc) value, if a client's nc
exceeds the specified value then @code{MHD_DAUTH_NONCE_STALE} is
returned; if zero is specified the daemon's default value is used;

@item mqop
the quality of protection (QOP) level to use;

@item malgo
digest algorithms allowed to use, authentication will fail if
the algorithm used by the client is not allowed by this parameter;
only the base algorithms (MD5, SHA-256, SHA-512/256)
cannot be used at the same time for this function
as the @var{userdigest} must match specified algorithm;

@end table

The function returns the result of the authentication check.
@xref{MHD_DigestAuthResult}.

@end deftypefun

@anchor{MHD_DigestAuthResult}
@deftp {Enumeration} MHD_DigestAuthResult

The result of digest authentication of the client.

@table @code
@item MHD_DAUTH_OK
Authentication is OK;

@item MHD_DAUTH_ERROR
General error, like ``out of memory'';
Authentication may be valid, but cannot be checked;

@item MHD_DAUTH_HEADER_MISSING
No "Authorization" header for digest authentication
is present in the client's request.

@item MHD_DAUTH_HEADER_BROKEN
Header is in a wrong format.
Also returned if required parameters in Authorization header are missing
or broken.

@item MHD_DAUTH_UNSUPPORTED_ALGO
Unsupported algorithm used by the client.

@item MHD_DAUTH_UNSUPPORTED_QOP
Unsupported quality of protection (qop) level used by the client.

@item MHD_DAUTH_INVALID_USERDIGEST_SIZE
Incorrect userdigest size used by the client.

@item MHD_DAUTH_WRONG_USERNAME
Wrong 'username'.

@item MHD_DAUTH_WRONG_REALM
Wrong 'realm'.

@item MHD_DAUTH_WRONG_URI
Wrong 'URI' (or URI parameters).

@item MHD_DAUTH_WRONG_QOP
'qop' value specified by the client is not in the set of values
allowed by the specification or the server disallowed the use of
the given 'qop' mode.

@item MHD_DAUTH_WRONG_ALGO
The (hash) algorithm value specified by the client is not in the set of values
allowed by the specification or the server disallowed the use of
the given hash function mode.

@item MHD_DAUTH_TOO_LARGE
Too large (larger than 64 KiB) ``Authorization'' parameter value.

@item MHD_DAUTH_NONCE_STALE
The 'nonce' is too old. Suggest the client to retry with the same
username and password to get the fresh 'nonce'.
The validity of the 'nonce' may be not checked.

@item MHD_DAUTH_NONCE_WRONG
The 'nonce' is wrong. May indicate an attempted attack.

@item MHD_DAUTH_RESPONSE_WRONG
The 'response' is wrong. May indicate using the wrong password.

@end table
@end deftp


@c @node libmicrohttpd-dauth example
@c @section Example for Digest Authentication
@c @cindex digest authentication

@c FIXME: we should probably add an example here...

@c @example
@c @verbatiminclude examples/digest-authentication.c
@c @end example
