@menu
* microhttpd-add connection::   Passing externally accepted connections
@c * microhttpd-util feature::     Test supported MHD features
@c * microhttpd-util unescape::    Unescape strings
@end menu


@c ------------------------------------------------------------
@node microhttpd-add connection
@section Passing externally accepted connections


@anchor{MHD_daemon_add_connection}
@deftypefun void MHD_daemon_add_connection (struct MHD_Daemon *daemon, MHD_Socket client_socket, size_t addrlen, const struct sockaddr *addr, void *connection_cntx)
Add another client connection to the set of connections
managed by MHD.  This API is usually not needed (since
MHD will accept inbound connections on the server socket).
Use this API in special cases, for example if your HTTP
server is behind NAT and needs to connect out to the
HTTP client, or if you are building a proxy.

The given client socket will be managed (and closed!) by MHD after
this call and must no longer be used directly by the application
afterwards.

The client socket will be closed by MHD even if error returned.
 
@table @var
@item daemon
daemon that manages the connection
@item client_socket
socket to manage (MHD will expect to receive an HTTP request from this socket next).
@item addrlen
number of bytes in @var{addr}
@item addr
IP address of the client
@item connection_cntx meta data the application wants to
 *        associate with the new connection object
@end table

This function will return @code{MHD_SC_OK} on success,
or an code indicating an error if this daemon could
not handle the connection (i.e. malloc failed, etc).
The socket will be closed in any case.
@end deftypefun


