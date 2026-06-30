.. index::
   double: GNU Name System; subsystem
   see: GNS; GNU Name System

.. _GNU-Name-System-Dev:

GNS
===

The GNS API itself is extremely simple. Clients first connect to the GNS
service using ``GNUNET_GNS_connect``. They can then perform lookups
using ``GNUNET_GNS_lookup`` or cancel pending lookups using
``GNUNET_GNS_lookup_cancel``. Once finished, clients disconnect using
``GNUNET_GNS_disconnect``.

.. _Looking-up-records:

Looking up records
^^^^^^^^^^^^^^^^^^

``GNUNET_GNS_lookup`` takes a number of arguments:

handle This is simply the GNS connection handle from
   ``GNUNET_GNS_connect``.

name The client needs to specify the name to
   be resolved. This can be any valid DNS or GNS hostname.

zone The client
   needs to specify the public key of the GNS zone against which the
   resolution should be done. Note that a key must be provided, the
   client should look up plausible values using its configuration, the
   identity service and by attempting to interpret the TLD as a
   base32-encoded public key.

type This is the desired GNS or DNS record type
   to look for. While all records for the given name will be returned,
   this can be important if the client wants to resolve record types
   that themselves delegate resolution, such as CNAME, PKEY or GNS2DNS.
   Resolving a record of any of these types will only work if the
   respective record type is specified in the request, as the GNS
   resolver will otherwise follow the delegation and return the records
   from the respective destination, instead of the delegating record.

only_cached This argument should typically be set to
   ``GNUNET_NO``. Setting it to ``GNUNET_YES`` disables resolution via
   the overlay network.

shorten_zone_key If GNS encounters new names during resolution,
   their respective zones can automatically be learned and added to the
   \"shorten zone\". If this is desired, clients must pass the private
   key of the shorten zone. If NULL is passed, shortening is disabled.

proc This argument identifies
   the function to call with the result. It is given proc_cls, the
   number of records found (possibly zero) and the array of the records
   as arguments. proc will only be called once. After proc,> has been
   called, the lookup must no longer be canceled.

proc_cls The closure for proc.

.. _Accessing-the-records:

Accessing the records
^^^^^^^^^^^^^^^^^^^^^

The ``libgnunetgnsrecord`` library provides an API to manipulate the GNS
record array that is given to proc. In particular, it offers functions
such as converting record values to human-readable strings (and back).
However, most ``libgnunetgnsrecord`` functions are not interesting to
GNS client applications.

For DNS records, the ``libgnunetdnsparser`` library provides functions
for parsing (and serializing) common types of DNS records.

.. _Creating-records:

Creating records
^^^^^^^^^^^^^^^^

Creating GNS records is typically done by building the respective record
information (possibly with the help of ``libgnunetgnsrecord`` and
``libgnunetdnsparser``) and then using the ``libgnunetnamestore`` to
publish the information. The GNS API is not involved in this operation.

.. _Future-work:

Future work
^^^^^^^^^^^

In the future, we want to expand ``libgnunetgns`` to allow applications
to observe shortening operations performed during GNS resolution, for
example so that users can receive visual feedback when this happens.

libgnunetgnsrecord
~~~~~~~~~~~~~~~~~~

The ``libgnunetgnsrecord`` library is used to manipulate GNS records (in
plaintext or in their encrypted format). Applications mostly interact
with ``libgnunetgnsrecord`` by using the functions to convert GNS record
values to strings or vice-versa, or to lookup a GNS record type number
by name (or vice-versa). The library also provides various other
functions that are mostly used internally within GNS, such as converting
keys to names, checking for expiration, encrypting GNS records to GNS
blocks, verifying GNS block signatures and decrypting GNS records from
GNS blocks.

We will now discuss the four commonly used functions of the
API. ``libgnunetgnsrecord`` does not perform these operations itself,
but instead uses plugins to perform the operation. GNUnet includes
plugins to support common DNS record types as well as standard GNS
record types.

.. _Value-handling:

Value handling
^^^^^^^^^^^^^^

``GNUNET_GNSRECORD_value_to_string`` can be used to convert the (binary)
representation of a GNS record value to a human readable, 0-terminated
UTF-8 string. NULL is returned if the specified record type is not
supported by any available plugin.

``GNUNET_GNSRECORD_string_to_value`` can be used to try to convert a
human readable string to the respective (binary) representation of a GNS
record value.

.. _Type-handling:

Type handling
^^^^^^^^^^^^^

``GNUNET_GNSRECORD_typename_to_number`` can be used to obtain the
numeric value associated with a given typename. For example, given the
typename \"A\" (for DNS A reocrds), the function will return the number
1. A list of common DNS record types is
`here <http://en.wikipedia.org/wiki/List_of_DNS_record_types>`__. Note
that not all DNS record types are supported by GNUnet GNSRECORD plugins
at this time.

``GNUNET_GNSRECORD_number_to_typename`` can be used to obtain the
typename associated with a given numeric value. For example, given the
type number 1, the function will return the typename \"A\".

.. _GNS-plugins:

GNS plugins
~~~~~~~~~~~

Adding a new GNS record type typically involves writing (or extending) a
GNSRECORD plugin. The plugin needs to implement the
``gnunet_gnsrecord_plugin.h`` API which provides basic functions that
are needed by GNSRECORD to convert typenames and values of the
respective record type to strings (and back). These gnsrecord plugins
are typically implemented within their respective subsystems. Examples
for such plugins can be found in the GNSRECORD, GNS and CONVERSATION
subsystems.

The ``libgnunetgnsrecord`` library is then used to locate, load and
query the appropriate gnsrecord plugin. Which plugin is appropriate is
determined by the record type (which is just a 32-bit integer). The
``libgnunetgnsrecord`` library loads all block plugins that are
installed at the local peer and forwards the application request to the
plugins. If the record type is not supported by the plugin, it should
simply return an error code.

The central functions of the block APIs (plugin and main library) are
the same four functions for converting between values and strings, and
typenames and numbers documented in the previous subsection.

.. _The-GNS-Client_002dService-Protocol:

The GNS Client-Service Protocol
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The GNS client-service protocol consists of two simple messages, the
``LOOKUP`` message and the ``LOOKUP_RESULT``. Each ``LOOKUP`` message
contains a unique 32-bit identifier, which will be included in the
corresponding response. Thus, clients can send many lookup requests in
parallel and receive responses out-of-order. A ``LOOKUP`` request also
includes the public key of the GNS zone, the desired record type and
fields specifying whether shortening is enabled or networking is
disabled. Finally, the ``LOOKUP`` message includes the name to be
resolved.

The response includes the number of records and the records themselves
in the format created by ``GNUNET_GNSRECORD_records_serialize``. They
can thus be deserialized using ``GNUNET_GNSRECORD_records_deserialize``.

.. _Hijacking-the-DNS_002dTraffic-using-gnunet_002dservice_002ddns:

Hijacking the DNS-Traffic using gnunet-service-dns
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This section documents how the gnunet-service-dns (and the
gnunet-helper-dns) intercepts DNS queries from the local system. This is
merely one method for how we can obtain GNS queries. It is also possible
to change ``resolv.conf`` to point to a machine running
``gnunet-dns2gns`` or to modify libc's name system switch (NSS)
configuration to include a GNS resolution plugin. The method described
in this chapter is more of a last-ditch catch-all approach.

``gnunet-service-dns`` enables intercepting DNS traffic using policy
based routing. We MARK every outgoing DNS-packet if it was not sent by
our application. Using a second routing table in the Linux kernel these
marked packets are then routed through our virtual network interface and
can thus be captured unchanged.

Our application then reads the query and decides how to handle it. If
the query can be addressed via GNS, it is passed to
``gnunet-service-gns`` and resolved internally using GNS. In the future,
a reverse query for an address of the configured virtual network could
be answered with records kept about previous forward queries. Queries
that are not hijacked by some application using the DNS service will be
sent to the original recipient. The answer to the query will always be
sent back through the virtual interface with the original nameserver as
source address.

.. _Network-Setup-Details:

Network Setup Details
^^^^^^^^^^^^^^^^^^^^^

The DNS interceptor adds the following rules to the Linux kernel:

::

   iptables -t mangle -I OUTPUT 1 -p udp --sport $LOCALPORT --dport 53 \
   -j ACCEPT iptables -t mangle -I OUTPUT 2 -p udp --dport 53 -j MARK \
   --set-mark 3 ip rule add fwmark 3 table2 ip route add default via \
   $VIRTUALDNS table2

.. todo:: 
   FIXME: Rewrite to reflect display which is no longer content 
   by line due to the < 74 characters limit.

Line 1 makes sure that all packets coming from a port our application
opened beforehand (``$LOCALPORT``) will be routed normally. Line 2 marks
every other packet to a DNS-Server with mark 3 (chosen arbitrarily). The
third line adds a routing policy based on this mark 3 via the routing
table.

.. _Importing-DNS-Zones-into-GNS:

Importing DNS Zones into GNS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This section discusses the challenges and problems faced when writing
the Ascension tool. It also takes a look at possible improvements in the
future.

Consider the following diagram that shows the workflow of Ascension:

|ascension|

Further the interaction between components of GNUnet are shown in the
diagram below:

DNS Conversion
.. _Conversions-between-DNS-and-GNS:

Conversions between DNS and GNS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The differences between the two name systems lies in the details and is
not always transparent. For instance an SRV record is converted to a BOX
record which is unique to GNS.

This is done by converting to a BOX record from an existing SRV record:

::

   # SRV
   # _service._proto.name. TTL class SRV priority weight port target
   _sip._tcp.example.com. 14000 IN SRV     0 0 5060 www.example.com.
   # BOX
   # TTL BOX flags port protocol recordtype priority weight port target
   14000 BOX n 5060 6 33 0 0 5060 www.example.com

Other records that need to undergo such transformation is the MX record
type, as well as the SOA record type.

Transformation of a SOA record into GNS works as described in the
following example. Very important to note are the rname and mname keys.

::

   # BIND syntax for a clean SOA record
      IN SOA master.example.com. hostmaster.example.com. (
       2017030300 ; serial
       3600       ; refresh
       1800       ; retry
       604800     ; expire
       600 )      ; ttl
   # Recordline for adding the record
   $ gnunet-namestore -z example.com -a -n  -t SOA -V \
       rname=master.example.com mname=hostmaster.example.com  \
       2017030300,3600,1800,604800,600 -e 7200s

The transformation of MX records is done in a simple way.

::

   # mail.example.com. 3600 IN MX 10 mail.example.com.
   $ gnunet-namestore -z example.com -n mail -R 3600 MX n 10,mail

Finally, one of the biggest struggling points were the NS records that
are found in top level domain zones. The intended behaviour for those is
to add GNS2DNS records for those so that gnunet-gns can resolve records
for those domains on its own. Those require the values from DNS GLUE
records, provided they are within the same zone.

The following two examples show one record with a GLUE record and the
other one does not have a GLUE record. This takes place in the 'com'
TLD.

.. code-block:: shell

   # ns1.example.com 86400 IN A 127.0.0.1
   # example.com 86400 IN NS ns1.example.com.
   $ gnunet-namestore -z com -n example -R 86400 GNS2DNS n \
       example.com@127.0.0.1

   # example.com 86400 IN NS ns1.example.org.
   $ gnunet-namestore -z com -n example -R 86400 GNS2DNS n \
       example.com@ns1.example.org

As you can see, one of the GNS2DNS records has an IP address listed and
the other one a DNS name. For the first one there is a GLUE record to do
the translation directly and the second one will issue another DNS query
to figure out the IP of ns1.example.org.

A solution was found by creating a hierarchical zone structure in GNS
and linking the zones using PKEY records to one another. This allows the
resolution of the name servers to work within GNS while not taking
control over unwanted zones.

Currently the following record types are supported:

-  A

-  AAAA

-  CNAME

-  MX

-  NS

-  SRV

-  TXT

This is not due to technical limitations but rather a practical ones.
The problem occurs with DNSSEC enabled DNS zones. As records within
those zones are signed periodically, and every new signature is an
update to the zone, there are many revisions of zones. This results in a
problem with bigger zones as there are lots of records that have been
signed again but no major changes. Also trying to add records that are
unknown that require a different format take time as they cause a CLI
call of the namestore. Furthermore certain record types need
transformation into a GNS compatible format which, depending on the
record type, takes more time.

Further a blacklist was added to drop for instance DNSSEC related
records. Also if a record type is neither in the white list nor the
blacklist it is considered as a loss of data and a message is shown to
the user. This helps with transparency and also with contributing, as
the not supported record types can then be added accordingly.

.. _DNS-Zone-Size:

DNS Zone Size
^^^^^^^^^^^^^

Another very big problem exists with very large zones. When migrating a
small zone the delay between adding of records and their expiry is
negligible. However when working with big zones that easily have more
than a few million records this delay becomes a problem.

Records will start to expire well before the zone has finished
migrating. This is usually not a problem but can cause a high CPU load
when a peer is restarted and the records have expired.

A good solution has not been found yet. One of the idea that floated
around was that the records should be added with the s (shadow) flag to
keep the records resolvable even if they expired. However this would
introduce the problem of how to detect if a record has been removed from
the zone and would require deletion of said record(s).

Another problem that still persists is how to refresh records. Expired
records are still displayed when calling gnunet-namestore but do not
resolve with gnunet-gns. Zonemaster will sign the expired records again
and make sure that the records are still valid. With a recent change
this was fixed as gnunet-gns to improve the suffix lookup which allows
for a fast lookup even with thousands of local egos.

Currently the pace of adding records in general is around 10 records per
second. Crypto is the upper limit for adding of records. The performance
of your machine can be tested with the perf_crypto\_\* tools. There is
still a big discrepancy between the pace of Ascension and the
theoretical limit.

A performance metric for measuring improvements has not yet been
implemented in Ascension.

.. _Performance:

Performance
^^^^^^^^^^^

The performance when migrating a zone using the Ascension tool is
limited by a handful of factors. First of all ascension is written in
Python3 and calls the CLI tools of GNUnet. This is comparable to a fork
and exec call which costs a few CPU cycles. Furthermore all the records
that are added to the same label are signed using the zones private key.
This signing operation is very resource heavy and was optimized during
development by adding the '-R' (Recordline) option to gnunet-namestore
which allows to specify multiple records using the CLI tool. Assuming
that in a TLD zone every domain has at least two name servers this
halves the amount of signatures needed.

Another improvement that could be made is with the addition of multiple
threads or using asynchronous subprocesses when opening the GNUnet CLI
tools. This could be implemented by simply creating more workers in the
program but performance improvements were not tested.

Ascension was tested using different hardware and database backends.
Performance differences between SQLite and postgresql are marginal and
almost non existent. What did make a huge impact on record adding
performance was the storage medium. On a traditional mechanical hard
drive adding of records were slow compared to a solid state disk.

In conclusion there are many bottlenecks still around in the program,
namely the single threaded implementation and inefficient, sequential
calls of gnunet-namestore. In the future a solution that uses the C API
would be cleaner and better.

.. |ascension| image:: /images/ascension_ssd.png
