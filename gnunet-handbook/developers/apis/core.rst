.. _CORE-Subsystem-Dev:

.. index::
   double: CORE; subsystem

CORE
====

The CORE API (defined in ``gnunet_core_service.h``) is the basic
messaging API used by P2P applications built using GNUnet. It provides
applications the ability to send and receive encrypted messages to the
peer's \"directly\" connected neighbours.

As CORE connections are generally \"direct\" connections, applications
must not assume that they can connect to arbitrary peers this way, as
\"direct\" connections may not always be possible. Applications using
CORE are notified about which peers are connected. Creating new
\"direct\" connections must be done using the TRANSPORT API.

The CORE API provides unreliable, out-of-order delivery. While the
implementation tries to ensure timely, in-order delivery, both message
losses and reordering are not detected and must be tolerated by the
application. Most important, the core will NOT perform retransmission if
messages could not be delivered.

Note that CORE allows applications to queue one message per connected
peer. The rate at which each connection operates is influenced by the
preferences expressed by local application as well as restrictions
imposed by the other peer. Local applications can express their
preferences for particular connections using the \"performance\" API of
the ATS service.

Applications that require more sophisticated transmission capabilities
such as TCP-like behavior, or if you intend to send messages to
arbitrary remote peers, should use the CADET API.

The typical use of the CORE API is to connect to the CORE service using
``GNUNET_CORE_connect``, process events from the CORE service (such as
peers connecting, peers disconnecting and incoming messages) and send
messages to connected peers using ``GNUNET_CORE_notify_transmit_ready``.
Note that applications must cancel pending transmission requests if they
receive a disconnect event for a peer that had a transmission pending;
furthermore, queuing more than one transmission request per peer per
application using the service is not permitted.

The CORE API also allows applications to monitor all communications of
the peer prior to encryption (for outgoing messages) or after decryption
(for incoming messages). This can be useful for debugging, diagnostics
or to establish the presence of cover traffic (for anonymity). As
monitoring applications are often not interested in the payload, the
monitoring callbacks can be configured to only provide the message
headers (including the message type and size) instead of copying the
full data stream to the monitoring client.

The init callback of the ``GNUNET_CORE_connect`` function is called with
the hash of the public key of the peer. This public key is used to
identify the peer globally in the GNUnet network. Applications are
encouraged to check that the provided hash matches the hash that they
are using (as theoretically the application may be using a different
configuration file with a different private key, which would result in
hard to find bugs).

As with most service APIs, the CORE API isolates applications from
crashes of the CORE service. If the CORE service crashes, the
application will see disconnect events for all existing connections.
Once the connections are re-established, the applications will be
receive matching connect events.

core client-service protocol
.. _The-CORE-Client_002dService-Protocol:

The CORE Client-Service Protocol
--------------------------------

This section describes the protocol between an application using the
CORE service (the client) and the CORE service process itself.

.. _Setup2:

Setup2
^^^^^^

When a client connects to the CORE service, it first sends a
``InitMessage`` which specifies options for the connection and a set of
message type values which are supported by the application. The options
bitmask specifies which events the client would like to be notified
about. The options include:

**GNUNET_CORE_OPTION_NOTHING**
   No notifications

**GNUNET_CORE_OPTION_STATUS_CHANGE**
   Peers connecting and disconnecting

**GNUNET_CORE_OPTION_FULL_INBOUND**
   All inbound messages (after decryption) with full payload

**GNUNET_CORE_OPTION_HDR_INBOUND** 
   Just the ``MessageHeader`` of all inbound messages

**GNUNET_CORE_OPTION_FULL_OUTBOUND**
   All outbound messages (prior to encryption) with full payload

**GNUNET_CORE_OPTION_HDR_OUTBOUND**
   Just the ``MessageHeader`` of all outbound messages

Typical applications will only monitor for connection status changes.

The CORE service responds to the ``InitMessage`` with an
``InitReplyMessage`` which contains the peer's identity. Afterwards,
both CORE and the client can send messages.

.. _Notifications:

Notifications
^^^^^^^^^^^^^

The CORE will send ``ConnectNotifyMessage``\ s and
``DisconnectNotifyMessage``\ s whenever peers connect or disconnect from
the CORE (assuming their type maps overlap with the message types
registered by the client). When the CORE receives a message that matches
the set of message types specified during the ``InitMessage`` (or if
monitoring is enabled in for inbound messages in the options), it sends
a ``NotifyTrafficMessage`` with the peer identity of the sender and the
decrypted payload. The same message format (except with
``GNUNET_MESSAGE_TYPE_CORE_NOTIFY_OUTBOUND`` for the message type) is
used to notify clients monitoring outbound messages; here, the peer
identity given is that of the receiver.

.. _Sending:

Sending
^^^^^^^

When a client wants to transmit a message, it first requests a
transmission slot by sending a ``SendMessageRequest`` which specifies
the priority, deadline and size of the message. Note that these values
may be ignored by CORE. When CORE is ready for the message, it answers
with a ``SendMessageReady`` response. The client can then transmit the
payload with a ``SendMessage`` message. Note that the actual message
size in the ``SendMessage`` is allowed to be smaller than the size in
the original request. A client may at any time send a fresh
``SendMessageRequest``, which then superceeds the previous
``SendMessageRequest``, which is then no longer valid. The client can
tell which ``SendMessageRequest`` the CORE service's
``SendMessageReady`` message is for as all of these messages contain a
\"unique\" request ID (based on a counter incremented by the client for
each request).

CORE Peer-to-Peer Protocol
.. _The-CORE-Peer_002dto_002dPeer-Protocol:

The CORE Peer-to-Peer Protocol
------------------------------

EphemeralKeyMessage creation
.. _Creating-the-EphemeralKeyMessage:

Creating the EphemeralKeyMessage
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When the CORE service starts, each peer creates a fresh ephemeral (ECC)
public-private key pair and signs the corresponding
``EphemeralKeyMessage`` with its long-term key (which we usually call
the peer's identity; the hash of the public long term key is what
results in a ``struct GNUNET_PeerIdentity`` in all GNUnet APIs. The
ephemeral key is ONLY used for an ECDHE (`Elliptic-curve
Diffie---Hellman <http://en.wikipedia.org/wiki/Elliptic_curve_Diffie%E2%80%93Hellman>`__)
exchange by the CORE service to establish symmetric session keys. A peer
will use the same ``EphemeralKeyMessage`` for all peers for
``REKEY_FREQUENCY``, which is usually 12 hours. After that time, it will
create a fresh ephemeral key (forgetting the old one) and broadcast the
new ``EphemeralKeyMessage`` to all connected peers, resulting in fresh
symmetric session keys. Note that peers independently decide on when to
discard ephemeral keys; it is not a protocol violation to discard keys
more often. Ephemeral keys are also never stored to disk; restarting a
peer will thus always create a fresh ephemeral key. The use of ephemeral
keys is what provides `forward
secrecy <http://en.wikipedia.org/wiki/Forward_secrecy>`__.

Just before transmission, the ``EphemeralKeyMessage`` is patched to
reflect the current sender_status, which specifies the current state of
the connection from the point of view of the sender. The possible values
are:

-  ``KX_STATE_DOWN`` Initial value, never used on the network

-  ``KX_STATE_KEY_SENT`` We sent our ephemeral key, do not know the key
   of the other peer

-  ``KX_STATE_KEY_RECEIVED`` This peer has received a valid ephemeral
   key of the other peer, but we are waiting for the other peer to
   confirm it's authenticity (ability to decode) via challenge-response.

-  ``KX_STATE_UP`` The connection is fully up from the point of view of
   the sender (now performing keep-alive)

-  ``KX_STATE_REKEY_SENT`` The sender has initiated a rekeying
   operation; the other peer has so far failed to confirm a working
   connection using the new ephemeral key

.. _Establishing-a-connection:

Establishing a connection
^^^^^^^^^^^^^^^^^^^^^^^^^

Peers begin their interaction by sending a ``EphemeralKeyMessage`` to
the other peer once the TRANSPORT service notifies the CORE service
about the connection. A peer receiving an ``EphemeralKeyMessage`` with a
status indicating that the sender does not have the receiver's ephemeral
key, the receiver's ``EphemeralKeyMessage`` is sent in response.
Additionally, if the receiver has not yet confirmed the authenticity of
the sender, it also sends an (encrypted)\ ``PingMessage`` with a
challenge (and the identity of the target) to the other peer. Peers
receiving a ``PingMessage`` respond with an (encrypted) ``PongMessage``
which includes the challenge. Peers receiving a ``PongMessage`` check
the challenge, and if it matches set the connection to ``KX_STATE_UP``.

.. _Encryption-and-Decryption:

Encryption and Decryption
^^^^^^^^^^^^^^^^^^^^^^^^^

All functions related to the key exchange and encryption/decryption of
messages can be found in ``gnunet-service-core_kx.c`` (except for the
cryptographic primitives, which are in ``util/crypto*.c``). Given the
key material from ECDHE, a Key derivation function (`Key derivation
function <https://en.wikipedia.org/wiki/Key_derivation_function>`__) is
used to derive two pairs of encryption and decryption keys for AES-256
and TwoFish, as well as initialization vectors and authentication keys
(for HMAC (`HMAC <https://en.wikipedia.org/wiki/HMAC>`__)). The HMAC is
computed over the encrypted payload. Encrypted messages include an
iv_seed and the HMAC in the header.

Each encrypted message in the CORE service includes a sequence number
and a timestamp in the encrypted payload. The CORE service remembers the
largest observed sequence number and a bit-mask which represents which
of the previous 32 sequence numbers were already used. Messages with
sequence numbers lower than the largest observed sequence number minus
32 are discarded. Messages with a timestamp that is less than
``REKEY_TOLERANCE`` off (5 minutes) are also discarded. This of course
means that system clocks need to be reasonably synchronized for peers to
be able to communicate. Additionally, as the ephemeral key changes every
12 hours, a peer would not even be able to decrypt messages older than
12 hours.

.. _Type-maps:

Type maps
^^^^^^^^^

Once an encrypted connection has been established, peers begin to
exchange type maps. Type maps are used to allow the CORE service to
determine which (encrypted) connections should be shown to which
applications. A type map is an array of 65536 bits representing the
different types of messages understood by applications using the CORE
service. Each CORE service maintains this map, simply by setting the
respective bit for each message type supported by any of the
applications using the CORE service. Note that bits for message types
embedded in higher-level protocols (such as MESH) will not be included
in these type maps.

Typically, the type map of a peer will be sparse. Thus, the CORE service
attempts to compress its type map using ``gzip``-style compression
(\"deflate\") prior to transmission. However, if the compression fails
to compact the map, the map may also be transmitted without compression
(resulting in ``GNUNET_MESSAGE_TYPE_CORE_COMPRESSED_TYPE_MAP`` or
``GNUNET_MESSAGE_TYPE_CORE_BINARY_TYPE_MAP`` messages respectively).
Upon receiving a type map, the respective CORE service notifies
applications about the connection to the other peer if they support any
message type indicated in the type map (or no message type at all). If
the CORE service experience a connect or disconnect event from an
application, it updates its type map (setting or unsetting the
respective bits) and notifies its neighbours about the change. The CORE
services of the neighbours then in turn generate connect and disconnect
events for the peer that sent the type map for their respective
applications. As CORE messages may be lost, the CORE service confirms
receiving a type map by sending back a
``GNUNET_MESSAGE_TYPE_CORE_CONFIRM_TYPE_MAP``. If such a confirmation
(with the correct hash of the type map) is not received, the sender will
retransmit the type map (with exponential back-off).


