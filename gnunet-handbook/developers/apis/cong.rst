.. _CONG-Subsystem-Dev:

.. index::
   double: CONG; subsystem

CONG
====

CONG (COre Next Generation) is the name of the project redesigns the
:ref:`CORE-Subsystem-Dev` service. Here we document the design decisions and
parts that are changing.
The most notable change concerns peer ids: In order to avoid location tracking,
they are being made non-permanent - they change with each change of underlying
addresses. Next to that, the cryptographic primitives in use change, and the
interface to the (transport) layers below. The interface to the underlying
layers is made more generic so that libp2p can be switched in for gnunet's own
transport (layer 2 overlay/L2O). Finally, protocol-versioning above core will
be introduced.

..
  TODO (from project plan)
  - cryptographic protocol:
    - key exchange
    - key management

..
  TODO write a short overview
  - peer ids
  - libp2p
  - protocol versioning

..
  Design goals
  ------------
   - limit tracking


Key exchange
------------

While we are at it we may as well improve the key exchange
(:ref:`The-CORE-Peer_002dto_002dPeer-Protocol`).
Currently, we are using our own ECDHE key exchange that derives 2x2 keys.
2 keys for each direction (sending/receiving).
Each direction uses two 256-bit symmetric encryption keys derived through the ECDH exchange.
Each payload is encrypted using AES(kA, Twofish(kB, payload)) both in CFB mode (!).

..
  TODO Ephemeral key derivation, material sent, checks, ...
  TODO Initiate handshake

Next Steps
^^^^^^^^^^

For CONG, we should double-check the security of your ECDHE construction.
We decided on 11/03/2024 to investigate XChaCha20-Poly1305:

Proposal:

  * Use X25519 for the KX with ephemeral Curve25519 keys.
  * Use XChaCha20-Poly1305 and kTx,kRx := KDF(X25519(),senderPK,receiverPK) for symmetric encryption

We will have to replace the use of ``GNUNET_CRYPTO_symmetric_encrypt`` and
HMAC use in ``gnunet-service-core_kx.c`` including the respective keys and IVs.

Handshake Protocol (Current)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. warning:: This is incomplete as the protocol is very messy and has around 6 RTTs

We assume that the peers have semi-*static* (as opposed to ephemeral) key pairs.
Let (pk\ :sub:`A`,sk\ :sub:`A`) be the key pair of peer PID\ :sub:`A` and (pk\ :sub:`B`,sk\ :sub:`B`) the
key pair of peer PID\ :sub:`B`.

For any secure handshake protocol, we have to dermine an initiator and a receiver in the protocol.
We use `GNUNET_CRYPTO_hash_cmp` to determine which peer is the receiver `R` and which peer the initiator `I`:

.. code-block:: c

  if (GNUNET_CRYPTO_hash_cmp (pk_A, pk_B))
  {
    pk_I = pk_A
    pk_R = pk_B
  }
  else
  {
    pk_I = pk_B
    pk_R = pk_A
  }

It is possible that the designated initiator does not initiate the handshake. After a pre-determined timeout,
the respective other peer may initiate.
  
We assume that the initiator knows pk\ :sub:`R` (pre-distributed through HELLO, for example).

``I`` and ``R`` calculate *before any connection attempt is made*:

* (pk\ :sub:`e`,sk\ :sub:`e`) <- *KeyGen*\ ()

.. danger:: Yes, both peers calculate *ephemeral* keys that are used for a set period of time in **all** handshakes.

``I`` calculates:

* ``EphemeralKeyMessage`` <- (pk\ :sub:`I`, pk\ :sub:`e`, creation_time, ...)
* sig\ :sub:`e` <- *Sign*\ (sk\ :sub:`I`, ``EphemeralKeyMessage``)
  
.. admonition:: ``I`` sends to ``R``

                ``EphemeralKeyMessage``, sig\ :sub:`e`

``R`` calculates:

* assert *Verify*\ (pk\ :sub:`R`, ``EphemeralKeyMessage``, sig\ :sub:`e`)
* Establish session keys through ECDH with *ephemeral* keys.
* ``EphemeralKeyMessage`` <- (pk\ :sub:`R`, pk\ :sub:`e`, creation_time, ...)
* sig\ :sub:`e` <- *Sign*\ (sk\ :sub:`R`, ``EphemeralKeyMessage``)
  
.. admonition:: ``R`` sends to ``I``

                ``EphemeralKeyMessage``, sig\ :sub:`e`

``I`` calculates:

* assert *Verify*\ (pk\ :sub:`R`, ``EphemeralKeyMessage``, sig\ :sub:`e`)
* Establish session keys through ECDH with *ephemeral* keys.

.. admonition:: ``I`` sends to ``R``

                ``PingMessage``

``R`` calculates:

* Pong message

.. admonition:: ``R`` sends to ``I``

                ``PongMessage``


Draft: CORE Authenticated Key Exchange (CAKE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. seealso:: This protocol is derived from `KEMTLS <https://thomwiggers.nl/publication/thesis/thesis.pdf>`_ (page 81ff).


The initiator selection remains unchanged from the above protocol.



First Message (RTT=0) 
"""""""""""""""""""""

``I`` sends to ``R`` the following message:

``MessageHeader||InitiatorHello||EncryptedInitiatorCert``

where:

``MessageHeader`` is a ``GNUNET_MessageHeader`` of type *TBD* (GANA registration).

The ``InitiatorHello`` consists of:

* (pk\ :sub:`e`, c\ :sub:`R`, SHA512(pk\ :sub:`R`), r\ :sub:`I`, [SupportedAlgs,Version])

r\ :sub:`I` is nonce value (256 or 512 bit TBD).
pk\ :sub:`e` is the public key from a freshly generated ephemeral key pair:

* (pk\ :sub:`e`,sk\ :sub:`e`) <- *KeyGen*\ ()

In GNUnet this ``KeyGen`` corresponds to ``GNUNET_CRYPTO_ecdhe_key_create()``. This is a X25519 key pair.
The ``InitiatorHello`` may contain our version and other metadata which
is indicated by the brackets. But, we may want to put this information into the ``cert`` below, as this is encrypted
(identity hiding).

The ``EncryptedInitiatorCert`` is created as 

* *Enc*\ (**ETS**, cert [pk\ :sub:`I`])

We may encode the capabilities/supported class in ``cert``.
We do not want to use X.509 here, probably. ``cert`` is just a placeholder for the
signed metadata (e.g. supported services string) and pk\ :sub:`I`.

.. admonition:: Certificate definition
                
                The definition of certificate is incomplete.
                For now, we only use self-signed certificates: The cert is signed using sk\ :sub:`I`. Use ``GNUNET_CRYPTO_eddsa_sign`` API for this.


.. admonition:: Supported services string
                
                Supported services are just a series of *service*:*version* strings
                separated by a separator. We may want to use libtool versioning?

*Enc* is XChaCha20-Poly1305 (IETF version).
To derive **ETS** we create our first shared secret as:

* (ss\ :sub:`R`,c\ :sub:`R`) <- *Encaps*\ (pk\ :sub:`R`)

pk\ :sub:`R` is the EdDSA public key of the peer we want to connect to.
We should have received this as part of the trigger for this message.
In GNUnet, ``Encaps`` corresponds to ``GNUNET_CRYPTO_eddsa_kem_encaps``.

.. admonition:: X25519 vs EdDSA
                Notice how X25519 and EdDSA keys are different. Accordingly, different/additional KEM implementations may be required in this handshake, but here the EdDSA version is correct.

We then derive our encryption keys:

* **ES** <- *HKDF-Extract*\ (ss\ :sub:`R`, 0)
* **ETS** <- *HKDF-Expand*\ (**ES**, ``"early data"``, ``InitiatorHello``)

and encrypt the certificate giving us the ``EncryptedInitiatorCert``.



Second Message (RTT=0.5) 
""""""""""""""""""""""""

``R`` the receives:

``MessageHeader||InitiatorHello||EncryptedInitiatorCert``

and sends to ``I`` the following message:

``MessageHeader||ReceiverHello||EncryptedExtensions||ReceiverKemCiphertext||ReceiverFinished``

The message may have already application payload appended, but in our case this is unlikely.

First ``R`` processes the message received from ``I``:

* Verify that the message type is *TBD*

* Decrypt ``EncryptedInitiatorCert`` (cf. encryption above):

  * (ss\ :sub:`R`) <- Decaps(sk\ :sub:`R`, c\ :sub:`R`)
  * **ES** <- *HKDF-Extract*\ (ss\ :sub:`R`, 0)
  * **ETS** <- *HKDF-Expand*\ (**ES**, "early data", ``InitiatorHello``) 
  * cert [pk\ :sub:`I`] <- *Dec*\ (**ETS**, ``EncryptedInitiatorCert``)

.. admonition:: Encaps X25519

                Here, ``Decaps`` corresponds to ``GNUNET_CRYPTO_eddsa_kem_decaps``.

* Setup Handshake and Master Secrets (these can also be done as-needed and not necessarily all at once here):

  * **dES** <- *HKDF-Expand*\ (**ES**, "derived", ``NULL``)
  * (ss\ :sub:`e`,c\ :sub:`e`) <- *Encaps*\ (pk\ :sub:`e`)
  * **HS** <- *HKDF-Extract*\ (ss\ :sub:`e`, **dES**)
  * **dHS** <-  *HKDF-Expand*\ (**HS**, "derived", ``NULL``)
  * (ss\ :sub:`I`,c\ :sub:`I`) <- ``Encaps``\ (pk\ :sub:`I`)
  * **MS** <- *HKDF-Extract*\ (ss\ :sub:`I`, **dHS**)

.. admonition:: Encaps X25519

                Here, ``Encaps`` corresponds to ``GNUNET_CRYPTO_hpke_kem_encaps``.
    
* Derive Handshake Traffic Encryption Keys:

  * ``ReceiverHello`` <- (c\ :sub:`e`, r\ :sub:`R`, [SelectedAlgs])
  * **IHTS** <- *HKDF-Expand*\ (**HS**, "i hs traffic", ``InitiatorHello...ReceiverHello``)
  * **RHTS** <- *HKDF-Expand*\ (**HS**, "r hs traffic", ``InitiatorHello...ReceiverHello``)
  * ``ReceiverKemCiphertext`` <- *Enc*\ (**RHTS**, c\ :sub:`I`)
  * (Optional) ``EncryptedExtensions`` <- *Enc*\ (**RHTS**, SupportedAlgs/Services?)

* Build ``ReceiverFinished`` message:

  * fk\ :sub:`R` <- *HKDF-Expand*\ (**MS**, "r finished", ``NULL``)
  * ``RF`` <- *HMAC*\ (fk\ :sub:`R`, ``InitiatorHello...ReceiverKemCiphertext``)
  * ``ReceiverFinished`` <- *Enc*\ (**RHTS**, ``RF``) (TLS1.3-style explicit authentication of receiver after 1RTT!)

* Derive Application Traffic Encryption Key:

  * **RATS** <- *HKDF-Expand*\ (**MS**, "r ap traffic", ``InitiatorHello...ReceiverFinished``)


Third Message (RTT=1.5) 
"""""""""""""""""""""""

``I`` receives:

``MessageHeader||ReceiverHello||EncryptedExtensions||ReceiverKemCiphertext||ReceiverFinished``

and sends to ``I`` the following message:

``MessageHeader||IteratorFinished``

The message may have already application payload appended, but in our case this again is unlikely.

First ``I`` processes the message received from ``R``:

* Verify that the message type is *TBD*

* Setup Master Secret (cf. derivation in Second Message):

  * (ss\ :sub:`e`) <- *Decaps*\ (sk\ :sub:`e`, c\ :sub:`e`) (X25519 KEM)

  * **dES** <- *HKDF-Expand*\ (**ES**, ``"derived"``, ``NULL``)
  * **HS** <- *HKDF-Extract*\ (ss\ :sub:`e`, **dES**)
  * **dHS** <- *HKDF-Expand*\ (**HS**, "derived", ``NULL``)
  * (ss\ :sub:`I`) <- *Decaps*\ (sk\ :sub:`I`, c\ :sub:`I`) (EdDSA KEM)
  * **MS** <- *HKDF-Extract*\ (ss\ :sub:`I`, **dHS**)


* Derive Traffic Encryption Keys (these can also be done as-needed and not necessarily all at once here):

  * **IHTS** <- *HKDF-Expand*\ (**HS**, "i hs traffic", ``InitiatorHello...ReceiverHello``)
  * **RHTS** <- *HKDF-Expand*\ (**HS**, "r hs traffic", ``InitiatorHello...ReceiverHello``)
  * **IATS** <- *HKDF-Expand*\ (**MS**, "i ap traffic", ``InitiatorHello...InitiatorFinished``)
  * **RATS** <- *HKDF-Expand*\ (**MS**, "r ap traffic", ``InitiatorHello...ReceiverFinished``)

* Build ``ReceiverFinished`` and ``InitiatorFinished`` plain texts:

  * fk\ :sub:`I` <- *HKDF-Expand*\ (**MS**, "i finished", NULL)
  * ``IF`` <- *HMAC*\ (fk\ :sub:`I`, ``InitiatorHello...ReceiverFinished``)
  * fk\ :sub:`R` <- *HKDF-Expand*\ (**MS**, "r finished", NULL)
  * ``RF`` <- *Dec*\ (RHTS, ``ReceiverFinished``)
  * assert *HMAC*\ (fk\ :sub:`R`, ``InitiatorHello...ReceiverKemCiphertext``) == ``RF``

* ``InitiatorFinished`` <- *Enc*\ (**IHTS**, ``IF``)


Confirmation (RTT=1.5)
""""""""""""""""""""""

``R`` receives ``IteratorFinished`` and computes:

* ``IF`` <- *Dec*\ (IHTS, ``InitiatorFinished``)
* fk\ :sub:`I` <- *HKDF-Expand*\ (**MS**, "i finished", ``NULL``)
* assert *HMAC*\ (fk\ :sub:`I`, ``InitiatorHello...ReceiverFinished``) == ``IF``
* **IATS** <- *HKDF-Expand*\ (**MS**, "i ap traffic", ``InitiatorHello...InitiatorFinished``)


At this point we have a secure channel and application payload can be en/decrypted using **IATS** and **RATS**, respectively.

Rekey / Service Status change
"""""""""""""""""""""""""""""

TODO


Edge Cases
""""""""""

* The Initiator/Receiver selection logic may require a timed fallback: The designates Initiator may never initiate (NAT, already has sufficient connections, learns about receiver later than receiver about initiator etc.)
* This may result in edge cases where the Initiator initiates a handshake and the Receiver also initiates a handshake at the same time switching roles.
* In such cases we may simply do both key exchanges. If both succeed, we drop the key exchange that was not initiated by the designated initiator on both peers. Otherwise we use the successful key exchange and the roles are swapped. 


Glossary
""""""""

  * **IATS**: Initiator Application Traffic Secret Key
  * **RATS**: Receiver Application Traffic Secret Key
  * **dES**: Derived Early Secret Key
  * **dHS**: Derived Handshake Key
  * **ES**: Early Secret Key
  * **ETS**: Early Traffic Secret Key
  * **HS**: Handshake Secret Key
  * **MS**: Main Secret Key
  * **ES**: Early Secret Key
  * **IHTS**: Initiator Handshake Secret Key
  * **RHTS**: Receiver Handshake Secret Key
  * ``Foo...Bar`` means the transcript of received/send messages from ``Foo`` until ``Bar``
  

Unified Address Format for L2O and libp2p
-----------------------------------------

..
  TODO
  libp2p peer-id multiaddress?, gnunet-hello
  https://github.com/libp2p/specs/blob/master/peer-ids/peer-ids.md
  https://github.com/libp2p/specs/blob/master/addressing/README.md#multiaddr-basics
  https://docs.gnunet.org/latest/users/subsystems.html#hellos
  https://docs.gnunet.org/latest/users/subsystems.html#hello-ng
  https://docs.libp2p.io/concepts/fundamentals/addressing/
  https://github.com/multiformats/multiaddr/

As a unified address format for L2O and libp2p we will use a concatenation of
the string representations of gnunet's hellos and libp2p's multiaddress,
separated by `;;`.
For example: `gnunet://hello/XXPIDXX/XXPIDXX/1725622944?udp=%5B%3A%3A1%5D%3A2086&;;libp2p:///ip4/127.0.0.1/tcp/24915`

This is only for the time being. For the long run the integration within each
other's addressing schemes should be evaluated. Meaning: Integrate a
gnunet-hello address type in libp2p's multiaddress format and integrate the
multiaddress format with the gnunet-hello.


.. _Peer-IDs:

Peer IDs
--------

Peer ids stop to be unique for the lifetime of a peer, but change each time a
peer's addresses change. This includes gaining or losing an address.

It is important to note that this design choice only increases the cost of
network location tracking and does not fully prevent it. For this feature onion
routing on top of CADET is envisioned.

At this point it seems like one has to weigh privacy versus performance when it
comes to this design decision.

.. _Reasoning:

Reasoning
^^^^^^^^^

..
  	 - why this decision (and not others?)
  		 - (scenarios to understand whether it makes sense)

This change was introduced in order to stop tracking of more mobile peers
across the network. For example a more mobile peer (laptop) that logs into the
network at different places can be easily tracked by everyone just by recording
the different addresses that are tied to that peer id over time.

..
  TODO
   - why does this prevent tracking? (partially answered in the attacker
     model section below
   - tracking hellos of peer: visible
   - reverse mapping (address to peer?) not possible - there's no functionality
     for that
   - gaining/losing as criterium to change peer id makes sense?


.. _Attacker-Model:

Attacker Model
""""""""""""""
An attacker observes the hellos (containing ip and other addresses) published
under a peer id and is thus capable of tracking locations and thus obtaining a
movement profile of this peer.

With the proposed changes to the peer id an attacker can only see a peer id and
its connected set of addresses. A movement profile can not be obtained in the
previous way.

..
  TODO how much info can be gained by that? all an observing peer sees is the
  ip addresses. how much can this tell?
  - mobile provider (implicit: mobile connection)
  - internet provider - home, company, public, ...
  - region (country, city, building, ...)

Tracking of addresses/locations might still be possible in the scenario that a
mobile client uses mobile broadband and wifi uplinks and uses them in an
'overlapping' manner. (Switching on mobile broadband before leaving the range
of a wifi hotspot and vice versa Turning off mobile broadband after connecting
to a wifi.) For the 'overlapping' time the peer would publish a HELLO
containing the old and the new network addresses. After the overlapping time,
the peer's HELLO would just contain the new network address, which was already
in the HELLO from during the 'overlap'. That way the overlap can be used to
link the old and new address and - in extreme scenarios - obtain the full
movement profile again. Note that this does not work on all, not all the time
and requires work for the correlation. It should be highlighted that, although
this attack is possible, the new design still greatly reduces the attack
surface.

A way to circumvent this anti-tracking mechanism would be for an attacker to
exploit the means for consistently connect to the same peer.
.. TODO wording/conceptualisation - overlap with next sentence
For example with the means of higher-layer services like gns. With the
knowledge of the gns entry, its peer ids and thus its addresses can be fully
tracked. Until the existence of an onion-routing service and its decoupling of
identities and network addresses, this behavior is probably intended and maybe
the only way to connect endpoints. Although this leak is still existent the
new design still prevents a lot of other leakage. Also it is important to point
out that it is impossible to prevent leakage at higher layers, but necessary to
prevent leakage at this layer, because leakage at this level could not be fixed
at higher layers.

.. TODO review for wording/clarity


.. _Implications-PeerIDs:

Implications
^^^^^^^^^^^^

Here we present the implications for different parts of the framework. This
should help get a grasp of the implications of the change.
.. TODO poor phrasing?

.. _DHT:

DHT
"""

The DHT uses the peer id such that it determines which buckets a peer is
responsible for. So each time the peer id changes, the peer becomes responsible
for different data. This means that each time a peer changes address, it leaves
and re-enters the DHT, changes its and other peer's neighbors and changes the
stored data.


.. _Scope-of-Peer-ID-for-higher-Layers:

Scope of Peer ID for higher Layers
""""""""""""""""""""""""""""""""""

When peer ids stop to be unique over time, the framework is in lack of a
globally unique identifier. Higher layers may rely (have relied) on the
uniqueness of the ids. This means gnunet has to use other means for this
purpose. The Reconnects_ section below is concerned with the specific impact on
reconnects for different higher-layer services. In general gns/identity offers
this functionality.

As peer ids cease to be unique over time, this might be a good point to review
the scope of its and other elements' usage and terminology. (See
Open_Design_Questions_)

.. FIXME This section overlaps in scope with the next section.


.. _Reconnects:

Reconnects
""""""""""

When addresses (and with those the peer id) of a peer change, all core
connections need to be torn down and with them all higher-layer connections.
This affects the layers above CADET as follows:

 - Revocation: Is not really affected as it is only connected to direct CADET
   neighbors and makes no use of CADET's routing, only of its flow and
   congestion control.
 - File sharing: Only the non-anonymous filesharing uses CADET connections.
   This is not significantly affected by a reconnect as it only looks up peer
   and port in the DHT, so in the meanwhile it's looking for other peers. TODO
   this is very unclear to me!
 - Messenger: All CADET connections would break and the peer might assume that
   all previously connected peers went offline. So it would require a
   mechanism to reconnect to peers with a known peer identity which offer
   routing capability (open port via CADET to connect to). In case the peer
   itself is providing such capability, it would help to know about peer ID
   changes ahead of time to communicate a switch between IDs to other peers.
   For other reconnections via GNS lookups are required.
 - Conversation: The call would be interrupted until the new peer id of the
   other has been found (potentially via GNS).

Open question: is gns needed for a reconnect? Could the peer with the new
id not simply 'call back' the other peer? Of course this would only work if
only one peer changes its peer id. If both change their peer id at the same
time, an external mechanism would be needed.

See Open_Design_Questions_ for thoughts on good designs to handle address
changes more smoothly.


.. _Messenger-Implications:

Messenger
"""""""""

The current implementation of messenger heavily relies on a globally unique
peer id. The change requires messenger to account for peer id changes.


.. _Details-on-how_PeerIDs:

Details on how
^^^^^^^^^^^^^^

Peer ids will be generated from the set of a peer's current addresses. Once a
peer obtains the same set of addresses it shall be using the same peer id. To
achieve this behavior, the current string representation of addresses is
sorted, the sorted representation hashed and the peer id generated from the
hash. We call the hash 'network generation hash'. The hash can be used to
quickly identify and recognise a set of addresses that was used in the past.

Once a peer id changes, all communication via open channels shall immediately
cease. To signal this, the mq is to be used.
To identify all queued messages that are to be cancelled, the 'network
generation id' is used. (This was decided to be implemented as a simple counter
of 64 bit.) With each address and peer id change, the network generation id is
incremented. When enqueueing a message with the mq, the current generation id
is stored with is. This way, when the generation id changes, all enqueued
messages with the old id (which still might refer to the old peer id in some
way) can be identified and dequeued. It is important to manage the network
generation id as close to the communicators as possible to be able to stop the
actual outgoing messages as quick as possible.

To be more precise, the communication via open channels should not cease when
the new peer id is available, but already when there's a change to the
addresses in use. This means there is a small window, until the new peer id is
generated, in which the peer is without peer id and thus without means to
communicate. This might open a new attack window by trying to change the
addresses of a peer via for example opening and closing wifi hotspots and
sending out new addresses via dhcp.

..
  64 bit: similar size to pid/hash

..
   - adress bundle (transport communicators implement decision?)
   - I vaguely remember the talk of 'insignificant' address changes (within
     local network or such), that don't require a new peer id, but forgot the
     specifics.


.. _Open-questions:

Open questions
^^^^^^^^^^^^^^

..
  overall for this "peer id" section
  Notes from the meeting:
   - higher-layer roll-over
  	 - problem in practice 'phone call not interrupted by change of
  	   location'
  	 - solved by gns entries? and re-connect?
  	 - cadet search for peer id - search in dht and via gns
  	 - caching-problem for gns
  	 - no signaling for roll-over
  	 - estimated reconnect within 15 minutes
  	 - change of address/peerid results in unclear state (down? lost
  	   connection, ....?)
  	 - cadet needs to regularily check gns entries
   - how signal the change of addresses?
  	 - do we signal trear-down?
  	 - scenario address change in internet cafe?
  	 - choice between privacy and connectivity
  	 - sense of changing ids for cadet?
  		 - -> rendez-vous peer (more stable)
  	 - tracking via gns?
  		 - only for people who are connected/known (tracking might be
  		   ok for this)
  			 - requires pir for gns
  		 - tracking impossible/hard for others


.. _Ownership-of-Peer-IDs:

Ownership of Peer IDs
^^^^^^^^^^^^^^^^^^^^^

When the peer id was static, all parts of gnunet had a simple way to interface
with it. Once it becomes dynamic, it makes a lot of sense that a single part
takes control/responsibility/ownership for it. A new service is created for
this purpose. A name suggestion was "peer id lifecycle service" - PILS.

The reasons for this new service:

 - Good encapsulation (which is even more important as it is a component deals
   with crucial cryptographic operation).
 - Avoidance of circular subscriptions of core and transport.
 - Avoidance of callback api hell between core and transport.
 - Using peerstore for this would be really messy.

The reasons against this new service:
 - Having yet another service.

Core will be the responsible service to provide addresses to pils for the
switching of peer ids.

Peer ids should not be stored in permanent memory (on disk), but kept in
working memory.
They will be (re-) computed from the new set of addresses. The advantage is
that it is not possible to prove that a peer id belonged to a peer by
examining its hard-drive. Keeping the respective HELLO in the peerstore is fine
as a peer also keeps other peer's hello in there and the existence of a hello
in the peerstore proves nothing.


.. _Implications-onwnership_PeerIDs:

Implications
^^^^^^^^^^^^

Pils needs to take ownership. It is responsible for generating, changing of
peer ids, informing subscribers of changes and signing with the current peer
id. (To be gentle on the ipc, it should not sign big amounts of data - if
applicable rather hashes of data or such.)


.. _Transport:

Transport
"""""""""

Up until now transport still creates, signs and puts hellos into the peerstore.
When pils takes ownership of peer ids, transport will need to ask it for
signing the hellos before putting it into the peerstore. (With libp2p as
alternative/additional underlay in mind, everything related to hellos needs to
move to core eventually as libp2p is oblivious about that part of gnunet. Core
will have to create and provide the hellos. See libp2p-Underlay_ below.)


.. _Other:

Other
"""""

Generally, all parts that so far read the peer id from a file into local
memory, will need to ask pils for it. All services that sign/encrypt something
with the peer id now will need to ask pils to do it.


.. _Details-on-how_ownership_PeerIDs:

Details on how
^^^^^^^^^^^^^^

In order to provide the needed functionality, pils needs to expose it via its
api:

 - a call to provide the current set of addresses, so the generation of the
   peer id can be triggered.
 - a call to obtain the current peer id
 - registering a handler that informs about new peer ids
 - signing data with the peer id



.. _libp2p-Underlay:

libp2p Underlay
---------------

Get gnunet to work on libp2p. This includes the FFI with rust (or binding to
implementations in other languages), converting address formats, signaling
metadata (traffic class and priority, as far as libp2p supports it) and
signalling connectivity changes.  This is a first attempt to technically link
the two projects. Therefore the feasibility is quite uncertain and the
milestones might have to be re-evaluated after the report on the needs and
feasibility. Should it turn out that the needed resources are beyond the
capabilities of this project, a detailed report on the requirements and roadmap
of and for the realisation shall be written and published.

For a lot of practical purposes it would be a lot easier to add a new
communicator for libp2p instead of integrating it as an underlay below core,
next to transport. The downside would be that there might be a lot of
duplication of functionality.

..
  TODO this generally needs more love

..
  - addresses from underlay in string representation go into core
  - libp2p can't handle hellos/peerstore
  - address signaling between transport and core
  - hellos should be handled by core


.. _Protocol_002dVersioning:

Protocol-Versioning
-------------------

Currently applications signal to core which message types they support. For
this milestone we will implement proper protocol versioning where higher-level
applications can signal a range of protocol versions which they support (min,
max) and exchange messages at the CONG layer between peers to determine common
protocols.



.. _Open_Design_Questions:

Open Design Questions
---------------------

In this section we list design question that are not decided on, yet.

..
 - scope of peer ids and other such elements (gns/identity, ...)
 - resolve gns to pid on lower layer? (cadet or even core)
    - convenience api: cadet connection to domain name
    - -> not the scope of cong


.. _Peer-ID-changes-and-connectivity:

Peer ID changes and connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

..
 - smooth handling of address/pid change possible?/signaling change of address
    - should be possible at least when we gain one address
    - construct that allows an additional peer id that is pre-computed (not
      based on addresses) and announced ahead of address change?
    - other terminology: provide tracking capability selectively
    - use peer ids as long as there are open connections with them?

In case a peer's addresses change, it gets a new peer id and therefor needs to
reconnect. The challenge is to reconnect as fast as possible. The main problem
is that a peer cannot know its next peer id in all cases. Connections that have
dedicated peers at its endpoints will probably look up the new peer id of the
other peer in a higher-layer service, most probably gns.

In the case in which a peer just gains an additional address, that peer can
pre-calculate its next peer id, signal it via still open connections on the old
peer id and finally switch to using the new peer id.

Other more evolved ideas include using multiple peer ids per peer: Either an
additional address-independent peer id that will 'survive' address changes and
serve as means to link to the address-based peer id after a change. It would
just be sent to connected peers and reset once all connections have been
re-established.

Alternatively (maybe in addition) peers could use multiple address-based peer
ids - one per address. Thus some peer ids might stay unchanged while others go
offline.

..
    - tracking even harder
    - comes closer to equivalent of ip addresses
    - possibly helps with (privacy-preserving) roll-overs
    - we ceased having unique peer ids over time, why not cease to have a
      single address at a time?
    - might serve as an 'onion-address light'?
    - cleaner, less messy abstraction


Another idea to address this challenge is to keep peer ids in use on
connections which are still in use, but don't publish those ids anymore.

Terminology-wise we might add another perspective and say that we selectively
and deliberately provide tracking capability to peers which we want to stay in
touch with.


.. _Peer-ID-Terminology:

Peer ID Terminology
^^^^^^^^^^^^^^^^^^^

Once peer ids cease to be unique over time the question raises whether they
should actually be called identities. (In my intuition an identity is something
more persistent.)
As we discuss peer ids as an analogue to ip addresses, a natural close idea
would be to use "peer address".


.. _Requirements:


Requirements
------------

In the discussions we seem to have lost partial oversight over things.
In this section we figure out the requirements for core (and possibly other
components) it's mechanisms.

TODO: what requirements exactly did we want to document here?


.. _Use-Cases-and-Scenarios:

Use Cases and Scenarios
-----------------------

This section is supposed to help with the understanding of the use of elements
and structures by imagined examples.


.. _Peer-ID:

Peer ID
^^^^^^^

The peer id has been used throughout gnunet as a very convenient means for many
purposes. It was used as globally unique and stable identifier of a peer. From
now on it should rather be treated as something more volatile and fitting for
its layer: an ip address. Below we collect the intended purposes and use cases
and also point out some uses for which it was not intended and point to other
means to achieve it.

..
   - address change: shutdown vs. standby vs. ...


.. _GNS:

GNS
^^^

TODO

..
  - identify on user level
  - vpn record: globally unique id


.. _CADET:

CADET
^^^^^

TODO


.. _Messenger-scenarios:

Messenger
^^^^^^^^^

TODO

..
  TODO 'tracking capability' via gns: shared secret
   - this a good place?
   - understand how that would work
   - hide multiple devices behind one identity


..
  TODO Overall: Look at the project milestones to check coverage
