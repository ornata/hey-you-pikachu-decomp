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
          ido = pkgs.stdenvNoCC.mkDerivation {
            pname = "ido-static-recomp";
            version = "5.3-v1.2";
            src = pkgs.fetchurl {
              url = "https://github.com/decompals/ido-static-recomp/releases/download/v1.2/ido-5.3-recomp-${if pkgs.stdenv.isDarwin then "macos" else "linux"}.tar.gz";
              hash = if pkgs.stdenv.isDarwin
                then "sha256-WxygBu5LFY/7oEIvw/ALmzMPkDbyebri6htHAzF62cA="
                else "sha256-q1x0FWH4CRPVjIsHR3HyOUGj7dMSUFqOvtbR3+tl5QY=";
            };
            unpackPhase = ''
              mkdir source
              tar -xf "$src" -C source
              cd source
            '';
            nativeBuildInputs = pkgs.lib.optional pkgs.stdenv.isLinux pkgs.autoPatchelfHook;
            buildInputs = pkgs.lib.optional pkgs.stdenv.isLinux pkgs.stdenv.cc.cc.lib;
            dontConfigure = true;
            dontBuild = true;
            dontStrip = true;
            # Leave the bundled MIPS/IRIX support objects untouched.
            dontPatchELF = true;
            dontAutoPatchelf = true;
            installPhase = ''
              mkdir -p "$out/libexec/ido"
              cp -R ./* "$out/libexec/ido/"
            '';
            postFixup = pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              for tool in cc acpp as0 as1 cfe copt c++filt ld strip ugen ujoin uld umerge uopt upas usplit; do
                autoPatchelf "$out/libexec/ido/$tool"
              done
            '';
          };
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
          development = pkgs.mkShellNoCC {
            packages = [ shellBash mipsTools pkgs.gnumake pkgs.python312 pkgs.uv
                         pkgs.git pkgs.curl pkgs.gnutar pkgs.gzip pkgs.coreutils pkgs.perl ];
            UV_PYTHON = "${pkgs.python312}/bin/python3";
            UV_PYTHON_DOWNLOADS = "never";
            IDO = "${ido}/libexec/ido/cc";
          };
        in {
          default = development;
          review = development.overrideAttrs (old: {
            nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [
              pkgs.clang pkgs.pkg-config pkgs.SDL2 pkgs.zlib pkgs.libpng
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.libGL ];
          });
          lint = development.overrideAttrs (old: {
            nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [ pkgs.llvmPackages.clang-tools ];
          });
        });
    };
}
