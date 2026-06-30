.. index::
   double: subsystem; SET

.. _SET-Subsystem-Dev:

SET
===

.. _Sets:

Sets
^^^^

New sets are created with ``GNUNET_SET_create``. Both the local peer's
configuration (as each set has its own client connection) and the
operation type must be specified. The set exists until either the client
calls ``GNUNET_SET_destroy`` or the client's connection to the service
is disrupted. In the latter case, the client is notified by the return
value of functions dealing with sets. This return value must always be
checked.

Elements are added and removed with ``GNUNET_SET_add_element`` and
``GNUNET_SET_remove_element``.

.. _Listeners:

Listeners
^^^^^^^^^

Listeners are created with ``GNUNET_SET_listen``. Each time time a
remote peer suggests a set operation with an application id and
operation type matching a listener, the listener's callback is invoked.
The client then must synchronously call either ``GNUNET_SET_accept`` or
``GNUNET_SET_reject``. Note that the operation will not be started until
the client calls ``GNUNET_SET_commit`` (see Section \"Supplying a
Set\").

.. _Operations:

Operations
^^^^^^^^^^

Operations to be initiated by the local peer are created with
``GNUNET_SET_prepare``. Note that the operation will not be started
until the client calls ``GNUNET_SET_commit`` (see Section \"Supplying a
Set\").

.. _Supplying-a-Set:

Supplying a Set
^^^^^^^^^^^^^^^

To create symmetry between the two ways of starting a set operation
(accepting and initiating it), the operation handles returned by
``GNUNET_SET_accept`` and ``GNUNET_SET_prepare`` do not yet have a set
to operate on, thus they can not do any work yet.

The client must call ``GNUNET_SET_commit`` to specify a set to use for
an operation. ``GNUNET_SET_commit`` may only be called once per set
operation.

.. _The-Result-Callback:

The Result Callback
^^^^^^^^^^^^^^^^^^^

Clients must specify both a result mode and a result callback with
``GNUNET_SET_accept`` and ``GNUNET_SET_prepare``. The result callback
with a status indicating either that an element was received, or the
operation failed or succeeded. The interpretation of the received
element depends on the result mode. The callback needs to know which
result mode it is used in, as the arguments do not indicate if an
element is part of the full result set, or if it is in the difference
between the original set and the final set.

.. _The-SET-Client_002dService-Protocol:

The SET Client-Service Protocol
-------------------------------

.. _Creating-Sets:

Creating Sets
^^^^^^^^^^^^^

For each set of a client, there exists a client connection to the
service. Sets are created by sending the ``GNUNET_SERVICE_SET_CREATE``
message over a new client connection. Multiple operations for one set
are multiplexed over one client connection, using a request id supplied
by the client.

.. _Listeners2:

Listeners
^^^^^^^^^

Each listener also requires a separate client connection. By sending the
``GNUNET_SERVICE_SET_LISTEN`` message, the client notifies the service
of the application id and operation type it is interested in. A client
rejects an incoming request by sending ``GNUNET_SERVICE_SET_REJECT`` on
the listener's client connection. In contrast, when accepting an
incoming request, a ``GNUNET_SERVICE_SET_ACCEPT`` message must be sent
over the set that is supplied for the set operation.

.. _Initiating-Operations:

Initiating Operations
^^^^^^^^^^^^^^^^^^^^^

Operations with remote peers are initiated by sending a
``GNUNET_SERVICE_SET_EVALUATE`` message to the service. The client
connection that this message is sent by determines the set to use.

.. _Modifying-Sets:

Modifying Sets
^^^^^^^^^^^^^^

Sets are modified with the ``GNUNET_SERVICE_SET_ADD`` and
``GNUNET_SERVICE_SET_REMOVE`` messages.

.. _Results-and-Operation-Status:

Results and Operation Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The service notifies the client of result elements and success/failure
of a set operation with the ``GNUNET_SERVICE_SET_RESULT`` message.

.. _Iterating-Sets:

Iterating Sets
^^^^^^^^^^^^^^

All elements of a set can be requested by sending
``GNUNET_SERVICE_SET_ITER_REQUEST``. The server responds with
``GNUNET_SERVICE_SET_ITER_ELEMENT`` and eventually terminates the
iteration with ``GNUNET_SERVICE_SET_ITER_DONE``. After each received
element, the client must send ``GNUNET_SERVICE_SET_ITER_ACK``. Note that
only one set iteration may be active for a set at any given time.

.. _The-SET-Intersection-Peer_002dto_002dPeer-Protocol:

The SET Intersection Peer-to-Peer Protocol
------------------------------------------

The intersection protocol operates over CADET and starts with a
GNUNET_MESSAGE_TYPE_SET_P2P_OPERATION_REQUEST being sent by the peer
initiating the operation to the peer listening for inbound requests. It
includes the number of elements of the initiating peer, which is used to
decide which side will send a Bloom filter first.

The listening peer checks if the operation type and application
identifier are acceptable for its current state. If not, it responds
with a GNUNET_MESSAGE_TYPE_SET_RESULT and a status of
GNUNET_SET_STATUS_FAILURE (and terminates the CADET channel).

If the application accepts the request, the listener sends back a
``GNUNET_MESSAGE_TYPE_SET_INTERSECTION_P2P_ELEMENT_INFO`` if it has more
elements in the set than the client. Otherwise, it immediately starts
with the Bloom filter exchange. If the initiator receives a
``GNUNET_MESSAGE_TYPE_SET_INTERSECTION_P2P_ELEMENT_INFO`` response, it
beings the Bloom filter exchange, unless the set size is indicated to be
zero, in which case the intersection is considered finished after just
the initial handshake.

.. _The-Bloom-filter-exchange:

The Bloom filter exchange
^^^^^^^^^^^^^^^^^^^^^^^^^

In this phase, each peer transmits a Bloom filter over the remaining
keys of the local set to the other peer using a
``GNUNET_MESSAGE_TYPE_SET_INTERSECTION_P2P_BF`` message. This message
additionally includes the number of elements left in the sender's set,
as well as the XOR over all of the keys in that set.

The number of bits 'k' set per element in the Bloom filter is calculated
based on the relative size of the two sets. Furthermore, the size of the
Bloom filter is calculated based on 'k' and the number of elements in
the set to maximize the amount of data filtered per byte transmitted on
the wire (while avoiding an excessively high number of iterations).

The receiver of the message removes all elements from its local set that
do not pass the Bloom filter test. It then checks if the set size of the
sender and the XOR over the keys match what is left of its own set. If
they do, it sends a ``GNUNET_MESSAGE_TYPE_SET_INTERSECTION_P2P_DONE``
back to indicate that the latest set is the final result. Otherwise, the
receiver starts another Bloom filter exchange, except this time as the
sender.

.. _Salt:

Salt
^^^^

Bloomfilter operations are probabilistic: With some non-zero probability
the test may incorrectly say an element is in the set, even though it is
not.

To mitigate this problem, the intersection protocol iterates exchanging
Bloom filters using a different random 32-bit salt in each iteration
(the salt is also included in the message). With different salts, set
operations may fail for different elements. Merging the results from the
executions, the probability of failure drops to zero.

The iterations terminate once both peers have established that they have
sets of the same size, and where the XOR over all keys computes the same
512-bit value (leaving a failure probability of 2\ :superscript:`-511`\ ).

.. _The-SET-Union-Peer_002dto_002dPeer-Protocol:

The SET Union Peer-to-Peer Protocol
-----------------------------------

The SET union protocol is based on Eppstein's efficient set
reconciliation without prior context. You should read this paper first
if you want to understand the protocol.

.. todo:: Link to Eppstein's paper!

The union protocol operates over CADET and starts with a
GNUNET_MESSAGE_TYPE_SET_P2P_OPERATION_REQUEST being sent by the peer
initiating the operation to the peer listening for inbound requests. It
includes the number of elements of the initiating peer, which is
currently not used.

The listening peer checks if the operation type and application
identifier are acceptable for its current state. If not, it responds
with a ``GNUNET_MESSAGE_TYPE_SET_RESULT`` and a status of
``GNUNET_SET_STATUS_FAILURE`` (and terminates the CADET channel).

If the application accepts the request, it sends back a strata estimator
using a message of type GNUNET_MESSAGE_TYPE_SET_UNION_P2P_SE. The
initiator evaluates the strata estimator and initiates the exchange of
invertible Bloom filters, sending a
GNUNET_MESSAGE_TYPE_SET_UNION_P2P_IBF.

During the IBF exchange, if the receiver cannot invert the Bloom filter
or detects a cycle, it sends a larger IBF in response (up to a defined
maximum limit; if that limit is reached, the operation fails). Elements
decoded while processing the IBF are transmitted to the other peer using
GNUNET_MESSAGE_TYPE_SET_P2P_ELEMENTS, or requested from the other peer
using GNUNET_MESSAGE_TYPE_SET_P2P_ELEMENT_REQUESTS messages, depending
on the sign observed during decoding of the IBF. Peers respond to a
GNUNET_MESSAGE_TYPE_SET_P2P_ELEMENT_REQUESTS message with the respective
element in a GNUNET_MESSAGE_TYPE_SET_P2P_ELEMENTS message. If the IBF
fully decodes, the peer responds with a
GNUNET_MESSAGE_TYPE_SET_UNION_P2P_DONE message instead of another
GNUNET_MESSAGE_TYPE_SET_UNION_P2P_IBF.

All Bloom filter operations use a salt to mingle keys before hashing
them into buckets, such that future iterations have a fresh chance of
succeeding if they failed due to collisions before.

