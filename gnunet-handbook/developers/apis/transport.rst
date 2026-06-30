.. index::
   double: TRANSPORT; subsystem

.. _TRANSPORT-Subsystem-Dev:

TRANSPORT
=========

.. _Address-validation-protocol:

Address validation protocol
---------------------------

This section documents how the GNUnet transport service validates
connections with other peers. It is a high-level description of the
protocol necessary to understand the details of the implementation. It
should be noted that when we talk about PING and PONG messages in this
section, we refer to transport-level PING and PONG messages, which are
different from core-level PING and PONG messages (both in implementation
and function).

The goal of transport-level address validation is to minimize the
chances of a successful man-in-the-middle attack against GNUnet peers on
the transport level. Such an attack would not allow the adversary to
decrypt the P2P transmissions, but a successful attacker could at least
measure traffic volumes and latencies (raising the adversaries
capabilities by those of a global passive adversary in the worst case).
The scenarios we are concerned about is an attacker, Mallory, giving a
``HELLO`` to Alice that claims to be for Bob, but contains Mallory's IP
address instead of Bobs (for some transport). Mallory would then forward
the traffic to Bob (by initiating a connection to Bob and claiming to be
Alice). As a further complication, the scheme has to work even if say
Alice is behind a NAT without traversal support and hence has no address
of her own (and thus Alice must always initiate the connection to Bob).

An additional constraint is that ``HELLO`` messages do not contain a
cryptographic signature since other peers must be able to edit (i.e.
remove) addresses from the ``HELLO`` at any time (this was not true in
GNUnet 0.8.x). A basic **assumption** is that each peer knows the set of
possible network addresses that it **might** be reachable under (so for
example, the external IP address of the NAT plus the LAN address(es)
with the respective ports).

The solution is the following. If Alice wants to validate that a given
address for Bob is valid (i.e. is actually established **directly** with
the intended target), she sends a PING message over that connection to
Bob. Note that in this case, Alice initiated the connection so only
Alice knows which address was used for sure (Alice may be behind NAT, so
whatever address Bob sees may not be an address Alice knows she has).
Bob checks that the address given in the ``PING`` is actually one of
Bob's addresses (ie: does not belong to Mallory), and if it is, sends
back a ``PONG`` (with a signature that says that Bob owns/uses the
address from the ``PING``). Alice checks the signature and is happy if
it is valid and the address in the ``PONG`` is the address Alice used.
This is similar to the 0.8.x protocol where the ``HELLO`` contained a
signature from Bob for each address used by Bob. Here, the purpose code
for the signature is ``GNUNET_SIGNATURE_PURPOSE_TRANSPORT_PONG_OWN``.
After this, Alice will remember Bob's address and consider the address
valid for a while (12h in the current implementation). Note that after
this exchange, Alice only considers Bob's address to be valid, the
connection itself is not considered 'established'. In particular, Alice
may have many addresses for Bob that Alice considers valid.

The ``PONG`` message is protected with a nonce/challenge against replay
attacks (`replay <http://en.wikipedia.org/wiki/Replay_attack>`__) and
uses an expiration time for the signature (but those are almost
implementation details).

NAT library
.. _NAT-library:

NAT library
-----------

The goal of the GNUnet NAT library is to provide a general-purpose API
for NAT traversal **without** third-party support. So protocols that
involve contacting a third peer to help establish a connection between
two peers are outside of the scope of this API. That does not mean that
GNUnet doesn't support involving a third peer (we can do this with the
distance-vector transport or using application-level protocols), it just
means that the NAT API is not concerned with this possibility. The API
is written so that it will work for IPv6-NAT in the future as well as
current IPv4-NAT. Furthermore, the NAT API is always used, even for
peers that are not behind NAT --- in that case, the mapping provided is
simply the identity.

NAT traversal is initiated by calling ``GNUNET_NAT_register``. Given a
set of addresses that the peer has locally bound to (TCP or UDP), the
NAT library will return (via callback) a (possibly longer) list of
addresses the peer **might** be reachable under. Internally, depending
on the configuration, the NAT library will try to punch a hole (using
UPnP) or just \"know\" that the NAT was manually punched and generate
the respective external IP address (the one that should be globally
visible) based on the given information.

The NAT library also supports ICMP-based NAT traversal. Here, the other
peer can request connection-reversal by this peer (in this special case,
the peer is even allowed to configure a port number of zero). If the NAT
library detects a connection-reversal request, it returns the respective
target address to the client as well. It should be noted that
connection-reversal is currently only intended for TCP, so other plugins
**must** pass ``NULL`` for the reversal callback. Naturally, the NAT
library also supports requesting connection reversal from a remote peer
(``GNUNET_NAT_run_client``).

Once initialized, the NAT handle can be used to test if a given address
is possibly a valid address for this peer (``GNUNET_NAT_test_address``).
This is used for validating our addresses when generating PONGs.

Finally, the NAT library contains an API to test if our NAT
configuration is correct. Using ``GNUNET_NAT_test_start`` **before**
binding to the respective port, the NAT library can be used to test if
the configuration works. The test function act as a local client,
initialize the NAT traversal and then contact a ``gnunet-nat-server``
(running by default on ``gnunet.org``) and ask for a connection to be
established. This way, it is easy to test if the current NAT
configuration is valid.

.. _Distance_002dVector-plugin:

Distance-Vector plugin
----------------------

The Distance Vector (DV) transport is a transport mechanism that allows
peers to act as relays for each other, thereby connecting peers that
would otherwise be unable to connect. This gives a larger connection set
to applications that may work better with more peers to choose from (for
example, File Sharing and/or DHT).

The Distance Vector transport essentially has two functions. The first
is \"gossiping\" connection information about more distant peers to
directly connected peers. The second is taking messages intended for
non-directly connected peers and encapsulating them in a DV wrapper that
contains the required information for routing the message through
forwarding peers. Via gossiping, optimal routes through the known DV
neighborhood are discovered and utilized and the message encapsulation
provides some benefits in addition to simply getting the message from
the correct source to the proper destination.

The gossiping function of DV provides an up to date routing table of
peers that are available up to some number of hops. We call this a
fisheye view of the network (like a fish, nearby objects are known while
more distant ones unknown). Gossip messages are sent only to directly
connected peers, but they are sent about other knowns peers within the
\"fisheye distance\". Whenever two peers connect, they immediately
gossip to each other about their appropriate other neighbors. They also
gossip about the newly connected peer to previously connected neighbors.
In order to keep the routing tables up to date, disconnect notifications
are propagated as gossip as well (because disconnects may not be
sent/received, timeouts are also used remove stagnant routing table
entries).

Routing of messages via DV is straightforward. When the DV transport is
notified of a message destined for a non-direct neighbor, the
appropriate forwarding peer is selected, and the base message is
encapsulated in a DV message which contains information about the
initial peer and the intended recipient. At each forwarding hop, the
initial peer is validated (the forwarding peer ensures that it has the
initial peer in its neighborhood, otherwise the message is dropped).
Next the base message is re-encapsulated in a new DV message for the
next hop in the forwarding chain (or delivered to the current peer, if
it has arrived at the destination).

Assume a three peer network with peers Alice, Bob and Carol. Assume that

::

   Alice <-> Bob and Bob <-> Carol

are direct (e.g. over TCP or UDP transports) connections, but that Alice
cannot directly connect to Carol. This may be the case due to NAT or
firewall restrictions, or perhaps based on one of the peers respective
configurations. If the Distance Vector transport is enabled on all three
peers, it will automatically discover (from the gossip protocol) that
Alice and Carol can connect via Bob and provide a \"virtual\" Alice <->
Carol connection. Routing between Alice and Carol happens as follows;
Alice creates a message destined for Carol and notifies the DV transport
about it. The DV transport at Alice looks up Carol in the routing table
and finds that the message must be sent through Bob for Carol. The
message is encapsulated setting Alice as the initiator and Carol as the
destination and sent to Bob. Bob receives the messages, verifies that
both Alice and Carol are known to Bob, and re-wraps the message in a new
DV message for Carol. The DV transport at Carol receives this message,
unwraps the original message, and delivers it to Carol as though it came
directly from Alice.

SMTP plugin
.. _SMTP-plugin:

SMTP plugin
-----------

.. todo:: Update?

This section describes the new SMTP transport plugin for GNUnet as it
exists in the 0.7.x and 0.8.x branch. SMTP support is currently not
available in GNUnet 0.9.x. This page also describes the transport layer
abstraction (as it existed in 0.7.x and 0.8.x) in more detail and gives
some benchmarking results. The performance results presented are quite
old and maybe outdated at this point. For the readers in the year 2019,
you will notice by the mention of version 0.7, 0.8, and 0.9 that this
section has to be taken with your usual grain of salt and be updated
eventually.

-  Why use SMTP for a peer-to-peer transport?

-  SMTPHow does it work?

-  How do I configure my peer?

-  How do I test if it works?

-  How fast is it?

-  Is there any additional documentation?

.. _Why-use-SMTP-for-a-peer_002dto_002dpeer-transport_003f:

Why use SMTP for a peer-to-peer transport?
------------------------------------------

There are many reasons why one would not want to use SMTP:

-  SMTP is using more bandwidth than TCP, UDP or HTTP

-  SMTP has a much higher latency.

-  SMTP requires significantly more computation (encoding and decoding
   time) for the peers.

-  SMTP is significantly more complicated to configure.

-  SMTP may be abused by tricking GNUnet into sending mail
   to non-participating third parties.

So why would anybody want to use SMTP?

-  SMTP can be used to contact peers behind NAT boxes (in virtual
   private networks).

-  SMTP can be used to circumvent policies that limit or prohibit
   peer-to-peer traffic by masking as \"legitimate\" traffic.

-  SMTP uses E-mail addresses which are independent of a specific IP,
   which can be useful to address peers that use dynamic IP addresses.

-  SMTP can be used to initiate a connection (e.g. initial address
   exchange) and peers can then negotiate the use of a more efficient
   protocol (e.g. TCP) for the actual communication.

In summary, SMTP can for example be used to send a message to a peer
behind a NAT box that has a dynamic IP to tell the peer to establish a
TCP connection to a peer outside of the private network. Even an
extraordinary overhead for this first message would be irrelevant in
this type of situation.

.. _How-does-it-work_003f:

How does it work?
-----------------

When a GNUnet peer needs to send a message to another GNUnet peer that
has advertised (only) an SMTP transport address, GNUnet base64-encodes
the message and sends it in an E-mail to the advertised address. The
advertisement contains a filter which is placed in the E-mail header,
such that the receiving host can filter the tagged E-mails and forward
it to the GNUnet peer process. The filter can be specified individually
by each peer and be changed over time. This makes it impossible to
censor GNUnet E-mail messages by searching for a generic filter.

.. _How-do-I-configure-my-peer_003f:

How do I configure my peer?
---------------------------

First, you need to configure ``procmail`` to filter your inbound E-mail
for GNUnet traffic. The GNUnet messages must be delivered into a pipe,
for example ``/tmp/gnunet.smtp``. You also need to define a filter that
is used by ``procmail`` to detect GNUnet messages. You are free to
choose whichever filter you like, but you should make sure that it does
not occur in your other E-mail. In our example, we will use
``X-mailer: GNUnet``. The ``~/.procmailrc`` configuration file then
looks like this:

::

   :0:
   * ^X-mailer: GNUnet
   /tmp/gnunet.smtp
   # where do you want your other e-mail delivered to
   # (default: /var/spool/mail/)
   :0: /var/spool/mail/

After adding this file, first make sure that your regular E-mail still
works (e.g. by sending an E-mail to yourself). Then edit the GNUnet
configuration. In the section ``SMTP`` you need to specify your E-mail
address under ``EMAIL``, your mail server (for outgoing mail) under
``SERVER``, the filter (X-mailer: GNUnet in the example) under
``FILTER`` and the name of the pipe under ``PIPE``. The completed
section could then look like this:

.. code-block:: text

   EMAIL = me@mail.gnu.org MTU = 65000 SERVER = mail.gnu.org:25 FILTER =
   "X-mailer: GNUnet" PIPE = /tmp/gnunet.smtp

.. todo:: set highlighting for this code block properly.

Finally, you need to add ``smtp`` to the list of ``TRANSPORTS`` in the
``GNUNETD`` section. GNUnet peers will use the E-mail address that you
specified to contact your peer until the advertisement times out. Thus,
if you are not sure if everything works properly or if you are not
planning to be online for a long time, you may want to configure this
timeout to be short, e.g. just one hour. For this, set ``HELLOEXPIRES``
to ``1`` in the ``GNUNETD`` section.

This should be it, but you may probably want to test it first.

.. _How-do-I-test-if-it-works_003f:

How do I test if it works?
--------------------------

Any transport can be subjected to some rudimentary tests using the
``gnunet-transport-check`` tool. The tool sends a message to the local
node via the transport and checks that a valid message is received.
While this test does not involve other peers and can not check if
firewalls or other network obstacles prohibit proper operation, this is
a great testcase for the SMTP transport since it tests pretty much
nearly all of the functionality.

``gnunet-transport-check`` should only be used without running
``gnunetd`` at the same time. By default, ``gnunet-transport-check``
tests all transports that are specified in the configuration file. But
you can specifically test SMTP by giving the option
``--transport=smtp``.

Note that this test always checks if a transport can receive and send.
While you can configure most transports to only receive or only send
messages, this test will only work if you have configured the transport
to send and receive messages.

.. _How-fast-is-it_003f:

How fast is it?
---------------

We have measured the performance of the UDP, TCP and SMTP transport
layer directly and when used from an application using the GNUnet core.
Measuring just the transport layer gives the better view of the actual
overhead of the protocol, whereas evaluating the transport from the
application puts the overhead into perspective from a practical point of
view.

The loopback measurements of the SMTP transport were performed on three
different machines spanning a range of modern SMTP configurations. We
used a PIII-800 running RedHat 7.3 with the Purdue Computer Science
configuration which includes filters for spam. We also used a Xenon 2
GHZ with a vanilla RedHat 8.0 sendmail configuration. Furthermore, we
used qmail on a PIII-1000 running Sorcerer GNU Linux (SGL). The numbers
for UDP and TCP are provided using the SGL configuration. The qmail
benchmark uses qmail's internal filtering whereas the sendmail
benchmarks relies on procmail to filter and deliver the mail. We used
the transport layer to send a message of b bytes (excluding transport
protocol headers) directly to the local machine. This way, network
latency and packet loss on the wire have no impact on the timings. n
messages were sent sequentially over the transport layer, sending
message i+1 after the i-th message was received. All messages were sent
over the same connection and the time to establish the connection was
not taken into account since this overhead is minuscule in practice ---
as long as a connection is used for a significant number of messages.

+--------------+----------+----------+----------+----------+----------+
| Transport    | UDP      | TCP      | SMTP     | SMTP (RH | SMTP     |
|              |          |          | (Purdue  | 8.0)     | (SGL     |
|              |          |          | s        |          | qmail)   |
|              |          |          | endmail) |          |          |
+==============+==========+==========+==========+==========+==========+
| 11 bytes     | 31 ms    | 55 ms    | 781 s    | 77 s     | 24 s     |
+--------------+----------+----------+----------+----------+----------+
| 407 bytes    | 37 ms    | 62 ms    | 789 s    | 78 s     | 25 s     |
+--------------+----------+----------+----------+----------+----------+
| 1,221 bytes  | 46 ms    | 73 ms    | 804 s    | 78 s     | 25 s     |
+--------------+----------+----------+----------+----------+----------+

The benchmarks show that UDP and TCP are, as expected, both
significantly faster compared with any of the SMTP services. Among the
SMTP implementations, there can be significant differences depending on
the SMTP configuration. Filtering with an external tool like procmail
that needs to re-parse its configuration for each mail can be very
expensive. Applying spam filters can also significantly impact the
performance of the underlying SMTP implementation. The microbenchmark
shows that SMTP can be a viable solution for initiating peer-to-peer
sessions: a couple of seconds to connect to a peer are probably not even
going to be noticed by users. The next benchmark measures the possible
throughput for a transport. Throughput can be measured by sending
multiple messages in parallel and measuring packet loss. Note that not
only UDP but also the TCP transport can actually loose messages since
the TCP implementation drops messages if the ``write`` to the socket
would block. While the SMTP protocol never drops messages itself, it is
often so slow that only a fraction of the messages can be sent and
received in the given time-bounds. For this benchmark we report the
message loss after allowing t time for sending m messages. If messages
were not sent (or received) after an overall timeout of t, they were
considered lost. The benchmark was performed using two Xeon 2 GHZ
machines running RedHat 8.0 with sendmail. The machines were connected
with a direct 100 MBit Ethernet connection. Figures udp1200, tcp1200 and
smtp-MTUs show that the throughput for messages of size 1,200 octets is
2,343 kbps, 3,310 kbps and 6 kbps for UDP, TCP and SMTP respectively.
The high per-message overhead of SMTP can be improved by increasing the
MTU, for example, an MTU of 12,000 octets improves the throughput to 13
kbps as figure smtp-MTUs shows. Our research paper [Transport2014]_ has 
some more details on the benchmarking results.

Bluetooth plugin
.. _Bluetooth-plugin:

Bluetooth plugin
----------------

This page describes the new Bluetooth transport plugin for GNUnet. The
plugin is still in the testing stage so don't expect it to work
perfectly. If you have any questions or problems just post them here or
ask on the IRC channel.

-  What do I need to use the Bluetooth plugin transport?

-  BluetoothHow does it work?

-  What possible errors should I be aware of?

-  How do I configure my peer?

-  How can I test it?

.. _What-do-I-need-to-use-the-Bluetooth-plugin-transport_003f:

What do I need to use the Bluetooth plugin transport?
-----------------------------------------------------

If you are a GNU/Linux user and you want to use the Bluetooth transport
plugin you should install the ``BlueZ`` development libraries (if they
aren't already installed). For instructions about how to install the
libraries you should check out the BlueZ site
(`http://www.bluez.org <http://www.bluez.org/>`__). If you don't know if
you have the necessary libraries, don't worry, just run the GNUnet
configure script and you will be able to see a notification at the end
which will warn you if you don't have the necessary libraries.

.. _How-does-it-work2_003f:

.. todo:: Change to unique title?

How does it work2?
------------------

The Bluetooth transport plugin uses virtually the same code as the WLAN
plugin and only the helper binary is different. The helper takes a
single argument, which represents the interface name and is specified in
the configuration file. Here are the basic steps that are followed by
the helper binary used on GNU/Linux:

-  it verifies if the name corresponds to a Bluetooth interface name

-  it verifies if the interface is up (if it is not, it tries to bring
   it up)

-  it tries to enable the page and inquiry scan in order to make the
   device discoverable and to accept incoming connection requests *The
   above operations require root access so you should start the
   transport plugin with root privileges.*

-  it finds an available port number and registers a SDP service which
   will be used to find out on which port number is the server listening
   on and switch the socket in listening mode

-  it sends a HELLO message with its address

-  finally it forwards traffic from the reading sockets to the STDOUT
   and from the STDIN to the writing socket

Once in a while the device will make an inquiry scan to discover the
nearby devices and it will send them randomly HELLO messages for peer
discovery.

.. _What-possible-errors-should-I-be-aware-of_003f:

What possible errors should I be aware of?
------------------------------------------

*This section is dedicated for GNU/Linux users*

Well there are many ways in which things could go wrong but I will try
to present some tools that you could use to debug and some scenarios.

-  ``bluetoothd -n -d`` : use this command to enable logging in the
   foreground and to print the logging messages

-  ``hciconfig``: can be used to configure the Bluetooth devices. If you
   run it without any arguments it will print information about the
   state of the interfaces. So if you receive an error that the device
   couldn't be brought up you should try to bring it manually and to see
   if it works (use ``hciconfig -a hciX up``). If you can't and the
   Bluetooth address has the form 00:00:00:00:00:00 it means that there
   is something wrong with the D-Bus daemon or with the Bluetooth
   daemon. Use ``bluetoothd`` tool to see the logs

-  ``sdptool`` can be used to control and interrogate SDP servers. If
   you encounter problems regarding the SDP server (like the SDP server
   is down) you should check out if the D-Bus daemon is running
   correctly and to see if the Bluetooth daemon started correctly(use
   ``bluetoothd`` tool). Also, sometimes the SDP service could work but
   somehow the device couldn't register its service. Use
   ``sdptool browse [dev-address]`` to see if the service is registered.
   There should be a service with the name of the interface and GNUnet
   as provider.

-  ``hcitool`` : another useful tool which can be used to configure the
   device and to send some particular commands to it.

-  ``hcidump`` : could be used for low level debugging

.. _How-do-I-configure-my-peer2_003f:

.. todo:: Fix name/referencing now that we're using Sphinx.

How do I configure my peer2?
----------------------------

On GNU/Linux, you just have to be sure that the interface name
corresponds to the one that you want to use. Use the ``hciconfig`` tool
to check that. By default it is set to hci0 but you can change it.

A basic configuration looks like this:

::

   [transport-bluetooth]
   # Name of the interface (typically hciX)
   INTERFACE = hci0
   # Real hardware, no testing
   TESTMODE = 0 TESTING_IGNORE_KEYS = ACCEPT_FROM;

In order to use the Bluetooth transport plugin when the transport
service is started, you must add the plugin name to the default
transport service plugins list. For example:

::

   [transport] ...  PLUGINS = dns bluetooth ...

If you want to use only the Bluetooth plugin set *PLUGINS = bluetooth*

On Windows, you cannot specify which device to use. The only thing that
you should do is to add *bluetooth* on the plugins list of the transport
service.

.. _How-can-I-test-it_003f:

How can I test it?
------------------

If you have two Bluetooth devices on the same machine and you are using
GNU/Linux you must:

-  create two different file configuration (one which will use the first
   interface (*hci0*) and the other which will use the second interface
   (*hci1*)). Let's name them *peer1.conf* and *peer2.conf*.

-  run *gnunet-core -c peerX.conf -i* in order to generate the peers
   private keys. The **X** must be replace with 1 or 2.

-  run *gnunet-arm -c peerX.conf -s -i=transport* in order to start the
   transport service. (Make sure that you have \"bluetooth\" on the
   transport plugins list if the Bluetooth transport service doesn't
   start.)

-  run *gnunet-core -c peer1.conf -i* to get the first peer's ID. If
   you already know your peer ID (you saved it from the first command),
   this can be skipped.

-  run *gnunet-transport -c peer2.conf -p=PEER1_ID -s* to start sending
   data for benchmarking to the other peer.

This scenario will try to connect the second peer to the first one and
then start sending data for benchmarking.

If you have two different machines and your configuration files are good
you can use the same scenario presented on the beginning of this
section.

Another way to test the plugin functionality is to create your own
application which will use the GNUnet framework with the Bluetooth
transport service.

.. _The-implementation-of-the-Bluetooth-transport-plugin:

The implementation of the Bluetooth transport plugin
----------------------------------------------------

This page describes the implementation of the Bluetooth transport
plugin.

First I want to remind you that the Bluetooth transport plugin uses
virtually the same code as the WLAN plugin and only the helper binary is
different. Also the scope of the helper binary from the Bluetooth
transport plugin is the same as the one used for the WLAN transport
plugin: it accesses the interface and then it forwards traffic in both
directions between the Bluetooth interface and stdin/stdout of the
process involved.

The Bluetooth plugin transport could be used both on GNU/Linux and
Windows platforms.

-  Linux functionality

-  Pending Features

.. _Linux-functionality:

Linux functionality
^^^^^^^^^^^^^^^^^^^

In order to implement the plugin functionality on GNU/Linux I used the
BlueZ stack. For the communication with the other devices I used the
RFCOMM protocol. Also I used the HCI protocol to gain some control over
the device. The helper binary takes a single argument (the name of the
Bluetooth interface) and is separated in two stages:

.. _THE-INITIALIZATION:

.. todo:: 'THE INITIALIZATION' should be in bigger letters or stand out, not
          starting a new section?

THE INITIALIZATION
^^^^^^^^^^^^^^^^^^

-  first, it checks if we have root privileges (*Remember that we need
   to have root privileges in order to be able to bring the interface up
   if it is down or to change its state.*).

-  second, it verifies if the interface with the given name exists.

   **If the interface with that name exists and it is a Bluetooth
   interface:**

-  it creates a RFCOMM socket which will be used for listening and call
   the *open_device* method

   On the *open_device* method:

   -  creates a HCI socket used to send control events to the device

   -  searches for the device ID using the interface name

   -  saves the device MAC address

   -  checks if the interface is down and tries to bring it UP

   -  checks if the interface is in discoverable mode and tries to make
      it discoverable

   -  closes the HCI socket and binds the RFCOMM one

   -  switches the RFCOMM socket in listening mode

   -  registers the SDP service (the service will be used by the other
      devices to get the port on which this device is listening on)

-  drops the root privileges

   **If the interface is not a Bluetooth interface the helper exits with
   a suitable error**

.. _THE-LOOP:

THE LOOP
^^^^^^^^

The helper binary uses a list where it saves all the connected neighbour
devices (*neighbours.devices*) and two buffers (*write_pout* and
*write_std*). The first message which is send is a control message with
the device's MAC address in order to announce the peer presence to the
neighbours. Here are a short description of what happens in the main
loop:

-  Every time when it receives something from the STDIN it processes the
   data and saves the message in the first buffer (*write_pout*). When
   it has something in the buffer, it gets the destination address from
   the buffer, searches the destination address in the list (if there is
   no connection with that device, it creates a new one and saves it to
   the list) and sends the message.

-  Every time when it receives something on the listening socket it
   accepts the connection and saves the socket on a list with the
   reading sockets.

-  Every time when it receives something from a reading socket it parses
   the message, verifies the CRC and saves it in the *write_std* buffer
   in order to be sent later to the STDOUT.

So in the main loop we use the select function to wait until one of the
file descriptor saved in one of the two file descriptors sets used is
ready to use. The first set (*rfds*) represents the reading set and it
could contain the list with the reading sockets, the STDIN file
descriptor or the listening socket. The second set (*wfds*) is the
writing set and it could contain the sending socket or the STDOUT file
descriptor. After the select function returns, we check which file
descriptor is ready to use and we do what is supposed to do on that kind
of event. *For example:* if it is the listening socket then we accept a
new connection and save the socket in the reading list; if it is the
STDOUT file descriptor, then we write to STDOUT the message from the
*write_std* buffer.

To find out on which port a device is listening on we connect to the
local SDP server and search the registered service for that device.

*You should be aware of the fact that if the device fails to connect to
another one when trying to send a message it will attempt one more time.
If it fails again, then it skips the message.* *Also you should know
that the transport Bluetooth plugin has support for*\ **broadcast
messages**\ *.*

.. _Details-about-the-broadcast-implementation:

Details about the broadcast implementation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

First I want to point out that the broadcast functionality for the
CONTROL messages is not implemented in a conventional way. Since the
inquiry scan time is too big and it will take some time to send a
message to all the discoverable devices I decided to tackle the problem
in a different way. Here is how I did it:

-  If it is the first time when I have to broadcast a message I make an
   inquiry scan and save all the devices' addresses to a vector.

-  After the inquiry scan ends I take the first address from the list
   and I try to connect to it. If it fails, I try to connect to the next
   one. If it succeeds, I save the socket to a list and send the message
   to the device.

-  When I have to broadcast another message, first I search on the list
   for a new device which I'm not connected to. If there is no new
   device on the list I go to the beginning of the list and send the
   message to the old devices. After 5 cycles I make a new inquiry scan
   to check out if there are new discoverable devices and save them to
   the list. If there are no new discoverable devices I reset the
   cycling counter and go again through the old list and send messages
   to the devices saved in it.

**Therefore**:

-  every time when I have a broadcast message I look up on the list for
   a new device and send the message to it

-  if I reached the end of the list for 5 times and I'm connected to all
   the devices from the list I make a new inquiry scan. *The number of
   the list's cycles after an inquiry scan could be increased by
   redefining the MAX_LOOPS variable*

-  when there are no new devices I send messages to the old ones.

Doing so, the broadcast control messages will reach the devices but with
delay.

*NOTICE:* When I have to send a message to a certain device first I
check on the broadcast list to see if we are connected to that device.
If not we try to connect to it and in case of success we save the
address and the socket on the list. If we are already connected to that
device we simply use the socket.

.. _Pending-features:

Pending features
^^^^^^^^^^^^^^^^

-  Implement a testcase for the helper : *The testcase consists of a
   program which emulates the plugin and uses the helper. It will
   simulate connections, disconnections and data transfers.*

If you have a new idea about a feature of the plugin or suggestions
about how I could improve the implementation you are welcome to comment
or to contact me.

.. _WLAN-plugin:

WLAN plugin
-----------

This section documents how the wlan transport plugin works. Parts which
are not implemented yet or could be better implemented are described at
the end.

.. [Transport2014] https://bib.gnunet.org/date.html#paper_5fshort2014
