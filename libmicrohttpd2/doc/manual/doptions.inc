@anchor{MHD_DaemonOptionAndValue}
@cindex option

This chapter is about configuring an MHD daemon.  MHD daemon
configuration options are represented using a @code{struct
MHD_DameonOptionAndValue}.  However, applications should never be
concerned with the internals of this data structure, and only use the
provided API functions and macros explained in this chapter to
construct these options.

@node libmicrohttpd-daemon-options-setting
@section Setting daemon options

MHD offers three main ways to set options for a @code{struct MHD_Daemon}:

@itemize
@item @code{MHD_daemon_set_options()} sets an array of options,
@item @code{MHD_daemon_set_option()} sets an individual option, and
@item @code{MHD_DAEMON_SET_OPTIONS()} sets a list of options.
@end itemize

All three approaches can be combined for the same daemon
to set various options. We will now look at each of them.

@code{MHD_daemon_set_options()} is useful if the application needs
complex code to first initialize a set of options, or if it has a
static data buffer with all of the options (say in ROM). It is also
the underlying function used to implement the other styles.

@anchor{MHD_daemon_set_options}
@deftypefun {struct MHD_StatusCode} MHD_daemon_set_options (struct MHD_Daemon *daemon, const struct MHD_DaemonOptionAndValue *options, size_t options_max_num)

@table @var
@item daemon
the daemon to configure

@item options
array of options to set, either @code{MHD_D_OPTION_TERIMINATE()}-terminated or
limited by @var{options_max_num} (whatever comes first).

@item options_max_num
maximum number of options to process from the @var{options} array,
or @code{MHD_OPTIONS_ARRAY_MAX_SIZE} to process all options from
@var{options} until the @code{MHD_D_OPTION_TERMINATE()}-terminator.

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code of
the first option that could not be set.
@end deftypefun

To set only a single option, MHD defines a simple convenience API with
@code{MHD_daemon_set_option()}. This is useful if there is only one
option to be set, or if error handling for that specific option is
somehow special (like not failing when setting this particular option
failed).

@anchor{MHD_DAEMON_set_option}
@deftypefun {struct MHD_StatusCode} MHD_daemon_set_option (struct MHD_Daemon *daemon, const struct MHD_DaemonOptionAndValue *option)

@table @var
@item daemon
the daemon to configure

@item option
the option to set

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code from
trying to set the @var{option}.
@end deftypefun


An easy way to set a bunch of options without having to worry
about explicitly allocating an array or checking each return
value is to use @code{MHD_DAEMON_SET_OPTIONS()}.

@anchor{MHD_DAEMON_SET_OPTIONS}
@deftypefun {struct MHD_StatusCode} MHD_DAEMON_SET_OPTIONS (struct MHD_Daemon *daemon, ...)

@table @var
@item daemon
the daemon to configure

@item ...
variable-length list of @code{MHD_DaemonOptionAndValue} options
to set; does @emph{not} need to be terminated via
@code{MHD_D_OPTION_TERMINATE()} as such a terminator is added
automatically.

@end table

Returns @code{MHD_SC_OK} on success, otherwise the error code of
the first option that could not be set.
@end deftypefun

The following example illustrates how to set daemon options:

@example
@verbatiminclude examples/options-example.c
@end example

The remaining sections of this chapter discuss the various
daemon options available to applications today.

@include manual/daemon_options.inc
