{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/release-21.11";
  inputs.stlsc.url = "github:lincolnauster/stlsc";

  outputs = { self, nixpkgs, stlsc }:
    let
      pkgs = import nixpkgs { system = "x86_64-linux"; };
      sacrificial-cert = stlsc.defaultPackage.x86_64-linux;
    in {
      defaultPackage.x86_64-linux = pkgs.stdenv.mkDerivation {
        name = "systemgmi";
        version = "0.0.1";
        src = ./.;

        installPhase = ''
          mkdir -p $out/bin
          cp systemgmi $out/bin/
        '';

        doConfigure = false;

        buildInputs = with pkgs; [ openssl ];
      };

      devShell.x86_64-linux = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [ valgrind openssl_3_0 gcc gnumake ];

        shellHook = ''
          export SYSTEMGMI_TLS_KEY=${sacrificial-cert}/privkey.pem
          export SYSTEMGMI_TLS_CERT=${sacrificial-cert}/tlscert.pem
        '';
      };
    };
}
