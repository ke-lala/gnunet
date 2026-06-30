{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gnumake
    meson
    ninja
    pkg-config
    libsodium
    libtool
    libunistring
    libgcrypt
    sqlite
    curl
    libmicrohttpd
    zlib
    jansson
    gmp
    gnutls
    libidn2
  ];

  shellHook = ''
    echo "GNUnet environment loaded."
    export CC=gcc
    export CFLAGS="-O"
    '';
}
