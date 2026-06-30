# This Makefile.am is in the public domain
# src/tests/unit/Makefile.am
EMPTY_ITEM =

H2_SRC_DIR = $(srcdir)/../../mhd2/h2
HPACK_SRC_DIR = $(H2_SRC_DIR)/hpack

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/mhd2 \
  -I$(top_srcdir)/src/incl_priv \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/tests \
  -DMHD_CPU_COUNT=$(CPU_COUNT) \
  -DMHD_UNIT_TESTING \
  $(CPPFLAGS_ac)\
  $(MHD_LIB_CPPFLAGS) \
  $(MHD_TLS_LIB_CPPFLAGS)

AM_CFLAGS = $(CFLAGS_ac) $(MHD_LIB_CFLAGS)

AM_LDFLAGS = $(LDFLAGS_ac) \
  $(MHD_LIB_LDFLAGS) \
  $(MHD_TLS_LIBDEPS)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

if USE_COVERAGE
  AM_CFLAGS += -fprofile-arcs -ftest-coverage
endif

LDADD = $(MHD_LIBDEPS)

check_PROGRAMS = \
  unit_str_base64 \
  unit_str_bin_hex \
  unit_str_compare \
  unit_str_from_value \
  unit_str_quote \
  unit_str_to_value \
  unit_str_token \
  unit_str_token_remove \
  unit_str_tokens_remove \
  unit_str_pct \
  unit_str

if MHD_SUPPORT_HTTP2
check_PROGRAMS += \
  unit_h2_huffman_encode \
  unit_h2_huffman_decode \
  unit_hpack_tables_dynamic \
  unit_hpack_tables_static \
  unit_hpack_tables_combined \
  $(EMPTY_ITEM)
endif

if MHD_SUPPORT_MD5
check_PROGRAMS += \
  unit_md5
endif

if MHD_SUPPORT_SHA256
check_PROGRAMS += \
  unit_sha256
endif

if MHD_SUPPORT_SHA512_256
check_PROGRAMS += \
  unit_sha512_256
endif

TESTS = $(check_PROGRAMS)

unit_str_SOURCES = unit_str.c

unit_str_compare_SOURCES = \
  unit_str_compare.c \
  $(srcdir)/../mhdt_has_in_name.h

unit_str_to_value_SOURCES = \
  unit_str_to_value.c \
  $(srcdir)/../mhdt_has_in_name.h

unit_str_from_value_SOURCES = \
  unit_str_from_value.c \
  $(srcdir)/../mhdt_has_in_name.h

unit_str_token_SOURCES = \
  unit_str_token.c

unit_str_token_remove_SOURCES = \
  unit_str_token_remove.c

unit_str_tokens_remove_SOURCES = \
  unit_str_tokens_remove.c

unit_h2_huffman_encode_SOURCES = \
  unit_h2_huffman_codec.c \
  $(srcdir)/../mhdt_has_in_name.h \
  $(srcdir)/../mhdt_has_param.h \
  $(HPACK_SRC_DIR)/h2_huffman_codec.c

unit_h2_huffman_decode_SOURCES = $(unit_h2_huffman_encode_SOURCES)

unit_hpack_tables_dynamic_SOURCES = \
  unit_hpack_tables.c \
  $(srcdir)/../mhdt_has_in_name.h \
  $(srcdir)/../mhdt_has_param.h \
  $(srcdir)/../mhdt_checks.h

unit_hpack_tables_static_SOURCES = $(unit_hpack_tables_dynamic_SOURCES)

unit_hpack_tables_combined_SOURCES = $(unit_hpack_tables_dynamic_SOURCES)


unit_md5_SOURCES = unit_md5.c

unit_sha256_SOURCES = unit_sha256.c

unit_sha512_256_SOURCES = unit_sha512_256.c
