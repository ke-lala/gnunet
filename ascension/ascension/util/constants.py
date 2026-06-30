"""
This file is part of Ascension.
Copyright (C) 2022 GNUnet e.V.

Ascension is free software: you can redistribute it and/or modify it
under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Ascension is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

SPDX-License-Identifier: AGPL3.0-or-later

Author: rexxnor
"""

# This is the list of record types Ascension (and GNS) currently
# explicitly supports.  Record types we encounter that are not
# in this list and not in the OBSOLETE_RECORD_TYPES list will
# create a warning (information loss during migration).
SUPPORTED_RECORD_TYPES = [
    "A", "AAAA", "NS", "MX", "SRV", "TXT", "CNAME", "SOA", "CAA",
]

# Record types that exist in DNS but that won't ever exist in GNS
# as they are not needed anymore (so we should not create a warning
# if we drop one of these).
OBSOLETE_RECORD_TYPES = [
    "PTR",
    "SIG", "KEY",
    "RRSIG", "NSEC", "DNSKEY", "NSEC3", "NSEC3PARAM", "CDNSKEY", "DS",
    "TKEY", "TSIG",
    "TA", "DLV",
]

# Union of the above
PROCESSABLE_RECORD_TYPES = SUPPORTED_RECORD_TYPES #+ OBSOLETE_RECORD_TYPES
