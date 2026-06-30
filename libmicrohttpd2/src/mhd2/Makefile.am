# This Makefile.am is in the public domain

AM_CPPFLAGS = \
  -I$(srcdir)/../incl_priv \
  -I$(srcdir)/../include \
  $(CPPFLAGS_ac)

AM_CFLAGS = $(CFLAGS_ac)

if USE_COVERAGE
  AM_CFLAGS += --coverage
endif

AM_LDFLAGS = $(LDFLAGS_ac)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

lib_LTLIBRARIES = \
  libmicrohttpd2.la

noinst_DATA =
MOSTLYCLEANFILES =

nodist_libmicrohttpd2_la_SOURCES = $(CONFIG_HEADER)
libmicrohttpd2_la_SOURCES = \
  $(srcdir)/../incl_priv/autoinit_funcs.h \
  $(srcdir)/../incl_priv/mhd_sys_options.h \
  sys_offsetof.h \
  sys_sizet_type.h          sys_null_macro.h          sys_bool_type.h \
  sys_base_types.h \
  sys_sockets_types.h       sys_sockets_headers.h     sys_ip_headers.h \
  sys_errno.h               sys_file_fd.h             sys_malloc.h \
  sys_select.h              sys_poll.h \
  sys_sendfile.h \
  compat_calloc.h \
  sys_w32_ver.h \
  mhd_align.h               mhd_bithelpers.h          mhd_byteorder.h \
  mhd_assert.h              mhd_assume.h              mhd_unreachable.h \
  mhd_constexpr.h           mhd_predict.h \
  mhd_cntnr_ptr.h           mhd_arr_num_elems.h \
  mhd_tristate.h            mhd_status_code_int.h \
  mhd_socket_type.h         mhd_sockets_macros.h \
  mhd_sockets_funcs.c       mhd_sockets_funcs.h \
  mhd_socket_error_funcs.c  mhd_socket_error_funcs.h  mhd_socket_error.h \
  mhd_atomic_counter.c      mhd_atomic_counter.h \
  mhd_bool.h \
  mhd_str.c                 mhd_str.h \
  mhd_rng.c                 mhd_rng.h \
  mhd_str_macros.h          mhd_str_types.h \
  mhd_buffer.h \
  mhd_comm_layer_state.h \
  mhd_limits.h \
  mhd_iovec.h \
  mhd_dbg_print.h \
  mhd_panic.c               mhd_panic.h \
  mhd_lib_init.c            mhd_lib_init_impl.h       mhd_lib_init.h \
  mhd_lib_init_auto.h \
  lib_get_info.c \
  mhd_dlinked_list.h \
  mhd_conn_socket.h         mhd_connection.h \
  mhd_stream.h \
  mhd_locks.h \
  mhd_itc.c                 mhd_itc.h                 mhd_itc_types.h \
  mhd_mono_clock.c          mhd_mono_clock.h \
  mempool_funcs.c           mempool_funcs.h           mempool_types.h \
  mhd_read_file.c           mhd_read_file.h \
  sckt_recv.c               sckt_recv.h \
  mhd_recv.c                mhd_recv.h \
  mhd_send.c                mhd_send.h                sckt_send.h \
  mhd_daemon.h \
  mhd_public_api.h \
  mhd_request.h             mhd_reply.h               mhd_response.h \
  http_method.h             http_prot_ver.h \
  http_status_str.c         http_status_str.h \
  mhd_enum_to_str.c \
  action.c                  mhd_action.h \
  dcc_action.c              mhd_dcc_action.h \
  events_process.c          events_process.h \
  daemon_logger.c           daemon_logger.h \
  daemon_logger_default.c   daemon_logger_default.h \
  daemon_options.h          daemon_set_options.c \
  daemon_create.c \
  daemon_start.c \
  daemon_get_info.c \
  daemon_add_conn.c         daemon_add_conn.h \
  daemon_funcs.c            daemon_funcs.h \
  daemon_event_update.c     extr_events_funcs.c       extr_events_funcs.h \
  conn_timeout.c            conn_timeout.h \
  conn_data_process.c       conn_data_process.h \
  conn_data_recv.c          conn_data_recv.h \
  conn_data_send.c          conn_data_send.h \
  conn_mark_ready.h \
  conn_get_info.c \
  request_funcs.c           request_funcs.h \
  request_get_value.c       request_get_value.h \
  request_resume.c \
  respond_with_error.c      respond_with_error.h \
  request_get_info.c \
  response_from.c           response_from.h \
  response_add_header.c     response_add_header.h \
  response_destroy.c        response_destroy.h \
  response_funcs.c          response_funcs.h \
  response_set_options.c    response_set_options.h    response_options.h \
  stream_funcs.c            stream_funcs.h \
  stream_process_states.c   stream_process_states.h \
  stream_process_request.c  stream_process_request.h \
  stream_process_reply.c    stream_process_reply.h \
  stream_get_info.c

if ! HAVE_SYS_CALLOC
libmicrohttpd2_la_SOURCES += \
  compat_calloc.c
endif

#
httptwo_OPTSOURCES = \
  mhd_http_layer_state.h \
  h2/h2_bit_masks.h \
  h2/h2_req_data.h \
  h2/h2_stream_data.h \
  h2/h2_resp_data.h \
  h2/h2_conn_data.h         h2/h2_err_codes.h         h2/h2_frame_types.h \
  h2/h2_frame_length.h      h2/h2_frame_init.h \
  h2/h2_frame_codec.c       h2/h2_frame_codec.h \
  h2/h2_proc_settings.c     h2/h2_proc_settings.h     h2/h2_settings.h \
  h2/h2_req_item_kinds.h    h2/h2_req_item_struct.h \
  h2/h2_req_items_funcs.c   h2/h2_req_items_funcs.h \
  h2/h2_comm.c              h2/h2_comm.h \
  h2/h2_proc_conn.c         h2/h2_proc_conn.h \
  h2/h2_proc_in.c           h2/h2_proc_in.h \
  h2/h2_proc_out.c          h2/h2_proc_out.h \
  h2/h2_conn_streams.c      h2/h2_conn_streams.h \
  h2/h2_req_fields.c        h2/h2_req_fields.h \
  h2/h2_req_get_items.c     h2/h2_req_get_items.h \
  h2/h2_app_cb.c            h2/h2_app_cb.h \
  h2/h2_action.c            h2/h2_action.h \
  h2/h2_reply_funcs.c       h2/h2_reply_funcs.h \
  h2/hpack/h2_huffman_est.h \
  h2/hpack/h2_huffman_codec.c \
  h2/hpack/h2_huffman_codec.h \
  h2/hpack/mhd_hpack_dec_types.h \
  h2/hpack/mhd_hpack_enc_types.h \
  h2/hpack/mhd_hpack_codec.c  \
  h2/hpack/mhd_hpack_codec.h

threads_OPTSOURCES = \
  mhd_threads.c             mhd_threads.h             sys_thread_entry_type.h

post_parser_OPTSOURCES = \
  http_post_enc.h \
  mhd_post_parser.h         mhd_post_result.h         mhd_postfield_int.h \
  post_parser_funcs.c       post_parser_funcs.h

auth_OPTSOURCES = \
  request_auth_get.c        request_auth_get.h

auth_basic_OPTSOURCES = \
  auth_basic.c              auth_basic.h \
  response_auth_basic.c


if MHD_USE_MD5_INTR
md5_OPTSOURCES = \
  md5_int.c                 md5_int.h \
  mhd_md5.h
endif

if MHD_USE_MD5_GNUTLS
md5_OPTSOURCES = \
  md5_ext.h                 mhd_md5.h \
  md5_ext_gnutls.c
endif

if MHD_USE_MD5_OPENSSL
md5_OPTSOURCES = \
  md5_ext.h                 mhd_md5.h \
  md5_ext_openssl.c
endif

if MHD_USE_MD5_MBEDTLS
md5_OPTSOURCES = \
  md5_ext.h                 mhd_md5.h \
  md5_ext_mbedtls.c
endif


if MHD_USE_SHA256_INTR
sha256_OPTSOURCES = \
  sha256_int.c              sha256_int.h \
  mhd_sha256.h
endif

if MHD_USE_SHA256_GNUTLS
sha256_OPTSOURCES = \
  sha256_ext.h              mhd_sha256.h \
  sha256_ext_gnutls.c
endif

if MHD_USE_SHA256_OPENSSL
sha256_OPTSOURCES = \
  sha256_ext.h              mhd_sha256.h \
  sha256_ext_openssl.c
endif

if MHD_USE_SHA256_MBEDTLS
sha256_OPTSOURCES = \
  sha256_ext.h              mhd_sha256.h \
  sha256_ext_mbedtls.c
endif


if MHD_USE_SHA512_256_INTR
sha512_256_OPTSOURCES = \
  sha512_256_int.c          sha512_256_int.h \
  mhd_sha256.h
endif

if MHD_USE_SHA512_256_OPENSSL
sha512_256_OPTSOURCES = \
  sha512_256_ext.h          mhd_sha512_256.h \
  sha512_256_ext_openssl.c
endif

if MHD_USE_SHA512_256_MBEDTLS
sha512_256_OPTSOURCES = \
  sha512_256_ext.h          mhd_sha512_256.h \
  sha512_256_ext_mbedtls.c
endif


auth_digest_OPTSOURCES = \
  mhd_digest_auth_data.h    mhd_auth_digest_hdr.h \
  response_auth_digest.c    response_auth_digest.h \
  auth_digest.c             auth_digest.h

if MHD_SUPPORT_MD5
  auth_digest_OPTSOURCES += $(md5_OPTSOURCES)
endif

if MHD_SUPPORT_SHA256
  auth_digest_OPTSOURCES += $(sha256_OPTSOURCES)
endif

if MHD_SUPPORT_SHA512_256
  auth_digest_OPTSOURCES += $(sha512_256_OPTSOURCES)
endif

upgrade_OPTSOURCES = \
  mhd_upgrade.h \
  upgrade_prep.c            upgrade_prep.h \
  upgrade_proc.c            upgrade_proc.h \
  upgraded_net.c

tls_common_OPTSOURCES = \
  mhd_tls_enums.h           mhd_tls_ver_stct.h        tls_dh_params.h \
  mhd_tls_internal.c        mhd_tls_internal.h \
  mhd_tls_common.c          mhd_tls_common.h \
  mhd_tls_choice.h          mhd_tls_funcs.h \
  conn_tls_check.c          conn_tls_check.h

tls_multi_OPTSOURCES = \
  tls_multi_tls_lib.h       tls_multi_daemon_data.h   tls_multi_conn_data.h \
  tls_multi_funcs.c         tls_multi_funcs.h

tls_gnu_OPTSOURCES = \
  tls_gnu_tls_lib.h         tls_gnu_daemon_data.h     tls_gnu_conn_data.h \
  tls_gnu_funcs.c           tls_gnu_funcs.h

tls_open_OPTSOURCES = \
  tls_open_tls_lib.h        tls_open_daemon_data.h    tls_open_conn_data.h \
  tls_open_funcs.c          tls_open_funcs.h

tls_mbed_OPTSOURCES = \
  tls_mbed_tls_lib.h        tls_mbed_daemon_data.h    tls_mbed_conn_data.h \
  tls_mbed_funcs.c          tls_mbed_funcs.h

if MHD_SUPPORT_HTTP2
  libmicrohttpd2_la_SOURCES += $(httptwo_OPTSOURCES)
endif

if MHD_SUPPORT_THREADS
  libmicrohttpd2_la_SOURCES += $(threads_OPTSOURCES)
endif

if MHD_SUPPORT_POST_PARSER
  libmicrohttpd2_la_SOURCES += $(post_parser_OPTSOURCES)
endif

if MHD_SUPPORT_AUTH
  libmicrohttpd2_la_SOURCES += $(auth_OPTSOURCES)
endif

if MHD_SUPPORT_AUTH_BASIC
  libmicrohttpd2_la_SOURCES += $(auth_basic_OPTSOURCES)
endif

if MHD_SUPPORT_AUTH_DIGEST
  libmicrohttpd2_la_SOURCES += $(auth_digest_OPTSOURCES)
endif

if MHD_SUPPORT_UPGRADE
  libmicrohttpd2_la_SOURCES += $(upgrade_OPTSOURCES)
endif

if MHD_SUPPORT_HTTPS
  libmicrohttpd2_la_SOURCES += $(tls_common_OPTSOURCES)

if MHD_ENABLE_MULTITLS
  libmicrohttpd2_la_SOURCES += $(tls_multi_OPTSOURCES)
endif

if MHD_SUPPORT_GNUTLS
  libmicrohttpd2_la_SOURCES += $(tls_gnu_OPTSOURCES)
endif

if MHD_SUPPORT_OPENSSL
  libmicrohttpd2_la_SOURCES += $(tls_open_OPTSOURCES)
endif

if MHD_SUPPORT_MBEDTLS
  libmicrohttpd2_la_SOURCES += $(tls_mbed_OPTSOURCES)
endif
endif

libmicrohttpd2_la_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_LIB_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS) \
  -DBUILDING_MHD_LIB=1
libmicrohttpd2_la_CFLAGS = \
  $(AM_CFLAGS) $(MHD_LIB_CFLAGS)
libmicrohttpd2_la_LDFLAGS = \
  $(AM_LDFLAGS) $(MHD_LIB_LDFLAGS) $(MHD_TLS_LIB_LDFLAGS) \
  $(W32_MHD_LIB_LDFLAGS) \
  -export-dynamic -no-undefined \
  -version-info @LIB_VER_CURRENT@:@LIB_VER_REVISION@:@LIB_VER_AGE@
libmicrohttpd2_la_LIBADD = \
  $(MHD_TLS_LIBDEPS) $(MHD_LIBDEPS)
libmicrohttpd2_la_SHORTNAME = mhd2


INCL_SRC_DIR = $(srcdir)/../include

# $(srcdir)/daemon_options.h - excluded to allow parallel builds in this dir
$(srcdir)/daemon_set_options.c: $(INCL_SRC_DIR)/d_options.rec $(INCL_SRC_DIR)/options-generator.c
	@echo "cd $(INCL_SRC_DIR) && $(MAKE) $(AM_MAKEFLAGS) update-daemon-gen-files" && \
	  $(am__cd) $(INCL_SRC_DIR) && $(MAKE) $(AM_MAKEFLAGS) update-daemon-gen-files

# $(srcdir)/response_options.h - excluded to allow parallel builds in this dir
$(srcdir)/response_set_options.c: $(INCL_SRC_DIR)/r_options.rec $(INCL_SRC_DIR)/options-generator.c
	@echo "cd $(INCL_SRC_DIR) && $(MAKE) $(AM_MAKEFLAGS) update-response-gen-files" && \
	  $(am__cd) $(INCL_SRC_DIR) && $(MAKE) $(AM_MAKEFLAGS) update-response-gen-files

AM_V_RC = $(am__v_RC_@AM_V@)
am__v_RC_ = $(am__v_RC_@AM_DEFAULT_V@)
am__v_RC_0 = @echo "  RC      " $@;
am__v_RC_1 =

# General rule is not required, but keep it just in case
# Note: windres does not understand '-isystem' flag, so all
# possible '-isystem' flags are replaced by simple '-I' flags.
.rc.lo:
	$(AM_V_RC) RC_ALL_CPPFLAGS=`echo ' $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) ' | $(SED) -e 's/ -isystem / -I/g'`; \
          $(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) $(RCFLAGS) $(DEFS) $${RC_ALL_CPPFLAGS} $< -o $@

# Note: windres does not understand '-isystem' flag, so all
# possible '-isystem' flags are replaced by simple '-I' flags.
libmicrohttpd2_la-w32_lib_res.lo: $(builddir)/w32_lib_res.rc
	$(AM_V_RC) RC_ALL_CPPFLAGS=`echo ' $(DEFAULT_INCLUDES) $(INCLUDES) $(libmicrohttpd_la_CPPFLAGS) $(CPPFLAGS) ' | $(SED) -e 's/ -isystem / -I/g'`; \
          $(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) $(RCFLAGS) $(DEFS) $${RC_ALL_CPPFLAGS} $(builddir)/w32_lib_res.rc -o $@

if HAVE_W32
MHD_DLL_RES_LO = libmicrohttpd2_la-w32_lib_res.lo
else
MHD_DLL_RES_LO =
endif

EXTRA_libmicrohttpd2_la_DEPENDENCIES = $(MHD_DLL_RES_LO)
libmicrohttpd2_la_LIBADD += $(MHD_DLL_RES_LO)

update-po-POTFILES.in: $(top_srcdir)/po/POTFILES.in

$(top_srcdir)/po/POTFILES.in: $(srcdir)/Makefile.am
	@echo "Creating $@"
	@echo  "" > "$@" && \
	for src in $(am__libmicrohttpd2_la_SOURCES_DIST) ; do \
	  echo "$(subdir)/$$src" >> "$@" || exit $$?; \
	done

EXTRA_DIST = \
  libmicrohttpd2.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmicrohttpd2.pc

$(CONFIG_HEADER): $(builddir)/../incl_priv/config/mhd_config.h.in $(top_builddir)/config.status
	@echo "cd $(srcdir)/../incl_priv/config && $(MAKE) $(AM_MAKEFLAGS) mhd_config.h" && \
	  $(am__cd) $(srcdir)/../incl_priv/config && $(MAKE) $(AM_MAKEFLAGS) mhd_config.h

$(builddir)/../incl_priv/config/mhd_config.h.in: $(top_srcdir)/configure.ac
	@echo "cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh" && \
	  $(am__cd) $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh

check-sources-missing:
	@$(am__cd) "$(srcdir)" && \
	echo $(ECHO_N) "Checking for sources missing in Makefile" ; \
	res="" && \
	makefile_srcs='$(DIST_SOURCES)' && \
	fs_srcs="*.[ch] h2/*.[ch] h2/hpack/*.[ch]" && \
	for src in $$fs_srcs ; do \
	  case " $$makefile_srcs " in \
	    *" $$src "*) echo $(ECHO_N) "." ;; \
	    *) res="$$res $$src" ;; \
	  esac ; \
	done ; \
	echo " done" ; \
	if test -n "$$res" ; then \
	  echo "The following source files are missing in Makefile:" ; \
	  echo " $${res}"; \
	fi

check-missing-sources: check-sources-missing

update-vs-files:
	@$(am__cd) "$(top_srcdir)/w32/common" && \
	  ../../contrib/update_vs_files.sh $(am__libmicrohttpd2_la_SOURCES_DIST)

.PHONY: update-po-POTFILES.in check-sources-missing check-missing-sources update-vs-files

.NOTPARALLEL: $(srcdir)/daemon_options.h $(srcdir)/daemon_set_options.c $(srcdir)/response_options.h $(srcdir)/response_set_options.c
