This chapter discusses how to convert an existing codebase that uses
GNU libmicrohttpd 1.x (or 0.x) to the GNU libmicrohttpd 2.x API. It
was written based on the experience of migrating GNU Taler's Sync
service to the new API. It should cover @emph{most} of the important
steps, but may omit some details. In general, there are no
@emph{subtle} steps needed to migrate a code base: if the new code
compiles, it most likely is OK as there is no function that kept is
name but changed semantics, and the new API is generally simply safer
to use and has fewer corner cases to consider.

@node libmicrohttpd-configure
@section Compile-time testing for the MHD 2.x dependency
@cindex autoconf
@cindex automake
@cindex pkg-config

Typically, the first step for migrating to the new API is to
add (or replace) the detection logic for the MHD library in
the build system. With GNU autotools and @code{pkg-config},
the following snippet could be placed in @code{configure.ac}
to detect libmicrohttpd2:

@verbatim
AC_ARG_WITH([microhttpd2],
  [AS_HELP_STRING([--with-microhttpd2=PFX],
                  [base of libmicrohttpd2 installation])],
  [SAVE_PKG_PATH="$PKG_CONFIG_PATH"
   PKG_CONFIG_PATH="${withval}/lib/pkgconfig"
   export PKG_CONFIG_PATH
   PKG_CHECK_MODULES([MHD], [libmicrohttpd2 >= 2.0.0], [libmhd2=1], [libmhd2=0])
   PKG_CONFIG_PATH="$SAVE_PKG_PATH"],
  [PKG_CHECK_MODULES([MHD], [libmicrohttpd2 >= 2.0.0], [libmhd2=1], [libmhd2=0])])

# Do this only if you require MHD2:
AS_IF([test "x$libmhd2" = "x0"],
  [AC_MSG_ERROR([This program requires libmicrohttpd >= 2.0.0])])

# Alternatively, do this to conditionally compile MHD2 code:
AM_CONDITIONAL([HAVE_MHD2], [test "x$libmhd2" = "x1"])

# and/or use this to use ifdef to compile code only when MHD2 is available:
AC_DEFINE_UNQUOTED([HAVE_MHD2], [$libmhd2],
                   [Define to 1 if libmicrohttpd2 is available])
@end verbatim

If you do not want to rely on @code{pkg-config}, you can use
a more direct test:

@verbatim
# check for libmicrohttpd
AC_MSG_CHECKING([for microhttpd2])
AC_ARG_WITH([microhttpd2],
            [AS_HELP_STRING([--with-microhttpd2=PFX], [base of libmicrohttpd2 installation])],
            [AC_MSG_RESULT([given as $with_microhttpd2])],
            [AC_MSG_RESULT([not given])
             with_microhttpd2=yes])
AS_CASE([$with_microhttpd2],
        [yes],,
        [no],,
        [LDFLAGS="-L$with_microhttpd2/lib $LDFLAGS"
         CPPFLAGS="-I$with_microhttpd2/include $CPPFLAGS"])
# This will set HAVE_MHD2 as above for conditional compilation
MHD2_VERSION_AT_LEAST([2.0.0])
@end verbatim

The @code{MHD2_VERSION_AT_LEAST} is an M4 macro available in the
@code{mhd2.m4} file as part of the GNU libmicrohttpd2 library which
implements a test for the specific MHD version. You may need to copy
the @code{mhd2.m4} file into your @code{m4/} directory so that GNU
autoconf can find it.

@cindex meson

When using @code{Meson} as the buidl system, a dependency test
would typically look like this:

@verbatim
mhd2_dep = dependency('libmicrohttpd2', required : false)
if not mhd2_dep.found()
  mhd_dep2 = cc.find_library('microhttpd2', required : false)
endif
@end verbatim

Adjust the @code{required} argument to
@code{true} if MHD2 is a mandatory dependency.

Naturally, there are various variations of the above that
could be used to detect MHD2, so these are merely examples
to get started.


@node libmicrohttpd-include
@section Changes to includes
@cindex include

The obvious change here is to change
@verbatim
#include <microhttpd.h>
@end verbatim

to

@verbatim
#include <microhttpd2.h>
@end verbatim

However, there might be one more subtle consideration, which
is that in the previous version the @code{microhttpd.h} header
required the application to possibly include various system
header first. That requirement was lifted with MHD2, so this
could be an opportunity to review whether some other includes
are still necessary and possibly remove them.

@node libmicrohttpd-constant-replacement
@section Global constants to migrate

Globally, the following constants must be renamed. The
constants in this list are 1:1 replacements and thus
a simple global rename operation should be safe:

@table @code
@item MHD_HEADER_KIND
must be replaced by @code{MHD_VK_HEADER}.

@item MHD_COOKIE_KIND
must be replaced by @code{MHD_VK_COOKIE}.

@item MHD_POSTDATA_KIND
must be replaced by @code{MHD_VK_POSTDATA}.

@item MHD_FOOTER_KIND
must be replaced by @code{MHD_VK_FOOTER}.

@item MHD_GET_ARGUMENT_KIND
must be replaced by @code{MHD_VK_GET_ARGUMENT}.
@c FIXME: under discussion to change away from GET!

@item MHD_HTTP_REQUEST_ENTITY_TOO_LARGE
must be replaced by @code{MHD_HTTP_STATUS_CONTENT_TOO_LARGE}.

@end table




@node libmicrohttpd-migrate-start-daemon
@section Changes to @code{MHD_start_daemon()}

The main change to @code{MHD_start_daemon} is to rename the
call to @code{MHD_daemon_create}. The existing argument of
type @code{MHD_AccessHandlerCallback} should be changed
into the new @code{MHD_RequestCallback}. All other arguments
(explicit and from the varargs)
need to be set via @ref{MHD_DAEMON_SET_OPTIONS} and related
APIs. @xref{libmicrohttpd-daemon-options-setting}
Finally, after setting the options, the application
must call @ref{MHD_daemon_start}. Thus, the existing
@code{MHD_start_daemon} is replaced by three main calls:

@itemize
@item @ref{MHD_daemon_create}
@item @ref{MHD_daemon_set_options}
@item @ref{MHD_daemon_start}
@end itemize

There are a few minor changes to be made to the related API calls
to quiesce and destroy the daemon: the functions
@code{MHD_stop_daemon()} and @code{MHD_quiesce_daemon()} were renamed
to @code{MHD_daemon_destroy()} and @code{MHD_daemon_quiesce()}
respectively. Nevertheless, the behavior is completely unchanged,
so applications only need to rename the calls.

@node libmicrohttpd-migrate-loop
@section Migrating an external event loop

When migrating an external event loop, the first significant change
is that one must expicitly set an option with a callback for MHD
to call on changes to the set of watched sockets.  Previously, the
event loop could query MHD using @code{MHD_get_fdset()} (and related
APIs). Now, the application must instead configure the daemon with
a callback:

@verbatim
  MHD_DAEMON_SET_OPTIONS (
    daemon,
    MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL (
      &socket_registration_update,
      NULL));
@end verbatim

For our example, the application will define
@code{MHD_APP_SOCKET_CNTX_TYPE} to a @code{struct ScoketContext}.
Inside of that structure, the application will track its state for
the external event loop:

@verbatim
#define MHD_APP_SOCKET_CNTX_TYPE struct SocketContext
#include <microhttpd2.h>

struct SocketContext {
  // Note: add other data specific to the event loop!
  struct MHD_EventUpdateContext *ecb_cntx;
  MHD_Socket fd;
};
@end verbatim

In the respective callback (here @code{socket_registration_update()})
the application must then update its state for the event loop:

@verbatim
static MHD_APP_SOCKET_CNTX_TYPE *
socket_registration_update (
  void *cls,
  MHD_Socket fd,
  enum MHD_FdState watch_for,
  MHD_APP_SOCKET_CNTX_TYPE *app_cntx_old,
  struct MHD_EventUpdateContext *ecb_cntx)
{
  struct SocketContext *sc;

  if (NULL == app_cntx_old)
  {
    sc = malloc (sizeof (struct SocketContext));
    sc->ecb_cntx = ecb_cntx;
    sc->fd = fd;
  }
  else
  {
    sc = app_cntx_old;
  }
  if (MHD_FD_STATE_NONE == watch_for)
  {
    // clean up everything in 'app_cntx_old'
    free (app_cntx_old);
    return NULL;
  }
  if (MHD_FD_STATE_RECV & watch_for)
  {
    // Update 'app_cntx_old' to notify on read-ready
    // (e.g. FD_SET (fd, readfds))
  }
  else
  {
    // Update 'app_cntx_old' to not notify on read-ready
    // (e.g. FD_CLEAR (fd, readfds))
  }
  if (MHD_FD_STATE_SEND & watch_for)
  {
    // Update 'app_cntx_old' to notify on write-ready
    // (e.g. FD_SET (fd, writefds))
  }
  else
  {
    // Update 'app_cntx_old' to not notify on write-ready
    // (e.g. FD_CLEAR (fd, writefds))
  }
  if (MHD_FD_STATE_EXCEPT & watch_for)
  {
    // Update 'app_cntx_old' to notify on exceptions
    // (e.g. FD_SET (fd, exceptfds))
  }
  else
  {
    // Update 'app_cntx_old' to not notify on exceptions
    // (e.g. FD_CLEAR (fd, exceptfds))
  }
  return sc;
}
@end verbatim

Finally, when the external event loop detects that the socket is
ready for some operation, it must then signal that the respective
operation is ready:

@verbatim
struct SocketContext *sc = ...;

MHD_daemon_event_update (daemon,
                         sc->ecb_cntx,
                         MHD_FD_STATE_RECV);
@end verbatim

Naturally, @code{MHD_FD_STATE_RECV} should be replaced by the
correct set of operations that are ready.

Finally, the application needs to take care to give MHD a chance
to actually process events. This must be done every time either
the global timeout expired, or @code{MHD_daemon_event_update()}
was called:

@verbatim
void
run_daemon (void)
{
  uint_fast64_t next_max_wait;

  MHD_daemon_process_reg_events (daemon,
                                 &next_max_wait));
  // wait next_max_wait *or* FD ready before calling
  // again
}
@end verbatim

There is a special case to consider, namely when
@var{next_max_wait} is set to @code{MHD_WAIT_INDEFINITELY}
then the timeout is "forever" and the application should only
wait on the socket states.

A complete example for this type of external event loop migration
using the GNUnet event loop can be found at
@url{https://git.taler.net/exchange.git/tree/src/mhd}. Here, the
@file{mhd_run.c} file contains the MHD v1.0 logic, and @file{mhd2_run.c}
contains the equivalent MHD v2.0 logic.  The external API of the
module is completely unchanged.

@node libmicrohttpd-migrate-request-handling
@section Migrating request handling

The first change to request handling is to replace all existing uses
of @code{struct MHD_Connection} with the new @code{struct
MHD_Request}.  While @code{struct MHD_Connection} still exists in the
new API, it represents something else and thus legacy code should be
transformed by first changing all @code{struct MHD_Connection} to
@code{struct MHD_Request}.

When inspecting the HTTP request, the following substitutions
should be made:

@table @code
@item MHD_lookup_connection_value
replaced by @ref{MHD_request_get_value}; note the slight change
in the return value type.

@end table

Applications that previously used the
@var{req_cls} argument of the
@code{MHD_AccessHandlerCallback} must now use
@ref{MHD_REQUEST_INFO_FIXED_APP_CONTEXT} with
@ref{MHD_request_get_info_fixed} to get the
same @code{void **} from inside the
@ref{MHD_RequestInfoFixedData}.
@c FIXME: specify which member (to be renamed...)


@node libmicrohttpd-migrate-upload-handling
@section Migrating upload handling

When handling requests with a body, previous code had to return
@code{MHD_YES} from the main callback on the first call and
then receive the upload data incrementally in subsequent calls.
In MHD2, the application must instead return an action
generated from calls to @ref{MHD_action_process_upload},
@ref{MHD_action_process_upload_full}, or
@ref{MHD_action_process_upload_inc}.

Explicitly checking the "Content-Length" request header to ensure
that the upload is not too big is no longer necessary as MHD
now provides the request body length explicitly to the main
callback.

When returning a response from an upload, calls to
@code{MHD_queue_response} must be replaced by
returning the action created by
@code{MHD_upload_action_from_response}.


@node libmicrohttpd-migrate-suspend-handling
@section Migrating suspended requests handling

Previous calls to @code{MHD_suspend_request} must be
replaced by returning an action from
@ref{MHD_action_suspend}, @ref{MHD_upload_action_suspend}
or @ref{MHD_DCC_action_suspend}.

Previous calls to @code{MHD_resume_request} must be
replaced by calls to @ref{MHD_request_resume}.



@node libmicrohttpd-migrate-response-generation
@section Migrating response generation

The main change in response generation is that with MHD2 you
must always already provide the HTTP status code when creating
a response. Thus, functions that could previously merely create
the response from headers and body must be modified to also
take the HTTP status code. Also, previously applications
may have used a simple @code{unsigned int} to represent the
HTTP status code. This should be changed to the new
@code{enum MHD_HTTP_StatusCode}.

The names for the HTTP status code constants changed, all constants
that started with @code{MHD_HTTP_} need to be updated to use the new
@code{MHD_HTTP_STATUS_} prefix.  Note that this only applies
to the HTTP status constants: the constants starting with the
@code{MHD_HTTP_HEADER_} prefix must not be changed!

Other than that, the main change is that the functions
to generate responses were renamed, the new functions
all start with @code{MHD_response_from_}. All previous
functions have a clear match in the new API. The following
table contains a short cheat-list:

@table @code
@item MHD_add_response_header
replaced by @ref{MHD_response_add_header}; note the slight change
in the return value type (now @code{MHD_SC_OK} (aka 0) is returned
instead of @code{MHD_YES} (aka 1) for success).

@item MHD_create_response_from_buffer
replaced by @ref{MHD_response_from_buffer} and convenience
wrappers @ref{MHD_response_from_buffer_static},
@ref{MHD_response_from_buffer_copy},
@ref{MHD_response_from_empty}.

@item MHD_create_response_from_fd
replaced by @ref{MHD_response_from_fd} (with additional @var{offset}
argument).

@end table

Finally, to return a response, the call to @code{MHD_queue_response()}
will need to be replaced by a call to
@code{MHD_action_from_response()} or
@code{MHD_upload_action_from_response()} depending on which callback
is returning the response.  After calling
@code{MHD_upload_action_from_response()} the calls to
@code{MHD_destroy_response()} should be removed, @emph{unless}
@ref{MHD_R_OPTION_REUSABLE} was set on the response.
Calls to @code{MHD_destroy_response()} on code paths that do not
lead to the response being turned into an action should be
replaced by @code{MHD_response_destroy()}.

@node libmicrohttpd-migrate-post-processor
@section Migrating post processing

The main change for post-processing is that instead of taking the
upload data from the main callback and incrementally feeding it into a
@code{struct MHD_PostProcessor}, the application should now simply
create a @ref{MHD_Action} via @ref{MHD_action_parse_post}. This
effectively combines creating the post processor, passing it the
uploaded data and receiving the results. The new API is more powerful,
so if the upload is known to be small enough (like for a simple HTML
form without file uploads) the application no longer needs to deal
with incremental value uploads and can simply inspect all uploaded
values in the @code{MHD_PostDataFinished} callback using
@ref{MHD_request_get_value} (and related APIs) with the
@code{MHD_VK_POSTDATA}.


@node libmicrohttpd-migrate-notifications
@section Changes to notification callbacks

The @code{MHD_OPTION_NOTIFY_COMPLETED} was replaced
with the @code{MHD_R_OTION_TERMINATION_CALLBACK} which
must now be set on each request instead of once per
daemon.


@node libmicrohttpd-migrate-introspection
@section Changes to introspection calls

These are pretty simple substitutions:

@table @code
@item MHD_get_request_info
replaced by @ref{MHD_request_get_info_fixed} or
@ref{MHD_request_get_info_dynamic} depending on the
type of information requested

@item MHD_RequestInfo
replaced by @ref{MHD_RequestInfoFixedData} or
@ref{MHD_RequestInfoDynamicData} depending on the
type of information requested

@item MHD_REQUEST_INFO_
replaced by @code{MHD_REQUEST_INFO_FIXED_} or
@code{MHD_REQUEST_INFO_DYNAMIC_} depending on the
type of information requested;
see @ref{libmicrohttpd2-info request,,request introspection}

@item MHD_get_daemon_info
replaced by @ref{MHD_daemon_get_info_fixed} or
@ref{MHD_daemon_get_info_dynamic} depending on the
type of information daemoned

@item MHD_DaemonInfo
replaced by @ref{MHD_DaemonInfoFixedData} or
@ref{MHD_DaemonInfoDynamicData} depending on the
type of information daemoned

@item MHD_DAEMON_INFO_
replaced by @code{MHD_DAEMON_INFO_FIXED_} or
@code{MHD_DAEMON_INFO_DYNAMIC_} depending on the
type of information daemoned;
see @ref{libmicrohttpd2-info daemon,,daemon introspection}

@end table

The @code{MHD_is_feature_supported} function is replaced by the new
@ref{libmicrohttpd2-info library,,library introspection} API.