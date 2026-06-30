{
  inputs = {
    nixpkgs.url = "nixpkgs/release-26.05";
    systems.url = "github:nix-systems/default";
    self.submodules = true;
  };

  outputs = { self, nixpkgs, systems, ... } @ inputs:
    let
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];
      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forEachSystem (system: import nixpkgs { inherit system; });
    in
    {
      # This defines (installable) package derivations
      # For use in flakes that use this flake as input in order
      # to specify/use gnunet from git as a dependency
      packages = forEachSystem (system:
        let
          pkgs = nixpkgsFor.${system};
        in {
          gnunet = pkgs.stdenv.mkDerivation {
            name = "gnunet";
            src = ./.;
            nativeBuildInputs = [
              pkgs.gcc
              pkgs.gnumake
              pkgs.meson
              pkgs.ninja
              pkgs.pkg-config
              pkgs.libtool
              pkgs.git
             ];
            propagatedBuildInputs = [
              pkgs.libsodium
              pkgs.libtool
              pkgs.libunistring
              pkgs.libgcrypt
              pkgs.sqlite
              pkgs.curlWithGnuTls
              pkgs.libmicrohttpd
              pkgs.zlib
              pkgs.jansson
              pkgs.gmp
              pkgs.gnutls
              pkgs.libidn2
              pkgs.gettext
              pkgs.postgresql
              pkgs.jose
              pkgs.zbar
             ];
            preConfigure = ''
              ./bootstrap
            '';
          };
        }
      );
      defaultPackage = forEachSystem (system: self.packages.${system}.gnunet);
      # This defines a development shell in which you can compile
      # (and use) gnunet
       devShells = forEachSystem
        (system:
          let
            pkgs = nixpkgsFor.${system};
          in
          {
            default = pkgs.mkShell {
              packages = [
                pkgs.gcc
                pkgs.gnumake
                pkgs.meson
                pkgs.ninja
                pkgs.pkg-config
                pkgs.libsodium
                pkgs.libtool
                pkgs.libunistring
                pkgs.libgcrypt
                pkgs.sqlite
                pkgs.curlWithGnuTls
                pkgs.libmicrohttpd
                pkgs.zlib
                pkgs.jansson
                pkgs.gmp
                pkgs.gnutls
                pkgs.libidn2
                pkgs.codespell
                pkgs.clang-tools
                pkgs.gettext
                pkgs.postgresql
                pkgs.jose
                pkgs.zbar
                pkgs.uncrustify
                pkgs.syft
                pkgs.ngtcp2
                pkgs.nghttp3
              ];

              shellHook = ''
                echo "GNUnet environment loaded."
                export CC=gcc
                export CFLAGS="-O"
               '';
            };
          });
    };
}
