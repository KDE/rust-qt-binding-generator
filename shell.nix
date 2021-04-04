with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "rust-env";
  buildInputs = [
    rustc
    cargo
    rustfmt
    qt5.qtquickcontrols cmake ninja openssl pkgconfig
    valgrind gdb
  ];

  # Set Environment Variables
  RUST_BACKTRACE = 1;
}
