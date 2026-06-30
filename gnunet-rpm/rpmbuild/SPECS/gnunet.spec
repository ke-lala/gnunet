%global gnunetuser gnunet
%global gnunethome %{_sharedstatedir}/%{name}

Name:           gnunet
Version:        0.26.1
Release:        1%{?dist}
Summary:        Framework for secure peer-to-peer networking
License:        AGPL-3.0-or-later
Group:          Productivity/Networking/File-Sharing
URL:            https://%{name}.org
%global         _disable_source_fetch 0
Source0:        https://ftpmirror.gnu.org/gnunet//%{name}-%{version}.tar.gz
Source1:        https://ftpmirror.gnu.org/gnunet//%{name}-%{version}.tar.gz.sig
Source2:        https://www.gnunet.org/~schanzen/schanzen.asc
%global         SHA256SUM0 edd293be045649ac06227b019d5105cac677fc7caacafc951444c15cb2ba4c45
BuildRequires:  libtool
BuildRequires:  libtool-ltdl
BuildRequires:  libtool-ltdl-devel
BuildRequires:  meson
BuildRequires:  make
BuildRequires:  gettext
BuildRequires:  gcc
BuildRequires:  openssl
BuildRequires:  libatomic
BuildRequires:  libgcrypt-devel
BuildRequires:  libunistring-devel
BuildRequires:  miniupnpc
BuildRequires:  libidn2-devel
BuildRequires:  zlib-devel
BuildRequires:  jansson-devel
BuildRequires:  gnutls-devel
BuildRequires:  libmicrohttpd-devel
BuildRequires:  zbar-devel
BuildRequires:  gnutls-dane
BuildRequires:  glibc
BuildRequires:  pkgconf
BuildRequires:  libgsf
BuildRequires:  pkgconfig
BuildRequires:  libextractor-devel
BuildRequires:  python3
BuildRequires:  libcurl-devel
BuildRequires:  dbus
BuildRequires:  libsodium-devel
BuildRequires:  sqlite-devel
BuildRequires:  gnupg2
BuildRequires:  libjose-devel
BuildRequires: systemd-rpm-macros
Requires:       openssl
Requires:       nss-util
Requires:       gnutls
Requires:       curl
Requires:       net-tools
Requires:       iptables
Requires:       miniupnpc
Requires:       dbus
Requires:       authselect
Requires:       lib%{name}%{?_isa} = %{version}-%{release}

%description
GNUnet is peer-to-peer framework providing a network abstractions and
applications focusing on security and privacy.  So far, we have
created applications for anonymous file-sharing, decentralized naming
and identity management, decentralized and confidential telephony and
tunneling IP traffic over GNUnet.  GNUnet is currently developed by a
worldwide group of independent free software developers.  GNUnet is a
GNU package (http://www.gnu.org/).

Additional documentation about GNUnet can be found at
https://gnunet.org/ and in the 'doc/' folder.
Online documentation is provided at
'https://docs.gnunet.org'.

%package -n libgnunet
Summary: The GNUnet base libraries
%description -n libgnunet
The GNUnet base libraries.

%package -n libgnunet-devel
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Summary: The GNUnet base libraries development files
%description -n libgnunet-devel
The GNUnet base libraries development files.

%package doc
Requires: %{name}%{?_isa} = %{version}-%{release}
BuildRequires: mandoc doxygen texinfo python3-sphinx_rtd_theme
BuildArchitectures: noarch
Summary: The GNUnet documentation
%description doc
The GNUnet documentation.

# gnunet-bcd
%package bcd
BuildRequires: texlive-scheme-medium
Requires: texlive-scheme-medium
Requires: %{name}%{?_isa} = %{version}-%{release}
Summary: The GNUnet GNS business card generator
%description bcd
The GNUnet GNS business card generator to generate business cards with
QR codes of zone keys.

%package devel
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Requires: lib%{name}-devel = %{version}-%{release}
Summary: GNUnet development headers
%description devel
Files needed to develop GNUnet applications and services.

# gnunet-postgresql-plugins
%package -n libgnunetpq
Requires: libgnunet%{?_isa} = %{version}-%{release}
BuildRequires:  libpq-devel
Recommends: postgresql-server
Summary: The GNUnet PostgreSQL library
%description -n libgnunetpq
The GNUnet PostgreSQL utility library.

# gnunet-postgresql-plugins
%package postgresql-plugins
Requires: %{name}%{?_isa} = %{version}-%{release}
Recommends: postgresql-server
Summary: The GNUnet PostgreSQL plugins
%description postgresql-plugins
The GNUnet PostgreSQL plugins for the datacache, datastore, namecache
and namestore components.

%package -n libgnunetpq-devel
Requires: libgnunet-devel%{?_isa} = %{version}-%{release}
Requires: libgnunetpq%{?_isa} = %{version}-%{release}
Summary: The GNUnet PostgreSQL development files
%description -n libgnunetpq-devel
The GNUnet PostgreSQL plugins development files.


# gnunet-conversation functionality
%package conversation
Requires: %{name}%{?_isa} = %{version}-%{release}
BuildRequires:  opus-devel
BuildRequires:  pulseaudio-libs
BuildRequires:  libogg-devel
#Conversation requires gstreamer-app-1.0 and gstreamer-audio-1.0
BuildRequires:  gstreamer1-plugins-base-devel
Summary: The GNUnet CONVERSATION VoP2P component
%description conversation
The GNUnet CONVERSATION component. Allows you to make Voice-over-Peer-to-Peer
audio calls.

%package conversation-devel
Requires: conversation
Requires: %{name}-devel%{?_isa} = %{version}-%{release}
Requires: %{name}%{?_isa} = %{version}-%{release}
#Conversation requires gstreamer-app-1.0 and gstreamer-audio-1.0
Summary: The GNUnet CONVERSATION VoP2P component development files
%description conversation-devel
The GNUnet CONVERSATION VoP2P component development files.

%prep
echo "%SHA256SUM0 %SOURCE0" | sha256sum -c -
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'

%setup -q

%build
%meson -Dinstall-rpath=false
%meson_build

%pre
getent group %{name}dns >/dev/null || /usr/sbin/groupadd -r %{name}
getent group %{name}dns >/dev/null || /usr/sbin/groupadd -r %{name}dns
getent passwd %{name} >/dev/null || useradd -r -g %{name} -G %{name}dns -m -d %{gnunethome} %{gnunetuser} -c "GNUnet system account"
exit 0

%install
%meson_install
%find_lang %{name}
install -Dm644 "contrib/packages/fedora/%{name}.service" "%{buildroot}/%{_unitdir}/%{name}.service"
install -Dm644 "contrib/packages/fedora/%{name}-user.service" "%{buildroot}/%{_userunitdir}/%{name}-user.service"
install -Dm644 "contrib/packages/fedora/gnunet-system.conf" %{buildroot}/%{_sysconfdir}/gnunet.conf
# NOTE: Let's not do that for now automatically, because systemd is, well, special
# https://github.com/systemd/systemd/issues/25020
# https://github.com/systemd/systemd/issues/5755
# https://github.com/systemd/systemd/issues/21174
#mkdir -p %%{buildroot}/%%{_sysconfdir}/NetworkManager/dispatcher.d
#install -Dm755 "contrib/packages/fedora/10-dns2gns.sh" %%{buildroot}/%%{_sysconfdir}/NetworkManager/dispatcher.d/
# Remove the dir file like, e.g., bash.
rm -f %{buildroot}%{_infodir}/dir

%check
%meson_test --suite util --timeout-multiplier 0

%post
%systemd_post %{name}.service
%systemd_user_post %{name}-user.service

%preun
%systemd_preun %{name}.service

%postun
%systemd_postun_with_restart %{name}.service
%systemd_user_post %{name}-user.service

%files -n libgnunet-devel
%{_libdir}/libgnunetsq.so
%{_libdir}/libgnunetcurl.so
%{_libdir}/libgnunetabd.so
%{_libdir}/libgnunetarm.so
%{_libdir}/libgnunetjson.so
%{_libdir}/libgnunetrest.so
%{_libdir}/libgnunetutil.so
%{_libdir}/pkgconfig/gnunetabd.pc
%{_libdir}/pkgconfig/gnunetarm.pc
%{_datadir}/%{name}/testing_hostkeys.ecc
%dir %{_includedir}/%{name}
%{_includedir}/%{name}/gnunet_util_lib.h
%{_includedir}/%{name}/gnunet_json_lib.h
%{_includedir}/%{name}/gnunet_load_lib.h
%{_includedir}/%{name}/compat.h
%{_includedir}/%{name}/gettext.h
%{_includedir}/%{name}/gnunet_constants.h
%{_includedir}/%{name}/gnunet_applications.h
%{_includedir}/%{name}/gnunet_arm_service.h
%{_includedir}/%{name}/gnunet_time_lib.h
%{_includedir}/%{name}/gnunet_uri_lib.h
%{_includedir}/%{name}/gnunet_sq_lib.h
%{_includedir}/%{name}/gnunet_network_lib.h
%{_includedir}/%{name}/gnunet_disk_lib.h
%{_includedir}/%{name}/gnunet_config.h
%{_includedir}/%{name}/gnunet_common.h
%{_includedir}/%{name}/gnunet_crypto_lib.h
%{_includedir}/%{name}/gnunet_bandwidth_lib.h
%{_includedir}/%{name}/gnunet_bio_lib.h
%{_includedir}/%{name}/gnunet_buffer_lib.h
%{_includedir}/%{name}/gnunet_client_lib.h
%{_includedir}/%{name}/gnunet_configuration_lib.h
%{_includedir}/%{name}/gnunet_container_lib.h
%{_includedir}/%{name}/gnunet_getopt_lib.h
%{_includedir}/%{name}/gnunet_helper_lib.h
%{_includedir}/%{name}/gnunet_mhd_compat.h
%{_includedir}/%{name}/gnunet_mst_lib.h
%{_includedir}/%{name}/gnunet_mq_lib.h
%{_includedir}/%{name}/gnunet_nc_lib.h
%{_includedir}/%{name}/gnunet_op_lib.h
%{_includedir}/%{name}/gnunet_os_lib.h
%{_includedir}/%{name}/gnunet_peer_lib.h
%{_includedir}/%{name}/gnunet_plugin_lib.h
%{_includedir}/%{name}/gnunet_program_lib.h
%{_includedir}/%{name}/gnunet_protocols.h
%{_includedir}/%{name}/gnunet_service_lib.h
%{_includedir}/%{name}/gnunet_signal_lib.h
%{_includedir}/%{name}/gnunet_strings_lib.h
%{_includedir}/%{name}/gnunet_tun_lib.h
%{_includedir}/%{name}/gnunet_dnsstub_lib.h
%{_includedir}/%{name}/gnunet_dnsparser_lib.h
%{_includedir}/%{name}/gnunet_child_management_lib.h
%{_includedir}/%{name}/gnunet_error_codes.h
%{_includedir}/%{name}/gnunet_db_lib.h
%{_includedir}/%{name}/gnunet_curl_lib.h
%{_includedir}/%{name}/gnunet_scheduler_lib.h

%files -n libgnunet -f %{name}.lang
%dir %{_defaultdocdir}/%{name}
%{_defaultdocdir}/%{name}/README
%license %{_defaultdocdir}/%{name}/COPYING
%config(noreplace) %{_sysconfdir}/gnunet.conf
%{_libdir}/libgnunetsq.so.0
%{_libdir}/libgnunetsq.so.0.0.0
%{_libdir}/libgnunetcurl.so.0
%{_libdir}/libgnunetcurl.so.0.0.0
%{_libdir}/libgnunetabd.so.0
%{_libdir}/libgnunetabd.so.0.0.0
%{_libdir}/libgnunetarm.so.2
%{_libdir}/libgnunetarm.so.2.0.0
%{_libdir}/libgnunetjson.so.3
%{_libdir}/libgnunetjson.so.3.1.0
%{_libdir}/libgnunetrest.so.0
%{_libdir}/libgnunetrest.so.0.0.0
%{_libdir}/libgnunetutil.so.18
%{_libdir}/libgnunetutil.so.18.1.0
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/config.d
%{_datadir}/%{name}/config.d/util.conf
%{_bindir}/gnunet-base32
%{_bindir}/gnunet-bugreport
%{_bindir}/gnunet-config
%{_bindir}/gnunet-ecc
%{_mandir}/man1/gnunet-base32.1.gz
%{_mandir}/man1/gnunet-bugreport.1.gz
%{_mandir}/man1/gnunet-config.1.gz
%{_mandir}/man1/gnunet-ecc.1.gz

%files
# AGPLv3
%attr(0700, %{gnunetuser}, %{gnunetuser})
%{_bindir}/gnunet-abd
%{_bindir}/gnunet-arm
%{_bindir}/gnunet-auto-share
%{_bindir}/gnunet-cadet
%{_bindir}/gnunet-core
%{_bindir}/gnunet-datastore
%{_bindir}/gnunet-dht-get
%{_bindir}/gnunet-dht-monitor
%{_bindir}/gnunet-dht-put
%{_bindir}/gnunet-directory
%{_bindir}/gnunet-download
%{_bindir}/gnunet-fs
%{_bindir}/gnunet-gns
%{_bindir}/gnunet-gns-proxy-setup-ca
%{_bindir}/gnunet-hello
%{_bindir}/gnunet-identity
%{_bindir}/gnunet-namecache
%{_bindir}/gnunet-namestore
%{_bindir}/gnunet-namestore-dbtool
%{_bindir}/gnunet-namestore-zonefile
%{_bindir}/gnunet-nat
%{_bindir}/gnunet-nat-auto
%{_bindir}/gnunet-nat-server
%{_bindir}/gnunet-nse
%{_bindir}/gnunet-pils
%{_bindir}/gnunet-publish
%{_bindir}/gnunet-qr
%{_bindir}/gnunet-reclaim
%{_bindir}/gnunet-resolver
%{_bindir}/gnunet-revocation
%{_bindir}/gnunet-rps
%{_bindir}/gnunet-scrypt
%{_bindir}/gnunet-search
%{_bindir}/gnunet-statistics
%{_bindir}/gnunet-testbed
%{_bindir}/gnunet-testing-netjail-launcher
%{_bindir}/gnunet-transport-certificate-creation
%{_bindir}/gnunet-unindex
%{_bindir}/gnunet-uri
%{_bindir}/gnunet-vpn
%{_bindir}/gnunet-zoneimport
%{_bindir}/gnunet-dht-hello
%{_bindir}/gnunet-did
%{_bindir}/gnunet-messenger
%{_bindir}/gnunet-scalarproduct
%dir %{_libdir}/%{name}
%dir %{_libdir}/%{name}/libexec
%{_libdir}/%{name}/libexec/gnunet-communicator-tcp
%{_libdir}/%{name}/libexec/gnunet-communicator-udp
%{_libdir}/%{name}/libexec/gnunet-communicator-unix
%{_libdir}/%{name}/libexec/gnunet-daemon-exit
%{_libdir}/%{name}/libexec/gnunet-daemon-hostlist
%{_libdir}/%{name}/libexec/gnunet-daemon-pt
%{_libdir}/%{name}/libexec/gnunet-daemon-regexprofiler
%{_libdir}/%{name}/libexec/gnunet-daemon-topology
%{_libdir}/%{name}/libexec/gnunet-dns2gns
%{_libdir}/%{name}/libexec/gnunet-helper-dns
%{_libdir}/%{name}/libexec/gnunet-helper-exit
%{_libdir}/%{name}/libexec/gnunet-helper-fs-publish
%{_libdir}/%{name}/libexec/gnunet-helper-nat-client
%{_libdir}/%{name}/libexec/gnunet-helper-nat-server
%{_libdir}/%{name}/libexec/gnunet-helper-vpn
%{_libdir}/%{name}/libexec/gnunet-gns-proxy
%{_libdir}/%{name}/libexec/gnunet-rest-server
%{_libdir}/%{name}/libexec/gnunet-service-abd
%{_libdir}/%{name}/libexec/gnunet-service-arm
%{_libdir}/%{name}/libexec/gnunet-service-cadet
%{_libdir}/%{name}/libexec/gnunet-service-consensus
%{_libdir}/%{name}/libexec/gnunet-service-core
%{_libdir}/%{name}/libexec/gnunet-service-datastore
%{_libdir}/%{name}/libexec/gnunet-service-dht
%{_libdir}/%{name}/libexec/gnunet-service-dns
%{_libdir}/%{name}/libexec/gnunet-service-fs
%{_libdir}/%{name}/libexec/gnunet-service-gns
%{_libdir}/%{name}/libexec/gnunet-service-identity
%{_libdir}/%{name}/libexec/gnunet-service-namecache
%{_libdir}/%{name}/libexec/gnunet-service-namestore
%{_libdir}/%{name}/libexec/gnunet-service-nat
%{_libdir}/%{name}/libexec/gnunet-service-nat-auto
%{_libdir}/%{name}/libexec/gnunet-service-nse
%{_libdir}/%{name}/libexec/gnunet-service-peerstore
%{_libdir}/%{name}/libexec/gnunet-service-pils
%{_libdir}/%{name}/libexec/gnunet-service-reclaim
%{_libdir}/%{name}/libexec/gnunet-service-regex
%{_libdir}/%{name}/libexec/gnunet-service-resolver
%{_libdir}/%{name}/libexec/gnunet-service-revocation
%{_libdir}/%{name}/libexec/gnunet-service-rps
%{_libdir}/%{name}/libexec/gnunet-service-secretsharing
%{_libdir}/%{name}/libexec/gnunet-service-set
%{_libdir}/%{name}/libexec/gnunet-service-seti
%{_libdir}/%{name}/libexec/gnunet-service-setu
%{_libdir}/%{name}/libexec/gnunet-service-statistics
%{_libdir}/%{name}/libexec/gnunet-service-transport
%{_libdir}/%{name}/libexec/gnunet-service-vpn
%{_libdir}/%{name}/libexec/gnunet-service-zonemaster
%{_libdir}/%{name}/libexec/gnunet-suidfix
%{_libdir}/%{name}/libexec/gnunet-timeout
%{_libdir}/%{name}/libexec/gnunet-cmds-helper
%{_libdir}/%{name}/libexec/gnunet-service-messenger
%{_libdir}/%{name}/libexec/gnunet-service-scalarproduct-alice
%{_libdir}/%{name}/libexec/gnunet-service-scalarproduct-bob
%{_libdir}/%{name}/libexec/gnunet-service-scalarproduct-ecc-alice
%{_libdir}/%{name}/libexec/gnunet-service-scalarproduct-ecc-bob
%{_libdir}/%{name}/libgnunet_plugin_gnsrecord_messenger.so
%{_libdir}/%{name}/libgnunet_plugin_block_consensus.so
%{_libdir}/%{name}/libgnunet_plugin_block_dht.so
%{_libdir}/%{name}/libgnunet_plugin_block_dns.so
%{_libdir}/%{name}/libgnunet_plugin_block_fs.so
%{_libdir}/%{name}/libgnunet_plugin_block_gns.so
%{_libdir}/%{name}/libgnunet_plugin_block_regex.so
%{_libdir}/%{name}/libgnunet_plugin_block_revocation.so
%{_libdir}/%{name}/libgnunet_plugin_block_set_test.so
%{_libdir}/%{name}/libgnunet_plugin_block_seti_test.so
%{_libdir}/%{name}/libgnunet_plugin_block_setu_test.so
%{_libdir}/%{name}/libgnunet_plugin_block_test.so
%{_libdir}/%{name}/libgnunet_plugin_datacache_heap.so
%{_libdir}/%{name}/libgnunet_plugin_datacache_sqlite.so
%{_libdir}/%{name}/libgnunet_plugin_datastore_heap.so
%{_libdir}/%{name}/libgnunet_plugin_datastore_sqlite.so
%{_libdir}/%{name}/libgnunet_plugin_gnsrecord_dns.so
%{_libdir}/%{name}/libgnunet_plugin_gnsrecord_gns.so
%{_libdir}/%{name}/libgnunet_plugin_gnsrecord_reclaim.so
%{_libdir}/%{name}/libgnunet_plugin_namecache_flat.so
%{_libdir}/%{name}/libgnunet_plugin_namecache_sqlite.so
%{_libdir}/%{name}/libgnunet_plugin_namestore_sqlite.so
%{_libdir}/%{name}/libgnunet_plugin_peerstore_sqlite.so
%{_libdir}/%{name}/libgnunet_plugin_reclaim_attribute_basic.so
%{_libdir}/%{name}/libgnunet_plugin_reclaim_credential_jwt.so
%{_libdir}/libgnunetblock.so.0
%{_libdir}/libgnunetblock.so.0.0.0
%{_libdir}/libgnunetblockgroup.so.0
%{_libdir}/libgnunetblockgroup.so.0.0.0
%{_libdir}/libgnunetcadet.so.7
%{_libdir}/libgnunetcadet.so.7.0.0
%{_libdir}/libgnunetconsensus.so.0
%{_libdir}/libgnunetconsensus.so.0.0.0
%{_libdir}/libgnunetcore.so.0
%{_libdir}/libgnunetcore.so.0.0.1
%{_libdir}/libgnunetcoreunderlaydummy.so.0.0.0
%{_libdir}/libgnunetdatacache.so.0
%{_libdir}/libgnunetdatacache.so.0.0.1
%{_libdir}/libgnunetdatastore.so.1
%{_libdir}/libgnunetdatastore.so.1.0.0
%{_libdir}/libgnunetdht.so.4
%{_libdir}/libgnunetdht.so.4.0.0
%{_libdir}/libgnunetdid.so.0
%{_libdir}/libgnunetdid.so.0.0.0
%{_libdir}/libgnunetdns.so.0
%{_libdir}/libgnunetdns.so.0.0.0
%{_libdir}/libgnunetfs.so.2
%{_libdir}/libgnunetfs.so.2.1.1
%{_libdir}/libgnunetgns.so.0
%{_libdir}/libgnunetgns.so.0.0.0
%{_libdir}/libgnunetgnsrecord.so.0
%{_libdir}/libgnunetgnsrecord.so.0.0.0
%{_libdir}/libgnunetgnsrecordjson.so.0
%{_libdir}/libgnunetgnsrecordjson.so.0.0.0
%{_libdir}/libgnunethello.so.0
%{_libdir}/libgnunethello.so.0.1.0
%{_libdir}/libgnunetidentity.so.1
%{_libdir}/libgnunetidentity.so.1.0.0
%{_libdir}/libgnunetmhd.so.0
%{_libdir}/libgnunetmhd.so.0.0.1
%{_libdir}/libgnunetnamecache.so.0
%{_libdir}/libgnunetnamecache.so.0.0.0
%{_libdir}/libgnunetnamestore.so.0
%{_libdir}/libgnunetnamestore.so.0.0.1
%{_libdir}/libgnunetnat.so.2
%{_libdir}/libgnunetnat.so.2.0.0
%{_libdir}/libgnunetnatauto.so.0
%{_libdir}/libgnunetnatauto.so.0.0.0
%{_libdir}/libgnunetnatnew.so.2
%{_libdir}/libgnunetnatnew.so.2.0.0
%{_libdir}/libgnunetnse.so.0
%{_libdir}/libgnunetnse.so.0.0.0
%{_libdir}/libgnunetpeerstore.so.0
%{_libdir}/libgnunetpeerstore.so.0.0.0
%{_libdir}/libgnunetpils.so.0
%{_libdir}/libgnunetpils.so.0.0.0
%{_libdir}/libgnunetreclaim.so.0
%{_libdir}/libgnunetreclaim.so.0.0.0
%{_libdir}/libgnunetregex.so.3
%{_libdir}/libgnunetregex.so.3.0.1
%{_libdir}/libgnunetregexblock.so.1
%{_libdir}/libgnunetregexblock.so.1.0.0
%{_libdir}/libgnunetrps.so.0
%{_libdir}/libgnunetrps.so.0.0.0
%{_libdir}/libgnunetrevocation.so.0
%{_libdir}/libgnunetrevocation.so.0.0.0
%{_libdir}/libgnunetsecretsharing.so.0
%{_libdir}/libgnunetsecretsharing.so.0.0.0
%{_libdir}/libgnunetset.so.0
%{_libdir}/libgnunetset.so.0.0.0
%{_libdir}/libgnunetseti.so.0
%{_libdir}/libgnunetseti.so.0.0.0
%{_libdir}/libgnunetsetu.so.0
%{_libdir}/libgnunetsetu.so.0.0.0
%{_libdir}/libgnunetstatistics.so.2
%{_libdir}/libgnunetstatistics.so.2.0.0
%{_libdir}/libgnunettestbed.so.0
%{_libdir}/libgnunettestbed.so.0.0.0
%{_libdir}/libgnunettesting.so.3
%{_libdir}/libgnunettesting.so.3.0.0
%{_libdir}/libgnunettestingarm.so.0
%{_libdir}/libgnunettestingarm.so.0.0.0
%{_libdir}/libgnunettestingcore.so.0
%{_libdir}/libgnunettestingcore.so.0.0.0
%{_libdir}/libgnunettestingtestbed.so.0
%{_libdir}/libgnunettestingtestbed.so.0.0.0
%{_libdir}/libgnunettestingtransport.so.0
%{_libdir}/libgnunettestingtransport.so.0.0.0
%{_libdir}/libgnunettransportapplication.so.0
%{_libdir}/libgnunettransportapplication.so.0.0.0
%{_libdir}/libgnunettransportcommunicator.so.0
%{_libdir}/libgnunettransportcommunicator.so.0.0.0
%{_libdir}/libgnunettransportcore.so.0
%{_libdir}/libgnunettransportcore.so.0.0.0
%{_libdir}/libgnunettransportmonitor.so.0
%{_libdir}/libgnunettransportmonitor.so.0.0.0
%{_libdir}/libgnunetvpn.so.0
%{_libdir}/libgnunetvpn.so.0.0.0
%{_libdir}/libgnunetmessenger.so.0
%{_libdir}/libgnunetmessenger.so.0.0.0
%{_libdir}/libgnunetscalarproduct.so.0
%{_libdir}/libgnunetscalarproduct.so.0.0.0
%{_datadir}/applications/gnunet-uri.desktop
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/openssl.cnf
%{_datadir}/%{name}/config.d/abd.conf
%{_datadir}/%{name}/config.d/arm.conf
%{_datadir}/%{name}/config.d/cadet.conf
%{_datadir}/%{name}/config.d/consensus.conf
%{_datadir}/%{name}/config.d/core.conf
%{_datadir}/%{name}/config.d/datacache.conf
%{_datadir}/%{name}/config.d/datastore.conf
%{_datadir}/%{name}/config.d/dht.conf
%{_datadir}/%{name}/config.d/dns.conf
%{_datadir}/%{name}/config.d/exit.conf
%{_datadir}/%{name}/config.d/fs.conf
%{_datadir}/%{name}/config.d/gns.conf
%{_datadir}/%{name}/config.d/hostlist.conf
%{_datadir}/%{name}/config.d/identity.conf
%{_datadir}/%{name}/config.d/namecache.conf
%{_datadir}/%{name}/config.d/namestore.conf
%{_datadir}/%{name}/config.d/nat-auto.conf
%{_datadir}/%{name}/config.d/nat.conf
%{_datadir}/%{name}/config.d/nse.conf
%{_datadir}/%{name}/config.d/peerstore.conf
%{_datadir}/%{name}/config.d/pils.conf
%{_datadir}/%{name}/config.d/pt.conf
%{_datadir}/%{name}/config.d/reclaim.conf
%{_datadir}/%{name}/config.d/regex.conf
%{_datadir}/%{name}/config.d/resolver.conf
%{_datadir}/%{name}/config.d/rest.conf
%{_datadir}/%{name}/config.d/revocation.conf
%{_datadir}/%{name}/config.d/rps.conf
%{_datadir}/%{name}/config.d/secretsharing.conf
%{_datadir}/%{name}/config.d/set.conf
%{_datadir}/%{name}/config.d/seti.conf
%{_datadir}/%{name}/config.d/setu.conf
%{_datadir}/%{name}/config.d/statistics.conf
%{_datadir}/%{name}/config.d/topology.conf
%{_datadir}/%{name}/config.d/transport.conf
%{_datadir}/%{name}/config.d/vpn.conf
%{_datadir}/%{name}/config.d/zonemaster.conf
%{_datadir}/%{name}/config.d/dhtu.conf
%{_datadir}/%{name}/config.d/messenger.conf
%{_datadir}/%{name}/config.d/scalarproduct.conf
%{_datadir}/%{name}/config.d/tlds.conf
%{_datadir}/%{name}/gnunet-logo-dark-only-text.svg
%{_datadir}/%{name}/gnunet-logo-dark-text.svg
%{_datadir}/%{name}/netjail_core.sh
%{_datadir}/%{name}/netjail_exec.sh
%{_datadir}/%{name}/netjail_start.sh
%{_datadir}/%{name}/netjail_start_new.sh
%{_datadir}/%{name}/netjail_stop.sh
%{_datadir}/%{name}/netjail_test_master.sh
%{_datadir}/%{name}/topo.sh
%{_datadir}/%{name}/gnunet-gns-proxy-ca.template
%{_datadir}/%{name}/gnunet-logo.png
%{_datadir}/%{name}/hellos/
%{_datadir}/%{name}/sql/
%{_mandir}/man1/gnunet-arm.1.gz
%{_mandir}/man1/gnunet-auto-share.1.gz
%{_mandir}/man1/gnunet-cadet.1.gz
%{_mandir}/man1/gnunet-core.1.gz
%{_mandir}/man1/gnunet-datastore.1.gz
%{_mandir}/man1/gnunet-dht-get.1.gz
%{_mandir}/man1/gnunet-dht-put.1.gz
%{_mandir}/man1/gnunet-dht-monitor.1.gz
%{_mandir}/man1/gnunet-dht-hello.1.gz
%{_mandir}/man1/gnunet-did.1.gz
%{_mandir}/man1/gnunet-directory.1.gz
%{_mandir}/man1/gnunet-dns2gns.1.gz
%{_mandir}/man1/gnunet-download.1.gz
%{_mandir}/man1/gnunet-fs.1.gz
%{_mandir}/man1/gnunet-gns-proxy-setup-ca.1.gz
%{_mandir}/man1/gnunet-gns-proxy.1.gz
%{_mandir}/man1/gnunet-gns.1.gz
%{_mandir}/man1/gnunet-hello.1.gz
%{_mandir}/man1/gnunet-identity.1.gz
%{_mandir}/man1/gnunet-messenger.1.gz
%{_mandir}/man1/gnunet-namecache.1.gz
%{_mandir}/man1/gnunet-namestore.1.gz
%{_mandir}/man1/gnunet-namestore-dbtool.1.gz
%{_mandir}/man1/gnunet-namestore-zonefile.1.gz
%{_mandir}/man1/gnunet-nat-auto.1.gz
%{_mandir}/man1/gnunet-nat-server.1.gz
%{_mandir}/man1/gnunet-nat.1.gz
%{_mandir}/man1/gnunet-nse.1.gz
%{_mandir}/man1/gnunet-publish.1.gz
%{_mandir}/man1/gnunet-pils.1.gz
%{_mandir}/man1/gnunet-qr.1.gz
%{_mandir}/man1/gnunet-reclaim.1.gz
%{_mandir}/man1/gnunet-resolver.1.gz
%{_mandir}/man1/gnunet-revocation.1.gz
%{_mandir}/man1/gnunet-scalarproduct.1.gz
%{_mandir}/man1/gnunet-scrypt.1.gz
%{_mandir}/man1/gnunet-search.1.gz
%{_mandir}/man1/gnunet-statistics.1.gz
%{_mandir}/man1/gnunet-testing.1.gz
%{_mandir}/man1/gnunet-testing-run-service.1.gz
%{_mandir}/man1/gnunet-timeout.1.gz
%{_mandir}/man1/gnunet-transport-certificate-creation.1.gz
%{_mandir}/man1/gnunet-transport.1.gz
%{_mandir}/man1/gnunet-unindex.1.gz
%{_mandir}/man1/gnunet-uri.1.gz
%{_mandir}/man1/gnunet-vpn.1.gz
%{_mandir}/man1/gnunet-zoneimport.1.gz
%{_mandir}/man5/gnunet.conf.5.gz
%{_libdir}/libnss_gns.so.2
%{_libdir}/libnss_gns4.so.2
%{_libdir}/libnss_gns6.so.2
%{_unitdir}/%{name}.service
%{_userunitdir}/%{name}-user.service
#%%{_sysconfdir}/NetworkManager/dispatcher.d/10-dns2gns.sh

%files devel
%{_libdir}/libgnunetblockgroup.so
%{_libdir}/libgnunetblock.so
%{_libdir}/libgnunetcadet.so
%{_libdir}/libgnunetconsensus.so
%{_libdir}/libgnunetcore.so
%{_libdir}/libgnunetcoreunderlaydummy.so
%{_libdir}/libgnunetdatacache.so
%{_libdir}/libgnunetdatastore.so
%{_libdir}/libgnunetdht.so
%{_libdir}/libgnunetdid.so
%{_libdir}/libgnunetdns.so
%{_libdir}/libgnunetfs.so
%{_libdir}/libgnunetgns.so
%{_libdir}/libgnunetgnsrecord.so
%{_libdir}/libgnunetgnsrecordjson.so
%{_libdir}/libgnunethello.so
%{_libdir}/libgnunetidentity.so
%{_libdir}/libgnunetmhd.so
%{_libdir}/libgnunetnamecache.so
%{_libdir}/libgnunetnamestore.so
%{_libdir}/libgnunetnat.so
%{_libdir}/libgnunetnatauto.so
%{_libdir}/libgnunetnatnew.so
%{_libdir}/libgnunetnse.so
%{_libdir}/libgnunetpeerstore.so
%{_libdir}/libgnunetpils.so
%{_libdir}/libgnunetreclaim.so
%{_libdir}/libgnunetregex.so
%{_libdir}/libgnunetregexblock.so
%{_libdir}/libgnunetrevocation.so
%{_libdir}/libgnunetrps.so
%{_libdir}/libgnunetsecretsharing.so
%{_libdir}/libgnunetset.so
%{_libdir}/libgnunetseti.so
%{_libdir}/libgnunetsetu.so
%{_libdir}/libgnunetstatistics.so
%{_libdir}/libgnunettestbed.so
%{_libdir}/libgnunettesting.so
%{_libdir}/libgnunettestingarm.so
%{_libdir}/libgnunettestingcore.so
%{_libdir}/libgnunettestingtestbed.so
%{_libdir}/libgnunettestingtransport.so
%{_libdir}/libgnunettransportapplication.so
%{_libdir}/libgnunettransportcommunicator.so
%{_libdir}/libgnunettransportmonitor.so
%{_libdir}/libgnunettransportcore.so
%{_libdir}/libgnunetvpn.so
%{_libdir}/libgnunetmessenger.so
%{_libdir}/libgnunetscalarproduct.so
%{_libdir}/libnss_gns.so
%{_libdir}/libnss_gns4.so
%{_libdir}/libnss_gns6.so
%{_includedir}/%{name}/gnunet_abd_service.h
%{_includedir}/%{name}/gnunet_block_group_lib.h
%{_includedir}/%{name}/gnunet_block_lib.h
%{_includedir}/%{name}/gnunet_block_plugin.h
%{_includedir}/%{name}/gnunet_cadet_service.h
%{_includedir}/%{name}/gnunet_consensus_service.h
%{_includedir}/%{name}/gnunet_core_service.h
%{_includedir}/%{name}/gnunet_datacache_lib.h
%{_includedir}/%{name}/gnunet_datacache_plugin.h
%{_includedir}/%{name}/gnunet_datastore_plugin.h
%{_includedir}/%{name}/gnunet_datastore_service.h
%{_includedir}/%{name}/gnunet_dht_service.h
%{_includedir}/%{name}/gnunet_dht_block_types.h
%{_includedir}/%{name}/gnunet_dns_service.h
%{_includedir}/%{name}/gnunet_fs_service.h
%{_includedir}/%{name}/gnunet_gns_service.h
%{_includedir}/%{name}/gnunet_gnsrecord_lib.h
%{_includedir}/%{name}/gnunet_gnsrecord_plugin.h
%{_includedir}/%{name}/gnunet_gnsrecord_json_lib.h
%{_includedir}/%{name}/gnunet_identity_service.h
%{_includedir}/%{name}/gnunet_messenger_service.h
%{_includedir}/%{name}/gnunet_mhd_lib.h
%{_includedir}/%{name}/gnunet_namecache_plugin.h
%{_includedir}/%{name}/gnunet_namecache_service.h
%{_includedir}/%{name}/gnunet_namestore_plugin.h
%{_includedir}/%{name}/gnunet_namestore_service.h
%{_includedir}/%{name}/gnunet_nat_auto_service.h
%{_includedir}/%{name}/gnunet_nat_lib.h
%{_includedir}/%{name}/gnunet_nat_service.h
%{_includedir}/%{name}/gnunet_nse_service.h
%{_includedir}/%{name}/gnunet_nt_lib.h
%{_includedir}/%{name}/gnunet_peerstore_plugin.h
%{_includedir}/%{name}/gnunet_peerstore_service.h
%{_includedir}/%{name}/gnunet_pils_service.h
%{_includedir}/%{name}/gnunet_reclaim_lib.h
%{_includedir}/%{name}/gnunet_reclaim_plugin.h
%{_includedir}/%{name}/gnunet_reclaim_service.h
%{_includedir}/%{name}/gnunet_regex_service.h
%{_includedir}/%{name}/gnunet_resolver_service.h
%{_includedir}/%{name}/gnunet_rest_lib.h
%{_includedir}/%{name}/gnunet_rest_plugin.h
%{_includedir}/%{name}/gnunet_revocation_service.h
%{_includedir}/%{name}/gnunet_rps_service.h
%{_includedir}/%{name}/gnunet_secretsharing_service.h
%{_includedir}/%{name}/gnunet_set_service.h
%{_includedir}/%{name}/gnunet_seti_service.h
%{_includedir}/%{name}/gnunet_setu_service.h
%{_includedir}/%{name}/gnunet_signatures.h
%{_includedir}/%{name}/gnunet_socks.h
%{_includedir}/%{name}/gnunet_statistics_service.h
%{_includedir}/%{name}/gnunet_testing_lib.h
%{_includedir}/%{name}/gnunet_testing_arm_lib.h
%{_includedir}/%{name}/gnunet_testing_core_lib.h
%{_includedir}/%{name}/gnunet_testing_testbed_lib.h
%{_includedir}/%{name}/gnunet_testing_transport_lib.h
%{_includedir}/%{name}/gnunet_transport_application_service.h
%{_includedir}/%{name}/gnunet_transport_communication_service.h
%{_includedir}/%{name}/gnunet_transport_core_service.h
%{_includedir}/%{name}/gnunet_transport_monitor_service.h
%{_includedir}/%{name}/gnunet_vpn_service.h
%{_includedir}/%{name}/gnunet_dhtu_plugin.h
%{_includedir}/%{name}/gnunet_hello_uri_lib.h
%{_includedir}/%{name}/gnu_name_system_record_types.h
%{_includedir}/%{name}/gnu_name_system_protocols.h
%{_includedir}/%{name}/gnu_name_system_service_ports.h
%{_includedir}/%{name}/gnunet_scalarproduct_service.h
%{_libdir}/pkgconfig/gnunetblock.pc
%{_libdir}/pkgconfig/gnunetcadet.pc
%{_libdir}/pkgconfig/gnunetconsensus.pc
%{_libdir}/pkgconfig/gnunetcore.pc
%{_libdir}/pkgconfig/gnunetdatacache.pc
%{_libdir}/pkgconfig/gnunetdatastore.pc
%{_libdir}/pkgconfig/gnunetdht.pc
%{_libdir}/pkgconfig/gnunetdid.pc
%{_libdir}/pkgconfig/gnunetdns.pc
%{_libdir}/pkgconfig/gnunetfs.pc
%{_libdir}/pkgconfig/gnunetgns.pc
%{_libdir}/pkgconfig/gnunethello.pc
%{_libdir}/pkgconfig/gnunetidentity.pc
%{_libdir}/pkgconfig/gnunetjson.pc
%{_libdir}/pkgconfig/gnunetmhd.pc
%{_libdir}/pkgconfig/gnunetmessenger.pc
%{_libdir}/pkgconfig/gnunetnamestore.pc
%{_libdir}/pkgconfig/gnunetnat.pc
%{_libdir}/pkgconfig/gnunetnatauto.pc
%{_libdir}/pkgconfig/gnunetnse.pc
%{_libdir}/pkgconfig/gnunetpils.pc
%{_libdir}/pkgconfig/gnunetregex.pc
%{_libdir}/pkgconfig/gnunetrevocation.pc
%{_libdir}/pkgconfig/gnunetset.pc
%{_libdir}/pkgconfig/gnunetstatistics.pc
%{_libdir}/pkgconfig/gnunettestbed.pc
%{_libdir}/pkgconfig/gnunettesting.pc
%{_libdir}/pkgconfig/gnunetutil.pc
%{_libdir}/pkgconfig/gnunetvpn.pc
%{_libdir}/pkgconfig/gnunetrps.pc
%{_libdir}/pkgconfig/gnunetscalarproduct.pc
%{_libdir}/pkgconfig/gnunetgnsrecord.pc
%{_libdir}/pkgconfig/gnunetnamecache.pc
%{_libdir}/pkgconfig/gnunetpeerstore.pc
%{_libdir}/pkgconfig/gnunetreclaim.pc
%{_libdir}/pkgconfig/gnunetrest.pc
%{_libdir}/pkgconfig/gnunetsecretsharing.pc
%{_libdir}/pkgconfig/gnunetseti.pc
%{_libdir}/pkgconfig/gnunetsetu.pc
%{_libdir}/pkgconfig/gnunetsq.pc
%{_libdir}/pkgconfig/gnunettransportapplication.pc
%{_libdir}/pkgconfig/gnunettransportcommunicator.pc
%{_libdir}/pkgconfig/gnunettransportcore.pc
%{_libdir}/pkgconfig/gnunettransportmonitor.pc

%files -n libgnunetpq
%{_libdir}/libgnunetpq.so.5
%{_libdir}/libgnunetpq.so.5.4.0

%files -n libgnunetpq-devel

%{_libdir}/libgnunetpq.so
%{_includedir}/%{name}/gnunet_pq_lib.h

%files postgresql-plugins
%{_libdir}/%{name}/libgnunet_plugin_datacache_postgres.so
%{_libdir}/%{name}/libgnunet_plugin_datastore_postgres.so
%{_libdir}/%{name}/libgnunet_plugin_namecache_postgres.so
%{_libdir}/%{name}/libgnunet_plugin_namestore_postgres.so

%files conversation
%{_bindir}/gnunet-conversation
%{_bindir}/gnunet-conversation-test
%{_libdir}/%{name}/libexec/gnunet-helper-audio-playback
%{_libdir}/%{name}/libexec/gnunet-helper-audio-record
%{_libdir}/%{name}/libexec/gnunet-service-conversation
%{_libdir}/%{name}/libgnunet_plugin_gnsrecord_conversation.so
%{_libdir}/libgnunetconversation.so.0
%{_libdir}/libgnunetconversation.so.0.0.0
%{_libdir}/libgnunetmicrophone.so.0
%{_libdir}/libgnunetmicrophone.so.0.0.0
%{_libdir}/libgnunetspeaker.so.0
%{_libdir}/libgnunetspeaker.so.0.0.0
%{_mandir}/man1/gnunet-conversation-test.1.gz
%{_mandir}/man1/gnunet-conversation.1.gz
%{_datadir}/%{name}/config.d/conversation.conf

%files conversation-devel
%{_libdir}/libgnunetconversation.so
%{_libdir}/libgnunetmicrophone.so
%{_libdir}/libgnunetspeaker.so
%{_includedir}/%{name}/gnunet_microphone_lib.h
%{_includedir}/%{name}/gnunet_speaker_lib.h
%{_includedir}/%{name}/gnunet_conversation_service.h
%{_libdir}/pkgconfig/gnunetmicrophone.pc
%{_libdir}/pkgconfig/gnunetspeaker.pc
%{_libdir}/pkgconfig/gnunetconversation.pc


%files bcd
%{_libdir}/%{name}/libexec/gnunet-bcd
%{_datadir}/%{name}/def.tex
%{_datadir}/%{name}/gns-bcd.html
%{_datadir}/%{name}/gns-bcd.tex
%{_datadir}/%{name}/gns-bcd-forbidden.html
%{_datadir}/%{name}/gns-bcd-internal-error.html
%{_datadir}/%{name}/gns-bcd-invalid-key.html
%{_datadir}/%{name}/gns-bcd-not-found.html
%{_datadir}/%{name}/gns-bcd-png.tex
%{_datadir}/%{name}/gns-bcd-simple.html
%{_datadir}/%{name}/gns-bcd-simple.tex
%{_mandir}/man1/gnunet-bcd.1.gz

#%files doc
%{_defaultdocdir}/%{name}/html/
%exclude %{_defaultdocdir}/%{name}/html/.buildinfo
%{_infodir}/gnunet.info.gz

%changelog
* Sat Nov 15 2025 Martin Schanzenbach <schanzen@gnunet.org> - 0.26.0-1
- New release

* Mon Oct 20 2025 Martin Schanzenbach <schanzen@gnunet.org> - 0.25.2-1
- New release

* Fri Apr 11 2025 Martin Schanzenbach <schanzen@gnunet.org> - 0.24.1-1
- New release

* Fri Mar 14 2025 Martin Schanzenbach <schanzen@gnunet.org> - 0.24.0-1
- New release

* Sat Jun 08 2024 Martin Schanzenbach <schanzen@gnunet.org> - 0.21.2-1
- New release

* Fri Mar 15 2024 Martin Schanzenbach <schanzen@gnunet.org> - 0.21.1-1
- New release

* Wed Mar 06 2024 Martin Schanzenbach <schanzen@gnunet.org> - 0.21.0-1
- New release

* Wed Sep 27 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.20.0-1
- New release

* Thu May 18 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-6
- Move gnunet_mhd_compat.h to libgnunet-devel.

* Thu May 18 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-5
- Move gnunet_configuration_lib.h to libgnunet-devel.

* Thu May 18 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-4
- Better sort header distribution especially for libgnunet.

* Thu May 18 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-3
- Fix broken libgnunetutil and missing gnunet_bandwidth_lib.h

* Thu May 18 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-2
- Separate out postgres/mysql utility libraries in preparation for Taler packages.

* Mon May 15 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.4-1
- Version bump

* Sun Jan 08 2023 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.2-1
- Version bump

* Sat Dec 31 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.1-3
- Properly use %%lang
- Exclude some files from being installed that are not really required (images)
- Exclude openrc scripts from being installed needlessly
- Separate more files from bcd into subpackage
- Adjust license to AGPL-3.0-or-later
- Move testing hostkeys and gnunet.m4 to devel

* Fri Dec 30 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.1-2
- Try to address a few rpmlint issues
* Thu Dec 29 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.1
- Version bump
* Wed Dec 07 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.19.0
- Version bump
* Fri Nov 04 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.18.1
- Version bump
* Mon Sep 26 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.6
- Version bump
* Fri Sep 09 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.5
- Add systemd scripts
* Sun Sep 04 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.5
- Version bump
* Fri Aug 12 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.4
- Simplify documentation package
* Thu Aug 11 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.4
- Version bump
* Sun Aug 07 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.3
- Version bump
* Tue Jul 12 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.2
- Version bump
* Mon Jul 04 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.1
- Version bump
- Split up package better
* Mon Jun 06 2022 Martin Schanzenbach <schanzen@gnunet.org> - 0.17.0
- Version bump
* Mon May 17 2021 Jospeh Burchetta <joseph@seattlemesh.net> - 0.14.1
- GPG verification
* Sat Mar 27 2021 Joseph Burchetta <joseph@seattlemesh.net> - 0.14.0
- Updated for Fedora 34
- Fixed package license details
* Thu Jul 09 2020 Joseph Burchetta <joseph@seattlemesh.net> - 0.13.0
- ARMv7 packaged
- Update for Fedora 32
* Sun Dec 1 2019 Joseph Burchetta <joseph@seattlemesh.net> - 0.11.8-0-1550
- First GNUnet RPM

