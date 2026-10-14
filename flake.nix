{
  description = "Pinned development tools for the matching Hey You, Pikachu! build";

  inputs.nixpkgs.url = "https://channels.nixos.org/nixos-26.05/nixexprs.tar.xz";

  outputs = { self, nixpkgs }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "x86_64-linux" ];
      forSystems = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          shellBash = pkgs.bashInteractive.overrideAttrs (old:
            pkgs.lib.optionalAttrs pkgs.stdenv.isDarwin {
              # Darwin can block writes above PIPE_BUF before the heredoc reader starts.
              env = (old.env or { }) // {
                NIX_CFLAGS_COMPILE = (old.env.NIX_CFLAGS_COMPILE or "")
                  + " -DHEREDOC_PIPESIZE=512";
              };
              preConfigure = (old.preConfigure or "") + ''
                export SHELL=${pkgs.dash}/bin/dash
                export CONFIG_SHELL="$SHELL"
                configureScript="$CONFIG_SHELL ./configure"
              '';
            });
          # Native executables targeting big-endian MIPS, without libc/linker wrappers.
          bintools = pkgs.pkgsCross.mips-linux-gnu.buildPackages.binutils-unwrapped.overrideAttrs (old: pkgs.lib.optionalAttrs pkgs.stdenv.isDarwin {
            # Autoconf's shell probes stall with Bash 5.3 on this Darwin builder.
            preConfigure = (old.preConfigure or "") + ''
              export SHELL=${pkgs.dash}/bin/dash
              export CONFIG_SHELL="$SHELL"
              configureScript="$CONFIG_SHELL $configureScript"
            '';
          });
          mipsTools = pkgs.runCommand "pikachu-mips-binutils" { } ''
            mkdir -p "$out/bin"
            for tool in as ld objcopy objdump nm readelf strip ar ranlib size strings; do
              ln -s ${bintools}/bin/${bintools.targetPrefix}$tool "$out/bin/mips-linux-gnu-$tool"
            done
          '';
        in {
          default = pkgs.mkShellNoCC {
            packages = [ shellBash mipsTools pkgs.gnumake pkgs.python312 pkgs.uv
                         pkgs.git pkgs.curl pkgs.gnutar pkgs.gzip pkgs.coreutils pkgs.perl ];
            UV_PYTHON = "${pkgs.python312}/bin/python3";
            UV_PYTHON_DOWNLOADS = "never";
          };
        });
    };
}
