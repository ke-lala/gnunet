```
# Install Deps
zypper install libtool autoconf make makeinfo gettext-tools gcc openssl \
               libgcrypt-devel libunistring-devel miniupnpc \
               libidn-devel zlib-devel libglpk40 libjansson-devel \
               libgnutls-devel libsqlite3-0 libmicrohttpd-devel \
               libopus-devel libpulse-devel libogg-devel sqlite3-devel \
               libzbar-devel libgnutls-dane-devel

# Install libextractor from the experimental repo
zypper addrepo https://download.opensuse.org/repositories/filesharing/openSUSE_Tumbleweed/filesharing.repo
zypper refresh
zypper install libextractor

# Set the repo priority to 100 so that it is deprioritized
zypper modifyrepo -p 100 filesharing
```
