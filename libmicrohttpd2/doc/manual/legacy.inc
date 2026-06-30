@c graveyard, currently not used, but might be useful in the future...

@c ------------------------------------------------------------
@node microhttpd-util feature
@section Testing for supported MHD features


@deftp {Enumeration} MHD_FEATURE
Values of this enum are used to specify what
information about a daemon is desired.
@table @code
@item MHD_FEATURE_MESSAGES
Get whether messages are supported. If supported then in debug
mode messages can be printed to stderr or to external logger.

@item MHD_FEATURE_SSL
Get whether HTTPS is supported.  If supported then flag
MHD_USE_SSL and options MHD_OPTION_HTTPS_MEM_KEY,
MHD_OPTION_HTTPS_MEM_CERT, MHD_OPTION_HTTPS_MEM_TRUST,
MHD_OPTION_HTTPS_MEM_DHPARAMS, MHD_OPTION_HTTPS_CRED_TYPE,
MHD_OPTION_HTTPS_PRIORITIES can be used.

@item MHD_FEATURE_HTTPS_CERT_CALLBACK
Get whether option #MHD_OPTION_HTTPS_CERT_CALLBACK is
supported.

@item MHD_FEATURE_IPv6
Get whether IPv6 is supported. If supported then flag
MHD_USE_IPv6 can be used.

@item MHD_FEATURE_IPv6_ONLY
Get whether IPv6 without IPv4 is supported. If not supported
then IPv4 is always enabled in IPv6 sockets and
flag MHD_USE_DUAL_STACK if always used when MHD_USE_IPv6 is
specified.

@item MHD_FEATURE_POLL
Get whether @code{poll()} is supported. If supported then flag
MHD_USE_POLL can be used.

@item MHD_FEATURE_EPOLL
Get whether @code{epoll()} is supported. If supported then Flags
MHD_USE_EPOLL and
MHD_USE_EPOLL_INTERNAL_THREAD can be used.

@item MHD_FEATURE_SHUTDOWN_LISTEN_SOCKET
Get whether shutdown on listen socket to signal other
threads is supported. If not supported flag
MHD_USE_ITC is automatically forced.

@item MHD_FEATURE_SOCKETPAIR
Get whether a @code{socketpair()} is used internally instead of
a @code{pipe()} to signal other threads.

@item MHD_FEATURE_TCP_FASTOPEN
Get whether TCP Fast Open is supported. If supported then
flag MHD_USE_TCP_FASTOPEN and option
MHD_OPTION_TCP_FASTOPEN_QUEUE_SIZE can be used.

@item MHD_FEATURE_BASIC_AUTH
Get whether HTTP Basic authorization is supported. If supported
then functions @code{MHD_basic_auth_get_username_password()} and
@code{MHD_queue_basic_auth_fail_response()} can be used.

@item MHD_FEATURE_DIGEST_AUTH
Get whether HTTP Digest authorization is supported. If
supported then options MHD_OPTION_DIGEST_AUTH_RANDOM,
MHD_OPTION_NONCE_NC_SIZE and functions @code{MHD_digest_auth_check()},
can be used.

@item MHD_FEATURE_POSTPROCESSOR
Get whether postprocessor is supported. If supported then
functions @code{MHD_create_post_processor()},
@code{MHD_post_process()}, @code{MHD_destroy_post_processor()}
can be used.

@item MHD_FEATURE_SENDFILE
Get whether @code{sendfile()} is supported.

@end table
@end deftp



@deftypefun {int} MHD_is_feature_supported (enum MHD_FEATURE feature)
Get information about supported MHD features.  Indicate that MHD was
compiled with or without support for particular feature. Some features
require additional support by the kernel.  However, kernel support is not
checked by this function.

@table @var
@item feature
type of requested information
@end table

Returns @code{MHD_YES} if the feature is supported,
and @code{MHD_NO} if not.
@end deftypefun







@c ------------------------------------------------------------
@node microhttpd-util unescape
@section Unescape strings

@deftypefun {size_t} MHD_http_unescape (char *val)
Process escape sequences ('%HH') Updates val in place; the result
should be UTF-8 encoded and cannot be larger than the input.  The
result must also still be 0-terminated.

@table @var
@item val
value to unescape (modified in the process), must be
a 0-terminated UTF-8 string.
@end table

Returns length of the resulting val (@code{strlen(val)} may be
shorter afterwards due to elimination of escape sequences).

@end deftypefun
