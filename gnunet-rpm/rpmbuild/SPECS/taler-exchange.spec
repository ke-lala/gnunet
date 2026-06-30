Name:           taler-exchange
Version:        1.0.0
Release:        1%{?dist}
Summary:        The Taler exchange service.
License:        AGPL-3.0-or-later
URL:            https://taler.net
%global         _disable_source_fetch 0
Source0:        https://ftpmirror.gnu.org/gnu/taler/%{name}-%{version}.tar.gz
Source1:        https://ftpmirror.gnu.org/gnu/taler/%{name}-%{version}.tar.gz.sig
Source2:        https://keys.openpgp.org/vks/v1/by-fingerprint/D8423BCB326C7907033929C7939E6BE1E29FC3CC
%global         SHA512SUM0 ddb593e25817b534b22eb10ed589b33c8ebe42f48f9190aba4bba44995ab7e8569cfbcdbc27bba345ec26cbe4f3d32d80b439d1267c4731659b2a29c862f1324
BuildRequires:  libtool
BuildRequires:  libtool-ltdl
BuildRequires:  libtool-ltdl-devel
BuildRequires:  autoconf
BuildRequires:  make
BuildRequires:  gettext
BuildRequires:  gcc
BuildRequires:  sqlite-devel
BuildRequires:  libunistring-devel
BuildRequires:  libcurl-devel
BuildRequires:  libgcrypt-devel
BuildRequires:  libsodium-devel
BuildRequires:  libargon2-devel
BuildRequires:  jansson-devel
BuildRequires:  libpq-devel
BuildRequires:  gnupg2
BuildRequires:  jq
BuildRequires:  zlib-devel
Recommends: postgresql-server
Recommends: taler-exchange-offline
BuildRequires:  libmicrohttpd-devel
BuildRequires:  libgnunetpq-devel
BuildRequires:  python3-jinja2

%description
GNU Taler is the privacy-preserving digital payment
system from the GNU project. This package contains the
core logic that must be run by the payment service
provider or bank to offer payments to consumers and
merchants.  At least one exchange must be operated
per currency.
In addition to the core logic, an exchange operator
must also have a system running the "offline" logic
which is packaged as taler-exchange-offline. It is
recommended to keep the "offline" logic on a system
that is never connected to the Internet. However, it
is also possible to run the "offline" logic directly
on the production system, especially for testing.
Finally, an exchange operator should also be prepared
to run a taler-auditor.


%package devel
Summary: Taler exchange development headers
%description devel
Files needed to develop Taler exchange applications and services.

%package offline
Requires: libtalerexchange%{?_isa} = %{version}-%{release}
Summary: Tools for managing the GNU Taler exchange offline keys
%description offline
A GNU Taler exchange uses an offline key to sign its online
keys, fee structure, bank routing information and other meta
data. The offline signing key is the root of the Taler PKI
that is then embedded in consumer wallets and merchant backends.
This package includes the tool to download material to sign
from the exchange, create signatures, and upload the resulting
signatures to the exchange.


%package database
Summary: Programs and libraries to manage a GNU Taler exchange database.
%description database
This package contains only the code to setup the (Postgresql) database interaction (taler-exchange-dbinit and associated resource files).

%package -n libtalerexchange
Summary: Libraries to talk to a GNU Taler exchange
%description -n libtalerexchange
Libraries to talk to a GNU Taler exchange.
The package also contains various files fundamental to all GNU Taler installations, such as the taler-config configuration command-line tool, various base configuration files and associated documentation.

%package -n libtalerexchange-devel
Requires: libtalerexchange%{?_isa} = %{version}-%{release}
Summary: Development files to talk to a GNU Taler exchange
%description -n libtalerexchange-devel
Libraries to talk to a GNU Taler exchange (development).

%package -n taler-auditor
Requires: libtalerexchange%{?_isa} = %{version}-%{release}
Requires: taler-exchange-database%{?_isa} = %{version}-%{release}
Summary: GNU's payment system auditor.
%description -n taler-auditor
GNU Taler is the privacy-preserving digital payment
system from the GNU project. This package contains the
auditor logic. It verifies that the taler-exchange run
by a payment service provider is correctly performing
its bank transactions and thus has the correct balance
in its escrow account.  Each exchange operator is
expected to make use of one or more auditors as part
of its regulatory compliance.

%prep
echo "%SHA512SUM0 %SOURCE0" | sha512sum -c -
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'

%autosetup -N
%build
%configure
%{make_build}

%install
make install DESTDIR=%{buildroot}
#%find_lang %{name}
# FIXME: Installing default configs. Those must be shipped in the tarball.
#install debian/etc-libtalerexchange/* etc/
#install debian/etc-taler-auditor/* etc/
#install debian/etc-taler-exchange/* etc/
#install -Dm644 "contrib/packages/fedora/%{name}.service" "%{buildroot}/%{_unitdir}/%{name}.service"
#install -Dm644 "contrib/packages/fedora/%{name}-user.service" "%{buildroot}/%{_userunitdir}/%{name}-user.service"
#install -Dm644 "contrib/packages/fedora/gnunet-system.conf" %{buildroot}/%{_sysconfdir}/gnunet.conf
rm -f %{buildroot}%{_infodir}/dir

%check
# FIXME test builds require installed libtalerexchange. May be upstream bug.
#make check

%post
#%systemd_post %{name}.service
#%systemd_user_post %{name}-user.service

%preun
#%systemd_preun %{name}.service

%postun
#%systemd_postun_with_restart %{name}.service
#%systemd_user_post %{name}-user.service

%files
# AGPLv3
%{_bindir}/taler-exchange-aggregator
%{_bindir}/taler-exchange-closer
%{_bindir}/taler-exchange-dbinit
%{_bindir}/taler-exchange-drain
%{_bindir}/taler-exchange-expire
%{_bindir}/taler-exchange-dbconfig
%{_bindir}/taler-exchange-helper-converter-oauth2-test-full_name
%{_bindir}/taler-exchange-helper-measure-test-form
%{_bindir}/taler-exchange-helper-measure-test-oauth
%{_bindir}/taler-exchange-httpd
%{_bindir}/taler-exchange-kyc-aml-pep-trigger.sh
%{_bindir}/taler-exchange-kyc-kycaid-converter.sh
%{_bindir}/taler-exchange-kyc-oauth2-challenger.sh
%{_bindir}/taler-exchange-kyc-oauth2-nda.sh
%{_bindir}/taler-exchange-kyc-oauth2-test-converter.sh
%{_bindir}/taler-exchange-kyc-persona-converter.sh
%{_bindir}/taler-exchange-router
%{_bindir}/taler-exchange-secmod-cs
%{_bindir}/taler-exchange-secmod-eddsa
%{_bindir}/taler-exchange-secmod-rsa
%{_bindir}/taler-exchange-transfer
%{_bindir}/taler-exchange-wirewatch
%{_bindir}/taler-exchange-wire-gateway-client
%{_bindir}/taler-exchange-helper-measure-challenger-email-context-check
%{_bindir}/taler-exchange-helper-measure-challenger-postal-context-check
%{_bindir}/taler-exchange-helper-measure-challenger-sms-context-check
%{_bindir}/taler-exchange-helper-measure-clear-continue
%{_bindir}/taler-exchange-helper-measure-defaults-but-investigate
%{_bindir}/taler-exchange-helper-measure-freeze
%{_bindir}/taler-exchange-helper-measure-inform-investigate
%{_bindir}/taler-exchange-helper-measure-none
%{_bindir}/taler-exchange-helper-measure-preserve-but-investigate
%{_bindir}/taler-exchange-helper-measure-preserve-set-expiration
%{_bindir}/taler-exchange-helper-measure-tops-3rdparty-check
%{_bindir}/taler-exchange-helper-measure-tops-address-check
%{_bindir}/taler-exchange-helper-measure-tops-kyx-check
%{_bindir}/taler-exchange-helper-measure-tops-postal-check
%{_bindir}/taler-exchange-helper-measure-tops-sms-check
%{_bindir}/taler-exchange-helper-measure-update-from-context
%{_bindir}/taler-exchange-helper-measure-validate-accepted-tos
%{_bindir}/taler-exchange-helper-sanctions-dummy
%{_bindir}/taler-exchange-kyc-challenger-email-converter
%{_bindir}/taler-exchange-kyc-challenger-postal-converter
%{_bindir}/taler-exchange-kyc-challenger-sms-converter
%{_bindir}/taler-exchange-kyc-trigger
%{_bindir}/taler-exchange-sanctionscheck
%{_libdir}/taler-exchange/*
# FIXME different package?
%{_bindir}/taler-terms-generator
%{_bindir}/taler-unified-setup.sh
%{_mandir}/man1/taler-exchange-aggregator*
%{_mandir}/man1/taler-exchange-closer*
%{_mandir}/man1/taler-exchange-drain*
%{_mandir}/man1/taler-exchange-expire*
%{_mandir}/man1/taler-exchange-httpd*
%{_mandir}/man1/taler-exchange-router*
%{_mandir}/man1/taler-exchange-secmod-eddsa*
%{_mandir}/man1/taler-exchange-secmod-rsa*
%{_mandir}/man1/taler-exchange-secmod-cs*
%{_mandir}/man1/taler-exchange-transfer*
%{_mandir}/man1/taler-exchange-wirewatch*
%{_mandir}/man1/taler-bank*
%{_mandir}/man1/taler-exchange-wire-gateway-client*
%{_mandir}/man1/taler-aggregator-benchmark.1.gz
%{_mandir}/man1/taler-exchange-dbconfig.1.gz
%{_mandir}/man1/taler-exchange-kyc-aml-pep-trigger.1.gz
%{_mandir}/man1/taler-exchange-kyc-trigger.1.gz
%{_mandir}/man1/taler-exchange-sanctionscheck.1.gz
%{_mandir}/man1/taler-fakebank-run.1.gz
%{_mandir}/man1/taler-terms-generator.1.gz
%{_mandir}/man1/taler-unified-setup.1.gz
%{_mandir}/man5/taler-exchange.conf.5.gz
%{_infodir}/taler-exchange*
%{_datadir}/taler-exchange/*
# FIXME configuration files in /etc/taler

%files -n libtalerexchange
# FIXME:  All this should eventually go into taler-base.
%{_bindir}/taler-exchange-config
%{_mandir}/man1/taler-exchange-config*
%{_libdir}/libtaler*.so.*

%files -n libtalerexchange-devel
# Benchmarks, only install them for the dev package.
%{_bindir}/taler-aggregator-benchmark
%{_bindir}/taler-exchange-benchmark
%{_bindir}/taler-fakebank-run
%{_bindir}/taler-bank-benchmark
%{_bindir}/taler-exchange-kyc-tester
# Man pages
%{_mandir}/man1/taler-exchange-kyc-tester*
# Headers
%{_includedir}/taler/*
# Plain .so symlinks
%{_libdir}/libtaler*.so
# Documentation
%{_mandir}/man1/taler-exchange-benchmark*
%{_infodir}/taler-developer-manual*

%files -n taler-auditor
%{_bindir}/taler-auditor-config
%{_bindir}/taler-auditor-dbinit
%{_bindir}/taler-auditor-dbconfig
%{_bindir}/taler-auditor-httpd
%{_bindir}/taler-auditor-offline
%{_bindir}/taler-auditor-sync
%{_bindir}/taler-helper-auditor-*
%{_libdir}/libauditor*
%{_datadir}/taler-auditor/*
%{_libdir}/libtalerauditordb*
%{_libdir}/taler-auditor/*
%{_mandir}/man1/taler-auditor*
%{_mandir}/man1/taler-helper-auditor*
%{_infodir}/taler-auditor*
# FIXME Configuration

%files database
%{_bindir}/taler-exchange-dbinit
%{_mandir}/man1/taler-exchange-dbinit.1.gz

%files offline
%{_bindir}/taler-exchange-offline
%{_mandir}/*/taler-exchange-offline*

#%files doc


%changelog
* Fri Oct 24 2025 Martin Schanzenbach - 1.0.0-1
- Packaged old release

* Thu May 18 2023 Martin Schanzenbach - 0.9.2-1
- Initial package
