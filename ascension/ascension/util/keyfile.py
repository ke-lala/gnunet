"""
This file is part of Ascension.
Copyright (C) 2018-2022 GNUnet e.V.

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

import shlex

import dns.name
import dns.tsig

#@staticmethod
def parse_bind_keyfile(keyring: str) -> dict[dns.name.Name, dns.tsig.Key]:
    """
    Reads a BIND style keyfile and creates a dictionary in the form of:
    dict(dns.name.Name: dns.tsig.Key)
    """
    if not keyring:
        return None

    with open(keyring, 'r', encoding='utf-8') as keyfile:
        ast = shlex.shlex(keyfile.read())

    ast.whitespace_split = True

    keydict = {}

    while True:
        while ast.get_token() == "key":
            keyname = dns.name.from_text(ast.get_token())
            _ = ast.get_token()
            if ast.get_token() == "algorithm":
                keyalgo = ast.get_token().strip(";")
            if ast.get_token() == "secret":
                keysecret = ast.get_token().strip(";")
            keydict[keyname] = dns.tsig.Key(keyname, keysecret, keyalgo)
        if ast.get_token():
            continue
        break
    return keydict
