Advanced Configuration
----------------------


.. _Config-file-format:

Config file format
~~~~~~~~~~~~~~~~~~

In GNUnet realm, all components obey the same pattern to get
configuration values. According to this pattern, once the component has
been installed, the installation deploys default values in
``$prefix/share/gnunet/config.d/``, in ``.conf`` files. In order to
override these defaults, the user can write a custom ``.conf`` file and
either pass it to the component at execution time, or name it
``gnunet.conf`` and place it under ``$HOME/.config/``.

A config file is a text file containing sections, and each section
contains its values. The right format follows:

.. code-block:: text

   [section1]
   value1 = string
   value2 = 23

   [section2]
   value21 = string
   value22 = /path22

Throughout any configuration file, it is possible to use ``$``-prefixed
variables, like ``$VAR``, especially when they represent filenames in in
the filesystem. It is also possible to provide defaults values for those
variables that are unset, by using the following syntax:

.. code-block:: text

   ${VAR:-default}

However, there are two ways a user can set ``$``-prefixable variables:
(a) by defining them under a ``[paths]`` section

.. code-block:: text

   [paths]
   GNUNET_DEPLOYMENT_SHARED = ${HOME}/shared-data
   ..
   [section-x]
   path-x = ${GNUNET_DEPLOYMENT_SHARED}/x

or (b) by setting them in the environment

.. code-block:: text

   $ export VAR=/x

The configuration loader will give precedence to variables set under
``[path]``, though.

The utility '\ ``gnunet-config``\ ', which gets installed along with
GNUnet, serves to get and set configuration values without directly
editing the ``.conf`` file. The option '\ ``-f``\ ' is particularly
useful to resolve filenames, when they use several levels of
``$``-expanded variables. See '\ ``gnunet-config --help``\ '.

Note that, in this stage of development, the file
``$HOME/.config/gnunet.conf`` can contain sections for **all** the
components.
.. _The-Single_002dUser-Setup:

The Single-User Setup
~~~~~~~~~~~~~~~~~~~~~

For the single-user setup, you do not need to do anything special and
can just start the GNUnet background processes using ``gnunet-arm``. By
default, GNUnet looks in ``~/.config/gnunet.conf`` for a configuration
(or ``$XDG_CONFIG_HOME/gnunet.conf`` if ``$XDG_CONFIG_HOME`` is
defined). If your configuration lives elsewhere, you need to pass the
``-c FILENAME`` option to all GNUnet commands.

Assuming the configuration file is called ``~/.config/gnunet.conf``, you
start your peer using the ``gnunet-arm`` command (say as user
``gnunet``) using:


.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -s

The \"-s\" option here is for \"start\". The command should return
almost instantly. If you want to stop GNUnet, you can use:

.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -e

The \"-e\" option here is for \"end\".

Note that this will only start the basic peer, no actual applications
will be available. If you want to start the file-sharing service, use
(after starting GNUnet):

.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -i fs

The \"-i fs\" option here is for \"initialize\" the \"fs\"
(file-sharing) application. You can also selectively kill only
file-sharing support using

.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -k fs

Assuming that you want certain services (like file-sharing) to be always
automatically started whenever you start GNUnet, you can activate them
by setting \"IMMEDIATE_START=YES\" in the respective section of the
configuration file (for example, \"[fs]\"). Then GNUnet with
file-sharing support would be started whenever you enter:

.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -s

Alternatively, you can combine the two options:

.. code-block:: text

   gnunet-arm -c ~/.config/gnunet.conf -s -i fs

Using ``gnunet-arm`` is also the preferred method for initializing
GNUnet from ``init``.

Finally, you should edit your ``crontab`` (using the ``crontab``
command) and insert a line 

.. code-block:: text

   @reboot gnunet-arm -c ~/.config/gnunet.conf -s

to automatically start your peer whenever your system boots.

.. _The-Multi_002dUser-Setup:

The Multi-User Setup
~~~~~~~~~~~~~~~~~~~~

This requires you to create a user ``gnunet`` and an additional group
``gnunetdns``, prior to running ``make install`` during installation.
Then, you create a configuration file ``/etc/gnunet.conf`` which should
contain the lines: 

.. code-block:: text

   [arm]
   START_SYSTEM_SERVICES = YES
   START_USER_SERVICES = NO

Then, perform the same steps to run GNUnet as in the per-user
configuration, except as user ``gnunet`` (including the ``crontab``
installation). You may also want to run ``gnunet-setup`` to configure
your peer (databases, etc.). Make sure to pass ``-c /etc/gnunet.conf``
to all commands. If you run ``gnunet-setup`` as user ``gnunet``, you
might need to change permissions on ``/etc/gnunet.conf`` so that the
``gnunet`` user can write to the file (during setup).

Afterwards, you need to perform another setup step for each normal user
account from which you want to access GNUnet. First, grant the normal
user (``$USER``) permission to the group gnunet:

.. code-block:: text

   # adduser $USER gnunet

Then, create a configuration file in ``~/.config/gnunet.conf`` for the
$USER with the lines:

.. code-block:: text

   [arm]
   START_SYSTEM_SERVICES = NO
   START_USER_SERVICES = YES

This will ensure that ``gnunet-arm`` when started by the normal user
will only run services that are per-user, and otherwise rely on the
system-wide services. Note that the normal user may run gnunet-setup,
but the configuration would be ineffective as the system-wide services
will use ``/etc/gnunet.conf`` and ignore options set by individual
users.

Again, each user should then start the peer using ``gnunet-arm -s`` ---
and strongly consider adding logic to start the peer automatically to
their crontab.

Afterwards, you should see two (or more, if you have more than one USER)
``gnunet-service-arm`` processes running in your system.

.. _Access-Control-for-GNUnet:

Access Control for GNUnet
~~~~~~~~~~~~~~~~~~~~~~~~~

This chapter documents how we plan to make access control work within
the GNUnet system for a typical peer. It should be read as a
best-practice installation guide for advanced users and builders of
binary distributions. The recommendations in this guide apply to
POSIX-systems with full support for UNIX domain sockets only.

Note that this is an advanced topic. The discussion presumes a very good
understanding of users, groups and file permissions. Normal users on
hosts with just a single user can just install GNUnet under their own
account (and possibly allow the installer to use SUDO to grant
additional permissions for special GNUnet tools that need additional
rights). The discussion below largely applies to installations where
multiple users share a system and to installations where the best
possible security is paramount.

A typical GNUnet system consists of components that fall into four
categories:

User interfaces
   User interfaces are not security sensitive and are supposed to be run
   and used by normal system users. The GTK GUIs and most command-line
   programs fall into this category. Some command-line tools (like
   gnunet-transport) should be excluded as they offer low-level access
   that normal users should not need.

System services and support tools
   System services should always run and offer services that can then be
   accessed by the normal users. System services do not require special
   permissions, but as they are not specific to a particular user, they
   probably should not run as a particular user. Also, there should
   typically only be one GNUnet peer per host. System services include
   the gnunet-service and gnunet-daemon programs; support tools include
   command-line programs such as gnunet-arm.

Privileged helpers
   Some GNUnet components require root rights to open raw sockets or
   perform other special operations. These gnunet-helper binaries are
   typically installed SUID and run from services or daemons.

Critical services
   Some GNUnet services (such as the DNS service) can manipulate the
   service in deep and possibly highly security sensitive ways. For
   example, the DNS service can be used to intercept and alter any DNS
   query originating from the local machine. Access to the APIs of these
   critical services and their privileged helpers must be tightly
   controlled.

.. todo:: Shorten these subsection titles

.. _Recommendation-_002d-Disable-access-to-services-via-TCP:

Recommendation - Disable access to services via TCP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

GNUnet services allow two types of access: via TCP socket or via UNIX
domain socket. If the service is available via TCP, access control can
only be implemented by restricting connections to a particular range of
IP addresses. This is acceptable for non-critical services that are
supposed to be available to all users on the local system or local
network. However, as TCP is generally less efficient and it is rarely
the case that a single GNUnet peer is supposed to serve an entire local
network, the default configuration should disable TCP access to all
GNUnet services on systems with support for UNIX domain sockets. Since
GNUnet 0.9.2, configuration files with TCP access disabled should be
generated by default. Users can re-enable TCP access to particular
services simply by specifying a non-zero port number in the section of
the respective service.

.. _Recommendation-_002d-Run-most-services-as-system-user-_0022gnunet_0022:

Recommendation - Run most services as system user \"gnunet\"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

GNUnet's main services should be run as a separate user \"gnunet\" in a
special group \"gnunet\". The user \"gnunet\" should start the peer
using \"gnunet-arm -s\" during system startup. The home directory for
this user should be ``/var/lib/gnunet`` and the configuration file
should be ``/etc/gnunet.conf``. Only the ``gnunet`` user should have the
right to access ``/var/lib/gnunet`` (*mode: 700*).

.. _Recommendation-_002d-Control-access-to-services-using-group-_0022gnunet_0022:

Recommendation - Control access to services using group \"gnunet\"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Users that should be allowed to use the GNUnet peer should be added to
the group \"gnunet\". Using GNUnet's access control mechanism for UNIX
domain sockets, those services that are considered useful to ordinary
users should be made available by setting \"UNIX_MATCH_GID=YES\" for
those services. Again, as shipped, GNUnet provides reasonable defaults.
Permissions to access the transport and core subsystems might
additionally be granted without necessarily causing security concerns.
Some services, such as DNS, must NOT be made accessible to the
\"gnunet\" group (and should thus only be accessible to the \"gnunet\"
user and services running with this UID).

.. _Recommendation-_002d-Limit-access-to-certain-SUID-binaries-by-group-_0022gnunet_0022:

Recommendation - Limit access to certain SUID binaries by group \"gnunet\"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Most of GNUnet's SUID binaries should be safe even if executed by normal
users. However, it is possible to reduce the risk a little bit more by
making these binaries owned by the group \"gnunet\" and restricting
their execution to user of the group \"gnunet\" as well (4750).

.. _Recommendation-_002d-Limit-access-to-critical-gnunet_002dhelper_002ddns-to-group-_0022gnunetdns_0022:

Recommendation - Limit access to critical gnunet-helper-dns to group \"gnunetdns\"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A special group \"gnunetdns\" should be created for controlling access
to the \"gnunet-helper-dns\". The binary should then be owned by root
and be in group \"gnunetdns\" and be installed SUID and only be
group-executable (2750). **Note that the group \"gnunetdns\" should have
no users in it at all, ever.** The \"gnunet-service-dns\" program should
be executed by user \"gnunet\" (via gnunet-service-arm) with the binary
owned by the user \"root\" and the group \"gnunetdns\" and be SGID
(2700). This way, **only** \"gnunet-service-dns\" can change its group
to \"gnunetdns\" and execute the helper, and the helper can then run as
root (as per SUID). Access to the API offered by \"gnunet-service-dns\"
is in turn restricted to the user \"gnunet\" (not the group!), which
means that only \"benign\" services can manipulate DNS queries using
\"gnunet-service-dns\".

.. _Differences-between-_0022make-install_0022-and-these-recommendations:

Differences between \"make install\" and these recommendations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The current build system does not set all permissions automatically
based on the recommendations above. In particular, it does not use the
group \"gnunet\" at all (so setting gnunet-helpers other than the
gnunet-helper-dns to be owned by group \"gnunet\" must be done
manually). Furthermore, 'make install' will silently fail to set the DNS
binaries to be owned by group \"gnunetdns\" unless that group already
exists (!). An alternative name for the \"gnunetdns\" group can be
specified using the ``--with-gnunetdns=GRPNAME`` configure option.



.. _Configuring-the-hostlist-to-bootstrap:

Configuring the hostlist to bootstrap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After installing the software you need to get connected to the GNUnet
network. The configuration file included in your download is already
configured to connect you to the GNUnet network. In this section the
relevant configuration settings are explained.

To get an initial connection to the GNUnet network and to get to know
peers already connected to the network you can use the so called
\"bootstrap servers\". These servers can give you a list of peers
connected to the network. To use these bootstrap servers you have to
configure the hostlist daemon to activate bootstrapping.

To activate bootstrapping, edit the ``[hostlist]``-section in your
configuration file. You have to set the argument ``-b`` in the options
line:

.. code-block:: text

   [hostlist]
   OPTIONS = -b

Additionally you have to specify which server you want to use. The
default bootstrapping server is \"http://v10.gnunet.org/hostlist\". [^]
To set the server you have to edit the line \"SERVERS\" in the hostlist
section. To use the default server you should set the lines to

.. code-block:: text

   SERVERS = http://v10.gnunet.org/hostlist [^]

To use bootstrapping your configuration file should include these lines:

.. code-block:: text

   [hostlist]
   OPTIONS = -b
   SERVERS = http://v10.gnunet.org/hostlist [^]

Besides using bootstrap servers you can configure your GNUnet peer to
receive hostlist advertisements. Peers offering hostlists to other peers
can send advertisement messages to peers that connect to them. If you
configure your peer to receive these messages, your peer can download
these lists and connect to the peers included. These lists are
persistent, which means that they are saved to your hard disk regularly
and are loaded during startup.

To activate hostlist learning you have to add the ``-e`` switch to the
``OPTIONS`` line in the hostlist section:

.. code-block:: text

   [hostlist]
   OPTIONS = -b -e

Furthermore you can specify in which file the lists are saved. To save
the lists in the file ``hostlists.file`` just add the line:

.. code-block:: text

   HOSTLISTFILE = hostlists.file

Best practice is to activate both bootstrapping and hostlist learning.
So your configuration file should include these lines:

.. code-block:: text

   [hostlist]
   OPTIONS = -b -e
   HTTPPORT = 8080
   SERVERS = http://v10.gnunet.org/hostlist [^]
   HOSTLISTFILE = $SERVICEHOME/hostlists.file


.. _Disable_default_bootstrap:

Disable default bootstrap (private network)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A public node will, by default, connect to a gnunet.org peer to learn
of other peers to bootstrap the network.

To avoid this behavior, either:

- before build, remove the peer entry in ``$REPO/contrib/hellos``

- after build, remove the peer entry in ``$PREFIX/share/gnunet/hellos``

Conversely, any public keys added to the same directories will make the
node *always* make explicit connections to those corresponding peers.

The use of the HELLOs in this folder can be controlled with the configuration
setting ``USE_INCLUDED_HELLOS`` of the ``peerstore`` service:

.. code-block:: text

   $ gnunet-config -s peerstore -o USE_INCLUDED_HELLOS

Note, however, that once the included HELLOs have been parsed, the ``peerstore``
will cache them locally in its databse. To purge included HELLOs in this case,
the database will have to be deleted.

Unless you want to establish a private network, you should not have to touch
this option.

.. _Manually-connecting-peers:

Manually connecting peers
~~~~~~~~~~~~~~~~~~~~~~~~~

A gnunet node will learn peers to connect to from hostlist servers and/or
gossip from connected peers. It will however only connect to a selection
of peers on the network.

If you wish to connect to a specific peer apart from the automatically
negotiated connections, you can use the ``hello`` URI of the peer. The
URI is returned by the following command to *peer to be connected to*:

.. code-block:: text

   $ gnunet-hello --export-hello

The URI output is passed to the ``gnunet-hello`` command of *peer
that is connecting*:

.. code-block:: text

   $ echo "gnunet://hello/..." | gnunet-hello --import-hello


.. _Configuration-of-the-HOSTLIST-proxy-settings-cli:

Configuration of the HOSTLIST proxy settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The hostlist client can be configured to use a proxy to connect to the
hostlist server.

The hostlist client supports the following proxy types at the moment:

-  HTTP and HTTP 1.0 only proxy

-  SOCKS 4/4a/5/5 with hostname

In addition authentication at the proxy with username and password can
be configured.

To provide these options directly in the configuration, you can enter
the following settings in the ``[hostlist]`` section of the
configuration:

.. code-block:: text

   # Type of proxy server,
   # Valid values: HTTP, HTTP_1_0, SOCKS4, SOCKS5, SOCKS4A, SOCKS5_HOSTNAME
   # Default: HTTP
   # PROXY_TYPE = HTTP

   # Hostname or IP of proxy server
   # PROXY =
   # User name for proxy server
   # PROXY_USERNAME =
   # User password for proxy server
   # PROXY_PASSWORD =

.. _Configuring-your-peer-to-provide-a-hostlist:

Configuring your peer to provide a hostlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you operate a peer permanently connected to GNUnet you can configure
your peer to act as a hostlist server, providing other peers the list of
peers known to him.

Your server can act as a bootstrap server and peers needing to obtain a
list of peers can contact it to download this list. To download this
hostlist the peer uses HTTP. For this reason you have to build your peer
with libgnurl (or libcurl) and microhttpd support.

To configure your peer to act as a bootstrap server you have to add the
``-p`` option to ``OPTIONS`` in the ``[hostlist]`` section of your
configuration file. Besides that you have to specify a port number for
the http server. In conclusion you have to add the following lines:

.. code-block:: text

   [hostlist]
   HTTPPORT = 12980
   OPTIONS = -p

If your peer acts as a bootstrap server other peers should know about
that. You can advertise the hostlist your are providing to other peers.
Peers connecting to your peer will get a message containing an
advertisement for your hostlist and the URL where it can be downloaded.
If this peer is in learning mode, it will test the hostlist and, in the
case it can obtain the list successfully, it will save it for
bootstrapping.

To activate hostlist advertisement on your peer, you have to set the
following lines in your configuration file:

.. code-block:: text

   [hostlist]
   EXTERNAL_DNS_NAME = example.org
   HTTPPORT = 12981
   OPTIONS = -p -a

With this configuration your peer will a act as a bootstrap server and
advertise this hostlist to other peers connecting to it. The URL used to
download the list will be ``http://example.org:12981/``.

Please notice:

-  The hostlist is **not** human readable, so you should not try to
   download it using your webbrowser. Just point your GNUnet peer to the
   address!

-  Advertising without providing a hostlist does not make sense and will
   not work.

.. _Configuring-the-datastore:

Configuring the datastore
~~~~~~~~~~~~~~~~~~~~~~~~~

The datastore is what GNUnet uses for long-term storage of file-sharing
data. Note that long-term does not mean 'forever' since content does
have an expiration date, and of course storage space is finite (and
hence sometimes content may have to be discarded).

Use the ``QUOTA`` option to specify how many bytes of storage space you
are willing to dedicate to GNUnet.

In addition to specifying the maximum space GNUnet is allowed to use for
the datastore, you need to specify which database GNUnet should use to
do so. Currently, you have the choice between sqlite and
Postgres.

.. _Configuring-the-Postgres-database:

Configuring the Postgres database
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This text describes how to setup the Postgres database for GNUnet.

This Postgres plugin was developed for Postgres 8.3 but might work for
earlier versions as well.

.. _Reasons-to-use-Postgres:

Reasons to use Postgres
~~~~~~~~~~~~~~~~~~~~~~~

-  Easier to setup than MySQL

-  Real database

.. _Reasons-not-to-use-Postgres:

Reasons not to use Postgres
~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  Quite slow

-  Still some manual setup required

.. _Manual-setup-instructions:

Manual setup instructions
~~~~~~~~~~~~~~~~~~~~~~~~~

-  In ``gnunet.conf`` set in section ``DATASTORE`` the value for
   ``DATABASE`` to ``postgres``.

-  Access Postgres to create a user:

   with Postgres 8.x, use:
      ::

         # su - postgres
         $ createuser

      and enter the name of the user running GNUnet for the role
      interactively. Then, when prompted, do not set it to superuser,
      allow the creation of databases, and do not allow the creation of
      new roles.

   with Postgres 9.x, use:
      ::

         # su - postgres
         $ createuser -d $GNUNET_USER

      where $GNUNET_USER is the name of the user running GNUnet.

-  As that user (so typically as user \"gnunet\"), create a database (or
   two):

   ::

      $ createdb gnunet
      # this way you can run "make check"
      $ createdb gnunetcheck

Now you should be able to start ``gnunet-arm``.

.. _Testing-the-setup-manually:

Testing the setup manually
~~~~~~~~~~~~~~~~~~~~~~~~~~

You may want to try if the database connection works. First, again login
as the user who will run ``gnunet-arm``. Then use:

.. code-block:: psql

   $ psql gnunet # or gnunetcheck
   gnunet=> \dt

If, after you have started ``gnunet-arm`` at least once, you get a
``gn090`` table here, it probably works.

.. _Configuring-the-datacache:

Configuring the datacache
~~~~~~~~~~~~~~~~~~~~~~~~~

The datacache is what GNUnet uses for storing temporary data. This data
is expected to be wiped completely each time GNUnet is restarted (or the
system is rebooted).

You need to specify how many bytes GNUnet is allowed to use for the
datacache using the ``QUOTA`` option in the section ``[dhtcache]``.
Furthermore, you need to specify which database backend should be used
to store the data. Currently, you have the choice between sqLite, MySQL
and Postgres.

.. _Configuring-the-file_002dsharing-service:

Configuring the file-sharing service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to use GNUnet for file-sharing, you first need to make sure
that the file-sharing service is loaded. This is done by setting the
``START_ON_DEMAND`` option in section ``[fs]`` to \"YES\".
Alternatively, you can run

.. code-block:: text

   $ gnunet-arm -i fs

to start the file-sharing service by hand.

Except for configuring the database and the datacache the only important
option for file-sharing is content migration.

Content migration allows your peer to cache content from other peers as
well as send out content stored on your system without explicit
requests. This content replication has positive and negative impacts on
both system performance and privacy.

FIXME: discuss the trade-offs. Here is some older text about it\...

Setting this option to YES allows gnunetd to migrate data to the local
machine. Setting this option to YES is highly recommended for
efficiency. Its also the default. If you set this value to YES, GNUnet
will store content on your machine that you cannot decrypt. While this
may protect you from liability if the judge is sane, it may not (IANAL).
If you put illegal content on your machine yourself, setting this option
to YES will probably increase your chances to get away with it since you
can plausibly deny that you inserted the content. Note that in either
case, your anonymity would have to be broken first (which may be
possible depending on the size of the GNUnet network and the strength of
the adversary).

.. _Configuring-logging:

Configuring logging
~~~~~~~~~~~~~~~~~~~

Since version 0.9.0, logging in GNUnet is controlled via the ``-L`` and
``-l`` options. Using ``-L``, a log level can be specified. With log
level ``ERROR`` only serious errors are logged. The default log level is
``WARNING`` which causes anything of concern to be logged. Log level
``INFO`` can be used to log anything that might be interesting
information whereas ``DEBUG`` can be used by developers to log debugging
messages (but you need to run ``meson setup`` with
``-Dlogging=verbose`` to get them compiled). The ``-l`` option is
used to specify the log file.

Since most GNUnet services are managed by ``gnunet-arm``, using the
``-l`` or ``-L`` options directly is not possible. Instead, they can be
specified using the ``OPTIONS`` configuration value in the respective
section for the respective service. In order to enable logging globally
without editing the ``OPTIONS`` values for each service, ``gnunet-arm``
supports a ``GLOBAL_POSTFIX`` option. The value specified here is given
as an extra option to all services for which the configuration does
contain a service-specific ``OPTIONS`` field.

``GLOBAL_POSTFIX`` can contain the special sequence \"{}\" which is
replaced by the name of the service that is being started. Furthermore,
``GLOBAL_POSTFIX`` is special in that sequences starting with \"$\"
anywhere in the string are expanded (according to options in ``PATHS``);
this expansion otherwise is only happening for filenames and then the
\"$\" must be the first character in the option. Both of these
restrictions do not apply to ``GLOBAL_POSTFIX``. Note that specifying
``%`` anywhere in the ``GLOBAL_POSTFIX`` disables both of these
features.

In summary, in order to get all services to log at level ``INFO`` to
log-files called ``SERVICENAME-logs``, the following global prefix
should be used:

.. code-block:: text

   GLOBAL_POSTFIX = -l $SERVICEHOME/{}-logs -L INFO

.. _Configuring-the-transport-service-and-plugins:

Configuring the transport service and plugins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The transport service in GNUnet is responsible to maintain basic
connectivity to other peers. Besides initiating and keeping connections
alive it is also responsible for address validation.

The GNUnet transport supports more than one transport protocol. These
protocols are configured together with the transport service.

The configuration section for the transport service itself is quite
similar to all the other services

.. code-block:: text

   START_ON_DEMAND = YES
   @UNIXONLY@ PORT = 2091
   HOSTNAME = localhost
   HOME = $SERVICEHOME
   CONFIG = $DEFAULTCONFIG
   BINARY = gnunet-service-transport
   #PREFIX = valgrind
   NEIGHBOUR_LIMIT = 50
   ACCEPT_FROM = 127.0.0.1;
   ACCEPT_FROM6 = ::1;
   PLUGINS = tcp udp
   UNIXPATH = /tmp/gnunet-service-transport.sock

Different are the settings for the plugins to load ``PLUGINS``. The
first setting specifies which transport plugins to load.

-  transport-unix A plugin for local only communication with UNIX domain
   sockets. Used for testing and available on unix systems only. Just
   set the port

   ::

      [transport-unix]
      PORT = 22086
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

-  transport-tcp A plugin for communication with TCP. Set port to 0 for
   client mode with outbound only connections

   ::

      [transport-tcp]
      # Use 0 to ONLY advertise as a peer behind NAT (no port binding)
      PORT = 2086
      ADVERTISED_PORT = 2086
      TESTING_IGNORE_KEYS = ACCEPT_FROM;
      # Maximum number of open TCP connections allowed
      MAX_CONNECTIONS = 128

-  transport-udp A plugin for communication with UDP. Supports peer
   discovery using broadcasts.

   ::

      [transport-udp]
      PORT = 2086
      BROADCAST = YES
      BROADCAST_INTERVAL = 30 s
      MAX_BPS = 1000000
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

-  transport-http HTTP and HTTPS support is split in two part: a client
   plugin initiating outbound connections and a server part accepting
   connections from the client. The client plugin just takes the maximum
   number of connections as an argument.

   ::

      [transport-http_client]
      MAX_CONNECTIONS = 128
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

   ::

      [transport-https_client]
      MAX_CONNECTIONS = 128
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

   The server has a port configured and the maximum number of
   connections. The HTTPS part has two files with the certificate key
   and the certificate file.

   The server plugin supports reverse proxies, so a external hostname
   can be set using the ``EXTERNAL_HOSTNAME`` setting. The webserver
   under this address should forward the request to the peer and the
   configure port.

   ::

      [transport-http_server]
      EXTERNAL_HOSTNAME = fulcrum.net.in.tum.de/gnunet
      PORT = 1080
      MAX_CONNECTIONS = 128
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

   ::

      [transport-https_server]
      PORT = 4433
      CRYPTO_INIT = NORMAL
      KEY_FILE = https.key
      CERT_FILE = https.cert
      MAX_CONNECTIONS = 128
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

-  transport-wlan

   The next section describes how to setup the WLAN plugin, so here only
   the settings. Just specify the interface to use:

   ::

      [transport-wlan]
      # Name of the interface in monitor mode (typically monX)
      INTERFACE = mon0
      # Real hardware, no testing
      TESTMODE = 0
      TESTING_IGNORE_KEYS = ACCEPT_FROM;

.. _Configuring-the-WLAN-transport-plugin:

Configuring the WLAN transport plugin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The wlan transport plugin enables GNUnet to send and to receive data on
a wlan interface. It has not to be connected to a wlan network as long
as sender and receiver are on the same channel. This enables you to get
connection to GNUnet where no internet access is possible, for example
during catastrophes or when censorship cuts you off from the internet.

.. _Requirements-for-the-WLAN-plugin:

Requirements for the WLAN plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  wlan network card with monitor support and packet injection (see
   `aircrack-ng.org <http://www.aircrack-ng.org/>`__)

-  Linux kernel with mac80211 stack, introduced in 2.6.22, tested with
   2.6.35 and 2.6.38

-  Wlantools to create the a monitor interface, tested with airmon-ng of
   the aircrack-ng package

.. _Configuration:

Configuration
^^^^^^^^^^^^^

There are the following options for the wlan plugin (they should be like
this in your default config file, you only need to adjust them if the
values are incorrect for your system)

.. code-block:: text

   # section for the wlan transport plugin
   [transport-wlan]
   # interface to use, more information in the
   # "Before starting GNUnet" section of the handbook.
   INTERFACE = mon0
   # testmode for developers:
   # 0 use wlan interface,
   #1 or 2 use loopback driver for tests 1 = server, 2 = client
   TESTMODE = 0

.. _Before-starting-GNUnet:

Before starting GNUnet
^^^^^^^^^^^^^^^^^^^^^^

Before starting GNUnet, you have to make sure that your wlan interface
is in monitor mode. One way to put the wlan interface into monitor mode
(if your interface name is wlan0) is by executing:

.. code-block:: text

   sudo airmon-ng start wlan0

Here is an example what the result should look like:

.. code-block:: text

   Interface Chipset Driver
   wlan0 Intel 4965 a/b/g/n iwl4965 - [phy0]
   (monitor mode enabled on mon0)

The monitor interface is mon0 is the one that you have to put into the
configuration file.

.. _Limitations-and-known-bugs:

Limitations and known bugs
^^^^^^^^^^^^^^^^^^^^^^^^^^

Wlan speed is at the maximum of 1 Mbit/s because support for choosing
the wlan speed with packet injection was removed in newer kernels.
Please pester the kernel developers about fixing this.

The interface channel depends on the wlan network that the card is
connected to. If no connection has been made since the start of the
computer, it is usually the first channel of the card. Peers will only
find each other and communicate if they are on the same channel.
Channels must be set manually, e.g. by using:

.. code-block:: text

   iwconfig wlan0 channel 1

.. _Configuring-HTTP_0028S_0029-reverse-proxy-functionality-using-Apache-or-nginx:

Configuring HTTP(S) reverse proxy functionality using Apache or nginx
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The HTTP plugin supports data transfer using reverse proxies. A reverse
proxy forwards the HTTP request he receives with a certain URL to
another webserver, here a GNUnet peer.

So if you have a running Apache or nginx webserver you can configure it
to be a GNUnet reverse proxy. Especially if you have a well-known
website this improves censorship resistance since it looks as normal
surfing behaviour.

To do so, you have to do two things:

-  Configure your webserver to forward the GNUnet HTTP traffic

-  Configure your GNUnet peer to announce the respective address

As an example we want to use GNUnet peer running:

-  HTTP server plugin on ``gnunet.foo.org:1080``

-  HTTPS server plugin on ``gnunet.foo.org:4433``

-  A apache or nginx webserver on
   `http://www.foo.org:80/ <http://www.foo.org/>`__

-  A apache or nginx webserver on https://www.foo.org:443/

And we want the webserver to accept GNUnet traffic under
``http://www.foo.org/bar/``. The required steps are described here:

.. _Reverse-Proxy-_002d-Configure-your-Apache2-HTTP-webserver:

Reverse Proxy - Configure your Apache2 HTTP webserver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

First of all you need mod_proxy installed.

Edit your webserver configuration. Edit ``/etc/apache2/apache2.conf`` or
the site-specific configuration file.

In the respective ``server config``,\ ``virtual host`` or ``directory``
section add the following lines:

.. code-block:: text

   ProxyTimeout 300
   ProxyRequests Off
   <Location /bar/ >
   ProxyPass http://gnunet.foo.org:1080/
   ProxyPassReverse http://gnunet.foo.org:1080/
   </Location>

.. _Reverse-Proxy-_002d-Configure-your-Apache2-HTTPS-webserver:

Reverse Proxy - Configure your Apache2 HTTPS webserver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We assume that you already have an HTTPS server running, if not please
check how to configure a HTTPS host. An uncomplicated to use example is
the example configuration file for Apache2/HTTPD provided in
``apache2/sites-available/default-ssl``.

In the respective HTTPS ``server config``,\ ``virtual host`` or
``directory`` section add the following lines:

.. code-block:: text

   SSLProxyEngine On
   ProxyTimeout 300
   ProxyRequests Off
   <Location /bar/ >
   ProxyPass https://gnunet.foo.org:4433/
   ProxyPassReverse https://gnunet.foo.org:4433/
   </Location>

More information about the apache mod_proxy configuration can be found
in the `Apache
documentation <http://httpd.apache.org/docs/2.2/mod/mod_proxy.html#proxypass>`__.

.. _Reverse-Proxy-_002d-Configure-your-nginx-HTTPS-webserver:

Reverse Proxy - Configure your nginx HTTPS webserver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since nginx does not support chunked encoding, you first of all have to
install the ``chunkin``
`module <http://wiki.nginx.org/HttpChunkinModule>`__.

To enable chunkin add:

.. code-block:: nginx

   chunkin on;
   error_page 411 = @my_411_error;
   location @my_411_error {
   chunkin_resume;
   }

Edit your webserver configuration. Edit ``/etc/nginx/nginx.conf`` or the
site-specific configuration file.

In the ``server`` section add:

.. code-block:: nginx

   location /bar/ {
   proxy_pass http://gnunet.foo.org:1080/;
   proxy_buffering off;
   proxy_connect_timeout 5; # more than http_server
   proxy_read_timeout 350; # 60 default, 300s is GNUnet's idle timeout
   proxy_http_version 1.1; # 1.0 default
   proxy_next_upstream error timeout invalid_header http_500 http_503 http_502 http_504;
   }

.. _Reverse-Proxy-_002d-Configure-your-nginx-HTTP-webserver:

Reverse Proxy - Configure your nginx HTTP webserver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Edit your webserver configuration. Edit ``/etc/nginx/nginx.conf`` or the
site-specific configuration file.

In the ``server`` section add:

.. code-block:: nginx

   ssl_session_timeout 6m;
   location /bar/
   {
   proxy_pass https://gnunet.foo.org:4433/;
   proxy_buffering off;
   proxy_connect_timeout 5; # more than http_server
   proxy_read_timeout 350; # 60 default, 300s is GNUnet's idle timeout
   proxy_http_version 1.1; # 1.0 default
   proxy_next_upstream error timeout invalid_header http_500 http_503 http_502 http_504;
   }

.. _Reverse-Proxy-_002d-Configure-your-GNUnet-peer:

Reverse Proxy - Configure your GNUnet peer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To have your GNUnet peer announce the address, you have to specify the
``EXTERNAL_HOSTNAME`` option in the ``[transport-http_server]`` section:

.. code-block:: text

   [transport-http_server]
   EXTERNAL_HOSTNAME = http://www.foo.org/bar/

and/or ``[transport-https_server]`` section:

.. code-block:: text

   [transport-https_server]
   EXTERNAL_HOSTNAME = https://www.foo.org/bar/

Now restart your webserver and your peer\...

.. _Blacklisting-peers:

Blacklisting peers
~~~~~~~~~~~~~~~~~~

Transport service supports to deny connecting to a specific peer of to a
specific peer with a specific transport plugin using the blacklisting
component of transport service. With blacklisting it is possible to deny
connections to specific peers of to use a specific plugin to a specific
peer. Peers can be blacklisted using the configuration or a blacklist
client can be asked.

To blacklist peers using the configuration you have to add a section to
your configuration containing the peer id of the peer to blacklist and
the plugin if required.

Examples:

To blacklist connections to P565\... on peer AG2P\... using tcp add:

.. todo:: too long?
.. todo:: verify whether these still produce errors in pdf output

.. code-block:: text

   [transport-blacklist AG2PHES1BARB9IJCPAMJTFPVJ5V3A72S3F2A8SBUB8DAQ2V0O3V8G6G2JU56FHGFOHMQVKBSQFV98TCGTC3RJ1NINP82G0RC00N1520]
   P565723JO1C2HSN6J29TAQ22MN6CI8HTMUU55T0FUQG4CMDGGEQ8UCNBKUMB94GC8R9G4FB2SF9LDOBAJ6AMINBP4JHHDD6L7VD801G = tcp

To blacklist connections to P565\... on peer AG2P\... using all plugins
add:

.. code-block:: text

   [transport-blacklist-AG2PHES1BARB9IJCPAMJTFPVJ5V3A72S3F2A8SBUB8DAQ2V0O3V8G6G2JU56FHGFOHMQVKBSQFV98TCGTC3RJ1NINP82G0RC00N1520]
   P565723JO1C2HSN6J29TAQ22MN6CI8HTMUU55T0FUQG4CMDGGEQ8UCNBKUMB94GC8R9G4FB2SF9LDOBAJ6AMINBP4JHHDD6L7VD801G =

You can also add a blacklist client using the blacklist API. On a
blacklist check, blacklisting first checks internally if the peer is
blacklisted and if not, it asks the blacklisting clients. Clients are
asked if it is OK to connect to a peer ID, the plugin is omitted.

On blacklist check for (peer, plugin)

-  Do we have a local blacklist entry for this peer and this plugin?

-  YES: disallow connection

-  Do we have a local blacklist entry for this peer and all plugins?

-  YES: disallow connection

-  Does one of the clients disallow?

-  YES: disallow connection

.. _Configuration-of-the-HTTP-and-HTTPS-transport-plugins-cli:

Configuration of the HTTP and HTTPS transport plugins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The client parts of the http and https transport plugins can be
configured to use a proxy to connect to the hostlist server.

Both the HTTP and HTTPS clients support the following proxy types at the
moment:

-  HTTP 1.1 proxy

-  SOCKS 4/4a/5/5 with hostname

In addition authentication at the proxy with username and password can
be configured.

To configure these options directly in the configuration, you can
configure the following settings in the ``[transport-http_client]`` and
``[transport-https_client]`` section of the configuration:

.. code-block:: text

   # Type of proxy server,
   # Valid values: HTTP, SOCKS4, SOCKS5, SOCKS4A, SOCKS5_HOSTNAME
   # Default: HTTP
   # PROXY_TYPE = HTTP

   # Hostname or IP of proxy server
   # PROXY =
   # User name for proxy server
   # PROXY_USERNAME =
   # User password for proxy server
   # PROXY_PASSWORD =

.. _Configuring-the-GNUnet-VPN:

Configuring the GNUnet VPN
~~~~~~~~~~~~~~~~~~~~~~~~~~

Before configuring the GNUnet VPN, please make sure that system-wide DNS
interception is configured properly as described in the section on the
GNUnet DNS setup. see `Configuring the GNU Name
System <#Configuring-the-GNU-Name-System>`__, if you haven't done so
already.

The default options for the GNUnet VPN are usually sufficient to use
GNUnet as a Layer 2 for your Internet connection. However, what you
always have to specify is which IP protocol you want to tunnel: IPv4,
IPv6 or both. Furthermore, if you tunnel both, you most likely should
also tunnel all of your DNS requests. You theoretically can tunnel
\"only\" your DNS traffic, but that usually makes little sense.

The other options as shown on the gnunet-setup tool are:

.. _IPv4-address-for-interface:

IPv4 address for interface
^^^^^^^^^^^^^^^^^^^^^^^^^^

This is the IPv4 address the VPN interface will get. You should pick a
'private' IPv4 network that is not yet in use for you system. For
example, if you use ``10.0.0.1/255.255.0.0`` already, you might use
``10.1.0.1/255.255.0.0``. If you use ``10.0.0.1/255.0.0.0`` already,
then you might use ``192.168.0.1/255.255.0.0``. If your system is not in
a private IP-network, using any of the above will work fine. You should
try to make the mask of the address big enough (``255.255.0.0`` or, even
better, ``255.0.0.0``) to allow more mappings of remote IP Addresses
into this range. However, even a ``255.255.255.0`` mask will suffice for
most users.

.. _IPv6-address-for-interface:

IPv6 address for interface
^^^^^^^^^^^^^^^^^^^^^^^^^^

The IPv6 address the VPN interface will get. Here you can specify any
non-link-local address (the address should not begin with ``fe80:``). A
subnet Unique Local Unicast (``fd00::/8`` prefix) that you are currently
not using would be a good choice.

.. _Configuring-the-GNUnet-VPN-DNS:

Configuring the GNUnet VPN DNS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To resolve names for remote nodes, activate the DNS exit option.

.. _Configuring-the-GNUnet-VPN-Exit-Service:

Configuring the GNUnet VPN Exit Service
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to allow other users to share your Internet connection (yes,
this may be dangerous, just as running a Tor exit node) or want to
provide access to services on your host (this should be less dangerous,
as long as those services are secure), you have to enable the GNUnet
exit daemon.

You then get to specify which exit functions you want to provide. By
enabling the exit daemon, you will always automatically provide exit
functions for manually configured local services (this component of the
system is under development and not documented further at this time). As
for those services you explicitly specify the target IP address and
port, there is no significant security risk in doing so.

Furthermore, you can serve as a DNS, IPv4 or IPv6 exit to the Internet.
Being a DNS exit is usually pretty harmless. However, enabling IPv4 or
IPv6-exit without further precautions may enable adversaries to access
your local network, send spam, attack other systems from your Internet
connection and do other mischiefs that will appear to come from your
machine. This may or may not get you into legal trouble. If you want to
allow IPv4 or IPv6-exit functionality, you should strongly consider
adding additional firewall rules manually to protect your local network
and to restrict outgoing TCP traffic (e.g. by not allowing access to
port 25). While we plan to improve exit-filtering in the future, you're
currently on your own here. Essentially, be prepared for any kind of
IP-traffic to exit the respective TUN interface (and GNUnet will enable
IP-forwarding and NAT for the interface automatically).

Additional configuration options of the exit as shown by the
gnunet-setup tool are:

.. _IP-Address-of-external-DNS-resolver:

IP Address of external DNS resolver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If DNS traffic is to exit your machine, it will be send to this DNS
resolver. You can specify an IPv4 or IPv6 address.

.. _IPv4-address-for-Exit-interface:

IPv4 address for Exit interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is the IPv4 address the Interface will get. Make the mask of the
address big enough (255.255.0.0 or, even better, 255.0.0.0) to allow
more mappings of IP addresses into this range. As for the VPN interface,
any unused, private IPv4 address range will do.

.. _IPv6-address-for-Exit-interface:

IPv6 address for Exit interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The public IPv6 address the interface will get. If your kernel is not a
very recent kernel and you are willing to manually enable IPv6-NAT, the
IPv6 address you specify here must be a globally routed IPv6 address of
your host.

Suppose your host has the address ``2001:4ca0::1234/64``, then using
``2001:4ca0::1:0/112`` would be fine (keep the first 64 bits, then
change at least one bit in the range before the bitmask, in the example
above we changed bit 111 from 0 to 1).

You may also have to configure your router to route traffic for the
entire subnet (``2001:4ca0::1:0/112`` for example) through your computer
(this should be automatic with IPv6, but obviously anything can be
disabled).

.. _Bandwidth-Configuration:

Bandwidth Configuration
~~~~~~~~~~~~~~~~~~~~~~~

You can specify how many bandwidth GNUnet is allowed to use to receive
and send data. This is important for users with limited bandwidth or
traffic volume.

.. _Configuring-NAT:

Configuring NAT
~~~~~~~~~~~~~~~

Most hosts today do not have a normal global IP address but instead are
behind a router performing Network Address Translation (NAT) which
assigns each host in the local network a private IP address. As a
result, these machines cannot trivially receive inbound connections from
the Internet. GNUnet supports NAT traversal to enable these machines to
receive incoming connections from other peers despite their limitations.

In an ideal world, you can press the \"Attempt automatic configuration\"
button in gnunet-setup to automatically configure your peer correctly.
Alternatively, your distribution might have already triggered this
automatic configuration during the installation process. However,
automatic configuration can fail to determine the optimal settings,
resulting in your peer either not receiving as many connections as
possible, or in the worst case it not connecting to the network at all.

To manually configure the peer, you need to know a few things about your
network setup. First, determine if you are behind a NAT in the first
place. This is always the case if your IP address starts with \"10.*\"
or \"192.168.*\". Next, if you have control over your NAT router, you
may choose to manually configure it to allow GNUnet traffic to your
host. If you have configured your NAT to forward traffic on ports 2086
(and possibly 1080) to your host, you can check the \"NAT ports have
been opened manually\" option, which corresponds to the \"PUNCHED_NAT\"
option in the configuration file. If you did not punch your NAT box, it
may still be configured to support UPnP, which allows GNUnet to
automatically configure it. In that case, you need to install the
\"upnpc\" command, enable UPnP (or PMP) on your NAT box and set the
\"Enable NAT traversal via UPnP or PMP\" option (corresponding to
\"ENABLE_UPNP\" in the configuration file).

Some NAT boxes can be traversed using the autonomous NAT traversal
method. This requires certain GNUnet components to be installed with
\"SUID\" privileges on your system (so if you're installing on a system
you do not have administrative rights to, this will not work). If you
installed as 'root', you can enable autonomous NAT traversal by checking
the \"Enable NAT traversal using ICMP method\". The ICMP method requires
a way to determine your NAT's external (global) IP address. This can be
done using either UPnP, DynDNS, or by manual configuration. If you have
a DynDNS name or know your external IP address, you should enter that
name under \"External (public) IPv4 address\" (which corresponds to the
\"EXTERNAL_ADDRESS\" option in the configuration file). If you leave the
option empty, GNUnet will try to determine your external IP address
automatically (which may fail, in which case autonomous NAT traversal
will then not work).

Finally, if you yourself are not behind NAT but want to be able to
connect to NATed peers using autonomous NAT traversal, you need to check
the \"Enable connecting to NATed peers using ICMP method\" box.

.. _Peer-configuration-for-distributors-_0028e_002eg_002e-Operating-Systems_0029:

Peer configuration for distributors (e.g. Operating Systems)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The \"GNUNET_DATA_HOME\" in \"[PATHS]\" in ``/etc/gnunet.conf`` should
be manually set to \"/var/lib/gnunet/data/\" as the default
\"~/.local/share/gnunet/\" is probably not that appropriate in this
case. Similarly, distributors may consider pointing
\"GNUNET_RUNTIME_DIR\" to \"/var/run/gnunet/\" and \"GNUNET_HOME\" to
\"/var/lib/gnunet/\". Also, should a distributor decide to override
system defaults, all of these changes should be done in a custom
``/etc/gnunet.conf`` and not in the files in the ``config.d/``
directory.

Given the proposed access permissions, the \"gnunet-setup\" tool must be
run as use \"gnunet\" (and with option \"-c /etc/gnunet.conf\" so that
it modifies the system configuration). As always, gnunet-setup should be
run after the GNUnet peer was stopped using \"gnunet-arm -e\".
Distributors might want to include a wrapper for gnunet-setup that
allows the desktop-user to \"sudo\" (e.g. using gtksudo) to the
\"gnunet\" user account and then runs \"gnunet-arm -e\",
\"gnunet-setup\" and \"gnunet-arm -s\" in sequence.
