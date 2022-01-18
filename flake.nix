{
  description = "Binding generator to combine Qt and Rust code";
  inputs = {
    utils.url = "github:numtide/flake-utils";
    naersk.url = "github:nix-community/naersk";
    naersk.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, utils, naersk }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages."${system}";
        naersk-lib = naersk.lib."${system}";
        inputs = with pkgs; [
          cmake
          extra-cmake-modules
          libsForQt5.kirigami2
          ninja
          openssl
          pkgconfig
          qt5.qtcharts
          qt5.qtquickcontrols
          qt5.qtquickcontrols2
          qt5.wrapQtAppsHook
        ];
      in
      rec {
        # `nix build`
        packages.rust_qt_binding_generator = naersk-lib.buildPackage {
          pname = "rust_qt_binding_generator";
          root = ./.;
          nativeBuildInputs = inputs;
        };
        defaultPackage = packages.rust_qt_binding_generator;

        # `nix run`
        apps.rust_qt_binding_generator = utils.lib.mkApp {
          drv = packages.rust_qt_binding_generator;
        };
        defaultApp = apps.rust_qt_binding_generator;

        # `nix develop`
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cargo
            clippy
            gdb
            rust-analyzer
            rustc
            rustfmt
            valgrind
          ] ++ inputs;
        };
      });
}

