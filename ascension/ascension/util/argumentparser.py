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

import argparse
import logging

def parse_arguments() -> argparse.Namespace:
    """Parses arguments for Ascension"""
    parser = argparse.ArgumentParser(
        description='Tool to migrate DNS Zones to the GNU Name System using DNS zone transfer',
    )
    parser.add_argument('domain', metavar='domain',
                        type=str,
                        help='Domain to be migrated into GNS')
    parser.add_argument('-n', '--nameserver',
                        help='Nameserver to use for migrating',
                        required=False)
    parser.add_argument('-G', '--gnunetprefix',
                        help='GNUnet prefix',
                        default='/usr/lib',
                        required=False)
    parser.add_argument('-P', '--port',
                        help='Port to use for zone transfer with nameserver',
                        required=False,
                        default=53)
    parser.add_argument('-W', '--workers',
                        help='Number of parallel processes to use for inserts',
                        required=False,
                        default=4)
    parser.add_argument('-B', '--batchsize',
                        help='How many RRsets to group for batch when sending to namestore',
                        required=False,
                        default=200)
    parser.add_argument('-Z', '--zonefile',
                        help='Zonefile to use for initial bootstrap',
                        required=False)
    parser.add_argument('-k', '--keyfile',
                        help='Keyfile to use for DNS TSIG',
                        required=False,
                        default=None)
    parser.add_argument('-l', '--loglevel',
                        help='Level to use, 10 Debug, 20 Info, 30 Warning, 40 Error, 50 Critical',
                        required=False,
                        default=logging.WARNING)
    parser.add_argument('-t', '--ttl',
                        help='Sets the minimum ttl of records added to GNS',
                        required=False, default=3600, type=int)
    parser.add_argument('-s', '--standalone',
                        help='Run ascension once and not as daemon',
                        action='store_true',
                        required=False, default=False)
    parser.add_argument('-p', '--public',
                        help='Push records to the public DHT',
                        action='store_true',
                        required=False, default=False)
    parser.add_argument('-g', '--dryrun',
                        help='Tests if zone is transferrable without changing anything',
                        action='store_true',
                        required=False, default=False)
    parser.add_argument('-V', '--version', action='version', version='%(prog)s 0.17.6')
    return parser.parse_args()
