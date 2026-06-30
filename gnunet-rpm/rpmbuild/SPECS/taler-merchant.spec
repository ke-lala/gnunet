Name:           taler-merchant
Version:        0.13.0
Release:        1%{?dist}
Summary:        GNU's payment system merchant backend.
License:        AGPL-3.0-or-later
URL:            https://taler.net
%global         _disable_source_fetch 0
Source0:        https://ftpmirror.gnu.org/gnu/taler/%{name}-%{version}.tar.gz
Source1:        https://ftpmirror.gnu.org/gnu/taler/%{name}-%{version}.tar.gz.sig
Source2:        https://grothoff.org/christian/grothoff.asc
%global         SHA512SUM0 23ee2121156a4a6e1c6001807334f66f8dc45b0ea78ff045b055c014ab3a4b448cd01892f1f36728faacc542bbeef723e10a64ea42af5325d8facfef2e6902e0
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
BuildRequires:  qrencode-devel
BuildRequires:  gnupg2
Recommends: postgresql-server
BuildRequires:  libmicrohttpd-devel
BuildRequires:  libgnunetpq-devel
BuildRequires:  libtalerexchange-devel
BuildRequires:  python3-jinja2

%description
GNU's payment system merchant backend.
The GNU Taler merchant backend provides e-commerce
applications with a simple RESTful API to integrate
payments using GNU Taler. This package provides the
merchant backend.

%package -n libtalermerchant
Summary: Libraries to talk to a GNU Taler merchant.
%description -n libtalermerchant
This package contains the development files for libtalermerchant.
Libraries to talk to a GNU Taler merchant.
The GNU Taler merchant backend provides e-commerce
applications with a simple RESTful API to integrate
payments using GNU Taler. This C library implements
a client-side version of that REST API.  Please note
that the core API is pretty simple, so applications
may legitimately choose to implement the API directly
instead of using this wrapper.

%package -n libtalermerchant-devel
Requires: libtalermerchant%{?_isa} = %{version}-%{release}
Summary: Libraries to talk to a GNU Taler merchant (development).
%description -n libtalermerchant-devel
This package contains the development files for libtalermerchant.

%prep
echo "%SHA512SUM0 %SOURCE0" | sha512sum -c -
#%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'

%autosetup

%build
%configure
%{make_build}

%install
make install DESTDIR=%{buildroot}
#install -Dm644 "contrib/packages/fedora/%{name}.service" "%{buildroot}/%{_unitdir}/%{name}.service"
#install -Dm644 "contrib/packages/fedora/%{name}-user.service" "%{buildroot}/%{_userunitdir}/%{name}-user.service"
#install -Dm644 "contrib/packages/fedora/gnunet-system.conf" %{buildroot}/%{_sysconfdir}/gnunet.conf
rm -f %{buildroot}%{_infodir}/dir

%check
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
%{_libdir}/libtalermerchantdb.so.*
%{_mandir}/man1/*
%{_bindir}/taler-merchant*
%{_infodir}/taler-merchant*
%{_datadir}/taler/merchant/*
%{_datadir}/taler/sql/merchant/*
%{_datadir}/taler/config.d/*
%{_libdir}/taler/libtaler_plugin_merchantdb_postgres.so

%files -n libtalermerchant
%{_libdir}/libtalermerchant.so.*

%files -n libtalermerchant-devel
%{_libdir}/libtalermerchant.so
%{_libdir}/libtalermerchanttesting.so*
%{_includedir}/taler/taler_merchant*.h
%{_libdir}/libtalermerchantdb.so

%changelog
* Thu May 18 2023 Martin Schanzenbach - 0.9.2-1
- Initial package
