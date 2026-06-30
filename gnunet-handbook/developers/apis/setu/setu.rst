
.. index:: 
   double: SETU; subsystem

.. _SETU-Subsystem-Dev:

SETU
====

.. _Union-Set-API:

Union Set API
^^^^^^^^^^^^^

New sets are created with ``GNUNET_SETU_create``. Only the local peer's
configuration (as each set has its own client connection) must be
provided. The set exists until either the client calls
``GNUNET_SETU_destroy`` or the client's connection to the service is
disrupted. In the latter case, the client is notified by the return
value of functions dealing with sets. This return value must always be
checked.

Elements are added with ``GNUNET_SETU_add_element``.

.. _Union-Listeners:

Union Listeners
^^^^^^^^^^^^^^^

Listeners are created with ``GNUNET_SETU_listen``. Each time time a
remote peer suggests a set operation with an application id and
operation type matching a listener, the listener's callback is invoked.
The client then must synchronously call either ``GNUNET_SETU_accept`` or
``GNUNET_SETU_reject``. Note that the operation will not be started
until the client calls ``GNUNET_SETU_commit`` (see Section \"Supplying a
Set\").

.. _Union-Operations:

Union Operations
^^^^^^^^^^^^^^^^

Operations to be initiated by the local peer are created with
``GNUNET_SETU_prepare``. Note that the operation will not be started
until the client calls ``GNUNET_SETU_commit`` (see Section \"Supplying a
Set\").

.. _Supplying-a-Set-for-Union:

Supplying a Set for Union
^^^^^^^^^^^^^^^^^^^^^^^^^

To create symmetry between the two ways of starting a set operation
(accepting and initiating it), the operation handles returned by
``GNUNET_SETU_accept`` and ``GNUNET_SETU_prepare`` do not yet have a set
to operate on, thus they can not do any work yet.

The client must call ``GNUNET_SETU_commit`` to specify a set to use for
an operation. ``GNUNET_SETU_commit`` may only be called once per set
operation.

.. _The-Union-Result-Callback:

The Union Result Callback
^^^^^^^^^^^^^^^^^^^^^^^^^

Clients must specify both a result mode and a result callback with
``GNUNET_SETU_accept`` and ``GNUNET_SETU_prepare``. The result callback
with a status indicating either that an element was received,
transmitted to the other peer (if this information was requested), or if
the operation failed or ultimately succeeded.

.. _The-SETU-Client_002dService-Protocol:

The SETU Client-Service Protocol
--------------------------------

.. _Creating-Union-Sets:

Creating Union Sets
^^^^^^^^^^^^^^^^^^^

For each set of a client, there exists a client connection to the
service. Sets are created by sending the ``GNUNET_SERVICE_SETU_CREATE``
message over a new client connection. Multiple operations for one set
are multiplexed over one client connection, using a request id supplied
by the client.

.. _Listeners-for-Union:

Listeners for Union
^^^^^^^^^^^^^^^^^^^

Each listener also requires a separate client connection. By sending the
``GNUNET_SERVICE_SETU_LISTEN`` message, the client notifies the service
of the application id and operation type it is interested in. A client
rejects an incoming request by sending ``GNUNET_SERVICE_SETU_REJECT`` on
the listener's client connection. In contrast, when accepting an
incoming request, a ``GNUNET_SERVICE_SETU_ACCEPT`` message must be sent
over the set that is supplied for the set operation.

.. _Initiating-Union-Operations:

Initiating Union Operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Operations with remote peers are initiated by sending a
``GNUNET_SERVICE_SETU_EVALUATE`` message to the service. The client
connection that this message is sent by determines the set to use.

.. _Modifying-Union-Sets:

Modifying Union Sets
^^^^^^^^^^^^^^^^^^^^

Sets are modified with the ``GNUNET_SERVICE_SETU_ADD`` message.

.. _Union-Results-and-Operation-Status:

Union Results and Operation Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The service notifies the client of result elements and success/failure
of a set operation with the ``GNUNET_SERVICE_SETU_RESULT`` message.

.. _The-SETU-Union-Peer_002dto_002dPeer-Protocol:

The SETU Union Peer-to-Peer Protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The SET union protocol is based on Eppstein's efficient set
reconciliation without prior context. You should read this paper first
if you want to understand the protocol.

.. todo:: Link to Eppstein's paper!

The union protocol operates over CADET and starts with a
GNUNET_MESSAGE_TYPE_SETU_P2P_OPERATION_REQUEST being sent by the peer
initiating the operation to the peer listening for inbound requests. It
includes the number of elements of the initiating peer, which is
currently not used.

The listening peer checks if the operation type and application
identifier are acceptable for its current state. If not, it responds
with a ``GNUNET_MESSAGE_TYPE_SETU_RESULT`` and a status of
``GNUNET_SETU_STATUS_FAILURE`` (and terminates the CADET channel).

If the application accepts the request, it sends back a strata estimator
using a message of type GNUNET_MESSAGE_TYPE_SETU_P2P_SE. The initiator
evaluates the strata estimator and initiates the exchange of invertible
Bloom filters, sending a GNUNET_MESSAGE_TYPE_SETU_P2P_IBF.

During the IBF exchange, if the receiver cannot invert the Bloom filter
or detects a cycle, it sends a larger IBF in response (up to a defined
maximum limit; if that limit is reached, the operation fails). Elements
decoded while processing the IBF are transmitted to the other peer using
GNUNET_MESSAGE_TYPE_SETU_P2P_ELEMENTS, or requested from the other peer
using GNUNET_MESSAGE_TYPE_SETU_P2P_ELEMENT_REQUESTS messages, depending
on the sign observed during decoding of the IBF. Peers respond to a
GNUNET_MESSAGE_TYPE_SETU_P2P_ELEMENT_REQUESTS message with the
respective element in a GNUNET_MESSAGE_TYPE_SETU_P2P_ELEMENTS message.
If the IBF fully decodes, the peer responds with a
GNUNET_MESSAGE_TYPE_SETU_P2P_DONE message instead of another
GNUNET_MESSAGE_TYPE_SETU_P2P_IBF.

All Bloom filter operations use a salt to mingle keys before hashing
them into buckets, such that future iterations have a fresh chance of
succeeding if they failed due to collisions before.
