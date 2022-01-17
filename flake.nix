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

          buildInputs = with pkgs; [ icu systemd openssl_3_0 pkg-config ];
        };

        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            icu systemd openssl_3_0 gcc gnumake pkg-config valgrind
          ];

          shellHook = ''
            export SYSTEMGMI_TLS_KEY=${sacrificial-cert}/privkey.pem
            export SYSTEMGMI_TLS_CERT=${sacrificial-cert}/tlscert.pem
          '';
        };
      }
    ) // {
      nixosModule = { config, pkgs, ... }: {
        options.services.systemgmi = {
          enable = pkgs.lib.mkOption {
            default = false;
            type = pkgs.lib.types.bool;
            description = ''
              Whether or not to run a gemini server for system monitoring.
            '';
          };

          user = pkgs.lib.mkOption {
            default = "nobody";
            type = pkgs.lib.types.str;
            description = ''
              The user to run the gemini server as.
            '';
          };
        };

        config = {
          systemd.services.systemgmi = pkgs.lib.mkIf config.services.systemgmi.enable (
            let deriv = self.defaultPackage."${pkgs.system}"; in {
              wantedBy = [ "multi-user.target" ];
              after = [ "network.target" "dbus.socket" ];
              description = "Run a gemini server for system monitoring.";
              serviceConfig = {
                Type = "exec";
                User = "${config.services.systemgmi.user}";
                ExecStart = "${deriv}/bin/systemgmi";
              };
          });

          networking.firewall.allowedTCPPorts =
            pkgs.lib.mkIf config.services.systemgmi.enable [ 1965 ];
        };
      };
    };
}
