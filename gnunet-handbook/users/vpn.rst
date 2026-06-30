.. _Using-the-Virtual-Public-Network:

Virtual Public Network
----------------------

Using the GNUnet Virtual Public Network (VPN) application you can tunnel
IP traffic over GNUnet. Moreover, the VPN comes with built-in protocol
translation and DNS-ALG support, enabling IPv4-to-IPv6 protocol
translation (in both directions). This chapter documents how to use the
GNUnet VPN.

The first thing to note about the GNUnet VPN is that it is a public
network. All participating peers can participate and there is no secret
key to control access. So unlike common virtual private networks, the
GNUnet VPN is not useful as a means to provide a \"private\" network
abstraction over the Internet. The GNUnet VPN is a virtual network in
the sense that it is an overlay over the Internet, using its own routing
mechanisms and can also use an internal addressing scheme. The GNUnet
VPN is an Internet underlay --- TCP/IP applications run on top of it.

The VPN is currently only supported on GNU/Linux systems. Support for
operating systems that support TUN (such as FreeBSD) should be easy to
add (or might not even require any coding at all --- we just did not
test this so far). Support for other operating systems would require
re-writing the code to create virtual network interfaces and to
intercept DNS requests.

The VPN does not provide good anonymity. While requests are routed over
the GNUnet network, other peers can directly see the source and
destination of each (encapsulated) IP packet. Finally, if you use the
VPN to access Internet services, the peer sending the request to the
Internet will be able to observe and even alter the IP traffic. We will
discuss additional security implications of using the VPN later in this
chapter.

.. _Setting-up-an-Exit-node:

Setting up an Exit node
~~~~~~~~~~~~~~~~~~~~~~~

Any useful operation with the VPN requires the existence of an exit node
in the GNUnet Peer-to-Peer network. Exit functionality can only be
enabled on peers that have regular Internet access. If you want to play
around with the VPN or support the network, we encourage you to setup
exit nodes. This chapter documents how to setup an exit node.

There are four types of exit functions an exit node can provide, and
using the GNUnet VPN to access the Internet will only work nicely if the
first three types are provided somewhere in the network. The four exit
functions are:

-  DNS: allow other peers to use your DNS resolver

-  IPv4: allow other peers to access your IPv4 Internet connection

-  IPv6: allow other peers to access your IPv6 Internet connection

-  Local service: allow other peers to access a specific TCP or UDP
   service your peer is providing

By enabling \"exit\" in gnunet-setup and checking the respective boxes
in the \"exit\" tab, you can easily choose which of the above exit
functions you want to support.

Note, however, that by supporting the first three functions you will
allow arbitrary other GNUnet users to access the Internet via your
system. This is somewhat similar to running a Tor exit node. The
Torproject has a nice article about what to consider if you want to do
this here. We believe that generally running a DNS exit node is
completely harmless.

The exit node configuration does currently not allow you to restrict the
Internet traffic that leaves your system. In particular, you cannot
exclude SMTP traffic (or block port 25) or limit to HTTP traffic using
the GNUnet configuration. However, you can use your host firewall to
restrict outbound connections from the virtual tunnel interface. This is
highly recommended. In the future, we plan to offer a wider range of
configuration options for exit nodes.

Note that by running an exit node GNUnet will configure your kernel to
perform IP-forwarding (for IPv6) and NAT (for IPv4) so that the traffic
from the virtual interface can be routed to the Internet. In order to
provide an IPv6-exit, you need to have a subnet routed to your host's
external network interface and assign a subrange of that subnet to the
GNUnet exit's TUN interface.

When running a local service, you should make sure that the local
service is (also) bound to the IP address of your EXIT interface (e.g.
169.254.86.1). It will NOT work if your local service is just bound to
loopback. You may also want to create a \"VPN\" record in your zone of
the GNU Name System to make it easy for others to access your service
via a name instead of just the full service descriptor. Note that the
identifier you assign the service can serve as a passphrase or shared
secret, clients connecting to the service must somehow learn the
service's name. VPN records in the GNU Name System can make this easier.

The config file configuration to offer a service `http` on port 80 which
is forwarded to port 8080 on IP `169.254.86.1` is as follows:

::

  [http.gnunet.]
  TCP_REDIRECTS = 80:169.254.86.1:8080

You can then add a GNS record to this service:

::

  $  gnunet-namestore -z myzone -a -e "1 d" -p -t VPN -n www -V "6 <ZKEY> http"

Where `myzone` is the name of your GNS zone.
We are adding a record of type `VPN`, and the value is a string containing three values:
The first is a boolean indicating the use of TCP (`6`) or UDP (`17`),
your peer identity and the identifier of the service we used in the configuration section above
before the `.gnunet.`.

When using the `dns2gns` DNS server it will automatically synthesize `A`/`AAAA` records from
encountered `VPN` records using the `VPN` service.
Otherwise, this must be done by the client that resolved the `VPN` record.

.. _Fedora-and-the-Firewall:

Fedora and the Firewall
~~~~~~~~~~~~~~~~~~~~~~~

When using an exit node on Fedora 15, the standard firewall can create
trouble even when not really exiting the local system! For IPv4, the
standard rules seem fine. However, for IPv6 the standard rules prohibit
traffic from the network range of the virtual interface created by the
exit daemon to the local IPv6 address of the same interface (which is
essentially loopback traffic, so you might suspect that a standard
firewall would leave this traffic alone). However, as somehow for IPv6
the traffic is not recognized as originating from the local system (and
as the connection is not already \"established\"), the firewall drops
the traffic. You should still get ICMPv6 packets back, but that's
obviously not very useful.

Possible ways to fix this include disabling the firewall (do you have a
good reason for having it on?) or disabling the firewall at least for
the GNUnet exit interface (or the respective IPv4/IPv6 address range).
The best way to diagnose these kinds of problems in general involves
setting the firewall to REJECT instead of DROP and to watch the traffic
using wireshark (or tcpdump) to see if ICMP messages are generated when
running some tests that should work.

.. _Setting-up-VPN-node-for-protocol-translation-and-tunneling:

Setting up VPN node for protocol translation and tunneling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The GNUnet VPN/PT subsystem enables you to tunnel IP traffic over the
VPN to an exit node, from where it can then be forwarded to the
Internet. This section documents how to setup VPN/PT on a node. Note
that you can enable both the VPN and an exit on the same peer. In this
case, IP traffic from your system may enter your peer's VPN and leave
your peer's exit. This can be useful as a means to do protocol
translation. For example, you might have an application that supports
only IPv4 but needs to access an IPv6-only site. In this case, GNUnet
would perform 4to6 protocol translation between the VPN (IPv4) and the
Exit (IPv6). Similarly, 6to4 protocol translation is also possible.
However, the primary use for GNUnet would be to access an Internet
service running with an IP version that is not supported by your ISP. In
this case, your IP traffic would be routed via GNUnet to a peer that has
access to the Internet with the desired IP version.

Setting up an entry node into the GNUnet VPN primarily requires you to
enable the \"VPN/PT\" option in \"gnunet-setup\". This will launch the
\"gnunet-service-vpn\", \"gnunet-service-dns\" and \"gnunet-daemon-pt\"
processes. The \"gnunet-service-vpn\" will create a virtual interface
which will be used as the target for your IP traffic that enters the
VPN. Additionally, a second virtual interface will be created by the
\"gnunet-service-dns\" for your DNS traffic. You will then need to
specify which traffic you want to tunnel over GNUnet. If your ISP only
provides you with IPv4 or IPv6-access, you may choose to tunnel the
other IP protocol over the GNUnet VPN. If you do not have an ISP (and
are connected to other GNUnet peers via WLAN), you can also choose to
tunnel all IP traffic over GNUnet. This might also provide you with some
anonymity. After you enable the respective options and restart your
peer, your Internet traffic should be tunneled over the GNUnet VPN.

The GNUnet VPN uses DNS-ALG to hijack your IP traffic. Whenever an
application resolves a hostname (like 'gnunet.org'), the
\"gnunet-daemon-pt\" will instruct the \"gnunet-service-dns\" to
intercept the request (possibly route it over GNUnet as well) and
replace the normal answer with an IP in the range of the VPN's
interface. \"gnunet-daemon-pt\" will then tell \"gnunet-service-vpn\" to
forward all traffic it receives on the TUN interface via the VPN to the
original destination.

For applications that do not use DNS, you can also manually create such
a mapping using the gnunet-vpn command-line tool. Here, you specify the
desired address family of the result (e.g. \"-4\"), and the intended
target IP on the Internet (e.g. \"-i 131.159.74.67\") and \"gnunet-vpn\"
will tell you which IP address in the range of your VPN tunnel was
mapped.

``gnunet-vpn`` can also be used to access \"internal\" services offered
by GNUnet nodes. So if you happen to know a peer and a service offered
by that peer, you can create an IP tunnel to that peer by specifying the
peer's identity, service name and protocol (--tcp or --udp) and you will
again receive an IP address that will terminate at the respective peer's
service.

