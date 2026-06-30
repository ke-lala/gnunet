FAQs
====

General
-------

General questions about the project.

What do I do if my question is not answered here?
   A: There are many other sources of information. You can read additional documentation or ask the question on the help-gnunet@gnu.org mailing list.

When are you going to release the next version?
   A: The general answer is, when it is ready. A better answer may be: earlier if you contribute (test, debug, code, document). Every release will be anounced on the info-gnunet@gnu.org mailing list and on planet GNU. You can subscribe to the mailing list or the RSS feed of this site to automatically receive a notification.

Is the code free?
   A: GNUnet is free software, available under the GNU Affero Public License (AGPL).

Are there any known bugs?
   A: We track the list of currently known bugs in the Mantis system. Some bugs are occasionally reported directly to developers or the developer mailing list. This is discouraged since developers often do not have the time to feed these bugs back into the Mantis database. Please report bugs directly to the bug tracking system. If you believe a bug is sensitive, you can set its view status to private (this should be the exception).

Is there a graphical user interface?
   A: gnunet-gtk is a separate download. The package contains various GTK+ based graphical interfaces, including a graphical tool for configuration.

Why does gnunet-service-nse create a high CPU load?
   A: The gnunet-service-nse process will initially compute a so-called "proof-of-work" which is used to convince the network that your peer is real (or, rather, make it expensive for an adversary to mount a Sybil attack on the network size estimator). The calculation is expected to take a few days, depending on how fast your CPU is. If the CPU load is creating a problem for you, you can set the value "WORKDELAY" in the "nse" section of your configuration file to a higher value. The default is "5 ms".

How does GNUnet compare to Tor?
   A: Tor focuses on anonymous communication and censorship-resistance for TCP connections and, with the Tor Browser Bundle, for the Web in particular. GNUnet does not really have one focus; our theme is secure decentralized networking, but that is too broad to be called a focus.

How does GNUnet compare to I2P?
   A: Both GNUnet and I2P want to build a better, more secure, more decentralized Internet. However, on the technical side, there are almost no overlaps.
   I2P is written in Java, and has (asymmetric) tunnels using onion (or garlic) routing as the basis for various (anonymized) applications. I2P is largely used via a Web frontend.

Is GNUnet ready for use on production systems?
   A: GNUnet is still undergoing major development. It is largely not yet ready for usage beyond developers. Your mileage will vary depending on the functionality you use, but you will always likely run into issues with our current low-level transport system. We are currently in the process of rewriting it (Project "Transport Next Generation [TNG]")

Is GNUnet build using distributed ledger technologies?
   A: No. GNUnet is a new network protocol stack for building secure, distributed, and privacy-preserving applications. While a ledger could be built using GNUnet, we currently have no plans in doing so.

Features
--------

What can I do with GNUnet?
   A: GNUnet is a peer-to-peer framework, by which we mostly mean that it can do more than just one thing. Naturally, the implementation and documentation of some of the features that exist are more advanced than others. For users, GNUnet offers anonymous and non-anonymous file-sharing, a fully decentralized and censorship-resistant replacement for DNS and a mechanism for IPv4-IPv6 protocol translation and tunneling (NAT-PT with DNS-ALG). See also: Applications.

Is it possible to surf the WWW anonymously with GNUnet?
   A: It is not possible use GNUnet for anonymous browsing at this point. We recommend that you use Tor for anonymous surfing.

Is it possible to access GNUnet via a browser as an anonymous WWW?
   A: There is currently no proxy (like fproxy in Freenet) for GNUnet that would make it accessible via a browser. It is possible to build such a proxy and all one needs to know is the protocol used between the browser and the proxy and the GNUnet code for file-sharing.

Is there a graphical user interface?
   A: There are actually a few graphical user interfaces for different functions. gnunet-setup is to configure GNUnet, and gnunet-fs-gtk is for file-sharing. There are a few other gnunet-XXX-gtk GUIs of lesser importance. Note that in order to obtain the GUI, you need to install the gnunet-gtk package, which is a separate download. gnunet-gtk is a meta GUI that integrates most of the other GUIs in one window. One exception is gnunet-setup, which must still be run separately at this time (as setup requires the peer to be stopped).

Which operating systems does GNUnet run on?
   A: GNUnet is being developed and tested primarily under Debian GNU/Linux. Furthermore, we regularly build and test GNUnet on Fedora, Ubuntu, Arch, FreeBSD and macOS. We have reports of working versions on many other GNU/Linux distributions; in the past we had reports of working versions on NetBSD, OpenBSD and Solaris. However, not all of those reports are recent, so if you cannot get GNUnet to work on those systems please let us know.

GNU Name System
---------------

Who runs the GNS root zone?
   A: Short answer: you. The long answer is the GNUnet will ship with a default configuration of top-level domains. The governance of this default configuration is not yet established. In any case, the user will be able to modify this configuration at will. We expect normal users to have no need to edit their own GNS zone(s) unless they host services themselves.

Where is the per-user GNS database kept?
   A: The short answer is that the database is kept at the user's GNUnet peer. Now, a user may run multiple GNUnet peers, in which case the database could be kept at each peer (however, we don't have code for convenient replication). Similarly, multiple GNUnet peers can share one instance of the database --- the "gnunet-service-namestore" can be accessed from remote (via TCP). The actual data can be stored in a Postgres database, for which various replication options are again applicable. Ultimately, there are many options for how users can store (and secure) their GNS database.

What is the expected average size of a GNS namestore database?
   A: Pretty small. Based on our user study where we looked at browser histories and the number of domains visited, we expect that GNS databases will only grow to a few tens of thousands of entries, small enough to fit even on mobile devices.

Is GNS resistant to the attacks on DNS used by the US?
   A: We believe so, as there is no entity that any government could force to change the mapping for a name except for each individual user (and then the changes would only apply to the names that this user is the authority for). So if everyone used GNS, the only practical attack of a government would be to force the operator of a server to change the GNS records for his server to point elsewhere. However, if the owner of the private key for a zone is unavailable for enforcement, the respective zone cannot be changed and any other zone delegating to this zone will achieve proper resolution.


How does GNS compare to other name systems?
   A: A scientific paper on this topic `has been published <https://grothoff.org/christian/ns2018.pdf>`_ and below is a table from the publication. For detailed descriptions please refer to the paper.


What is the difference between GNS and CoDoNS?
   A: CoDoNS decentralizes the DNS database (using a DHT) but preserves the authority structure of DNS. With CoDoNS, IANA/ICANN are still in charge, and there are still registrars that determine who owns a name.
   With GNS, we decentralize the database and also decentralize the responsibility for naming: each user runs their own personal root zone and is thus in complete control of the names they use. GNS also has many additional features (to keep names short and enable migration) which don't even make sense in the context of CoDoNS.

What is the difference between GNS and SocialDNS?
   A: Like GNS, SocialDNS allows each user to create DNS mappings. However, with SocialDNS the mappings are shared through the social network and subjected to ranking. As the social relationships evolve, names can thus change in surprising ways. With GNS, names are primarily shared via delegation, and thus mappings will only change if the user responsible for the name (the authority) manually changes the record.

What is the difference between GNS and ODDNS?
   A: ODDNS is primarily designed to bypass the DNS root zone and the TLD registries (such as those for ".com" and ".org"). Instead of using those, each user is expected to maintain a database of (second-level) domains (like "gnu.org") and the IP addresses of the respective name servers. Resolution will fail if the target name servers change IPs.

What is the difference between GNS and Handshake?
   A: Handshake is a blockchain-based method for root zone governance. Hence, it does not address the name resolution process itself but delegates resolution into DNS after the initial TLD resolution. Not taking sustainablility considerations into account, Handshake could be used as an additional supporting GNS root zone governance model, but we currently do not have such plans in mind.

What is the difference between GNS and TrickleDNS?
   A: TrickleDNS pushes ("critical") DNS records between DNS resolvers of participating domains to provide "better availability, lower query resolution times, and faster update propagation". Thus TrickleDNS is focused on defeating attacks on the availability (and performance) of record propagation in DNS, for example via DDoS attacks on DNS root servers. TrickleDNS is thus concerned with how to ensure distribution of authoritative records, and authority remains derived from the DNS hierarchy.

Does GNS require real-world introduction (secure PKEY exchange) in the style of the PGP web of trust?
   A: For security, it is well known that an initial trust path between the two parties must exist. However, for applications where this is not required, weaker mechanisms can be used. For example, we have implemented a first-come-first-served (FCFS) authority which allows arbitrary users to register arbitrary names. The key of this authority is included with every GNUnet installation. Thus, any name registered with FCFS is in fact global and requires no further introduction. However, the security of these names depends entirely on the trustworthiness of the FCFS authority. The authority can be queried under the ".pin" TLD.

How can a legitimate domain owner tell other people to not use his name in GNS?
   A: Names have no owners in GNS, so there cannot be a "legitimate" domain owner. Any user can claim any name (as his preferred name or "pseudonym") in his NICK record. Similarly, all other users can choose to ignore this preference and use a name of their choice (or even assign no name) for this user.

Did you consider the privacy implications of making your personal GNS zone visible?
   A: Each record in GNS has a flag "private". Records are shared with other users (via DHT or zone transfers) only if this flag is not set. Thus, users have full control over what information about their zones is made public.

Are "Legacy Host" (LEHO) records not going to be obsolete with IPv6?
   A: The question presumes that (a) virtual hosting is only necessary because of IPv4 address scarcity, and (b) that LEHOs are only useful in the context of virtual hosting. However, LEHOs are also useful to help with X.509 certificate validation (as they specify for which legacy hostname the certificate should be valid). Also, even with IPv6 fully deployed and "infinite" IP addresses being available, we're not sure that virtual hosting would disappear. Finally, we don't want to have to wait for IPv6 to become commonplace, GNS should work with today's networks.

Why does GNS not use a trust metric or consensus to determine globally unique names?
   A: Trust metrics have the fundamental problem that they have thresholds. As trust relationships evolve, mappings would change their meaning as they cross each others thresholds. We decided that the resulting unpredictability of the resolution process was not acceptable. Furthermore, trust and consensus might be easy to manipulate by adversaries.


How do you handle compromised zone keys in GNS?
   A: The owner of a private key can create a revocation message. This one can then be flooded throughout the overlay network, creating a copy at all peers. Before using a public key, peers check if that key has been revoked. All names that involve delegation via a revoked zone will then fail to resolve. Peers always automatically check for the existence of a revocation message when resolving names.

Could the signing algorithm of GNS be upgraded in the future?
   A: Yes. In our efforts to standardize GNS, we have already modified the protocol to support alternative delegation records.
   Naturally, deployed GNS implementations would have to be updated to support the new signature scheme. The new scheme can then be run in parallel with the existing system by using a new record type to indicate the use of a different cipher system.

How can a GNS zone maintain several name servers, e.g. for load balancing?
   A: We don't expect this to be necessary, as GNS records are stored (and replicated) in the R5N DHT. Thus the authority will typically not be contacted whenever clients perform a lookup. Even if the authority goes (temporarily) off-line, the DHT will cache the records for some time. However, should having multiple servers for a zone be considered truly necessary, the owner of the zone can simply run multiple peers (and share the zone's key and database among them).

Why do you believe it is worth giving up unique names for censorship resistance?
   A: The GNU Name system offers an alternative to DNS that is censorship resistant. As with any security mechanism, this comes at a cost (names are not globally unique). To draw a parallel, HTTPS connections use more bandwidth and have higher latency than HTTP connections. Depending on your application, HTTPS may not be worth the cost. However, for users that are experiencing censorship (or are concerned about it), giving up globally unique names may very well be worth the cost. After all, what is a "globally" unique name worth, if it does not resolve?

Why do you say that DNS is 'centralized' and 'distributed'?
   A: We say that DNS is 'centralized' because it has a central component / central point of failure --- the root zone and its management by IANA/ICANN. This centralization creates vulnerabilities. For example, the US government was able to reassign the management of the country-TLDs of Afganistan and Iraq during the wars at the beginning of the 21st century.

How does GNS protect against layer-3 censorship?
   A: GNS does not directly help with layer-3 censorship, but it does help indirectly in two ways:
   Many websites today use virtual hosting, so blocking a particular IP address causes much more collateral damage than blocking a DNS name. It thus raises the cost of censorship.
   Existing layer-3 circumvention solutions (such as Tor) would benefit from a censorship resistant naming system. Accessing Tor's ".onion" namespace currently requires users to use unmemorable cryptographic identifiers. With nicer names, Tor and tor2web-like services would be even easier to use.

Does GNS work with search engines?
   A: GNS creates no significant problems for search engines, as they can use GNS to perform name resolution as well as any normal user. Naturally, while we typically expect normal users to install custom software for name resolution, this is unlikely to work for search engines today. However, the DNS2GNS gateway allows search engines to use DNS to resolve GNS names, so they can still index GNS resources. However, as using DNS2GNS gateways breaks the cryptographic chain of trust, legacy search engines will obviously not obtain censorship-resistant names.

How does GNS compare to the Unmanaged Internet Architecture (UIA)?
   A: UIA and GNS both share the same basic naming model, which actually originated with Rivest's SDSI. However, UIA is not concerned about integration with legacy applications and instead focuses on universal connectivity between a user's many machines. In contrast, GNS was designed to interoperate with DNS as much as possible, and to also work as much as possible with the existing Web infrastructure. UIA is not at all concerned about legacy systems (clean slate).

Doesn't GNS increase the trusted-computing base compared to DNS(SEC)?
   A: First of all, in GNS you can explicitly see the trust chain, so you know if a name you are resolving belongs to a friend, or a friend-of-a-friend, and can thus decide how much you trust the result. Naturally, the trusted-computing base (TCB) can become arbitrarily large this way --- however, given the name length restriction, for an individual name it is always less than about 128 entities.

How does GNS handle SRV/TLSA records where service and protocol are part of the domain name?
   A: When GNS splits a domain name into labels for resolution, it detects the "_Service._Proto" syntax, converts "Service" to the corresponding port number and "Proto" to the corresponding protocol number. The rest of the name is resolved as usual. Then, when the result is presented, GNS looks for the GNS-specific "BOX" record type. A BOX record is a record that contains another record (such as SRV or TLSA records) and adds a service and protocol number (and the original boxed record type) to it.

Error messages
--------------

I receive many "WARNING Calculated flow delay for X at Y for Z". Should I worry?
   A: Right now, this is expected and a known cause for high latency in GNUnet. We have started a major rewrite to address this and other problems, but until the Transport Next Generation (TNG) is ready, these warnings are expected.

Error opening ``/dev/net/tun: No such file or directory``?
   A: If you get this error message, the solution is simple. Issue the following commands (as root) to create the required device file

::

    # mkdir /dev/net
    # mknod /dev/net/tun c 10 200

``iptables: No chain/target/match by that name.`` (when running ``gnunet-service-dns``)?
   A: For GNUnet DNS, your iptables needs to have "owner" match support. This is accomplished by having the correct kernel options. Check if your kernel has ``CONFIG_NETFILTER_XT_MATCH_OWNER`` set to either ``y`` or ``m`` (and the module is loaded).

``Timeout was reached`` when running PT on Fedora (and possibly others)?
   A: If you get an error stating that the VPN timeout was reached, check if your firewall is enabled and blocking the connections.

I'm getting an ``error while loading shared libraries: libgnunetXXX.so.X``
   A: This error usually occurs when your linker fails to locate one of GNUnet's libraries. This can have two causes. First, it is theoretically possible that the library is not installed on your system;
   however, if you compiled GNUnet the normal way and/or used a binary package, that is highly unlikely.
   The more common cause is that you installed GNUnet to a directory that your linker does not search.
   There are several ways to fix this that are described below. If you are ``root`` and you installed to a system folder (such as ``/usr/local``), you want to add the libraries to the system-wide search path.
   This is done by adding a line ``/usr/local/lib/`` to ``/etc/ld.so.conf`` and running "ldconfig".
   If you installed GNUnet to ``/opt`` or any other similar path, you obviously have to change ``/usr/local`` accordingly.
   If you do not have ``root`` rights or if you installed GNUnet to say ``/home/$USER/``, then you can explicitly tell your linker to search a particular directory for libraries using the ``LD_LIBRARY_PATH`` environment variable.
   For example, if you configured GNUnet using a prefix of ``$HOME/gnunet/`` you want to run:

::

   $ export LD_LIBRARY_PATH=$HOME/gnunet/lib:$LD_LIBRARY_PATH
   $ export PATH=$HOME/gnunet/bin:$PATH

to ensure GNUnet's binaries and libraries are found. In order to avoid having to do so each time, you can add the above lines (without the "$") to your .bashrc or .profile file. You will have to logout and login again to have this new profile be applied to all shells (including your desktop environment).

What error messages can be ignored?
  A: Error messages flagged as ``DEBUG`` should be disabled in binaries built for end-users and can always be ignored.
  Error messages flagged as ``INFO`` always refer to harmless events that require no action.
  For example, GNUnet may use an ``INFO`` message to indicate that it is currently performing an expensive operation that will take some time.
  GNUnet will also use ``INFO`` messages to display information about important configuration values.

File-sharing
------------

How does GNUnet compare to other file-sharing applications?
   A: As opposed to Napster, Gnutella, Kazaa, FastTrack, eDonkey and most other P2P networks, GNUnet was designed with security in mind as the highest priority. We intend on producing a network with comprehensive security features. Many other P2P networks are open to a wide variety of attacks, and users have little privacy. GNUnet is also Free Software and thus the source code is available, so you do not have to worry about being spied upon by the software. The following table summarises the main differences between GNUnet and other systems. The information is accurate to the best of our knowledge. The comparison is difficult since there are sometimes differences between various implementations of (almost) the same protocol. In general, we pick a free implementation as the reference implementation since it is possible to inspect the free code. Also, all of these systems are changing over time and thus the data below may not be up-to-date. If you find any flaws, please let us know. Finally, the table is not saying terribly much (it is hard to compare these systems this briefly), so if you want the real differences, read the research papers (and probably the code).

   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Net-  | GNUnet| One   | Nap-  | Direct| Fast  | e-    | Gnu-  | Free- |
   | work  | FS    | Swarm | ster  | Co-   | Track | Donkey| tella | net   |
   |       |       |       |       | nnect |       |       |       |       |
   |       |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Distri| yes   | yes   | no    | hubs  | s     | DHT   | yes   | yes   |
   | buted |       |       |       |       | uper- | (e    |       |       |
   | Quer- |       |       |       |       | peers | Mule) |       |       |
   | ies   |       |       |       |       |       |       |       |       |
   |       |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Multi-| yes   | yes   | no    | no    | yes   | yes   | yes   | no    |
   | source|       |       |       |       |       |       |       |       |
   | Down- |       |       |       |       |       |       |       |       |
   | load  |       |       |       |       |       |       |       |       |
   |       |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Econ- | yes   | yes   | no    | no    | no    | yes   | no    | no    |
   | omics |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Anon- | yes   | maybe | no    | no    | no    | no    | no    | yes   |
   | ymity |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Lan-  | C     | Java  | often | C++   | C     | C++   | often | Java  |
   | guage |       |       | C     |       |       |       | C     |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Trans-| UDP,  | TCP   | TCP   | TCP?  | UDP,  | UDP,  | TCP   | TCP   |
   | port  | TCP,  |       |       |       | TCP   | TCP   |       |       |
   | Proto-| SMTP, |       |       |       |       |       |       |       |
   | col   | HTTP  |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Query | key   | fil   | key   | file  | file  | file  | file  | s     |
   | Format| words | ename | words | name, | name, | name, | name, | ecret |
   | (UI)  | / CHK | /     |       | THEX  | SHA   | MD4?  | SHA   | key,  |
   |       |       | SHA?  |       |       |       |       |       | CHK   |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Rout- | dy-   |       |       |       |       |       |       |       |
   | ing   | namic | static| always| always| always| always| always| always|
   |       | (in-  | (in-  |       |       |       |       |       | in-   |
   |       | direct| direct| direct| direct| direct| direct| direct| direct|
   |       | di-   | di-   |       |       |       |       |       |       |
   |       | rect) | rect) |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+
   | Licen-| GPL   | GPL   | GPL   | GPL   | GPL   | GPL   | GPL   | GPL   |
   | se    |       |       | (knap | (Val  | (giFT)| (e-   | (gtk  |       |
   |       |       |       | ster) | knut) |       | Mule) | -gnut |       |
   |       |       |       |       |       |       |       | ella) |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+-------+

   Another important point of reference are the various anonymous peer-to-peer networks. Here, there are differences in terms of application domain and how specifically anonymity is achieved. Anonymous routing is a hard research topic, so for a superficial comparison like this one we focus on the latency. Another important factor is the programming language. Type-safe languages may offer certain security benefits; however, this may come at the cost of significant increases in resource consumption which in turn may reduce anonymity.


Are there any known attacks (on GNUnet's file-sharing application)?
   A: Generally, there is the possibility of a known plaintext attack on keywords, but since the user has control over the keywords that are associated with the content he inserts, the user can take advantage of the same techniques used to generate reasonable passwords to defend against such an attack. In any event, we are not trying to hide content; thus, unless the user is trying to insert information into the network that can only be shared with a small group of people, there is no real reason to try to obfuscate the content by choosing a difficult keyword anyway.

What do you mean by anonymity?
   A: Anonymity is the lack of distinction of an individual from a (large) group. A central goal for anonymous file-sharing in GNUnet is to make all users (peers) form a group and to make communications in that group anonymous, that is, nobody (but the initiator) should be able to tell which of the peers in the group originated the message. In other words, it should be difficult to impossible for an adversary to distinguish between the originating peer and all other peers.

What does my system do when participating in GNUnet file sharing?
   A: In GNUnet you set up a node (a peer). It is identified by an ID (hash of its public key) and has a number of addresses it is reachable by (may have no addresses, for instance when it's behind a NAT). You specify bandwidth limits (how much traffic GNUnet is allowed to consume) and datastore quote (how large your on-disk block storage is) . Your node will then proceed to connect to other nodes, becoming part of the network.

Contributing
------------

How can I help translate this webpage into other languages?
   A: First, you need to register an account with our weblate system. Please send an e-mail with the desired target language to translators@gnunet.org or ask for help on the #gnunet chat on irc.freenode.net. Typically someone with sufficient permissions will then grant you access. Naturally, any abuse will result in the loss of permissions.

I have some great idea for a new feature, what should I do?
   A: Sadly, we have many more feature requests than we can possibly implement. The best way to actually get a new feature implemented is to do it yourself --- and to then send us a patch.

