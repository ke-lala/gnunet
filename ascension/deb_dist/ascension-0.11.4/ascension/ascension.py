#!/usr/bin/env python3
# This file is part of Ascension.
# Copyright (C) 2019 GNUnet e.V.
#
# Ascension is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# Ascension is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: AGPL3.0-or-later
#
# Author rexxnor
"""
Usage:
    ascension <domain> [-d] [-p] [-s] [--minimum-ttl=<ttl>] [--dry-run]
    ascension <domain> <port> [-d] [-p] [-s] [--minimum-ttl=<ttl>] [--dry-run]
    ascension <domain> -n <transferns> [-d] [-p] [-s] [--minimum-ttl=<ttl>] [--dry-run]
    ascension <domain> -n <transferns> <port> [-d] [-p] [-s] [--minimum-ttl=<ttl>] [--dry-run]
    ascension -p | --public
    ascension -d | --debug
    ascension -s | --standalone
    ascension -h | --help
    ascension -v | --version

Options:
    <domain>              Domain to migrate
    <port>                Port for zone transfer
    <transferns>          DNS Server that does the zone transfer
    --minimum-ttl=<ttl>   Minimum TTL for records to migrate [default: 3600]
    --dry-run             Only try if a zone transfer is allowed
    -p --public           Make records public on the DHT
    -s --standalone       Run ascension once
    -d --debug            Enable debugging
    -h --help         Show this screen.
    -v --version      Show version.
"""

# imports
import logging
import queue
import re
import socket
import sys
import time
import subprocess as sp
import threading
import dns.query
import dns.resolver
import dns.zone
import docopt

# GLOBALS
GNUNET_ZONE_CREATION_COMMAND = 'gnunet-identity'
GNUNET_NAMESTORE_COMMAND = 'gnunet-namestore'
GNUNET_GNS_COMMAND = 'gnunet-gns'
GNUNET_ARM_COMMAND = 'gnunet-arm'
# This is the list of record types Ascension (and GNS) currently
# explicitly supports.  Record types we encounter that are not
# in this list and not in the OBSOLETE_RECORD_TYPES list will
# create a warning (information loss during migration).
SUPPORTED_RECORD_TYPES = [
    "A", "AAAA", "NS", "MX", "SRV", "TXT", "CNAME"
]
# Record types that exist in DNS but that won't ever exist in GNS
# as they are not needed anymore (so we should not create a warning
# if we drop one of these).
OBSOLETE_RECORD_TYPES = [
    "PTR",
    "SIG", "KEY",
    "RRSIG", "NSEC", "DNSKEY", "NSEC3", "NSEC3PARAM", "CDNSKEY", "DS"
    "TKEY", "TSIG",
    "TA", "DLV",
]

class Ascender():
    """
    Class that provides migration for any given domain
    """
    def __init__(self,
                 domain: str,
                 transferns: str,
                 port: str,
                 flags: str,
                 minimum: str) -> None:
        self.domain = domain
        if domain[-1] == '.':
            self.domain = self.domain[:-1]
        self.port = int(port)
        self.transferns = transferns
        self.soa = None
        self.tld = self.domain.split(".")[::-1][0]
        self.zone = None
        self.zonegenerator = None
        self.flags = flags
        self.minimum = int(minimum)
        self.subzonedict = dict()

    def bootstrap_zone(self) -> None:
        """
        Creates the zone in gnunet
        """
        try:
            ret = sp.run([GNUNET_ZONE_CREATION_COMMAND,
                          '-C', self.domain,
                          '-V'],
                         stdout=sp.PIPE,
                         stderr=sp.DEVNULL)
            pkey = ret.stdout.decode().strip()
            self.subzonedict[self.domain] = (pkey, self.minimum)
            logging.info("executed command: %s", " ".join(ret.args))
        except sp.CalledProcessError:
            logging.info("Zone %s already exists!", self.domain)

    def get_dns_zone_serial(self,
                            domain: str,
                            resolver=None) -> int:
        """
        Gets the current serial for a given zone
        :param domain: Domain to query for in DNS
        :param resolver: Nameserver to query in DNS, defaults to None
        :returns: Serial of the zones SOA record
        """
        # Makes domains better resolvable
        domain = domain + "."
        # SOA is different if taken directly from SOA record
        # compared to AXFR/IXFR - changed to respect this
        try:
            soa_answer = dns.resolver.query(domain, 'SOA')
            master_answer = dns.resolver.query(soa_answer[0].mname, 'A')
        except dns.resolver.NoAnswer:
            logging.warning("The domain '%s' is not publicly resolvable.",
                            domain)
        except dns.resolver.NXDOMAIN:
            logging.warning("The domain '%s' is not publicly resolvable.",
                            domain)
        except Exception:
            logging.warning("The domain '%s' is not publicly resolvable.",
                            domain)

        try:
            if resolver:
                zone = dns.zone.from_xfr(dns.query.xfr(
                    resolver, domain, port=self.port))
            else:
                zone = dns.zone.from_xfr(dns.query.xfr(
                    master_answer[0].address, domain,
                    port=self.port))
        except dns.resolver.NoAnswer:
            logging.critical("Nameserver for '%s' did not answer.", domain)
            return None
        except dns.exception.FormError:
            logging.critical("Domain '%s' does not allow xfr requests.",
                             domain)
            return None
        except Exception:
            logging.error("Unexpected error while transfering domain '%s'",
                          domain)
            return None

        for soa_record in zone.iterate_rdatas(rdtype=dns.rdatatype.SOA):
            if not self.transferns:
                mname = soa_record[2].mname
                if self.domain not in mname:
                    self.transferns = str(soa_record[2].mname) + "." + domain
                else:
                    self.transferns = str(soa_record[2].mname)
            return int(soa_record[2].serial)

    def add_records_to_gns(self) -> None:
        """
        Extracts records from zone and adds them to GNS
        :raises AttributeError: When getting incomplete data
        """
        logging.info("Starting to add records into GNS...")

        # Defining FIFO Queue
        taskqueue = queue.Queue(maxsize=5)

        # Defining worker
        def worker():
            while True:
                # define recordline
                recordline = list()
                label = ""
                bestlabel = ""
                domain = None

                labelrecords = taskqueue.get()
                # break if taskqueue is empty
                if labelrecords is None:
                    break

                # execute thing to run on item
                label, listofrdatasets = labelrecords
                label = str(label)

                subzones = str(label).split('.')
                domain = self.domain
                bestlabel = label

                if len(subzones) > 1:
                    label = subzones[0]
                    subdomains = ".".join(subzones[1:])
                    subzone = "%s.%s" % (subdomains, domain)
                    fqdn = "%s.%s.%s" % (label, subdomains, domain)
                    if fqdn in self.subzonedict.keys():
                        label = "@"
                        domain = fqdn
                    elif subzone in self.subzonedict.keys():
                        domain = subzone
                    bestlabel = label

                for rdataset in listofrdatasets:
                    for record in rdataset:
                        rdtype = dns.rdatatype.to_text(record.rdtype)
                        if rdtype == "SOA":
                            continue
                        if rdtype not in SUPPORTED_RECORD_TYPES:
                            if rdtype not in OBSOLETE_RECORD_TYPES:
                                logging.critical("%s records not supported!",
                                                 rdtype)
                            continue

                        try:
                            if rdataset.ttl <= self.minimum:
                                ttl = self.minimum
                            else:
                                ttl = rdataset.ttl
                        except AttributeError:
                            ttl = self.minimum

                        value = str(record)

                        # ignore NS for itself here
                        if label == '@' and rdtype == 'NS':
                            logging.info("ignoring NS record for itself")

                        # modify value to fit gns syntax
                        rdtype, value, label = \
                            self.transform_to_gns_format(record,
                                                         rdtype,
                                                         domain,
                                                         bestlabel)
                        # skip record if value is none
                        if value is None:
                            continue

                        if isinstance(value, list):
                            for element in value:
                                # build recordline
                                recordline.append("-R")
                                recordline.append('%d %s %s %s' %
                                                  (int(ttl),
                                                   rdtype,
                                                   self.flags,
                                                   element))
                        else:
                            # build recordline
                            recordline.append("-R")
                            recordline.append('%d %s %s %s' %
                                              (int(ttl),
                                               rdtype,
                                               self.flags,
                                               value))

                # add recordline to gns and filter out empty lines
                if len(recordline) > 1:
                    self.add_recordline_to_gns(recordline,
                                               domain,
                                               label)

                taskqueue.task_done()
        # End of worker

        self.create_zone_hierarchy()

        # Create one thread
        thread = threading.Thread(target=worker)
        thread.start()

        # add records
        for name, rdatasets in self.zone.nodes.items():
            # log if the rdataset is empty for some reason
            if not rdatasets:
                logging.warning("Empty Rdataset!")
                continue
            taskqueue.put((name, rdatasets))

        # Block until all tasks are done
        taskqueue.join()

        # Stop workers and threads
        taskqueue.put(None)
        thread.join(timeout=10)
        if thread.is_alive():
            logging.critical("thread join timed out, still running")

        # Add soa record to GNS once completed (updates the previous one)
        self.add_soa_record_to_gns(self.soa)

        logging.info("All records have been added!")

    @staticmethod
    def add_recordline_to_gns(recordline: list,
                              zonename: str,
                              label: str) -> None:
        """
        Replaces records in zone or adds them if not
        :param recordline: records to replace as list in form
        ['-R', 'TTL TYPE FLAGS VALUE']
        :param zonename: zonename of zone to add records to
        :param label: label under which to add the records
        """
        logging.info("trying to add %d records with name %s",
                     len(recordline)/2, label)

        ret = sp.run([GNUNET_NAMESTORE_COMMAND,
                      '-z', zonename,
                      '-n', str(label),
                      ] + recordline)

        if ret.returncode != 0:
            logging.warning("failed adding record with name %s",
                            ' '.join(ret.args))
        else:
            logging.info("successfully added record with command %s",
                         ' '.join(ret.args))

    def resolve_glue(self,
                     authorityname: str) -> list:
        """
        Resolves IP Adresses within zone
        :param authorityname:
        """
        try:
            rdsets = self.zone[authorityname].rdatasets
        except KeyError:
            return []
        value = []
        for rdataset in rdsets:
            if rdataset.rdtype in [dns.rdatatype.A, dns.rdatatype.AAAA]:
                for rdata in rdataset:
                    value.append("%s.%s@%s" % (authorityname,
                                               self.domain,
                                               str(rdata)))
        return value

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
        if rdtype == 'SOA':
            zonetuple = str(value).split(' ')
            authns, owner, serial, refresh, retry, expiry, irefresh = zonetuple
            if authns[-1] == '.':
                authns = authns[:-1]
            if owner[-1] == '.':
                owner = owner[:-1]
            # hacky and might cause bugs
            authns += self.tld
            owner += self.tld
            value = "rname=%s.%s mname=%s.%s %d,%d,%d,%d,%d" % (
                authns, zonename, owner, zonename,
                int(serial), int(refresh), int(retry),
                int(expiry), int(irefresh)
            )
        elif rdtype in ['CNAME']:
            if value[-1] == ".":
                value = value[:-1]
            else:
                value = "%s.%s" % (value, self.domain)
        elif rdtype == 'NS':
            if self.subzonedict.get(str(label) + "." + zonename):
                return (None, None, None)
            nameserver = str(record.target)
            if nameserver[-1] == ".":
                nameserver = nameserver[:-1]
            if value[-1] == ".":
                # FQDN provided
                if value.endswith(".%s." % zonename):
                    # in bailiwick
                    value = self.resolve_glue(record.target)
                else:
                     # out of bailiwick
                    value = '%s@%s' % (zonename, nameserver)
            else:
                # Name is relative to zone, must be in bailiwick
                value = self.resolve_glue(record.target)
                if not value:
                    if label.startswith("@"):
                        value = '%s@%s.%s' % (self.domain,
                                              record.target,
                                              self.domain)
                    else:
                        value = '%s.%s@%s.%s' % (str(label), self.domain,
                                                 record.target, self.domain)

            logging.info("transformed %s record to GNS2DNS format", rdtype)
            rdtype = 'GNS2DNS'
        elif rdtype == 'MX':
            priority, mailserver = str(value).split(' ')
            if mailserver[-1] == ".":
                mailserver = mailserver[:-1]
            mailserver = '%s.%s' % (mailserver, zonename)
            value = '%s,%s' % (priority, mailserver)
            logging.info("transformed %s record to GNS format", rdtype)
        elif rdtype == 'SRV':
            # this is the number for a SRV record
            rdtype = 'BOX'
            srv = 33

            # tearing the record apart
            try:
                srvrecord = str(label).split('.')
                proto = srvrecord[1]
            except IndexError:
                logging.warning("could not parse SRV label %s", label)
                return (rdtype, None, None)
            priority, weight, destport, target = value.split(' ')

            try:
                protostring = proto.strip('_')
                protonum = socket.getprotobyname(protostring)
            except OSError:
                logging.warning("invalid protocol: %s", protostring)
                return (rdtype, None, None)

            if target[:-1] == ".":
                value = '%s %s %s %s %s %s %s' % (
                    destport, protonum, srv, priority, weight, destport,
                    "%s" % target
                )
            else:
                value = '%s %s %s %s %s %s %s' % (
                    destport, protonum, srv, priority, weight, destport,
                    "%s.%s" % (target, zonename)
                )

            label = target
        else:
            logging.info("Did not transform record of type: %s", rdtype)
        return (rdtype, value, label)

    def get_gns_zone_serial(self) -> int:
        """
        Fetches the zones serial from GNS
        :returns: serial of the SOA record in GNS
        """
        try:
            #serial = sp.check_output([GNUNET_GNS_COMMAND,
            #                          '-t', 'SOA',
            #                          '-u', '%s' % self.domain,])
            serial = sp.check_output([GNUNET_NAMESTORE_COMMAND,
                                      '-D',
                                      '-z', self.domain,
                                      '-t', 'SOA',
                                      '-n', '@'])
            serial = serial.decode()
        except sp.CalledProcessError:
            serial = ""
            soa_serial = 0
        soapattern = re.compile(r'.+\s(\d+),\d+,\d+,\d+,\d+', re.M)
        if re.findall(soapattern, serial):
            soa_serial = re.findall(soapattern, serial)[0]
        else:
            soa_serial = 0
        return int(soa_serial)

    @staticmethod
    def get_zone_soa(zone) -> dns.rdatatype.SOA:
        """
        Fetches soa record from zone a given zone
        :param zone: A dnspython zone
        :returns: SOA record of given zone
        """
        soa = None
        for soarecord in zone.iterate_rdatas(rdtype=dns.rdatatype.SOA):
            if str(soarecord[0]) == '@':
                soa = soarecord
        return soa

    def add_soa_record_to_gns(self, record) -> None:
        """
        Adds a SOA record to GNS
        :param record: The record to add
        """
        label, ttl, rdata = record
        zonetuple = str(rdata).split(' ')
        authns, owner, serial, refresh, retry, expiry, irefresh = zonetuple
        if authns[-1] == '.':
            authns = authns[:-1]
        else:
            authns = "%s.%s" % (authns, self.domain)
        if owner[-1] == '.':
            owner = owner[:-1]
        else:
            owner = "%s.%s" % (owner, self.domain)

        value = "rname=%s mname=%s %s,%s,%s,%s,%s" % (authns,
                                                      owner,
                                                      serial,
                                                      refresh,
                                                      retry,
                                                      expiry,
                                                      irefresh)
        # Deleting old SOA record and ignoring errors
        sp.run([GNUNET_NAMESTORE_COMMAND,
                '-d',
                '-z', self.domain,
                '-n', str(label),
                '-t', "SOA",],
               stderr=sp.DEVNULL)
        logging.info("Deleted old SOA record")
        # Adding new SOA record
        sp.run([GNUNET_NAMESTORE_COMMAND,
                '-a',
                '-z', self.domain,
                '-n', str(label),
                '-t', "SOA",
                '-V', value,
                '-e', "%ss" % str(self.minimum)])
        logging.info("Added new SOA record")

    @staticmethod
    def create_zone_and_get_pkey(zonestring: str) -> str:
        """
        Creates the zone in zonestring and returns pkey
        :param zonestring: The label name of the zone
        :returns: gnunet pkey of the zone
        """
        try:
            ret = sp.run([GNUNET_ZONE_CREATION_COMMAND,
                          '-C', zonestring,
                          '-V'],
                         stdout=sp.PIPE,
                         stderr=sp.DEVNULL,
                         check=True)
            logging.info("executed command: %s", " ".join(ret.args))
            pkey_zone = ret.stdout.decode().strip()
        except sp.CalledProcessError:
            ret = sp.run([GNUNET_ZONE_CREATION_COMMAND,
                          '-dq',
                          '-e', zonestring],
                         stdout=sp.PIPE)
            logging.info("executed command: %s", " ".join(ret.args))
            pkey_zone = ret.stdout.decode().strip()
        return pkey_zone

    @staticmethod
    def add_pkey_record_to_zone(pkey: str,
                                domain: str,
                                label: str,
                                ttl: str) -> None:
        """
        Adds the pkey of the subzone to the parent zone
        :param pkey: the public key of the child zone
        :param domain: the name of the parent zone
        :param label: the label under which to add the pkey
        :param ttl: the time to live the record should have
        """
        #lookup = sp.run(GNUNET_NAMESTORE_COMMAND,
        #                '-z', domain,
        #                '-n', label,
        #                '-t', 'PKEY')
        #if not 'Got result:' in lookup.stdout:
        debug = " ".join([GNUNET_NAMESTORE_COMMAND,
                          '-z', domain,
                          '-a', '-n', label,
                          '-t', 'PKEY',
                          '-V', pkey,
                          '-e', "%ss" % ttl])
        ret = sp.run([GNUNET_NAMESTORE_COMMAND,
                      '-z', domain,
                      '-a', '-n', label,
                      '-t', 'PKEY',
                      '-V', pkey,
                      '-e', "%ss" % ttl],
                     stdout=sp.DEVNULL,
                     stderr=sp.DEVNULL)
        logging.info("executed command: %s", debug)
        if ret.returncode != 0:
            # FIXME: extend gnunet-namestore to return *specific* error code for
            # "record already exists", and in that case reduce log level to DEBUG here.
            logging.info("failed to add PKEY record %s to %s",
                         label, domain)
        #logging.warning("PKEY record %s already exists in %s", label, domain)

    def create_zone_hierarchy(self) -> None:
        """
        Creates the zone hierarchy in GNS for label
        """
        # Extend Dictionary using GNS identities that already exist,
        # checking for conflicts with information in DNS
        ids = sp.run([GNUNET_ZONE_CREATION_COMMAND, '-d'], stdout=sp.PIPE)
        domainlist = ''.join(col for col in ids.stdout.decode()).split('\n')
        # Filter for domains relevant for us, i.e. that end in self.domain
        altdomainlist = [e for e in domainlist if self.domain + " " in e]
        for zone in altdomainlist:
            zonename, _, pkey = zone.split(" ")
            self.subzonedict[zonename] = (pkey, self.minimum)

        # Create missing zones (and add to dict) for GNS zones that are NOT DNS zones
        # ("." is not a zone-cut in DNS, but always in GNS).
        for name in self.zone.nodes.keys():
            subzones = str(name).split('.')
            for i in range(1, len(subzones)):
                subdomain = ".".join(subzones[i:])
                zonename = "%s.%s" % (subdomain, self.domain)
                ttl = self.minimum # new record, cannot use existing one, might want to use larger value
                if self.subzonedict.get(zonename) is None:
                    pkey = self.create_zone_and_get_pkey(zonename)
                    self.subzonedict[zonename] = (pkey, ttl)

        # Check if a delegated zone is available in GNS as per NS record
        # Adds NS records that contain "gns--pkey--" to dictionary
        nsrecords = self.zone.iterate_rdatasets(dns.rdatatype.NS)
        for nsrecord in nsrecords:
            name = str(nsrecord[0])
            values = nsrecord[1]
            ttl = values.ttl

            gnspkeys = list(filter(lambda record:
                                   str(record).startswith('gns--pkey--'),
                                   values))
            num_gnspkeys = len(gnspkeys)
            if not num_gnspkeys:
                # skip empty values
                continue
            if num_gnspkeys > 1:
                logging.critical("Detected ambiguous PKEY records for label \
                                  %s (not generating PKEY record)", name)
                continue
            gnspkey = str(gnspkeys[0])

            # FIXME: test strlen(gnspkey) "right length", theoretically:
            # Crockford base32 decoder... -> base32-crockford looks promising
            zone = "%s.%s" % (name, self.domain)
            if not self.subzonedict.get(zone):
                self.subzonedict[zone] = (gnspkey[11:], ttl)
            else:
                # This should be impossible!!?
                pkey_ttl = self.subzonedict[zone]
                pkey2, ttl = pkey_ttl
                if pkey2 != pkey:
                    logging.critical("PKEY in DNS does not match PKEY in GNS for name %s", name)
                    continue

        # Generate PKEY records for all entries in subzonedict
        for zone, pkeyttltuple in self.subzonedict.items():
            pkey, ttl = pkeyttltuple
            domain = ".".join(zone.split('.')[1::])
            # This happens if root is reached
            if domain == '':
                logging.info("Reached domain root")
                continue
            label = zone.split('.')[0]
            logging.info("adding zone %s with %s pkey into %s", zone, pkey, domain)
            self.add_pkey_record_to_zone(pkey, domain, label, ttl)

def main():
    """
    Initializes object and handles arguments
    """
    # argument parsing from docstring definition
    args = docopt.docopt(__doc__, version='Ascension 0.11.4')

    # argument parsing
    debug = args['--debug']
    domain = args.get('<domain>', None)
    transferns = args['<transferns>'] if args['<transferns>'] else None
    port = args['<port>'] if args['<port>'] else "53"
    flags = "p" if args.get('--public') else "n"
    standalone = bool(args.get('--standalone'))
    dryrun = bool(args.get('--dry-run'))
    minimum = args['--minimum-ttl']

    # Change logging severity to debug
    if debug:
        logging.basicConfig(level=logging.DEBUG)

    # Initialize class instance
    ascender = Ascender(domain, transferns, port, flags, minimum)

    # Do dry run before GNUnet check
    if dryrun:
        dns_zone_serial = ascender.get_dns_zone_serial(ascender.domain,
                                                       ascender.transferns)
        if dns_zone_serial is None:
            return 1
        else:
            return 0

    # Checks if GNUnet services are running
    try:
        sp.check_output([GNUNET_ARM_COMMAND, '-I'], timeout=1)
    except sp.TimeoutExpired:
        logging.critical('GNUnet services are not running!')
        sys.exit(1)

    # Set to defaults to use before we get a SOA for the first time
    retry = 300
    refresh = 300

    # Main loop for actual daemon
    while True:
        gns_zone_serial = ascender.get_gns_zone_serial()
        if gns_zone_serial:
            ascender.zonegenerator = dns.query.xfr(ascender.transferns,
                                                   ascender.domain,
                                                   rdtype=dns.rdatatype.IXFR,
                                                   serial=gns_zone_serial,
                                                   port=ascender.port)
        else:
            ascender.zonegenerator = dns.query.xfr(ascender.transferns,
                                                   ascender.domain,
                                                   port=ascender.port)
        dns_zone_serial = ascender.get_dns_zone_serial(ascender.domain,
                                                       ascender.transferns)

        if not dns_zone_serial:
            logging.error("Could not get DNS zone serial")
            if standalone:
                return 1
            time.sleep(retry)
            continue
        if not gns_zone_serial:
            logging.info("GNS zone does not exist yet, performing full transfer.")
            print("GNS zone does not exist yet, performing full transfer.")
            ascender.bootstrap_zone()
        elif gns_zone_serial == dns_zone_serial:
            logging.info("GNS zone is up to date.")
            print("GNS zone is up to date.")
            if standalone:
                return 0
            time.sleep(refresh)
            continue
        elif gns_zone_serial > dns_zone_serial:
            logging.critical("SOA serial in GNS is bigger than SOA serial in DNS?")
            logging.critical("GNS zone: %s, DNS zone: %s", gns_zone_serial, dns_zone_serial)
            if standalone:
                return 1
            time.sleep(retry)
            continue
        else:
            logging.info("GNS zone is out of date, performing incremental transfer.")
            print("GNS zone is out of date, performing incremental transfer.")

        try:
            ascender.zone = dns.zone.from_xfr(ascender.zonegenerator,
                                              check_origin=False)
            ascender.soa = ascender.get_zone_soa(ascender.zone)
            refresh = int(str(ascender.soa[2]).split(" ")[3])
            retry = int(str(ascender.soa[2]).split(" ")[4])
        except dns.zone.BadZone:
            logging.critical("Malformed DNS Zone '%s'", ascender.domain)
            if standalone:
                return 2
            time.sleep(retry)
            continue

        ascender.add_records_to_gns()
        logging.info("Finished migration of the zone %s", ascender.domain)

if __name__ == '__main__':
    main()
