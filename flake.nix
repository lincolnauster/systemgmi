{
  inputs.flake-utils.url = "github:numtide/flake-utils";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/release-21.11";
  inputs.stlsc.url = "github:lincolnauster/stlsc";

  outputs = { self, flake-utils, nixpkgs, stlsc }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        sacrificial-cert = stlsc.defaultPackage.x86_64-linux;
        pkgs = import nixpkgs {
          system = system;
        };
      in {
        defaultPackage = pkgs.stdenv.mkDerivation {
          name = "systemgmi";
          version = "0.0.1";
          src = ./.;

          installPhase = ''
            mkdir -p $out/bin
            cp systemgmi $out/bin/
          '';

          dontConfigure = true;

          buildInputs = with pkgs; [ systemd openssl ];
        };

        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            systemd openssl_3_0 gcc gnumake valgrind
          ];

          shellHook = ''
            export SYSTEMGMI_TLS_KEY=${sacrificial-cert}/privkey.pem
            export SYSTEMGMI_TLS_CERT=${sacrificial-cert}/tlscert.pem
          '';
        };
      }
    );
}
