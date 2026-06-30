
@deftypefun enum MHD_Result MHD_get_fdset (struct MHD_Daemon *daemon, fd_set * read_fd_set, fd_set * write_fd_set, fd_set * except_fd_set, int *max_fd)
Obtain the @code{select()} sets for this daemon. The daemon's socket
is added to @var{read_fd_set}. The list of currently existent
connections is scanned and their file descriptors added to the correct
set.

When calling this function, FD_SETSIZE is assumed to be platform's
default.  If you changed FD_SETSIZE for your application,
you should use @code{MHD_get_fdset2()} instead.

This function should only be called in when MHD is configured to use
external select with @code{select()} or with @code{epoll()}.  In
the latter case, it will only add the single @code{epoll()} file
descriptor used by MHD to the sets.

After the call completed successfully: the variable referenced by
@var{max_fd} references the file descriptor with highest integer
identifier. The variable must be set to zero before invoking this
function.

Return @code{MHD_YES} on success, @code{MHD_NO} if: the arguments are
invalid (example: @code{NULL} pointers); this daemon was not started with
the right options for this call.
@end deftypefun


@deftypefun enum MHD_Result MHD_get_fdset2 (struct MHD_Daemon *daemon, fd_set * read_fd_set, fd_set * write_fd_set, fd_set * except_fd_set, int *max_fd, unsigned int fd_setsize)
Like @code{MHD_get_fdset()}, except that you can manually specify the value of FD_SETSIZE used by your application.
@end deftypefun


@deftypefun enum MHD_Result MHD_get_timeout (struct MHD_Daemon *daemon, unsigned long long *timeout)
@cindex timeout
Obtain timeout value for select for this daemon (only needed if
connection timeout is used).  The returned value is how many
milliseconds @code{select} should at most block, not the timeout value
set for connections.  This function must not be called if the
@code{MHD_USE_THREAD_PER_CONNECTION} mode is in use (since then it is
not meaningful to ask for a timeout, after all, there is concurrenct
activity).  The function must also not be called by user-code if
@code{MHD_USE_INTERNAL_POLLING_THREAD} is in use.  In the latter case, the
behavior is undefined.

@table @var
@item daemon
which daemon to obtain the timeout from.
@item timeout
will be set to the timeout (in milliseconds).
@end table

Return @code{MHD_YES} on success, @code{MHD_NO} if timeouts are not used
(or no connections exist that would necessitate the use of a timeout
right now).
@end deftypefun

