************
About GNUnet
************

GNUnet in its current version is the result of over 20 years of work
from many contributors. So far, most contributions were made by
volunteers or people paid to do fundamental research. At this stage,
GNUnet remains an experimental system where significant parts of the
software lack a reasonable degree of professionalism in its
implementation. Furthermore, we are aware of a significant number of
existing bugs and critical design flaws, as some unfortunate early
design decisions remain to be rectified. There are still known open
problems; GNUnet remains an active research project.

The project was started in 2001 when some initial ideas for improving
Freenet’s file-sharing turned out to be too radical to be easily
realized within the scope of the existing Freenet project. We lost our
first contributor on 11.9.2001 as the contributor realized that privacy
may help terrorists. The rest of the team concluded that it was now even
more important to fight for civil liberties. The first release was
called “GNet” – already with the name GNUnet in mind, but without the
blessing of GNU we did not dare to call it GNUnet immediately. A few
months after the first release we contacted the GNU project, happily
agreed to their governance model and became an official GNU package.

Within the first year, we created GNU libextractor, a helper library for
meta data extraction which has been used by a few other projects as
well. 2003 saw the emergence of pluggable transports, the ability for
GNUnet to use different mechanisms for communication, starting with TCP,
UDP and SMTP (support for the latter was later dropped due to a lack of
maintenance). In 2005, the project first started to evolve beyond the
original file-sharing application with a first simple P2P chat. In 2007,
we created GNU libmicrohttpd to support a pluggable transport based on
HTTP. In 2009, the architecture was radically modularized into the
multi-process system that exists today. Coincidentally, the first
version of the ARM service (ARM: Automatic Restart Manager) was
implemented a day before systemd was announced. From 2009 to 2014 work
progressed rapidly thanks to a significant research grant from the
Deutsche Forschungsgesellschaft. This resulted in particular in the
creation of the R5N DHT, CADET, ATS and the GNU Name System. In 2010,
GNUnet was selected as the basis for the secushare online social
network, resulting in a significant growth of the core team. In 2013, we
launched GNU Taler to address the challenge of convenient and
privacy-preserving online payments. In 2015, the pretty Easy privacy
(pEp) project announced that they will use GNUnet as the technology for
their meta-data protection layer, ultimately resulting in GNUnet e.V.
entering into a formal long-term collaboration with the pEp Foundation.
In 2016, Taler Systems SA, a first startup using GNUnet technology, was
founded with support from the community.

GNUnet is not merely a technical project, but also a political mission:
like the GNU project as a whole, we are writing software to achieve
political goals with a focus on the human right of informational
self-determination. Putting users in control of their computing has been
the core driver of the GNU project. With GNUnet we are focusing on
informational self-determination for collaborative computing and
communication over networks.

The Internet is shaped as much by code and protocols as it is by its
associated political processes (IETF, ICANN, IEEE, etc.). Similarly its
flaws are not limited to the protocol design. Thus, technical excellence
by itself will not suffice to create a better network. We also need to
build a community that is wise, humble and has a sense of humor to
achieve our goal to create a technical foundation for a society we would
like to live in.

Project governance
==================

GNUnet, like the GNU project and many other free software projects,
follows the governance model of a benevolent dictator. This means that
ultimately, the GNU project appoints the GNU maintainer and can overrule
decisions made by the GNUnet maintainer. Similarly, the GNUnet
maintainer can overrule any decisions made by individual developers.
Still, in practice neither has happened in the last 20 years for GNUnet,
and we hope to keep it that way.

The current maintainers of GNUnet are:

-  `Christian Grothoff <https://grothoff.org/christian/>`__
-  `Martin Schanzenbach <https://schanzen.eu/>`__

The GNUnet project is supported by GNUnet e.V., a German association
where any developer can become a member. GNUnet e.V. serves as a legal
entity to hold the copyrights to GNUnet. GNUnet e.V. may also choose to
pay for project resources, and can collect donations as well as choose
to adjust the license of the software (with the constraint that it has
to remain free software). In 2018 we switched from GPL3 to AGPL3, in
practice these changes do not happen very often.

Philosophy
==========

The primary goal of the GNUnet project is to provide a reliable, open,
non-discriminating and censorship-resistant system for information
exchange. We value free speech above state interests and intellectual
monopoly. GNUnet’s long-term goal is to serve as a development platform
for the next generation of Internet protocols.

Participants are encouraged to contribute at least as much resources
(storage, bandwidth) to the network as they consume, so that their
participation does not have a negative impact on other users.

Design Principles
-----------------

These are the GNUnet design principles, in order of importance:

-  GNUnet must be implemented as Free Software — This means that you
   have the four essential freedoms: to run the program, to study and
   change the program in source code form, to redistribute exact copies,
   and to distribute modified versions.
   (https://www.gnu.org/philosophy/free-sw.html).
-  GNUnet must minimize the amount of personally identifiable
   information exposed.
-  GNUnet must be fully distributed and resilient to external attacks
   and rogue participants.
-  GNUnet must be self-organizing and not depend on administrators or
   centralized infrastructure.
-  GNUnet must inform the user which other participants have to be
   trusted when establishing private communications.
-  GNUnet must be open and permit new peers to join.
-  GNUnet must support a diverse range of applications and devices.
-  GNUnet must use compartmentalization to protect sensitive
   information.
-  The GNUnet architecture must be resource efficient.
-  GNUnet must provide incentives for peers to contribute more resources
   than they consume.

Privacy and Anonymity
---------------------

The GNUnet protocols minimize the leakage of personally identifiable
information of participants and do not allow adversaries to control,
track, monitor or censor users activities. The GNUnet protocols also
make it as hard as possible to disrupt operations by participating in
the network with malicious intent.

Analyzing participant’s activities becomes more difficult as the number
of peers and applications that generate traffic on the network grows,
even if the additional traffic generated is not related to anonymous
communication. This is one of the reasons why GNUnet is developed as a
peer-to-peer framework where many applications share the lower layers of
an increasingly complex protocol stack. The GNUnet architecture
encourages many different forms of peer-to-peer applications.

Practicality
------------

Wherever possible GNUnet allows the peer to adjust its operations and
functionalities to specific use cases. A GNUnet peer running on a mobile
device with limited battery for example might choose not to relay
traffic for other participants.

For certain applications like file-sharing GNUnet allows participants to
trade degrees of anonymity in exchange for increased efficiency.
However, it is not possible for any user’s efficiency requirements to
compromise the anonymity of any other user.

Key Concepts
============

GNUnet is an alternative network stack for building secure, decentralized and privacy-preserving distributed applications. Our goal is to replace the old insecure Internet protocol stack. Starting from an application for secure publication of files, it has grown to include all kinds of basic protocol components and applications towards the creation of a GNU internet.

Today, the actual use and thus the social requirements for a global network differs widely from those goals of 1970. While the Internet remains suitable for military use, where the network equipment is operated by a command hierarchy and when necessary isolated from the rest of the world, the situation is less tenable for civil society.

Due to fundamental Internet design choices, Internet traffic can be misdirected, intercepted, censored and manipulated by hostile routers on the network. And indeed, the modern Internet has evolved exactly to the point where, as Matthew Green put it, "the network is hostile".

We believe liberal societies need a network architecture that uses the anti-authoritarian decentralized peer-to-peer paradigm and privacy-preserving cryptographic protocols. The goal of the GNUnet project is to provide a Free Software realization of this ideal.

Specifically, GNUnet tries to follow the following design principles, in order of importance:

  1. GNUnet must be implemented as Free Software.
  2. GNUnet must minimize the amount of personally identifiable information exposed.
  3. GNUnet must be fully distributed and resilient to external attacks and rogue participants.
  4. GNUnet must be self-organizing and not depend on administrators or centralized infrastructure.
  5. GNUnet must inform the user which other participants have to be trusted when establishing private communications.
  6. GNUnet must be open and permit new peers to join.
  7. GNUnet must support a diverse range of applications and devices.
  8. GNUnet must use compartmentalization to protect sensitive information.
  9. The GNUnet architecture must be resource efficient.
  10. GNUnet must provide incentives for peers to contribute more resources than they consume.

Architecture
------------

GNUnet consists of a set of services.
In order to realize a peer-to-peer network stack, a subset of GNUnet subsystems
emulate what can be found in the ISO/OSI layer of the Internet.
(TODO insert image here)

Peer Identities
~~~~~~~~~~~~~~~

In GNUnet, the identity of a host is its public key called **Peer Identity**.
For that reason, man-in-the-middle attacks will not break the authentication or
accounting goals. Essentially, for GNUnet, the IP of the host has
nothing to do with the identity of the host. As the public key is the
only thing that truly matters, faking an IP, a port or any other
property of the underlying transport protocol is irrelevant. In fact,
GNUnet peers can use multiple IPs (IPv4 and IPv6) on multiple ports — or
even not use the IP protocol at all (by running directly on layer 2).

Peer identities are used to identify peers in the network and are unique
for each peer. The identity for a peer is simply its public key, which
is generated along with a private key when the peer is started for the
first time. While the identity is binary data, it is often expressed as
an ASCII string. For example, the following is a peer identity as you
might see it in various places:

::

   UAT1S6PMPITLBKSJ2DGV341JI6KF7B66AC4JVCN9811NNEGQLUN0

You can find your peer identity by running ``gnunet-core``.

Almost all peer-to-peer communications in GNUnet are between mutually
authenticated peers.
GNUnet uses a special type of message to communicate a binding between
public (ECC) keys to their current network address. These messages are
commonly called **HELLOs** or peer advertisements. They contain the public
key of the peer and its current network addresses for various transport
services. A transport service is a special kind of shared library that
provides (possibly unreliable, out-of-order) message delivery between
peers. For the UDP and TCP transport services, a network address is an
IP and a port. GNUnet can also use other transports (HTTP, HTTPS, WLAN,
etc.) which use various other forms of addresses. Note that any node can
have many different active transport services at the same time, and each
of these can have a different addresses. Binding messages expire after
at most a week (the timeout can be shorter if the user configures the
node appropriately). This expiration ensures that the network will
eventually get rid of outdated advertisements.

For more information, refer to the following paper:

Ronaldo A. Ferreira, Christian Grothoff, and Paul Ruth. A Transport
Layer Abstraction for Peer-to-Peer Networks Proceedings of the 3rd
International Symposium on Cluster Computing and the Grid (GRID 2003),
2003. (https://git.gnunet.org/bibliography.git/plain/docs/transport.pdf)


Security goals and threat model
-------------------------

GNUnet is designed as to subsist in the face of a strong adversaries (malicous, bad actors).
This includes, in decending strength, malicous

  1. nation states,
  2. network operators,
  3. peer operators,
  4. and GNUnet users.

External, network-level adversaries may attempt to identify GNUnet traffic
and throttle or otherwise impair its use.
To prevent easy identification, GNUnet communication is cryptographically and
steganographically obfuscated.
For example, GNUnet traffic can be made to look like QUIC or HTTP/3 traffic or
even look like random noise on the network.
Further, through the use of multiple communication protocols at the same time
(e.g. QUIC and Ad-hoc WiFi), loss of a single communication method does not cause complete communication breakdown.
Adversaries outside of GNUnet are not supposed
to know what kind of actions a peer is involved in. Only the specific
neighbor of a peer that is the corresponding sender or recipient of a
message may know its contents, and then application protocols may
place further restrictions on that knowledge. In order to ensure
confidentiality, GNUnet uses link encryption, that is each message
exchanged between two peers is encrypted using a pair of keys only known
to these two peers. Encrypting traffic like this makes any kind of
traffic analysis much harder. Naturally, for some applications, it may
still be desirable if even neighbors cannot determine the concrete
contents of a message. In GNUnet, this problem is addressed by the
specific application-level protocols. See for example the following
sections: `Anonymity <about.md#anonymity>`__, see `How file-sharing
achieves Anonymity <about.md#how-file-sharing-achieves-anonymity>`__,
and see `Deniability <about.md#deniability>`__.

GNUnet attemtps to satisfy the following security goals in the face of those adversaries:

1. Censorship resistance
2. Confidential communication
3. Anonymity (where possible)


From the lowest layer to the applications layer, the securty goals and associated subsystems are:

1. Base layer (Communicators/TRANSPORT): This layer optionally provides steganographic and ad-hoc security guarantees against external adversaries that largely depend on the communicator(s) used. For example, use of the HTTP3/QUIC communicator will use TLS and try to validate a certificate signed by the peer we want to connect to. Other communicators may not provide the same properties.
  - QUIC Communicator: Appears to be a regular TLS connection (EdDSA/X25519).
  - TCP/UDP Communicator: Uses Diffie-Hellman with Elligator (`LSD 0011 <https://lsd.gnunet.org/lsd0011/>`_) to look like random noise.
2. Peer connectivity and routing layer (CORE, R5N): This layer provides a secure channel between two (physically) connected peers. Peers are mutually authenticated and a secure cryptographic channel is established, but there is no particular trust required between the communication partners. It does not assume any security guarantees from the previous layer. It provides confidential communication in the face of an external adversary. The R5N uses this layer to establish an overlay network (DHT).
  - CORE: DTLS-style KEMTLS called CAKE with EdDSA and X25519. Specification: `LSD 0012 <https://lsd.gnunet.org/lsd0012/>`_
  - R5N: EdDSA signatures for route recording: `LSD 0004 <https://lsd.gnunet.org/lsd0004/>`_
3. Peer connectivity layer (CADET): This layer provides an end-to-end secure secure cryptographic channel between two peers. It is assumed that this channel is established between to peers that share a strong trust relationship.
  - CADET: Axolotl 3DH with EdDSA Peer Identities to provide perfect forward secrecy, post-compromise security, secure out-of-order delivery and participant repudication (deniability).
4. Application layer: Each :ref:`subsystem <subsystems>` of GNUnet incorporates its own security mechanism taking the existing baseline of the GNUnet network as well as the adversary model into account. See the respective section in the User handbook.
  - GNS: Resource records are signed using EDDSA (or EdDSA) for data origin authentication and encrypted using AES-CTR (or EdDSA+XSalsa20-Poly1305) to achieve data confidentiality against certain adversaries. Public keys are blinded to prevent censorship. Specification: `LSD 0001 <https://lsd.gnunet.org/lsd0012/>`_


Cryptography
------------

Cryptographic Inventory
~~~~~~~~~~~~~~~~~~~~~~~

GNUnet makes heavy use of standard, well-tested cryptographic primitives to
implement its protocols. The primary primtives are:

- Digital signatures: For Peer Identities and general (data origin) authentication. Scheme: EdDSA.
- Key exchange and KEMs: For handshakes. Schemes: X25519 (with Ed25519-to-Curve25519 transformations where necessary, such as CORE).
- Blindable signature keys: For the GNU Name System. Schemes: EDDSA and EdDSA.
- Blind signatures: For blind signing. Primarily used by GNU Taler. Scheme: RSA-FDH.
- Symmetric encryption: For secure communication. Schemes: XSalsa20-Poly1305, AES (to be phased out in favor of AEGIS where possible).
- Public-key encryption: To send encrypted messages. Schemes: HPKE (RFC 9180), only DHKEM(X25519, HKDF-SHA256), HKDF-SHA256, ChaCha20-Poly1305.
- Hash functions and KDF: GNUnet primarily uses SHA(-512) and HKDF.

Currently, no clear path to post-quantum primitives has been laid out.
This is mostly due to open research questions in the areas of key blinding and blind signatures.

Egos
~~~~

**Egos** are your “identities” in GNUnet. Any user can assume multiple
identities, for example to separate their activities online. Egos can
correspond to “pseudonyms” or “real-world identities”. Technically an
ego is first of all a key pair of a public- and private-key.
The current primary use for Egos are in the GNU Name System as zone keys.

Zones in the GNU Name System
""""""""""""""""""""""""""""

Egos are used as **GNS zones**.

GNS zones are similar to those of DNS zones, but instead of a hierarchy
of authorities to governing their use, GNS zones are controlled by a
private key. When you create a record in a DNS zone, that information is
stored in your nameserver. Anyone trying to resolve your domain then
gets pointed (hopefully) by the centralised authority to your
nameserver. Whereas GNS, being fully decentralized by design, stores
that information in DHT. The validity of the records is assured
cryptographically, by signing them with the private key of the
respective zone.

Anyone trying to resolve records in a zone of your domain can then
verify the signature of the records they get from the DHT and be assured
that they are indeed from the respective zone. To make this work, there
is a 1:1 correspondence between zones and their public-private key
pairs. So when we talk about the owner of a GNS zone, that’s really the
owner of the private key. And a user accessing a zone needs to somehow
specify the corresponding public key first.

For more information, refer to RFC 9498.



Anonymity
---------

Providing anonymity for users is the central goal for the anonymous
file-sharing application. Many other design decisions follow in the
footsteps of this requirement. Anonymity is never absolute. While there
are various scientific metrics (Claudia Díaz, Stefaan Seys, Joris
Claessens, and Bart Preneel. Towards measuring anonymity. 2002.
(https://git.gnunet.org/bibliography.git/plain/docs/article-89.pdf))
that can help quantify the level of anonymity that a given mechanism
provides, there is no such thing as “complete anonymity”.

GNUnet’s file-sharing implementation allows users to select for each
operation (publish, search, download) the desired level of anonymity.
The metric used is based on the amount of cover traffic needed to hide
the request.

While there is no clear way to relate the amount of available cover
traffic to traditional scientific metrics such as the anonymity set or
information leakage, it is probably the best metric available to a peer
with a purely local view of the world, in that it does not rely on
unreliable external information or a particular adversary model.

The default anonymity level is 1, which uses anonymous routing but
imposes no minimal requirements on cover traffic. It is possible to
forego anonymity when this is not required. The anonymity level of 0
allows GNUnet to use more efficient, non-anonymous routing.

How file-sharing achieves Anonymity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Contrary to other designs, we do not believe that users achieve strong
anonymity just because their requests are obfuscated by a couple of
indirections. This is not sufficient if the adversary uses traffic
analysis. The threat model used for anonymous file sharing in GNUnet
assumes that the adversary is quite powerful. In particular, we assume
that the adversary can see all the traffic on the Internet. And while we
assume that the adversary can not break our encryption, we assume that
the adversary has many participating nodes in the network and that it
can thus see many of the node-to-node interactions since it controls
some of the nodes.

The system tries to achieve anonymity based on the idea that users can
be anonymous if they can hide their actions in the traffic created by
other users. Hiding actions in the traffic of other users requires
participating in the traffic, bringing back the traditional technique of
using indirection and source rewriting. Source rewriting is required to
gain anonymity since otherwise an adversary could tell if a message
originated from a host by looking at the source address. If all packets
look like they originate from one node, the adversary can not tell which
ones originate from that node and which ones were routed. Note that in
this mindset, any node can decide to break the source-rewriting paradigm
without violating the protocol, as this only reduces the amount of
traffic that a node can hide its own traffic in.

If we want to hide our actions in the traffic of other nodes, we must
make our traffic indistinguishable from the traffic that we route for
others. As our queries must have us as the receiver of the reply
(otherwise they would be useless), we must put ourselves as the receiver
of replies that actually go to other hosts; in other words, we must
indirect replies. Unlike other systems, in anonymous file-sharing as
implemented on top of GNUnet we do not have to indirect the replies if
we don’t think we need more traffic to hide our own actions.

This increases the efficiency of the network as we can indirect less
under higher load. Refer to the following paper for more: Krista Bennett
and Christian Grothoff. GAP — practical anonymous networking. In
Proceedings of Designing Privacy Enhancing Technologies, 2003.
(https://git.gnunet.org/bibliography.git/plain/docs/aff.pdf)

How messaging provided Anonymity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While the file-sharing tries to achieve anonymity through hiding actions
in other traffic, the messaging service provides a weaker form of
protection against identification.

The messaging service allows the use of an anonymous ego for the signing
and verification process of messages instead of a unique ego. This
anonymous ego is a publicly known key pair which is shared between all
peers in GNUnet.

Using this ego only ensures that individual messages alone can’t
identify its sender inside of a messenger room. It should be clarified
that the route of the traffic for each message can still be tracked to
identify the senders peer inside of a messenger room if the threat agent
controls certain peers hosting the room.

Also opening a room in the messenger service will potentially match your
peer identity with the internal member identity from the messenger
service. So despite using the anonymous ego you can reveal your peer
identity. This means to decrease the chance of being identified, it is
recommended to enter rooms but you should not open them for others.

Deniability
-----------

Even if the user that downloads data and the server that provides data
are anonymous, the intermediaries may still be targets. In particular,
if the intermediaries can find out which queries or which content they
are processing, a strong adversary could try to force them to censor
certain materials.

With the file-encoding used by GNUnet’s anonymous file-sharing, this
problem does not arise. The reason is that queries and replies are
transmitted in an encrypted format such that intermediaries cannot tell
what the query is for or what the content is about. Mind that this is
not the same encryption as the link-encryption between the nodes. GNUnet
has encryption on the network layer (link encryption, confidentiality,
authentication) and again on the application layer (provided by
gnunet-publish, gnunet-download, gnunet-search and gnunet-fs-gtk).

Refer to the following paper for more: Christian Grothoff, Krista
Grothoff, Tzvetan Horozov, and Jussi T. Lindgren. An Encoding for
Censorship-Resistant Sharing. 2009.
(https://git.gnunet.org/bibliography.git/plain/docs/ecrs.pdf)

Accounting to Encourage Resource Sharing
----------------------------------------

Most distributed P2P networks suffer from a lack of defenses or
precautions against attacks in the form of freeloading. While the
intentions of an attacker and a freeloader are different, their effect
on the network is the same; they both render it useless. Most simple
attacks on networks such as Gnutella involve flooding the network with
traffic, particularly with queries that are, in the worst case,
multiplied by the network.

In order to ensure that freeloaders or attackers have a minimal impact
on the network, GNUnet’s file-sharing implementation (FS) tries to
distinguish good (contributing) nodes from malicious (freeloading)
nodes. In GNUnet, every file-sharing node keeps track of the behavior of
every other node it has been in contact with. Many requests (depending
on the application) are transmitted with a priority (or importance)
level. That priority is used to establish how important the sender
believes this request is. If a peer responds to an important request,
the recipient will increase its trust in the responder: the responder
contributed resources. If a peer is too busy to answer all requests, it
needs to prioritize. For that, peers do not take the priorities of the
requests received at face value. First, they check how much they trust
the sender, and depending on that amount of trust they assign the
request a (possibly lower) effective priority. Then, they drop the
requests with the lowest effective priority to satisfy their resource
constraints. This way, GNUnet’s economic model ensures that nodes that
are not currently considered to have a surplus in contributions will not
be served if the network load is high.

For more information, refer to the following paper: Christian Grothoff.
An Excess-Based Economic Model for Resource Allocation in Peer-to-Peer
Networks. Wirtschaftsinformatik, June 2003.
(https://git.gnunet.org/bibliography.git/plain/docs/ebe.pdf)

