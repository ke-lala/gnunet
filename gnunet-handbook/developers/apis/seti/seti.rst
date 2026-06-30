.. index::
   double: subsystem; SETI

.. _SETI-Subsystem-Dev:

SETI
====

.. _Intersection-Set-API:

Intersection Set API
^^^^^^^^^^^^^^^^^^^^

New sets are created with ``GNUNET_SETI_create``. Only the local peer's
configuration (as each set has its own client connection) must be
provided. The set exists until either the client calls
``GNUNET_SET_destroy`` or the client's connection to the service is
disrupted. In the latter case, the client is notified by the return
value of functions dealing with sets. This return value must always be
checked.

Elements are added with ``GNUNET_SET_add_element``.

.. _Intersection-Listeners:

Intersection Listeners
^^^^^^^^^^^^^^^^^^^^^^

Listeners are created with ``GNUNET_SET_listen``. Each time time a
remote peer suggests a set operation with an application id and
operation type matching a listener, the listener's callback is invoked.
The client then must synchronously call either ``GNUNET_SET_accept`` or
``GNUNET_SET_reject``. Note that the operation will not be started until
the client calls ``GNUNET_SET_commit`` (see Section \"Supplying a
Set\").

.. _Intersection-Operations:

Intersection Operations
^^^^^^^^^^^^^^^^^^^^^^^

Operations to be initiated by the local peer are created with
``GNUNET_SET_prepare``. Note that the operation will not be started
until the client calls ``GNUNET_SET_commit`` (see Section \"Supplying a
Set\").

.. _Supplying-a-Set-for-Intersection:

Supplying a Set for Intersection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To create symmetry between the two ways of starting a set operation
(accepting and initiating it), the operation handles returned by
``GNUNET_SET_accept`` and ``GNUNET_SET_prepare`` do not yet have a set
to operate on, thus they can not do any work yet.

The client must call ``GNUNET_SET_commit`` to specify a set to use for
an operation. ``GNUNET_SET_commit`` may only be called once per set
operation.

.. _The-Intersection-Result-Callback:

The Intersection Result Callback
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Clients must specify both a result mode and a result callback with
``GNUNET_SET_accept`` and ``GNUNET_SET_prepare``. The result callback
with a status indicating either that an element was received, or the
operation failed or succeeded. The interpretation of the received
element depends on the result mode. The callback needs to know which
result mode it is used in, as the arguments do not indicate if an
element is part of the full result set, or if it is in the difference
between the original set and the final set.

.. _The-SETI-Client_002dService-Protocol:

The SETI Client-Service Protocol
--------------------------------

.. _Creating-Intersection-Sets:

Creating Intersection Sets
^^^^^^^^^^^^^^^^^^^^^^^^^^

For each set of a client, there exists a client connection to the
service. Sets are created by sending the ``GNUNET_SERVICE_SETI_CREATE``
message over a new client connection. Multiple operations for one set
are multiplexed over one client connection, using a request id supplied
by the client.

.. _Listeners-for-Intersection:

Listeners for Intersection
^^^^^^^^^^^^^^^^^^^^^^^^^^

Each listener also requires a separate client connection. By sending the
``GNUNET_SERVICE_SETI_LISTEN`` message, the client notifies the service
of the application id and operation type it is interested in. A client
rejects an incoming request by sending ``GNUNET_SERVICE_SETI_REJECT`` on
the listener's client connection. In contrast, when accepting an
incoming request, a ``GNUNET_SERVICE_SETI_ACCEPT`` message must be sent
over the set that is supplied for the set operation.

.. _Initiating-Intersection-Operations:

Initiating Intersection Operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Operations with remote peers are initiated by sending a
``GNUNET_SERVICE_SETI_EVALUATE`` message to the service. The client
connection that this message is sent by determines the set to use.

.. _Modifying-Intersection-Sets:

Modifying Intersection Sets
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sets are modified with the ``GNUNET_SERVICE_SETI_ADD`` message.

.. _Intersection-Results-and-Operation-Status:

Intersection Results and Operation Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The service notifies the client of result elements and success/failure
of a set operation with the ``GNUNET_SERVICE_SETI_RESULT`` message.

.. _The-SETI-Intersection-Peer_002dto_002dPeer-Protocol:

The SETI Intersection Peer-to-Peer Protocol
-------------------------------------------

The intersection protocol operates over CADET and starts with a
GNUNET_MESSAGE_TYPE_SETI_P2P_OPERATION_REQUEST being sent by the peer
initiating the operation to the peer listening for inbound requests. It
includes the number of elements of the initiating peer, which is used to
decide which side will send a Bloom filter first.

The listening peer checks if the operation type and application
identifier are acceptable for its current state. If not, it responds
with a GNUNET_MESSAGE_TYPE_SETI_RESULT and a status of
GNUNET_SETI_STATUS_FAILURE (and terminates the CADET channel).

If the application accepts the request, the listener sends back a
``GNUNET_MESSAGE_TYPE_SETI_P2P_ELEMENT_INFO`` if it has more elements in
the set than the client. Otherwise, it immediately starts with the Bloom
filter exchange. If the initiator receives a
``GNUNET_MESSAGE_TYPE_SETI_P2P_ELEMENT_INFO`` response, it beings the
Bloom filter exchange, unless the set size is indicated to be zero, in
which case the intersection is considered finished after just the
initial handshake.

.. _The-Bloom-filter-exchange-in-SETI:

The Bloom filter exchange in SETI
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this phase, each peer transmits a Bloom filter over the remaining
keys of the local set to the other peer using a
``GNUNET_MESSAGE_TYPE_SETI_P2P_BF`` message. This message additionally
includes the number of elements left in the sender's set, as well as the
XOR over all of the keys in that set.

The number of bits 'k' set per element in the Bloom filter is calculated
based on the relative size of the two sets. Furthermore, the size of the
Bloom filter is calculated based on 'k' and the number of elements in
the set to maximize the amount of data filtered per byte transmitted on
the wire (while avoiding an excessively high number of iterations).

The receiver of the message removes all elements from its local set that
do not pass the Bloom filter test. It then checks if the set size of the
sender and the XOR over the keys match what is left of its own set. If
they do, it sends a ``GNUNET_MESSAGE_TYPE_SETI_P2P_DONE`` back to
indicate that the latest set is the final result. Otherwise, the
receiver starts another Bloom filter exchange, except this time as the
sender.

.. _Intersection-Salt:

Intersection Salt
^^^^^^^^^^^^^^^^^

Bloom filter operations are probabilistic: With some non-zero
probability the test may incorrectly say an element is in the set, even
though it is not.

To mitigate this problem, the intersection protocol iterates exchanging
Bloom filters using a different random 32-bit salt in each iteration
(the salt is also included in the message). With different salts, set
operations may fail for different elements. Merging the results from the
executions, the probability of failure drops to zero.

The iterations terminate once both peers have established that they have
sets of the same size, and where the XOR over all keys computes the same
512-bit value (leaving a failure probability of 2-511).


