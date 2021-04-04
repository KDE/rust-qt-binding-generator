with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "rust-env";
  buildInputs = [
    rustc
    cargo
    rustfmt
    cmake ninja openssl pkgconfig
    extra-cmake-modules
    qt5.qtcharts
    qt5.qtquickcontrols
    qt5.qtquickcontrols2
    libsForQt5.kirigami2
    valgrind gdb
  ];

  # Set Environment Variables
  RUST_BACKTRACE = 1;
}
