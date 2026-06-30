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

import logging
import socket

import dns.rdata

import ascension.util.classes


class Transformer:
    """Class containing all record transformation functions"""
    def __init__(self, domain: str, dnszone: ascension.util.classes.DNSZone):
        """Contructor"""
        self.domain = domain
        self.dnszone = dnszone
        self.subzonedict = None
        self.logger = logging.getLogger(__name__)

    def transform_ns_record_to_gns(self,
                                   label: str,
                                   zonename: str,
                                   record: dns.rdata.Rdata) -> tuple:
        """Handles NS record transformation to GNS"""
        value = str(record)
        # check if it is zone root
        if label == "@" and zonename == self.domain:
            return (None, None, None)
        if self.subzonedict.get(str(label) + "." + zonename):
            return (None, None, None)
        nameserver = str(record.target)
        if nameserver[-1] == ".":
            nameserver = nameserver[:-1]
        if str(value)[-1] == ".":
            # FQDN provided
            if value.endswith(f".{zonename}."):
                # in bailiwick
                value = self.dnszone.resolve_glue(record.target)
            else:
                # out of bailiwick
                value = f"{str(label)}.{zonename}@{nameserver}"
        else:
            # Name is relative to zone, must be in bailiwick
            value = self.dnszone.resolve_glue(record.target)
            if not value:
                if label.startswith("@"):
                    value = f"{zonename}@{record.target}.{self.domain}"
                else:
                    value = f"{str(label)}.{self.domain}@{record.target}.{self.domain}"
        self.logger.debug("Transformed NS record to GNS2DNS record")
        rdtype = "GNS2DNS"
        return (rdtype, value, label)


    def transform_soa_record_to_gns(self,
                                    label: str,
                                    zonename: str,
                                    record: dns.rdata.Rdata) -> tuple:
        """Handles SOA record transformation to GNS"""
        value = str(record)
        zonetuple = str(value).split(' ')
        authns, owner, serial, refresh, retry, expiry, irefresh = zonetuple
        if authns[-1] == '.':
            authns = authns[:-1]
        if owner[-1] == '.':
            owner = owner[:-1]
        value = (
            f"{authns}.{zonename} {owner}.{zonename} "
            f"({int(serial)} {int(refresh)} {int(retry)} {int(expiry)} {int(irefresh)})"
        )
        self.logger.info("Transformed SOA record to GNS format")
        return ('SOA', value, label)


    def transform_cname_record_to_gns(self,
                                      label: str,
                                      _: str,
                                      record: dns.rdata.Rdata) -> tuple:
        """Handles CNAME record transformation to GNS"""
        self.logger.info("Transformed CNAME record to GNS format")
        return ('REDIRECT', f"{str(record)}.+", label)


    def transform_mx_record_to_gns(self,
                                   label: str,
                                   zonename: str,
                                   record: dns.rdata.Rdata) -> tuple:
        """Handles MX record transformation to GNS"""
        value = str(record)
        priority, mailserver = str(value).split(' ')
        if mailserver[-1] == ".":
            mailserver = mailserver[:-1]
        mailserver = f"{mailserver}.{zonename}"
        value = f"{priority},{mailserver}"
        self.logger.info("Transformed MX record to GNS format")
        return ('MX', value, label)


    def transform_srv_record_to_gns(self, label: str, _: str, record: dns.rdata.Rdata) -> tuple:
        """Handles SRV record transformation to GNS"""
        value = str(record)

        # record type number required for the BOX record SRV = 33
        srv = int(dns.rdatatype.SRV)
        priority, weight, destport, target = value.split(' ')
        try:
            service, proto = label.split('.')[0:2]
        except ValueError:
            service = "_https"
            proto = "_tcp"

        label = label.lstrip(f"{service}.{proto}") or '@'

        try:
            proto = str(socket.getprotobyname(proto.strip('_')))
            service = str(socket.getservbyname(service.strip('_')))
        except OSError:
            self.logger.warning("invalid protocol: %s", proto)
            return ('BOX', None, None)

        value = f"{proto} {service} {srv} {priority} {weight} {destport} {target}.{self.domain}"
        self.logger.debug("Transformed SRV record to BOX record")
        return ('BOX', value, label)


    def transform_to_gns_format(self,
                                record: dns.rdata.Rdata,
                                rdtype: dns.rdata.Rdata,
                                zonename: str,
                                label: str) -> tuple:
        """
        Transforms value of record to GNS compatible format
        :param record: record to transform
        :param rdtype: record value to transform
        :param zonename: name of the zone to add to
        :param label: label under which the record is stored
        :returns: a tuple consisting of the new rdtype, the label and value
        """
        value = str(record)

        if label is None:
            label = '@'

        record_transformations = {
            "CNAME": self.transform_cname_record_to_gns,
            "MX": self.transform_mx_record_to_gns,
            "NS": self.transform_ns_record_to_gns,
            "SOA": self.transform_soa_record_to_gns,
            "SRV": self.transform_srv_record_to_gns,
        }
        if rdtype not in record_transformations:
            self.logger.debug("Did not transform record of type: %s", rdtype)
            return (rdtype, value, label)

        # Should be able to just take rdtype directly
        transformation_function = record_transformations.get(rdtype)

        # Leave it to the implementations to return the values
        return transformation_function(label, zonename, record)
