/* MHD_config.h for W32 */
/* Created manually. */

/* *** Basic OS/compiler information *** */

/* This is a Windows system */
#define WINDOWS 1

#ifndef __clang__
/* Define that MS VC does not support VLAs */
#ifndef __STDC_NO_VLA__
#define __STDC_NO_VLA__ 1
#endif /* ! __STDC_NO_VLA__ */
#else
/* If clang is used then variable-length arrays are supported. */
#define HAVE_C_VARARRAYS 1
#endif


/* Define to '1' if your compiler supports 'array[static N]' with fixed N as
   function parameter */
#define HAVE_FUNC_PARAM_ARR_STATIC_FIXED 1

#ifdef __clang__
/* Define to '1' if your compiler supports 'array[static N]' with variable N
   as a function parameter */
#  define HAVE_FUNC_PARAM_ARR_STATIC_VAR 1
#endif /*  __clang__ */

/* Define to 1 if your compiler supports __func__ magic-macro. */
#define HAVE___FUNC__ 1

/* Define to 1 if your C compiler supports inline functions. */
#define INLINE_FUNC 1

/* Define to prefix which will be used with MHD static inline functions. */
#define mhd_static_inline static __forceinline

/* Compound literals are supported natively since VS2013, but only in C */
#if defined(__clang__) || \
  (defined(_MSC_VER) && _MSC_VER >= 1800 && ! defined(__cplusplus) && \
  (defined(_MSC_EXTENSIONS) || \
  ((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112))))
/* Define to '1' if your compiler supports compound literals */
#  define HAVE_COMPOUND_LITERALS 1

/* Define to '1' if your compiler supports compound literals as local arrays
   */
#  define HAVE_COMPOUND_LITERALS_ARRAYS_LOCAL 1

/* Define to '1' if your compiler supports compound literals as arrays of the
   scope */
#  define HAVE_COMPOUND_LITERALS_ARRAYS_SCOPE 1

/* Define to '1' if your compiler supports compound literals as lvalues */
#  define HAVE_COMPOUND_LITERALS_LVALUES 1
#endif

#ifdef __clang__
/* Define to 1 if you have __builtin_bswap32() builtin function */
#  define MHD_HAVE___BUILTIN_BSWAP32 1

/* Define to 1 if you have __builtin_bswap64() builtin function */
#  define MHD_HAVE___BUILTIN_BSWAP64 1
#endif /* __clang__ */

/* Define to '1' if your compiler supports variadic macros */
#define HAVE_MACRO_VARIADIC 1

/* Define to '1' if NULL pointers binary representation is all zero bits */
#define HAVE_NULL_PTR_ALL_ZEROS 1

/* Define to keyword supported to indicate unreachable code paths */
#ifdef __clang__
#  define MHD_UNREACHABLE_KEYWORD __builtin_unreachable ()
#elif defined(_MSC_VER)
#  define MHD_UNREACHABLE_KEYWORD __assume (0)
#endif

#if defined(__clang__) || \
  (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901)
/* Define to '1' if your compiler supports 'restrict' keyword */
#  define HAVE_RESTRICT 1
#elif defined(_MSC_VER) && _MSC_VER >= 1400 && defined(_MSC_EXTENSIONS)
#  define restrict __restrict
/* Define to '1' if your compiler supports 'restrict' keyword */
#  define HAVE_RESTRICT 1
#endif

#if ! defined(HAVE_RESTRICT)
#  define restrict /* empty */
#endif

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703) \
  || (defined(__cplusplus) && __cplusplus >= 201703)
/* Define to keyword marking intentional missing 'break' at the end of 'case:'
   */
#  define mhd_FALLTHROUGH [[fallthrough]]
#elif defined(__has_attribute)
#  if __has_attribute (__fallthrough__)
/* Define to keyword marking intentional missing 'break' at the end of 'case:'
   */
#    define mhd_FALLTHROUGH __attribute__((__fallthrough__))
#  endif
#endif

#if ! defined(mhd_FALLTHROUGH)
/* Define to keyword marking intentional missing 'break' at the end of 'case:'
   This macro is actually just empty. */
#  define mhd_FALLTHROUGH /* Intentional fallthrough */
#endif

/* The size of 'char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of 'int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of 'int_fast64_t', as computed by sizeof. */
#define SIZEOF_INT_FAST64_T 8

/* The size of 'size_t', as computed by sizeof. */
#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64) || defined(_WIN64)
#define SIZEOF_SIZE_T 8
#else  /* ! _WIN64 */
#define SIZEOF_SIZE_T 4
#endif /* ! _WIN64 */

/* The size of `tv_sec' member of `struct timeval', as computed by sizeof */
#define SIZEOF_STRUCT_TIMEVAL_TV_SEC 4

/* The size of 'unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of 'unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* Define to supported 'noreturn' function declaration */
#if defined(_STDC_VERSION__) && (__STDC_VERSION__ + 0) >= 201112L
#define MHD_NORETURN_ _Noreturn
#else  /* before C11 */
#define MHD_NORETURN_ __declspec(noreturn)
#endif /* before C11 */

/* *** OS features *** */

/* Provides IPv6 headers */
#define HAVE_INET6 1

/* Define to '1' if you have the declaration of 'IPV6_V6ONLY' */
#define HAVE_DCLR_IPV6_V6ONLY 1

/* Define to 1 if your system allow overriding the value of FD_SETSIZE macro  */
#define HAS_FD_SETSIZE_OVERRIDABLE 1

#if 0 /* Do not define the macro to keep maintability simple if system value is updated */
/* Define to system default value of FD_SETSIZE macro */
#  define MHD_SYS_FD_SETSIZE_ 64
#endif

/* Define to use socketpair for inter-thread communication */
#define MHD_ITC_SOCKETPAIR_ 1

/* define to use W32 threads */
#define mhd_THREADS_KIND_W32 1

#ifndef _WIN32_WINNT
/* MHD supports Windows XP and later W32 systems*/
#define _WIN32_WINNT 0x0600
#endif /* _WIN32_WINNT */

/* winsock poll is available only on Vista and later */
#if _WIN32_WINNT >= 0x0600
#  define HAVE_POLL 1

/* Define to 1 if you have the 'WSAPoll' function. */
#  define HAVE_WSAPOLL 1

#endif /* _WIN32_WINNT >= 0x0600 */

/* Define to '1' if select() is supported on your platform */
#define HAVE_SELECT 1

/* Define to 1 if you have the `gmtime_s' function in W32 form. */
#define HAVE_W32_GMTIME_S 1

/* Define to 1 if you have the usable `calloc' function. */
#define HAVE_CALLOC 1

/* Define if you have usable assert() and assert.h */
#define HAVE_ASSERT 1

#if _MSC_VER >= 1900 /* snprintf() supported natively since VS2015 */
/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1
#endif

#if _MSC_VER + 0 >= 1800 /* VS 2013 and later */ || defined(__cplusplus)
#  if ! defined(__cplusplus) && \
  (! defined(__STDC_VERSION__) || (__STDC_VERSION__ + 0) < 202311)
/* Define to 1 if you have the <stdbool.h> header file and <stdbool.h> is
   required to use 'bool' type. */
#    define HAVE_STDBOOL_H 1
#  endif
/* Define to 1 if you have the boolean type that takes only 'true' and
   'false'. */
#  define HAVE_BUILTIN_TYPE_BOOL 1
#else  /* before VS 2013 */

/* Define to type name which will be used as boolean type. */
#define bool int

/* Define to value interpreted by compiler as boolean "false", if "false" is
   not defined by system headers. */
#define false 0

/* Define to value interpreted by compiler as boolean "true", if "true" is not
   defined by system headers. */
#define true (! 0)
#endif /* before VS 2013 */

/* Define if you have usable `getsockname' function. */
#define MHD_USE_GETSOCKNAME 1

#if _MSC_VER + 0 >= 1900 /* VS 2015 and later */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ + 0) >= 201112L
/* Define to 1 if your compiler supports 'alignof()' */
#define HAVE_C_ALIGNOF 1
/* Define to 1 if you have the <stdalign.h> header file. */
#define HAVE_STDALIGN_H 1
#endif /* C11 */
#endif /* VS 2015 and later */

/* Define to '1' if you have the declaration of 'AF_UNIX' */
#define HAVE_DCLR_AF_UNIX 1

/* Define to 1 if you have the 'nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Define to 1 if the system has the type 'ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if the system has the type 'size_t'. */
#define HAVE_SIZE_T 1

/* Define to 1 if you have the 'timespec_get' function. */
#define HAVE_TIMESPEC_GET 1

/* Define to 1 if the system has the type 'uint8_t'. */
#define HAVE_UINT8_T 1

/* Define to 1 if the system has the type 'uintptr_t'. */
#define HAVE_UINTPTR_T 1

/* *** Headers information *** */

#if _MSC_VER >= 1800
/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#define HAVE_WINSOCK2_H 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
#define HAVE_WS2TCPIP_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H   1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H       1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <sdkddkver.h> header file. */
#define HAVE_SDKDDKVER_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* *** Declarations in headers *** */

#ifdef HAVE_WSAPOLL
/* Define to '1' if you have the declaration of 'POLLIN' */
#  define HAVE_DCLR_POLLIN 1

/* Define to '1' if you have the declaration of 'POLLOUT' */
#  define HAVE_DCLR_POLLOUT 1

/* Define to '1' if you have the declaration of 'POLLPRI' */
#  define HAVE_DCLR_POLLPRI 1

/* Define to '1' if you have the declaration of 'POLLRDBAND' */
#  define HAVE_DCLR_POLLRDBAND 1

/* Define to '1' if you have the declaration of 'POLLRDNORM' */
#  define HAVE_DCLR_POLLRDNORM 1

/* Define to '1' if you have the declaration of 'POLLWRBAND' */
#  define HAVE_DCLR_POLLWRBAND 1

/* Define to '1' if you have the declaration of 'POLLWRNORM' */
#  define HAVE_DCLR_POLLWRNORM 1
#endif /* HAVE_WSAPOLL */

/* Define to '1' if you have the declaration of 'SD_SEND' */
#define HAVE_DCLR_SD_SEND 1

/* Define to '1' if you have the declaration of 'SOL_SOCKET' */
#define HAVE_DCLR_SOL_SOCKET 1

/* Define to '1' if you have the declaration of 'SOMAXCONN' */
#define HAVE_DCLR_SOMAXCONN 1

/* Define to '1' if you have the declaration of 'SO_LINGER' */
#define HAVE_DCLR_SO_LINGER 1

/* Define to '1' if you have the declaration of 'SO_REUSEADDR' */
#define HAVE_DCLR_SO_REUSEADDR 1

/* Define to '1' if you have the declaration of 'TCP_FASTOPEN' */
/* #define HAVE_DCLR_TCP_FASTOPEN 1 */ /* Should be autodetected in header */

/* Define to '1' if you have the declaration of 'TCP_NODELAY' */
#define HAVE_DCLR_TCP_NODELAY 1


/* *** MHD configuration *** */
/* Undef to disable feature */

/* Define to '1' to enable use of select() system call */
#define MHD_SUPPORT_SELECT 1

#ifdef HAVE_WSAPOLL
/* Define to '1' to enable use of select() system call */
#  define MHD_SUPPORT_POLL 1
#endif

/* Define to 1 if libmicrohttpd is compiled with HTTP cookie parsing support.
   */
#define MHD_SUPPORT_COOKIES 1

/* The default HTTP Digest Auth default maximum nc (nonce count) value */
#define MHD_AUTH_DIGEST_DEF_MAX_NC 1000

/* The default HTTP Digest Auth default nonce timeout value (in seconds) */
#define MHD_AUTH_DIGEST_DEF_TIMEOUT 90

/* Define to '1' to enable internal logging and log messages. */
#define MHD_SUPPORT_LOG_FUNCTIONALITY 1

/* Define to '1' to enable verbose text bodies for automatic HTTP replies. */
#define MHD_ENABLE_AUTO_MESSAGES_BODIES 1

/* Define to '1' if libmicrohttpd should be compiled with Basic Auth support.
   */
#define MHD_SUPPORT_AUTH_BASIC 1

/* Define to 1 if libmicrohttpd is compiled with Digest Auth support. */
#define MHD_SUPPORT_AUTH_DIGEST 1

/* Define to 1 if libmicrohttpd is compiled with MD5 hashing support. */
#define MHD_SUPPORT_MD5 1

/* Define to 1 if libmicrohttpd is compiled with SHA-256 hashing support. */
#define MHD_SUPPORT_SHA256 1

/* Define to 1 if libmicrohttpd is compiled with SHA-512/256 hashing support.
   */
#define MHD_SUPPORT_SHA512_256 1

/* Define to 1 if libmicrohttpd is compiled with POST parser support. */
#define MHD_SUPPORT_POST_PARSER 1

/* Define to 1 if libmicrohttpd is compiled with HTTP Upgrade support. */
#define MHD_SUPPORT_UPGRADE 1

/* Define to 1 if libmicrohttpd is compiled with HTTP cookie parsing support.
   */
#define MHD_SUPPORT_COOKIES 1


/* *** Other useful staff *** */

#define _GNU_SOURCE  1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1


/* End of mhd_config.h */
