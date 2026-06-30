Installation
============

This guide is intended for those who want to install GNUnet from source.
For instructions on how to install GNUnet as a binary package please
refer to the official documentation of your operating system or package
manager.

For understanding this guide properly it is important to know that there
are two different ways of running GNUnet:

-  the single-user setup
-  the multi-user setup

The latter variant has a better security model and requires extra
preparation before running make install and a different configuration.
Beginners who want to quickly try out GNUnet can use the single-user
setup.

Dependencies
------------

GNUnet needs few libraries and applications for being able to run and
another few optional ones for using certain features. Preferably they
should be installed with a package manager.

The mandatory libraries and applications are

-  meson 1.0 or above
-  recutils 1.0 or above (when building from git)
-  gettext
-  glibc (read below, other libcs work)
-  GnuTLS 3.2.12 or above, recommended to be linked against libunbound
-  GNU make 4.0 or higher (other make implementations do work)
-  iptables (on Linux systems)
-  libtool 2.2 or above
-  libltdl (part of libtool)
-  libgcrypt 1.6 or above
-  libidn2 or libidn
-  libmicrohttpd 0.9.63 or above
-  libunistring
-  libjansson
-  libjose (optional, for reclaimID)
-  libgmp
-  libcurl (ideally linked to GnuTLS) 7.35.0 or
   above
-  Texinfo 5.2 or above (for building the documentation)
-  Texlive 2012 or above (for building the documentation, and for
   gnunet-bcd)
-  makeinfo 4.8 or above
-  pkgconf (or pkg-config)
-  zlib

Glibc is required for certain NSS features:

.. code-block:: text

  One mechanism of integrating GNS with legacy applications via NSS is
  not available if this is disabled. But applications that don't use the
  glibc for NS resolution won't work anyway with this, so little is lost
  on *BSD systems.
  GNS via direct use or via the HTTP or DNS proxies is unaffected.

Other libcs should work, the resulting builds just don’t include the
glibc NSS specific code. One example is the build against NetBSD’s libc
as detailed in https://bugs.gnunet.org/view.php?id=5605.

In addition GNUnet needs at least one of these three databases (at the
minimum sqlite3)

-  sqlite + libsqlite 3.8 or above (the default, requires no further
   configuration)
-  postgres + libpq
-  mysql + libmysqlclient

These are the dependencies only required for certain features

-  miniupnpc (for traversing NAT boxes more reliably)
-  libnss
-  libopus (for running the GNUnet conversation telephony application)
-  libogg (for running the GNUnet conversation telephony application)
-  gstreamer OR libpulse (for running the GNUnet conversation telephony
   application)
-  bluez (for bluetooth support)
-  libextractor (optional but highly recommended, read below)
-  texi2mdoc (for automatic mdoc generation)
-  perl5 for some utilities (which are not installed)

About libextractor being optional:

.. code-block:: text

   While libextractor ("LE") is optional, it is recommended to build gnunet
   against it. If you install it later, you won't benefit from libextractor.
   If you are a distributor, we recommend to split LE into basis + plugins
   rather than making LE an option as an afterthought by the user.  LE
   itself is very small, but its dependency chain on first, second, third
   etc level can be big.  There is a small effect on privacy if your LE
   build differs from one which includes all plugins (plugins are build as
   shared objects): if users publish a directory with a mixture of file
   types (for example mpeg, jpeg, png, gif) the configuration of LE could
   leak which plugins are installed for which filetypes are not providing
   more details.  However, this leak is just a minor concern.

These are the test-suite requirements:

-  python3.6 or higher
-  gnunet (installation first)
-  some core-utils: which(1), bc(1), curl(1), sed(1), awk(1), etc.
-  a shell (very few Bash scripts, the majority are POSIX sh scripts)

These are runtime requirements:

-  nss (the certutil binary, for gnunet-gns-proxy-setup-ca)
-  openssl (openssl binary, for gnunet-gns-proxy-setup-ca)

Getting the Source Code
-----------------------

You can either download the source code using git (you obviously need
git installed) or as an archive.

Using git type

::

   $ git clone git://git.gnunet.org/gnunet.git

The release archive can be found at https://ftpmirror.gnu.org/gnu/gnunet/.
You can find nightly builds at https://buildbot.gnunet.org/artifacts/.
Extract it using a graphical archive tool or tar:

::

   tar xf gnunet-<VERSION> .tar.gz

In the next chapter we will assume that the source code is available in
the home directory at ~/gnunet.

Create user and groups for the system services
----------------------------------------------

**For single-user setup this section can be skipped**

The multi-user setup means that there are system services, which are run
once per machine as a dedicated system user (called gnunet) and user
services which can be started by every user who wants to use GNUnet
applications. The user services communicate with the system services
over unix domain sockets. To gain permissions to read and write those
sockets the users running GNUnet applications will need to be in the
gnunet group. In addition the group gnunetdns may be needed (see below).

Create user gnunet who is member of the group gnunet (automatically
created) and specify a home directory where the GNUnet services will
store persistent data such as information about peers.

::

   $ sudo useradd --system --home-dir /var/lib/gnunet --create-home gnunet

Now add your own user to the gnunet group:

::

   $ sudo usermod -aG gnunet <user>

Create a group gnunetdns. This allows using setgid in a way that only
the DNS service can run the gnunet-helper-dns binary. This is only
needed if system-wide DNS interception will be used. For more
information see `Configuring system-wide DNS
interception <installing#configuring%20system-wide-dns-interception>`__.

::

   $ sudo groupadd gnunetdns

Preparing and Compiling the Source Code
---------------------------------------

Get the source code from git or a source tarball. When running
`meson setup`, options can be specified to customize the compilation and
installation process. For details execute:

::

   $ meson setup --help

The following example configures the installation prefix `/usr/local`
and creates the out-of-tree build folder `build`.

::

   $ ./bootstrap (only when installing from git)
   $ meson setup -Dprefix=/usr/local build

After running the bootstrap script and setup successfully the source
code can be compiled and the compiled binaries can be installed using:

::

   $ meson compile -C build
   $ meson install -C build

The latter command may need to be run as root (or with sudo) because
some binaries need the suid bit set. Without that some features
(e.g. the VPN service, system-wide DNS interception, NAT traversal using
ICMP) will not work.

NSS plugin (optional)
~~~~~~~~~~~~~~~~~~~~~

**NOTE: The installation of the NSS plugin is only necessary if GNS
resolution shall be used with legacy applications (that only support
DNS) and if you cannot do not want to use the DNS2GNS service.**

One important library is the GNS plugin for NSS (the name services
switch) which allows using GNS (the GNU name system) in the normal DNS
resolution process. Unfortunately NSS expects it in a specific location
(probably ``/lib``) which may differ from the installation prefix (see
``--prefix`` option in the previous section). This is why the plugin has
to be installed manually.

Find the directory where nss plugins are installed on your system, e.g.:

::

   $ ls -l /lib/libnss_*
   /lib/libnss_mymachines.so.2
   /lib/libnss_resolve.so.2
   /lib/libnss_myhostname.so.2
   /lib/libnss_systemd.so.2

Copy the GNS NSS plugin to that directory:

::

   cp ~/gnunet/src/gns/nss/.libs/libnss_gns.so.2 /lib

Installing the GNS Certificate Authority (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**NOTE: Installing the GNS certificate authority is only necessary if
GNS shall be used in a browser and if you cannot or do not want to use
the DNS2GNS service.**

The GNS Certificate authority can provide TLS certificates for GNS names
while downloading webpages from legacy webservers. This allows browsers
to use HTTPS in combinations with GNS name resolution.

To install it execute the GNS CA-setup script. So far Firefox and
Chromium are supported.

::

   $ gnunet-gns-proxy-setup-ca

A local proxy server, that takes care of the name resolution and
provides certificates on-the-fly needs to be started:

::

   $ /usr/lib/gnunet/libexec/gnunet-gns-proxy

Now GNS should work in browsers that are configured to use a SOCKS proxy
on localhost:7777.


.. _Minimal-configuration:

Minimal configuration
---------------------

GNUnet needs a configuration file to start (see `Config file
format <#Config-file-format>`__). For the *single-user setup* an empty
file is sufficient:

::

   $ touch ~/.config/gnunet.conf

For the *multi-user setup* we need an extra config file for the system
services. The default location is ``/etc/gnunet.conf``. The minimal
content of that file which activates the system services roll is:

::

   [arm]
   START_SYSTEM_SERVICES = YES
   START_USER_SERVICES = NO

The config file for the user services (``~/.config/gnunet.conf``) needs
the opposite configuration to activate the user services roll:

::

   [arm]
   START_SYSTEM_SERVICES = NO
   START_USER_SERVICES = YES
