# systemgmi
low-configuration systemd monitoring over gemini.

## Overview
systemgmi is a tool for remote server monitoring written in C. You can think of
it as sort of like `systemctl status` served over Gemini. It latches on to the
configuration of systemd, and, as such, requires no configuration of its own
outside of a TLS certificate and a few environment variables.

## Goals & Scope
Systemgmi should slot into any server environment with very little effort (or,
ideally, no effort at all). If people are doing crazy things with this, it's
built incorrectly. Working around its environment, systemgmi should never crash
or error out at runtime (excluding startup checks) and never exhibit unexpected
behavior. Some environment variables and `systemctl enable` is all that should
be required to have an instance of systemgmi in *any case*.

## Usage
Enable/start the systemgmi service, giving it a TLS certificate in environment
variables (see `--help` for which env vars exactly). If you navigate to your IP,
you'll see systegmi's index. It contains the following:

+ the hostname
+ a little neofetch run (if neofetch is in PATH)
+ [WIP] a link to the system journal (if permissions are present)
+ [WIP] a link to a streaming version of the journal (if permissions are present)
+ a list of units

If you follow the link for a unit, you'll see information roughly equivalent to
running `systemctl status <unit>`: is it running, how much RAM is it using, etc.
If systemgmi is running as a user with the proper permissions, you can also view
the journal for that unit.

### NixOS
If your server is running NixOS and supports Flakes, then you can wire this
repository up as an input and enable it with `services.systemgmi.enable = true'.
See the below for a complete (though untested) example of a flake.nix.

```nix
{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/-nixos-21.11";
  inputs.systemgmi.url = "github:lincolnauster/systemgmi";

  outputs = { self, nixpkgs, systemgmi }: {
    nixosConfigurations."<hostname>" = nixpkgs.lib.nixosSystem {
      # ... all the standard boilerplate ...

      modules = [
        systemgmi.nixosModule

        ({
          services.systemgmi.enable = true;
          services.systemgmi.user = "myaccount";
        })
      ];
    };
  }
}
```

## Contributing
Do not be afraid to deform systemgmi beyond recognition! And, if you'd like,
definitely do not be afraid to contribute those changes (even highly opinionated
ones) upstream! If you have Nix installed already and want a quick development
environment, `nix develop` will place you in a shell with a completely working
build and test environment including libraries and down to a sacrificial TLS
certificate pointed to by the proper environment variables. The PEM password is
"password".

If you want a style guide, I roughly adhere to Suckless' style: function
declarations are split over three lines, use 8-character tabs for indentation
and spaces for alignment, etc. The only part of that which is actually important
is that headers are not to be included by other headers. Only individual
compilation units may `#include` things. Other than that, just make a good faith
effort to keep the code you write homogeneous with what's already there. I find
this style keeps individual units nice and isolated and encapsulated, which is
particularly helpful when working with 'heavy' bindings like that of dbus and
OpenSSL.

If you want something to work on, you can `git grep` for `TODO`s in the code
base, or take a look at this list of things I haven't gotten to yet and/or fully
considered:

+ Error messages and handling isn't great. I haven't really tested the edge
  cases, and I'd expect safety issues and a variety of problems recovering debug
  information (i.e., `errno`).
+ Systemgmi can't be bound to listen on any IP other than 0.0.0.0. It would make
  sense to refactor this into an environment variable.
+ Systemgmi can't control processes. Adding some links to start, restart, stop,
  enable, and disable units (maybe with access limited to a client-side cert)
  could be a good idea. Maybe.
+ Logging has no unified approach. Systemgmi just sort of logs whatever it feels
  like without considering why. Someone help.
+ Systemgmi doesn't quite have a pedantic interpretation of the draft spec. This
  is fine in practice, but if any purists want to give the implementation a look
  and go over any rough spots, it would be greatly appreciated.
+ Systemgmi gives little insight into and control of itself. Maybe at some point
  an admin console providing access to things like cache management, etc, could
  be useful.

## Building
On systems that support Nix Flakes, simply `nix build` a checkout of the source
tree. On systems with only the traditional toolchain, systemgmi is tested on and
known to be supported by GCC and GNU Make. It depends upon OpenSSL 3.0.1,
systemd 249.7 (the library, not the init system), and ICU 70.1. While untested,
other versions of these dependencies will, in high probability, work just as
well: systemgmi isn't doing anything too fancy with any of them.

## Licensing
systemgmi is Free Software which you may distribute under the terms of the GNU
Affero General Public License as published by the FSF and distributed in the
LICENSE file at the root of this source tree.

In addition, as a special exception, the copyright holders provide permission to
link the code residing in this file tree with the OpenSSL library. You must obey
the GNU Affero General Public License in all respects for all code used other
than OpenSSL. If you modify file(s) with this exception, may extend this
exception to your file(s), but you are not obligated to do so. If you do not
wish to do so, remove these paragraphs declaring the exception from your
version.

## Conforming To
+ C17 (current minimum C11)
+ POSIX.1-2008
