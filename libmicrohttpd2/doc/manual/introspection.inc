This chapter explains how applications can introspect the state of the
MHD library, a @code{struct MHD_Daemon}, a @code{struct
MHD_Connection}, a @code{struct MHD_Session} or a
@code{MHD_Request}. Now, until now most of the discussed APIs have
primarily interacted with a daemon or a request, which raises the
question how applications can even get hold of a connection or
session. The two main ways are either to set daemon options to receive
callbacks (say when a connection or session is established or
completed), or via introspection of a given request.

All introspection APIs work in a similar way: they are given the
object to inspect, a member of an enumeration specifying what
property the application is interested in, and a pointer to a
@code{union} where MHD is to store the value of the property.

Introspection APIs for each type of object are split into two
sub-APIs. One function to obtain @emph{fixed} data that never changes
during the lifetime of the inspected object and that the application
can thus continue to reference until the underlying object is
destroyed, and a second function to obtain @emph{dynamic} data that is
ephemeral and that can change over time (including becoming available
later or disappearing) and where the application thus has to be
careful when to ask for the data and to not keep pointers to stale
addresses.

Furthermore, each of the two sub-APIs comes in two variants, one
main function (such as @code{MHD_daemon_get_info_dynamic_sz()})
and associated macro (for example, @code{MHD_daemon_get_info_dynamic()})
that the application should actually use. The difference between
the macro and the function is that the function takes an extra
@code{size_t} argument that specifies the size of the
@code{union} used for the @var{output}. This argument allows MHD
to check that the application's output buffer is of sufficient
size, which could be important if the @code{union} changes between
MHD versions and thus the application possibly has been built with
a different @code{union} definition.  The macro simply runs
@code{sizeof()} on the @var{output} variable to provide the
size of the application's allocation. If the buffer is too small
for the requested information, MHD fails safe and returns
@code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}.  In the next subsections
we will thus only present the macros that applications should
use, and skip over the @code{_sz()}-functions with the extra
@code{size_t} argument.

@menu
* libmicrohttpd2-info library::     State information about the library
* libmicrohttpd2-info daemon::      State information about a daemon
* libmicrohttpd2-info connection::  State information about a connection
* libmicrohttpd2-info session::     State information about a session
* libmicrohttpd2-info request::     State information about a request
@end menu

@node libmicrohttpd2-info library
@section Obtaining state information about the MHD library

This introspection API returns information about how the MHD
library was compiled. It is primarily useful for applications
that want to double-check that particular features are available.

@anchor{MHD_lib_get_info_fixed}
@deftypefun {enum MHD_StatusCode} MHD_lib_get_info_fixed (enum MHD_LibInfoFixed info_type, union MHD_LibInfoFixedData *output_buf)

Obtain information about the MHD library.

@table @var
@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun

@anchor{MHD_LibInfoFixed}
@deftp {Enumeration} MHD_LibInfoFixed
Values of this enum are used to specify what
dynamic information about a daemon is desired.

@table @code

@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@anchor{MHD_LibInfoFixedData}
@deftp {C Union} MHD_LibInfoFixedData
Represents introspection data that can be returned about the
MHD library. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp



@anchor{MHD_lib_get_info_dynamic}
@deftypefun {enum MHD_StatusCode} MHD_lib_get_info_dynamic (enum MHD_LibInfoDynamicType info_type, union MHD_LibInfoDynamicData *output_buf)

Obtain information about the MHD library.
This information may change after the start of MHD.

@table @var
@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@anchor{MHD_LibInfoDynamic}
@deftp {Enumeration} MHD_LibInfoDynamic
Values of this enum are used to specify what
dynamic information about a lib is desired.

@table @code
@anchor{MHD_LIB_INFO_FIXED_HAS_AGGREGATE_FD}
@item MHD_LIB_INFO_FIXED_HAS_AGGREGATE_FD
Check if aggregate file-descriptor support is available
for this build of the libarary.

@anchor{MHD_LIB_INFO_FIXED_HAS_UPGRADE}
@item MHD_LIB_INFO_FIXED_HAS_UPGRADE
Check if HTTP upgrade support is available
for this build of the libarary.

@anchor{MHD_LIB_INFO_FIXED_HAS_AUTH_BASIC}
@item MHD_LIB_INFO_FIXED_HAS_AUTH_BASIC
Check if HTTP basic authentication support is available
for this build of the libarary.

@anchor{MHD_LIB_INFO_FIXED_HAS_AUTH_DIGEST}
@item MHD_LIB_INFO_FIXED_HAS_AUTH_DIGEST
Check if HTTP digest authentication support is available
for this build of the libarary.

@anchor{MHD_LIB_INFO_FIXED_HAS_LARGE_FILE}
@item MHD_LIB_INFO_FIXED_HAS_LARGE_FILE
Check if MHD was build with support for "large" files
(above 2 Gigabytes). This should be the case on all modern
platforms, but may not be the case on some embedded or legacy
systems.

@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@deftp {C Union} MHD_LibInfoDynamicData
Represents introspection data that can be returned about a
@var{lib}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp



@node libmicrohttpd2-info daemon
@section Obtaining state information about a daemon

This introspection API returns information about a daemon.

@anchor{MHD_daemon_get_info_fixed}
@deftypefun {enum MHD_StatusCode} MHD_daemon_get_info_fixed (struct MHD_Daemon *daemon, enum MHD_DaemonInfoFixed info_type, union MHD_DaemonInfoFixedData *output_buf)

Obtain information about the MHD @var{daemon}.
The information returned is not changed at after the start
of the daemon until the daemon is destroyed.

@table @var
@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@c FIXME: can we really have TOO_EARLY/TOO_LATE for *fixed* data?
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@anchor{MHD_DaemonInfoFixed}
@deftp {Enumeration} MHD_DaemonInfoFixed
Values of this enum are used to specify what
dynamic information about a @var{daemon} is desired.

@table @code
@anchor{MHD_DAEMON_INFO_FIXED_AGGREGATE_FD}
@item MHD_DAEMON_INFO_FIXED_AGGREGATE_FD
Aggregate file-descriptor applications should use when
using an external event loop in combination with
@ref{MHD_D_OPTION_WM_EXTERNAL_SINGLE_FD_WATCH}.

@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@anchor{MHD_DaemonInfoFixedData}
@deftp {C Union} MHD_DaemonInfoFixedData
Represents introspection data that can be returned about a
@var{daemon}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var

@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp

@anchor{MHD_daemon_get_info_dynamic}
@deftypefun {enum MHD_StatusCode} MHD_daemon_get_info_dynamic (struct MHD_Daemon *daemon, enum MHD_DaemonInfoDynamicType info_type, union MHD_DaemonInfoDynamicData *output_buf)

Obtain information about the given @var{daemon}.
This information may change after the start of the @var{daemon}.

@table @var
@item daemon
the daemon about which information is desired;

@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@c FIXME: inconsistent suffix "Type" not used for others!
@anchor{MHD_DaemonInfoDynamicType}
@deftp {Enumeration} MHD_DaemonInfoDynamicType
Values of this enum are used to specify what
dynamic information about a daemon is desired.

@table @code
@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@anchor{MHD_DaemonInfoDynamicData}
@deftp {C Union} MHD_DaemonInfoDynamicData
Represents introspection data that can be returned about a
@var{daemon}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp


@node libmicrohttpd2-info connection
@section Obtaining state information about a connection

This introspection API returns information about a connection.


@anchor{MHD_connection_get_info_fixed}
@deftypefun {enum MHD_StatusCode} MHD_connection_get_info_fixed (struct MHD_Connection *connection, enum MHD_ConnectionInfoFixed info_type, union MHD_ConnectionInfoFixedData *output_buf)

Obtain information about the MHD @var{connection}.
The information returned is not changed at after the start
of the connection until the connection is destroyed.

@table @var
@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@c FIXME: can we really have TOO_EARLY/TOO_LATE for *fixed* data?
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@anchor{MHD_ConnectionInfoFixed}
@deftp {Enumeration} MHD_ConnectionInfoFixed
Values of this enum are used to specify what
dynamic information about a @var{connection} is desired.

@table @code
@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@anchor{MHD_ConnectionInfoFixedData}
@deftp {C Union} MHD_ConnectionInfoFixedData
Represents introspection data that can be returned about a
@var{connection}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp


@anchor{MHD_connection_get_info_dynamic}
@deftypefun {enum MHD_StatusCode} MHD_connection_get_info_dynamic (struct MHD_Connection *connection, enum MHD_ConnectionInfoDynamicType info_type, union MHD_ConnectionInfoDynamicData *output_buf)

Obtain information about the given @var{connection}.
This information may change after the start of the @var{connection}.

@table @var
@item connection
the connection about which information is desired;

@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@c FIXME: inconsistent suffix "Type" not used for others!
@deftp {Enumeration} MHD_ConnectionInfoDynamicType
Values of this enum are used to specify what
dynamic information about a connection is desired.

@table @code
@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@deftp {C Union} MHD_ConnectionInfoDynamicData
Represents introspection data that can be returned about a
@var{connection}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp



@node libmicrohttpd2-info session
@section Obtaining state information about a session


This introspection API returns information about a session.

@deftypefun {enum MHD_StatusCode} MHD_session_get_info_fixed (struct MHD_Session *session, enum MHD_SessionInfoFixed info_type, union MHD_SessionInfoFixedData *output_buf)

Obtain information about the MHD @var{session}.
The information returned is not changed at after the start
of the session until the session is destroyed.

@table @var
@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@c FIXME: can we really have TOO_EARLY/TOO_LATE for *fixed* data?
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@deftp {Enumeration} MHD_SessionInfoFixed
Values of this enum are used to specify what
dynamic information about a @var{session} is desired.

@table @code
@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@deftp {C Union} MHD_SessionInfoFixedData
Represents introspection data that can be returned about a
@var{session}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp


@deftypefun {enum MHD_StatusCode} MHD_session_get_info_dynamic (struct MHD_Session *session, enum MHD_SessionInfoDynamicType info_type, union MHD_SessionInfoDynamicData *output_buf)

Obtain information about the given @var{session}.
This information may change after the start of the @var{session}.

@table @var
@item session
the session about which information is desired;

@item info_type
the type of information that is desired

@item output_buf
where to write the desired information (which member is set depends on @var{info_type})
@end table

Returns a status code:
@itemize
@item @code{MHD_SC_OK} on success,
@item @code{MHD_SC_TOO_EARLY} if the information is not yet available,
  usually because the respective stage in processing was not yet reached,
@item @code{MHD_SC_TOO_LATE} if the information is no longer available,
  usually because processing is past the stage where the information is kept,
@item @code{MHD_SC_INFO_GET_TYPE_UNKNOWN} if @var{info_type} is unknown,
@item @code{MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE}
  if the requested information is not available for this specific
  object due to how that object was configured
@item @code{MHD_SC_INFO_GET_TYPE_UNOBTAINABLE}
  if the requested information should be available
  but cannot be provided due to some error or other reason,
@item @code{MHD_SC_INFO_GET_BUFF_TOO_SMALL}
  if @var{output_buf} is too small for this request
@end itemize
The list is not exhaustive, other error codes may be added in the future.
@end deftypefun


@c FIXME: inconsistent suffix "Type" not used for others!
@anchor{MHD_SessionInfoDynamicType}
@deftp {Enumeration} MHD_SessionInfoDynamicType
Values of this enum are used to specify what
dynamic information about a session is desired.

@table @code
@item FIXME_CONSTANT_NAME
FIXME_CONSTANT_DESCRIPTION

@end table
@end deftp


@anchor{MHD_SessionInfoDynamicData}
@deftp {C Union} MHD_SessionInfoDynamicData
Represents introspection data that can be returned about a
@var{session}. Which member will be set on success depends on the value
given for @var{info_type}.
@table @var
@item @code{FIXME_DATATYPE} FIXME_name
FIXME_DESCRIPTION.

@end table
@end deftp


@node libmicrohttpd2-info request
@section Obtaining state information about a request

Introspection of requests is described in
@ref{libmicrohttpd-request-status-info}.
