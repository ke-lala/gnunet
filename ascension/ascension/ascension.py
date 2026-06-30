#!/usr/bin/env python3
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
import os
import time
import subprocess
import itertools
import multiprocessing as mp

import dns.rdatatype
import dns.zone

import ascension.util.argumentparser
import ascension.util.classes
import ascension.util.constants
import ascension.util.keyfile
import ascension.util.rest
import ascension.util.transformers

def work_slice(n,step,pp_setslice,gn_path):
    # TODO Check path / exit code? Config could be generated.
    worker_cfg = 'namestore-ascension-worker-' + str(n) + '.conf'
    with open(worker_cfg, 'w') as f:
        f.write('[namestore]\n')
        f.write('DATABASE = postgres\n')
        worker_sock = 'UNIXPATH = $GNUNET_USER_RUNTIME_DIR/gnunet-service-namestore-'+str(n)+'.sock'
        f.write(worker_sock)

    ns_svc_full_path = os.path.join(gn_path, 'gnunet', 'libexec', 'gnunet-service-namestore')
    ns_svc_process = subprocess.Popen([ns_svc_full_path, '-c', worker_cfg])
    ns_process = subprocess.Popen(["gnunet-namestore", "-B", str(step), "-a", "-S", "-c", worker_cfg], stdin=subprocess.PIPE, text=True)
    start = time.time()
    i = 0
    j = 0
    psetcount = len(pp_setslice)
    for name, payload in pp_setslice.items():
        # log if the rdataset is empty for some reason
        i += 1
        if not payload:
            print("Empty Rdataset!")
            continue
        j += len(payload.data)
        if (i % step) == 0:
            print("Worker #%d: Adding record set %d/%d for a total of %d records\n"%(n,i,psetcount,j), end="")
        ns_process.stdin.write(name + ":\n")
        for r in payload.data:
            flags = "[r{}]".format('p' if not r.is_private else '')
            # FIXME we have many more flags. but probably not in our use
            # case? We always have relative expirations, for example.
            ns_process.stdin.write("{} {} {} {}\n".format(r.record_type,
                                                   r.relative_expiration,
                                                   flags,
                                                   r.value))
    ns_process.stdin.close()
    ns_process.wait()
    ns_svc_process.terminate()
    os.remove(worker_cfg)

class Ascension():
    """
    Provides migration utilities for any given domain that supports zone transfer
    """
    def __init__(self, args: argparse.Namespace):
        """Constructor initializing all the classes and variables needed"""
        # Logging
        logging.basicConfig()
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(int(args.loglevel))
        domain = args.domain

        # special case for root zone
        if args.domain[-1] == '.' and len(args.domain) == 1:
            domain = '@'
        if args.domain[-1] == '.':
            domain = domain[:-1]

        self.rrsetcount = 0
        self.subzonedict = {}
        self.gnunet_prefix = args.gnunetprefix
        self.num_workers = int(args.workers)
        self.batch_size = int(args.batchsize)

        self.session = ascension.util.rest.GNUnetRestSession()
        self.gnszone = ascension.util.classes.GNSZone(
            self.session, domain, args.public, args.ttl, self.logger
        )
        self.dnszone = ascension.util.classes.DNSZone(
            domain, args.nameserver, args.port, args.keyfile
        )
        self.transformer = ascension.util.transformers.Transformer(
            domain,
            self.dnszone
        )



    def add_records_to_gns(self) -> None:
        """
        Extracts records from transferred zone and adds them to GNS
        :raises AttributeError: When getting incomplete data
        """
        self.logger.info("Starting to add records into GNS...")
        self.rrsetcount = 0

        # Defining worker
        def worker(labelrecords):
            label = ""
            bestlabel = ""
            domain = None

            # break if taskqueue is empty
            if not labelrecords:
                return

            record_data = ascension.util.classes.GNSRRecordSet(
                record_name=label,
                data=[]
            )

            # execute thing to run on item
            label, listofrdatasets = labelrecords
            label = str(label)

            subzones = str(label).split('.')
            domain = self.gnszone.domain
            bestlabel = label

            if len(subzones) > 1:
                label = subzones[0]
                subdomains = ".".join(subzones[1:])
                subzone = f"{subdomains}.{domain}"
                fqdn = f"{label}.{subdomains}.{domain}"
                if subzone.startswith(("_tcp", "_udp")):
                    subzone = subzone.lstrip('_tcp.')
                    subzone = subzone.lstrip('_udp.')
                bestlabel = label
                if fqdn in self.subzonedict:
                    label = "@"
                    domain = fqdn
                elif subzone in self.subzonedict:
                    if any(proto in fqdn for proto in ('_tcp', '_udp')):
                        fragment = fqdn.split('.')
                        bestlabel = '.'.join(fragment[0:2])
                        domain = '.'.join(fragment[2:])
                    else:
                        domain = subzone

            for rdataset in listofrdatasets:
                for record in rdataset:
                    rdtype = dns.rdatatype.to_text(record.rdtype)
                    if rdtype not in ascension.util.constants.PROCESSABLE_RECORD_TYPES:
                        self.logger.debug("%s records not supported!", rdtype)
                        continue

                    try:
                        if rdataset.ttl <= self.gnszone.minimum:
                            ttl = self.gnszone.minimum
                        else:
                            ttl = rdataset.ttl
                    except AttributeError:
                        ttl = self.gnszone.minimum

                    value = str(record)

                    # ignore NS for itself here
                    if label == '@' and rdtype == 'NS':
                        self.logger.debug("ignoring NS record for itself")

                    # modify value to fit gns syntax
                    rdtype, value, label = \
                        self.transformer.transform_to_gns_format(record,
                                                                 rdtype,
                                                                 domain,
                                                                 bestlabel)
                    # skip record if value is none
                    if value is None:
                        continue

                    # if label has changed, adjust GNSRecordData label as well
                    if record_data.record_name != label:
                        record_data.record_name = label

                    if isinstance(value, list):
                        for element in value:
                            entry = ascension.util.classes.GNSRecordData(
                                value=element,
                                record_type=rdtype,
                                relative_expiration=ttl,
                                is_relative_expiration=True,
                                is_private=not self.gnszone.public
                            )
                            record_data.data.append(entry)
                    else:
                        entry = ascension.util.classes.GNSRecordData(
                            value=value,
                            record_type=rdtype,
                            relative_expiration=ttl,
                            is_relative_expiration=True,
                            is_private=not self.gnszone.public
                        )
                        record_data.data.append(entry)

            payload = record_data
            if not record_data.data:
                self.logger.warning("Empty record %s", record_data)
                return "", None
            #self.logger.debug("Payload: %s", payload.to_json())

            # Replace the records already present in GNS as old ones are not deleted
            self.logger.debug(payload.record_name + "." + domain + ":\n")
            return payload.record_name + "." + domain, payload
            # FIXME error checking
            #response = self.session.post(f"/namestore/{domain}", data=payload.to_json())

            #if response.status_code == 204:
            #    self.logger.debug("Record(s) with label %s added", label)
            #else:
            #    data = response.json()
            #    error = data.get('error')
            #    self.logger.error("Unable to add record %s at URL %s: %s",
            #                      record_data.to_json(),
            #                      f"{self.session.base_url}/namestore/{domain}",
            #                      error)

            self.rrsetcount = self.rrsetcount + 1
        # End of worker

        # Building hierarchy afterwards
        tstart = time.time()
        self.create_zone_hierarchy()
        # Needs to happen after the previous line and before the adding of records
        self.transformer.subzonedict = self.subzonedict
        tend = time.time()
        self.logger.info("Zone hierarchy in %s seconds", str(tend - tstart))

        # Do it single threaded because threading scares me
        setcount = len(self.dnszone.zone.nodes.items())
        pp_set = {}
        rrcount = 0
        # TODO:
        # So, what we want to do here is to get all "dirty"
        # record sets. Dirty record sets are records that were
        # modified during the last pass (should here always be
        # > 0 since serial changed)
        # We may have just been given an AXFR even though IXFR was
        # requested.
        # So, after we add the records with the new serial, we
        # should delete all records with the old.
        # CAREFUL: This would mean that if the did receive an IXFR,
        # we have to update all serial numbers in the DB!
        for name, rdatasets in self.dnszone.zone.nodes.items():
            # log if the rdataset is empty for some reason
            if not rdatasets:
                print("Empty Rdataset!")
                continue
            name,payload = worker((name, rdatasets))
            if payload == None:
                continue
            pp_set[name] = payload
            if payload:
                rrcount += len(payload.data)

        pp_setcount = len(pp_set.items())
        left = pp_setcount
        start = 0
        slice0count = int(pp_setcount/self.num_workers)
        workers = []
        for i in range(self.num_workers):
            slice1count = slice0count
            if (i+1 == self.num_workers):
                slice1count = left
            ppslice = dict(itertools.islice(pp_set.items(), start, start+slice1count))
            p0 = mp.Process(target=work_slice, args=(i,self.batch_size,ppslice,self.gnunet_prefix))
            p0.start()
            workers.append(p0)
            start += slice1count
        for w in workers:
            w.join()
        tend = time.time()

        self.logger.info("Added %d RRSets for a total of %d RRs", pp_setcount, rrcount)
        self.logger.info("All records have been added in %s seconds",
                         str(tend - tstart))


    def add_pkey_record_to_zone(self, pkey: str, domain: str, label: str, ttl: int) -> None:
        """
        Adds the pkey of the subzone to the parent zone
        :param pkey: the public key of the child zone
        :param domain: the name of the parent zone
        :param label: the label under which to add the pkey
        :param ttl: the time to live the record should have in seconds
        """
        data = ascension.util.classes.GNSRecordData(
            value=pkey,
            record_type='EDKEY',
            relative_expiration=ttl,
            is_relative_expiration=True,
            is_private=not self.gnszone.public
        )

        record_data = ascension.util.classes.GNSRRecordSet(
            record_name=label,
            data=[data]
        )
        self.logger.debug("Added records to /namestore/%s with data %s", domain, record_data)
        payload = record_data
        self.ns_process.stdin.write(payload.record_name + "." + domain + ":\n")
        for r in payload.data:
            flags = "[r{}]".format('p' if not r.is_private else '')
            # FIXME we have many more flags. but probably not in our use
            # case? We always have relative expirations, for example.
            self.ns_process.stdin.write("{} {} {} {}\n".format(r.record_type,
                                                         r.relative_expiration,
                                                         flags,
                                                         r.value))
        #FIXME error checking
        #response = self.session.post(f"/namestore/{domain}", data=payload.to_json())

        #if response.status_code == 204:
        #    self.logger.debug("Added PKEY Record(s) with label %s", label)
        #    return

        #resp = response.json()
        #error = resp.get('error')
        #self.logger.error("Task failed with error %s %s",
        #                  error,
        #                  ascension.util.rest.NAMESTORE_REST_API_ERRORS.get(error))


    def create_zone_hierarchy(self) -> None:
        """
        Create equivalent DNS zone in GNS
        This transformation is necessary as DNS zones are not equivalent to GNS zones
        """
        # Extend Dictionary using GNS identities that already exist,
        # checking for conflicts with information in DNS
        self.logger.debug("Requesting all zones from the identity service")
        response = self.session.get("/identity")

        relevant_domains = list(filter(
            lambda x: x['name'].endswith(self.gnszone.domain),
            response.json())
        )
        for zone in relevant_domains:
            self.subzonedict[zone['name']] = (zone['pubkey'], self.gnszone.minimum)

        # Check if a delegated zone is available in GNS as per NS record
        # Adds NS records that contain "gns--pkey--" to dictionary
        nsrecords = self.dnszone.zone.iterate_rdatasets(dns.rdatatype.NS)
        nameserverlist = []
        for nsrecord in nsrecords:
            name = str(nsrecord[0])
            values = nsrecord[1]
            ttl = values.ttl

            # save DNS name object of nameservers for later
            for nameserver in values:
                nameserverlist.append(nameserver.target)

            # filter for gns--pkey record in rdatas
            gnspkeys = list(filter(lambda record:
                                   str(record).startswith('gns--pkey--'),
                                   values))
            num_gnspkeys = len(gnspkeys)
            if not num_gnspkeys:
                # skip empty values
                continue
            if num_gnspkeys > 1:
                self.logger.critical(
                    "Detected ambiguous EDKEY records for label %s (not generating EDKEY record)",
                    name
                )
                continue
            gnspkey = str(gnspkeys[0])

            zonepkey = gnspkey[11:]
            if len(zonepkey) != 59:
                continue

            zone = f"{name}.{self.gnszone.domain}"
            if not self.subzonedict.get(zone):
                self.subzonedict[zone] = (zonepkey, ttl)
            else:
                # This should be impossible!!?
                pkey_ttl = self.subzonedict[zone]
                pkey2, ttl = pkey_ttl
                if pkey2 != gnspkey:
                    self.logger.critical("EDKEY in DNS does not match EDKEY in GNS for name %s", name)
                    continue

        # Create missing zones (and add to dict) for GNS zones that are NOT DNS
        # zones ("." in a label is not a zone-cut in DNS, but always in GNS).
        # Only add the records for which there are no NS records for
        remaining_nsrecords = set(filter(lambda name: not name.is_absolute(),
                                         nameserverlist))
        remaining = set(filter(lambda name: name not in remaining_nsrecords,
                               self.dnszone.zone.nodes.keys()))
        final = set(filter(lambda name: len(str(name).split('.')) > 1,
                           remaining))

        for name in final:
            subzones = str(name).split('.')
            for i in range(1, len(subzones)):
                subdomain = ".".join(subzones[i:])
                zonename = f"{subdomain}.{self.gnszone.domain}"
                ttl = self.gnszone.minimum # new record, cannot use existing one
                if self.subzonedict.get(zonename) is None:
                    test = str(name)
                    if any(proto in test for proto in ('_tcp', '_udp')):
                        while not test.endswith(("_tcp", "_udp")):
                            chunk = test.split('.')
                            test = '.'.join(chunk[:-1])
                            zonename = f"{chunk[-1]}.{self.gnszone.domain}"
                            self.subzonedict[zonename] = (None, ttl)
                        self.subzonedict[zonename] = (None, ttl)
                        continue
                    pkey = self.gnszone.create_zone_and_get_pkey(zonename)
                    self.subzonedict[zonename] = (pkey, ttl)

        self.ns_process = subprocess.Popen(["gnunet-namestore", "-a", "-S"], stdin=subprocess.PIPE, text=True)
        # Generate EDKEY records for all entries in subzonedict
        for zone, pkeyttltuple in self.subzonedict.items():
            pkey, ttl = pkeyttltuple
            # Allow for any amount of subzones
            sub = zone.rstrip(self.gnszone.domain)
            domain = ".".join(zone.split('.')[1:])
            # This happens if root is reached - can happen multiple times
            if sub == '' or not pkeyttltuple[0]:
                continue
            label = zone.split('.')[0]
            self.logger.info("Adding zone %s with %s zkey into %s", zone, pkey, domain)
            self.add_pkey_record_to_zone(pkey, domain, label, int(ttl))
        self.ns_process.stdin.close()
        self.ns_process.wait()

def purge_subzones(self):
    for zname, zvalue in self.subzonedict:
        self.gnszone.delete_zone(zname)

def main():
    """
    Initializes the Ascension class, handles arguments and daemon
    """
    args = ascension.util.argumentparser.parse_arguments()

    # Initialize class instance
    ascender = Ascension(args)

    # Attempt a zone transfer with the given arguments and keys
    if args.dryrun:
        transferrable = ascender.dnszone.test_zone_transfer()
        if transferrable is None:
            ascender.logger.critical(
                'The specified domain is not transferrable using the given options!'
            )
            return 1
        ascender.logger.critical('SUCCESS! The specified domain is transferrable!')
        return 0

    # Checks if GNUnet REST API is running
    #if not ascender.session.is_running():
    #    ascender.logger.critical('GNUnet REST API is not reachable!')

    # Set defaults to use before we get a SOA for the first time
    retry = 300

    # variable to keep state
    needsupdate = False
    first_run = True

    # Main loop for actual daemon
    while True:
        gns_zone_serial = ascender.gnszone.get_gns_zone_serial()

        ascender.logger.info("GNS zone serial is %s", gns_zone_serial)
        dns_zone_serial = ascender.dnszone.get_dns_zone_serial()
        ascender.logger.info("DNS zone serial is %s", dns_zone_serial)

        if not dns_zone_serial:
            ascender.logger.error("Could not get DNS zone serial")
            if args.standalone:
                return 1
            time.sleep(retry)
            continue
        if not gns_zone_serial:
            print("GNS zone does not exist yet, performing full transfer.")
            ascender.gnszone.bootstrap_zone()
        elif gns_zone_serial == dns_zone_serial:
            print("GNS zone is up to date.")
            if args.standalone:
                return 0
            time.sleep(retry)
        elif gns_zone_serial > dns_zone_serial:
            ascender.logger.critical("SOA serial in GNS is bigger than SOA serial in DNS?")
            ascender.logger.critical("GNS zone: %s, DNS zone: %s", gns_zone_serial, dns_zone_serial)
            if args.standalone:
                return 1
            time.sleep(retry)
            continue
        else:
            print("GNS zone is out of date, performing incremental transfer.")
            needsupdate = True

        try:
            start = time.time()
            if not ascender.dnszone.zone or needsupdate:
                # Zonebackups are needed for retaining information for IXFR and
                # offer a zone for dnspython to patch
                # On a first run, we may have been given an initial zone file to
                # use as import source.
                zf = None
                if first_run:
                    zf = args.zonefile # May also be None
                gns_zone_serial = ascender.dnszone.restore_from_file(gns_zone_serial, zonefile=zf)
                ascender.logger.info("Zone serial for DNS zone transfer used: `%s'", gns_zone_serial)
                # Transfer the actual zone
                if None == zf:
                    ascender.dnszone.transfer_zone(gns_zone_serial)
                ascender.dnszone.backup_to_file()
            needsupdate = False
            soa = ascender.dnszone.get_zone_soa()
            end = time.time()
            ascender.logger.info("Transferring the zone took %s seconds", str(end - start))
            retry = int(str(soa[2]).split(" ")[4])
        except dns.zone.BadZone:
            ascender.logger.critical("Malformed DNS Zone '%s'", ascender.dnszone.domain)
            if args.standalone:
                return 2
            time.sleep(retry)
            continue

        # FIXME: IXFR would require a "merge"
        # FIXME: AXFR would require a GNS zone "purge"
        # For now, even if we IXFR update the zone, perform a full update
        ascender.gnszone.purge_records()
        ascender.add_records_to_gns()

        first_run = False
        if args.standalone:
            return 0

if __name__ == '__main__':
    mp.set_start_method('spawn')
    main()
