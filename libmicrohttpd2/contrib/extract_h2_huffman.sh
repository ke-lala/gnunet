#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0)

set -euo pipefail

OUT="h2_huffman_insert.h"
TMPDIR="$(mktemp -d)"
RFC_TEXT_FILE="$TMPDIR/rfc7541.txt"
trap 'rm -rf "$TMPDIR"' EXIT

echo "Downloading RFC specification..."
wget --show-progress -O "$RFC_TEXT_FILE" "https://www.rfc-editor.org/rfc/rfc7541.txt"

echo "Extractibe data..."
{
  gawk -f - "$RFC_TEXT_FILE" <<'END_OF_AWK_SCRIPT'
BEGIN { IGNORECASE=1 }
{
  sub(/\r$/, "", $0)  # CRLF -> LF (if any)

  # Strictly match table lines
  if (match($0,
    /^[ ]+('.'[ ]+)?\([ ]*([0-9]+)[ ]*\)[ ]*\|[ 01|]+[ ]+([0-9A-Fa-f]+)[ ]+\[[ ]*([0-9]{1,2})[ ]*\][ ]*$/,
    M))
  {
    idx  = M[2] + 0
    hex  = M[3]
    blen = M[4] + 0
    if (idx <= 255) {
      code[idx] = strtonum("0x" hex)
      clen[idx] = blen
    }
  }
}
END {
  # Check completeness
  for (i = 0; i <= 255; i++)
  {
    if (!(i in code)) {
      printf("/* ERROR: missing Huffman entry %d */\n", i) > "/dev/stderr"
      exit 2
    }
  }

  print "/**"
  print " * HTTP/2 static Huffman codes from RFC 7541"
  print " *"
  print " * Index is the character value (0..255)."
  print " * The table values are codes that are left-algined (MSB-aligned) to"
  print " * the 32 bit value."
  print " * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B"
  print " *"
  print " * @note The table does not include EOS code."
  print " */"
  print "mhd_constexpr uint_least32_t mhd_h2huff_code_by_sym[256] ="
  print "{"
  lens = ""
  for (i = 0; i <= 255; i++)
  {
    blen = clen[i]

    # Check length
    if (blen < 1 || blen > 32)
    {
      printf("/* ERROR: bad bit length %u for index %d */\n", blen, i) > "/dev/stderr"
      exit 2
    }

    # Check code value fits into the declared length
    if (rshift(code[i], blen) != 0)
    {
      printf("/* ERROR: code 0x%X for index %d exceeds its length %u */\n",
             code[i], i, blen) > "/dev/stderr"
      exit 2
    }

    # Left-align to the MSB of a 32-bit word
    shift = 32 - blen
    msb   = lshift(code[i], shift)

    # Check nothing beyond 32 bits (not really needed, the length has been already checked)
    if (rshift(msb, 32) != 0)
    {
      printf("/* ERROR: left-aligned code for index %d does not fit in 32 bits */\n",
             i) > "/dev/stderr"
      exit 2
    }

    # A label for printable US-ASCII chars
    label = ""
    if (i >= 32 && i <= 126)
    {
      ch = sprintf("%c", i)
      if (i == 92) ch = "\\"   # this prints a single backslash
      label = sprintf("%c%s%c", 39, ch, 39)
    }
    comment = sprintf("/* %3s (%3d) */", label, i)

    printf("  %s 0x%08Xu%s\n", comment, msb, (i < 255 ? "," : ""))
    lens = (lens sprintf("  %s %2uu%s\n", comment, blen, (i < 255 ? "," : "")))
  }
  print "};"
  print ""
  print ""
  print "/**"
  print " * HTTP/2 static Huffman codes length in bits from RFC 7541"
  print " * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B"
  print " *"
  print " * Index is the character value (0..255)."
  print " * The table values are lengths of the code in bits."
  print " * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B"
  print " *"
  print " * @note The table does not include EOS code."
  print " */"
  print "mhd_constexpr uint8_t mhd_h2huff_bitlen_by_sym[256] ="
  printf ("{\n%s};\n", lens)
}
END_OF_AWK_SCRIPT
} > "$OUT"

echo "Success."
echo "The output file is: $OUT"
