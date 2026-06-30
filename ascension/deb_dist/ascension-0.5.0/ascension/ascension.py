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
    ascension <domain> [-d] [-p] [-s] [--minimum-ttl=<ttl>]
    ascension <domain> <port> [-d] [-p] [-s] [--minimum-ttl=<ttl>]
    ascension <domain> -n <transferns> [-d] [-p] [-s] [--minimum-ttl=<ttl>]
    ascension <domain> -n <transferns> <port> [-d] [-p] [-s] [--minimum-ttl=<ttl>]
    ascension -p | --public
    ascension -s | --standalone
    ascension -h | --help
    ascension -v | --version

Options:
    <domain>              Domain to migrate
    <port>                Port for zone transfer
    <transferns>          DNS Server that does the zone transfer
    --minimum-ttl=<ttl>   Minimum TTL for records to migrate [default: 3600]
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
# TODO find better solution for ignoring DNSSEC record types
SUPPORTED_RECORD_TYPES = [
    "A", "AAAA", "NS", "MX", "SRV", "TXT", "CNAME"
]

class Ascender():
    """
    Class that provides migration for any given domain
    """
    @classmethod
    def __init__(cls, domain, transferns, port, flags, minimum):
        cls.domain = domain
        if domain[-1] == '.':
            cls.domain = cls.domain[:-1]
        cls.port = int(port)
        cls.transferns = transferns
        cls.soa = None
        cls.tld = cls.domain.split(".")[::-1][0]
        cls.zone = None
        cls.zonegenerator = None
        cls.nscache = dict()
        cls.flags = flags
        cls.minimum = int(minimum)
        cls.subzonedict = dict()

    @classmethod
    def initial_zone_transfer(cls, serial=None):
        """
        Initialize the zone transfer generator
        :param serial: The serial to base the transfer on
        """
        if serial:
            cls.zonegenerator = dns.query.xfr(cls.transferns,
                                              cls.domain,
                                              rdtype=dns.rdatatype.IXFR,
                                              serial=serial,
                                              port=cls.port)
        else:
            cls.zonegenerator = dns.query.xfr(cls.transferns,
                                              cls.domain,
                                              port=cls.port)

    @classmethod
    def bootstrap_zone(cls):
        """
        Creates the zone in gnunet
        """
        try:
            ret = sp.run([GNUNET_ZONE_CREATION_COMMAND,
                          '-C', cls.domain])
            logging.info("executed command: %s", " ".join(ret.args))
        except sp.CalledProcessError:
            logging.info("Zone %s already exists!", cls.domain)

    @classmethod
    def get_current_serial(cls, domain, resolver=None):
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
        except dns.resolver.NoAnswer:
            logging.critical("the domain '%s' does not exist", domain)
            sys.exit(1)
        except dns.resolver.NXDOMAIN:
            logging.critical("the domain '%s' is invalid", domain)
            sys.exit(1)
        master_answer = dns.resolver.query(soa_answer[0].mname, 'A')
        try:
            if resolver:
                zone = dns.zone.from_xfr(dns.query.xfr(
                    resolver, domain, port=cls.port))
            else:
                zone = dns.zone.from_xfr(dns.query.xfr(
                    master_answer[0].address, domain,
                    port=cls.port))
        except dns.resolver.NoAnswer:
            logging.error("nameserver for '%s' did not answer", domain)
        except dns.exception.FormError:
            logging.critical("domain '%s' does not allow xfr requests", domain)
            sys.exit(1)
        except dns.query.TransferError:
            logging.critical("domain '%s' does not allow xfr requests", domain)
            sys.exit(1)
        for soa_record in zone.iterate_rdatas(rdtype=dns.rdatatype.SOA):
            if not cls.transferns:
                mname = soa_record[2].mname
                if cls.domain not in mname:
                    cls.transferns = str(soa_record[2].mname) + "." + domain
                else:
                    cls.transferns = str(soa_record[2].mname)
            return soa_record[2].serial

    @classmethod
    def mirror_zone(cls):
        """
        Extract necessary information from Generator
        """
        currentserial = int(cls.get_current_serial(cls.domain, cls.transferns))
        zoneserial = int(cls.get_zone_serial())
        if zoneserial == 0:
            logging.info("zone does not exist yet")
            cls.initial_zone_transfer()
            try:
                cls.zone = dns.zone.from_xfr(cls.zonegenerator,
                                             check_origin=False)
            except dns.zone.BadZone:
                logging.critical("Malformed DNS Zone '%s'", cls.domain)
            cls.soa = cls.get_zone_soa(cls.zone)
        elif zoneserial < currentserial:
            logging.info("zone is out of date")
            cls.initial_zone_transfer(serial=zoneserial)
            try:
                cls.zone = dns.zone.from_xfr(cls.zonegenerator)
            except dns.zone.BadZone:
                logging.critical("Malformed DNS Zone '%s'", cls.domain)
            cls.soa = cls.get_zone_soa(cls.zone)
        elif zoneserial == currentserial:
            logging.info("zone is up to date")
        # should be unnecessary but AXFR SOA might not be equal to direct SOA
        else:
            # because it runs as a daemon, ignore this case but log it
            logging.warning("SOA serial is bigger than zone serial?")
            logging.warning("zone: %s, current: %s", zoneserial, currentserial)

    @classmethod
    def add_records_to_gns(cls):
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
                domain = None

                labelrecords = taskqueue.get()
                # break if taskqueue is empty
                if labelrecords is None:
                    break

                # execute thing to run on item
                label, listofrdatasets = labelrecords
                subzones = label.split('.')
                domain = cls.domain

                if len(subzones) > 1:
                    ttl = cls.get_zone_refresh_time()
                    label = subzones[0]
                    subdomains = ".".join(subzones[1:])
                    subzone = "%s.%s" % (subdomains, domain)
                    fqdn = "%s.%s.%s" % (label, subdomains, domain)
                    if fqdn in cls.subzonedict.keys():
                        label = "@"
                        domain = fqdn
                    elif subzone in cls.subzonedict.keys():
                        domain = subzone

                for rdataset in listofrdatasets:
                    for record in rdataset:
                        rdtype = dns.rdatatype.to_text(record.rdtype)
                        if rdtype not in SUPPORTED_RECORD_TYPES:
                            continue

                        try:
                            if rdataset.ttl <= cls.minimum:
                                ttl = cls.minimum
                            else:
                                ttl = rdataset.ttl
                        except AttributeError:
                            ttl = cls.minimum

                        value = str(record)

                        # ignore NS for itself here
                        if label == '@' and rdtype == 'NS':
                            logging.info("ignoring NS record for itself")

                        # modify value to fit gns syntax
                        rdtype, value, label = \
                            cls.transform_to_gns_format(record,
                                                        rdtype,
                                                        domain,
                                                        label)
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
                                                   cls.flags,
                                                   element))
                        else:
                            # build recordline
                            recordline.append("-R")

                            # TODO possible regression here; maybe use a separate
                            # list to pass those arguments to prevent quoting
                            # issues in the future.
                            recordline.append('%d %s %s %s' %
                                              (int(ttl),
                                               rdtype,
                                               cls.flags,
                                               value))

                # add recordline to gns and filter out empty lines
                if len(recordline) > 1:
                    cls.add_recordline_to_gns(recordline,
                                              domain,
                                              label)

                taskqueue.task_done()

        # Check if there is zone has already been migrated
        nsrecords = cls.zone.iterate_rdatas(dns.rdatatype.NS)

        gnspkey = list(filter(lambda record: str(record[2]).startswith('gns--pkey--'), nsrecords))
        if gnspkey:
            label = str(gnspkey[0][0])
            ttl = gnspkey[0][1]
            pkey = str(gnspkey[0][2])
            # TODO Check this check
            if not cls.transferns in ['127.0.0.1', '::1', 'localhost']:
                logging.warning("zone exists in GNS, adding it to local store")
                cls.add_pkey_record_to_zone(pkey[11:], cls.domain,
                                            label, ttl)
                return

        # Unify all records under same label into datastructure
        customrdataset = dict()
        for remaining in cls.zone.iterate_rdatasets():
            # build lookup table for later GNS2DNS records
            domain = "%s.%s" % (str(remaining[0]), cls.domain)
            elementlist = []
            for element in remaining[1]:
                if dns.rdatatype.to_text(element.rdtype) in ['A', 'AAAA']:
                    elementlist.append(str(element))
            cls.nscache[str(domain)] = elementlist
            rdataset = remaining[1]
            if customrdataset.get(str(remaining[0])) is None:
                work = list()
                work.append(rdataset)
                customrdataset[str(remaining[0])] = work
            else:
                customrdataset[str(remaining[0])].append(rdataset)

        for label, value in customrdataset.items():
            if value is None:
                continue

            subzones = label.split('.')
            label = subzones[0]
            subdomain = ".".join(subzones[1:])
            zonename = "%s.%s" % (subdomain, cls.domain)

            refresh = cls.get_zone_refresh_time()
            if refresh <= cls.minimum:
                ttl = cls.minimum
            else:
                ttl = refresh

            if len(subzones) > 1:
                if cls.subzonedict.get(zonename):
                    continue
                else:
                    cls.subzonedict[zonename] = (False, ttl)

        cls.create_zone_hierarchy()

        # Create one thread
        thread = threading.Thread(target=worker)
        thread.start()

        # add records
        for label, value in customrdataset.items():
            if value is None:
                continue
            taskqueue.put((label, value))

        # Block until all tasks are done
        taskqueue.join()

        # Stop workers and threads
        taskqueue.put(None)
        thread.join(timeout=10)
        if thread.is_alive():
            logging.critical("thread join timed out, still running")

        # Add soa record to GNS once completed (updates the previous one)
        soa = cls.get_zone_soa(cls.zone)
        cls.add_soa_record_to_gns(soa)
        logging.info("All records have been added!")

    @staticmethod
    def add_recordline_to_gns(recordline, zonename, label):
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

    @classmethod
    def transform_to_gns_format(cls, record, rdtype, zonename, label):
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
            authns += cls.tld
            owner += cls.tld
            value = "rname=%s.%s mname=%s.%s %d,%d,%d,%d,%d" % (
                authns, zonename, owner, zonename,
                int(serial), int(refresh), int(retry),
                int(expiry), int(irefresh)
            )
        elif rdtype in ['CNAME']:
            if value[-1] == ".":
                value = value[:-1]
            else:
                value = "%s.%s" % (value, zonename)
        elif rdtype == 'NS':
            nameserver = str(record)
            if value[-1] == ".":
                value = value[:-1]
            else:
                value = "%s.%s" % (value, zonename)
            if zonename[-1] == ".":
                zonename = zonename[:-1]
            if nameserver[-1] == ".":
                dnsresolver = nameserver[:-1]
                dnsresolver = cls.nscache.get(dnsresolver, dnsresolver)
            else:
                dnsresolver = "%s.%s" % (nameserver, zonename)
                dnsresolver = cls.nscache.get(dnsresolver, dnsresolver)
            if isinstance(dnsresolver, list):
                value = []
                for nsip in dnsresolver:
                    value.append("%s@%s" % (zonename, nsip))
            else:
                value = '%s.%s@%s' % (str(label), zonename, dnsresolver)

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

    @classmethod
    def get_zone_serial(cls):
        """
        Fetches the zones serial from GNS
        :returns: serial of the SOA record in GNS
        """
        try:
            serial = sp.check_output([GNUNET_GNS_COMMAND,
                                      '-t', 'SOA',
                                      '-u', '@.%s' % cls.domain,])
            serial = serial.decode()
        except sp.CalledProcessError:
            serial = ""
            soa_serial = 0
        soapattern = re.compile(r'.+\s(\d+),\d+,\d+,\d+,\d+', re.M)
        if re.findall(soapattern, serial):
            soa_serial = re.findall(soapattern, serial)[0]
        else:
            soa_serial = 0
        return soa_serial

    @classmethod
    def get_zone_soa_expiry(cls):
        """
        Extracts the current serial from the class SOA
        :returns: refresh time of the current SOA record
        """
        ttlpattern = re.compile(r'.+\s\d+\s(\d+)\s\d+\s\d+\s\d+', re.M)
        return re.findall(ttlpattern, str(cls.soa[2]))

    @classmethod
    def get_zone_refresh_time(cls):
        """
        Extracts the current refresh time of the zone from GNS
        :returns: refresh time of the current SOA record
        """
        try:
            serial = sp.check_output([GNUNET_GNS_COMMAND,
                                      '-t', 'SOA',
                                      '-u', '@.%s' % cls.domain])
            serial = serial.decode()
        except sp.CalledProcessError:
            serial = ""
            refresh = 0
        soapattern = re.compile(r'.+\s\d+,(\d+),\d+,\d+,\d+', re.M)
        if re.findall(soapattern, serial):
            refresh = re.findall(soapattern, serial)[0]
        else:
            refresh = 0
        return refresh

    @classmethod
    def get_zone_retry_time(cls):
        """
        Extracts the current retry time of the zone from GNS
        :returns: retry time of the current SOA record
        """
        try:
            serial = sp.check_output([GNUNET_GNS_COMMAND,
                                      '-t', 'SOA',
                                      '-u', '@.%s' % cls.domain])
            serial = serial.decode()
        except sp.CalledProcessError:
            serial = ""
            retry = 300
        soapattern = re.compile(r'.+\s\d+,\d+,(\d+),\d+,\d+', re.M)
        if re.findall(soapattern, serial):
            retry = re.findall(soapattern, serial)[0]
        else:
            retry = 300
        return retry

    @staticmethod
    def get_zone_soa(zone):
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

    @classmethod
    def add_soa_record_to_gns(cls, record):
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
            authns = "%s.%s" % (authns, cls.domain)
        if owner[-1] == '.':
            owner = owner[:-1]
        else:
            owner = "%s.%s" % (owner, cls.domain)

        value = "rname=%s mname=%s %s,%s,%s,%s,%s" % (authns,
                                                      owner,
                                                      serial,
                                                      refresh,
                                                      retry,
                                                      expiry,
                                                      irefresh)
        recordval = '%s %s %s %s' % (ttl, "SOA", cls.flags, str(value))
        recordline = ['-R', recordval]
        cls.add_recordline_to_gns(recordline, cls.domain, str(label))

    @staticmethod
    def create_zone_and_get_pkey(zonestring):
        """
        Creates the zone in zonestring and returns pkey
        :param zonestring: The label name of the zone
        :returns: gnunet pkey of the zone
        """
        try:
            ret = sp.run([GNUNET_ZONE_CREATION_COMMAND,
                          '-C', zonestring],
                         stdout=sp.DEVNULL,
                         stderr=sp.DEVNULL)
            logging.info("executed command: %s", " ".join(ret.args))
        except sp.CalledProcessError:
            logging.info("Zone %s already exists!", zonestring)

        pkey_lookup = sp.Popen([GNUNET_ZONE_CREATION_COMMAND,
                                '-d'],
                               stdout=sp.PIPE)
        pkey_line = sp.Popen(['grep', '^' + zonestring],
                             stdin=pkey_lookup.stdout,
                             stdout=sp.PIPE)
        pkey_zone = sp.check_output(['cut', '-d',
                                     ' ', '-f3'],
                                    stdin=pkey_line.stdout)
        pkey_zone = pkey_zone.decode().strip()
        pkey_lookup.stdout.close()
        pkey_line.stdout.close()
        return pkey_zone

    @staticmethod
    def add_pkey_record_to_zone(pkey, domain, label, ttl):
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
                      '-e', "%ss" % ttl])
        logging.info("executed command: %s", debug)
        if ret.returncode != 0:
            logging.warning("failed to add PKEY record %s to %s",
                            label, domain)
        #logging.warning("PKEY record %s already exists in %s", label, domain)

    @classmethod
    def create_zone_hierarchy(cls):
        """
        Creates the zone hierarchy in GNS for label
        :param label: the split record to create zones for
        """
        domain = cls.domain

        zonelist = cls.subzonedict.items()
        sortedlist = sorted(zonelist, key=lambda s: len(str(s).split('.')))
        for zone, pkeyttltuple in sortedlist:
            pkey, ttl = pkeyttltuple
            if not pkey:
                domain = ".".join(zone.split('.')[1::])
                label = zone.split('.')[0]
                pkey = cls.create_zone_and_get_pkey(zone)
                logging.info("adding zone %s with %s pkey into %s", zone, pkey, domain)
                cls.add_pkey_record_to_zone(pkey, domain, label, pkeyttltuple[1])
                cls.subzonedict[zone] = (pkey, ttl)

def main():
    """
    Initializes object and handles arguments
    """
    # argument parsing from docstring definition
    args = docopt.docopt(__doc__, version='Ascension 0.5.0')

    # argument parsing
    debug = args['--debug']
    domain = args.get('<domain>', None)
    transferns = args['<transferns>'] if args['<transferns>'] else None
    port = args['<port>'] if args['<port>'] else 53
    flags = "p" if args.get('--public') else "n"
    standalone = bool(args.get('--standalone'))
    minimum = args['--minimum-ttl']

    # Change logging severity to debug
    if debug:
        logging.basicConfig(level=logging.DEBUG)

    # Checks if GNUnet services are running
    try:
        sp.check_output([GNUNET_ARM_COMMAND, '-I'], timeout=1)
    except sp.TimeoutExpired:
        logging.critical('GNUnet Services are not running!')
        sys.exit(1)

    # Initialize class instance
    ascender = Ascender(domain, transferns, port, flags, minimum)

    # Event loop for actual daemon
    while 1:
        serial = ascender.get_zone_serial()
        ascender.initial_zone_transfer(serial)
        ascender.mirror_zone()
        ascender.bootstrap_zone()
        if ascender.zone is not None:
            ascender.add_records_to_gns()
            logging.info("Finished migration of the zone %s", ascender.domain)
        else:
            logging.info("Zone %s already up to date", ascender.domain)
        refresh = int(ascender.get_zone_refresh_time())
        retry = int(ascender.get_zone_retry_time())
        if standalone:
            return 0
        if refresh == 0:
            logging.info("unable to refresh zone, retrying in %ds", retry)
            time.sleep(retry)
        else:
            logging.info("refreshing zone in %ds", refresh)
            print("refreshing zone in %ds" % refresh)
            time.sleep(refresh)

if __name__ == '__main__':
    main()
