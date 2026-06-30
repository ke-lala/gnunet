The GNU Name System
-------------------

The GNU Name System (GNS) is secure and decentralized naming system. It
allows its users to register names as top-level domains (TLDs) and
resolve other namespaces within their TLDs.

GNS is designed to provide:

-  Censorship resistance
-  Query privacy
-  Secure name resolution
-  Compatibility with DNS

Unlike DNS, GNS does not rely on central root zones or authorities.
Instead any user administers their own root and can can create arbitrary
name value mappings. Furthermore users can delegate resolution to other
users’ zones just like DNS NS records do. Zones are uniquely identified
via public keys and resource records are signed using the corresponding
public key. Delegation to another user’s zone is done using special delegation
records and petnames. A petname is a name that can be freely chosen by
the user. This results in non-unique name-value mappings as www.bob to
one user might be www.friend for someone else.

For a complete specification of the protocol, we refer to `LSD0001 <https://lsd.gnunet.org/lsd0001>`__.

Start Zones
~~~~~~~~~~~

In GNS you are the master of your own Root.
This means that you are able (and encouraged!) to manage your own *Start Zones*.
A *Start Zone* is a mapping from a name suffix (e.g. ``.gnunet.org``) to
a GNS zone key.
A user's configuration of *Start Zones* is the equivalent of DNS's `Root Zone <https://en.wikipedia.org/wiki/DNS_root_zone>`_.
In other words, in GNS you can be your very own `ICANN <https://en.wikipedia.org/wiki/ICANN>`_.

There are three types of *Start Zones*:

  1. Mappings to *remote* GNS zones.
  2. Mappings to your own *local* GNS zones.
  3. Explicit zone Top-Level-Domains (zTLDs)

Remote
""""""

Your GNUnet installation ships with a default configuration of remote
*Start Zones*.
The first is ``.gnunet.org``, which points to the authoritative zone of the
GNUnet project.
This *Start Zone* allows you to resolve names ending with ``.gnunet.org``.
It can be used to resolve, for example, ``www.gnunet.org``.

You can try it out for yourself (if you are connected to the peer-to-peer network):

::

  $ gnunet-gns --lookup=www.gnunet.org


Another *Start Zone* configured by default is ``.pin.gns.alt``.
It points to a special zone also managed by the GNUnet project.
Users may register subdomains on a first-come first-served-basis at https://fcfs.gnunet.org.

Use ``gnunet-config -s gns`` to view the GNS configuration, including all
configured external zones that are operated by other users.
The respective configuration entry names start with a ``.``, e.g. ``.pin.gns.alt``.

You can change this default configuration at any time and add *Start Zone* to the
respective zones of your friends! For this, simply obtain the respective
public key (you will learn how below) and extend the configuration:

::

   $ gnunet-config -s gns -o .myfriend -V PUBLIC_KEY

Local
"""""

In order to create a local GNS zone you have to create a so-called ``ego`` in GNUnet.
Egos may correspond to pseudonyms or real-world identities.
It is recommended to use separate egos for separate activities.
All egos can potentially serve as a GNS zone, but you do not have to use
all egos as GNS zones.
All your local zones are used as *Start Zones*.

Technically, an ego is first of all a public-private key pair, and thus
egos also always correspond to a GNS zone. Egos are managed through the
IDENTITY service and its tooling.
The IDENTITY service is used to create and stores private keys with associated
human-readable nicknames.
Those mappings serve as local *Start Zones*.

For example, you can create a new ego (or zone) with name *myzone* using the
``gnunet-identity`` tool using:

::

   $ gnunet-identity --create="myzone"

Henceforth, on your system you control the TLD ``myzone``.
You are able to resolve GNS names ending with ``.myzone``.
However, the zone is still empty.
We will look at local zone maintenance in the next section.
Note that by default, this will create ECDSA key pairs.
GNS also supports the use of EdDSA keys.
In order to create a EdDSA zone, execute:

::

  $ gnunet-identity --create="myedzone" --eddsa

All of your zones can be listed (displayed) using the gnunet-identity
command line tool as well:

::

   $ gnunet-identity --display



Zone Top-Level-Domains
""""""""""""""""""""""

If you know the public key of a GNS zone, you can resolve records in that zone
without a *Start Zone* mapping.
To do so, you must provide the string-representation of the zone key
as Top-Level Domain. Consider the zone key of ``.gnunet.org`` in the default
configuration. In order to resolve the same record as above using its respective
zTLD, you can do so as follows:

::

  $ gnunet-gns --lookup=www.000G0047M3HN599H57MPXZK4VB59SWK4M9NRD68E1JQFY3RWAHDMKAPN30

The use of zTLDs is mostly useful in the absence of a *Start Zone* configuration
for that zone or when querying names programmatically.

Local zone maintenance
~~~~~~~~~~~~~~~~~~~~~~

So you are ready to maintain your own GNS zone? Great.
First you should review and optionally configure your NAMESTORE database which
will hold your zone's resource records.
You have the choice between ``sqlite`` and ``postgres`` as databases.
Most users will be served well with the ``sqlite`` backend and with its default
configuration.
To change the database backend of your NAMESTORE to ``postgres`` execute:

::

  $ gnunet-config -s namestore -o DATABASE -V postgres

You can now configure the ``postgres`` database backend to find your database.
See the ``namestore-postgres`` configuration section on possible options:

::

  $ gnunet-config -s namestore-postgres

If you ever need to reset your zone database, you can use ``gnunet-namestore-dbtool``
with the ``--reset`` option.
**DANGER: This command will delete ALL of your records in all zones!**

Orphaning
"""""""""

It can happen that you accidentally or semi-accidentally delete your ego, e.g.
using ``gnunet-identity --delete=myzone``.
In this case the records stored in the zone are still in the database but the
missing ego causes them to be *orphaned*.

Orphaned records are still published in the DHT but access to them is limited.
For example, managing records through the NAMESTORE API is not longer possible.

You can check for orphaned records using:

::

  $ gnunet-namestore --list-orphans

This will output any orphaned records in the format

.. code-block:: text

  <$LABEL>.<$PRIVATE_KEY>:
  $RECORD_DATA

In order to recover (and un-orphan) the records, you can re-add the ego by
executing

::

  $ gnunet-identity --create=<myzone> --privkey=$PRIVATE_KEY

If instead you should decide that you want to **purge** all orphaned records,
you can execute

::

  $ gnunet-namestore --purge-orphans

Records
"""""""

At this point you can add (or edit, or remove) records in your GNS zone using the
using the gnunet-namestore command-line tool.
Your records will be stored in a database
of the NAMESTORE service. Note that for multi-user setups, the NAMESTORE
database will include the combined records of all users.
However, users will not be able to see each other’s records if they are marked
as private.

To provide a short example for editing your own zone, suppose you have
your own web server with the IP ``1.2.3.4``. Then you can put an A record (A
records in DNS are for IPv4 IP addresses) into your local zone ``myzone``
using the command:

::

   $ gnunet-namestore --zone=myzone \
                      --add \
                      --name=www \
                      --type=A \
                      --value=1.2.3.4 \
                      --expiration=1d

Similar commands will work for other types of DNS and GNS records, the
syntax largely depending on the type of the record. Naturally, most
users may find editing the zones using the gnunet-namestore-gtk GUI to
be easier.

Note that by default, records are **private**. This means that they are not
published and cannot be resolved by other users that may have configured your
zone as one of their ``Start Zones``.
In order to create a public record, provide the ``--public`` switch to the
``gnunet-namestore`` command above.

You can now try to resolve your own GNS record. The method we found
to be the most uncomplicated is to do this by explicitly resolving using
``gnunet-gns``.
In the shell, type:

::

   $ gnunet-gns -u www.myzone

You should receive the record as configured above.
At this point, you can try addition additional records to ``www`` or add other
names to your zone.

Each zone in GNS has a public-private key. Usually, gnunet-namestore and
gnunet-setup will access your private key as necessary, so you do not
have to worry about those. What is important is your public key (or
rather, the hash of your public key), as you will likely want to give it
to others so that they can securely link to you.

A central operation in GNS is the ability to securely delegate to other
zones. Basically, by adding a delegation you make all of the names from
the other zone available to yourself. This section describes how to
create delegations.

Suppose you have a friend who you call ’bob’ who also uses GNS. You can
then delegate resolution of names to Bob’s zone by adding a PKEY record
to their local zone:

::

   $ gnunet-namestore -a -n bob --type PKEY -V $BOBKEY -e 1d -z myzone

Note that ``$BOBKEY`` in the command above must be replaced with the hash of
Bob’s public key (the output your friend obtained using the
gnunet-identity command from the previous section and told you, for
example by giving you a business card containing this information as a
QR code).

Assuming Bob has an ``A`` record for their website under the name of ``www``
in his zone, you can then access Bob’s website under ``www.bob.myzone`` -
as well as any (public) GNS record that Bob has in their zone by
replacing www with the respective name of the record in Bob’s zone.

Furthermore, if Bob has themselves a (public) delegation to Carol’s zone
under *carol*, you can access Carol’s records under
*NAME.carol.bob.myzone* (where *NAME* is the name of Carol’s record you
want to access).

You can find more detailed information in available record types in the
`Resource Records`_ section.

Revocation
""""""""""

Now, in the situation of an attacker gaining access to the private key
of one of your egos, the attacker can create records in the respective
GNS zone and publish them as if you published them. Anyone resolving
your domain will get these new records and when they verify they seem
authentic because the attacker has signed them with your key.

To address this potential security issue, you can pre-compute a
revocation certificate corresponding to your ego. This certificate, when
published on the P2P network, flags your private key as invalid, and all
further resolutions or other checks involving the key will fail.

A revocation certificate is thus a useful tool when things go out of
control, but at the same time it should be stored securely. Generation
of the revocation certificate for a zone can be done through
``gnunet-revocation``. For example, the following command (as unprivileged
user) generates a revocation file ``revocation.dat`` for the zone ``myzone``:

::

  $ gnunet-revocation -f revocation.dat -R myzone

The above command only pre-computes a revocation certificate. It does
not revoke the given zone. Pre-computing a revocation certificate
involves computing a proof-of-work and hence may take up to 4 to 5 days
on a modern processor. Note that you can abort and resume the
calculation at any time. Also, even if you did not finish the
calculation, the resulting file will contain the signature, which is
sufficient to complete the revocation process even without access to the
private key. So instead of waiting for a few days, you can just abort
with ``CTRL-C``, backup the revocation certificate and run the calculation
only if your key actually was compromised. This has the disadvantage of
revocation taking longer after the incident, but the advantage of saving
a significant amount of energy. So unless you believe that a key
compromise will need a rapid response, we urge you to wait with
generating the revocation certificate. Also, the calculation is
deliberately expensive, to deter people from doing this just for fun (as
the actual revocation operation is expensive for the network, not for
the peer performing the revocation).

To avoid TL;DR ones from accidentally revoking their zones, we are not
giving away the command, but it is uncomplicated: the actual revocation
is performed by using the ``-p`` option of ``gnunet-revocation``.

Backup
""""""

One should always backup their files, especially in these SSD days (our
team has suffered 3 SSD crashes over a span of 2 weeks). Backing up peer
identity and zones is achieved by copying the following files:

The peer identity file can be found in
``~/.local/share/gnunet/private_key.ecc``.

The private keys of your egos are stored in the directory
``~/.local/share/gnunet/identity/egos/``. They are stored in files whose
filenames correspond to the zones’ ego names. These are probably the
most important files you want to backup from a GNUnet installation.

Note: All these files contain cryptographic keys and they are stored
without any encryption. So it is advisable to backup encrypted copies of
them.

This does **NOT** backup your record data.
Your record data is stored in the respective database backend, and hence
you may want to backup the respective databases as well.


Large Zones
"""""""""""

Large zones in DNS such as Top-Level Domains can grow to millions of names.
For example, the ``.fr`` DNS zone contained over 3.9 million entries in 2022.
Such use cases require good performance and careful consideration of a couple
of choices in GNS and NAMESTORE.

The cryptographic operations of EdDSA are significantly faster than their
ECDSA counterparts.
Most users will be unable to observe this difference.
But, if you plan on managing large zones, you may want to consider using EdDSA
zones.

For large zones, using the PostgreSQL NAMESTORE database backend is a must.
Further, when importing large numbers of records at once, it is advised to make
use of the bulk import functionality of the NAMESTORE API.
There is a :ref:`C API for bulk imports <NAMESTORE-Subsystem>`.
There is also a :ref:`REST API endpoint <namestore_rest_api>` which calls this API.

Zones are published and signed by the ZONEMASTER service.
In order to speed up this process for large zones, you can edit the number of
spawned worker threads for the service:

::

  $ gnunet-config -s zonemaster -o WORKER_COUNT -V $COUNT

It is reasonable to choose your number of CPUs as ``$COUNT``.

Application integration
~~~~~~~~~~~~~~~~~~~~~~~

Once you have verified that resolution of names using ``gnunet-gns`` works,
we can start integrating GNS with applications.
We can distinguish between two types of applications:

  1. **GNS-aware applications**: Such applications know what GNS is and how it
     can be used to resolve names. Examples are some of GNUnet's services such
     as `re:claimID <reclaimID-Identity-Provider>`_ or `Messenger <Using-the-GNUnet-Messenger>`_.
  2. **GNS-unaware applications**: Applications that implicitly assume the names
     are resolved through DNS. Such applications use OS-specific APIs or query
     configured DNS servers directly.

In the following, we will discuss integration paths of GNS-unaware applications.

Integration with Browsers (DNS2GNS service)
"""""""""""""""""""""""""""""""""""""""""""

Most OSes allow you to either modify your ``/etc/resolv.conf`` directly
or through ``resolvectl``. We are going to configure the dns2gns service
in order to translate DNS name queries by applications to GNS name
queries where applicable and else fall back to DNS.

Optionally, you may want to configure your dns2gns service to run on a
non-privileged port like 5353. But, in case you are going to edit
``/etc/resolv.conf`` directly, the dns2gns service MUST run on port 53
as you cannot specify the port number. A ``$FALLBACK_DNS`` variable
should be a DNS server you trust such as your local router:

::

   $ gnunet-config -s dns2gns -o OPTIONS -V "-d $FALLBACK_DNS -p 5252"
   $ gnunet-arm -i dns2gns

If you edit your resolv.conf directly, it should contain and entry like
this:

::

   nameserver 127.0.0.1

In any case, it is very likely that the method of modification of your
resolver is OS specific. Recently, the combination of NetworkManager and
systemd-resolved is becoming increasingly popular.

If you use resolvectl and systemd-resolved you can temporarily set the
nameserver like this:

::

   $ resolvectl $INTERFACE 127.0.0.1:5353

Where ``$INTERFACE`` is your network interface such as ``eth0``.

In order to automatically set the DNS2GNS server if it is running
already you can use NetworkManager-dispatcher. First, enable it:

::

   $ sudo systemctl enable NetworkManager-dispatcher.service
   $ sudo systemctl start NetworkManager-dispatcher.service

Then, create a script ``/etc/NetworkManager/dispatch.h/10-dns2-gns.sh``:

.. code-block:: bash

   #!/bin/sh
   interface=$1
   status=$2

   if [ "$interface" = "eth0" ]; then
     case $status in
       up)
         if nc -u -z 127.0.0.1 5353; then
         resolvectl dns $interface 127.0.0.1:5353
       fi
       ;;
       down)
       ;;
     esac
   fi

Make sure the script is owned by root and executable:

::

   $ sudo root:root /etc/NetworkManager/dispatch.d/10-dns2gns.sh
   $ sudo +x /etc/NetworkManager/dispatch.d/10-dns2gns.sh

You can test accessing this website using your browser or curl:

::

   $ curl www.gnunet.org

Note that *gnunet.org* is a domain that also exists in DNS and for which
the GNUnet project webservers can provide trusted TLS certificates. When
using non-DNS names with GNS or aliases, this may result in issues when
accessing HTTPS websites with browsers. In order learn how to provide
relief for this issue, read on.

Integration with Browsers (SOCKS proxy)
"""""""""""""""""""""""""""""""""""""""

While we recommend integrating GNS using the DNS2GNS service or the
NSSwitch plugin, you can also integrate GNS directly with your browser
via the gnunet-gns-proxy. This method can have the advantage that the
proxy can validate TLS/X.509 records and thus strengthen web security;
however, the proxy is still a bit brittle, so expect subtle failures. We
have had reasonable success with Chromium, and various frustrations with
Firefox in this area recently.

The first step is to start the proxy. As the proxy is (usually) not
started by default, this is done as a unprivileged user using gnunet-arm
-i gns-proxy. Use gnunet-arm -I as a unprivileged user to check that the
proxy was actually started. (The most common error for why the proxy may
fail to start is that you did not run gnunet-gns-proxy-setup-ca during
installation.) The proxy is a SOCKS5 proxy running (by default) on port
7777. Thus, you need to now configure your browser to use this proxy.
With Chromium, you can do this by starting the browser as a unprivileged
user using chromium –proxy-server=*socks5://localhost:7777* For Firefox
(or Icecat), select *Edit-Preferences* in the menu, and then select the
*Advanced* tab in the dialog and then *Network*:

Here, select *Settings…* to open the proxy settings dialog. Select
*Manual proxy configuration* and enter localhost with port 7777 under
SOCKS Host. Furthermore, set the checkbox *Proxy DNS when using SOCKS
v5* at the bottom of the dialog. Finally, push *OK*.

You must also go to about:config and change the
browser.fixup.alternate.enabled option to false, otherwise the browser
will autoblunder an address like www.gnu to www.gnu.com. If you want to
resolve @ in your own TLDs, you must additionally set
browser.fixup.dns_first_use_for_single_words to true.

After configuring your browser, you might want to first confirm that it
continues to work as before. (The proxy is still experimental and if you
experience *odd* failures with some webpages, you might want to disable
it again temporarily.) Next, test if things work by typing
*http://test.gnu/* into the URL bar of your browser. This currently
fails with (my version of) Firefox as Firefox is super-smart and tries
to resolve *http://www.test.gnu/* instead of *test.gnu*. Chromium can be
convinced to comply if you explicitly include the *http://* prefix —
otherwise a Google search might be attempted, which is not what you
want. If successful, you should see a simple website.

Note that while you can use GNS to access ordinary websites, this is
more an experimental feature and not really our primary goal at this
time. Still, it is a possible use-case and we welcome help with testing
and development.

NSS plugin
""""""""""

To activate the plugin, you need to edit your
``/etc/nsswitch.conf`` where you should find a line like this:

::

   hosts: files mdns4_minimal [NOTFOUND=return] dns mdns4

The exact details may differ a bit, which is fine. Add the text
``gns [NOTFOUND=return]`` after ``files``:

::

   hosts: files gns [NOTFOUND=return] mdns4_minimal [NOTFOUND=return] dns mdns4

Note that some systems, in particular those running systemd-resolvd, manual modification of ``/etc/nsswitch.conf`` is not possible as it is autogenerated.
In such cases it is better to use the *DNS2GNS* service approach in combination with ``resolvectl`` as explained above.

Zone dissemination
~~~~~~~~~~~~~~~~~~

So you have decided to maintain and publish one or more GNS zones.
You are probably asking yourself how your zone finds its way into the
*Start Zone* configuration of other users.

Below are pointers on how to make your zone known.

The .pin zone
"""""""""""""

First, you may want to visit https://fcfs.gnunet.org and register your zone(s)
with the ``.pin.gns.alt`` top-level domain shipped by default with GNUnet.
This will allow other GNUnet users to resolve your zone under the name you
managed to acquire for yourself.

The registration policy of PIN is "first-come, first-served".

Be Social
"""""""""

Next, you should print out your business card and be social. Find a
friend, help them install GNUnet and exchange business cards with them.
Or, if you’re a desperate loner, you might try the next step with your
own card. Still, it’ll be hard to have a conversation with yourself
later, so it would be better if you could find a friend. You might also
want a camera attached to your computer, so you might need a trip to the
store together.

Before we get started, we need to tell gnunet-qr which zone it should
import new records into. For this, run:

::

   $ gnunet-identity -s namestore -e NAME

where NAME is the name of the zone you want to import records into. In
our running example, this would be *gnu*.

Henceforth, for every business card you collect, simply run:

::

   $ gnunet-qr

to open a window showing whatever your camera points at. Hold up your
friend’s business card and tilt it until the QR code is recognized. At
that point, the window should automatically close. At that point, your
friend’s NICKname and their public key should have been automatically
imported into your zone.

Assuming both of your peers are properly integrated in the GNUnet
network at this time, you should thus be able to resolve your friends
names. Suppose your friend’s nickname is *Bob*. Then, type

::

   $ gnunet-gns -u test.bob

to check if your friend was as good at following instructions as you
were.


Creating a business card
""""""""""""""""""""""""

Before we can really use GNS, you should create a business card. Note
that this requires having LaTeX installed on your system. If you are
using a Debian GNU/Linux based operating system, the following command
should install the required components. Keep in mind that this requires
3GB of downloaded data and possibly even more when unpacked. On a GNU
Guix based system texlive 2017 has returns a DAG size of 5032.4 MiB. The
packages which are confirmed to be required are:

-  texlive-units
-  texlive-labels
-  texlive-pst-barcode
-  texlive-luatex85
-  texlive-preview
-  texlive-pdfcrop
-  texlive-koma-script

**We welcome any help in identifying the required components of the
TexLive Distribution. This way we could just state the required
components without pulling in the full distribution of TexLive.**

::

   apt-get install texlive-full

Start creating a business card by clicking the *Copy* button in
gnunet-namestore-gtk. Next, you should start the gnunet-bcd program (in
the terminal, on the command-line). You do not need to pass any options,
and please be not surprised if there is no output:

::

   $ gnunet-bcd

Then, start a browser and point it to http://localhost:8888/ where
gnunet-bcd is running a Web server!

First, you might want to fill in the *GNS Public Key* field by
right-clicking and selecting *Paste*, filling in the public key from the
copy you made in gnunet-namestore-gtk. Then, fill in all of the other
fields, including your GNS NICKname. Adding a GPG fingerprint is
optional. Once finished, click *Submit Query*. If your LaTeX
installation is incomplete, the result will be disappointing. Otherwise,
you should get a PDF containing fancy 5x2 double-sided translated
business cards with a QR code containing your public key and a GNUnet
logo. We’ll explain how to use those a bit later. You can now go back to
the shell running gnunet-bcd and press CTRL-C to shut down the Web
server.

The GNUnet default Start Zones
""""""""""""""""""""""""""""""

The GNUnet default *Start Zones* are managed through `GANA <https://gana.gnunet.org>`_ and
follows a `restrictive registration policy <https://git.gnunet.org/gana.git/tree/gnu-name-system-default-tlds/POLICY>`_.

The GNUnet GNS Registrar
""""""""""""""""""""""""

The GNUnet GNS Registrar is a standalone package availiable as download
in the `GNU FTP Mirror <https://ftpmirror.gnu.org/gnunet>`_.

You can compile it using

::

  $ ./configure && make

To run it execute

::

  $ ./gnunet-gns-registrar

The registrar requires configuration of a `GNU Taler Merchant <https://www.taler.net/en/>`_.
You can look at the example configuration for possible options.
https://fcfs.gnunet.org is running a GNS registrar instance connected to the GNU Taler
demo exchanges with a merchant running on https://merchant.gnunet.org for testing purposes
where you can exchange KUDOS (which you get for free upon registration at the GNU Taler Demo
system).
Consequently, while requiring KUDOS as payment to register, this is still effectively a
first-come-first server service.

Resource Records
~~~~~~~~~~~~~~~~

GNS supports the majority of the DNS records as defined in `RFC
1035 <http://www.ietf.org/rfc/rfc1035.txt>`_. Additionally, GNS defines
some new record types the are unique to the GNS system. For example,
GNS-specific resource records are used to give petnames for zone
delegation, revoke zone keys and provide some compatibility features.

For some DNS records, GNS does extended processing to increase their
usefulness in GNS. In particular, GNS introduces special names referred
to as *zone relative names*. Zone relative names are allowed in some
resource record types (for example, in NS and CNAME records) and can
also be used in links on webpages. Zone relative names end in `.+` which
indicates that the name needs to be resolved relative to the current
authoritative zone. The extended processing of those names will expand
the `.+` with the correct delegation chain to the authoritative zone
(replacing `.+` with the name of the location where the name was
encountered) and hence generate a valid GNS name.

The GNS currently supports the record types as defined in
`GANA <https://git.gnunet.org/gana.git/tree/gnu-name-system-record-types/registry.rec>`__.
In addition, GNS supports DNS record types, such as `A`, `AAAA` or `TXT`.

For a complete description of the records, please refer to the
specification at `LSD0001 <https://lsd.gnunet.org/lsd0001>`__.

In the following, we discuss GNS records with specific behavior or
special handling in GNUnet.


VPN
"""

GNS allows easy access to services provided by the GNUnet Virtual Public
Network. When the GNS resolver encounters a VPN record it will contact
the VPN service to try and allocate an IPv4/v6 address (if the queries
record type is an IP address) that can be used to contact the service.

**Example**

I want to provide access to the VPN service *web.gnu.* on port 80 on
peer ABC012: Name: www; RRType: VPN; Value: 80 ABC012 web.gnu.

The peer ABC012 is configured to provide an exit point for the service
*web.gnu.* on port 80 to it’s server running locally on port 8080 by
having the following lines in the gnunet.conf configuration file:

::

   [web.gnunet.]
   TCP_REDIRECTS = 80:localhost4:8080

DNS Migration
~~~~~~~~~~~~~

The following tools facilitate DNS to GNS zone migrations.

The Zoneimport mechanism through ``gnunet-zoneimport`` allows you to mirror delegations in a TLD. We provide an example for AFNIC's ``.fr``.
Why would you do it this way instead of a zone transfer using AXFR?
Because some TLD authorities, such as AFNIC, do not support AXFR and only publish a list
of delegated names.
In such cases, ``gnunet-zoneimport`` comes in handy. While not perfect in terms
of mirroring record updates and expirations, it is better than not being able to
mirror the ``.fr`` TLD at all.
In general, if the naming authority supports AXFR, then you should consider using Ascension (see below).
If you are the naming authority, then you should consider manually managing synchronization
between DNS and GNS or use the ``gnunet-namestore-zonefile`` tool to import DNS zonefiles
into GNS.

Note that the namestore by default also populates the namecache. This
pre-population is cryptographically expensive. Thus, on systems that
only serve to import a large (millions of records) DNS zone and that do
not have a local gns service in use, it is thus advisable to disable the
namecache by setting the option *DISABLE* to *YES* in section
*[namecache]*.


Zoneimport
""""""""""

If you want to support GNS but the master database for a zone is only
available and maintained in DNS, GNUnet includes the gnunet-zoneimport
tool to monitor a DNS zone and automatically import records into GNS.
Today, the tool does not yet support DNS AF(X)R, as we initially used it
on the *.fr* zone which does not allow us to perform a DNS zone
transfer. Instead, gnunet-zoneimport reads a list of DNS domain names
from stdin, issues DNS queries for each, converts the obtained records
(if possible) and stores the result in the namestore.

The zonemaster service then takes the records from the namestore,
publishes them into the DHT which makes the result available to the GNS
resolver. In the GNS configuration, non-local zones can be configured to
be intercepted by specifying *.tld = PUBLICKEY* in the configuration
file in the *[gns]* section.

For example, assuming you have the AFNIC NomsDeDomainEnPointFr CSV file you can execute:

::

    $ cat OPENDATA_A-NomsDeDomaineEnPointFr.csv | cut -d';' -f 1 | tail -n+2 | gnunet-zoneimport 194.0.9.1

This reads the first value of each line in the CSV as the domain name to import.
The first line is skipped because it (usually) is a header, not an entry.
The authoritative DNS server for ``fr`` is provided as IP.
Note that you need to provide authoritative DNS servers.

Zonefile
""""""""

In order to import DNS zones which are already defined in a
`zone file <https://en.wikipedia.org/wiki/Zone_file>`_ in use
with existing DNS daemons such as BIND you may try to import your DNS zones
into DNS using ``gnunet-namestore-zonefile``.
Assuming your zonefile is called ``myzonefile``, execute:

::

  $ gnunet-namestore-zonefile < myzonefile

As you can see, the program reads the zonefile from standard input.
Upon encountering an ``$ORIGIN`` directive a new ego will be created with that
name unless it already exists.
Note that ``$ORIGIN`` is a FQDN and ends with a ``.``, e.g. ``example.com.``.
The ego will be called ``example.com`` (without the ``.``).
All records following the directive will be imported into the zone identified by
this origin.
Multiple ``$ORIGIN`` directives within the same zone file are supported and
egos will be created accordingly.
If no ``$ORIGIN`` is explicitly specified in the beginning of the file
(as is sometimes the case in BIND setups) or if you want to override the (first)
occurence, you can provide an ego to use:

::

  $ gnunet-namestore-zonefile -z myorigin < myzonefile

Note that in the conversion process some information found in the zone file
is lost and other information is less precise than in GNS.
For example, records stored in the ``$ORIGIN`` can be specified in multiple
ways:

.. code-block:: text

  $ORIGIN example.com.
  $TTL 3600
  example.com.    IN  MX    10 mail.example.com.  ; mail.example.com is the mailserver for example.com
  @               IN  MX    20 mail2.example.com. ; equivalent to above line, "@" represents zone origin

GNS will map the origin ``example.com.`` to the empty label ``@`` whenever possible.
Further, ``$TTL`` is given in seconds. GNS record expirations are defined in
microseconds. The seconds will be converted to their equivalent in microseconds.
Note how this means that conversion of GNS records (in particular their expirations)
to a DNS zone file cannot be done without loss of information.
This also means that converting the GNS zone back into a zone file (a function
currently not implemented) would not yield the same zone file that was imported.

Ascension
"""""""""

Ascension is a tool to migrate existing DNS zones into GNS.

Compared to the gnunet-zoneimport tool it strictly uses AXFR or IXFR
depending on whether or not there exists a SOA record for the zone. If
that is the case it will take the serial as a reference point and
request the zone. The server will either answer the IXFR request with a
correct incremental zone or with the entire zone, which depends on the
server configuration.

Before you can migrate any zone though, you need to start a local GNUnet
peer. To migrate the Syrian top level domain - one of the few top level
domains that support zone transfers - into GNS use the following
command:

::

   $ ascension sy. -n ns1.tld.sy. -p

The -p flag will tell GNS to put these records on the DHT so that other
users may resolve these records by using the public key of the zone.

Once the zone is migrated, Ascension will output a message telling you,
that it will refresh the zone after the time has elapsed. You can
resolve the names in the zone directly using GNS or if you want to use
it with your browser, check out the GNS manual section. Configuring the
GNU Name System. To resolve the records from another system you need the
respective zones PKEY. To get the zones public key, you can run the
following command:

::

   $ gnunet-identity -dqe sy

Where *sy* is the name of the zone you want to migrate.

You can share the PKEY of the zone with your friends. They can then
resolve records in the zone by doing a lookup replacing the zone label
with your PKEY:

::

   $ gnunet-gns -t SOA -u "$PKEY"

The program will continue to run as a daemon and update once the refresh
time specified in the zones SOA record has elapsed.

DNSCurve style records are supported in the latest release and they are
added as a PKEY record to be referred to the respective GNS public key.
Key distribution is still a problem but provided someone else has a
public key under a given label it can be looked up.

There is an unofficial Debian package called python3-ascension that adds
a system user ascension and runs a GNUnet peer in the background.

Ascension-bind is also an unofficial Debian package that on installation
checks for running DNS zones and whether or not they are transferable
using DNS zone transfer (AXFR). It asks the administrator which zones to
migrate into GNS and installs a systemd unit file to keep the zone up to
date. If you want to migrate different zones you might want to check the
unit file from the package as a guide.
