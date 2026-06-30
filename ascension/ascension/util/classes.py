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

import dataclasses
import logging
import ipaddress
import json
import re
import subprocess

import dns.exception
import dns.rdatatype
import dns.resolver
import dns.query
import dns.xfr
import dns.zone

import ascension.util.rest


@dataclasses.dataclass
class GNSRecordData():
    """Specifies record data"""
    value: str
    record_type: str
    relative_expiration: int
    is_private: bool = True
    is_relative_expiration: bool = False
    is_supplemental: bool = False
    is_shadow: bool = False

    def __post_init__(self):
        """Called after data is initialized"""
        # Convert the relative_expiration from seconds to microseconds
        self.relative_expiration = self.relative_expiration * 1000 * 1000

    def to_json(self):
        """JSON Serializing"""
        return json.dumps(self.__dict__)


@dataclasses.dataclass
class GNSRRecordSet():
    """Defines a GNS record set"""
    record_name: str
    data: list[GNSRecordData]

    def to_json(self):
        """JSON Serializing"""
        return json.dumps(self, default=lambda x: x.__dict__)


class GNSZone():
    """Defines a GNS Zone"""
    def __init__(self,
                 gnunet_rest: ascension.util.rest.GNUnetRestSession,
                 zonename: str,
                 public: bool,
                 minimum: int,
                 logger: logging.Logger):
        """Constructor"""
        self.logger = logger
        self.domain = zonename
        self.gnunet_rest = gnunet_rest
        self.public = public
        self.minimum = minimum


    def bootstrap_zone(self):
        """
        Creates the zone in gnunet
        """
        res = subprocess.run(["gnunet-identity", "-X", "-C", self.domain])
        if res.returncode == 201:
            self.logger.info("Identity %s already exists", self.domain)
        elif res.returncode != 0:
            self.logger.error("Failed to create identity %s", self.domain)

    def delete_zone(self, zone):
        """
        Deletes the zone in gnunet
        """
        self.purge_records(zone)
        self.logger.warning("Deleting zone %s", zone)
        res = subprocess.run(["gnunet-identity", "-D", zone])
        if res.returncode != 0:
            self.logger.error("Failed to delete zone %s", self.domain)

    def purge_records(self, zone=None):
        """
        Purges the zone in gnunet
        """
        if zone == None:
            zone = self.domain
        self.logger.warning("Purging records of zone %s", zone)
        res = subprocess.run(["gnunet-namestore", "-X", "-z", zone])
        if res.returncode != 0:
            self.logger.error("Failed to purge zone %s", self.domain)

    def get_gns_zone_serial(self) -> int:
        """
        Fetches the zones latest (highest) serial from GNS
        :returns: serial of the SOA record in GNS
        """
        response = self.gnunet_rest.get(f"/namestore/{self.domain}/@?record_type=SOA")
        if response.status_code == 404:
            return 0

        data = [response.json()]
        if not isinstance(data, list):
            error = data.get('error')
            if error in ascension.util.rest.NAMESTORE_REST_API_ERRORS:
                self.logger.warning("Task failed with known error: %s", error)
            self.logger.warning("Task failed with unknown error: %s", error)
            return 0

        soapattern = re.compile(r'.+\s(\d+) \d+ \d+ \d+ \d+', re.M)
        recordlists = [x for x in data if x.get('data')]
        soa_serials = [0]
        for rlist in recordlists:
            for record in rlist.get('data'):
                if record.get('record_type') == 'SOA':
                    soa_serials.append(int(re.findall(soapattern, record.get('value'))[0]))
        return max(soa_serials)


    def create_zone_and_get_pkey(self, zonename: str) -> str:
        """
        Creates or gets the zone in zonename and returns a the zones public key
        :param zonename: The label name of the zone
        :returns: str of pubkey of created or existing GNUnet zone
        """
        self.logger.info('Creating zone ' + zonename)
        # This is needed including the argument for subzones
        res = subprocess.run(["gnunet-identity", "-X", "-C", zonename])
        if res.returncode == 201:
            self.logger.error("Identity %s already exists", zonename)
        elif res.returncode != 0:
            self.logger.error("Failed to create identity %s", zonename)
            return None
        else:
            self.logger.info("Created identity %s", zonename)
        res = subprocess.run(["gnunet-identity", "-q", "-d", "-e",  zonename], text=True, capture_output=True)
        if res.returncode != 0:
            self.logger.error("Failed to read identity key for `%s'", zonename)
            return None
        return res.stdout.rstrip()


class DNSZone:
    """
    Uniform representation of a DNS Zone
    """
    def __init__(self,
                 domain: str,
                 transferns: str,
                 port: int,
                 keyring: str):
        """Constructor"""
        self.logger = logging.getLogger(__name__)
        self.domain = domain
        self.zone = None
        self.resolver = dns.resolver.Resolver()
        self.keyring = ascension.util.keyfile.parse_bind_keyfile(keyring)
        try:
            if transferns:
                ipaddress.ip_address(transferns)
            else:
                temp_soa = self.resolver.resolve(self.domain, dns.rdatatype.SOA)
                transferns = str(dns.resolver.resolve(temp_soa.rrset[0].mname, 'A')[0])
        except ValueError:
            # intentional to resolve should it not be an IP
            transferns = str(dns.resolver.resolve(transferns, 'A')[0])
        self.transferns = transferns
        self.resolver.nameservers = [transferns]
        self.resolver.port = int(port)
        self.zone_backup_file = f"dnszone_{self.domain}"


    def test_zone_transfer(self):
        """Attempts a zone transfer with a short timeout"""
        # Makes domains better resolvable
        domain = self.domain
        if not domain == "@":
            domain = domain + "."
        # SOA is different if taken directly from SOA record
        # compared to AXFR/IXFR - changed to respect this
        try:
            soa_answer = self.resolver.resolve(domain, 'SOA')
            _ = self.resolver.resolve(soa_answer[0].mname, 'A')
        except (dns.resolver.NoAnswer, dns.resolver.NXDOMAIN):
            self.logger.warning("The domain '%s' is not resolvable.",
                                domain)

        try:
            zonegenerator, _ = dns.xfr.make_query(self,
                                                  keyring=self.keyring)
            dns.query.inbound_xfr(self.resolver,
                                  self,
                                  query=zonegenerator,
                                  port=self.resolver.port,
                                  timeout=60,
                                  udp_mode=dns.UDPMode.TRY_FIRST,
                                  lifetime=600)
        except dns.resolver.NoAnswer:
            self.logger.critical("Nameserver for '%s' did not answer.", domain)
            return None
        except dns.exception.FormError:
            self.logger.critical("Domain '%s' does not allow xfr requests.",
                                 domain)
            return None
        return True


    def get_dns_zone_serial(self) -> int:
        """
        Gets the current serial for a given zone
        :param domain: Domain to query for in DNS
        :param resolver: Nameserver to query in DNS, defaults to None
        :returns: Serial of the zones SOA record
        """
        domain = self.domain
        # Makes domains better resolvable
        if not domain == "@":
            domain = domain + "."

        soa_answer = 0

        # Resolve SOA directly without transferring the zone first
        try:
            soa_answer = self.resolver.resolve(domain, 'SOA')
        except (dns.resolver.NoAnswer, dns.resolver.NXDOMAIN):
            self.logger.warning("The SOA for domain '%s' is not resolvable",
                                domain)
        try:
            a_answer = dns.resolver.Resolver().resolve(soa_answer[0].mname, 'A')
        except (dns.resolver.NoAnswer, dns.resolver.NXDOMAIN):
            self.logger.warning("The domain '%s' is not resolvable via nameserver in SOA",
                                soa_answer[0].mname)

        if soa_answer:
            return soa_answer[0].serial
        return 0


    def restore_from_file(self, serial: int, zonefile: None) -> int:
        """
        Loads a zonebackup previously created to enable incremental zone transfers
        :param serial: Serial of the zone according to GNS
        :returns: Serial of the restored zone or 0 if loading failed
        """
        try:
            if zonefile:
                zf = zonefile
            else:
                zf = self.zone_backup_file
            self.zone = dns.zone.from_file(zf, origin=self.domain)
            self.logger.info("Zonebackup file %s loaded", zf)
        except FileNotFoundError:
            self.logger.info("Zonebackup file was not found, will be created")
            return 0
        return self.zone.get_soa().serial


    def backup_to_file(self) -> None:
        """
        Saves a zonebackup for later use with IXFR
        """
        self.logger.info(
            "Writing zone backup file %s to use for IXFR later",
            self.zone_backup_file
        )
        self.zone.to_file(self.zone_backup_file)


    def transfer_zone(self, gns_zone_serial: int):
        """
        Transfers the DNS Zone via AXFR or IXFR should gns_zone_serial be available
        """
        if gns_zone_serial == 0:
            self.zone = dns.zone.Zone(self.domain)
        assert self.zone != None
        self.logger.info("Preparing zonegenerator for transferring %s", self.domain)
        zonegenerator, _ = dns.xfr.make_query(
            self.zone,
            serial=gns_zone_serial,
            keyring=self.keyring,
        )

        self.logger.info("Transferring Zone %s", self.domain)
        dns.query.inbound_xfr(self.transferns,
                              self.zone,
                              query=zonegenerator,
                              port=self.resolver.port)

    def get_zone_soa(self) -> dns.rdatatype.SOA:
        """
        Fetches soa record from zone a given dnspython zone
        :param zone: A dnspython zone
        :returns: SOA record of given zone
        """
        for soarecord in self.zone.iterate_rdatas(rdtype=dns.rdatatype.SOA):
            if str(soarecord[0]) == '@':
                return soarecord
        return None


    def resolve_glue(self, authorityname: str) -> list[str]:
        """
        Resolves IP Adresses within zone
        :param authorityname:
        :returns: list of $nameserver@$ip_address that are contained in the zone
        """
        try:
            rdsets = self.zone[authorityname].rdatasets
        except KeyError:
            return []
        glue = []
        for rdataset in rdsets:
            if rdataset.rdtype in [dns.rdatatype.A, dns.rdatatype.AAAA]:
                for rdata in rdataset:
                    glue.append(f"{authorityname}.{self.domain}@{str(rdata)}")
        return glue
