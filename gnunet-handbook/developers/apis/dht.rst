.. _Distributed-Hash-Table-Dev:

.. index::
   double: Distributed hash table; subsystem
   see: DHT; Distributed hash table

DHT
===

The API of libgnunetblock
^^^^^^^^^^^^^^^^^^^^^^^^^

The block library requires for each (family of) block type(s) a block
plugin (implementing ``gnunet_block_plugin.h``) that provides basic
functions that are needed by the DHT (and possibly other subsystems) to
manage the block. These block plugins are typically implemented within
their respective subsystems. The main block library is then used to
locate, load and query the appropriate block plugin. Which plugin is
appropriate is determined by the block type (which is just a 32-bit
integer). Block plugins contain code that specifies which block types
are supported by a given plugin. The block library loads all block
plugins that are installed at the local peer and forwards the
application request to the respective plugin.

The central functions of the block APIs (plugin and main library) are to
allow the mapping of blocks to their respective key (if possible) and
the ability to check that a block is well-formed and matches a given
request (again, if possible). This way, GNUnet can avoid storing invalid
blocks, storing blocks under the wrong key and forwarding blocks in
response to a query that they do not answer.

One key function of block plugins is that it allows GNUnet to detect
duplicate replies (via the Bloom filter). All plugins MUST support
detecting duplicate replies (by adding the current response to the Bloom
filter and rejecting it if it is encountered again). If a plugin fails
to do this, responses may loop in the network.

.. _Queries:

Queries
^^^^^^^

The query format for any block in GNUnet consists of four main
components. First, the type of the desired block must be specified.
Second, the query must contain a hash code. The hash code is used for
lookups in hash tables and databases and must not be unique for the
block (however, if possible a unique hash should be used as this would
be best for performance). Third, an optional Bloom filter can be
specified to exclude known results; replies that hash to the bits set in
the Bloom filter are considered invalid. False-positives can be
eliminated by sending the same query again with a different Bloom filter
mutator value, which parametrizes the hash function that is used.
Finally, an optional application-specific \"eXtended query\" (xquery)
can be specified to further constrain the results. It is entirely up to
the type-specific plugin to determine whether or not a given block
matches a query (type, hash, Bloom filter, and xquery). Naturally, not
all xquery's are valid and some types of blocks may not support Bloom
filters either, so the plugin also needs to check if the query is valid
in the first place.

Depending on the results from the plugin, the DHT will then discard the
(invalid) query, forward the query, discard the (invalid) reply, cache
the (valid) reply, and/or forward the (valid and non-duplicate) reply.

.. _Sample-Code:

Sample Code
^^^^^^^^^^^

The source code in **plugin_block_test.c** is a good starting point for
new block plugins --- it does the minimal work by implementing a plugin
that performs no validation at all. The respective **Makefile.am** shows
how to build and install a block plugin.

.. _Conclusion2:

Conclusion2
^^^^^^^^^^^

In conclusion, GNUnet subsystems that want to use the DHT need to define
a block format and write a plugin to match queries and replies. For
testing, the ``GNUNET_BLOCK_TYPE_TEST`` block type can be used; it
accepts any query as valid and any reply as matching any query. This
type is also used for the DHT command line tools. However, it should NOT
be used for normal applications due to the lack of error checking that
results from this primitive implementation.

libgnunetdht
:index:`libgnunetdht <single: libgnunet; dht>`
libgnunetdht
------------

The DHT API itself is pretty simple and offers the usual GET and PUT
functions that work as expected. The specified block type refers to the
block library which allows the DHT to run application-specific logic for
data stored in the network.

.. _GET:

GET
^^^

When using GET, the main consideration for developers (other than the
block library) should be that after issuing a GET, the DHT will
continuously cause (small amounts of) network traffic until the
operation is explicitly canceled. So GET does not simply send out a
single network request once; instead, the DHT will continue to search
for data. This is needed to achieve good success rates and also handles
the case where the respective PUT operation happens after the GET
operation was started. Developers should not cancel an existing GET
operation and then explicitly re-start it to trigger a new round of
network requests; this is simply inefficient, especially as the internal
automated version can be more efficient, for example by filtering
results in the network that have already been returned.

If an application that performs a GET request has a set of replies that
it already knows and would like to filter, it can
call ``GNUNET_DHT_get_filter_known_results`` with an array of hashes
over the respective blocks to tell the DHT that these results are not
desired (any more). This way, the DHT will filter the respective blocks
using the block library in the network, which may result in a
significant reduction in bandwidth consumption.

.. _PUT:

PUT
^^^

.. todo:: inconsistent use of "must" above it's written "MUST"

In contrast to GET operations, developers **must** manually re-run PUT
operations periodically (if they intend the content to continue to be
available). Content stored in the DHT expires or might be lost due to
churn. Furthermore, GNUnet's DHT typically requires multiple rounds of
PUT operations before a key-value pair is consistently available to all
peers (the DHT randomizes paths and thus storage locations, and only
after multiple rounds of PUTs there will be a sufficient number of
replicas in large DHTs). An explicit PUT operation using the DHT API
will only cause network traffic once, so in order to ensure basic
availability and resistance to churn (and adversaries), PUTs must be
repeated. While the exact frequency depends on the application, a rule
of thumb is that there should be at least a dozen PUT operations within
the content lifetime. Content in the DHT typically expires after one
day, so DHT PUT operations should be repeated at least every 1-2 hours.

.. _MONITOR:

MONITOR
^^^^^^^

The DHT API also allows applications to monitor messages crossing the
local DHT service. The types of messages used by the DHT are GET, PUT
and RESULT messages. Using the monitoring API, applications can choose
to monitor these requests, possibly limiting themselves to requests for
a particular block type.

The monitoring API is not only useful for diagnostics, it can also be
used to trigger application operations based on PUT operations. For
example, an application may use PUTs to distribute work requests to
other peers. The workers would then monitor for PUTs that give them
work, instead of looking for work using GET operations. This can be
beneficial, especially if the workers have no good way to guess the keys
under which work would be stored. Naturally, additional protocols might
be needed to ensure that the desired number of workers will process the
distributed workload.

.. _DHT-Routing-Options:

DHT Routing Options
^^^^^^^^^^^^^^^^^^^

There are two important options for GET and PUT requests:

GNUNET_DHT_RO_DEMULITPLEX_EVERYWHERE This option means that all
   peers should process the request, even if their peer ID is not
   closest to the key. For a PUT request, this means that all peers that
   a request traverses may make a copy of the data. Similarly for a GET
   request, all peers will check their local database for a result.
   Setting this option can thus significantly improve caching and reduce
   bandwidth consumption --- at the expense of a larger DHT database. If
   in doubt, we recommend that this option should be used.

GNUNET_DHT_RO_RECORD_ROUTE This option instructs the DHT to record
   the path that a GET or a PUT request is taking through the overlay
   network. The resulting paths are then returned to the application
   with the respective result. This allows the receiver of a result to
   construct a path to the originator of the data, which might then be
   used for routing. Naturally, setting this option requires additional
   bandwidth and disk space, so applications should only set this if the
   paths are needed by the application logic.

GNUNET_DHT_RO_FIND_PEER This option is an internal option used by
   the DHT's peer discovery mechanism and should not be used by
   applications.

GNUNET_DHT_RO_BART This option is currently not implemented. It may
   in the future offer performance improvements for clique topologies.

.. _The-DHT-Client_002dService-Protocol:

The DHT Client-Service Protocol
-------------------------------

.. _PUTting-data-into-the-DHT:

PUTting data into the DHT
^^^^^^^^^^^^^^^^^^^^^^^^^

To store (PUT) data into the DHT, the client sends a
``struct GNUNET_DHT_ClientPutMessage`` to the service. This message
specifies the block type, routing options, the desired replication
level, the expiration time, key, value and a 64-bit unique ID for the
operation. The service responds with a
``struct GNUNET_DHT_ClientPutConfirmationMessage`` with the same 64-bit
unique ID. Note that the service sends the confirmation as soon as it
has locally processed the PUT request. The PUT may still be propagating
through the network at this time.

In the future, we may want to change this to provide (limited) feedback
to the client, for example if we detect that the PUT operation had no
effect because the same key-value pair was already stored in the DHT.
However, changing this would also require additional state and messages
in the P2P interaction.

.. _GETting-data-from-the-DHT:

GETting data from the DHT
^^^^^^^^^^^^^^^^^^^^^^^^^

To retrieve (GET) data from the DHT, the client sends a
``struct GNUNET_DHT_ClientGetMessage`` to the service. The message
specifies routing options, a replication level (for replicating the GET,
not the content), the desired block type, the key, the (optional)
extended query and unique 64-bit request ID.

Additionally, the client may send any number of
``struct GNUNET_DHT_ClientGetResultSeenMessage``\ s to notify the
service about results that the client is already aware of. These
messages consist of the key, the unique 64-bit ID of the request, and an
arbitrary number of hash codes over the blocks that the client is
already aware of. As messages are restricted to 64k, a client that
already knows more than about a thousand blocks may need to send several
of these messages. Naturally, the client should transmit these messages
as quickly as possible after the original GET request such that the DHT
can filter those results in the network early on. Naturally, as these
messages are sent after the original request, it is conceivable that the
DHT service may return blocks that match those already known to the
client anyway.

In response to a GET request, the service will send ``struct
GNUNET_DHT_ClientResultMessage``\ s to the client. These messages
contain the block type, expiration, key, unique ID of the request and of
course the value (a block). Depending on the options set for the
respective operations, the replies may also contain the path the GET
and/or the PUT took through the network.

A client can stop receiving replies either by disconnecting or by
sending a ``struct GNUNET_DHT_ClientGetStopMessage`` which must contain
the key and the 64-bit unique ID of the original request. Using an
explicit \"stop\" message is more common as this allows a client to run
many concurrent GET operations over the same connection with the DHT
service --- and to stop them individually.

.. _Monitoring-DHT:

Monitoring the DHT
^^^^^^^^^^^^^^^^^^

To begin monitoring, the client sends a
``struct GNUNET_DHT_MonitorStartStop`` message to the DHT service. In
this message, flags can be set to enable (or disable) monitoring of GET,
PUT and RESULT messages that pass through a peer. The message can also
restrict monitoring to a particular block type or a particular key. Once
monitoring is enabled, the DHT service will notify the client about any
matching event using ``struct GNUNET_DHT_MonitorGetMessage``\ s for GET
events, ``struct GNUNET_DHT_MonitorPutMessage`` for PUT events and
``struct GNUNET_DHT_MonitorGetRespMessage`` for RESULTs. Each of these
messages contains all of the information about the event.

.. _The-DHT-Peer_002dto_002dPeer-Protocol:

The DHT Peer-to-Peer Protocol
-----------------------------

.. _Routing-GETs-or-PUTs:

Routing GETs or PUTs
^^^^^^^^^^^^^^^^^^^^

When routing GETs or PUTs, the DHT service selects a suitable subset of
neighbours for forwarding. The exact number of neighbours can be zero or
more and depends on the hop counter of the query (initially zero) in
relation to the (log of) the network size estimate, the desired
replication level and the peer's connectivity. Depending on the hop
counter and our network size estimate, the selection of the peers maybe
randomized or by proximity to the key. Furthermore, requests include a
set of peers that a request has already traversed; those peers are also
excluded from the selection.

.. _PUTting-data-into-the-DHT2:

PUTting data into the DHT
^^^^^^^^^^^^^^^^^^^^^^^^^

To PUT data into the DHT, the service sends a ``struct PeerPutMessage``
of type ``GNUNET_MESSAGE_TYPE_DHT_P2P_PUT`` to the respective neighbour.
In addition to the usual information about the content (type, routing
options, desired replication level for the content, expiration time, key
and value), the message contains a fixed-size Bloom filter with
information about which peers (may) have already seen this request. This
Bloom filter is used to ensure that DHT messages never loop back to a
peer that has already processed the request. Additionally, the message
includes the current hop counter and, depending on the routing options,
the message may include the full path that the message has taken so far.
The Bloom filter should already contain the identity of the previous
hop; however, the path should not include the identity of the previous
hop and the receiver should append the identity of the sender to the
path, not its own identity (this is done to reduce bandwidth).

.. _GETting-data-from-the-DHT2:

GETting data from the DHT
^^^^^^^^^^^^^^^^^^^^^^^^^

A peer can search the DHT by sending ``struct PeerGetMessage``\ s of
type ``GNUNET_MESSAGE_TYPE_DHT_P2P_GET`` to other peers. In addition to
the usual information about the request (type, routing options, desired
replication level for the request, the key and the extended query), a
GET request also contains a hop counter, a Bloom filter over the peers
that have processed the request already and depending on the routing
options the full path traversed by the GET. Finally, a GET request
includes a variable-size second Bloom filter and a so-called Bloom
filter mutator value which together indicate which replies the sender
has already seen. During the lookup, each block that matches they block
type, key and extended query is additionally subjected to a test against
this Bloom filter. The block plugin is expected to take the hash of the
block and combine it with the mutator value and check if the result is
not yet in the Bloom filter. The originator of the query will from time
to time modify the mutator to (eventually) allow false-positives
filtered by the Bloom filter to be returned.

Peers that receive a GET request perform a local lookup (depending on
their proximity to the key and the query options) and forward the
request to other peers. They then remember the request (including the
Bloom filter for blocking duplicate results) and when they obtain a
matching, non-filtered response a ``struct PeerResultMessage`` of type
``GNUNET_MESSAGE_TYPE_DHT_P2P_RESULT`` is forwarded to the previous hop.
Whenever a result is forwarded, the block plugin is used to update the
Bloom filter accordingly, to ensure that the same result is never
forwarded more than once. The DHT service may also cache forwarded
results locally if the \"CACHE_RESULTS\" option is set to \"YES\" in the
configuration.
