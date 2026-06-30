
.. index::
   double: subsystem; REVOCATION

.. _REVOCATION-Subsystem-Dev:

REVOCATION
==========

The REVOCATION API consists of two parts, to query and to issue
revocations.

.. _Querying-for-revoked-keys:

Querying for revoked keys
^^^^^^^^^^^^^^^^^^^^^^^^^

``GNUNET_REVOCATION_query`` is used to check if a given ECDSA public key
has been revoked. The given callback will be invoked with the result of
the check. The query can be canceled using
``GNUNET_REVOCATION_query_cancel`` on the return value.

.. _Preparing-revocations:

Preparing revocations
^^^^^^^^^^^^^^^^^^^^^

It is often desirable to create a revocation record ahead-of-time and
store it in an off-line location to be used later in an emergency. This
is particularly true for GNUnet revocations, where performing the
revocation operation itself is computationally expensive and thus is
likely to take some time. Thus, if users want the ability to perform
revocations quickly in an emergency, they must pre-compute the
revocation message. The revocation API enables this with two functions
that are used to compute the revocation message, but not trigger the
actual revocation operation.

``GNUNET_REVOCATION_check_pow`` should be used to calculate the
proof-of-work required in the revocation message. This function takes
the public key, the required number of bits for the proof of work (which
in GNUnet is a network-wide constant) and finally a proof-of-work number
as arguments. The function then checks if the given proof-of-work number
is a valid proof of work for the given public key. Clients preparing a
revocation are expected to call this function repeatedly (typically with
a monotonically increasing sequence of numbers of the proof-of-work
number) until a given number satisfies the check. That number should
then be saved for later use in the revocation operation.

``GNUNET_REVOCATION_sign_revocation`` is used to generate the signature
that is required in a revocation message. It takes the private key that
(possibly in the future) is to be revoked and returns the signature. The
signature can again be saved to disk for later use, which will then
allow performing a revocation even without access to the private key.

.. _Issuing-revocations:

Issuing revocations
^^^^^^^^^^^^^^^^^^^

Given a ECDSA public key, the signature from ``GNUNET_REVOCATION_sign``
and the proof-of-work, ``GNUNET_REVOCATION_revoke`` can be used to
perform the actual revocation. The given callback is called upon
completion of the operation. ``GNUNET_REVOCATION_revoke_cancel`` can be
used to stop the library from calling the continuation; however, in that
case it is undefined whether or not the revocation operation will be
executed.

.. _The-REVOCATION-Client_002dService-Protocol:

The REVOCATION Client-Service Protocol
--------------------------------------

The REVOCATION protocol consists of four simple messages.

A ``QueryMessage`` containing a public ECDSA key is used to check if a
particular key has been revoked. The service responds with a
``QueryResponseMessage`` which simply contains a bit that says if the
given public key is still valid, or if it has been revoked.

The second possible interaction is for a client to revoke a key by
passing a ``RevokeMessage`` to the service. The ``RevokeMessage``
contains the ECDSA public key to be revoked, a signature by the
corresponding private key and the proof-of-work. The service responds
with a ``RevocationResponseMessage`` which can be used to indicate that
the ``RevokeMessage`` was invalid (e.g. the proof of work is incorrect),
or otherwise to indicate that the revocation has been processed
successfully.

.. _The-REVOCATION-Peer_002dto_002dPeer-Protocol:

The REVOCATION Peer-to-Peer Protocol
------------------------------------

Revocation uses two disjoint ways to spread revocation information among
peers. First of all, P2P gossip exchanged via CORE-level neighbours is
used to quickly spread revocations to all connected peers. Second,
whenever two peers (that both support revocations) connect, the SET
service is used to compute the union of the respective revocation sets.

In both cases, the exchanged messages are ``RevokeMessage``\ s which
contain the public key that is being revoked, a matching ECDSA
signature, and a proof-of-work. Whenever a peer learns about a new
revocation this way, it first validates the signature and the
proof-of-work, then stores it to disk (typically to a file
$GNUNET_DATA_HOME/revocation.dat) and finally spreads the information to
all directly connected neighbours.

For computing the union using the SET service, the peer with the smaller
hashed peer identity will connect (as a \"client\" in the two-party set
protocol) to the other peer after one second (to reduce traffic spikes
on connect) and initiate the computation of the set union. All
revocation services use a common hash to identify the SET operation over
revocation sets.

The current implementation accepts revocation set union operations from
all peers at any time; however, well-behaved peers should only initiate
this operation once after establishing a connection to a peer with a
larger hashed peer identity.
