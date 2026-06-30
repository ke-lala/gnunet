@noindent

GNU libmicrohttpd (MHD) is a small HTTP daemon library implementing
the server-side functionality of the HTTP protocol.  MHD is an
official GNU package and part of the GNU project, bringing you
Free/Libre Software since 1983. MHD is released under the GNU
Lesser General Public License (LGPLv3 or later)  and gives you
the freedoms to run, copy, distribute, study, change and improve
the software for yourself and others. MHD without TLS support
is also dual-licensed under eCos license (the GPLv3+ with the
eCOS exception).

Many of MHD's features are optional and can be turned off using
compiler flags. Features include:

@itemize @bullet
@item Supports HTTP 1.0, HTTP/1.1 and (soon) HTTP2
@item Basic and digest authentication
@item TLS via OpenSSL or libgnutls (mbedTLS planned)
@item Websockets
@item Different threading modes (select, poll, epoll, thread-per-connection, thread-pool)
@item Stream processing (arbitrary size uploads)
@item Chunked encoding, HTTP pipelining
@item Limiting connections per client IP address
@item Use of @code{sendfile()}
@end itemize

This manual is for libmicrohttpd2, a major new release of the
library. It is still not battle-tested (compared to the previous)
version, and we would be excited to hear back from you about
real-world use!  The previous implementation of MHD is used in a wide
range of implementations.  Examples based on reports we've received
from developers include:

@itemize
@item Embedded HTTP server on a cortex M3 (128 KB code space)
@item Large-scale multimedia server (reportedly serving at the
      simulator limit of 7.5 GB/s)
@item Administrative console (via HTTP/HTTPS) for network appliances
@item Streaming Earth observation data from NASA to the Internet
@end itemize

The remainder of this chapter will give a high-level overview of
the library.

@menu
* libmicrohttpd-introduction-limitations::  Library limitations.
* libmicrohttpd-introduction-threadmodes::  Thread modes and event loops.
* libmicrohttpd-introduction-compiling::    Compiling libmicrohttpd.
* libmicrohttpd-introduction-pointers::     Using MHD data.
* libmicrohttpd-introduction-sigpipe::      SIGPIPE.
* libmicrohttpd-introduction-w32::          Portability to W32.
* libmicrohttpd-introduction-zos::          Portability to z/OS.
@end menu


@node libmicrohttpd-introduction-limitations
@section Limitations

MHD is supposed to handle everything that it must handle (because the
API would not allow clients to do this), such as basic connection
management. However, some detailed interpretations of HTTP headers, such as
range requests, are deliberately left to the main application.  In particular,
if an application developer wants to support range requests, he needs
to explicitly indicate support in responses and also explicitly parse
the range header and generate a suitable response.
@ref{libmicrohttpd2-requests,,Accessing client request data} explains how applications can learn
everything about incoming HTTP requests.

MHD does understands HTTP headers that control connection management
(specifically, @code{Connection: close} and @code{Expect: 100
continue} are understood and handled automatically).
@code{Connection: upgrade} is supported by passing control over
the communication to the application (after sending the HTTP
upgrade response header).
@ref{libmicrohttpd-upgrade,Upgrading HTTP connections} explains how to implement connection
upgrades (for example, to implement WebSockets).

MHD largely leaves implementing the semantics of the different HTTP
methods to the application.  One exception is that MHD does understand
@code{HEAD} and will only send the headers of the response and not the
body, even if the client supplied a body.  (In fact, applications do
need to construct a response with the correct length, even for
@code{HEAD} request.)

MHD understands @code{POST} data and is able to decode certain formats
(@code{application/x-www-form-urlencoded}, @code{multipart/form-data}
and @code{text/plain}) using the post processor API.  The data stream
of a POST can also be provided directly to the main application, thus
unsupported encodings could still be processed using custom
application logic.  @ref{libmicrohttpd2-postprocessor,,Parsing POST
data} explains MHD functions for dealing with POST data in more
detail.

The @code{microhttp2.h} header file defines various constants used by
the HTTP protocol.  This does not mean that MHD actually interprets
all of these values.  The provided constants are exported as a
convenience for users of the library.  MHD does not require that all
transmitted HTTP headers are part of the standard specification;
applications are free to define their own custom headers and use those
with MHD.  Even setting standard headers incorrectly (for example, to test
clients) is possible via the use of options to override sanity checks.

All symbols defined in the public API start with @code{MHD_}.  All
functions are guaranteed to be completely reentrant and thread-safe.
MHD checks for allocation failures and tries to recover gracefully
(for example, by closing the connection).  Additionally, clients can
specify resource limits on the overall number of connections, number
of connections per IP address and memory used per connection to avoid
resource exhaustion.  @ref{libmicrohttpd2-doptions,,Configuring your
HTTP daemon} explains the MHD API for configuring the various settings
supported for the daemon.


@node libmicrohttpd-introduction-threadmodes
@section Thread modes and event loops
@cindex poll
@cindex epoll
@cindex mode
@cindex threads
@cindex select

MHD supports four basic thread modes and up to three event loop
styles.

The four basic thread modes are external sockets polling (MHD creates
no threads, event loop is fully managed by the application), internal
polling (MHD creates one thread for all connections), polling in
thread pool (MHD creates a thread pool which is used to process all
connections) and thread-per-connection (MHD creates one thread for
listen sockets and then one thread per accepted connection).

These thread modes are then combined with the evet loop styles
(polling function type).  MHD supports @code{select}, @code{poll} and
@code{epoll}. @code{select} is available on all platforms,
@code{epoll} and @code{poll} may not be available on some platforms.
Note that it is possible to combine MHD using @code{epoll} with an
external @code{select}-based event loop.

The default (if no other option is passed) is ``external periodic''
where the application must periodically call on MHD to give it a
chance to do its work.  For simple applications, the recommended mode
is an external event loop where MHD tells the application which events
to watch for and when the application needs to call the library.  The
highest performance can typically be obtained with a thread pool using
@code{epoll}.

@c Apache Benchmark (ab) was used to compare the
@c performance of @code{select} and @code{epoll} when using a thread pool
@c and a large number of connections.

Not all combinations of thread modes and event loop styles are
supported.  This is partially to keep the API simple, and partially
because some combinations simply make no sense as others are strictly
superior.  Note that the choice of style depends first of all on the
application logic, and then on the performance requirements.
Applications that perform a blocking operation while handling a
request within the callbacks from MHD must use a thread per
connection.  This is typically rather costly.  Applications that do
not support threads or that must run on embedded devices without
thread-support must use the external mode.  Using @code{epoll} is only
supported by some platforms, thus portable applications must at least
have a fallback option available.  @ref{tbl:supported} lists the sane
combinations.

@float Table,tbl:supported
@multitable {@b{thread-per-connection}} {@b{select}} {@b{poll}} {@b{epoll}}
@item                           @tab @b{select} @tab @b{poll} @tab @b{epoll}
@item @b{external}              @tab  yes       @tab yes      @tab yes
@item @b{internal}              @tab  yes       @tab yes      @tab yes
@item @b{thread pool}           @tab  yes       @tab yes      @tab yes
@item @b{thread-per-connection} @tab  no        @tab no       @tab no @c not yet implemented
@end multitable
@caption{Supported combinations of event styles and thread modes.}
@end float

In practice, you rarely need to worry about selecting the event
loop style: MHD offers the "automatic" setting, which will pick
the fastest mode available on your platform automatically.

@ref{libmicrohttpd2-external,,Using external event loops} for details
on how to use MHD with an external event loop.

@node libmicrohttpd-introduction-compiling
@section Compiling GNU libmicrohttpd
@cindex compilation
@cindex embedded systems
@cindex portability

MHD uses the standard GNU system where the usual build process
involves running:

@verbatim
$ ./configure
$ make
$ make install
@end verbatim

In terms of dependencies, MHD can optionally use OpenSSL or
libgnutls for TLS support.  MHD has no hard dependencies
other than libc, and tries to impose minimal requirements
on the C standard library as well.

MHD supports various options to be given to configure to tailor the
binary to a specific situation.  Note that some of these options will
remove portions of the MHD code that are required for
binary-compatibility.  They should only be used on embedded systems
with tight resource constraints and no concerns about library
versioning.  Standard distributions including MHD are expected to
always ship with all features enabled and with at least one TLS
backend, otherwise unexpected incompatibilities can arise. In
particular, the libtool versioning supported by MHD developers
assumes that you enable all of the optional features.

Below is a list of MHD-specific options that can be given to
configure:@footnote{Canonical configure options such
as @code{--prefix} are also supported; for a
full list of options run @code{./configure --help}}

@c FIXME: check with current configure options!
@table @code
@item ``--disable-curl''
disable running testcases using libcurl

@item ``--disable-largefile''
disable support for 64-bit files

@item ``--disable-messages''
disable logging of error messages (smaller binary size, not so much fun for debugging)

@item ``--disable-https''
disable HTTPS support, even if GNUtls is found; this option must be used if eCOS license is desired as an option (in all cases the resulting binary falls under a GNU LGPL-only license)

@item ``--disable-postprocessor''
do not include the post processor API (results in binary incompatibility)

@item ``--disable-dauth''
do not include the authentication APIs (results in binary incompatibility)

@item ``--disable-httpupgrade''
do not build code for HTTP ``Upgrade'' (smaller binary size, binary incompatible library)

@item ``--disable-epoll''
do not include epoll support, even if it supported (minimally smaller binary size, good for portability testing)

@item ``--enable-coverage''
set flags for analysis of code-coverage with gcc/gcov (results in slow, large binaries)

@item ``--with-threads=posix,w32,none,auto''
sets threading library to use. With use ``none'' to not support threads. In this case, MHD will only support the ``external'' threading modes and not perform any locking of data structures! Use @code{MHD_is_feature_supported(MHD_FEATURE_THREADS)} to test if threads are available. Default is ``auto''.

@item ``--with-gnutls=PATH''
specifies path to libgnutls installation

@end table


@node libmicrohttpd-introduction-pointers
@section Applications using data provided by MHD

@cindex lifetime
@subsection Data lifetime

MHD will give applications access to its internal data structures via
arguments and return values pointing to memory managed by MHD.  This
creates the question as to how long those pointers are assured to stay
valid.

Most MHD data structures are associated with an HTTP session,
connection, request or response.  Thus, pointers associated with a
session, connection, request or response are typically valid until the
respective session, connection, request or response are completed.
This is in particular true for all values obtained via the
introspection API.
@ref{libmicrohttpd2-introspection,,introspection APIs} has more
details on the introspection API.

Otherwise, values can generally only be assumed to be valid during the
callback where MHD provided a value to the application. Finally,
unless stated otherwise, the application does not need to guarantee
that values it passes into MHD APIs remain valid after the call to an
MHD API returns.

@cindex string
@subsection Strings

MHD will frequently make strings available to applications.
In this case, MHD will return a @code{struct MHD_String} or
a @code{struct MHD_StringNullable}.

@anchor{MHD_String}
@deftp {C Struct} MHD_String len cstr
Represents a string, the pointer is never @code{NULL}.
@table @var
@item @code{size_t} len
Number of characters in @var{str}, not counting 0-termination;

@item @code{const char *} cstr
0-terminated C-string. Guaranteed to not be @code{NULL}.
@end table
@end deftp

@anchor{MHD_StringNullable}
@deftp {C Struct} MHD_StringNullable len cstr
Represents a string that could also be @code{NULL}.
@table @var
@item @code{size_t} len
Number of characters in @var{str}, not counting 0-termination;

@c FIXME: really? or is the pointer to the Nullable NULL!?
@item @code{const char *} cstr
0-terminated C-string. Could be @code{NULL} in some cases.
@end table
@end deftp

@cindex boolean
@subsection Booleans

@deftp {Enumeration} MHD_Bool
MHD's representation of a boolean choice.
Values include @code{MHD_NO} (0) and @code{MHD_YES} (1).
@end deftp


@node libmicrohttpd-introduction-sigpipe
@section SIGPIPE
@cindex signals

MHD does not install a signal handler for SIGPIPE.  On platforms where
this is possible (such as GNU/Linux), it disables SIGPIPE for its I/O
operations (by passing @code{MSG_NOSIGNAL} or similar flags).  On
platforms where this is not possible, SIGPIPE signals may be generated
from network operations by MHD. This will cause the process to die
unless the application explicitly installs a signal handler for SIGPIPE.

Hence portable code using MHD @strong{must} install a SIGPIPE handler
@emph{or} explicitly block the SIGPIPE signal.  MHD does not do so to
avoid messing with other parts of the application that may need to
handle SIGPIPE in a particular way.  You can make your application
handle SIGPIPE by calling the following function in @code{main()}:

@example
@verbatiminclude examples/sigpipe.c
@end example


@section Portability to Android

To cross-compile MHD for Android, install the Android NDK and use:
@verbatim
$ ./configure \
    --target=arm-linux-androideabi \
    --host=arm-linux-androideabi \
    --disable-doc \
    --disable-examples
$ make
@end verbatim

Similar build commands should work for cross-compilation to other platforms.
Note that you may have to first cross-compile the respective TLS library
to get MHD with TLS support.

@node libmicrohttpd-introduction-w32
@section Portability to W32

MHD in general ported well to W32. Most MHD features are
supported. W32 do not support some functions, like @code{epoll} and
corresponding MHD features are thus not available on W32.

@node libmicrohttpd-introduction-zos
@section Portability to z/OS

It has been a long time since the developers had access to z/OS.
While in principle the code should work, we would appreciate
feedback from the z/OS community on the current version.



