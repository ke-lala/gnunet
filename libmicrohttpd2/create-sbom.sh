#! /bin/sh
#
# This file is part of GNU libmicrohttpd.
# (C) 2026 Evgeny Grin (Karlson2k)
#
# GNU libmicrohttpd is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# GNU libmicrohttpd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# Alternatively, you can redistribute GNU libmicrohttpd and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version, together
# with the eCos exception, as follows:
#
#   As a special exception, if other files instantiate templates or
#   use macros or inline functions from this file, or you compile this
#   file and link it with other works to produce a work based on this
#   file, this file does not by itself cause the resulting work to be
#   covered by the GNU General Public License. However the source code
#   for this file must still be made available in accordance with
#   section (3) of the GNU General Public License v2.
#
#   This exception does not invalidate any other reasons why a work
#   based on this file might be covered by the GNU General Public
#   License.
#
# You should have received copies of the GNU Lesser General Public
# License and the GNU General Public License along with this library;
# if not, see <https://www.gnu.org/licenses/>.
#

pkgname='libmicrohttpd2'
mhd_sbom_tool_ver="0.9"
mhd_sbom_pkg_homepage='https://www.gnu.org/software/libmicrohttpd/'
case ${0} in
  -*/*|-*'\'*) mhd_sbom_tool=`expr "X${0}" : 'X.*[/\\]\([^/\\][^/\\]*\)$'`;;
  -*) mhd_sbom_tool="${0}" ;;
  *) mhd_sbom_tool=`basename "${0}"` ;;
esac
test -n "${mhd_sbom_tool}" || mhd_sbom_tool='create-sbom.sh'

test -n "${mhd_sbom_spdx_filename}" || mhd_sbom_spdx_filename="${pkgname}.spdx.json"
test -n "${mhd_sbom_cdx_filename}" || mhd_sbom_cdx_filename="${pkgname}.cdx.json"

print_help_fn() {
  cat << _EOF_
Usage:
  ${0} [var=value ...] {${mhd_sbom_spdx_filename} [${mhd_sbom_cdx_filename}] | ${mhd_sbom_cdx_filename}}
_EOF_
}

mhd_var_nl="
"
mhd_var_cr=`printf '\r'`
mhd_var_tab="	"
# Check whether mhd_var_cr is really set to avoid matching everything
test -n "${mhd_var_cr}" || mhd_var_cr="${mhd_var_nl}"

for param in "$@"
do
  case $param in
    *"'"*|*'"'*|*"${mhd_var_nl}"*|*"${mhd_var_cr}"*|*'\'*) echo "Bad parameter: '$param'" >&2; exit 2 ;;
  esac
  if expr "X${param}" : 'X[A-Za-z][A-Za-z0-9_]*=.*' >/dev/null ; then
    tmp_var_name=`expr "X${param}" : 'X\([A-Za-z][A-Za-z0-9_]*\)='`
    test -n "${tmp_var_name}" || exit 3
    if expr "X${param}" : 'X[A-Za-z][A-Za-z0-9_]*=$' >/dev/null ; then
      tmp_var_val=""
    else
      # Do not check "expr" return code otherwise resulting "0" interpreted as failure
      tmp_var_val=`expr "X${param}" : 'X[A-Za-z][A-Za-z0-9_]*=\(.*\)'`
      test -n "${tmp_var_val}" || exit 3
    fi
    eval "${tmp_var_name}=\"\${tmp_var_val}\"" || exit 1
  else
    case $param in
      "${mhd_sbom_spdx_filename}") mhd_sbom_spdx_outfile="$param" ;;
      "${mhd_sbom_cdx_filename}") mhd_sbom_cdx_outfile="$param" ;;
      --help|-h) print_help_fn; exit 0 ;;
      *) echo "Unknown parameter: '$param'" >&2; exit 2 ;;
    esac
  fi
done

if test -z "${mhd_sbom_spdx_outfile}${mhd_sbom_cdx_outfile}" ; then
  echo "No output file is specified." >&2
  exit 2
fi

# Start from scratch
rm -f "${mhd_sbom_spdx_outfile}" "${mhd_sbom_cdx_outfile}" || exit 1

test -n "${AM_V_P}" || AM_V_P=":"
if ${AM_V_P} >/dev/null 2>/dev/null; then
  AM_V_P=":"
else
  AM_V_P="false"
fi

mhd_sbom_mhd_licence_num='0'

if test "Xno" = "X${mhd_sbom_gnutls_ver}" || test -z "${mhd_sbom_gnutls_ver}"; then
  mhd_sbom_gnutls_ver=""
elif test "X0" = "X${mhd_sbom_gnutls_ver}" || \
    expr "X${mhd_sbom_gnutls_ver}" : "X[1-9][0-9]*\." >/dev/null || \
    expr "X${mhd_sbom_gnutls_ver}" : "X[0-9]\." >/dev/null ; then
  test "2" -le "${mhd_sbom_mhd_licence_num}" || mhd_sbom_mhd_licence_num="2"
else
  echo "Bad GnuTLS version: '${mhd_sbom_gnutls_ver}'" >&2
  exit 2
fi

if test "Xno" = "X${mhd_sbom_openssl_ver}" || test -z "${mhd_sbom_openssl_ver}"; then
  mhd_sbom_openssl_ver=""
elif test "X0" = "X${mhd_sbom_openssl_ver}" || \
    expr "X${mhd_sbom_openssl_ver}" : "X[1-9][0-9]*\." >/dev/null || \
    expr "X${mhd_sbom_openssl_ver}" : "X[0-9]\." >/dev/null ; then
  test "3" -le "${mhd_sbom_mhd_licence_num}" || mhd_sbom_mhd_licence_num="3"
else
  echo "Bad OpenSSL version: '${mhd_sbom_openssl_ver}'" >&2
  exit 2
fi

if test "Xno" = "X${mhd_sbom_mbedtls_ver}" || test -z "${mhd_sbom_mbedtls_ver}"; then
  mhd_sbom_mbedtls_ver=""
elif test "X0" = "X${mhd_sbom_mbedtls_ver}" || \
    expr "X${mhd_sbom_mbedtls_ver}" : "X[1-9][0-9]*\." >/dev/null || \
    expr "X${mhd_sbom_mbedtls_ver}" : "X[0-9]\." >/dev/null ; then
  test "3" -le "${mhd_sbom_mhd_licence_num}" || mhd_sbom_mhd_licence_num="3"
else
  echo "Bad Mbed TLS version: '${mhd_sbom_mbedtls_ver}'" >&2
  exit 2
fi

if test -z "${mhd_sbom_mhd_licence}"; then
  case ${mhd_sbom_mhd_licence_num} in
    0) mhd_sbom_mhd_licence='LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0)' ;;
    2) mhd_sbom_mhd_licence='LGPL-2.1-or-later' ;;
    3) mhd_sbom_mhd_licence='LGPL-3.0-or-later' ;;
    *) echo "Internal error" >&2; exit 3 ;;
  esac
fi

test -n "${mhd_sbom_publisher}" || mhd_sbom_publisher='Evgeny Grin (Karlson2k), Christian Grothoff'

err_out_cleanup() {
  rm -f "${mhd_sbom_spdx_outfile}" "${mhd_sbom_cdx_outfile}"
  exit 1
}

is_uuid_valid_fn() {
  case ${1} in
    [0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]-[0-9a-f][0-9a-f][0-9a-f][0-9a-f]-[0-9a-f][0-9a-f][0-9a-f][0-9a-f]-[0-9a-f][0-9a-f][0-9a-f][0-9a-f]-[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]) return 0 ;;
  esac
  return 1
}

is_timestamp_valid_fn() {
  case ${1} in
    [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z) return 0 ;;
  esac
  return 1
}

is_timestamp_valid_fn "${mhd_sbom_timestamp}" || \
  { mhd_sbom_timestamp=`date -u '+%Y-%m-%dT%H:%M:%SZ'` || mhd_sbom_timestamp="" ; } 2>/dev/null

is_timestamp_valid_fn "${mhd_sbom_timestamp}" || mhd_sbom_timestamp=''


is_uuid_valid_fn "${mhd_sbom_uuid}" || \
  { mhd_sbom_uuid=`uuidgen` || mhd_sbom_uuid='' ; } 2>/dev/null

if is_uuid_valid_fn "${mhd_sbom_uuid}" ; then : ; else
  test -r '/proc/sys/kernel/random/uuid' && read mhd_sbom_uuid < '/proc/sys/kernel/random/uuid' 2>/dev/null
fi

is_uuid_valid_fn "${mhd_sbom_uuid}" || mhd_sbom_uuid=''


if test -n "${mhd_sbom_spdx_outfile}"; then

  if ${AM_V_P}; then
    echo "Generating '${mhd_sbom_spdx_outfile}'..."
  else
    echo "  GEN${mhd_var_tab}${mhd_sbom_spdx_outfile}"
  fi

  test -n "${mhd_sbom_spdx_name}" || mhd_sbom_spdx_name="${pkgname}${mhd_sbom_mhd_version_short:+-}${mhd_sbom_mhd_version_short}"

  test -n "${mhd_sbom_spdx_docnamespace_base}" || mhd_sbom_spdx_docnamespace_base="https://www.gnu.org/software/libmicrohttpd/spdx/${mhd_sbom_spdx_name}"
  test -n "${mhd_sbom_spdx_docnamespace_suff}" || mhd_sbom_spdx_docnamespace_suff="${mhd_sbom_uuid}"
  test -n "${mhd_sbom_spdx_docnamespace_suff}" || mhd_sbom_spdx_docnamespace_suff="${mhd_sbom_timestamp}"
  test -n "${mhd_sbom_spdx_docnamespace_suff}" || mhd_sbom_spdx_docnamespace_suff="${mhd_sbom_mhd_version_full}"
  test -n "${mhd_sbom_spdx_docnamespace_full}" \
    || mhd_sbom_spdx_docnamespace_full="${mhd_sbom_spdx_docnamespace_base}${mhd_sbom_spdx_docnamespace_suff:+-}${mhd_sbom_spdx_docnamespace_suff}"

  if test -z "${mhd_sbom_spdx_purl}" ; then
    mhd_sbom_spdx_purl="pkg:generic/${pkgname}"
    test -z "${mhd_sbom_mhd_version_full}" || mhd_sbom_spdx_purl="${mhd_sbom_spdx_purl}@${mhd_sbom_mhd_version_full}"
    test -z "${mhd_sbom_mhd_version_extra}" || mhd_sbom_spdx_purl="${mhd_sbom_spdx_purl}?${mhd_sbom_mhd_version_extra}"
  elif test "Xno" = "X${mhd_sbom_spdx_purl}" ; then
    mhd_sbom_spdx_purl=""
  fi

  # Basic checks only, not a real validation
  case "${pkgname}${mhd_sbom_mhd_version_full}${mhd_sbom_mhd_licence}${mhd_sbom_spdx_name}${mhd_sbom_spdx_docnamespace_full}${mhd_sbom_spdx_purl}${mhd_sbom_pkg_homepage}${mhd_sbom_pkg_dwnl_url}${mhd_sbom_gnutls_ver}${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}${mhd_sbom_tool}${mhd_sbom_tool_ver}" in
    *"'"*|*'"'*|*"${mhd_var_nl}"*|*"${mhd_var_cr}"*|*'\'*|*"${mhd_var_tab}"*) echo "Bad JSON data" >&2; exit 2 ;;
  esac

  # Cleanup partial output on early exit
  trap err_out_cleanup 0 1 2 13 15

  mhd_sbom_next_element_comma=''
  test -z "${mhd_sbom_gnutls_ver}${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}" || \
    mhd_sbom_next_element_comma=','

  cat >"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
{
  "SPDXID": "SPDXRef-DOCUMENT",
  "spdxVersion": "SPDX-2.3",
  "name": "${mhd_sbom_spdx_name}",
  "creationInfo": {
_JSON_EOF_
  test -z "${mhd_sbom_timestamp}" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    "created": "${mhd_sbom_timestamp}",
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    "creators": [
      "Tool: ${pkgname}-${mhd_sbom_tool}-${mhd_sbom_tool_ver}"
    ]
  },
  "dataLicense": "CC0-1.0",
  "documentNamespace": "${mhd_sbom_spdx_docnamespace_full}",
  "packages": [
    {
      "SPDXID": "SPDXRef-Package-libmicrohttpd2",
      "name": "${pkgname}",
_JSON_EOF_
  test -z "${mhd_sbom_mhd_version_full}" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "versionInfo": "${mhd_sbom_mhd_version_full}",
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "homepage": "${mhd_sbom_pkg_homepage}",
      "downloadLocation": "${mhd_sbom_pkg_dwnl_url:-NOASSERTION}",
      "filesAnalyzed": false,
      "licenseDeclared": "LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0)",
      "licenseConcluded": "${mhd_sbom_mhd_licence}",
      "copyrightText": "NOASSERTION",
_JSON_EOF_
  test -z "${mhd_sbom_spdx_purl}" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "externalRefs": [
        {
          "referenceCategory": "PACKAGE-MANAGER",
          "referenceType": "purl",
          "referenceLocator": "${mhd_sbom_spdx_purl}"
        }
      ],
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "description": "Small C library for embedding an HTTP server in applications"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  if test -n "${mhd_sbom_gnutls_ver}"; then
    mhd_sbom_next_element_comma=''
    test -z "${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}" || \
      mhd_sbom_next_element_comma=','
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "SPDXID": "SPDXRef-Package-GnuTLS",
      "name": "GnuTLS",
_JSON_EOF_
    test "X${mhd_sbom_gnutls_ver}" = "X0" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "versionInfo": "${mhd_sbom_gnutls_ver}",
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "downloadLocation": "NOASSERTION",
      "filesAnalyzed": false,
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  if test -n "${mhd_sbom_openssl_ver}"; then
    mhd_sbom_next_element_comma=''
    test -z "${mhd_sbom_mbedtls_ver}" || \
      mhd_sbom_next_element_comma=','
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "SPDXID": "SPDXRef-Package-OpenSSL",
      "name": "OpenSSL",
_JSON_EOF_
    test "X${mhd_sbom_openssl_ver}" = "X0" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "versionInfo": "${mhd_sbom_openssl_ver}",
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "downloadLocation": "NOASSERTION",
      "filesAnalyzed": false,
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  if test -n "${mhd_sbom_mbedtls_ver}"; then
    mhd_sbom_next_element_comma=''
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "SPDXID": "SPDXRef-Package-MbedTLS",
      "name": "MbedTLS",
_JSON_EOF_
    test "X${mhd_sbom_mbedtls_ver}" = "X0" || cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "versionInfo": "${mhd_sbom_mbedtls_ver}",
_JSON_EOF_
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
      "downloadLocation": "NOASSERTION",
      "filesAnalyzed": false,
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  mhd_sbom_next_element_comma=''
  test -z "${mhd_sbom_gnutls_ver}${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}" || \
    mhd_sbom_next_element_comma=','
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
  ],
  "relationships": [
    {
      "spdxElementId": "SPDXRef-DOCUMENT",
      "relationshipType": "DESCRIBES",
      "relatedSpdxElement": "SPDXRef-Package-libmicrohttpd2"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  if test -n "${mhd_sbom_gnutls_ver}"; then
    mhd_sbom_next_element_comma=''
    test -z "${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}" || \
      mhd_sbom_next_element_comma=','
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "spdxElementId": "SPDXRef-Package-libmicrohttpd2",
      "relationshipType": "DEPENDS_ON",
      "relatedSpdxElement": "SPDXRef-Package-GnuTLS"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  if test -n "${mhd_sbom_openssl_ver}"; then
    mhd_sbom_next_element_comma=''
    test -z "${mhd_sbom_mbedtls_ver}" || \
      mhd_sbom_next_element_comma=','
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "spdxElementId": "SPDXRef-Package-libmicrohttpd2",
      "relationshipType": "DEPENDS_ON",
      "relatedSpdxElement": "SPDXRef-Package-OpenSSL"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  if test -n "${mhd_sbom_mbedtls_ver}"; then
    mhd_sbom_next_element_comma=''
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "spdxElementId": "SPDXRef-Package-libmicrohttpd2",
      "relationshipType": "DEPENDS_ON",
      "relatedSpdxElement": "SPDXRef-Package-MbedTLS"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
  fi
  cat >>"${mhd_sbom_spdx_outfile}" <<_JSON_EOF_ || exit 1
  ]
}
_JSON_EOF_
fi

if test -n "${mhd_sbom_cdx_outfile}"; then

  if ${AM_V_P}; then
    echo "Generating '${mhd_sbom_cdx_outfile}'..."
  else
    echo "  GEN${mhd_var_tab}${mhd_sbom_cdx_outfile}"
  fi

  if test -z "${mhd_sbom_cdx_purl}" ; then
    mhd_sbom_cdx_purl="pkg:generic/${pkgname}"
    test -z "${mhd_sbom_mhd_version_full}" || mhd_sbom_cdx_purl="${mhd_sbom_cdx_purl}@${mhd_sbom_mhd_version_full}"
    test -z "${mhd_sbom_mhd_version_extra}" || mhd_sbom_cdx_purl="${mhd_sbom_cdx_purl}?${mhd_sbom_mhd_version_extra}"
  elif test "Xno" = "X${mhd_sbom_cdx_purl}" ; then
    mhd_sbom_cdx_purl=""
  fi

  if test -z "${mhd_sbom_cdx_bom_ref}" ; then
    if test -n "${mhd_sbom_cdx_purl}" ; then
      mhd_sbom_cdx_bom_ref="${mhd_sbom_cdx_purl}"
    else
      mhd_sbom_cdx_bom_ref="${pkgname}"
    fi
  fi

  # Basic checks only, not a real validation
  case "${pkgname}${mhd_sbom_pkg_homepage}${mhd_sbom_mhd_version_short}${mhd_sbom_mhd_licence}${mhd_sbom_cdx_purl}${mhd_sbom_cdx_bom_ref}${mhd_sbom_publisher}${mhd_sbom_gnutls_ver}${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}${mhd_sbom_tool}${mhd_sbom_tool_ver}" in
    *"'"*|*'"'*|*"${mhd_var_nl}"*|*"${mhd_var_cr}"*|*'\'*|*"${mhd_var_tab}"*) echo "Bad JSON data" >&2; exit 2 ;;
  esac

  mhd_sbom_cdx_spec_version="1.6"

  # Cleanup partial output on early exit
  trap err_out_cleanup 0 1 2 13 15

  cat >"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
{
  "\$schema": "http://cyclonedx.org/schema/bom-${mhd_sbom_cdx_spec_version}.schema.json",
  "bomFormat": "CycloneDX",
  "specVersion": "${mhd_sbom_cdx_spec_version}",
  "version": 1,
_JSON_EOF_
  test -z "${mhd_sbom_uuid}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  "serialNumber": "urn:uuid:${mhd_sbom_uuid}",
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  "metadata": {
_JSON_EOF_
  test -z "${mhd_sbom_timestamp}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    "timestamp": "${mhd_sbom_timestamp}",
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    "component": {
      "type": "library",
      "name": "${pkgname}",
      "description": "Small C library for embedding an HTTP server in applications",
_JSON_EOF_
  test -z "${mhd_sbom_mhd_version_short}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "version": "${mhd_sbom_mhd_version_short}",
_JSON_EOF_
  test -z "${mhd_sbom_mhd_licence}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "licenses": [
        {
          "expression": "${mhd_sbom_mhd_licence}"
        }
      ],
_JSON_EOF_
  test -z "${mhd_sbom_cdx_purl}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "purl": "${mhd_sbom_cdx_purl}",
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "bom-ref": "${mhd_sbom_cdx_bom_ref}",
      "publisher": "${mhd_sbom_publisher}"
    },
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    "tools": {
      "components": [
        {
          "type": "application",
          "group": "org.gnu.libmicrohttpd",
_JSON_EOF_
  test -z "${mhd_sbom_tool_ver}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
          "version": "${mhd_sbom_tool_ver}",
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
          "name": "${mhd_sbom_tool}"
        }
      ]
    },
    "lifecycles": [
      {
        "phase": "build"
      }
    ],
    "authors": [
      {
        "name": "Evgeny Grin (Karlson2k)"
      }
    ],
    "properties": [
      {
        "name": "org.gnu.libmicrohttpd:separate-sbom-license",
        "value": "CC0-1.0"
      }
    ],
    "licenses": [
      {
        "expression": "CC0-1.0"
      }
    ]
  },
_JSON_EOF_
  mhd_sbom_dependson=""
  if test -n "${mhd_sbom_gnutls_ver}" || test -n "${mhd_sbom_openssl_ver}" \
     || test -n "${mhd_sbom_mbedtls_ver}" ; then

    cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  "components": [
_JSON_EOF_

    if test -n "${mhd_sbom_gnutls_ver}" ; then
      mhd_sbom_dependson="${mhd_sbom_dependson}\"tlsbackend-gnutls\""
      mhd_sbom_next_element_comma=''
      if test -n "${mhd_sbom_openssl_ver}${mhd_sbom_mbedtls_ver}"; then
        mhd_sbom_dependson="${mhd_sbom_dependson},${mhd_var_nl}        "
        mhd_sbom_next_element_comma=','
      fi
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "type": "library",
      "name": "GnuTLS",
_JSON_EOF_
      test "X0" = "X${mhd_sbom_gnutls_ver}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "version": "${mhd_sbom_gnutls_ver}",
_JSON_EOF_
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "bom-ref": "tlsbackend-gnutls"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
    fi

    if test -n "${mhd_sbom_openssl_ver}" ; then
      mhd_sbom_dependson="${mhd_sbom_dependson}\"tlsbackend-openssl\""
      mhd_sbom_next_element_comma=''
      if test -n "${mhd_sbom_mbedtls_ver}"; then
        mhd_sbom_dependson="${mhd_sbom_dependson},${mhd_var_nl}        "
        mhd_sbom_next_element_comma=','
      fi
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "type": "library",
      "name": "OpenSSL",
_JSON_EOF_
      test "X0" = "X${mhd_sbom_openssl_ver}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "version": "${mhd_sbom_openssl_ver}",
_JSON_EOF_
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "bom-ref": "tlsbackend-openssl"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
    fi

    if test -n "${mhd_sbom_mbedtls_ver}" ; then
      mhd_sbom_dependson="${mhd_sbom_dependson}\"tlsbackend-mbedtls\""
      mhd_sbom_next_element_comma=''
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
    {
      "type": "library",
      "name": "MbedTLS",
_JSON_EOF_
      test "X0" = "X${mhd_sbom_mbedtls_ver}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "version": "${mhd_sbom_mbedtls_ver}",
_JSON_EOF_
      cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
      "bom-ref": "tlsbackend-mbedtls"
    }${mhd_sbom_next_element_comma}
_JSON_EOF_
    fi

    cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  ],
_JSON_EOF_
  fi
  test -z "${mhd_sbom_dependson}" || cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  "dependencies": [
    {
      "ref": "${mhd_sbom_cdx_bom_ref}",
      "dependsOn": [
        ${mhd_sbom_dependson}
      ]
    }
  ],
_JSON_EOF_
  cat >>"${mhd_sbom_cdx_outfile}" <<_JSON_EOF_ || exit 1
  "externalReferences": [
    {
      "type": "website",
      "url": "${mhd_sbom_pkg_homepage}"
    },
    {
      "type": "vcs",
      "url": "git://git.gnunet.org/libmicrohttpd2.git"
    },
    {
      "type": "issue-tracker",
      "url": "https://bugs.gnunet.org/view_all_bug_page.php?project_id=32"
    },
    {
      "type": "mailing-list",
      "url": "https://lists.gnu.org/mailman/listinfo/libmicrohttpd"
    }
  ]
}
_JSON_EOF_

  ${AM_V_P} && echo "'${mhd_sbom_cdx_outfile}' - done."
fi


trap '' 0
